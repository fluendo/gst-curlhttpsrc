#ifndef GSTCURLMULTICONTEXT_H_
#define GSTCURLMULTICONTEXT_H_

#include "gst-compat.h"
#include "gst-demo.h"
#include "gst-fluendo.h"

#include <curl/curl.h>

#include "gstcurldefaults.h"

typedef enum _GstCurlMultiContextSourceStatus GstCurlMultiContextSourceStatus;
typedef struct _GstCurlMultiContextSource GstCurlMultiContextSource;
typedef struct _GstCurlMultiContext GstCurlMultiContext;

enum _GstCurlMultiContextSourceStatus
{
  GST_CURL_MULTI_CONTEXT_SOURCE_STATUS_OK,
  GST_CURL_MULTI_CONTEXT_SOURCE_STATUS_ERROR,
};

struct _GstCurlMultiContextSource
{
  GMutex mutex;
  GCond signal;
  CURL *easy_handle;
  /* where to store the bytes */
  GstAdapter *adapter;
  /* the element request a cancel on the handle */
  gboolean cancel;
  /* the done message has been received */
  gboolean done;
  /* the status once the source has ended */
  GstCurlMultiContextSourceStatus status;
};

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
void gst_curl_multi_context_add_source (GstCurlMultiContext * thiz, CURL * handle);

void gst_curl_multi_context_source_cancel (GstCurlMultiContextSource * source);

#endif
