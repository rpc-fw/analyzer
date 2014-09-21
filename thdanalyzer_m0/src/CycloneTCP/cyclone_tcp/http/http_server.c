/**
 * @file http_server.c
 * @brief HTTP server (HyperText Transfer Protocol)
 *
 * @section License
 *
 * Copyright (C) 2010-2014 Oryx Embedded. All rights reserved.
 *
 * This file is part of CycloneTCP Open.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @section Description
 *
 * Using the HyperText Transfer Protocol, the HTTP server delivers web pages
 * to browsers as well as other data files to web-based applications. Refers
 * to the following RFCs for complete details:
 * - RFC 1945 : Hypertext Transfer Protocol - HTTP/1.0
 * - RFC 2616 : Hypertext Transfer Protocol - HTTP/1.1
 * - RFC 2617 : HTTP Authentication: Basic and Digest Access Authentication
 * - RFC 2818 : HTTP Over TLS
 *
 * @author Oryx Embedded (www.oryx-embedded.com)
 * @version 1.4.4
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL HTTP_TRACE_LEVEL

//Dependencies
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include "tcp_ip_stack.h"
#include "http_server.h"
#include "mime.h"
#include "ssi.h"
#include "str.h"
#include "path.h"
#include "debug.h"

//File system support?
#if (HTTP_SERVER_FS_SUPPORT == ENABLED)
   #include "fs_port.h"
#else
   #include "resource_manager.h"
#endif

//Check TCP/IP stack configuration
#if (HTTP_SERVER_SUPPORT == ENABLED)


/**
 * @brief HTTP status codes
 **/

static const HttpStatusCodeDesc statusCodeList[] =
{
   //Success
   {200, "OK"},
   {201, "Created"},
   {202, "Accepted"},
   {204, "No Content"},
   //Redirection
   {301, "Moved Permanently"},
   {302, "Found"},
   {304, "Not Modified"},
   //Client error
   {400, "Bad Request"},
   {401, "Unauthorized"},
   {403, "Forbidden"},
   {404, "Not Found"},
   //Server error
   {500, "Internal Server Error"},
   {501, "Not Implemented"},
   {502, "Bad Gateway"},
   {503, "Service Unavailable"}
};


/**
 * @brief Initialize settings with default values
 * @param[out] settings Structure that contains HTTP server settings
 **/

void httpServerGetDefaultSettings(HttpServerSettings *settings)
{
   //Use default interface
   settings->interface = NULL;
   //Listen to port 80
   settings->port = HTTP_PORT;
   //Specify the server's root directory
   strcpy(settings->rootDirectory, "/");
   //Set default home page
   strcpy(settings->defaultDocument, "index.htm");

#if (HTTP_SERVER_DIGEST_AUTH_SUPPORT == ENABLED)
   //Pseudo-random number generator
   settings->prngAlgo = NULL;
   settings->prngContext = NULL;
#endif

#if (HTTP_SERVER_TLS_SUPPORT == ENABLED)
   //SSL/TLS initialization callback function
   settings->tlsInitCallback = NULL;
#endif

#if (HTTP_SERVER_BASIC_AUTH_SUPPORT == ENABLED || HTTP_SERVER_DIGEST_AUTH_SUPPORT == ENABLED)
   //HTTP authentication callback function
   settings->authCallback = NULL;
#endif

   //CGI callback function
   settings->cgiCallback = NULL;
   //URI not found callback function
   settings->uriNotFoundCallback = NULL;
}


/**
 * @brief HTTP server initialization
 * @param[in] context Pointer to the HTTP server context
 * @param[in] settings HTTP server specific settings
 * @return Error code
 **/

error_t httpServerInit(HttpServerContext *context, const HttpServerSettings *settings)
{
   error_t error;
   OsTask *task;

   //Debug message
   TRACE_INFO("Initializing HTTP server...\r\n");

   //Ensure the parameters are valid
   if(!context || !settings)
      return ERROR_INVALID_PARAMETER;

   //Clear the HTTP server context
   memset(context, 0, sizeof(HttpServerContext));
   //Save user settings
   context->settings = *settings;

   //Start of exception handling block
   do
   {
      //Create a semaphore to limit the number of simultaneous connections
      context->semaphore = osSemaphoreCreate(HTTP_SERVER_MAX_CONNECTIONS,
         HTTP_SERVER_MAX_CONNECTIONS);
      //Out of resources?
      if(context->semaphore == OS_INVALID_HANDLE)
      {
         //Report an error
         error = ERROR_OUT_OF_RESOURCES;
         //Exit immediately
         break;
      }

#if (HTTP_SERVER_DIGEST_AUTH_SUPPORT == ENABLED)
      //Create a mutex to prevent simutaneous access to the nonce cache
      context->nonceCacheMutex = osMutexCreate(FALSE);
      //Out of resources?
      if(context->nonceCacheMutex == OS_INVALID_HANDLE)
      {
         //Report an error
         error = ERROR_OUT_OF_RESOURCES;
         //Exit immediately
         break;
      }
#endif

      //Open a TCP socket
      context->socket = socketOpen(SOCKET_TYPE_STREAM, SOCKET_IP_PROTO_TCP);
      //Failed to open socket?
      if(!context->socket)
      {
         //Report an error
         error = ERROR_OPEN_FAILED;
         //Exit immediately
         break;
      }

      //Set timeout for blocking functions
      error = socketSetTimeout(context->socket, INFINITE_DELAY);
      //Any error to report?
      if(error) break;

      //Associate the socket with the relevant interface
      error = socketBindToInterface(context->socket, settings->interface);
      //Unable to bind the socket to the desired interface?
      if(error) break;

      //Bind newly created socket to port 80
      error = socketBind(context->socket, &IP_ADDR_ANY, settings->port);
      //Failed to bind socket to port 80?
      if(error) break;

      //Place socket in listening state
      error = socketListen(context->socket);
      //Any failure to report?
      if(error) break;

      //Create the HTTP server task
      task = osTaskCreate("HTTP Listener", httpListenerTask,
         context, HTTP_SERVER_STACK_SIZE, HTTP_SERVER_PRIORITY);

      //Unable to create the task?
      if(task == OS_INVALID_HANDLE)
         error = ERROR_OUT_OF_RESOURCES;

   //End of exception handling block
   } while(0);

   //Did we encounter an error?
   if(error)
   {
      //Free previously allocated resources
      osSemaphoreClose(context->semaphore);
      //Close socket
      socketClose(context->socket);

#if (HTTP_SERVER_DIGEST_AUTH_SUPPORT == ENABLED)
      //Close mutex
      osMutexClose(context->nonceCacheMutex);
#endif
   }

   //Return status code
   return error;
}


/**
 * @brief Start HTTP server
 * @param[in] context Pointer to the HTTP server context
 * @return Error code
 **/

error_t httpServerStart(HttpServerContext *context)
{
   //Debug message
   TRACE_INFO("Starting HTTP server...\r\n");
   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Stop HTTP server
 * @param[in] context Pointer to the HTTP server context
 * @return Error code
 **/

error_t httpServerStop(HttpServerContext *context)
{
   //Debug message
   TRACE_INFO("Stopping HTTP server...\r\n");
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
}


/**
 * @brief HTTP server listener task
 * @param[in] param Pointer to the HTTP server context
 **/

void httpListenerTask(void *param)
{
   error_t error;
   uint_t counter;
   uint16_t clientPort;
   IpAddr clientIpAddr;
   HttpServerContext *context;
   HttpConnection* connection;
   Socket *socket;
   OsTask *task;

   //Retrieve the HTTP server context
   context = (HttpServerContext *) param;

   //Process incoming connections to the server
   for(counter = 1; ; counter++)
   {
      //Debug message
      TRACE_INFO("Ready to accept a new connection...\r\n");

      //Accept an incoming connection
      socket = socketAccept(context->socket, &clientIpAddr, &clientPort);
      //Failure detected?
      if(!socket) continue;

      //Limit the number of simultaneous connections to the server
      if(!osSemaphoreWait(context->semaphore, 0))
      {
         //Debug message
         TRACE_INFO("Connection #%u refused with client %s port %" PRIu16 "...\r\n",
            counter, ipAddrToString(&clientIpAddr, NULL), clientPort);
         //Close socket
         socketClose(socket);
         //Connection request is refused
         continue;
      }

      //Debug message
      TRACE_INFO("Connection #%u established with client %s port %" PRIu16 "...\r\n",
         counter, ipAddrToString(&clientIpAddr, NULL), clientPort);

      //Allocate resources for the new connection
      connection = osMemAlloc(sizeof(HttpConnection));

      //Successful memory allocation?
      if(connection)
      {
         //Reference to the HTTP server settings
         connection->settings = &context->settings;
         //Reference to the HTTP server context
         connection->serverContext = context;
         //Reference to the semaphore
         connection->semaphore = context->semaphore;
         //Reference to the new socket
         connection->socket = socket;

         //Set timeout for blocking functions
         error = socketSetTimeout(connection->socket, HTTP_SERVER_TIMEOUT);

         //Check error code
         if(!error)
         {
            //Create a task to service the current connection
            task = osTaskCreate("HTTP Connection", httpConnectionTask,
               connection, HTTP_SERVER_STACK_SIZE, HTTP_SERVER_PRIORITY);

            //Did we encounter an error?
            if(task == OS_INVALID_HANDLE)
            {
               //Close socket
               socketClose(connection->socket);
               //Release semaphore
               osSemaphoreRelease(connection->semaphore);
               //Free previously allocated memory
               osMemFree(connection);
            }
         }
      }
   }
}


/**
 * @brief Task that services requests from an active connection
 * @param[in] param Structure representing an HTTP connection with a client
 **/

void httpConnectionTask(void *param)
{
   error_t error;
   uint_t counter;
   HttpConnection *connection;

   //Point to the structure representing the HTTP connection
   connection = (HttpConnection *) param;
   //Initialize status code
   error = NO_ERROR;

#if (HTTP_SERVER_TLS_SUPPORT == ENABLED)
   //Use SSL/TLS to secure the connection?
   if(connection->settings->useTls)
   {
      //Debug message
      TRACE_INFO("Initializing SSL/TLS session...\r\n");

      //Start of exception handling block
      do
      {
         //Allocate SSL/TLS context
         connection->tlsContext = tlsInit();
         //Initialization failed?
         if(connection->tlsContext == NULL)
         {
            //Report an error
            error = ERROR_OUT_OF_MEMORY;
            //Exit immediately
            break;
         }

         //Select server operation mode
         error = tlsSetConnectionEnd(connection->tlsContext, TLS_CONNECTION_END_SERVER);
         //Any error to report?
         if(error) break;

         //Bind TLS to the relevant socket
         error = tlsSetSocket(connection->tlsContext, connection->socket);
         //Any error to report?
         if(error) break;

         //Invoke user-defined callback, if any
         if(connection->settings->tlsInitCallback != NULL)
         {
            //Perform SSL/TLS related initialization
            error = connection->settings->tlsInitCallback(connection);
            //Any error to report?
            if(error) break;
         }

         //Establish a secure session
         error = tlsConnect(connection->tlsContext);
         //Any error to report?
         if(error) break;

         //End of exception handling block
      } while(0);
   }
   else
   {
      //Do not use SSL/TLS
      connection->tlsContext = NULL;
   }
#endif

   //Check status code
   if(!error)
   {
      //Process incoming requests
      for(counter = 0; counter < HTTP_SERVER_MAX_REQUESTS; counter++)
      {
         //Debug message
         TRACE_INFO("Waiting for request...\r\n");

         //Clear request header
         memset(&connection->request, 0, sizeof(HttpRequest));
         //Clear response header
         memset(&connection->response, 0, sizeof(HttpResponse));

         //Read the HTTP request header and parse its contents
         error = httpReadHeader(connection);
         //Any error to report?
         if(error)
         {
            //Debug message
            TRACE_INFO("No HTTP request received or parsing error...\r\n");
            break;
         }

#if (HTTP_SERVER_BASIC_AUTH_SUPPORT == ENABLED || HTTP_SERVER_DIGEST_AUTH_SUPPORT == ENABLED)
         //No Authorization header found?
         if(!connection->request.auth.found)
         {
            //Invoke user-defined callback, if any
            if(connection->settings->authCallback != NULL)
            {
               //Check whether the access to the specified URI is authorized
               connection->status = connection->settings->authCallback(connection,
                  connection->request.auth.user, connection->request.uri);
            }
            else
            {
               //Access to the specified URI is allowed
               connection->status = HTTP_ACCESS_ALLOWED;
            }
         }

         //Check access status
         if(connection->status == HTTP_ACCESS_ALLOWED)
         {
            //Access to the specified URI is allowed
            error = NO_ERROR;
         }
         else if(connection->status == HTTP_ACCESS_BASIC_AUTH_REQUIRED)
         {
            //Basic access authentication is required
            connection->response.auth.mode = HTTP_AUTH_MODE_BASIC;
            //Report an error
            error = ERROR_AUTH_REQUIRED;
         }
         else if(connection->status == HTTP_ACCESS_DIGEST_AUTH_REQUIRED)
         {
            //Digest access authentication is required
            connection->response.auth.mode = HTTP_AUTH_MODE_DIGEST;
            //Report an error
            error = ERROR_AUTH_REQUIRED;
         }
         else
         {
            //Access to the specified URI is denied
            error = ERROR_NOT_FOUND;
         }
#endif

         //Debug message
         TRACE_INFO("Sending HTTP response to the client...\r\n");

         //Check status code
         if(!error)
         {
#if (HTTP_SERVER_SSI_SUPPORT == ENABLED)
            //Use server-side scripting to dynamically generate HTML code?
            if(httpCompExtension(connection->request.uri, ".stm") ||
               httpCompExtension(connection->request.uri, ".shtm") ||
               httpCompExtension(connection->request.uri, ".shtml"))
            {
               //SSI processing (Server Side Includes)
               error = ssiExecuteScript(connection, connection->request.uri, 0);
            }
            else
#endif
            {
               //Send the contents of the requested page
               error = httpSendResponse(connection, connection->request.uri);
            }

            //The requested resource is not available?
            if(error == ERROR_NOT_FOUND)
            {
               //Invoke user-defined callback, if any
               if(connection->settings->uriNotFoundCallback != NULL)
               {
                  error = connection->settings->uriNotFoundCallback(connection,
                     connection->request.uri);
               }
            }
         }

         //Bad request?
         if(error == ERROR_INVALID_REQUEST)
         {
            //Send an error 400 and close the connection immediately
            httpSendErrorResponse(connection, 400,
               "The request is badly formed");
         }
         //Authorization required?
         else if(error == ERROR_AUTH_REQUIRED)
         {
            //Send an error 401 and keep the connection alive
            error = httpSendErrorResponse(connection, 401,
               "Authorization required");
         }
         //Page not found?
         else if(error == ERROR_NOT_FOUND)
         {
            //Send an error 404 and keep the connection alive
            error = httpSendErrorResponse(connection, 404,
               "The requested page could not be found");
         }

         //Internal error?
         if(error)
         {
            //Close the connection immediately
            break;
         }

         //Check whether the connection is persistent or not
         if(!connection->request.keepAlive || !connection->response.keepAlive)
         {
            //Close the connection immediately
            break;
         }
      }
   }

#if (HTTP_SERVER_TLS_SUPPORT == ENABLED)
   //Valid SSL/TLS context?
   if(connection->tlsContext != NULL)
   {
      //Debug message
      TRACE_INFO("Closing SSL/TLS session...\r\n");

      //Gracefully close SSL/TLS session
      tlsShutdown(connection->tlsContext);
      //Release context
      tlsFree(connection->tlsContext);
   }
#endif

   //Debug message
   TRACE_INFO("Graceful shutdown...\r\n");
   //Graceful shutdown
   socketShutdown(connection->socket, SOCKET_SD_BOTH);

   //Debug message
   TRACE_INFO("Closing socket...\r\n");
   //Close socket
   socketClose(connection->socket);

   //Release semaphore
   osSemaphoreRelease(connection->semaphore);
   //Release connection context
   osMemFree(connection);

   //Kill ourselves
   osTaskDelete(NULL);
}


/**
 * @brief Read HTTP request header and parse its contents
 * @param[in] connection Structure representing an HTTP connection
 * @return Error code
 **/

error_t httpReadHeader(HttpConnection *connection)
{
   error_t error;
   size_t length;
   char_t *token;
   char_t *p;
   char_t *s;

   //Read the first line of the request
   error = httpReceive(connection, connection->buffer,
      HTTP_SERVER_BUFFER_SIZE - 1, &length, SOCKET_FLAG_BREAK_CRLF);
   //Unable to read any data?
   if(error) return error;

   //Properly terminate the string with a NULL character
   connection->buffer[length] = '\0';
   //Debug message
   TRACE_INFO("%s", connection->buffer);

   //The Request-Line begins with a method token
   token = strtok_r(connection->buffer, " \r\n", &p);
   //Unable to retrieve the method?
   if(!token) return ERROR_INVALID_REQUEST;

   //The Method token indicates the method to be performed on the
   //resource identified by the Request-URI
   error = strSafeCopy(connection->request.method, token, HTTP_SERVER_METHOD_MAX_LEN);
   //Any error to report?
   if(error) return ERROR_INVALID_REQUEST;

   //The Request-URI is following the method token
   token = strtok_r(NULL, " \r\n", &p);
   //Unable to retrieve the Request-URI?
   if(!token) return ERROR_INVALID_REQUEST;

   //Check whether a query string is present
   s = strchr(token, '?');

   //Query string found?
   if(s != NULL)
   {
      //Split the string
      *s = '\0';

      //Save the Request-URI
      error = httpDecodePercentEncodedString(token,
         connection->request.uri, HTTP_SERVER_URI_MAX_LEN);
      //Any error to report?
      if(error)
         return ERROR_INVALID_REQUEST;

      //Check the length of the query string
      if(strlen(s + 1) > HTTP_SERVER_QUERY_STRING_MAX_LEN)
         return ERROR_INVALID_REQUEST;

      //Save the query string
      strcpy(connection->request.queryString, s + 1);
   }
   else
   {
      //Save the Request-URI
      error = httpDecodePercentEncodedString(token,
         connection->request.uri, HTTP_SERVER_URI_MAX_LEN);
      //Any error to report?
      if(error)
         return ERROR_INVALID_REQUEST;

      //No query string
      connection->request.queryString[0] = '\0';
   }

   //Redirect to the default home page if necessary
   if(!strcasecmp(connection->request.uri, "/"))
      strcpy(connection->request.uri, connection->settings->defaultDocument);

   //Clean the resulting path
   pathCanonicalize(connection->request.uri);

   //The protocol version is following the Request-URI
   token = strtok_r(NULL, " \r\n", &p);

   //HTTP version 0.9?
   if(!token)
   {
      //Save version number
      connection->request.version = HTTP_VERSION_0_9;
      //Persistent connections are not supported
      connection->request.keepAlive = FALSE;
   }
   //HTTP version 1.0?
   else if(!strcasecmp(token, "HTTP/1.0"))
   {
      //Save version number
      connection->request.version = HTTP_VERSION_1_0;
      //By default connections are not persistent
      connection->request.keepAlive = FALSE;
   }
   //HTTP version 1.1?
   else if(!strcasecmp(token, "HTTP/1.1"))
   {
      //Save version number
      connection->request.version = HTTP_VERSION_1_1;
      //HTTP 1.1 makes persistent connections the default
      connection->request.keepAlive = TRUE;
   }
   //HTTP version not supported?
   else
   {
      return ERROR_INVALID_REQUEST;
   }

   //Default value for properties
   connection->request.chunkedEncoding = FALSE;
   connection->request.contentLength = 0;

   //HTTP 0.9 does not support Full-Request
   if(connection->request.version >= HTTP_VERSION_1_0)
   {
      //Local variables
      char_t firstChar;
      char_t *separator;
      char_t *name;
      char_t *value;

      //This variable is used to decode header fields that span multiple lines
      firstChar = '\0';

      //Parse the header fields of the HTTP request
      while(1)
      {
         //Decode multiple-line header field
         error = httpReadHeaderField(connection, connection->buffer,
            HTTP_SERVER_BUFFER_SIZE, &firstChar);
         //Any error to report?
         if(error) return error;

         //Debug message
         TRACE_DEBUG("%s", connection->buffer);

         //An empty line indicates the end of the header fields
         if(!strcmp(connection->buffer, "\r\n"))
            break;

         //Check whether a separator is present
         separator = strchr(connection->buffer, ':');

         //Separator found?
         if(separator != NULL)
         {
            //Split the line
            *separator = '\0';

            //Get field name and value
            name = strTrimWhitespace(connection->buffer);
            value = strTrimWhitespace(separator + 1);

            //Connection field found?
            if(!strcasecmp(name, "Connection"))
            {
               //Check whether persistent connections are supported or not
               if(!strcasecmp(value, "keep-alive"))
                  connection->request.keepAlive = TRUE;
               else if(!strcasecmp(value, "close"))
                  connection->request.keepAlive = FALSE;
            }
            //Transfer-Encoding field found?
            else if(!strcasecmp(name, "Transfer-Encoding"))
            {
               //Check whether chunked encoding is used
               if(!strcasecmp(value, "chunked"))
                  connection->request.chunkedEncoding = TRUE;
            }
            //Content-Length field found?
            else if(!strcasecmp(name, "Content-Length"))
            {
               //Get the length of the body data
               connection->request.contentLength = atoi(value);
            }
            //Authorization field found?
            else if(!strcasecmp(name, "Authorization"))
            {
               //Parse Authorization field
               httpParseAuthField(connection, value);
            }
         }
      }
   }

   //Prepare to read the HTTP request body
   if(connection->request.chunkedEncoding)
   {
      connection->request.byteCount = 0;
      connection->request.firstChunk = TRUE;
      connection->request.lastChunk = FALSE;
   }
   else
   {
      connection->request.byteCount = connection->request.contentLength;
   }

   //The request header has been successfully parsed
   return NO_ERROR;
}


/**
 * @brief Send HTTP response header
 * @param[in] connection Structure representing an HTTP connection
 * @return Error code
 **/

error_t httpWriteHeader(HttpConnection *connection)
{
   error_t error;
   uint_t i;
   char_t *p;

   //HTTP version 0.9?
   if(connection->response.version == HTTP_VERSION_0_9)
   {
      //Enforce default parameters
      connection->response.keepAlive = FALSE;
      connection->response.chunkedEncoding = FALSE;
      //The size of the response body is not limited
      connection->response.byteCount = UINT_MAX;
      //We are done since HTTP 0.9 does not support Full-Response format
      return NO_ERROR;
   }

   //When generating dynamic web pages with HTTP 1.0, the only way to
   //signal the end of the body is to close the connection
   if(connection->response.version == HTTP_VERSION_1_0 &&
      connection->response.chunkedEncoding)
   {
      //Make the connection non persistent
      connection->response.keepAlive = FALSE;
      connection->response.chunkedEncoding = FALSE;
      //The size of the response body is not limited
      connection->response.byteCount = UINT_MAX;
   }
   else
   {
      //Limit the size of the response body
      connection->response.byteCount = connection->response.contentLength;
   }

   //Point to the beginning of the buffer
   p = connection->buffer;

   //The first line of a response message is the Status-Line, consisting
   //of the protocol version followed by a numeric status code and its
   //associated textual phrase
   p += sprintf(p, "HTTP/%u.%u %u ", MSB(connection->response.version),
      LSB(connection->response.version), connection->response.statusCode);

   //Retrieve the Reason-Phrase that corresponds to the Status-Code
   for(i = 0; i < arraysize(statusCodeList); i++)
   {
      //Check the status code
      if(statusCodeList[i].value == connection->response.statusCode)
      {
         //Append the textual phrase to the Status-Line
         p += sprintf(p, statusCodeList[i].message);
         //Break the loop and continue processing
         break;
      }
   }

   //Properly terminate the Status-Line
   p += sprintf(p, "\r\n");
   //The Server response-header field contains information about the
   //software used by the origin server to handle the request
   p += sprintf(p, "Server: Oryx Embedded HTTP Server\r\n");

   //Persistent connection?
   if(connection->response.keepAlive)
   {
      //Set Connection field
      p += sprintf(p, "Connection: keep-alive\r\n");

      //Set Keep-Alive field
      p += sprintf(p, "Keep-Alive: timeout=%u, max=%u\r\n",
         HTTP_SERVER_TIMEOUT / 1000, HTTP_SERVER_MAX_REQUESTS);
   }
   else
   {
      //Set Connection field
      p += sprintf(p, "Connection: close\r\n");
   }

   //Prevent the client from using cache?
   if(connection->response.noCache)
   {
      //Set Pragma field
      p += sprintf(p, "Pragma: no-cache\r\n");
      //Set Cache-Control field
      p += sprintf(p, "Cache-Control: no-store, no-cache, must-revalidate\r\n");
      p += sprintf(p, "Cache-Control: post-check=0, pre-check=0\r\n");
   }

#if (HTTP_SERVER_BASIC_AUTH_SUPPORT == ENABLED)
   //Use basic access authentication?
   if(connection->response.auth.mode == HTTP_AUTH_MODE_BASIC)
   {
      //Set WWW-Authenticate field
      p += sprintf(p, "WWW-Authenticate: Basic realm=\"Protected Area\"\r\n");
   }
#endif
#if (HTTP_SERVER_DIGEST_AUTH_SUPPORT == ENABLED)
   //Use digest access authentication?
   if(connection->response.auth.mode == HTTP_AUTH_MODE_DIGEST)
   {
      size_t n;
      uint8_t opaque[16];

      //Set WWW-Authenticate field
      p += sprintf(p, "WWW-Authenticate: Digest\r\n");
      p += sprintf(p, "  realm=\"Protected Area\",\r\n");
      p += sprintf(p, "  qop=\"auth\",\r\n");
      p += sprintf(p, "  nonce=\"");

      //A server-specified data string which should be uniquely generated
      //each time a 401 response is made
      error = httpGenerateNonce(connection->serverContext, p, &n);
      //Any error to report?
      if(error) return error;

      //Advance pointer
      p += n;
      //Properly terminate the nonce string
      p += sprintf(p, "\",\r\n");

      //Format opaque parameter
      p += sprintf(p, "  opaque=\"");

      //Generate a random value
      error = connection->settings->prngAlgo->read(
         connection->settings->prngContext, opaque, 16);
      //Random number generation failed?
      if(error) return error;

      //Convert the byte array to hex string
      httpConvertArrayToHexString(opaque, 16, p);

      //Advance pointer
      p += 32;
      //Properly terminate the opaque string
      p += sprintf(p, "\"");

      //The STALE flag indicates that the previous request from the client
      //was rejected because the nonce value was stale
      if(connection->response.auth.stale)
         p += sprintf(p, ",\r\n  stale=TRUE");

      //Properly terminate the WWW-Authenticate field
      p += sprintf(p, "\r\n");
   }
#endif

   //Content type
   p += sprintf(p, "Content-Type: %s\r\n", connection->response.contentType);

   //Use chunked encoding transfer?
   if(connection->response.chunkedEncoding)
   {
      //Set Transfer-Encoding field
      p += sprintf(p, "Transfer-Encoding: chunked\r\n");
   }
   //Persistent connection?
   else if(connection->response.keepAlive)
   {
      //Set Content-Length field
      p += sprintf(p, "Content-Length: %" PRIuSIZE "\r\n", connection->response.contentLength);
   }

   //Terminate the header with an empty line
   p += sprintf(p, "\r\n");

   //Debug message
   TRACE_DEBUG("HTTP response header:\r\n%s", connection->buffer);

   //Send HTTP response header to the client
   error = httpSend(connection, connection->buffer,
      strlen(connection->buffer), 0);

   //Return status code
   return error;
}


/**
 * @brief Read data from client request
 * @param[in] connection Structure representing an HTTP connection
 * @param[out] data Buffer where to store the incoming data
 * @param[in] size Maximum number of bytes that can be received
 * @param[out] received Number of bytes that have been received
 * @param[in] flags Set of flags that influences the behavior of this function
 * @return Error code
 **/

error_t httpReadStream(HttpConnection *connection, void *data, size_t size, size_t *received, uint_t flags)
{
   error_t error;
   size_t n;

   //No data has been read yet
   *received = 0;

   //Chunked encoding transfer is used?
   if(connection->request.chunkedEncoding)
   {
      //Point to the output buffer
      char_t *p = data;

      //Read as much data as possible
      while(*received < size)
      {
         //End of HTTP request body?
         if(connection->request.lastChunk)
            return ERROR_END_OF_STREAM;

         //Acquire a new chunk when the current chunk
         //has been completely consumed
         if(connection->request.byteCount == 0)
         {
            //The size of each chunk is sent right before the chunk itself
            error = httpReadChunkSize(connection);
            //Failed to decode the chunk-size field?
            if(error) return error;

            //Any chunk whose size is zero terminates the data transfer
            if(!connection->request.byteCount)
            {
               //The user must be satisfied with data already on hand
               return (*received > 0) ? NO_ERROR : ERROR_END_OF_STREAM;
            }
         }

         //Limit the number of bytes to read at a time
         n = min(size - *received, connection->request.byteCount);

         //Read data
         error = httpReceive(connection, p, n, &n, flags);
         //Any error to report?
         if(error) return error;

         //Total number of data that have been read
         *received += n;
         //Remaining data still available in the current chunk
         connection->request.byteCount -= n;

         //The HTTP_FLAG_BREAK_CHAR flag causes the function to stop reading
         //data as soon as the specified break character is encountered
         if(flags & HTTP_FLAG_BREAK_CRLF)
         {
            //Check whether a break character has been received
            if(p[n - 1] == LSB(flags)) break;
         }
         //The HTTP_FLAG_WAIT_ALL flag causes the function to return
         //only when the requested number of bytes have been read
         else if(!(flags & HTTP_FLAG_WAIT_ALL))
         {
            break;
         }

         //Advance data pointer
         p += n;
      }
   }
   //Default encoding?
   else
   {
      //Return immediately if the end of the request body has been reached
      if(!connection->request.byteCount)
         return ERROR_END_OF_STREAM;

      //Limit the number of bytes to read
      n = min(size, connection->request.byteCount);

      //Read data
      error = httpReceive(connection, data, n, received, flags);
      //Any error to report
      if(error) return error;

      //Decrement the count of remaining bytes to read
      connection->request.byteCount -= *received;
   }

   //Successful read operation
   return NO_ERROR;
}


/**
 * @brief Read chunk-size field from the input stream
 * @param[in] connection Structure representing an HTTP connection
 **/

error_t httpReadChunkSize(HttpConnection *connection)
{
   error_t error;
   size_t n;
   char_t *end;
   char_t s[8];

   //First chunk to be received?
   if(connection->request.firstChunk)
   {
      //Clear the flag
      connection->request.firstChunk = FALSE;
   }
   else
   {
      //Read the CRLF that follows the previous chunk-data field
      error = httpReceive(connection, s, sizeof(s) - 1, &n, SOCKET_FLAG_BREAK_CRLF);
      //Any error to report?
      if(error) return error;

      //Properly terminate the string with a NULL character
      s[n] = '\0';

      //The chunk data must be terminated by CRLF
      if(strcmp(s, "\r\n"))
         return ERROR_WRONG_ENCODING;
   }

   //Read the chunk-size field
   error = httpReceive(connection, s, sizeof(s) - 1, &n, SOCKET_FLAG_BREAK_CRLF);
   //Any error to report?
   if(error) return error;

   //Properly terminate the string with a NULL character
   s[n] = '\0';
   //Remove extra whitespaces
   strRemoveTrailingSpace(s);

   //Retrieve the size of the chunk
   connection->request.byteCount = strtoul(s, &end, 16);

   //No valid conversion could be performed?
   if(end == s || *end != '\0')
      return ERROR_WRONG_ENCODING;

   //Any chunk whose size is zero terminates the data transfer
   if(!connection->request.byteCount)
   {
      //The end of the HTTP request body has been reached
      connection->request.lastChunk = TRUE;

      //Skip the trailer
      while(1)
      {
         //Read a complete line
         error = httpReceive(connection, s, sizeof(s) - 1, &n, SOCKET_FLAG_BREAK_CRLF);
         //Unable to read any data?
         if(error) return error;

         //Properly terminate the string with a NULL character
         s[n] = '\0';

         //The trailer is terminated by an empty line
         if(!strcmp(s, "\r\n"))
            break;
      }
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Write data to the client
 * @param[in] connection Structure representing an HTTP connection
 * @param[in] data Buffer containing the data to be transmitted
 * @param[in] length Number of bytes to be transmitted
 * @return Error code
 **/

error_t httpWriteStream(HttpConnection *connection, const void *data, size_t length)
{
   error_t error;
   uint_t n;

   //Use chunked encoding transfer?
   if(connection->response.chunkedEncoding)
   {
      //Any data to send?
      if(length > 0)
      {
         char_t s[8];

         //The chunk-size field is a string of hex digits
         //indicating the size of the chunk
         n = sprintf(s, "%X\r\n", length);

         //Send the chunk-size field
         error = httpSend(connection, s, n, 0);
         //Failed to send data?
         if(error) return error;

         //Send the chunk-data
         error = httpSend(connection, data, length, 0);
         //Failed to send data?
         if(error) return error;

         //Terminate the chunk-data by CRLF
         error = httpSend(connection, "\r\n", 2, 0);
      }
      else
      {
         //Any chunk whose size is zero may terminate the data
         //transfer and must be discarded
         error = NO_ERROR;
      }
   }
   //Default encoding?
   else
   {
      //The length of the body shall not exceed the value
      //specified in the Content-Length field
      length = min(length, connection->response.byteCount);

      //Send user data
      error = httpSend(connection, data, length, 0);

      //Decrement the count of remaining bytes to be transferred
      connection->response.byteCount -= length;
   }

   //Return status code
   return error;
}


/**
 * @brief Close output stream
 * @param[in] connection Structure representing an HTTP connection
 * @return Error code
 **/

error_t httpCloseStream(HttpConnection *connection)
{
   error_t error;

   //Use chunked encoding transfer?
   if(connection->response.chunkedEncoding)
   {
      //The chunked encoding is ended by any chunk whose size is zero
      error = httpSend(connection, "0\r\n\r\n", 5, 0);
   }
   else
   {
      //Chunked encoding is not used...
      error = NO_ERROR;
   }

   //Return status code
   return error;
}


/**
 * @brief Send HTTP response
 * @param[in] connection Structure representing an HTTP connection
 * @param[in] uri NULL-terminated string containing the file to be sent in response
 * @return Error code
 **/

error_t httpSendResponse(HttpConnection *connection, const char_t *uri)
{
#if (HTTP_SERVER_FS_SUPPORT == ENABLED)
   error_t error;
   uint32_t length;
   size_t n;
   FsFile *file;

   //Retrieve the full pathname
   httpGetAbsolutePath(connection, uri,
      connection->buffer, HTTP_SERVER_BUFFER_SIZE);

   //Retrieve the size of the specified file
   error = fsFileGetSize(connection->buffer, &length);
   //The specified URI cannot be found?
   if(error) return ERROR_NOT_FOUND;

   //Open the file for reading
   file = fsFileOpen(connection->buffer, FS_FILE_MODE_READ);
   //Failed to open the file?
   if(!file) return ERROR_NOT_FOUND;
#else
   error_t error;
   size_t length;
   uint8_t *data;
   char_t type;

   //Retrieve the full pathname
   httpGetAbsolutePath(connection, uri,
      connection->buffer, HTTP_SERVER_BUFFER_SIZE);

   //Get the resource data associated with the URI
   error = resGetData(connection->buffer, &data, &length, &type);
   //The specified URI cannot be found?
   if(error) return error;

#if (HTTP_SERVER_SSI_SUPPORT == ENABLED)
	//Use server-side scripting to dynamically generate HTML code?
	if(type == RES_TYPE_CGI)
	{
	   //SSI processing (Server Side Includes)
	   return ssiExecuteScript(connection, uri, 0);
	}
#endif
#endif

   //Format HTTP response header
   connection->response.version = connection->request.version;
   connection->response.statusCode = 200;
   connection->response.keepAlive = connection->request.keepAlive;
   connection->response.noCache = FALSE;
   connection->response.contentType = mimeGetType(uri);
   connection->response.chunkedEncoding = FALSE;
   connection->response.contentLength = length;

   //Send the header to the client
   error = httpWriteHeader(connection);
   //Any error to report?
   if(error)
   {
#if (HTTP_SERVER_FS_SUPPORT == ENABLED)
      //Close the file
      fsFileClose(file);
#endif
      //Return status code
      return error;
   }

#if (HTTP_SERVER_FS_SUPPORT == ENABLED)
   //Send response body
   while(length > 0)
   {
      //Limit the number of bytes to read at a time
      n = min(length, HTTP_SERVER_BUFFER_SIZE);

      //Read data from the specified file
      error = fsFileRead(file, connection->buffer, n, &n);
      //End of input stream?
      if(error) break;

      //Send data to the client
      error = httpWriteStream(connection, connection->buffer, n);
      //Any error to report?
      if(error) break;

      //Decrement the count of remaining bytes to be transferred
      length -= n;
   }

   //Close the file
   fsFileClose(file);

   //Successful file transfer?
   if(length == 0 && error == ERROR_END_OF_STREAM)
   {
      //Properly close the output stream
      error = httpCloseStream(connection);
   }
#else
   //Send response body
   error = httpWriteStream(connection, data, length);
   //Any error to report?
   if(error) return error;

   //Properly close output stream
   error = httpCloseStream(connection);
#endif

   //Return status code
   return error;
}


/**
 * @brief Send data to the client
 * @param[in] connection Structure representing an HTTP connection
 * @param[in] data Pointer to a buffer containing the data to be transmitted
 * @param[in] length Number of bytes to be transmitted
 * @param[in] flags Set of flags that influences the behavior of this function
 **/

error_t httpSend(HttpConnection *connection, const void *data, size_t length, uint_t flags)
{
#if (HTTP_SERVER_TLS_SUPPORT == ENABLED)
   //Check whether a secure connection is being used
   if(connection->tlsContext != NULL)
   {
      //Use SSL/TLS to transmit data to the client
      return tlsWrite(connection->tlsContext, data, length, flags);
   }
   else
#endif
   {
      //Transmit data to the client
      return socketSend(connection->socket, data, length, NULL, flags);
   }
}


/**
 * @brief Receive data from the client
 * @param[in] connection Structure representing an HTTP connection
 * @param[out] data Buffer into which received data will be placed
 * @param[in] size Maximum number of bytes that can be received
 * @param[out] received Actual number of bytes that have been received
 * @param[in] flags Set of flags that influences the behavior of this function
 * @return Error code
 **/

error_t httpReceive(HttpConnection *connection, void *data, size_t size, size_t *received, uint_t flags)
{
#if (HTTP_SERVER_TLS_SUPPORT == ENABLED)
   //Check whether a secure connection is being used
   if(connection->tlsContext != NULL)
   {
      //Use SSL/TLS to receive data from the client
      return tlsRead(connection->tlsContext, data, size, received, flags);
   }
   else
#endif
   {
      //Receive data from the client
      return socketReceive(connection->socket, data, size, received, flags);
   }
}


/**
 * @brief Send error response to the client
 * @param[in] connection Structure representing an HTTP connection
 * @param[in] statusCode HTTP status code
 * @param[in] message User message
 * @return Error code
 **/

error_t httpSendErrorResponse(HttpConnection *connection, uint_t statusCode, const char_t *message)
{
   error_t error;
   size_t length;

   //HTML response template
   static const char_t template[] =
      "<!doctype html>\r\n"
      "<html>\r\n"
      "<head><title>Error %03d</title></head>\r\n"
      "<body>\r\n"
      "<h2>Error %03d</h2>\r\n"
      "<p>%s</p>\r\n"
      "</body>\r\n"
      "</html>\r\n";

   //Compute the length of the response
   length = strlen(template) + strlen(message) - 4;

   //Format HTTP response header
   connection->response.version = connection->request.version;
   connection->response.statusCode = statusCode;
   connection->response.keepAlive = connection->request.keepAlive;
   connection->response.noCache = FALSE;
   connection->response.contentType = mimeGetType(".htm");
   connection->response.chunkedEncoding = FALSE;
   connection->response.contentLength = length;

   //Send the header to the client
   error = httpWriteHeader(connection);
   //Any error to report?
   if(error) return error;

   //Format HTML response
   sprintf(connection->buffer, template, statusCode, statusCode, message);

   //Send response body
   error = httpWriteStream(connection, connection->buffer, length);
   //Any error to report?
   if(error) return error;

   //Properly close output stream
   error = httpCloseStream(connection);
   //Return status code
   return error;
}


/**
 * @brief Read multiple-line header field
 * @param[in] connection Structure representing an HTTP connection
 * @param[out] buffer Buffer where to store the header field
 * @param[in] size Size of the buffer, in bytes
 * @param[in,out] firstChar Leading character of the header line
 * @return Error code
 **/

error_t httpReadHeaderField(HttpConnection *connection,
   char_t *buffer, size_t size, char_t *firstChar)
{
   error_t error;
   size_t n;
   size_t length;

   //This is the actual length of the header field
   length = 0;

   //The process of moving from a multiple-line representation of a header
   //field to its single line representation is called unfolding
   do
   {
      //Check the length of the header field
      if((length + 1) >= size)
      {
         //Report an error
         error = ERROR_INVALID_REQUEST;
         //Exit immediately
         break;
      }

      //NULL character found?
      if(*firstChar == '\0')
      {
         //Prepare to decode the first header field
         length = 0;
      }
      //LWSP character found?
      else if(*firstChar == ' ' || *firstChar == '\t')
      {
         //Unfolding is  accomplished by regarding CRLF immediately
         //followed by a LWSP as equivalent to the LWSP character
         buffer[length] = *firstChar;
         //The current header field spans multiple lines
         length++;
      }
      //Any other character?
      else
      {
         //Restore the very first character of the header field
         buffer[0] = *firstChar;
         //Prepare to decode a new header field
         length = 1;
      }

      //Read data until a CLRF character is encountered
      error = httpReceive(connection, buffer + length,
         size - 1 - length, &n, SOCKET_FLAG_BREAK_CRLF);
      //Any error to report?
      if(error) break;

      //Update the length of the header field
      length += n;
      //Properly terminate the string with a NULL character
      buffer[length] = '\0';

      //An empty line indicates the end of the header fields
      if(!strcmp(buffer, "\r\n"))
         break;

      //Read the next character to detect if the CRLF is immediately
      //followed by a LWSP character
      error = httpReceive(connection, firstChar,
         sizeof(char_t), &n, SOCKET_FLAG_WAIT_ALL);
      //Any error to report?
      if(error) break;

      //LWSP character found?
      if(*firstChar == ' ' || *firstChar == '\t')
      {
         //CRLF immediately followed by LWSP as equivalent to the LWSP character
         if(length >= 2)
         {
            if(buffer[length - 2] == '\r' || buffer[length - 1] == '\n')
            {
               //Remove trailing CRLF sequence
               length -= 2;
               //Properly terminate the string with a NULL character
               buffer[length] = '\0';
            }
         }
      }

      //A header field may span multiple lines...
   } while(*firstChar == ' ' || *firstChar == '\t');

   //Return status code
   return error;
}


/**
 * @brief Parse Authorization field
 * @param[in] connection Structure representing an HTTP connection
 * @param[in] value Authorization field value
 **/

void httpParseAuthField(HttpConnection *connection, char_t *value)
{
   char_t *p;
   char_t *token;

   //Retrieve the authentication scheme
   token = strtok_r(value, " \t", &p);

   //Any parsing error?
   if(token == NULL)
   {
      //Exit immediately
      return;
   }
#if (HTTP_SERVER_BASIC_AUTH_SUPPORT == ENABLED)
   //Basic access authentication?
   else if(!strcasecmp(token, "Basic"))
   {
      error_t error;
      size_t n;
      char_t *separator;

      //Use the relevant authentification scheme
      connection->request.auth.mode = HTTP_AUTH_MODE_BASIC;
      //Retrieve the credentials
      token = strtok_r(NULL, " \t", &p);

      //Any parsing error?
      if(token != NULL)
      {
         //Decrypt the Base64 encoded string
         error = base64Decode(token, strlen(token), token, &n);

         //Successful decoding?
         if(!error)
         {
            //Properly terminate the string
            token[n] = '\0';
            //Check whether a separator is present
            separator = strchr(token, ':');

            //Separator found?
            if(separator != NULL)
            {
               //Split the line
               *separator = '\0';

               //Save user name
               strSafeCopy(connection->request.auth.user,
                  token, HTTP_SERVER_USERNAME_MAX_LEN);

               //Point to the password
               token = separator + 1;
               //Save password
               connection->request.auth.password = token;
            }
         }
      }

      //Debug message
      TRACE_DEBUG("Authorization header:\r\n");
      TRACE_DEBUG("  username: %s\r\n", connection->request.auth.user);
      TRACE_DEBUG("  password: %s\r\n", connection->request.auth.password);
   }
#endif
#if (HTTP_SERVER_DIGEST_AUTH_SUPPORT == ENABLED)
   //Digest access authentication?
   else if(!strcasecmp(token, "Digest"))
   {
      size_t n;
      char_t *separator;
      char_t *name;

      //Use the relevant authentification scheme
      connection->request.auth.mode = HTTP_AUTH_MODE_DIGEST;
      //Get the first parameter
      token = strtok_r(NULL, ",", &p);

      //Parse the Authorization field
      while(token != NULL)
      {
         //Check whether a separator is present
         separator = strchr(token, '=');

         //Separator found?
         if(separator != NULL)
         {
            //Split the string
            *separator = '\0';

            //Get field name and value
            name = strTrimWhitespace(token);
            value = strTrimWhitespace(separator + 1);

            //Retrieve the length of the value field
            n = strlen(value);

            //Discard the surrounding quotes
            if(n > 0 && value[n - 1] == '\"')
               value[n - 1] = '\0';
            if(value[0] == '\"')
               value++;

            //Check parameter name
            if(!strcasecmp(name, "username"))
            {
               //Save user name
               strSafeCopy(connection->request.auth.user,
                  value, HTTP_SERVER_USERNAME_MAX_LEN);
            }
            else if(!strcasecmp(name, "realm"))
            {
               //Save realm
               connection->request.auth.realm = value;
            }
            else if(!strcasecmp(name, "nonce"))
            {
               //Save nonce parameter
               connection->request.auth.nonce = value;
            }
            else if(!strcasecmp(name, "uri"))
            {
               //Save uri parameter
               connection->request.auth.uri = value;
            }
            else if(!strcasecmp(name, "qop"))
            {
               //Save qop parameter
               connection->request.auth.qop = value;
            }
            else if(!strcasecmp(name, "nc"))
            {
               //Save nc parameter
               connection->request.auth.nc = value;
            }
            else if(!strcasecmp(name, "cnonce"))
            {
               //Save cnonce parameter
               connection->request.auth.cnonce = value;
            }
            else if(!strcasecmp(name, "response"))
            {
               //Save response parameter
               connection->request.auth.response = value;
            }
            else if(!strcasecmp(name, "opaque"))
            {
               //Save opaque parameter
               connection->request.auth.opaque = value;
            }

            //Get next parameter
            token = strtok_r(NULL, ",", &p);
         }
      }

      //Debug message
      TRACE_DEBUG("Authorization header:\r\n");
      TRACE_DEBUG("  username: %s\r\n", connection->request.auth.user);
      TRACE_DEBUG("  realm: %s\r\n", connection->request.auth.realm);
      TRACE_DEBUG("  nonce: %s\r\n", connection->request.auth.nonce);
      TRACE_DEBUG("  uri: %s\r\n", connection->request.auth.uri);
      TRACE_DEBUG("  qop: %s\r\n", connection->request.auth.qop);
      TRACE_DEBUG("  nc: %s\r\n", connection->request.auth.nc);
      TRACE_DEBUG("  cnonce: %s\r\n", connection->request.auth.cnonce);
      TRACE_DEBUG("  response: %s\r\n", connection->request.auth.response);
      TRACE_DEBUG("  opaque: %s\r\n", connection->request.auth.opaque);
   }
#endif
   else
   {
      //The specified authentication scheme is not supported
      return;
   }

#if (HTTP_SERVER_BASIC_AUTH_SUPPORT == ENABLED || HTTP_SERVER_DIGEST_AUTH_SUPPORT == ENABLED)
   //The Authorization header has been found
   connection->request.auth.found = TRUE;

   //Invoke user-defined callback, if any
   if(connection->settings->authCallback != NULL)
   {
      //Check whether the access to the specified URI is authorized
      connection->status = connection->settings->authCallback(connection,
         connection->request.auth.user, connection->request.uri);
   }
   else
   {
      //Access to the specified URI is allowed
      connection->status = HTTP_ACCESS_ALLOWED;
   }
#endif
}


/**
 * @brief Password verification
 * @param[in] connection Structure representing an HTTP connection
 * @param[in] password NULL-terminated string containing the password to be checked
 * @param[in] mode HTTP authentication scheme to be used. Acceptable
 *   values are HTTP_AUTH_MODE_BASIC or HTTP_AUTH_MODE_DIGEST
 * @return TRUE if the password is valid, else FALSE
 **/

bool_t httpCheckPassword(HttpConnection *connection,
   const char_t *password, HttpAuthMode mode)
{
   //This flag tells whether the password is valid
   bool_t status = FALSE;

   //Debug message
   TRACE_DEBUG("Password verification...\r\n");

#if (HTTP_SERVER_BASIC_AUTH_SUPPORT == ENABLED)
   //Basic authentication scheme?
   if(mode == HTTP_AUTH_MODE_BASIC)
   {
      //Point to the authentication credentials
      HttpAuthorizationHeader *auth = &connection->request.auth;

      //Make sure authentication credentials have been found
      if(auth->found && auth->mode == HTTP_AUTH_MODE_BASIC)
      {
         //Sanity check
         if(auth->password != NULL)
         {
            //Check whether the password is valid
            if(!strcmp(password, auth->password))
               status = TRUE;
         }
      }
   }
#endif
#if (HTTP_SERVER_DIGEST_AUTH_SUPPORT == ENABLED)
   //Digest authentication scheme?
   if(mode == HTTP_AUTH_MODE_DIGEST)
   {
      //Point to the authentication credentials
      HttpAuthorizationHeader *auth = &connection->request.auth;

      //Make sure authentication credentials have been found
      if(auth->found && auth->mode == HTTP_AUTH_MODE_DIGEST)
      {
         //Sanity check
         if(auth->realm != NULL && auth->realm != NULL &&
            auth->nonce != NULL && auth->qop != NULL &&
            auth->nc != NULL && auth->cnonce != NULL &&
            auth->response != NULL)
         {
            error_t error;
            Md5Context *md5Context;
            char_t ha1[2 * MD5_DIGEST_SIZE + 1];
            char_t ha2[2 * MD5_DIGEST_SIZE + 1];

            //Allocate a memory buffer to hold the MD5 context
            md5Context = osMemAlloc(sizeof(Md5Context));

            //MD5 context successfully allocated?
            if(md5Context != NULL)
            {
               //Compute HA1 = MD5(username : realm : password)
               md5Init(md5Context);
               md5Update(md5Context, auth->user, strlen(auth->user));
               md5Update(md5Context, ":", 1);
               md5Update(md5Context, auth->realm, strlen(auth->realm));
               md5Update(md5Context, ":", 1);
               md5Update(md5Context, password, strlen(password));
               md5Final(md5Context, NULL);

               //Convert MD5 hash to hex string
               httpConvertArrayToHexString(md5Context->digest, MD5_DIGEST_SIZE, ha1);
               //Debug message
               TRACE_DEBUG("  HA1: %s\r\n", ha1);

               //Compute HA2 = MD5(method : uri)
               md5Init(md5Context);
               md5Update(md5Context, connection->request.method, strlen(connection->request.method));
               md5Update(md5Context, ":", 1);
               md5Update(md5Context, auth->uri, strlen(auth->uri));
               md5Final(md5Context, NULL);

               //Convert MD5 hash to hex string
               httpConvertArrayToHexString(md5Context->digest, MD5_DIGEST_SIZE, ha2);
               //Debug message
               TRACE_DEBUG("  HA2: %s\r\n", ha2);

               //Compute MD5(HA1 : nonce : nc : cnonce : qop : HA1)
               md5Init(md5Context);
               md5Update(md5Context, ha1, strlen(ha1));
               md5Update(md5Context, ":", 1);
               md5Update(md5Context, auth->nonce, strlen(auth->nonce));
               md5Update(md5Context, ":", 1);
               md5Update(md5Context, auth->nc, strlen(auth->nc));
               md5Update(md5Context, ":", 1);
               md5Update(md5Context, auth->cnonce, strlen(auth->cnonce));
               md5Update(md5Context, ":", 1);
               md5Update(md5Context, auth->qop, strlen(auth->qop));
               md5Update(md5Context, ":", 1);
               md5Update(md5Context, ha2, strlen(ha2));
               md5Final(md5Context, NULL);

               //Convert MD5 hash to hex string
               httpConvertArrayToHexString(md5Context->digest, MD5_DIGEST_SIZE, ha1);
               //Debug message
               TRACE_DEBUG("  reponse: %s\r\n", ha1);

               //Release MD5 context
               osMemFree(md5Context);

               //Check response
               if(!strcasecmp(auth->response, ha1))
               {
                  //Perform nonce verification
                  error = httpVerifyNonce(connection->serverContext, auth->nonce, auth->nc);

                  //Valid nonce?
                  if(!error)
                  {
                     //Access to the resource is granted
                     status = TRUE;
                  }
                  else
                  {
                     //The client may wish to simply retry the request with a
                     //new encrypted response, without reprompting the user
                     //for a new username and password
                     connection->response.auth.stale = TRUE;
                  }
               }
            }
         }
      }
   }
#endif

   //Return TRUE is the password is valid, else FALSE
   return status;
}


#if (HTTP_SERVER_DIGEST_AUTH_SUPPORT == ENABLED)

/**
 * @brief Nonce generation
 * @param[in] context Pointer to the HTTP server context
 * @param[in] output NULL-terminated string containing the nonce
 * @param[in] length NULL-terminated string containing the nonce count
 * @return Error code
 **/

error_t httpGenerateNonce(HttpServerContext *context,
   char_t *output, size_t *length)
{
   error_t error;
   uint_t i;
   HttpNonceCacheEntry *entry;
   HttpNonceCacheEntry *oldestEntry;
   uint8_t nonce[HTTP_SERVER_NONCE_SIZE];

   //Acquire exclusive access to the nonce cache
   osMutexAcquire(context->nonceCacheMutex);

   //Keep track of the oldest entry
   oldestEntry = &context->nonceCache[0];

   //Loop through nonce cache entries
   for(i = 0; i < HTTP_SERVER_NONCE_CACHE_SIZE; i++)
   {
      //Point to the current entry
      entry = &context->nonceCache[i];

      //Check whether the entry is currently in used or not
      if(!entry->count)
         break;

      //Keep track of the oldest entry in the table
      if(timeCompare(entry->timestamp, oldestEntry->timestamp) < 0)
         oldestEntry = entry;
   }

   //The oldest entry is removed whenever the table runs out of space
   if(i >= HTTP_SERVER_NONCE_CACHE_SIZE)
      entry = oldestEntry;

   //Generate a new nonce
   error = context->settings.prngAlgo->read(context->settings.prngContext,
      nonce, HTTP_SERVER_NONCE_SIZE);

   //Check status code
   if(!error)
   {
      //Convert the byte array to hex string
      httpConvertArrayToHexString(nonce, HTTP_SERVER_NONCE_SIZE, entry->nonce);
      //Clear nonce count
      entry->count = 1;
      //Save the time at which the nonce was generated
      entry->timestamp = osGetTickCount();

      //Copy the nonce to the output buffer
      strcpy(output, entry->nonce);
      //Return the length of the nonce excluding the NULL character
      *length = HTTP_SERVER_NONCE_SIZE * 2;
   }
   else
   {
      //Random number generation failed
      memset(entry, 0, sizeof(HttpNonceCacheEntry));
   }

   //Release exclusive access to the nonce cache
   osMutexRelease(context->nonceCacheMutex);
   //Return status code
   return error;
}


/**
 * @brief Nonce verification
 * @param[in] context Pointer to the HTTP server context
 * @param[in] nonce NULL-terminated string containing the nonce
 * @param[in] nc NULL-terminated string containing the nonce count
 * @return Error code
 **/

error_t httpVerifyNonce(HttpServerContext *context,
   const char_t *nonce, const char_t *nc)
{
   error_t error;
   uint_t i;
   uint32_t count;
   systime_t time;
   HttpNonceCacheEntry *entry;

   //Check parameters
   if(nonce == NULL || nc == NULL)
      return ERROR_INVALID_PARAMETER;

   //Convert the nonce count to integer
   count = strtoul(nc, NULL, 16);
   //Get current time
   time = osGetTickCount();

   //Acquire exclusive access to the nonce cache
   osMutexAcquire(context->nonceCacheMutex);

   //Loop through nonce cache entries
   for(i = 0; i < HTTP_SERVER_NONCE_CACHE_SIZE; i++)
   {
      //Point to the current entry
      entry = &context->nonceCache[i];

      //Check nonce value
      if(!strcasecmp(entry->nonce, nonce))
      {
         //Make sure the nonce timestamp has not expired
         if((time - entry->timestamp) < HTTP_SERVER_NONCE_LIFETIME)
         {
            //Check nonce count to prevent replay attacks
            if(count >= entry->count)
            {
               //Update nonce count to the next expected value
               entry->count = count + 1;
               //We are done
               break;
            }
         }
      }
   }

   //Check whether the nonce is valid
   if(i < HTTP_SERVER_NONCE_CACHE_SIZE)
      error = NO_ERROR;
   else
      error = ERROR_NOT_FOUND;

   //Release exclusive access to the nonce cache
   osMutexRelease(context->nonceCacheMutex);
   //Return status code
   return error;
}

#endif


/**
 * @brief Retrieve the full pathname to the specified resource
 * @param[in] connection Structure representing an HTTP connection
 * @param[in] relative String containing the relative path to the resource
 * @param[out] absolute Resulting string containing the absolute path
 * @param[in] maxLen Maximum acceptable path length
 **/

void httpGetAbsolutePath(HttpConnection *connection,
   const char_t *relative, char_t *absolute, size_t maxLen)
{
   //Copy the root directory
   strcpy(absolute, connection->settings->rootDirectory);

   //Append the specified path
   pathCombine(absolute, relative, maxLen);

   //Clean the resulting path
   pathCanonicalize(absolute);
}


/**
 * @brief Compare filename extension
 * @param[in] filename Filename whose extension is to be checked
 * @param[in] extension String defining the extension to be checked
 * @return TRUE is the filename matches the given extension, else FALSE
 **/

bool_t httpCompExtension(const char_t *filename, const char_t *extension)
{
   uint_t n;
   uint_t m;

   //Get the length of the specified filename
   n = strlen(filename);
   //Get the length of the extension
   m = strlen(extension);

   //Check the length of the filename
   if(n < m)
      return FALSE;

   //Compare extensions
   if(!strncasecmp(filename + n - m, extension, m))
      return TRUE;
   else
      return FALSE;
}


/**
 * @brief Decode a percent-encoded string
 * @param[in] input NULL-terminated string to be decoded
 * @param[out] output NULL-terminated string resulting from the decoding process
 * @param[in] outputSize Size of the output buffer in bytes
 * @return Error code
 **/

error_t httpDecodePercentEncodedString(const char_t *input, char_t *output, size_t outputSize)
{
   size_t i;
   char_t buffer[3];

   //Check parameters
   if(input == NULL || output == NULL)
      return ERROR_INVALID_PARAMETER;

   //Decode the percent-encoded string
   for(i = 0; *input != '\0' && i < outputSize; i++)
   {
      //Check current character
      if(*input == '+')
      {
         //Replace '+' characters with spaces
         output[i] = ' ';
         //Advance data pointer
         input++;
      }
      else if(input[0] == '%' && input[1] != '\0' && input[2] != '\0')
      {
         //Process percent-encoded characters
         buffer[0] = input[1];
         buffer[1] = input[2];
         buffer[2] = '\0';
         //String to integer conversion
         output[i] = strtoul(buffer, NULL, 16);
         //Advance data pointer
         input += 3;
      }
      else
      {
         //Copy any other characters
         output[i] = *input;
         //Advance data pointer
         input++;
      }
   }

   //Check whether the output buffer runs out of space
   if(i >= outputSize)
      return ERROR_FAILURE;

   //Properly terminate the resulting string
   output[i] = '\0';
   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Convert byte array to hex string
 * @param[in] input Point to the byte array
 * @param[in] inputLength Length of the byte array
 * @param[out] output NULL-terminated string resulting from the conversion
 * @return Error code
 **/

void httpConvertArrayToHexString(const uint8_t *input, size_t inputLength, char_t *output)
{
   size_t i;

   //Hex conversion table
   static const char_t hexDigit[] =
   {
      '0', '1', '2', '3', '4', '5', '6', '7',
      '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
   };

   //Process byte array
   for(i = 0; i < inputLength; i++)
   {
      //Convert upper nibble
      output[i * 2] = hexDigit[(input[i] >> 4) & 0x0F];
      //Then convert lower nibble
      output[i * 2 + 1] = hexDigit[input[i] & 0x0F];
   }

   //Properly terminate the string with a NULL character
   output[i * 2] = '\0';
}

#endif
