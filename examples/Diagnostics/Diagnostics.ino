/*
  CRCibernetica.com
  http://enlace al producto

  On Arduino IDE select: "ESP32 Dev Module"

  Libraries that you need to install for this example:
  1. TinyGSM
  2. Wire (alredy in your Arduino default instalation)
  3. StreamDebugger


  This examples test basic functionality for your board
  1. ESP32 has comunication with SIM800 module?
  2. Can unlock your SIM card?
  3. Can connecto to APN
  4. Can connect to a test Server
  5. Download a little file to test GPRS

  Adapted from TinyGSM>tools>Diagnostics example
*/

// Configure TinyGSM library
#define TINY_GSM_MODEM_SIM800      // Modem is SIM800
#define TINY_GSM_RX_BUFFER   1024  // Set RX buffer to 1Kb

#include <Wire.h>
#include <TinyGsmClient.h>

// TTGO T-Call pins
#define MODEM_RST            5
#define MODEM_PWKEY          4
#define MODEM_POWER_ON       23
#define MODEM_TX             27
#define MODEM_RX             26
#define I2C_SDA              21
#define I2C_SCL              22


// Set serial for debug console (to Serial Monitor, default speed 115200)
#define SerialMon Serial
// Set serial for AT commands (to SIM800 module)
#define SerialAT Serial1

// Define the serial console for debug prints, if needed
//#define DUMP_AT_COMMANDS

#ifdef DUMP_AT_COMMANDS
  #include <StreamDebugger.h>
  StreamDebugger debugger(SerialAT, SerialMon);
  TinyGsm modem(debugger);
#else
  TinyGsm modem(SerialAT);
#endif

// Your GPRS credentials (leave empty, if not needed)
const char apn[]      = ""; // APN (example: kobi3g) use https://wiki.apnchanger.org
const char gprsUser[] = ""; // GPRS User
const char gprsPass[] = ""; // GPRS Password

// SIM card PIN (leave empty, if not defined)
const char simPIN[]   = ""; 

// Server details
// The server variable can be just a domain name or it can have a subdomain. It depends on the service you are using
const char server[] = "vsh.pp.ua";
const char resource[] = "/TinyGSM/logo.txt";
const int  port = 80;                       // server port number

// I2C for SIM800 (to keep it running when powered from battery)
TwoWire I2CPower = TwoWire(0);

// TinyGSM Client for Internet connection
TinyGsmClient client(modem);

//#define uS_TO_S_FACTOR 1000000UL   /* Conversion factor for micro seconds to seconds */

void setup() {
  // Set modem reset, enable, power pins
  pinMode(MODEM_POWER_ON, OUTPUT);
  pinMode(MODEM_PWKEY, OUTPUT);
  pinMode(MODEM_RST, OUTPUT);
  digitalWrite(MODEM_POWER_ON, HIGH);
  digitalWrite(MODEM_PWKEY, LOW);
  digitalWrite(MODEM_RST, HIGH);

  SerialMon.begin(115200);// Set serial monitor debugging window baud rate to 115200

  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);// Set GSM module baud rate and UART pins
  delay(3000);

  // Start I2C communication
  I2CPower.begin(I2C_SDA, I2C_SCL, 400000);
  
  // Restart SIM800 module, it takes quite some time
  // To skip it, call init() instead of restart()
  SerialMon.println("Initializing modem...");
  modem.restart();
  // use modem.init() if you don't need the complete restart

  // Unlock your SIM card with a PIN if needed
  if (strlen(simPIN) && modem.getSimStatus() != 3 ) {
    modem.simUnlock(simPIN);
  }
}

void loop() {
  SerialMon.print("Connecting to APN: ");
  SerialMon.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    SerialMon.println(" fail");
  }
  else {
    SerialMon.println(" OK");
    
    SerialMon.print("Connecting to ");
    SerialMon.print(server);
    if (!client.connect(server, port)) {
      SerialMon.println(" fail");
    }
    else {
      SerialMon.println(" OK");

  // Make a HTTP GET request:
  client.print(String("GET ") + resource + " HTTP/1.0\r\n");
  client.print(String("Host: ") + server + "\r\n");
  client.print("Connection: close\r\n\r\n");

  // Wait for data to arrive
  while (client.connected() && !client.available()) {
    delay(100);
    SerialMon.print('.');
  };
  SerialMon.println();

  // Skip all headers
  client.find("\r\n\r\n");

  // Read data
  unsigned long timeout = millis();
  unsigned long bytesReceived = 0;
  while (client.connected() && millis() - timeout < 10000L) {
    while (client.available()) {
      char c = client.read();
      //SerialMon.print(c);
      bytesReceived += 1;
      timeout = millis();
    }
  }

  client.stop();
  SerialMon.println(F("Server disconnected"));

  modem.gprsDisconnect();
  SerialMon.println(F("GPRS disconnected"));

  SerialMon.println();
  SerialMon.println(F("************************"));
  SerialMon.print  (F(" Received: "));
  SerialMon.print(bytesReceived);
  SerialMon.println(F(" bytes"));
  SerialMon.print  (F(" Test:     "));
  SerialMon.println((bytesReceived == 121) ? "PASSED" : "FAILED");
  SerialMon.println(F("************************"));
    }
  }
}