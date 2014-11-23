extern "C" {
#include "tcp_ip_stack.h"
#include "dhcp_client.h"
#include "http_server.h"

#include "lpc43xx_eth.h"
#include "lan8720.h"
};

#include "EthernetHost.h"
#include "HttpResource.h"
#include "CgiCallback.h"

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

const char* EthernetHost::IpAddress() const
{
	static char buf[17];
	buf[0] = '\0';

	if (gState.interface == 0) {
		return buf;
	}

	IpAddr addr;
	addr.length = 4;
	addr.ipv4Addr = gState.interface->ipv4Config.addr;
	ipAddrToString(&addr, buf);
	return buf;
}

const char* EthernetHost::HostName() const
{
	static char buf[17];
	buf[0] = '\0';

	if (gState.interface == 0) {
		return buf;
	}

	strncpy(buf, gState.interface->hostname, 16);
	buf[16] = '\0';
	return buf;
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

void EthernetHost::InitHttp()
{
	error_t error;

#if (IPV4_SUPPORT == ENABLED)

	InitHttpResources();

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

EthernetHost ethhost;
