#pragma once
#include "HTTPClient.h"
#include "Json.h"

// TODO put code in CPP and maybe create a class?
namespace api {
  #define BUFFER_SIZE 1024
  #define OPERATION_ID_MAX_LENGTH 4
  #define POT_IDENTIFIER_LENGTH 37
  #define OPERATIONS_MAX_LENTH 3
  #define ACTION_MAX_LENGTH 30

  struct Operation{
    char id[OPERATION_ID_MAX_LENGTH];
    char action[ACTION_MAX_LENGTH];
    char potIdentifer[POT_IDENTIFIER_LENGTH];
  };

  const int maxOperationsLength = OPERATIONS_MAX_LENTH;
  const char* BASE = "http://api.plantus.xyz/";
  const char* ethernetAdapterIP = "http://192.168.0.1/";
  char buffer[BUFFER_SIZE];
  void getOperationsFromResponse(char response[], Operation operations[maxOperationsLength]);

  void get(const char* path, const char* identifier, Operation operations[maxOperationsLength], Thread* ptrGetOperationsThread, bool staticAddress) {
    HTTPClient api(identifier);
    char uri[100];
    staticAddress ? strcpy(uri, ethernetAdapterIP) : strcpy(uri, BASE);    
    strcat(uri, path);
    printf("\r\nGET %s\r\n", uri);
    int ret = api.get(uri, buffer, HTTP_CLIENT_DEFAULT_TIMEOUT);
    if (!ret) {
      printf("Success %d bytes\r\n", strlen(buffer));
      getOperationsFromResponse(buffer, operations);
    } else {
      printf("Error %d - HTTP status: %d\r\n", ret, api.getHTTPResponseCode());
      for(int i = 0; i < maxOperationsLength; i++) { 
        // reset values
        strncpy(operations[i].id, "\0", 1);
        strncpy(operations[i].potIdentifer, "\0", 1);
        strncpy(operations[i].action, "\0", 1);
      }
    }
      printf("Operations thread Stack Size: '%d'\r\n", ptrGetOperationsThread->stack_size());
      printf("Operations thread Max Stack: '%d'\r\n", ptrGetOperationsThread->max_stack());
      printf("Operations thread 'free' Stack: '%d'\r\n", ptrGetOperationsThread->stack_size() - ptrGetOperationsThread->max_stack());
   }



  // TODO change body for HTTP map https://developer.mbed.org/teams/Avnet/code/WNCInterface_HTTP_example/docs/tip/main_8cpp_source.html?
  void post(const char* path, char* body, const char* identifier, bool staticAddress) {
    HTTPClient api(identifier);
    char uri[100];
    staticAddress ? strcpy(uri, ethernetAdapterIP) : strcpy(uri, BASE); 
    strcat(uri, path);
    HTTPText dataOut(body);
    HTTPText dataIn(buffer, BUFFER_SIZE);
    printf("\r\nPOST %s\r\n", uri);
    int ret = api.post(uri, dataOut, &dataIn, HTTP_CLIENT_DEFAULT_TIMEOUT);
    if (!ret) {
      printf("Success %d bytes\r\n", strlen(buffer));
      //printf("Result: %s\r\n", buffer);
    } else {
      printf("Error %d - HTTP status: %d\r\n", ret, api.getHTTPResponseCode());
    }
  }


  void getOperationsFromResponse(char response[], Operation operations[maxOperationsLength]) {
    Json json(response, strlen(response), 60);
    if (!json.isValidJson() || json.type(0) != JSMN_OBJECT) {
        printf("Invalid JSON: %s", response);
        return;
    }

    int resultsKeyIndex = json.findKeyIndexIn("results");
    if ( resultsKeyIndex == -1 ) {
        printf("\"results\" does not exist ... do something!!\r\n");
    } else {
        int resultsValueIndex = json.findChildIndexOf(resultsKeyIndex);
        int childIndexOffset = 0;
        int childIndex = 0;
        if (resultsValueIndex > 0) {
          // we create sub jsons for the max number of suported operations
          for(int i = 0; i < maxOperationsLength; i++) { 
            // initialize values
            strncpy(operations[i].id, "\0", 1);
            strncpy(operations[i].potIdentifer, "\0", 1);
            strncpy(operations[i].action, "\0", 1);
            childIndex = json.findChildIndexOf (resultsValueIndex, childIndexOffset);
            
            if(childIndex > 0) {
              childIndexOffset = childIndex;
              const char * valueStart  = json.tokenAddress ( childIndex );
              int          valueLength = json.tokenLength ( childIndex );
              char operationBuffer [200];

              strncpy(operationBuffer, valueStart, valueLength);
              operationBuffer [ valueLength ] = 0; // NULL-terminate the string
              Json operationJson(operationBuffer, strlen(operationBuffer));
              if (!operationJson.isValidJson() || operationJson.type(0) != JSMN_OBJECT) {
                  printf("Invalid JSON TEST: %s", operationBuffer);
                  return;
              }
              
              int potIdenfierKeyIndex = operationJson.findKeyIndexIn("pot_identifier", 0);
              if (potIdenfierKeyIndex == -1) {
                  printf("\"pot_identifier\" does not exist in the HTTP response");
                  return;
              }  else {
                  int potIdenfierValueIndex = operationJson.findChildIndexOf(potIdenfierKeyIndex);
                  if (potIdenfierValueIndex > 0) {
                      // we have a pot identifier!
                      const char * valueStart  = operationJson.tokenAddress (potIdenfierValueIndex);
                      int          valueLength = operationJson.tokenLength (potIdenfierValueIndex);
                      
                      strncpy(operations[i].potIdentifer, valueStart, valueLength); 
                      operations[i].potIdentifer[valueLength] = 0; // NULL-terminate the string
                      printf("Pot identifier for operation %d is %s\r\n", i, operations[i].potIdentifer);
                  }
              }
              int operationIdKeyIndex = operationJson.findKeyIndexIn("id", 0);
              if (operationIdKeyIndex == -1) {
                  printf("\"id\" does not exist in the HTTP response");
                  return;
              }  else {
                  int operationIdValueIndex = operationJson.findChildIndexOf(operationIdKeyIndex);
                  if (operationIdValueIndex > 0) {
                      // we have a id!
                      const char * valueStart  = operationJson.tokenAddress (operationIdValueIndex);
                      int          valueLength = operationJson.tokenLength  (operationIdValueIndex);
                                    
                      strncpy(operations[i].id, valueStart, valueLength); 
                      operations[i].id[valueLength] = 0; // NULL-terminate the string
                      printf("Operation ID for operation %d is %s\r\n", i, operations[i].id);
                  }
              }
              int actionKeyIndex = operationJson.findKeyIndexIn("action", 0);
              if (operationIdKeyIndex == -1) {
                  printf("\"action\" does not exist in the HTTP response");
                  return;
              }  else {
                  int actionValueIndex = operationJson.findChildIndexOf(actionKeyIndex);
                  if (actionValueIndex > 0) {
                      // we have a action!
                      const char * valueStart  = operationJson.tokenAddress (actionValueIndex);
                      int          valueLength = operationJson.tokenLength  (actionValueIndex);
                                    
                      strncpy(operations[i].action, valueStart, valueLength); 
                      operations[i].action[valueLength] = 0; // NULL-terminate the string
                      printf("Operation action for operation %d is %s\r\n", i, operations[i].action);
                  }
              }
            }
          }
        }
      }
    }
}