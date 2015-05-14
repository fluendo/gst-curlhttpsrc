#ifndef GSTCURLMULTICONTEXT_H_
#define GSTCURLMULTICONTEXT_H_

#include "gst-compat.h"
#include "gst-demo.h"
#include "gst-fluendo.h"

#include "gstcurlhttpsrc.h"
#include "gstcurldefaults.h"

typedef struct _GstCurlMultiContext GstCurlMultiContext;

struct _GstCurlMultiContext
{
  GstTask     *task;
#if GST_CHECK_VERSION(1,0,0)
  GRecMutex   task_rec_mutex;
#else
  GStaticRecMutex task_rec_mutex;
#endif
  GMutex      mutex;
  guint       refcount;
  GCond       signal;

  /* < private > */
  CURLM *multi_handle;
  int sources;
};

void gst_curl_multi_context_ref (GstCurlMultiContext * thiz);
void gst_curl_multi_context_unref (GstCurlMultiContext * thiz);
void gst_curl_multi_context_stop (GstCurlMultiContext * thiz);
void gst_curl_multi_context_add_source (GstCurlMultiContext * thiz, GstCurlHttpSrc * client);
void gst_curl_multi_context_remove_source (GstCurlMultiContext * thiz, GstCurlHttpSrc * client);

#endif
