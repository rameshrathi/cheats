# curl Cheat Sheet

This cheat sheet provides common `curl` commands for interacting with web servers and APIs, focusing on GET, POST, and PUT requests, along with viewing detailed request/response information.

**Placeholders Used:**

* `https://example.com/api/resource`: Replace with the target URL.
* `key=value`: Replace with form data parameters.
* `'{"key": "value"}'`: Replace with JSON data.
* `YOUR_TOKEN`: Replace with your actual authentication token.
* `output.txt`, `output.json`: Replace with desired output filenames.
* `/path/to/file.json`: Replace with the path to a local file containing data.

---

## Basic GET Requests

* **Simple GET:** Fetch the content of a URL.
    ```bash
    curl [https://example.com/api/resource](https://example.com/api/resource)
    ```

* **GET with Query Parameters:** Append parameters directly to the URL.
    ```bash
    curl "[https://example.com/api/search?query=keyword&limit=10](https://example.com/api/search?query=keyword&limit=10)"
    ```
    *(Note: Quoting the URL is recommended if it contains special characters like `&`)*

---

## Viewing Request/Response Details

* **Show Response Headers (-i):** Include the HTTP response headers in the output.
    ```bash
    curl -i [https://example.com/api/resource](https://example.com/api/resource)
    ```

* **Verbose Output (-v):** Show detailed information about the connection, request headers sent by curl, and response headers received. Excellent for debugging.
    ```bash
    curl -v [https://example.com/api/resource](https://example.com/api/resource)
    ```
    *(Output goes to stderr, response body goes to stdout)*

* **Very Verbose Output (--trace-ascii):** Dumps *all* incoming and outgoing data (including headers and data) in ASCII format to the specified file or stderr.
    ```bash
    curl --trace-ascii trace_log.txt [https://example.com/api/resource](https://example.com/api/resource)
    # Or to stderr
    curl --trace-ascii - [https://example.com/api/resource](https://example.com/api/resource)
    ```

---

## POST Requests

* **Simple POST (Form URL Encoded):** Send data as `application/x-www-form-urlencoded` (default for `-d`).
    ```bash
    curl -X POST -d "param1=value1&param2=value2" [https://example.com/api/resource](https://example.com/api/resource)
    ```
    * `-X POST`: Specifies the request method (though often inferred by `-d`).
    * `-d "..."`: Sends the specified data in the request body.

* **POST with Explicit Content-Type (Form URL Encoded):**
    ```bash
    curl -X POST -H "Content-Type: application/x-www-form-urlencoded" \
      -d "param1=value1&param2=value2" \
      [https://example.com/api/resource](https://example.com/api/resource)
    ```
    * `-H "Header: Value"`: Adds a custom request header.

* **POST JSON Data:**
    ```bash
    curl -X POST -H "Content-Type: application/json" \
      -d '{"key": "value", "another_key": 123}' \
      [https://example.com/api/resource](https://example.com/api/resource)
    ```
    *(Note: Use single quotes around the JSON data to prevent shell interpretation of double quotes within the JSON.)*

* **POST JSON Data (using --json, modern curl):** Simpler way to send JSON and automatically set `Content-Type: application/json` and `Accept: application/json`.
    ```bash
    curl -X POST --json '{"key": "value", "another_key": 123}' [https://example.com/api/resource](https://example.com/api/resource)
    # Alternatively, can be combined with -d
    curl -X POST --json @- [https://example.com/api/resource](https://example.com/api/resource) <<EOF
    {
      "key": "value",
      "another_key": 123
    }
    EOF
    ```


* **POST Data from a File:** Send the contents of a file as the request body.
    ```bash
    # Send raw file content (e.g., for JSON file)
    curl -X POST -H "Content-Type: application/json" \
      -d @/path/to/file.json \
      [https://example.com/api/resource](https://example.com/api/resource)

    # Send form data from file content (rare)
    curl -X POST -d @/path/to/formdata.txt [https://example.com/api/resource](https://example.com/api/resource)
    ```
    * `-d @filename`: Reads data from the specified file.

* **POST using URL Encoding (--data-urlencode):** Useful for easily encoding special characters in form data.
    ```bash
    curl -X POST --data-urlencode "comment=This has spaces & special chars!" \
      --data-urlencode "user=some user" \
      [https://example.com/api/resource](https://example.com/api/resource)
    ```

---

## PUT Requests

PUT is often used to update an existing resource or create a resource at a specific known URL. Syntax is similar to POST.

* **PUT JSON Data:**
    ```bash
    curl -X PUT -H "Content-Type: application/json" \
      -d '{"key": "updated_value"}' \
      [https://example.com/api/resource/123](https://example.com/api/resource/123)
    ```
    * `-X PUT`: Specifies the PUT method.

* **PUT Data from File:**
    ```bash
    curl -X PUT -H "Content-Type: application/json" \
      -d @/path/to/updated_data.json \
      [https://example.com/api/resource/123](https://example.com/api/resource/123)
    ```

* **PUT using --json (modern curl):**
    ```bash
    curl -X PUT --json '{"key": "updated_value"}' [https://example.com/api/resource/123](https://example.com/api/resource/123)
    ```

---

## Common Options

* **Specify Request Method (-X):** Explicitly set the HTTP method (GET, POST, PUT, DELETE, PATCH, etc.).
    ```bash
    curl -X DELETE [https://example.com/api/resource/123](https://example.com/api/resource/123)
    ```

* **Add Custom Headers (-H):** Add any HTTP header to the request. Can be used multiple times.
    ```bash
    # Example: Adding Authorization Bearer token and Accept header
    curl -H "Authorization: Bearer YOUR_TOKEN" \
         -H "Accept: application/json" \
         [https://example.com/api/resource](https://example.com/api/resource)
    ```

* **Follow Redirects (-L):** Follow HTTP 3xx redirects.
    ```bash
    curl -L [https://example.com/redirecting-url](https://example.com/redirecting-url)
    ```

* **Save Output to File (-o / -O):**
    * `-o filename`: Save the response body to the specified file.
        ```bash
        curl -o output.json [https://example.com/api/resource](https://example.com/api/resource)
        ```
    * `-O`: Save the response body to a local file named after the remote file (extracted from the URL).
        ```bash
        curl -O [https://example.com/path/to/downloadable_file.zip](https://example.com/path/to/downloadable_file.zip)
        ```

* **Show Only Response Headers (-I):** Fetch only the HTTP headers (uses a HEAD request).
    ```bash
    curl -I [https://example.com/api/resource](https://example.com/api/resource)
    ```

* **Ignore SSL Certificate Issues (-k / --insecure):** Use with caution for testing/development against servers with self-signed or invalid certificates. **Do not use in production.**
    ```bash
    curl -k [https://self-signed.local/api](https://self-signed.local/api)
    ```

* **Set Timeout (--connect-timeout / --max-time):**
    * `--connect-timeout seconds`: Max time allowed for connection.
    * `--max-time seconds`: Max total time allowed for the operation.
    ```bash
    curl --connect-timeout 5 --max-time 30 [https://example.com/api/resource](https://example.com/api/resource)
    ```

---

**Further Help:**

For more advanced options and details, consult the `curl` manual page:
```bash
man curl
# Or get basic help
curl --help
