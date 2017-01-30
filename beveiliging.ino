int kalibratietijd = 30;
long unsigned int LowIn ;   // tijd wanneer sensor outputs laag impuls
long unsigned int pauze = 30000; // na deze tijd mag de arduino aannemen dat er geen beweging meer is
boolean lockLow = true;
boolean takeLowTime;
#define irLedPin 5
#define irSensorPin 4
#define echoPin 3
#define triggerPin 2
#define ledPin 1

int maximumRange = 100;
int minimumRange = 0;
long duration, distance;
int tijden[20];
int y = 0;
int tijdvannu = 0;
bool melding = false;
int aantal = 0;

volatile unsigned long seconden = 0;
unsigned long seconden_oud = 0;

int seco;
int minu;
int uren;

unsigned int ethPort = 3300;
byte mac [] = { 0x40, 0x6c, 0x8f, 0x36, 0x84, 0x8a };
//IPAddress ip(192, 168, 1, 102);

ISR(TIMER1_OVF_vect) {
  TCNT1 = 0xBDC;
  seconden = seconden + 1;
};

#include <SPI.h>
#include <Ethernet.h>
EthernetServer server(ethPort);


void setup() {
  Serial.begin(9600);

  TIMSK1 = 0x01; // enabled global and timer overflow interrupt;
  TCCR1A = 0x00; // normal operation page 148 (mode0);
  TCNT1 = 0xBDC; // set initial value to remove time error (16bit counter register)
  TCCR1B = 0x04; // start timer/ set clock

  zetTijd();

  if (Ethernet.begin(mac) == 0)
  {
    Serial.print("Geen address te vinden!");
    while (true) {}
  }


  Serial.println("IP address ");
  Serial.print(Ethernet.localIP());
  //Serial.print("192.168.1.102")
  Ethernet.begin(mac, Ethernet.localIP());
  server.begin();

  pinMode(triggerPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledPin, OUTPUT);

  pinMode(irSensorPin, INPUT);
  pinMode(irLedPin, OUTPUT);
  digitalWrite(irSensorPin, LOW);

  //kalibratie tijd
  Serial.println("---");
  Serial.println("Kalibratie van sensor is bezig");
  for (int i = 0; i < kalibratietijd; i++) {
    Serial.print(".");
    delay(200);
  }
  Serial.println("");
  Serial.println("Klaar ");
  Serial.println("Sensor actief!");
  delay(50);
}

void loop() {
  EthernetClient ethernetClient = server.available();

  if (seconden_oud != seconden) {
    seconden_oud = seconden;
    seco++;
    if (seco == 60) {
      seco = 0;
      minu++;
      if (minu == 60) {
        minu = 0;
        uren++;
        if (uren == 24) {
          uren = 0;
        }
      }
    }
  }

  while (ethernetClient.available())
  {
    char inByte = ethernetClient.read();
    executeCommand(inByte);
    inByte = NULL;
  }

  if (digitalRead(irSensorPin) == HIGH) {
    digitalWrite(irLedPin, HIGH);   //als de sensor hoog is moet de pin ook hoog zijn
    if (lockLow) {
      while (ethernetClient.available())
      {
        char inByte = ethernetClient.read();
        executeCommand(inByte);
        inByte = NULL;
      }
      lockLow = false;
      Serial.println("---");
      Serial.println("motion gedetecteerd bij ");
      if (uren < 10)
      {
        Serial.print("0");
      }
      Serial.print(uren);
      Serial.print(":");
      if (minu < 10)
      {
        Serial.print("0");
      }
      Serial.print(minu);
      digitalWrite(triggerPin, LOW);
      delayMicroseconds(2);

      digitalWrite(triggerPin, HIGH);
      delayMicroseconds(10);

      digitalWrite(triggerPin, LOW);
      duration = pulseIn(echoPin, HIGH);

      distance = (duration / 2) / 29.2;
      if (distance >= maximumRange || distance <= minimumRange) {
        //Serial.println("-1");
        digitalWrite(ledPin, HIGH);
      }
      else {
        Serial.println(distance);
        Serial.print(" cm ");
        digitalWrite(ledPin, LOW);
      }
      delay(50);
    }
  }
  melding = true;
  if (melding == true && distance >= maximumRange || distance <= minimumRange) {
    if (y < 20) {
      {
        if (((uren * 100) + minu) != tijden[y - 1])
        { tijden[y] = ((uren * 100) + minu);
          Serial.println(tijden[y]);
          y++;
          melding = false;
        }
      }
    }
    else {
      if (((uren * 100) + minu) != tijden[y - 1] && ((uren * 100) + minu) != tijden[19])
      { y = 0;
        tijden[y] = ((uren * 100) + minu);
        Serial.println(tijden[y]);
        melding = false;
      }
    }
  }
  takeLowTime = true;


  if (digitalRead(irSensorPin) == LOW) {
    digitalWrite(irLedPin, LOW);  //als de sensor laag is moet de pin ook laag zijn

    if (takeLowTime) {
      LowIn = millis();          //tijd van hoog naar laag
      takeLowTime = false;
    }
    //als de sensor langer laag blijft dan de pauze is er nieuwe motion
    if (!lockLow && millis() - LowIn > pauze) {
      lockLow = true;
      Serial.println("motion gestopt bij ");
      if (uren < 10)
      {
        Serial.print("0");
      }
      Serial.print(uren);
      Serial.print(":");
      if (minu < 10)
      {
        Serial.print("0");
      }
      Serial.print(minu);
      delay(50);
    }
    digitalWrite(triggerPin, LOW);
  }
}
void zetTijd() {
  String tijd = __TIME__;
  uren = (tijd.charAt(0) - 48) * 10 + tijd.charAt(1) - 48;
  minu = (tijd.charAt(3) - 48) * 10 + tijd.charAt(4) - 48;
  seco = (tijd.charAt(6) - 48) * 10 + tijd.charAt(7) - 48;
}

void executeCommand(char cmd)
{
  switch (cmd) {
    case 'q': //update log
      for (int i = 0; i <= y; i++)
      {
        if (tijden[i] != 0 && tijden[i] != tijden[i - 1])
        {
          String tijd = String(tijden[i]);

          if (tijden[i] < 1000)
          {
            tijd = "0" + tijd;
          }
          for (int x = 0; x < 4; x++)
          {
            char c = tijd.charAt(x);
            server.write(c);
            Serial.println("{" + String(c) + "} + ");
          }
          Serial.println("{" + tijd + "} ");
        }

      }
    /*case 'q': //melding van motion
      if (tijdvannu > 19)
      {
        tijdvannu = 0;
      }
      String tijd = String(tijden[tijdvannu]);

      if (tijden[tijdvannu] < 1000)
      {
        tijd = "0" + tijd;
      }

      for (int a = 0; a < 4; a++)
      { char n = tijd.charAt(a);
        server.write(n);
        Serial.println("{" + String(n) + "} + ");
      }
      Serial.println("{" + tijd + "} ");
      tijdvannu++;
  }*/
}

