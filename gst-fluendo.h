#ifndef _INCLUDE_NOPIC_H_
#define _INCLUDE_NOPIC_H_

#ifdef ENABLE_STATIC_PLUGIN

#define FLUENDO_PLUGIN_DEFINE(major, minor, name, fun, desc, init, version, \
  license, pkg, url) \
    gboolean gst_##fun##_init_static() { \
      return gst_plugin_register_static(major, minor, name, desc, init, \
          version, license, "Fluendo",pkg,url); \
    }

#else

#define FLUENDO_PLUGIN_DEFINE(major, minor, name, fun, desc, init, version, \
  license,pkg,url) \
  GST_PLUGIN_DEFINE(major, minor, name, desc, init, version, license, pkg, url)

#endif


typedef struct _GstFluStatistics
{
	gint64 max_duration;
	gint64 decoded_duration;
} GstFluStatistics;

#if ENABLE_DEMO_PLUGIN

#define GSTFLU_SETUP_STATISTICS(sink, stats) gstflu_setup_statistics (sink, stats)
#define GSTFLU_PAD_PUSH(src, buf, stats) gstflu_pad_push (src, buf, stats)

static inline void gstflu_setup_statistics(GstPad *sink, GstFluStatistics *stats)
{
	GstQuery *q;
	GstPad *peer;
	gboolean res;

	stats->decoded_duration = 0;
	peer = gst_pad_get_peer (sink);

	if (!peer)
		return;

	q = gst_query_new_duration (GST_FORMAT_TIME);
	if (!q)
		return;

	res = gst_pad_query (peer, q);
	if (res) {
		gint64 duration;

		gst_query_parse_duration (q, NULL, &duration);
		stats->max_duration = DEMO_PERCENT * duration / 100;
	}
	else {
		stats->max_duration = G_GINT64_CONSTANT(30000000000); /* 30 seconds */
	}

	gst_query_unref (q);
	gst_object_unref (peer);
}

static inline gboolean gstflu_pad_push(GstPad *src, GstBuffer *out_buf, GstFluStatistics *stats)
{
	GstElement *element;
	GstState state;

	element = gst_pad_get_parent_element (src);
	gst_element_get_state (element, &state, NULL, 0);

	if (state != GST_STATE_PLAYING) {
		gst_object_unref (element);
		return gst_pad_push (src, out_buf);
	}

	stats->decoded_duration += GST_BUFFER_DURATION (out_buf);
	if (G_UNLIKELY(stats->decoded_duration >= stats->max_duration)) {
		gst_pad_push_event (src, gst_event_new_eos());
		GST_ELEMENT_ERROR (element, STREAM, FAILED,
				  ("Fluendo decoders terminated playback of this media"
				   " stream as this is an evaluation version of Fluendo's"
				   " technology. To get a licensed copy of this Fluendo"
				   " product please contact sales@fluendo.com."),
				  NULL);
		gst_object_unref (element);
		return FALSE;
	}
	else {
		gst_object_unref (element);
		return gst_pad_push (src, out_buf);
	}
}

#else

#define GSTFLU_SETUP_STATISTICS(sink, stats)
#define GSTFLU_PAD_PUSH(src, buf, stats) gst_pad_push (src, buf)

#endif /* ENABLE_DEMO_PLUGIN */

#endif /* _INCLUDE_NOPIC_H_ */
