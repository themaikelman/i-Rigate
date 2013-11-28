#include <EEPROM.h> // library to store information in firmware
#include <TrueRandom.h> // library for better randomization http://code.google.com/p/tinkerit/wiki/TrueRandom
// #include <JeeLib.h> // Low power functions library

#include <Adafruit_CC3000.h>
#include <SPI.h>
#include <sha1.h>

// DEFINICION DE PINES

// CC3000 interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3 // MUST be an interrupt pin!
#define ADAFRUIT_CC3000_VBAT  5 // These can be
#define ADAFRUIT_CC3000_CS   10 // any two pins
// Hardware SPI required for remaining pins.
// SCK = 13
// MISO = 12
// MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT, SPI_CLOCK_DIV2);

#define COMMLED 2 // LED that indicates communication status
#define MOISTLED 6  // LED that indicates the plant needs water
#define STATUSLED 8 // generic status LED

#define CLAVONEGRO A5 // moisture input is on analog pin
#define CLAVOROJO A3 // feeds power to the moisture probes (analog)
#define MOTORAGUA 9  // input for normally open momentary switch
#define TARGETPIN 4 // minimum level of satisfactory moisture

// All messages need to be less than 129 characters
// Arduino RAM is limited. If code fails, try shorter messages
#define URGENT_WATER "URGENTE! Que alguien me riegue!"
#define WATER "Una ronda de agua?"
#define WATER_OK "Todo va bien."
#define THANK_YOU "Gracias por el trago!"
#define OVER_WATERED "Glup glup..."
#define UNDER_WATERED "Dame mas agua!"

//tracks the state to avoid erroneously repeated tweets
#define URGENT_SENT 3
#define WATER_SENT 2
#define MOISTURE_OK 1

// #define MOIST 425 // minimum level of satisfactory moisture
#define HYSTERESIS 25 // stabilization value http://en.wikipedia.org/wiki/Hysteresis
#define SECO 200  // maximum level of tolerable dryness
#define WATERING_CRITERIA 100 // minimum change in value that indicates watering
#define MOIST_SAMPLE_INTERVAL 240 // seconds over which to average moisture samples
#define WATERED_INTERVAL 120 // seconds between checks for watering events
#define TWITTER_INTERVAL 1000 // minimum miliseconds between twitter postings
#define MOIST_SAMPLES 10 //number of moisture samples to average


// WiFi access point credentials
#define WLAN_SSID       "Love_Paradise"        // cannot be longer than 32 characters!
#define WLAN_PASS       "..antzokia..26112005"
#define WLAN_SECURITY   WLAN_SEC_WPA // This can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2

const char PROGMEM
  // Twitter application credentials -- see notes above -- DO NOT SHARE.
  consumer_key[]  = "1LMG9W2BTxxxEzBgIwNFw",
  access_token[]  = "1969302930-wOa0PpG26BqIXvynW7wSWFL3vQxgySNiczT0GkR",
  signingKey[]    = "uDSW40uq3saf0KfLzB8eU9H5GThBogY7fTPXLMOM"      // Consumer secret
    "&"             "8cYiXLbROOpPObfsFLn7Xtzrl76mlQwJo3CSHJlD2WrGO", // Access token secret
  endpoint[]      = "/1.1/statuses/update.json",
  agent[]         = "Arduino-Tweet-Test v1.0";
const char
  host[]          = "api.twitter.com";
const unsigned long
  dhcpTimeout     = 60L * 1000L, // Max time to wait for address from DHCP
  connectTimeout  = 15L * 1000L, // Max time to wait for server connection
  responseTimeout = 15L * 1000L; // Max time to wait for data from server
  
unsigned long  currentTime = 0L;
Adafruit_CC3000_Client  client;        // For WiFi connections

// Similar to F(), but for PROGMEM string pointers rather than literals
#define F2(progmem_ptr) (const __FlashStringHelper *)progmem_ptr

unsigned long lastMoistTime=0; // storage for millis of the most recent moisture reading
unsigned long lastWaterTime=0; // storage for millis of the most recent watering reading
unsigned long lastTwitterTime=0; // storage for millis of the most recent Twitter message

int moistValues[MOIST_SAMPLES];
int lastMoistAvg=0; // storage for moisture value
int lastWaterVal=0; // storage for watering detection value
int waterTarget=0;
int RIEGO_URGENTE=30; // teorica: 100 mL/min -> 50ml
int RIEGO_NORMAL=10; // teorica: 100 mL/min -> 15ml
static int state = 0; // tracks which messages have been sent

// DhcpState ipState = DhcpStateNone; // a variable to store the DHCP state
boolean ipState = false;

// ISR(WDT_vect) { Sleepy::watchdogEvent(); } // Setup the watchdog

void setup()  { 
  // serial = getSerial(); // create or obtain a serial number from EEPROM memory
  // counter = getCounter(); // create or obtain a tweet count from EEPROM memory
  
  pinMode(STATUSLED, OUTPUT);
  pinMode(MOISTLED, OUTPUT);
  pinMode(COMMLED, OUTPUT);
  pinMode(MOTORAGUA, OUTPUT);
  pinMode(TARGETPIN, INPUT);
  
  // initialize moisture value array
  for(int i = 0; i < MOIST_SAMPLES; i++) { 
    moistValues[i] = 0; 
  }
  
  analogWrite(CLAVOROJO, 255);
  delay(500); //Sleepy::loseSomeTime(100); // estabilizar la carga
  lastWaterVal = analogRead(CLAVONEGRO); //take a moisture measurement to initialize watering value
  digitalWrite(CLAVOROJO, 0);

  Serial.begin(115200);   // set the data rate for the hardware serial port
  
  // start Ethernet
  // beginCC3000();
  digitalWrite(STATUSLED, HIGH); // turn on the moisture LED
  digitalWrite(COMMLED, HIGH); // turn on the moisture LED
  digitalWrite(MOISTLED, HIGH); // turn on the moisture LED
  delay(500);
  digitalWrite(STATUSLED, LOW); // turn on the moisture LED
  digitalWrite(COMMLED, LOW); // turn on the moisture LED
  digitalWrite(MOISTLED, LOW); // turn on the moisture LED
}


void loop()       // main loop of the program     
{
  waterTarget = analogRead(TARGETPIN);
  
  moistureCheck(); // check to see if moisture levels require Twittering out
  wateringCheck(); // check to see if a watering event has occurred to report it
  
  // buttonCheck(cc3000); // check to see if the debugging button is pressed
  analogWrite(COMMLED,0); // douse comm light if it was on
}





