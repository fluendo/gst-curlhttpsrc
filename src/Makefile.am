plugin_LTLIBRARIES = libgstcurlhttpsrc.la

# sources used to compile this plug-in
libgstcurlhttpsrc_la_SOURCES = \
gstcurlhttpsrc.c \
gstcurlhttpsrc.h \
gstcurlmulticontext.c \
gstcurlmulticontext.h \
gstcurlqueue.c \
curltask.h \
gstcurldefaults.h \
gstcurlqueue.h

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstcurlhttpsrc_la_CFLAGS = -I${top_srcdir}/common $(GST_CFLAGS) $(GST_BASE_CFLAGS)
libgstcurlhttpsrc_la_LIBADD = $(GST_LIBS) $(GST_BASE_LIBS) -lcurl
libgstcurlhttpsrc_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstcurlhttpsrc_la_LIBTOOLFLAGS = --tag=disable-static
