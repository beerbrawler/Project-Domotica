#include "CommandServer.h"

CommandServer::CommandServer(unsigned int port) : server(port) { }

bool CommandServer::begin() {
	server.begin();
}

void CommandServer::update() {
	EthernetClient client = server.available();
	while (client.connected()) {
		if (connectHandler) connectHandler(client);
		
		uint8_t command;		
		if (client.available()) {
			command = client.read();
			
			if (!commands[command]) {
				if (errorHandler) errorHandler(client);
				if (disconnectHandler) disconnectHandler(client);
				client.stop();
				return;
			}
		} else {
			if (errorHandler) errorHandler(client);
			if (disconnectHandler) disconnectHandler(client);
			client.stop();
			return;
		}
		
		uint8_t buffer[client.available()];
		int i = 0;
		while (client.available()) {
		  buffer[i++] = client.read();
		}
		
		commands[command](client, buffer, i);
		if (disconnectHandler) disconnectHandler(client);
		client.stop();
	}
}

void CommandServer::addCommand(uint8_t id, handler callback) {
	commands[id] = callback;
}

void CommandServer::setOnErrorHandler(event callback) {
	errorHandler = callback;
}

void CommandServer::setOnConnectHandler(event callback) {
	connectHandler = callback;
}

void CommandServer::setOnDisconnectHandler(event callback) {
	disconnectHandler = callback;
}