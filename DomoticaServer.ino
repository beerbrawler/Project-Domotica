
#include <NewRemoteReceiver.h>
#include <NewRemoteTransmitter.h>
#include <RemoteTransmitter.h>
//  Domotica server with KAKU-controller 
//
// By Max Gerbrands, Student Computer Science NHL.
// V0.1, 19/1/2017. Works with Xamarin

// kaku, Gamma, APA3, codes based on Arduino -> Voorbeelden -> NewRemoteSwitch -> ShowReceivedCode
// 1 Addr 21177114 unit 0 on/off, period: 270us   replace with your own code
// Supported KaKu devices -> find, download en install corresponding libraries

#define unitCodeApa3      20144854  // replace with your own code

// Include files.
#include <time.h>
#include <SPI.h>                  // Ethernet shield uses SPI-interface
#include <Ethernet.h>             // Ethernet library (use Ethernet2.h for new ethernet shield v2)
#include <NewRemoteTransmitter.h> // Remote Control, Gamma, APA3


// Set Ethernet Shield MAC address  (check yours)
byte mac[] = { 0x40, 0x6c, 0x3f, 0x36, 0x64, 0x6a }; //Random mac adres
int ethPort = 3300;                                  // Take a free port (check your router)

#define RFPin        3  // output, pin to control the RF-sender (and Click-On Click-Off-device)
#define lowPin       5  // output, always LOW
#define highPin      6  // output, always HIGH
#define switchPin    7  // input, connected to some kind of inputswitch
#define infoPin      9  // output, more information
#define analogPin    0  // sensor value

EthernetServer server(ethPort);              // EthernetServer instance (listening on port <ethPort>).
NewRemoteTransmitter apa3Transmitter(unitCodeApa3, RFPin, 270, 3);  // APA3 (Gamma) remote, use pin <RFPin> 
//Is for the socket's
int socket = 0;                          // output, defines the socket 0 or 1
bool pinState = false;                   // Variable to store actual pin state
bool pinChange = false;                  // Variable to store actual pin change
bool pinState1 = false;                  // Variable to store actual pin state
bool pinChange1 = false;                 // Variable to store actual pin change
int  sensorValue = 0;                    // Variable to store actual sensor value

//Is for the timer
char Time[4];
char Time1[4];
volatile unsigned long seconds = 0;
unsigned long seconds_old = 0;
int Sec;
int Min;
int Hours;

ISR(TIMER1_OVF_vect) {
  TCNT1=0xBDC;
  seconds = seconds+1;
}

void setup()
{
   Serial.begin(9600);
   Serial.println("Domotica project, Arduino Domotica Server\n");
   
   //Init I/O-pins
   pinMode(switchPin, INPUT);            // hardware switch, for changing pin state
   pinMode(lowPin, OUTPUT);
   pinMode(highPin, OUTPUT);
   pinMode(RFPin, OUTPUT);
   pinMode(infoPin, OUTPUT);
   
   //Default states
   digitalWrite(switchPin, HIGH);        // Activate pullup resistors (needed for input pin)
   digitalWrite(lowPin, LOW);
   digitalWrite(highPin, HIGH);
   digitalWrite(RFPin, LOW);
   digitalWrite(infoPin, LOW);
   
   // for the internal timer
   TIMSK1=0x01; // enabled global and timer overflow interrupt;
   TCCR1A = 0x00; // normal operation page 148 (mode0);
   TCNT1=0xBDC; // set initial value to remove time error (16bit counter register)
   TCCR1B = 0x04; // start timer/ set clock
 

     settime();
   //Try to get an IP address from the DHCP server.
   if (Ethernet.begin(mac) == 0)
   {
      Serial.println("Could not obtain IP-address from DHCP -> do nothing");
      while (true){     // no point in carrying on, so do nothing forevermore; check your router
      }
   }
  
   Serial.print("Input switch on pin "); Serial.println(switchPin);
   Serial.println("Ethernetboard connected (pins 10, 11, 12, 13 and SPI)");
   Serial.println("Connect to DHCP source in local network");
   
   //Start the ethernet server.
   server.begin();

   // Print IP-address
   Serial.print("Listening address: ");
   Serial.print(Ethernet.localIP());
   
   // for hardware debug
   int IPnr = getIPComputerNumber(Ethernet.localIP());   // Get computernumber in local network 192.168.1.3 -> 3)
   Serial.print(" ["); Serial.print(IPnr); Serial.print("] "); 
   Serial.print("  [Testcase: telnet "); Serial.print(Ethernet.localIP()); Serial.print(" "); Serial.print(ethPort); Serial.println("]");

}

void loop()
{
  


   
   // Listen for incomming connection (app)
   EthernetClient ethernetClient = server.available();
   if (!ethernetClient) {
      return; // wait for connection
   }

   // Internal Timer
  if (seconds_old != seconds){
    seconds_old = seconds;
    Sec++;
    if (Sec == 60){
      Sec = 0;
      Min++;
      if (Min == 60){
        Min = 0;
        Hours++;
        if (Hours == 24){
          Hours = 0;
        }
      }
     }
    Serial.print(Hours);
    Serial.print(':');
    Serial.print(Min);
    Serial.print(':');
    Serial.println(Sec);

   Serial.println("Application connected");

   // Do what needs to be done while the socket is connected.
   while (ethernetClient.connected()) 
   {
      checkEvent(switchPin, pinState);          // update pin state
      checkEvent(switchPin, pinState1);         // update pin1 state
      sensorValue = readSensor(0, 100);         // update sensor value
        
      // Activate pin based op pinState
      if (pinChange) {
         if (pinState) { switchDefault(true); }
         else { switchDefault(false); }
         pinChange = false;
      }
      if (pinChange1) {
         if (pinState1) { switchDefault(true); }
         else { switchDefault(false); }
         pinChange1 = false;
      }
      // Execute when byte is received.
      while (ethernetClient.available())
      {
         char inByte = ethernetClient.read();   // Get byte from the client.
         executeCommand(inByte);                // Wait for command to execute
         inByte = NULL;                         // Reset the read byte.
      } 
   }
   Serial.println("Application disonnected");
}
}

// Choose and switch your Kaku device, state is true/false (HIGH/LOW)
void switchDefault(bool state)
{  
   Serial.print("code send");
   apa3Transmitter.sendUnit(socket, state);          // APA3 Kaku (Gamma)                
   delay(100);
}

// Implementation of (simple) protocol between app and Arduino
// Request (from app) is single char ('a', 's', 't', 'i' etc.)
// Response (to app) is 4 chars  (not all commands demand a response)
void executeCommand(char cmd)
{     
         char buf[6] = {'\0', '\0', '\0', '\0', '\0', '\0'};

         // Command protocol
         Serial.print("["); Serial.print(cmd); Serial.print("] -> ");
         switch (cmd) {
         case 'a': // Report sensor value to the app  
            intToCharBuf(sensorValue, buf, 4);                // convert to charbuffer
            server.write(buf, 4);                             // response is always 4 chars (\n included)
            Serial.print("Sensor: "); Serial.println(buf);
            break;
         case 'b': // send pin/socket state 
            intToCharBuf(sensorValue, buf, 4);                // convert to charbuffer
            server.write(buf, 4);                             // response is always 4 chars (\n included)
            Serial.print("Socket1 sate:  "); Serial.println(pinState); Serial.print("Socket2 state:  "); Serial.println(pinState1); // shows the state of the pin/sockets
            break;
          case 'u': // Toggle state of socket1; If state is already ON then turn it OFF
            socket = 1;
            if (pinState1) { pinState1 = false; Serial.println("Set pin state to \"OFF\""); }
            else { pinState1 = true; Serial.println("Set pin state to \"ON\""); }  
            pinChange1 = true; 
            break;
         case 't': // Toggle state; If state is already ON then turn it OFF
            socket = 0;
            if (pinState) { pinState = false; Serial.println("Set pin state to \"OFF\""); }
            else { pinState = true; Serial.println("Set pin state to \"ON\""); }  
            pinChange = true; 
            break;
         case 'i':    
            digitalWrite(infoPin, HIGH);
            break;
         default:
            digitalWrite(infoPin, LOW);
         }
}

// read value from pin pn, return value is mapped between 0 and mx-1
int readSensor(int pn, int mx)
{
  return map(analogRead(pn), 0, 1023, 0, mx-1);    
}

// Convert int <val> char buffer with length <len>
void intToCharBuf(int val, char buf[], int len)
{
   String s;
   s = String(val);                        // convert tot string
   if (s.length() == 1) s = "0" + s;       // prefix redundant "0" 
   if (s.length() == 2) s = "0" + s;  
   s = s + "\n";                           // add newline
   s.toCharArray(buf, len);                // convert string to char-buffer
}

// Check switch level and determine if an event has happend
// event: low -> high or high -> low
void checkEvent(int p, bool &state)
{
   static bool swLevel = false;       // Variable to store the switch level (Low or High)
   static bool prevswLevel = false;   // Variable to store the previous switch level

   swLevel = digitalRead(p);
   if (swLevel)
      if (prevswLevel) delay(1);
      else {               
         prevswLevel = true;   // Low -> High transition
         state = true;
         pinChange = true;
         pinChange1 = true;
      } 
   else // swLevel == Low
      if (!prevswLevel) delay(1);
      else {
         prevswLevel = false;  // High -> Low transition
         state = false;
         pinChange = true;
         pinChange1 = true;
      }
}

// Convert IPAddress tot String (e.g. "192.168.1.105")
String IPAddressToString(IPAddress address)
{
    return String(address[0]) + "." + 
           String(address[1]) + "." + 
           String(address[2]) + "." + 
           String(address[3]);
}

// Returns B-class network-id: 192.168.1.3 -> 1)
int getIPClassB(IPAddress address)
{
    return address[2];
}

// Returns computernumber in local network: 192.168.1.3 -> 3)
int getIPComputerNumber(IPAddress address)
{
    return address[3];
}

// Returns computernumber in local network: 192.168.1.105 -> 5)
int getIPComputerNumberOffset(IPAddress address, int offset)
{
    return getIPComputerNumber(address) - offset;
}

void settime(){
  String Time = __TIME__;
  Hours = (Time.charAt(0)-48)*10+Time.charAt(1)-48;
  Min = (Time.charAt(3)-48)*10+Time.charAt(4)-48;
  Sec = (Time.charAt(6)-48)*10+Time.charAt(7)-48;
}
