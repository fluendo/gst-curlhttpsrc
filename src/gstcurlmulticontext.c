#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "gstcurlmulticontext.h"

GST_DEBUG_CATEGORY_EXTERN (gst_curl_multi_debug);
#define GST_CAT_DEFAULT gst_curl_multi_debug

static void
gst_curl_multi_context_loop (gpointer thread_data)
{
  GstCurlMultiContext* thiz;
  int i, still_running;
  gboolean cond = FALSE;
  CURLMsg *curl_message;

  thiz = (GstCurlMultiContext *) thread_data;

  while (1) {
    struct timeval timeout;
    gint rc;
    fd_set fdread, fdwrite, fdexcep;
    int maxfd = -1;
    long curl_timeo = -1;

    g_mutex_lock (&thiz->mutex);
    /* Someone is holding a reference to us, but isn't using us so to avoid
     * unnecessary clock cycle wasting, sit in a conditional wait until woken.
     */
    while (thiz->sources == 0) {
      GST_DEBUG ("Entering wait state...");
      g_cond_wait (&thiz->signal, &thiz->mutex);
      GST_DEBUG ("Received wake up call!");
    }

    /* Because curl can possibly take some time here, be nice and let go of the
     * mutex so other threads can perform state/queue operations as we don't
     * care about those until the end of this. */
    g_mutex_unlock(&thiz->mutex);

    FD_ZERO (&fdread);
    FD_ZERO (&fdwrite);
    FD_ZERO (&fdexcep);

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    curl_multi_timeout (thiz->multi_handle, &curl_timeo);
    if (curl_timeo >= 0) {
    timeout.tv_sec = curl_timeo / 1000;
      if (timeout.tv_sec > 1) {
        timeout.tv_sec = 1;
      }
      else {
        timeout.tv_usec = (curl_timeo % 1000) * 1000;
      }
    }

    /* get file descriptors from the transfers */
    curl_multi_fdset (thiz->multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);

    rc = select (maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);

    switch (rc) {
    case -1:
      /* select error */
      break;
    case 0:
    default:
      /* timeout or readable/writable sockets */
      curl_multi_perform (thiz->multi_handle, &still_running);
      break;
    }

    /*
     * Check the CURL message buffer to find out if any transfers have
     * completed. If they have, call the signal_finished function which
     * will signal the g_cond_wait call in that calling instance.
     */
    i = 0;
    while (cond != TRUE) {
      curl_message = curl_multi_info_read (thiz->multi_handle, &i);
      if (curl_message == NULL) {
        cond = TRUE;
      }
      else if (curl_message->msg == CURLMSG_DONE) {
        /* A hack, but I have seen curl_message->easy_handle being
         * NULL randomly, so check for that. */
	g_mutex_lock(&thiz->mutex);
        if (curl_message->easy_handle == NULL) {
          break;
        }
#if 0
        curl_multi_remove_handle (thiz->multi_handle,
                                  curl_message->easy_handle);
        gst_curl_http_src_remove_queue_handle(&thiz->queue,
                                            curl_message->easy_handle);
#endif
        g_mutex_unlock(&thiz->mutex);
      }
    }

    if (still_running == 0) {
      /* We've finished processing, so set the state to wait.
       *
       * This is a little more complex, as we need to catch the edge
       * case of another thread adding a queue item while we've been
       * working.
       */
      g_mutex_lock (&thiz->mutex);
      thiz->sources = 0;
      g_mutex_unlock (&thiz->mutex);
    }
  }
}

void
gst_curl_multi_context_ref (GstCurlMultiContext * thiz)
{
  g_mutex_lock (&thiz->mutex);
  if (thiz->refcount == 0) {
    /* Set up various in-task properties */

    /* set up curl */
    thiz->multi_handle = curl_multi_init ();

    curl_multi_setopt (thiz->multi_handle,
                       CURLMOPT_PIPELINING, 1);
#ifdef CURLMOPT_MAX_HOST_CONNECTIONS
    curl_multi_setopt (thiz->multi_handle,
                       CURLMOPT_MAX_HOST_CONNECTIONS, 1);
#endif

    /* Start the thread */
#if GST_CHECK_VERSION(1,0,0)
    thiz->task = gst_task_new (
            (GstTaskFunction) gst_curl_multi_context_loop,
            (gpointer) thiz, NULL);
#else
    thiz->task = gst_task_create (
            (GstTaskFunction) gst_curl_multi_context_loop,
            (gpointer) thiz);
#endif
    gst_task_set_lock (thiz->task,
                       &thiz->task_rec_mutex);
    if (gst_task_start (thiz->task) == FALSE) {
      /*
       * This is a pretty critical failure and is not recoverable, so commit
       * sudoku and run away.
       */
      GST_ERROR ("Couldn't start curl_multi task! Aborting.");
      abort ();
    }
    GST_INFO ("Curl multi loop has been correctly initialised!");
  }
  thiz->refcount++;
  g_mutex_unlock (&thiz->mutex);
}

/*
 * Decrement the reference count on the curl multi loop. If this is called by
 * the last instance to hold a reference, shut down the worker. (Otherwise
 * GStreamer can't close down with a thread still running). Also offers the
 * "force_all" boolean parameter, which if TRUE removes all references and shuts
 * down.
 */
void
gst_curl_multi_context_unref (GstCurlMultiContext * thiz)
{
  g_mutex_lock(&thiz->mutex);
  thiz->refcount--;
  GST_INFO ("Worker thread refcount is now %u", thiz->refcount);

  if (thiz->refcount <= 0) {
    /* Everything's done! Clean up. */
    gst_task_pause (thiz->task);
    g_cond_signal (&thiz->signal);
    g_mutex_unlock (&thiz->mutex);
    gst_task_join (thiz->task);
  } else {
    g_mutex_unlock(&thiz->mutex);
  }
}

/* Is the following even necessary any more...? */
void
gst_curl_multi_context_stop (GstCurlMultiContext * thiz)
{
  /* Something wants us to shut down, so best to do a full cleanup as it
   * might be that something's gone bang.
   */
  g_mutex_lock (&thiz->mutex);
  /*gst_curl_http_src_unref_multi (NULL, GSTCURL_RETURN_PIPELINE_NULL, TRUE);*/
  GST_INFO ("Got instruction to shut down");
  g_mutex_unlock (&thiz->mutex);
}

void
gst_curl_multi_context_add_source (GstCurlMultiContext * thiz, GstCurlHttpSrc * client)
{
  GST_DEBUG ("Adding easy handle for URI %s", client->uri);
  g_mutex_lock (&thiz->mutex);
  curl_multi_add_handle (thiz->multi_handle, client->curl_handle);
  thiz->sources++;
  g_cond_signal (&thiz->signal);
  g_mutex_unlock (&thiz->mutex);
}


void
gst_curl_multi_context_remove_source (GstCurlMultiContext * thiz, GstCurlHttpSrc * client)
{
  g_mutex_lock (&thiz->mutex);
  curl_multi_remove_handle (thiz->multi_handle, client->curl_handle);
  g_mutex_unlock (&thiz->mutex);
}
