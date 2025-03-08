
## CURL options:
-v           # --verbose
-s           # --silent: don't show progress meter or errors

## CURL Examples 

Head request with verbose	
curl -v -I https://localhost:8080

Curl with explicit http method	
curl -X GET https://localhost:8080

With http proxy	
curl -x GET mysite.com --proxy yourproxy:port

Curl with timeout
curl --connect-timeout 10 -I -k https://www.google.com

Send a get request
curl -X GET http://localhost:8080/blogs

Send a post request with JSON Data
curl -d "param1=value1&param2=value2" -H "Content-Type: application/json" -X POST http://localhost:8080/

Send a file with PUT request
curl -d "@test.png" -X PUT http://localhost:8080/upload

Upload a file
curl -v -F filename=test.png -F upload=@localfilename http://localhost:8080/upload