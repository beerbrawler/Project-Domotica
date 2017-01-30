#include <VirtualWire.h>

const int RF_TRANSMIT_PIN = 7;

void setup() {
  // RF communication
  vw_set_tx_pin(RF_TRANSMIT_PIN);
  vw_set_ptt_inverted(true);
  vw_setup(2000);
}

void loop() {
  // Read from serial for testing purposes
  if (Serial.available() > 0) {
    String msg = Serial.readString();
    if (msg.length() > 0)
      transmit_msg(msg);
  }
}

void transmit_msg(String msg) {
  char buf[msg.length()+1];
  msg.toCharArray(buf, msg.length()+1);
  vw_send((uint8_t *)buf, msg.length()+1);
  vw_wait_tx();
}
