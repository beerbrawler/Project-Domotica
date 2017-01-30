#include <SPI.h>
#include <Ethernet.h>
#include <CommandServer.h>
#include <VirtualWire.h>

#define PORT 3000
#define STATUS_LED_PIN 7
#define TRANSMITTER_PIN 3
#define RECEIVER_PIN 2

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD };

CommandServer server(PORT);

void setup() {
  Serial.begin(9600);
  
  Serial.println(F("Setting up transmitter"));
  vw_set_tx_pin(TRANSMITTER_PIN);
  vw_set_ptt_inverted(true);
  vw_setup(2000);
  Serial.println(F("Transmitter ready for action"));

  Serial.println(F("Starting server"));
  if (!Ethernet.begin(mac)) {
    Serial.println(F("Failed to obtain DHCP address"));
    Serial.println(F("Foute boel. Tijd om niks te doen."));
    while (true) ;
  }

  server.begin();
  server.setOnErrorHandler(&onError);
  server.setOnConnectHandler(&onConnect);
  server.setOnDisconnectHandler(&onDisconnect);
  server.addCommand('p', &handlePing);
  server.addCommand('a', &handleBroadcast);

  Serial.print(F("Listening to connections on "));
  Serial.print(Ethernet.localIP());
  Serial.print(":");
  Serial.println(PORT);
}

void loop() {
  server.update();
}

void onError(EthernetClient client) {
  Serial.println(F("Error!"));
}

void onConnect(EthernetClient client) {
  Serial.println(F("Incoming connection"));
}

void onDisconnect(EthernetClient client) {
  Serial.println(F("Disconnected"));
}

bool handlePing(EthernetClient client, uint8_t * args, int length) {
  Serial.println(F("Pong!"));
  client.write("pong");
  
  return true;
}

bool handleBroadcast(EthernetClient client, uint8_t * args, int length) {
  vw_send(args, length);
  vw_wait_tx();

  Serial.print(F("Broadcasted: "));
  for (int i = 0; i < length; i++) {
    Serial.println(args[i]);
  }
  
  return true;
}

