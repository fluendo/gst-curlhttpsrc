# GstCurlHttpSrc

Fluendo's  GstCurlHttpSrc is a fork of [GstCurlHttpSrc](https://github.com/bbc/gst-curlhttpsrc)
It provides several features not found on the original plugin:

* The original plugin was downloading the whole content before sending downstream the single buffer.
  Now, as far as a new buffer arrives from cURL it is sent downstream
* The queue implementation was removed as it is no longer needed. In order to communicate
  between cURL and GStreamer we just use cond/mutex
* Support for GStreamer 0.10
* Support for seeking

## Properties
* location: The url to download
* user-id: See [CURLOPT_USERNAME](http://curl.haxx.se/libcurl/c/CURLOPT_USERNAME.html)
* user-pwd: See [CURLOPT_PASSWORD](http://curl.haxx.se/libcurl/c/CURLOPT_PASSWORD.html)
* proxy: See [CURLOPT_PROXY](http://curl.haxx.se/libcurl/c/CURLOPT_PROXY.html)
* proxyusername: See [CURLOPT_PROXYUSERNAME](http://curl.haxx.se/libcurl/c/CURLOPT_PROXYUSERNAME.html)
* proxypassword: See [CURLOPT_PROXYPASSWORD](http://curl.haxx.se/libcurl/c/CURLOPT_PROXYPASSWORD.html)
* cookies: See [CURLOPT_COOKIELIST](http://curl.haxx.se/libcurl/c/CURLOPT_COOKIELIST.html)
* user-agent: See [CURLOPT_USERAGENT](http://curl.haxx.se/libcurl/c/CURLOPT_USERAGENT.html)
* extra-headers: See [CURLOPT_HTTPHEADERS](http://curl.haxx.se/libcurl/c/CURLOPT_HTTPHEADERS.html)
* compress: See [CURLOPT_ACCEPT_ENCODING](http://curl.haxx.se/libcurl/c/CURLOPT_ACCEPT_ENCODING.html)
* automatic-redirect: See [CURLOPT_FOLLOWLOCATION](http://curl.haxx.se/libcurl/c/CURLOPT_FOLLOWLOCATION.html)
* maxredirect: See [CURLOPT_MAXREDIRS](http://curl.haxx.se/libcurl/c/CURLOPT_MAXREDIRS.html)
* keep-alive: See [CURLOPT_TCP_KEEPALIVE](http://curl.haxx.se/libcurl/c/CURLOPT_TCP_KEEPALIVE.html)
* timeout: See [CURLOPT_TIMEOUT](http://curl.haxx.se/libcurl/c/CURLOPT_TIMEOUT.html)
* ssl-strict: See [CURLOPT_SSL_VERIFYPEER](http://curl.haxx.se/libcurl/c/CURLOPT_SSL_VERIFYPEER.html)
* ssl-ca-file: See [CURLOPT_CAINFO](http://curl.haxx.se/libcurl/c/CURLOPT_CAINFO.html)
* retries: Not used
* max-connection-time: Not used
* max-connections-per-server: Not used
* max-connections-per-proxy: Not used
* max-connections: Not used
* httpversion: See [CURLOPT_HTTP_VERSION](http://curl.haxx.se/libcurl/c/CURLOPT_HTTP_VERSION.html)
