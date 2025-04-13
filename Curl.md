cURL Cheat SheetA quick reference for common cURL commands.Basic RequestsGET Request:curl https://example.com/api/data
POST Request:curl -X POST -d "param1=value1&param2=value2" https://example.com/api/data
or with JSON data:curl -X POST -H "Content-Type: application/json" -d '{"param1": "value1", "param2": "value2"}' https://example.com/api/data
PUT Request:curl -X PUT -d "param1=value1&param2=value2" https://example.com/api/data/123
or with JSON:curl -X PUT -H "Content-Type: application/json" -d '{"param1": "value1", "param2": "value2"}' https://example.com/api/data/123
DELETE Request:curl -X DELETE https://example.com/api/data/123
Request HeadersSetting Headers:curl -H "Content-Type: application/json" -H "Authorization: Bearer YOUR_TOKEN" https://example.com/api/data
Response HeadersIncluding Response Headers in Output:curl -i https://example.com/api/data
(Shows headers and body)Getting Only Response Headers:curl -I https://example.com/api/data
(Capital -I)Verbose OutputGetting Detailed Information (Request/Response):curl -v https://example.com/api/data
(Shows a lot of details, including headers, SSL info, etc.)File UploadUploading a File:curl -X POST -F "file=@/path/to/your/file.txt" https://example.com/api/upload
For multiple files:curl -X POST -F "file1=@/path/to/file1.txt" -F "file2=@/path/to/file2.jpg" https://example.com/upload
Output to FileSaving Output to a File:curl https://example.com/api/data -o output.txt
orcurl https://example.com/api/data > output.txt
AuthenticationBasic Authentication:curl -u "username:password" https://example.com/api/protected
SSL/TLSInsecure (Don't verify SSL certificate):curl -k https://example.com/api/data
(Use with caution!)ProxyUsing a Proxy:curl -x http://proxy.example.com:8080 https://example.com/api/data
Common Options-X:  Specify request method (GET, POST, PUT, DELETE, etc.)-H:  Set a header.-d:  Send data in the request body.  For POST/PUT.-i:  Include HTTP headers in the output.-I:  Show only the response headers.-v:  Verbose output.  Show details.-o:  Save the output to a file.-u:  Specify username:password for authentication.-k:  Allow insecure SSL connections.-x:  Use a proxy.-F:  Used for form data, especially file uploads. @ tells curl it's a file.Important Notes:This is a cheat sheet.  curl has many more options.  See man curl for full documentation.Be careful with the -k option.  It disables security checks.For JSON data, always set the Content-Type header.
