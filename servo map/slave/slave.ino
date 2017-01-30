#include <VirtualWire.h>
#include <ServoTimer2.h>

const int KEY = 81;

const int RF_RECEIVE_PIN = 7;

const int ULTRA_ECHO_PIN = 6;
const int ULTRA_TRIGGER_PIN = 11;
const int ULTRA_MIN_DISTANCE = 2; // inch
const int ULTRA_MAX_DISTANCE = 6; // inch

const int SERVO_PIN = 5;
const int SERVO_MIN_POS = 544;
const int SERVO_MAX_POS = 1472;
const int SERVO_STEP_SIZE = 32;

const int MAX_SWEEPS = 4;

bool start_signal = false;
int distance = 0;
int old_distance = distance;
bool alternate = true;
int sweep_count = 0;
ServoTimer2 myservo;

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(100);
  // RF communication
  vw_set_rx_pin(RF_RECEIVE_PIN);
  vw_set_ptt_inverted(true);
  vw_setup(2000);
  vw_rx_start();

  pinMode(ULTRA_ECHO_PIN, INPUT);
  pinMode(ULTRA_TRIGGER_PIN, OUTPUT);

  // Set start position
  open_servo();
}

void loop() {
  // RF start signal received
  uint8_t id = receive_id();
  if (id && id == KEY) {
    start_signal = true;
    sweep_count = 0;
  }

  // For old vs new distance comparison
  old_distance = distance ? distance : old_distance;
  distance = ping_distance();

  // Movement detected
  if ((old_distance > 0) &&
      (distance != old_distance) &&
      (distance <= ULTRA_MAX_DISTANCE) &&
      (distance >= ULTRA_MIN_DISTANCE) &&
      (sweep_count < MAX_SWEEPS) || start_signal) {
    if (start_signal) start_signal = false;

    // Move servo
    if (alternate) close_servo();
    else open_servo();

    sweep_count++;

    alternate = !alternate;
    delay(250);
  }
}

String receive_msg() {
  uint8_t buf[VW_MAX_MESSAGE_LEN];
  uint8_t buflen = VW_MAX_MESSAGE_LEN;
  String msg = "";
  
  if (vw_get_message(buf, &buflen)) {
    for (int i = 0; i < buflen; i++) {
      msg += char(buf[i]);
    }
  }
  return msg;
}

uint8_t receive_id() {
  uint8_t buf[VW_MAX_MESSAGE_LEN];
  uint8_t buflen = VW_MAX_MESSAGE_LEN;
  
  if (vw_get_message(buf, &buflen)) {
    for (int i = 0; i < buflen; i++) {
      Serial.print("R: ");
      Serial.println(buf[i]);
    }

    return buf[0];
  }

  return 0;
}

unsigned int ping_distance() {
  delay(50);
  unsigned long duration, distance;
  digitalWrite(ULTRA_TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRA_TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRA_TRIGGER_PIN, LOW);
  duration = pulseIn(ULTRA_ECHO_PIN, HIGH);
  return (duration/58.138) * .39; // remove *.39 for cm
}

void open_servo() {
  // Attach/detach to prevent RF interference
  myservo.attach(SERVO_PIN);
  for (int pos = SERVO_MAX_POS; pos >= SERVO_MIN_POS; pos -= SERVO_STEP_SIZE) {
    myservo.write(pos);
    delay(10);
  }
  myservo.detach();
}

void close_servo() {
  // Attach/detach to prevent RF interference
  myservo.attach(SERVO_PIN);
  for (int pos = SERVO_MIN_POS; pos <= SERVO_MAX_POS; pos += SERVO_STEP_SIZE) {
    myservo.write(pos);
    delay(10);
  }
  myservo.detach();
}


