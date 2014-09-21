/*
 * ethernet.h
 *
 *  Created on: Sep 14, 2014
 *      Author: ld0d
 */

#ifndef ETHERNET_H_
#define ETHERNET_H_

class EthernetHostState;

class EthernetHost
{
public:
	EthernetHost();
	~EthernetHost();

	void Init();

private:
	void InitStack();
	void InitDhcp();
	void InitHttp();

	EthernetHostState* _state;
};


#endif /* ETHERNET_H_ */
