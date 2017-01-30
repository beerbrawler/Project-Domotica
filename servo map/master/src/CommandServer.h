#pragma once

#include <SPI.h>
#include <Ethernet.h>

typedef bool (*handler)(EthernetClient client, uint8_t * args, int length);
typedef void (*event)(EthernetClient client);

class CommandServer 
{
	public:
		CommandServer(unsigned int port = 3000);
		bool begin(void);
		void update(void);
		void addCommand(uint8_t id, handler callback);
		void setOnErrorHandler(event callback);
		void setOnConnectHandler(event callback);
		void setOnDisconnectHandler(event callback);
	private:
		handler commands[255];
		event errorHandler;
		event connectHandler;
		event disconnectHandler;
		EthernetServer server;
};