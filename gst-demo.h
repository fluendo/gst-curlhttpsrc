#ifndef _GST_DEMO_H_
#define _GST_DEMO_H_

#ifdef POST_1_0
#include <gst/video/gstvideodecoder.h>
#endif

typedef struct _GstFluDemoStatistics {
  gint64 max_duration;
  gint64 decoded_duration;
} GstFluDemoStatistics;

static inline void
gstflu_demo_reset_statistics (GstFluDemoStatistics * stats)
{
  stats->max_duration = GST_SECOND * 30;
  stats->decoded_duration = GST_CLOCK_TIME_NONE;
}

static inline void
gstflu_demo_setup_statistics (GstFluDemoStatistics * stats, GstPad * sink)
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
gstflu_demo_check_buffer (GstFluDemoStatistics * stats, GstPad * sink,
    GstPad * src, GstBuffer * buf, GstClockTime duration)
{
#if ENABLE_DEMO_PLUGIN
  GstElement *element;

  element = gst_pad_get_parent_element (src);

  if (GST_STATE_PLAYING != GST_STATE (element)) {
    gst_object_unref (element);
    return GST_FLOW_OK;
  }

  if (!GST_CLOCK_TIME_IS_VALID (stats->decoded_duration)) {
    gstflu_demo_setup_statistics (stats, sink);
    stats->decoded_duration = 0;
  }

  stats->decoded_duration += duration;
  if (G_UNLIKELY (stats->decoded_duration >= stats->max_duration)) {
    gst_pad_push_event (src, gst_event_new_eos());
    GST_ELEMENT_ERROR (element, STREAM, FAILED,
        ("Fluendo decoders terminated playback of this media"
         " stream as this is an evaluation version of Fluendo's"
         " technology. To get a licensed copy of this Fluendo"
         " product please contact sales@fluendo.com."),
         (NULL));
    gst_object_unref (element);
    gst_buffer_unref (buf);
    return GST_FLOW_ERROR;
  } else {
    gst_object_unref (element);
    return GST_FLOW_OK;
  }
#else
  return GST_FLOW_OK;
#endif
}

static inline GstFlowReturn
gstflu_demo_check_video_buffer (GstFluDemoStatistics * stats, GstPad * sink,
    GstPad * src, GstBuffer * buf, gint fps_n, gint fps_d)
{
#if ENABLE_DEMO_PLUGIN
  return gstflu_demo_check_buffer (stats, sink, src, buf, gst_util_uint64_scale_int (fps_d, GST_SECOND, fps_n));
#else
  return GST_FLOW_OK;
#endif
}

static inline GstFlowReturn
gstflu_demo_check_audio_buffer (GstFluDemoStatistics * stats, GstPad * sink,
    GstPad * src, GstBuffer * buf, gint rate, gint channels, gint width)
{
#if ENABLE_DEMO_PLUGIN
  gint bs = channels * rate * (width >> 3);
  gsize size = gst_buffer_get_size (buf);

  return gstflu_demo_check_buffer (stats, sink, src, buf,
      gst_util_uint64_scale_int (size, GST_SECOND, bs));
#else
  return GST_FLOW_OK;
#endif
}

#ifdef POST_1_0
static inline GstFlowReturn
gstflu_demo_check_video_frame (GstFluDemoStatistics * stats,
    GstPad * sink, GstPad * src, GstVideoCodecFrame * frame)
{
#if ENABLE_DEMO_PLUGIN
  GstFlowReturn ret;

  /* we need to pass a ref on the buffer because on error the buffer
   * is unreffed
   */
  ret = gstflu_demo_check_buffer (stats, sink, src,
      gst_buffer_ref (frame->output_buffer), frame->duration);
  if (ret == GST_FLOW_OK)
    gst_buffer_unref (frame->output_buffer);
  return ret;
#else
  return GST_FLOW_OK;
#endif
}

static inline GstFlowReturn
gstflu_demo_check_video_decoder_frame (GstFluDemoStatistics * stats,
    GstVideoDecoder * dec, GstVideoCodecFrame * frame)
{
#if ENABLE_DEMO_PLUGIN

  return gstflu_demo_check_video_frame (stats,
      GST_VIDEO_DECODER_SINK_PAD (dec),
      GST_VIDEO_DECODER_SRC_PAD (dec),
      frame);
#else
  return GST_FLOW_OK;
#endif
}
#endif

#endif
