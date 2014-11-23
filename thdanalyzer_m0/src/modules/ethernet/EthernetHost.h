#ifndef ETHERNET_H_
#define ETHERNET_H_

class EthernetHostState;

class EthernetHost
{
public:
	EthernetHost();
	~EthernetHost();

	void Init();

	const char* IpAddress() const;
	const char* HostName() const;

private:
	void InitStack();
	void InitDhcp();
	void InitHttp();

	EthernetHostState* _state;
};

extern EthernetHost ethhost;

#endif /* ETHERNET_H_ */
