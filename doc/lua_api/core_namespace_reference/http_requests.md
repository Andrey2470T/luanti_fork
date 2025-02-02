HTTP Requests
=============

* `core.request_http_api()`:
    * returns `HTTPApiTable` containing http functions if the calling mod has
      been granted access by being listed in the `secure.http_mods` or
      `secure.trusted_mods` setting, otherwise returns `nil`.
    * The returned table contains the functions `fetch`, `fetch_async` and
      `fetch_async_get` described below.
    * Only works at init time and must be called from the mod's main scope
      (not from a function).
    * Function only exists if Luanti server was built with cURL support.
    * **DO NOT ALLOW ANY OTHER MODS TO ACCESS THE RETURNED TABLE, STORE IT IN
      A LOCAL VARIABLE!**
* `HTTPApiTable.fetch(HTTPRequest req, callback)`
    * Performs given request asynchronously and calls callback upon completion
    * callback: `function(HTTPRequestResult res)`
    * Use this HTTP function if you are unsure, the others are for advanced use
* `HTTPApiTable.fetch_async(HTTPRequest req)`: returns handle
    * Performs given request asynchronously and returns handle for
      `HTTPApiTable.fetch_async_get`
* `HTTPApiTable.fetch_async_get(handle)`: returns HTTPRequestResult
    * Return response data for given asynchronous HTTP request
