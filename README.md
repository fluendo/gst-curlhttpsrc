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
* user-id: See CURLOPT_USERNAME
* user-pwd: See CURLOPT_PASSWORD
* proxy: See CURLOPT_PROXY
* proxyusername: See CURLOPT_PROXYUSERNAME
* proxypassword: See CURLOPT_PROXYPASSWORD
* cookies: See CURLOPT_COOKIELIST
* user-agent: See CURLOPT_USERAGENT 
* extra-headers: See CURLOPT_HTTPHEADERS
* compress: See CURLOPT_ACCEPT_ENCODING
* automatic-redirect: See CURLOPT_FOLLOWLOCATION
* maxredirect: See CURLOPT_MAXREDIRS
* keep-alive: See CURLOPT_TCP_KEEPALIVE
* timeout: See CURLOPT_TIMEOUT
* ssl-strict: See CURLOPT_SSL_VERIFYPEER
* ssl-ca-file: See CURLOPT_CAINFO
* retries: Not used
* max-connection-time: Not used
* max-connections-per-server: Not used
* max-connections-per-proxy: Not used
* max-connections: Not used
* httpversion: See CURLOPT_HTTP_VERSION
