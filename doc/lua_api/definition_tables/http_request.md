`HTTPRequest` definition
------------------------

Used by `HTTPApiTable.fetch` and `HTTPApiTable.fetch_async`.

```lua
{
    url = "http://example.org",

    timeout = 10,
    -- Timeout for request to be completed in seconds. Default depends on engine settings.

    method = "GET", "POST", "PUT" or "DELETE"
    -- The http method to use. Defaults to "GET".

    data = "Raw request data string" OR {field1 = "data1", field2 = "data2"},
    -- Data for the POST, PUT or DELETE request.
    -- Accepts both a string and a table. If a table is specified, encodes
    -- table as x-www-form-urlencoded key-value pairs.

    user_agent = "ExampleUserAgent",
    -- Optional, if specified replaces the default Luanti user agent with
    -- given string

    extra_headers = { "Accept-Language: en-us", "Accept-Charset: utf-8" },
    -- Optional, if specified adds additional headers to the HTTP request.
    -- You must make sure that the header strings follow HTTP specification
    -- ("Key: Value").

    multipart = boolean
    -- Optional, if true performs a multipart HTTP request.
    -- Default is false.
    -- Post only, data must be array

    post_data = "Raw POST request data string" OR {field1 = "data1", field2 = "data2"},
    -- Deprecated, use `data` instead. Forces `method = "POST"`.
}
```

`HTTPRequestResult` definition
------------------------------

Passed to `HTTPApiTable.fetch` callback. Returned by
`HTTPApiTable.fetch_async_get`.

```lua
{
    completed = true,
    -- If true, the request has finished (either succeeded, failed or timed
    -- out)

    succeeded = true,
    -- If true, the request was successful

    timeout = false,
    -- If true, the request timed out

    code = 200,
    -- HTTP status code

    data = "response"
}
```
