#include "HTTPClient.h"
#include "yajl/yajl_tree.h"

namespace api {
  const char* BASE = "http://api.plantus.xyz/";
  const int BUF_SIZE = 1024;
  char str[BUF_SIZE];

  /**
    * GET request
    * @param [path] path to fetch
    * @param [identifier] place identifier
    * @return JSON root node
  */
  yajl_val get(const char* path, const char* identifier) {
    HTTPClient api(identifier);
    char uri[100];
    strcpy(uri, BASE);
    strcat(uri, path);
    printf("\r\nGET %s\r\n", uri);
    int ret = api.get(uri, str, HTTP_CLIENT_DEFAULT_TIMEOUT);
    if (!ret) {
      printf("Success %d bytes\r\n", strlen(str));
      printf("Result: %s\r\n", str);
    } else {
      printf("Error %d - HTTP status: %d\r\n", ret, api.getHTTPResponseCode());
    }

    char errbuf[BUF_SIZE];
    yajl_val node;
    node = yajl_tree_parse(str, errbuf, sizeof(errbuf));
    
    if (node == NULL) {
        printf("parse_error: ");
        if (strlen(errbuf)) {
          printf(" %s", errbuf);
        } else {
          printf("unknown error");
        }
        printf("\r\n");
    }
    return node;
  }

  /**
    * POST request
    * @param [path] path to fetch
    * @param [body] json body to send
    * @param [identifier] place identifier
    * @return JSON root node
  */
  void post(const char* path, char* body, const char* identifier) {
    HTTPClient api(identifier);
    char uri[100];
    strcpy(uri, BASE);
    strcat(uri, path);
    HTTPText dataOut(body);
    HTTPText dataIn(str, BUF_SIZE);
    printf("\r\nPOST %s\r\n", uri);
    int ret = api.post(uri, dataOut, &dataIn, HTTP_CLIENT_DEFAULT_TIMEOUT);
    if (!ret) {
      printf("Success %d bytes\r\n", strlen(str));
      printf("Result: %s\r\n", str);
    } else {
      printf("Error %d - HTTP status: %d\r\n", ret, api.getHTTPResponseCode());
    }
  }
}