#ifndef _GST_FLUENDO_H_
#define _GST_FLUENDO_H_

#if defined(GST_PLUGIN_BUILD_STATIC)
/* FIXME: Use GST_PLUGIN_DEFINE2 everywhere when all plugins are ported */
#if ENABLE_DEMO_PLUGIN
#define FLUENDO_PLUGIN_DEFINE(major, minor, name, fun, desc, init, version, \
  license,pkg,url) \
  GST_PLUGIN_DEFINE2(major, minor, fun, desc " [Demo Version]", init, version, license, pkg, url)
#else
#define FLUENDO_PLUGIN_DEFINE(major, minor, name, fun, desc, init, version, \
  license,pkg,url) \
  GST_PLUGIN_DEFINE2(major, minor, fun, desc, init, version, license, pkg, url)
#endif /* ENABLE_DEMO_PLUGIN */
#elif defined(ENABLE_STATIC_PLUGIN)
#if ENABLE_DEMO_PLUGIN
#define FLUENDO_PLUGIN_DEFINE(major, minor, name, fun, desc, init, version, \
  license, pkg, url) \
    gboolean gst_##fun##_init_static() { \
      return gst_plugin_register_static(major, minor, name, desc " [Demo Version]", init, \
          version, license, "Fluendo",pkg,url); \
    }
#else
#define FLUENDO_PLUGIN_DEFINE(major, minor, name, fun, desc, init, version, \
  license, pkg, url) \
    gboolean gst_##fun##_init_static() { \
      return gst_plugin_register_static(major, minor, name, desc, init, \
          version, license, "Fluendo",pkg,url); \
    }

#endif /* ENABLE_DEMO_PLUGIN */
#else
#if ENABLE_DEMO_PLUGIN
#define FLUENDO_PLUGIN_DEFINE(major, minor, name, fun, desc, init, version, \
  license,pkg,url) \
  GST_PLUGIN_DEFINE(major, minor, name, desc " [Demo Version]", init, version, license, pkg, url)
#else
#define FLUENDO_PLUGIN_DEFINE(major, minor, name, fun, desc, init, version, \
  license,pkg,url) \
  GST_PLUGIN_DEFINE(major, minor, name, desc, init, version, license, pkg, url)
#endif /* ENABLE_DEMO_PLUGIN */
#endif /* ENABLE_STATIC_PLUGIN */

#if ENABLE_DEMO_PLUGIN

typedef struct _GstFluStatistics {
  gint64 max_duration;
  gint64 decoded_duration;
} GstFluStatistics;

#define GSTFLU_SETUP_STATISTICS(sink, stats) gstflu_setup_statistics (sink, stats)
#define GSTFLU_PAD_PUSH(src, buf, stats) gstflu_pad_push (src, buf, stats)
#define GSTFLU_STATISTICS GstFluStatistics stats;

static inline void
gstflu_setup_statistics (GstPad *sink, GstFluStatistics *stats)
{
  GstQuery *q;
  GstPad *peer;
  GstFormat fmt;
  gint64 duration;

  /* 30 seconds in case we can't figure out the duration of the clip */
  stats->max_duration = GST_SECOND * 30;
  stats->decoded_duration = 0;
  peer = gst_pad_get_peer (sink);

  if (!peer)
    return;

  q = gst_query_new_duration (GST_FORMAT_TIME);
  if (!q) goto p_out;

  if (!gst_pad_query (peer, q))
    goto q_out;

  gst_query_parse_duration (q, &fmt, &duration);
  if (fmt == GST_FORMAT_TIME && GST_CLOCK_TIME_IS_VALID (duration)) {
    stats->max_duration = gst_util_uint64_scale_int (duration, DEMO_PERCENT, 100);
    /* max 30 minutes */
    if (stats->max_duration > (GST_SECOND * 60 * 30))
      stats->max_duration = GST_SECOND * 60 * 30;
    /* min 3 minutes */
    else if (stats->max_duration < (GST_SECOND * 60 * 3))
      stats->max_duration = GST_SECOND * 60 * 3;
  }

q_out:
  gst_query_unref (q);
p_out:
  gst_object_unref (peer);
}

static inline GstFlowReturn
gstflu_pad_push (GstPad *src, GstBuffer *out_buf, GstFluStatistics *stats)
{
  GstElement *element;

  element = gst_pad_get_parent_element (src);

  if (GST_STATE_PLAYING != GST_STATE (element)) {
    gst_object_unref (element);
    return gst_pad_push (src, out_buf);
  }

  stats->decoded_duration += GST_BUFFER_DURATION (out_buf);
  if (G_UNLIKELY (stats->decoded_duration >= stats->max_duration)) {
    gst_pad_push_event (src, gst_event_new_eos());
    GST_ELEMENT_ERROR (element, STREAM, FAILED,
        ("Fluendo decoders terminated playback of this media"
         " stream as this is an evaluation version of Fluendo's"
         " technology. To get a licensed copy of this Fluendo"
         " product please contact sales@fluendo.com."),
         (NULL));
    gst_object_unref (element);
    gst_buffer_unref (out_buf);
    return GST_FLOW_ERROR;
  }
  else {
    gst_object_unref (element);
    return gst_pad_push (src, out_buf);
  }
}

#else

#define GSTFLU_SETUP_STATISTICS(sink, stats)
#define GSTFLU_PAD_PUSH(src, buf, stats) gst_pad_push (src, buf)
#define GSTFLU_STATISTICS

#endif /* ENABLE_DEMO_PLUGIN */

#ifndef GST_COMPAT_H
#ifndef POST_10_14
#define gst_element_class_set_details_simple( \
    klass, longname, classification, description, author) \
    { \
      GstElementDetails details = { \
        longname, classification, description, author}; \
      gst_element_class_set_details (klass, &details); \
    }
#endif
#endif

#endif /* _GST_FLUENDO_H_ */
