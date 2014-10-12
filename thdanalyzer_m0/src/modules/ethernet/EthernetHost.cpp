/*
 * ethernet.cpp
 *
 *  Created on: Sep 14, 2014
 *      Author: ld0d
 */



#include "EthernetHost.h"

extern "C" {
#include "tcp_ip_stack.h"
#include "dhcp_client.h"
#include "http_server.h"

#include "lpc43xx_eth.h"
#include "lan8720.h"
};

class EthernetHostState
{
public:
	DhcpClientSettings dhcpClientSettings;
	DhcpClientCtx dhcpClientContext;

	HttpServerSettings httpServerSettings;
	HttpServerContext httpServerContext;

	NetInterface *interface;
};

EthernetHostState gState;

EthernetHost::EthernetHost()
: _state(&gState)
{
}

EthernetHost::~EthernetHost()
{
}

void EthernetHost::Init()
{
	InitStack();
	InitDhcp();
	InitHttp();
}

void EthernetHost::InitStack()
{
	error_t error;

	// Initialize IP stack
	error = tcpIpStackInit();

	// Configure the first Ethernet interface
	_state->interface = &netInterface[0];

	// Set interface name
	tcpIpStackSetInterfaceName(_state->interface, "eth0");

	//Set host name
	tcpIpStackSetHostname(_state->interface, "Analyzer-1");

	//Select the relevant network adapter
	tcpIpStackSetDriver(_state->interface, &lpc43xxEthDriver);
	tcpIpStackSetPhyDriver(_state->interface, &lan8720PhyDriver);

	//Set host MAC address
	MacAddr macAddr;
	macStringToAddr("00-AB-CD-EF-01-02", &macAddr);
	tcpIpStackSetMacAddr(_state->interface, &macAddr);

	//Initialize network interface
	error = tcpIpStackConfigInterface(_state->interface);
	//Any error to report?
	if(error)
	{
		//Debug message
		//TRACE_ERROR("Failed to configure interface %s!\r\n", interface->name);
	}
}

void EthernetHost::InitDhcp()
{
	error_t error;

#if (IPV4_SUPPORT == ENABLED)
   //Get default settings
   dhcpClientGetDefaultSettings(&_state->dhcpClientSettings);
   //Set the network interface to be configured by DHCP
   _state->dhcpClientSettings.interface = _state->interface;
   //Disable rapid commit option
   _state->dhcpClientSettings.rapidCommit = FALSE;

   //DHCP client initialization
   error = dhcpClientInit(&_state->dhcpClientContext, &_state->dhcpClientSettings);
   //Failed to initialize DHCP client?
   if(error)
   {
      //Debug message
      //TRACE_ERROR("Failed to initialize DHCP client!\r\n");
	   return;
   }

   //Start DHCP client
   error = dhcpClientStart(&_state->dhcpClientContext);
   //Failed to start DHCP client?
   if(error)
   {
      //Debug message
      //TRACE_ERROR("Failed to start DHCP client!\r\n");
   }
#endif
}

static error_t HttpCgiHeaderCallback(HttpConnection *connection, HttpResponse *response)
{
	static const char mimeType[] = "application/octet-stream";
	response->contentType = mimeType;

	return NO_ERROR;
}

static error_t HttpCgiCallback(HttpConnection *connection, const char_t *param)
{
	int32_t** readPtr = (int32_t**) 0x2000C000;
	int32_t* startPtr = *readPtr;
	const int32_t bufferend = 16*1048576;
	int32_t startpos = (int32_t)startPtr & 0xFFFFFF;
	int32_t endpos = startpos + 16*1048576/2;

	int firstpartlen = 0;
	int secondpartlen = 0;
	if (endpos > bufferend) {
		secondpartlen = endpos - bufferend;
		firstpartlen = bufferend - startpos;
	}
	else {
		firstpartlen = endpos - startpos;
		secondpartlen = 0;
	}

	int secondstart = 0x28000000;
	httpWriteStream(connection, &firstpartlen, 4);
	httpWriteStream(connection, &startPtr, 4);
	httpWriteStream(connection, &secondpartlen, 4);
	httpWriteStream(connection, &secondstart, 4);

	if (firstpartlen > 0) {
		httpWriteStream(connection, (void*)startPtr, firstpartlen);
	}
	if (secondpartlen > 0) {
		httpWriteStream(connection, (void*)0x28000000, secondpartlen);
	}
}

void EthernetHost::InitHttp()
{
	error_t error;

#if (IPV4_SUPPORT == ENABLED)
	httpServerGetDefaultSettings(&_state->httpServerSettings);
   _state->httpServerSettings.interface = _state->interface;
   _state->httpServerSettings.cgiCallback = HttpCgiCallback;
   _state->httpServerSettings.cgiHeaderCallback = HttpCgiHeaderCallback;

   error = httpServerInit(&_state->httpServerContext, &_state->httpServerSettings);
   //Failed to initialize DHCP client?
   if(error)
   {
	   return;
   }

   //Start DHCP client
   error = httpServerStart(&_state->httpServerContext);
   //Failed to start DHCP client?
   if(error)
   {
      //Debug message
      //TRACE_ERROR("Failed to start DHCP client!\r\n");
   }
#endif
}
