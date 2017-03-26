#include "HTTPClient.h"
#include "yajl/yajl_tree.h"

namespace api {
  const char* BASE = "http://api.plantus.xyz/";
  const int BUF_SIZE = 1024;
  char str[BUF_SIZE];

  /**
    * GET request
    * @param [path] path to fetch
    * @return JSON root node
  */
  yajl_val get(const char* path) {
    HTTPClient api;
    char uri[100];
    strcpy(uri, BASE);
    strcat(uri, path);
    printf("\r\nGET %s\r\n", uri);
    int ret = api.get(uri, str, BUF_SIZE);
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
}