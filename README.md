# GstCurlHttpSrc

Fluendo's  GstCurlHttpSrc is a fork of [GstCurlHttpSrc](https://github.com/bbc/gst-curlhttpsrc)
It provides several features not found on the original plugin:

* The original plugin was downloading the whole content before sending downstream the single buffer.
  Now, as far as a new buffer arrives from cURL it is sent downstream
* The queue implementation was removed as it is no longer needed. In order to communicate
  between cURL and GStreamer we just use cond/mutex
* Support for GStreamer 0.10

## Here is the list properties the source element provides
* uri:
* username:
* password:
* proxyuri:
* proxyusername:
* proxypassword:
* cookies:
* useragent:
* headers:
* compress:
* redirect:
* maxredirect:
* keepalive:
* timeout:
* strict_ssl:
* ssl_ca_file:
* retries:
* connectionmaxtime:
* maxconcurrent_server:
* maxconcurrent_proxy:
* maxconcurrent_global:
* httpversion:
