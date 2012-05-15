/*
 * FLUENDO S.A.
 * Copyright (C) <2012>  <support@fluendo.com>
 */

#ifndef GST_COMPAT_H
#define GST_COMPAT_H
#include <gst/gst.h>

#ifndef GST_CHECK_VERSION
#define GST_CHECK_VERSION(major,minor,micro)  \
    (GST_VERSION_MAJOR > (major) || \
     (GST_VERSION_MAJOR == (major) && GST_VERSION_MINOR > (minor)) || \
     (GST_VERSION_MAJOR == (major) && GST_VERSION_MINOR == (minor) && \
      GST_VERSION_MICRO >= (micro)))
#endif

#if !GST_CHECK_VERSION(0,10,14)
#define gst_element_class_set_details_simple(klass,longname,classification,description,author) \
    G_STMT_START{                                                             \
      static GstElementDetails details =                                      \
          GST_ELEMENT_DETAILS (longname,classification,description,author);   \
      gst_element_class_set_details (element_class, &details);                \
    }G_STMT_END
#endif

#endif /* GST_COMPAT_H */
