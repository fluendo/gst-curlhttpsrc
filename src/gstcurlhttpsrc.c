/*
 * GstCurlHttpSrc
 * Copyright 2014 British Broadcasting Corporation - Research and Development
 *
 * Author: Sam Hurst <samuelh@rd.bbc.co.uk>
 *
 * Based on the GstElement template, courtesy of
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-curlhttpsrc
 *
 * This plugin reads data from a remote location specified by a URI, when the
 * protocol is 'http' or 'https'.
 *
 * It is based on the cURL project (http://curl.haxx.se/) and is specifically
 * designed to be also used with nghttp2 (http://nghttp2.org) to enable HTTP/2
 * support for GStreamer. Your libcurl library MUST be compiled against nghttp2
 * for HTTP/2 support for this functionality. HTTPS support is dependent on
 * cURL being built with SSL support (OpenSSL/PolarSSL/NSS/GnuTLS).
 *
 * An HTTP proxy must be specified by URL.
 * If the "http_proxy" environment variable is set, its value is used.
 * The #GstCurlHttpSrc:proxy property can be used to override the default.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 curlhttpsrc location=http://127.0.1.1/index.html ! fakesink dump=1
 * ]| The above pipeline reads a web page from the local machine using HTTP and
 * dumps it to stdout.
 * |[
 * gst-launch-1.0 playbin uri=http://rdmedia.bbc.co.uk/dash/testmpds/multiperiod/bbb.php
 * ]| The above pipeline will start up a DASH streaming session from the given
 * MPD file. This requires GStreamer to have been built with dashdemux from
 * gst-plugins-bad.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "gstcurlhttpsrc.h"
#include "gstcurldefaults.h"
#include "gstcurlqueue.h"

GST_DEBUG_CATEGORY_STATIC (gst_curl_http_src_debug);
#define GST_CAT_DEFAULT gst_curl_http_src_debug
GST_DEBUG_CATEGORY_STATIC (gst_curl_loop_debug);

#define GSTCURL_ERROR_PRINT(...) GST_CAT_ERROR (gst_curl_loop_debug, __VA_ARGS__)
#define GSTCURL_WARNING_PRINT(...) GST_CAT_WARNING (gst_curl_loop_debug, __VA_ARGS__)
#define GSTCURL_INFO_PRINT(...) GST_CAT_INFO (gst_curl_loop_debug, __VA_ARGS__)
#define GSTCURL_DEBUG_PRINT(...) GST_CAT_DEBUG (gst_curl_loop_debug, __VA_ARGS__)
#define GSTCURL_TRACE_PRINT(...) GST_CAT_TRACE (gst_curl_loop_debug, __VA_ARGS__)

#define gst_curl_setopt_str(s,handle,type,option) \
  if(option != NULL) { \
    if(curl_easy_setopt(handle,type,option) != CURLE_OK) { \
      GST_WARNING_OBJECT (s, "Cannot set unsupported option %s", #type ); \
    } \
  } \

#define gst_curl_setopt_int(s,handle, type, option) \
  if((option >= GSTCURL_HANDLE_MIN_##type) && (option <= GSTCURL_HANDLE_MAX_##type)) { \
    if(curl_easy_setopt(handle,type,option) != CURLE_OK) { \
      GST_WARNING_OBJECT (s, "Cannot set unsupported option %s", #type ); \
    } \
  } \

#define gst_curl_setopt_str_default(s,handle,type,option) \
  if((option == NULL) && (GSTCURL_HANDLE_DEFAULT_##type != NULL)) { \
    if(curl_easy_setopt(handle,type,GSTCURL_HANDLE_DEFAULT_##type) != CURLE_OK) { \
      GST_WARNING_OBJECT(s, "Cannot set unsupported option %s,", #type ); \
    } \
  } \
  else { \
    if(curl_easy_setopt(handle,type,option) != CURLE_OK) { \
      GST_WARNING_OBJECT (s, "Cannot set unsupported option %s", #type ); \
    } \
  } \

#define gst_curl_setopt_int_default(s,handle,type,option) \
  if((option < GSTCURL_HANDLE_MIN_##type) || (option > GSTCURL_HANDLE_MAX_##type)) { \
    GST_WARNING_OBJECT(s, "Value of %ld out of acceptable range for %s", option, \
                       #type ); \
    if(curl_easy_setopt(handle,type,GSTCURL_HANDLE_DEFAULT_##type) != CURLE_OK) { \
      GST_WARNING_OBJECT(s, "Cannot set unsupported option %s,", #type ); \
    } \
  } \
  else { \
    if(curl_easy_setopt(handle,type,option) != CURLE_OK) { \
      GST_WARNING_OBJECT (s, "Cannot set unsupported option %s", #type ); \
    } \
  } \


/* As gboolean is either 0x0 or 0xffffffff, this sanitises things for curl. */
#define GSTCURL_BINARYBOOL(x) ((x != 0)?1:0)


/*
 * Make a source pad template to be able to kick out recv'd data
 */
static GstStaticPadTemplate srcpadtemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

/*
 * Function Definitions
 */
/* Gstreamer generic element functions */
static void gst_curl_http_src_class_init (GstCurlHttpSrcClass * klass);
static void gst_curl_http_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_curl_http_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_curl_http_src_init (GstCurlHttpSrc * source);
static void gst_curl_http_src_ref_multi (GstCurlHttpSrc *src);
static void gst_curl_http_src_unref_multi (GstCurlHttpSrc *src);
static void gst_curl_http_src_finalize (GObject *obj);
static GstFlowReturn gst_curl_http_src_create (GstPushSrc * psrc,
    GstBuffer ** outbuf);
static GstFlowReturn
gst_curl_http_src_handle_response (GstCurlHttpSrc * src, GstBuffer ** buf);
static gboolean gst_curl_http_src_negotiate_caps (GstCurlHttpSrc * src);
static GstStateChangeReturn gst_curl_http_src_change_state (GstElement *
    element, GstStateChange transition);
static void gst_curl_http_src_cleanup_instance(GstCurlHttpSrc *src);
static gboolean gst_curl_http_src_query (GstBaseSrc * bsrc, GstQuery * query);
static gboolean gst_curl_http_src_get_content_length (GstBaseSrc * bsrc,
    guint64 * size);

/* GstTask functions */
static void gst_curl_http_src_curl_multi_loop (gpointer thread_data);
static CURL *gst_curl_http_src_create_easy_handle (GstCurlHttpSrc * s);
static gboolean gst_curl_http_src_make_request (GstCurlHttpSrc * s);
static inline void gst_curl_http_src_destroy_easy_handle (GstCurlHttpSrc * src);
static size_t gst_curl_http_src_get_header (void *header, size_t size,
    size_t nmemb, void * src);
static size_t gst_curl_http_src_get_chunks (void *chunk, size_t size,
    size_t nmemb, void * src);
static void gst_curl_http_src_request_remove (GstCurlHttpSrc * src);
static char *gst_curl_http_src_strcasestr (const char *haystack,
    const char *needle);

#define gst_curl_http_src_parent_class parent_class

/*----------------------------------------------------------------------------*
 *                            The URI interface                               *
 *----------------------------------------------------------------------------*/
static GstURIType
#if GST_CHECK_VERSION(1,0,0)
gst_curl_http_src_urihandler_get_type (GType type)
#else
gst_curl_http_src_urihandler_get_type (void)
#endif
{
  return GST_URI_SRC;
}

#if GST_CHECK_VERSION(1,0,0)
static const gchar *const *
gst_curl_http_src_urihandler_get_protocols (GType type)
{
  static const gchar *protocols[] = { PROTOCOLS, NULL };

  return protocols;
}

#else
static gchar **
gst_curl_http_src_urihandler_get_protocols (void)
{
  static gchar *protocols[] = { PROTOCOLS, NULL };

  return protocols;
}
#endif

#if GST_CHECK_VERSION(1,0,0)
static gchar *
#else
static const gchar *
#endif
gst_curl_http_src_urihandler_get_uri (GstURIHandler * handler)
{
  gchar* ret;
  GstCurlHttpSrc *source;

  g_return_val_if_fail (GST_IS_URI_HANDLER (handler), FALSE);
  source = GST_CURLHTTPSRC (handler);

  GSTCURL_FUNCTION_ENTRY (source);

  g_mutex_lock(source->uri_mutex);
#if GST_CHECK_VERSION(1,0,0)
  ret = g_strdup (source->uri);
#else
  ret = source->uri;
#endif
  g_mutex_unlock(source->uri_mutex);

  GSTCURL_FUNCTION_EXIT (source);
  return ret;
}

static gboolean
#if GST_CHECK_VERSION(1,0,0)
gst_curl_http_src_urihandler_set_uri (GstURIHandler * handler,
    const gchar * uri, GError ** error)
#else
gst_curl_http_src_urihandler_set_uri (GstURIHandler * handler,
    const gchar * uri)
#endif
{
  GstCurlHttpSrc *source = GST_CURLHTTPSRC (handler);
  GSTCURL_FUNCTION_ENTRY (source);

  g_return_val_if_fail (GST_IS_URI_HANDLER (handler), FALSE);
  g_return_val_if_fail (uri != NULL, FALSE);

  g_mutex_lock(source->uri_mutex);

  if (source->uri != NULL) {
    GST_DEBUG_OBJECT (source,
        "URI already present as %s, updating to new URI %s", source->uri, uri);
    g_free (source->uri);
    source->end_of_message = FALSE;
  }

  source->uri = g_strdup (uri);
  if (source->uri == NULL) {
    return FALSE;
  }

  g_mutex_unlock(source->uri_mutex);

  GSTCURL_FUNCTION_EXIT (source);
  return TRUE;
}

static void
gst_curl_http_src_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *uri_iface = (GstURIHandlerInterface *) g_iface;

  uri_iface->get_type = gst_curl_http_src_urihandler_get_type;
  uri_iface->get_protocols = gst_curl_http_src_urihandler_get_protocols;
  uri_iface->get_uri = gst_curl_http_src_urihandler_get_uri;
  uri_iface->set_uri = gst_curl_http_src_urihandler_set_uri;
}

/*----------------------------------------------------------------------------*
 *                              The src class                                 *
 *----------------------------------------------------------------------------*/
#if GST_CHECK_VERSION(1,0,0)

G_DEFINE_TYPE_WITH_CODE (GstCurlHttpSrc, gst_curl_http_src, GST_TYPE_PUSH_SRC,
    G_IMPLEMENT_INTERFACE (GST_TYPE_URI_HANDLER,
        gst_curl_http_src_uri_handler_init));

#else

static gboolean
gst_curl_http_src_interface_supported (GstImplementsInterface * iface, GType type)
{
  if (type == GST_TYPE_URI_HANDLER)
    return TRUE;
  else
    return FALSE;
}

static void
gst_curl_http_src_interface_init (GstImplementsInterfaceClass * klass)
{
  klass->supported = gst_curl_http_src_interface_supported;
}

G_DEFINE_TYPE_WITH_CODE (GstCurlHttpSrc, gst_curl_http_src, GST_TYPE_PUSH_SRC,
    G_IMPLEMENT_INTERFACE (GST_TYPE_IMPLEMENTS_INTERFACE,
        gst_curl_http_src_interface_init);
    G_IMPLEMENT_INTERFACE (GST_TYPE_URI_HANDLER,
        gst_curl_http_src_uri_handler_init));

#endif

static void
gst_curl_http_src_class_init (GstCurlHttpSrcClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSrcClass *gstbasesrc_class;
  GstPushSrcClass *gstpushsrc_class;
  const gchar *http_env;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasesrc_class = (GstBaseSrcClass *) klass;
  gstpushsrc_class = (GstPushSrcClass *) klass;

  GST_INFO_OBJECT (klass, "class_init started!");

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_curl_http_src_change_state);
  gstpushsrc_class->create = GST_DEBUG_FUNCPTR (gst_curl_http_src_create);
  gstbasesrc_class->query = GST_DEBUG_FUNCPTR (gst_curl_http_src_query);
  gstbasesrc_class->get_size =
      GST_DEBUG_FUNCPTR (gst_curl_http_src_get_content_length);

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&srcpadtemplate));

  gst_curl_http_src_curl_capabilities = curl_version_info (CURLVERSION_NOW);
  http_env = g_getenv ("GST_CURL_HTTP_VER");
  if (http_env != NULL) {
    pref_http_ver = (gfloat) g_ascii_strtod (http_env, NULL);
    GST_INFO_OBJECT (klass, "Seen env var GST_CURL_HTTP_VER with value %.1f",
        pref_http_ver);
  }
  else {
    pref_http_ver = GSTCURL_HANDLE_DEFAULT_CURLOPT_HTTP_VERSION;
  }

  gst_curl_http_src_default_useragent =
      g_strdup_printf("GStreamer curlhttpsrc libcurl/%s",
                      gst_curl_http_src_curl_capabilities->version);

  gobject_class->set_property = gst_curl_http_src_set_property;
  gobject_class->get_property = gst_curl_http_src_get_property;
  gobject_class->finalize = gst_curl_http_src_finalize;

  g_object_class_install_property (gobject_class, PROP_URI,
      g_param_spec_string ("location", "Location", "URI of resource to read",
          GSTCURL_HANDLE_DEFAULT_CURLOPT_URL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_USERNAME,
      g_param_spec_string ("user-id", "user-id",
          "HTTP location URI user id for authentication",
          GSTCURL_HANDLE_DEFAULT_CURLOPT_USERNAME,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PASSWORD,
      g_param_spec_string ("user-pw", "user-pw",
          "HTTP location URI password for authentication",
          GSTCURL_HANDLE_DEFAULT_CURLOPT_PASSWORD,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PROXYURI,
      g_param_spec_string ("proxy", "Proxy", "URI of HTTP proxy server",
          GSTCURL_HANDLE_DEFAULT_CURLOPT_PROXY,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PROXYUSERNAME,
      g_param_spec_string ("proxy-id", "proxy-id",
          "HTTP proxy URI user id for authentication",
          GSTCURL_HANDLE_DEFAULT_CURLOPT_PROXYUSERNAME,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PROXYPASSWORD,
      g_param_spec_string ("proxy-pw", "proxy-pw",
          "HTTP proxy URI password for authentication",
          GSTCURL_HANDLE_DEFAULT_CURLOPT_PROXYPASSWORD,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_COOKIES,
      g_param_spec_boxed ("cookies", "Cookies", "List of HTTP Cookies",
          G_TYPE_STRV, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_USERAGENT,
      g_param_spec_string ("user-agent", "User-Agent",
          "URI of resource requested", GSTCURL_HANDLE_DEFAULT_CURLOPT_USERAGENT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_COMPRESS,
      g_param_spec_boolean ("compress", "Compress",
          "Allow compressed content encodings",
          GSTCURL_HANDLE_DEFAULT_CURLOPT_ACCEPT_ENCODING, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_REDIRECT,
      g_param_spec_boolean ("automatic-redirect", "automatic-redirect",
          "Allow HTTP Redirections (HTTP Status Code 300 series)",
          GSTCURL_HANDLE_DEFAULT_CURLOPT_FOLLOWLOCATION, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_MAXREDIRECT,
      g_param_spec_int ("max-redirect", "Max-Redirect",
          "Maximum number of permitted redirections. -1 is unlimited.",
          GSTCURL_HANDLE_MIN_CURLOPT_MAXREDIRS,
          GSTCURL_HANDLE_MAX_CURLOPT_MAXREDIRS,
          GSTCURL_HANDLE_DEFAULT_CURLOPT_MAXREDIRS, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_KEEPALIVE,
      g_param_spec_boolean ("keep-alive", "Keep-Alive",
          "Toggle keep-alive for connection reuse.",
          GSTCURL_HANDLE_DEFAULT_CURLOPT_TCP_KEEPALIVE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_TIMEOUT,
      g_param_spec_int ("timeout", "Timeout",
          "Value in seconds before timeout a blocking request (0 = no timeout)",
          GSTCURL_HANDLE_MIN_CURLOPT_TIMEOUT,
          GSTCURL_HANDLE_MAX_CURLOPT_TIMEOUT,
          GSTCURL_HANDLE_DEFAULT_CURLOPT_TIMEOUT, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_HEADERS,
      g_param_spec_boxed ("extra-headers", "Extra Headers",
          "Extra headers to append to the HTTP request",
          G_TYPE_STRV, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_STRICT_SSL,
      g_param_spec_boolean ("ssl-strict", "SSL Strict",
          "Strict SSL certificate checking",
          GSTCURL_HANDLE_DEFAULT_CURLOPT_SSL_VERIFYPEER, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_SSL_CA_FILE,
      g_param_spec_string ("ssl-ca-file", "SSL CA File",
          "Location of an SSL CA file to use for checking SSL certificates",
          GSTCURL_HANDLE_DEFAULT_CURLOPT_CAINFO, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_RETRIES,
      g_param_spec_int ("retries", "Retries",
          "Maximum number of retries until giving up (-1=infinite)",
          GSTCURL_HANDLE_MIN_RETRIES, GSTCURL_HANDLE_MAX_RETRIES,
          GSTCURL_HANDLE_DEFAULT_RETRIES, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CONNECTIONMAXTIME,
      g_param_spec_uint ("max-connection-time", "Max-Connection-Time",
          "Maximum amount of time to keep-alive HTTP connections",
          GSTCURL_MIN_CONNECTION_TIME, GSTCURL_MAX_CONNECTION_TIME, 30,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_MAXCONCURRENT_SERVER,
      g_param_spec_uint ("max-connections-per-server",
          "Max-Connections-Per-Server",
          "Maximum number of connections allowed per server for HTTP/1.x",
          GSTCURL_MIN_CONNECTIONS_SERVER, GSTCURL_MAX_CONNECTIONS_SERVER,
          GSTCURL_DEFAULT_CONNECTIONS_SERVER,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_MAXCONCURRENT_PROXY,
      g_param_spec_uint ("max-connections-per-proxy",
          "Max-Connections-Per-Proxy",
          "Maximum number of concurrent connections allowed per proxy for HTTP/1.x",
          GSTCURL_MIN_CONNECTIONS_PROXY, GSTCURL_MAX_CONNECTIONS_PROXY,
          GSTCURL_DEFAULT_CONNECTIONS_PROXY,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_MAXCONCURRENT_GLOBAL,
      g_param_spec_uint ("max-connections", "Max-Connections",
          "Maximum number of concurrent connections allowed for HTTP/1.x",
          GSTCURL_MIN_CONNECTIONS_GLOBAL, GSTCURL_MAX_CONNECTIONS_GLOBAL,
          GSTCURL_DEFAULT_CONNECTIONS_GLOBAL,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
#ifdef CURL_VERSION_HTTP2
  if (gst_curl_http_src_curl_capabilities->features && CURL_VERSION_HTTP2) {
    GST_INFO_OBJECT (klass, "Our curl version (%s) supports HTTP2!",
        gst_curl_http_src_curl_capabilities->version);
    g_object_class_install_property (gobject_class, PROP_HTTPVERSION,
        g_param_spec_float ("http-version", "HTTP-Version",
            "The preferred HTTP protocol version (Supported 1.0, 1.1, 2.0)",
            1.0, 2.0, pref_http_ver,
            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
  }
  else {
#endif
    if (pref_http_ver > 1.1) {
      pref_http_ver = GSTCURL_HANDLE_DEFAULT_CURLOPT_HTTP_VERSION;
    }
    g_object_class_install_property (gobject_class, PROP_HTTPVERSION,
        g_param_spec_float ("http-version", "HTTP-Version",
            "The preferred HTTP protocol version (Supported 1.0, 1.1)",
            1.0, 1.1, pref_http_ver,
            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
#ifdef CURL_VERSION_HTTP2
  }
#endif

  /* Add a debugging task so it's easier to debug in the Multi worker thread */
  GST_DEBUG_CATEGORY_INIT (gst_curl_loop_debug, "curl_multi_loop", 0,
      "libcURL loop thread debugging");
  gst_debug_log (gst_curl_loop_debug, GST_LEVEL_INFO, __FILE__, __func__,
      __LINE__, NULL, "Testing the curl_multi_loop debugging prints");

  g_mutex_init(&klass->multi_task_context.mutex);
  g_cond_init(&klass->multi_task_context.signal);
#if GST_CHECK_VERSION(1,0,0)
  g_rec_mutex_init(&klass->multi_task_context.task_rec_mutex);
#else
  g_static_rec_mutex_init (&klass->multi_task_context.task_rec_mutex);
#endif

#if GST_CHECK_VERSION(1,0,0)
  gst_element_class_set_static_metadata (gstelement_class,
#else
  gst_element_class_set_details_simple (gstelement_class,
#endif
      "HTTP Client Source using libcURL",
      "Source/Network",
      "Receiver data as a client over a network via HTTP using cURL",
      "Sam Hurst <samuelh@rd.bbc.co.uk>");
}

static void
gst_curl_http_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  gfloat f;
  GstCurlHttpSrc *source = GST_CURLHTTPSRC (object);
  GSTCURL_FUNCTION_ENTRY (source);

  switch (prop_id) {
    case PROP_URI:
      if (source->uri != NULL) {
        g_free (source->uri);
      }
      source->uri = g_value_dup_string (value);
      break;
    case PROP_USERNAME:
      if (source->username != NULL) {
	g_free (source->username);
      }
      source->username = g_value_dup_string (value);
      break;
    case PROP_PASSWORD:
      if (source->password != NULL) {
	g_free (source->password);
      }
      source->password = g_value_dup_string (value);
      break;
    case PROP_PROXYURI:
      if (source->proxy_uri != NULL) {
        g_free (source->uri);
      }
      source->proxy_uri = g_value_dup_string (value);
      break;
    case PROP_PROXYUSERNAME:
      if (source->proxy_user != NULL) {
	  g_free (source->proxy_user);
      }
      source->proxy_user = g_value_dup_string (value);
      break;
    case PROP_PROXYPASSWORD:
      if (source->proxy_pass != NULL) {
	  g_free (source->proxy_pass);
      }
      source->proxy_pass = g_value_dup_string (value);
      break;
    case PROP_COOKIES:
      g_strfreev (source->cookies);
      source->cookies = g_strdupv (g_value_get_boxed (value));
      source->number_cookies = g_strv_length (source->cookies);
      break;
    case PROP_USERAGENT:
      if (source->user_agent != NULL) {
        g_free (source->user_agent);
      }
      source->user_agent = g_value_dup_string (value);
      break;
    case PROP_HEADERS:
      g_strfreev (source->extra_headers);
      source->extra_headers = g_strdupv (g_value_get_boxed (value));
      if (source->extra_headers != NULL) {
        source->number_headers = g_strv_length (source->extra_headers);
      } else {
        source->number_headers = 0;
      }
      break;
    case PROP_COMPRESS:
      source->accept_compressed_encodings = g_value_get_boolean (value);
      break;
    case PROP_REDIRECT:
      source->allow_3xx_redirect = g_value_get_boolean (value);
      break;
    case PROP_MAXREDIRECT:
      source->max_3xx_redirects = g_value_get_int (value);
      break;
    case PROP_KEEPALIVE:
      source->keep_alive = g_value_get_boolean (value);
      break;
    case PROP_TIMEOUT:
      source->timeout_secs = g_value_get_int (value);
      break;
    case PROP_STRICT_SSL:
      source->strict_ssl = g_value_get_boolean (value);
      break;
    case PROP_SSL_CA_FILE:
      source->custom_ca_file = g_value_dup_string (value);
      break;
    case PROP_RETRIES:
      source->total_retries = g_value_get_int (value);
      break;
    case PROP_CONNECTIONMAXTIME:
      source->max_connection_time = g_value_get_uint (value);
      break;
    case PROP_MAXCONCURRENT_SERVER:
      source->max_conns_per_server = g_value_get_uint (value);
      break;
    case PROP_MAXCONCURRENT_PROXY:
      source->max_conns_per_proxy = g_value_get_uint (value);
      break;
    case PROP_MAXCONCURRENT_GLOBAL:
      source->max_conns_global = g_value_get_uint (value);
      break;
    case PROP_HTTPVERSION:
      f = g_value_get_float (value);
      if (f == 1.0) {
        source->preferred_http_version = GSTCURL_HTTP_VERSION_1_0;
      } else if (f == 1.1) {
        source->preferred_http_version = GSTCURL_HTTP_VERSION_1_1;
#ifdef CURL_VERSION_HTTP2
      } else if (f == 2.0) {
        source->preferred_http_version = GSTCURL_HTTP_VERSION_2_0;
#endif
      } else {
        source->preferred_http_version = GSTCURL_HTTP_VERSION_1_1;
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  GSTCURL_FUNCTION_EXIT (source);
}

static void
gst_curl_http_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstCurlHttpSrc *source = GST_CURLHTTPSRC (object);
  GSTCURL_FUNCTION_ENTRY (source);

  switch (prop_id) {
    case PROP_URI:
      g_value_set_string (value, source->uri);
      break;
    case PROP_USERNAME:
      g_value_set_string (value, source->username);
      break;
    case PROP_PASSWORD:
      g_value_set_string (value, source->password);
      break;
    case PROP_PROXYURI:
      g_value_set_string (value, source->proxy_uri);
      break;
    case PROP_PROXYUSERNAME:
      g_value_set_string (value, source->proxy_user);
      break;
    case PROP_PROXYPASSWORD:
      g_value_set_string (value, source->proxy_pass);
      break;
    case PROP_COOKIES:
      g_value_set_boxed (value, source->cookies);
      break;
    case PROP_USERAGENT:
      g_value_set_string (value, source->user_agent);
      break;
    case PROP_HEADERS:
      g_value_set_boxed (value, source->extra_headers);
      break;
    case PROP_COMPRESS:
      g_value_set_boolean (value, source->accept_compressed_encodings);
      break;
    case PROP_REDIRECT:
      g_value_set_boolean (value, source->allow_3xx_redirect);
      break;
    case PROP_MAXREDIRECT:
      g_value_set_int (value, source->max_3xx_redirects);
      break;
    case PROP_KEEPALIVE:
      g_value_set_boolean (value, source->keep_alive);
      break;
    case PROP_TIMEOUT:
      g_value_set_int (value, source->timeout_secs);
      break;
    case PROP_STRICT_SSL:
      g_value_set_boolean (value, source->strict_ssl);
      break;
    case PROP_SSL_CA_FILE:
      g_value_set_string (value, source->custom_ca_file);
      break;
    case PROP_RETRIES:
      g_value_set_int (value, source->total_retries);
      break;
    case PROP_CONNECTIONMAXTIME:
      g_value_set_uint (value, source->max_connection_time);
      break;
    case PROP_MAXCONCURRENT_SERVER:
      g_value_set_uint (value, source->max_conns_per_server);
      break;
    case PROP_MAXCONCURRENT_PROXY:
      g_value_set_uint (value, source->max_conns_per_proxy);
      break;
    case PROP_MAXCONCURRENT_GLOBAL:
      g_value_set_uint (value, source->max_conns_global);
      break;
    case PROP_HTTPVERSION:
      switch (source->preferred_http_version) {
        case GSTCURL_HTTP_VERSION_1_0:
          g_value_set_float (value, 1.0);
          break;
        case GSTCURL_HTTP_VERSION_1_1:
          g_value_set_float (value, 1.1);
          break;
#ifdef CURL_VERSION_HTTP2
        case GSTCURL_HTTP_VERSION_2_0:
          g_value_set_float (value, 2.0);
          break;
#endif
        default:
          GST_WARNING_OBJECT (source, "Bad HTTP version in object");
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  GSTCURL_FUNCTION_EXIT (source);
}

static void
gst_curl_http_src_init (GstCurlHttpSrc * source)
{
  GSTCURL_FUNCTION_ENTRY (source);

  /* Assume everything is already free'd */
  source->uri = NULL;
  source->redirect_uri = NULL;
  source->username = GSTCURL_HANDLE_DEFAULT_CURLOPT_USERNAME;
  source->password = GSTCURL_HANDLE_DEFAULT_CURLOPT_PASSWORD;
  source->proxy_uri = NULL;
  source->proxy_user = NULL;
  source->proxy_pass = NULL;
  source->cookies = NULL;
  source->user_agent = GSTCURL_HANDLE_DEFAULT_CURLOPT_USERAGENT;
  source->number_cookies = 0;
  source->extra_headers = NULL;
  source->number_headers = 0;
  source->end_of_message = FALSE;
  source->allow_3xx_redirect = GSTCURL_HANDLE_DEFAULT_CURLOPT_FOLLOWLOCATION;
  source->max_3xx_redirects = GSTCURL_HANDLE_DEFAULT_CURLOPT_MAXREDIRS;
  source->keep_alive = GSTCURL_HANDLE_DEFAULT_CURLOPT_TCP_KEEPALIVE;
  source->timeout_secs = GSTCURL_HANDLE_DEFAULT_CURLOPT_TIMEOUT;
  source->strict_ssl = GSTCURL_HANDLE_DEFAULT_CURLOPT_SSL_VERIFYPEER;
  source->custom_ca_file = NULL;
  source->preferred_http_version = pref_http_ver;
  source->total_retries = GSTCURL_HANDLE_DEFAULT_RETRIES;

  gst_caps_replace(&source->caps, NULL);
#if GST_CHECK_VERSION(1,0,0)
  gst_base_src_set_automatic_eos (GST_BASE_SRC (source), FALSE);
#endif

  source->proxy_uri = g_strdup (g_getenv ("http_proxy"));
  source->no_proxy_list = g_strdup (g_getenv ("no_proxy"));

  source->finished = g_new (GCond, 1);
  g_cond_init (source->finished);
  source->uri_mutex = g_new (GMutex, 1);
  g_mutex_init (source->uri_mutex);

  GSTCURL_FUNCTION_EXIT (source);
}

/*
 * Check if the Curl multi loop has been started. If not, initialise it and
 * start it running. If it is already running, increment the refcount.
 */
static void
gst_curl_http_src_ref_multi (GstCurlHttpSrc *src) {
  GstCurlHttpSrcClass *klass;

  GSTCURL_FUNCTION_ENTRY (source);

  /*klass = (GstCurlHttpSrcClass) g_type_class_peek_parent (src);*/
  klass = G_TYPE_INSTANCE_GET_CLASS (src, GST_TYPE_CURL_HTTP_SRC,
                                     GstCurlHttpSrcClass);

  g_mutex_lock(&klass->multi_task_context.mutex);
  if(klass->multi_task_context.refcount == 0) {
    /* Set up various in-task properties */

    /* NULL is treated as the start of the list, no need to allocate. */
    klass->multi_task_context.queue = NULL;

    /* set up curl */
    klass->multi_task_context.multi_handle = curl_multi_init ();

    curl_multi_setopt (klass->multi_task_context.multi_handle,
                       CURLMOPT_PIPELINING, 1);
#ifdef CURLMOPT_MAX_HOST_CONNECTIONS
    curl_multi_setopt (klass->multi_task_context.multi_handle,
                       CURLMOPT_MAX_HOST_CONNECTIONS, 1);
#endif

    /* Start the thread */
#if GST_CHECK_VERSION(1,0,0)
    klass->multi_task_context.task = gst_task_new (
            (GstTaskFunction) gst_curl_http_src_curl_multi_loop,
            (gpointer) &klass->multi_task_context, NULL);
#else
    klass->multi_task_context.task = gst_task_create (
            (GstTaskFunction) gst_curl_http_src_curl_multi_loop,
            (gpointer) &klass->multi_task_context);
#endif
    gst_task_set_lock (klass->multi_task_context.task,
                       &klass->multi_task_context.task_rec_mutex);
    if (gst_task_start (klass->multi_task_context.task) == FALSE) {
      /*
       * This is a pretty critical failure and is not recoverable, so commit
       * sudoku and run away.
       */
      GSTCURL_ERROR_PRINT("Couldn't start curl_multi task! Aborting.");
      abort ();
    }
    GSTCURL_INFO_PRINT("Curl multi loop has been correctly initialised!");
  }
  klass->multi_task_context.refcount++;
  g_mutex_unlock(&klass->multi_task_context.mutex);

  GSTCURL_FUNCTION_EXIT (source);
}

/*
 * Decrement the reference count on the curl multi loop. If this is called by
 * the last instance to hold a reference, shut down the worker. (Otherwise
 * GStreamer can't close down with a thread still running). Also offers the
 * "force_all" boolean parameter, which if TRUE removes all references and shuts
 * down.
 */
static void
gst_curl_http_src_unref_multi (GstCurlHttpSrc *src)
{
  GstCurlHttpSrcClass *klass;

  GSTCURL_FUNCTION_ENTRY (src);

  klass = G_TYPE_INSTANCE_GET_CLASS (src, GST_TYPE_CURL_HTTP_SRC,
                                     GstCurlHttpSrcClass);

  g_mutex_lock(&klass->multi_task_context.mutex);
  klass->multi_task_context.refcount--;
  GST_INFO_OBJECT (src, "Closing instance, worker thread refcount is now %u",
                   klass->multi_task_context.refcount);

  if(klass->multi_task_context.refcount <= 0) {
    /* Everything's done! Clean up. */
    gst_task_pause (klass->multi_task_context.task);
    klass->multi_task_context.state = GSTCURL_MULTI_LOOP_STATE_STOP;
    g_cond_signal (&klass->multi_task_context.signal);
    g_mutex_unlock (&klass->multi_task_context.mutex);
    gst_task_join (klass->multi_task_context.task);
  }
  else {
    g_mutex_unlock(&klass->multi_task_context.mutex);
  }

  GSTCURL_FUNCTION_EXIT (src);
}

static void
gst_curl_http_src_finalize (GObject *obj)
{
  GstCurlHttpSrc *src = GST_CURLHTTPSRC (obj);

  GSTCURL_FUNCTION_ENTRY (src);

  /* Cleanup all memory allocated */
  gst_curl_http_src_cleanup_instance (src);

  GSTCURL_FUNCTION_EXIT(src);
}

static GstFlowReturn
gst_curl_http_src_create (GstPushSrc * psrc, GstBuffer ** outbuf)
{
  GstFlowReturn ret;
  GstCurlHttpSrc *src = GST_CURLHTTPSRC (psrc);
  GSTCURL_FUNCTION_ENTRY (src);
  ret = GST_FLOW_OK;

  if (src->end_of_message == TRUE) {
    GST_DEBUG_OBJECT (src, "Full body received, signalling EOS for URI %s.",
        src->uri);
    src->end_of_message = FALSE;
    return GST_FLOW_EOS;
  }

  /* Create the easy handle */
  /* Create the main loop for this new handle */
  /* start it */
  /* lock the main loop */
  /* check that we have or either an error or data */
  /* Process the easy handle */

  src->retries_remaining = src->total_retries;

  /* If total_retries is -1, it's infinite so the value of retries_remaining
   * could be 0xDEADBEEF for all we care, it makes no difference. */
  while((src->retries_remaining >= 0) || (src->total_retries == -1)) {
    src->retries_remaining--;

    src->curl_handle = gst_curl_http_src_create_easy_handle (src);

    if (gst_curl_http_src_make_request (src) == FALSE) {
      return GST_FLOW_ERROR;
    }

    ret = gst_curl_http_src_handle_response (src, outbuf);

    gst_curl_http_src_destroy_easy_handle (src);

    if (ret == GST_FLOW_OK) {
      gst_curl_http_src_negotiate_caps (src);
      break;
    }
    else if (ret == GST_FLOW_ERROR) {
      break;
    }
  }

  /* Reset the return types as our instance will be reused with a new URI */
  g_free (src->msg);
  src->msg = NULL;
  g_free (src->headers.content_type);
  src->headers.content_type = NULL;
  src->len = 0;

  GSTCURL_FUNCTION_EXIT (src);
  return ret;
}

/*
 * From the data in the queue element s, create a CURL easy handle and populate
 * options with the URL, proxy data, login options, cookies,
 */
static CURL *
gst_curl_http_src_create_easy_handle (GstCurlHttpSrc * s)
{
  CURL *handle;
  gint i;
  GSTCURL_FUNCTION_ENTRY (s);

  handle = curl_easy_init ();
  if (handle == NULL) {
    GST_ERROR_OBJECT (s, "Couldn't init a curl easy handle!");
    return NULL;
  }
  GST_INFO_OBJECT (s, "Creating a new handle for URI %s", s->uri);

  /* This is mandatory and yet not default option, so if this is NULL
   * then something very bad is going on. */
  curl_easy_setopt (handle, CURLOPT_URL, s->uri);

  gst_curl_setopt_str (s, handle, CURLOPT_USERNAME, s->username);
  gst_curl_setopt_str (s, handle, CURLOPT_PASSWORD, s->password);
  gst_curl_setopt_str (s, handle, CURLOPT_PROXY, s->proxy_uri);
  gst_curl_setopt_str (s, handle, CURLOPT_NOPROXY, s->no_proxy_list);
  gst_curl_setopt_str (s, handle, CURLOPT_PROXYUSERNAME, s->proxy_user);
  gst_curl_setopt_str (s, handle, CURLOPT_PROXYPASSWORD, s->proxy_pass);

  for (i = 0; i < s->number_cookies; i++) {
    gst_curl_setopt_str (s, handle, CURLOPT_COOKIELIST, s->cookies[i]);
  }

  /* curl_slist_append dynamically allocates memory, but I need to free it */
  for (i = 0; i < s->number_headers; i++) {
    s->slist = curl_slist_append(s->slist, s->extra_headers[i]);
  }
  if(s->slist != NULL) {
      curl_easy_setopt(handle, CURLOPT_HTTPHEADER, s->slist);
  }

  gst_curl_setopt_str_default (s, handle, CURLOPT_USERAGENT, s->user_agent);

  /*
   * Unlike soup, this isn't a binary op, curl wants a string here. So if it's
   * TRUE, simply set the value as an empty string as this allows both gzip and
   * zlib compression methods.
   */
  if(s->accept_compressed_encodings == TRUE) {
      curl_easy_setopt(handle, CURLOPT_ACCEPT_ENCODING, "");
  }
  else {
      curl_easy_setopt(handle, CURLOPT_ACCEPT_ENCODING, "identity");
  }

  gst_curl_setopt_int (s, handle, CURLOPT_FOLLOWLOCATION,
                       s->allow_3xx_redirect);
  gst_curl_setopt_int_default (s, handle, CURLOPT_MAXREDIRS,
                       s->max_3xx_redirects);
  gst_curl_setopt_int (s, handle, CURLOPT_TCP_KEEPALIVE,
                       GSTCURL_BINARYBOOL (s->keep_alive));
  gst_curl_setopt_int (s, handle, CURLOPT_TIMEOUT, s->timeout_secs);
  gst_curl_setopt_int (s, handle, CURLOPT_SSL_VERIFYPEER,
                       GSTCURL_BINARYBOOL (s->strict_ssl));
  gst_curl_setopt_str (s, handle, CURLOPT_CAINFO, s->custom_ca_file);

  switch (s->preferred_http_version) {
    case GSTCURL_HTTP_VERSION_1_0:
      GST_DEBUG_OBJECT (s, "Setting version as HTTP/1.0");
      gst_curl_setopt_int (s, handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
      break;
    case GSTCURL_HTTP_VERSION_1_1:
      GST_DEBUG_OBJECT (s, "Setting version as HTTP/1.1");
      gst_curl_setopt_int (s, handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
      break;
#ifdef CURL_VERSION_HTTP2
    case GSTCURL_HTTP_VERSION_2_0:
      GST_DEBUG_OBJECT (s, "Setting version as HTTP/2.0");
      curl_easy_setopt (handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
      break;
#endif
    default:
      GST_WARNING_OBJECT (s,
          "Supplied a bogus HTTP version, using curl default!");
  }

  curl_easy_setopt (handle, CURLOPT_HEADERFUNCTION,
                    gst_curl_http_src_get_header);
  curl_easy_setopt (handle, CURLOPT_HEADERDATA, s);
  curl_easy_setopt (handle, CURLOPT_WRITEFUNCTION,
                    gst_curl_http_src_get_chunks);
  curl_easy_setopt (handle, CURLOPT_WRITEDATA, s);

  GSTCURL_FUNCTION_EXIT (s);
  return handle;
}

/*
 * Add the GstCurlHttpSrc item to the queue and then wait until the curl thread
 * signals us to say that our request has completed.
 */
static gboolean
gst_curl_http_src_make_request (GstCurlHttpSrc * s)
{
  GstCurlHttpSrcClass *klass;
  gboolean ret = FALSE;

  klass = G_TYPE_INSTANCE_GET_CLASS (s, GST_TYPE_CURL_HTTP_SRC,
                                     GstCurlHttpSrcClass);

  g_mutex_lock(&klass->multi_task_context.mutex);

  if (gst_curl_http_src_add_queue_item (&klass->multi_task_context.queue, s)
      == FALSE) {
    GST_ERROR_OBJECT (s, "Couldn't create new queue item! Aborting...");
    abort();
  }

  GST_DEBUG_OBJECT (s, "Submitting request for URI %s to curl", s->uri);

  /* Signal the worker thread */
  klass->multi_task_context.state = GSTCURL_MULTI_LOOP_STATE_QUEUE_EVENT;
  g_cond_signal (&klass->multi_task_context.signal);
  g_cond_wait (s->finished, &klass->multi_task_context.mutex);
  g_mutex_unlock (&klass->multi_task_context.mutex);

  switch (s->result) {
    case GSTCURL_RETURN_NONE:
      GST_WARNING_OBJECT (s, "Nothing ever happened to our request for URI %s!",
          s->uri);
      break;
    case GSTCURL_RETURN_DONE:
      GST_DEBUG_OBJECT (s, "cURL call finished and returned for URI %s",
          s->uri);
      s->end_of_message = TRUE;
      ret = TRUE;
      break;
    case GSTCURL_RETURN_BAD_QUEUE_REQUEST:
      GST_WARNING_OBJECT (s, "cURL call for URI %s returned as a bad queue",
          s->uri);
      break;
    case GSTCURL_RETURN_TOTAL_ERROR:
      GST_ERROR_OBJECT (s, "cURL call for URI %s returned as a total failure",
          s->uri);
      break;
    case GSTCURL_RETURN_PIPELINE_NULL:
      GST_INFO_OBJECT (s,
          "Pipeline is cleaning up before request for URI %s could complete",
          s->uri);
      break;
    default:
      /* Why are we here? */
      GST_WARNING_OBJECT (s, "Illegal curl worker thread result!");
  }

  GSTCURL_FUNCTION_EXIT (s);
  return ret;
}

/*
 * Check return codes
 */
static GstFlowReturn
gst_curl_http_src_handle_response (GstCurlHttpSrc * src, GstBuffer ** buf)
{
  GstFlowReturn ret = GST_FLOW_OK;
  glong curl_info_long;
  gdouble curl_info_dbl;
  gchar *redirect_url;
  size_t lena,lenb;
  GstBaseSrc *basesrc;
  GSTCURL_FUNCTION_ENTRY (src);

  /* Get back the return code for the session */
  if (curl_easy_getinfo (src->curl_handle, CURLINFO_RESPONSE_CODE,
          &curl_info_long) != CURLE_OK) {
    /* Curl cannot be relied on in this state, so return an error. */
    return GST_FLOW_ERROR;
  }

  if (GSTCURL_INFO_RESPONSE (curl_info_long) ||
      GSTCURL_SUCCESS_RESPONSE (curl_info_long)) {
    /* Everything should be fine. */
    GST_INFO_OBJECT (src, "Get for URI %s succeeded, response code %ld",
        src->uri, curl_info_long);
  }
  else if (GSTCURL_REDIRECT_RESPONSE (curl_info_long)) {
    /* Some redirection response. souphttpsrc reports errors here, so I'm
     * going to do the same. I should only see these if:
     *  > Curl has been configured not to follow redirects
     *  > Curl has been configured to follow redirects up to a given limit and
     *    that limit has been exceeded. (By default it's unlimited)
     *
     * Either way there won't be the response that was requested so signal a
     * flow error.
     */
    GST_WARNING_OBJECT (src, "Get for URI %s received redirection code %ld",
        src->uri, curl_info_long);
    /* Curl may have set the requested redirection URI that it was otherwise
     * barred from following, so set this just in case the sink element wants
     * to try again itself.
     */
    if (curl_easy_getinfo (src->curl_handle, CURLINFO_REDIRECT_URL,
                           &redirect_url) == CURLE_OK) {
      if(redirect_url != NULL) {
        GST_INFO_OBJECT(src, "Got a redirect to %s, setting as redirect URI",
                        src->redirect_uri);
        src->redirect_uri = g_strdup(redirect_url);
      }
    }

    ret = GST_FLOW_ERROR;
    /* Redirection limit has been exceeded, don't retry as we will only retry
     * from original URI.
     */
    src->retries_remaining = 0;
  }
  else if (GSTCURL_CLIENT_ERR_RESPONSE (curl_info_long)) {
    GST_ERROR_OBJECT (src, "Get for URI %s received client error code %ld",
        src->uri, curl_info_long);
    ret = GST_FLOW_ERROR;
    /* For client error, any retry with the same request is more than likely
     * going to fail. */
    src->retries_remaining = 0;
  }
  else if (GSTCURL_SERVER_ERR_RESPONSE (curl_info_long)) {
    GST_ERROR_OBJECT (src, "Get for URI %s received server error code %ld",
        src->uri, curl_info_long);
    ret = GST_FLOW_ERROR;
    /* Server isn't working, so again retries are best avoided. */
    src->retries_remaining = 0;
  }
  else {
    /*
     * If we got here, odds are that no actual conversation between client and
     * server took place. Check for timeouts so we can try again if retries are
     * > 0. Alternatively, this could be for an SSL-related error,
     */
    if (curl_easy_getinfo (src->curl_handle, CURLINFO_TOTAL_TIME,
                           &curl_info_dbl) != CURLE_OK) {
      /* Curl cannot be relied on in this state, so return an error. */
      return GST_FLOW_ERROR;
    }

    if (curl_info_dbl > src->timeout_secs) {
      GST_ERROR_OBJECT (src, "Request for URI %s timed out after %d seconds.",
                        src->uri, src->timeout_secs);
    }

    if (curl_easy_getinfo (src->curl_handle, CURLINFO_OS_ERRNO,
                           &curl_info_long) != CURLE_OK) {
      /* Curl cannot be relied on in this state, so return an error. */
      return GST_FLOW_ERROR;
    }

    GST_WARNING_OBJECT (src, "Errno for CONNECT call was %ld (%s)", curl_info_long,
                        g_strerror((gint) curl_info_long));

    /* Some of these responses are retry-able, others not. Set the returned
     * state to ERROR so we crash out instead of fruitlessly attempting.
     */
    if (curl_info_long == ECONNREFUSED) {
	ret = GST_FLOW_ERROR;
    }
    else {
      /* Don't die, but don't continue as if everything's okay either. Let the
       * retry logic decide next course of action. */
      ret = GST_FLOW_CUSTOM_SUCCESS;
      src->end_of_message = FALSE;
    }
  }

  /*
   * If the returned response has a body that we want to forward on, fill
   * in the buffer.
   */
  if (ret == GST_FLOW_OK) {
    guint8 *data;
#if GST_CHECK_VERSION(1,0,0)
    GstMapInfo info;

    *buf = gst_buffer_new_allocate (NULL, src->len, NULL);
    gst_buffer_map (*buf, &info, GST_MAP_READWRITE);
    data = info.data;
#else
    *buf = gst_buffer_new_and_alloc (src->len);
    data = GST_BUFFER_DATA (*buf);
#endif
    memcpy (data, src->msg, (size_t) src->len);
#if GST_CHECK_VERSION(1,0,0)
    gst_buffer_unmap (*buf, &info);
#endif
  }

  /*
   * Check for any redirection that happened. If the result of
   * CURLINFO_EFFECTIVE_URL is NULL, then the originally supplied URL was used
   * to retrieve the content. Otherwise, a redirected URL was used, and this
   * must be reused as part of our base URL from now on.
   *
   * TODO: At present, there is no easy way in curl to differentiate a
   * temporary redirect from a permanent one.
   */
  if(curl_easy_getinfo(src->curl_handle, CURLINFO_EFFECTIVE_URL, &redirect_url)
                       == CURLE_OK) {
    lena = strlen(src->uri);
    lenb = strlen(redirect_url);
    if(g_ascii_strncasecmp(src->uri, redirect_url, (lena>lenb)?lenb:lena) != 0)
    {
      GST_INFO_OBJECT(src, "Got a redirect to %s, setting as redirect URI",
                      redirect_url);
      src->redirect_uri = g_strdup(redirect_url);
    }
  }

  /*
   * Get the Content-Length to tell upstream elements the "duration" of this
   * downloaded item.
   */
  if(curl_easy_getinfo(src->curl_handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD,
                       &curl_info_dbl) == CURLE_OK) {
    if (curl_info_dbl == -1) {
      GST_WARNING_OBJECT(src,
                         "No Content-Length was specified in the response.");
    }
    else {
      GST_INFO_OBJECT(src, "Content-Length was given as %.0f", curl_info_dbl);
      basesrc = GST_BASE_SRC_CAST (src);
      basesrc->segment.duration = curl_info_dbl;
      src->content_length = (guint64) curl_info_dbl;
#if GST_CHECK_VERSION(1,0,0)
      gst_element_post_message (GST_ELEMENT (src),
                          gst_message_new_duration_changed (GST_OBJECT (src)));
#else
      gst_element_post_message (GST_ELEMENT (src),
                          gst_message_new_duration (GST_OBJECT (src),
                          GST_FORMAT_BYTES, GST_CLOCK_TIME_NONE));
#endif
    }
  }

  GSTCURL_FUNCTION_EXIT (src);
  return ret;
}

/*
 * "Negotiate" capabilities between us and the sink.
 * I.e. tell the sink device what data to expect. We can't be told what to send
 * unless we implement "only return to me if this type" property. Potential TODO
 */
static gboolean
gst_curl_http_src_negotiate_caps (GstCurlHttpSrc * src)
{
#if 0
  if (src->headers.content_type != NULL) {
    if (src->caps) {
      GST_INFO_OBJECT (src, "Setting cap on Content-Type of %s",
                       src->headers.content_type);
      src->caps = gst_caps_make_writable (src->caps);
      gst_caps_set_simple (src->caps, "content-type", G_TYPE_STRING,
                           src->headers.content_type, NULL);
      if (gst_base_src_set_caps(GST_BASE_SRC (src), src->caps) != TRUE) {
        GST_ERROR_OBJECT (src, "Setting caps failed!");
        return FALSE;
      }
    }
  }
  else {
    GST_INFO_OBJECT (src, "No caps have been set, continue.");
  }
#endif
  return TRUE;
}

/*
 * Cleanup the CURL easy handle once we're done with it.
 */
static inline void
gst_curl_http_src_destroy_easy_handle (GstCurlHttpSrc * src)
{
  /* Thank you Handles, and well done. Well done, mate. */
  if(src->curl_handle != NULL) {
    curl_easy_cleanup (src->curl_handle);
    src->curl_handle = NULL;
  }
}

static GstStateChangeReturn
gst_curl_http_src_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstCurlHttpSrc *source = GST_CURLHTTPSRC (element);
  GSTCURL_FUNCTION_ENTRY (source);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      gst_curl_http_src_ref_multi(source);
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      /* The pipeline has ended, so signal any running request to end. */
      gst_curl_http_src_request_remove (source);
      gst_curl_http_src_unref_multi(source);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  GSTCURL_FUNCTION_EXIT (source);
  return ret;
}

/*
 * Take care of any memory that may be left over from the instance that's now
 * closing before we leak it.
 */
static void
gst_curl_http_src_cleanup_instance(GstCurlHttpSrc *src)
{
  gint i;
  g_mutex_lock(src->uri_mutex);
  g_free(src->uri);
  src->uri = NULL;
  g_free(src->redirect_uri);
  src->redirect_uri = NULL;
  g_mutex_unlock(src->uri_mutex);
  g_mutex_clear(src->uri_mutex);
  g_free(src->uri_mutex);
  src->uri_mutex = NULL;

  g_free(src->proxy_uri);
  src->proxy_uri = NULL;
  g_free(src->no_proxy_list);
  src->no_proxy_list = NULL;
  g_free(src->proxy_user);
  src->proxy_user = NULL;
  g_free(src->proxy_pass);
  src->proxy_pass = NULL;

  for(i = 0; i < src->number_cookies; i++)
  {
    g_free(src->cookies[i]);
    src->cookies[i] = NULL;
  }
  g_free(src->cookies);
  src->cookies = NULL;

  g_cond_clear(src->finished);
  g_free(src->finished);
  src->finished = NULL;

  gst_curl_http_src_destroy_easy_handle(src);

  g_free(src->msg);
  src->msg = NULL;
  g_free(src->headers.content_type);
  src->headers.content_type = NULL;
}

static gboolean
gst_curl_http_src_query (GstBaseSrc * bsrc, GstQuery * query)
{
  GstCurlHttpSrc *src = GST_CURLHTTPSRC (bsrc);
  gboolean ret;
  GSTCURL_FUNCTION_ENTRY(src);

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_URI:
      gst_query_set_uri (query, src->uri);
#if GST_CHECK_VERSION(1,0,0)
      if (src->redirect_uri != NULL) {
        gst_query_set_uri_redirection (query, src->redirect_uri);
      }
#endif
      ret = TRUE;
      break;
    default:
      ret = GST_BASE_SRC_CLASS (parent_class)->query (bsrc, query);
      break;
  }

  GSTCURL_FUNCTION_EXIT(src);
  return ret;
}

static gboolean
gst_curl_http_src_get_content_length (GstBaseSrc * bsrc, guint64 * size)
{
  GstCurlHttpSrc *src = GST_CURLHTTPSRC (bsrc);

  if (src->content_length > 0) {
    *size = src->content_length;
    GST_DEBUG_OBJECT (src,
                      "Returning content length size of %" G_GUINT64_FORMAT,
                      *size);
    return TRUE;
  }
  GST_DEBUG_OBJECT (src,
                  "No content length has yet been set, or there was an error!");
  return FALSE;
}

/*****************************************************************************
 * Curl loop task functions begin
 *****************************************************************************/
static void
gst_curl_http_src_curl_multi_loop (gpointer thread_data)
{
  GstCurlHttpSrcMultiTaskContext* context;
  GstCurlHttpSrcQueueElement *qelement;
  int i, still_running;
  gboolean cond = FALSE;
  CURLMsg *curl_message;

  context = (GstCurlHttpSrcMultiTaskContext *) thread_data;

  g_mutex_lock (&context->mutex);

  /* Someone is holding a reference to us, but isn't using us so to avoid
   * unnecessary clock cycle wasting, sit in a conditional wait until woken.
   */
  while (context->state == GSTCURL_MULTI_LOOP_STATE_WAIT) {
    GSTCURL_DEBUG_PRINT ("Entering wait state...");
    g_cond_wait (&context->signal, &context->mutex);
    GSTCURL_DEBUG_PRINT ("Received wake up call!");
  }

  if (context->state == GSTCURL_MULTI_LOOP_STATE_QUEUE_EVENT) {
    GSTCURL_DEBUG_PRINT ("Received a new item on the queue!");
    if (context->queue == NULL) {
      GSTCURL_ERROR_PRINT ("Request Queue was empty on a Queue Event!");
      context->state = GSTCURL_MULTI_LOOP_STATE_WAIT;
      return;
    }

    /*
     * Use the running mutex to lock access to each element, as the
     * mutex's memory barriers stop cache optimisations from meaning
     * flag values can't be trusted. The trylock will only let us in
     * once and should fail immediately prior.
     */
    qelement = context->queue;
    while(qelement != NULL) {
      if (g_mutex_trylock(&qelement->running) == TRUE) {
        GSTCURL_DEBUG_PRINT ("Adding easy handle for URI %s", qelement->p->uri);
        cond = TRUE;
        curl_multi_add_handle (context->multi_handle, qelement->p->curl_handle);
      }
      qelement = qelement->next;
    }

    if(cond != TRUE) {
      GSTCURL_WARNING_PRINT ("All curl handles already added for QUEUE_EVENT!");
    }
    else {
      GSTCURL_DEBUG_PRINT ("Finished adding all handles, continuing.");
      context->state = GSTCURL_MULTI_LOOP_STATE_RUNNING;
    }
    g_mutex_unlock(&context->mutex);
  }
  else if (context->state == GSTCURL_MULTI_LOOP_STATE_RUNNING) {
    struct timeval timeout;
    gint rc;
    fd_set fdread, fdwrite, fdexcep;
    int maxfd = -1;
    long curl_timeo = -1;

    /* Because curl can possibly take some time here, be nice and let go of the
     * mutex so other threads can perform state/queue operations as we don't
     * care about those until the end of this. */
    g_mutex_unlock(&context->mutex);

    FD_ZERO (&fdread);
    FD_ZERO (&fdwrite);
    FD_ZERO (&fdexcep);

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    curl_multi_timeout (context->multi_handle, &curl_timeo);
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
    curl_multi_fdset (context->multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);

    rc = select (maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);

    switch (rc) {
    case -1:
      /* select error */
      break;
    case 0:
    default:
      /* timeout or readable/writable sockets */
      curl_multi_perform (context->multi_handle, &still_running);
      break;
    }

    /*
     * Check the CURL message buffer to find out if any transfers have
     * completed. If they have, call the signal_finished function which
     * will signal the g_cond_wait call in that calling instance.
     */
    i = 0;
    while (cond != TRUE) {
      curl_message = curl_multi_info_read (context->multi_handle, &i);
      if (curl_message == NULL) {
        cond = TRUE;
      }
      else if (curl_message->msg == CURLMSG_DONE) {
        /* A hack, but I have seen curl_message->easy_handle being
         * NULL randomly, so check for that. */
	g_mutex_lock(&context->mutex);
        if (curl_message->easy_handle == NULL) {
          break;
        }
        curl_multi_remove_handle (context->multi_handle,
                                  curl_message->easy_handle);
        gst_curl_http_src_remove_queue_handle(&context->queue,
                                            curl_message->easy_handle);
        g_mutex_unlock(&context->mutex);
      }
    }

    if (still_running == 0) {
      /* We've finished processing, so set the state to wait.
       *
       * This is a little more complex, as we need to catch the edge
       * case of another thread adding a queue item while we've been
       * working.
       */
      g_mutex_lock (&context->mutex);
      if ((context->state != GSTCURL_MULTI_LOOP_STATE_QUEUE_EVENT) &&
          (context->state != GSTCURL_MULTI_LOOP_STATE_REQUEST_REMOVAL)) {
        context->state = GSTCURL_MULTI_LOOP_STATE_WAIT;
      }
      g_mutex_unlock (&context->mutex);
    }
  }
  /* Is the following even necessary any more...? */
  else if (context->state == GSTCURL_MULTI_LOOP_STATE_STOP) {
    g_mutex_unlock (&context->mutex);
    /* Something wants us to shut down, so best to do a full cleanup as it
     * might be that something's gone bang.
     */
    /*gst_curl_http_src_unref_multi (NULL, GSTCURL_RETURN_PIPELINE_NULL, TRUE);*/
    GSTCURL_INFO_PRINT ("Got instruction to shut down");
  }
  else if (context->state == GSTCURL_MULTI_LOOP_STATE_REQUEST_REMOVAL) {
    qelement = context->queue;
    while (qelement != NULL) {
      if (qelement->p == context->request_removal_element) {
        curl_multi_remove_handle(context->multi_handle,
                                 context->request_removal_element->curl_handle);
        qelement->p->result = GSTCURL_RETURN_REMOVED;
        g_cond_signal(qelement->p->finished);
        gst_curl_http_src_remove_queue_item (&context->queue, qelement->p);
      }
    }
    context->request_removal_element = NULL;
    g_mutex_unlock (&context->mutex);
  }
  else {
    GSTCURL_WARNING_PRINT ("Curl Loop State was invalid or unsupported");
    GSTCURL_WARNING_PRINT ("Signal State is %d, resetting to RUNNING.",
                           context->state);
    /* Reset to running, so if there isn't anything to do it'll be
     * changed the WAIT once curl_multi_perform says it has no active
     * handles. */
    context->state = GSTCURL_MULTI_LOOP_STATE_RUNNING;
    g_mutex_unlock (&context->mutex);
  }
}

/*
 * Function to get individual headers from curl response.
 */
static size_t
gst_curl_http_src_get_header (void *header, size_t size, size_t nmemb,
    void * src)
{
  GstCurlHttpSrc *s = src;
  char *substr;
  int i, len;
  /*
   * All HTTP headers follow the same format.
   *      <<Identifier>>: <<Value>>
   *
   * So just parse for those!
   */
  substr = gst_curl_http_src_strcasestr (header, "Content-Type: ");
  if (substr != NULL) {
    /*Length of stuff we don't need is 14 bytes */
    substr += 14;
    len = (size * nmemb) - 14;
    if (s->headers.content_type != NULL) {
      GST_DEBUG_OBJECT (s, "Content Type header already present.");
      free (s->headers.content_type);
    }
    s->headers.content_type = malloc (sizeof (char) * (len + 1));
    if (s->headers.content_type == NULL) {
      GST_ERROR_OBJECT (s, "s->headers.content_type malloc failed!");
    }
    else {
      for (i = 0; i < len; i++) {
        /* For some reason, we get garbage characters at the end, so
         * quick and dirty bit of stripping. We only want printing
         * characters here. Also neatly null terminates! */
        if ((substr[i] >= 0x20) && (substr[i] < 0x7f)) {
          s->headers.content_type[i] = substr[i];
        }
        else {
          s->headers.content_type[i] = '\0';
        }
      }
      GST_INFO_OBJECT (s, "Got Content-Type of %s", s->headers.content_type);
    }
  }
  return size * nmemb;
}

/*
 * My own quick and dirty implementation of strcasestr. This is a GNU extension
 * (i.e. not portable) and not always guaranteed to be available.
 *
 * I know this doesn't work if the haystack and needle are the same size. But
 * this isn't necessarily a bad thing, as the only place we currently use this
 * is at a point where returning nothing even if a string match occurs but the
 * needle is the same size as the haystack actually saves us time.
 */
static char *
gst_curl_http_src_strcasestr (const char *haystack, const char *needle)
{
  int i, j, needle_len;
  char *location;

  needle_len = (int) strlen (needle);
  i = 0;
  j = 0;
  location = NULL;

  while (haystack[i] != '\0') {
    if (j == needle_len) {
      location = (char *) haystack + (i - j);
    }
    if (tolower (haystack[i]) == tolower (needle[j])) {
      j++;
    }
    else {
      j = 0;
    }
    i++;
  }

  return location;
}

/*
 * Get chunks for currently running curl process.
 */
static size_t
gst_curl_http_src_get_chunks (void *chunk, size_t size, size_t nmemb,
    void * src)
{
  GstCurlHttpSrc * s = src;
  size_t new_len = s->len + size * nmemb;
  GST_TRACE_OBJECT (s,
      "Received curl chunk for URI %s of size %d, new total size %d", s->uri,
      (int) (size * nmemb), (int) new_len);
  s->msg = realloc (s->msg, (new_len + 1) * sizeof (char));
  if (s->msg == NULL) {
    GST_ERROR_OBJECT (s, "Realloc for cURL response message failed!\n");
    return 0;
  }
  memcpy (s->msg + s->len, chunk, size * nmemb);
  s->len = new_len;
  return size * nmemb;
}

static void
gst_curl_http_src_request_remove (GstCurlHttpSrc * src)
{
  GstCurlHttpSrcClass *klass = G_TYPE_INSTANCE_GET_CLASS (src,
                                                        GST_TYPE_CURL_HTTP_SRC,
                                                        GstCurlHttpSrcClass);
  g_mutex_lock (&klass->multi_task_context.mutex);

  klass->multi_task_context.state = GSTCURL_MULTI_LOOP_STATE_REQUEST_REMOVAL;
  klass->multi_task_context.request_removal_element = src;
  g_cond_signal (&klass->multi_task_context.signal);
  g_mutex_unlock (&klass->multi_task_context.mutex);
  /* The following should be unlocked by the thread... */
  /*g_mutex_unlock(request_removal_mutex); */
}

/*****************************************************************************
 * Curl loop task functions end
 *****************************************************************************/

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
curlhttpsrc_init (GstPlugin * curlhttpsrc)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template curlhttpsrc' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_curl_http_src_debug, "curlhttpsrc",
      0, "UriHandler for libcURL");

  /* Set to 500 so we take precedence over soup for dev purposes. */
  return gst_element_register (curlhttpsrc, "curlhttpsrc", 500,
      GST_TYPE_CURLHTTPSRC);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "CurlHttpSrc"
#endif

FLUENDO_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "curlhttpsrc",
    curlhttpsrc,
    "UriHandler for libcURL",
    curlhttpsrc_init,
    VERSION, "LGPL", "BBC Research & Development", "http://www.bbc.co.uk/rd")
