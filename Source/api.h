#include "HTTPClient.h"
#include "Json.h"

namespace api {
  #define BUFFER_SIZE 1024

  struct Operation{
    char id[3];
    char potIdentifer[37];
  };

  const int maxOperationsLength = 3;
  const char* BASE = "http://api.plantus.xyz/";
  char buffer[BUFFER_SIZE];

  /**
    * GET request
    * @param [path] path to fetch
    * @param [identifier] place identifier
    * @return JSON root node
  */
  void get(const char* path, const char* identifier, Operation operations[maxOperationsLength]) {
    HTTPClient api(identifier);
    char uri[100];
    strcpy(uri, BASE);
    strcat(uri, path);
    printf("\r\nGET %s\r\n", uri);
    int ret = api.get(uri, buffer, HTTP_CLIENT_DEFAULT_TIMEOUT);
    if (!ret) {
      printf("Success %d bytes\r\n", strlen(buffer));
      //printf("Result: %s\r\n", buffer);
    } else {
      printf("Error %d - HTTP status: %d\r\n", ret, api.getHTTPResponseCode());
    }

    Json json(buffer, strlen(buffer), 64);
    if (!json.isValidJson() || json.type(0) != JSMN_OBJECT)
    {
        printf("Invalid JSON: %s", buffer);
        return;
    }
  
    int resultsKeyIndex = json.findKeyIndexIn("results");
    if ( resultsKeyIndex == -1 )
    {
        printf("\"results\" does not exist ... do something!!\r\n");
    } else {
        int resultsValueIndex = json.findChildIndexOf(resultsKeyIndex);
        int childIndexOffset = 0;
        int childIndex = 0;
        if (resultsValueIndex > 0)
        {
          // we create sub jsons for the max number of suported operations
          for(int i = 0; i < maxOperationsLength; i++) { // use define here
            strncpy(operations[i].id, "\0", 1);
            strncpy(operations[i].potIdentifer, "\0", 1);
            childIndex = json.findChildIndexOf (resultsValueIndex, childIndexOffset);
            //printf("childIndex %d\r\n", childIndex);
            
            if(childIndex > 0) {
              childIndexOffset = childIndex;
              const char * valueStart  = json.tokenAddress ( childIndex );
              int          valueLength = json.tokenLength ( childIndex );
              char operationBuffer [200];

              strncpy(operationBuffer, valueStart, valueLength);
              operationBuffer [ valueLength ] = 0; // NULL-terminate the string
              //printf("operationBuffer is %s\r\n", operationBuffer);
              Json operationJson(operationBuffer, strlen(operationBuffer));
              if (!operationJson.isValidJson() || operationJson.type(0) != JSMN_OBJECT)
              {
                  printf("Invalid JSON TEST: %s", operationBuffer);
                  return;
              }
              
              int potIdenfierKeyIndex = operationJson.findKeyIndexIn("pot_identifier", 0);
              if (potIdenfierKeyIndex == -1)
              {
                  printf("\"pot_identifier\" does not exist in the HTTP response");
                  return;
              }  else {
                  int potIdenfierValueIndex = operationJson.findChildIndexOf(potIdenfierKeyIndex);
                  if (potIdenfierValueIndex > 0)
                  {
                      // we have a pot identifier!
                      const char * valueStart  = operationJson.tokenAddress (potIdenfierValueIndex);
                      int          valueLength = operationJson.tokenLength (potIdenfierValueIndex);
                      
                      strncpy(operations[i].potIdentifer, valueStart, valueLength); 
                      operations[i].potIdentifer[valueLength] = 0; // NULL-terminate the string
                      printf("Pot identifier for operation %d is %s\r\n", i, operations[i].potIdentifer);
                  }
              }
              int operationIdKeyIndex = operationJson.findKeyIndexIn("id", 0);
              if (operationIdKeyIndex == -1)
              {
                  printf("\"id\" does not exist in the HTTP response");
                  return;
              }  else {
                  int operationIdValueIndex = operationJson.findChildIndexOf(operationIdKeyIndex);
                  if (operationIdValueIndex > 0)
                  {
                      // we have a id!
                      const char * valueStart  = operationJson.tokenAddress (operationIdValueIndex);
                      int          valueLength = operationJson.tokenLength  (operationIdValueIndex);
                                    
                      strncpy(operations[i].id, valueStart, valueLength); 
                      operations[i].id[valueLength] = 0; // NULL-terminate the string*/
                      printf("Operation ID for operation %d is %s\r\n", i, operations[i].id);
                  }
              }
            }
          }
        }
      }
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
    HTTPText dataIn(buffer, BUFFER_SIZE);
    printf("\r\nPOST %s\r\n", uri);
    int ret = api.post(uri, dataOut, &dataIn, HTTP_CLIENT_DEFAULT_TIMEOUT);
    if (!ret) {
      printf("Success %d bytes\r\n", strlen(buffer));
      printf("Result: %s\r\n", buffer);
    } else {
      printf("Error %d - HTTP status: %d\r\n", ret, api.getHTTPResponseCode());
    }
  }
}