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

#endif
