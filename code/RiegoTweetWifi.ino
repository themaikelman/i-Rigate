#include <JeeLib.h> // Low power functions library
#include <TrueRandom.h> // library for better randomization http://code.google.com/p/tinkerit/wiki/TrueRandom
#include <Adafruit_CC3000.h>
#include <SPI.h>
#include <sha1.h>

// DEFINICION DE PINES Y VALORES
#define TwittLED 2 // LED para mostrar la comunicacion
#define WaterLED 6  // LED para indicar la necesidad agua
#define MotorLED 8 // LED para indicar la activación del motor de agua
#define PotenciometroPIN 4 // Pin para indicar el nivel de agua deseado
#define MotorAGUA 9  // Pin para activar el motor de agua
#define ClavoROJO A3 // Borne rojo para medir la humedad (analog)
#define ClavoNEGRO A5 // Borne negativo para medir la humedad (analog)

#define MUESTRAS_HUMEDAD 10 // Numero de muestras a tomar para calcular la media

#define T_DESCANSO 60 // Minutos de descanso entre ciclos
#define T_MUESTRAS 20 // Segundos entre muestras
#define T_RIEGO 10 // Segundos de activacion de riego. Teoricamente 100 mL/min -> 1.6ml
#define T_CALAR_AGUA 60 // Segundos para permitir que cale el agua
#define T_TWITTER 2 // Minutos minimos entre envios a twitter
#define HYSTERESIS 20 // Valor de estabilizacion http://en.wikipedia.org/wiki/Hysteresis
#define SECO 100 // Minimo valor de humedad

#define REVISAR_DEPOSITO "Puede que el deposito no tenga agua" // Estado 1
#define RIEGAME "Dame de beber" // Estado 2
#define HUMEDAD_ALTA "Digamos que me sobra" // Estado 3
#define HUMEDAD_OK "Humedad ideal" // Estado 4
#define GRACIAS "Gracias por el trago" // Estado 5

// CC3000 interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3 // MUST be an interrupt pin!
#define ADAFRUIT_CC3000_VBAT  5 // These can be
#define ADAFRUIT_CC3000_CS   10 // any two pins
// Hardware SPI required for remaining pins.
// SCK = 13
// MISO = 12
// MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT, SPI_CLOCK_DIV2);

// WiFi access point credentials
#define WLAN_SSID       "SSID"        // cannot be longer than 32 characters!
#define WLAN_PASS       "PASSWORD"
#define WLAN_SECURITY   WLAN_SEC_WPA // This can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2

const char PROGMEM
  // Twitter application credentials -- see notes above -- DO NOT SHARE.
  consumer_key[]  = "CONSUMER KEY",
  access_token[]  = "ACCESS TOKEN",
  signingKey[]    = "CONSUMER SECRET"      // Consumer secret
    "&"             "ACCESS TOKEN SECRET", // Access token secret
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

unsigned long tUltimaMedida = 0; // Ultimo t en el que se tomo la medida (millis)
unsigned long tUltimoTwitter = 0; // Ultimo t en el que se envio un twitter (millis)

int muestras[MUESTRAS_HUMEDAD];
int ultimaMedia = 0; // storage for moisture value
int objetivoRegulado = 0;
static int dosis = 0;

ISR(WDT_vect) { Sleepy::watchdogEvent(); } // Setup the watchdog

void setup()  { 
  pinMode(MotorLED, OUTPUT);
  pinMode(WaterLED, OUTPUT);
  pinMode(TwittLED, OUTPUT);
  pinMode(MotorAGUA, OUTPUT);
  pinMode(PotenciometroPIN, INPUT);
  
  // initialize moisture value array
  for(int i = 0; i < MUESTRAS_HUMEDAD; i++) { 
    muestras[i] = 0; 
  }
  
  Serial.begin(115200);   // set the data rate for the hardware serial port

  digitalWrite(MotorLED, HIGH); // turn on the moisture LED
  digitalWrite(TwittLED, HIGH); // turn on the moisture LED
  digitalWrite(WaterLED, HIGH); // turn on the moisture LED

  delay(500);

  digitalWrite(MotorLED, LOW); // turn on the moisture LED
  digitalWrite(TwittLED, LOW); // turn on the moisture LED
  digitalWrite(WaterLED, LOW); // turn on the moisture LED
}

int estado = 0;

void loop()       // main loop of the program     
{
  dosis = 0;
  objetivoRegulado = analogRead(PotenciometroPIN);
  medirHumedad();
  
  if( (millis() > (tUltimoTwitter + (T_TWITTER * 60L * 1000L)) ) ) {
    if(estado == 1) sendTweet(REVISAR_DEPOSITO);
    else if(estado == 2) sendTweet(RIEGAME);
    else if(estado == 3) sendTweet(HUMEDAD_ALTA);
    else if(estado == 4) sendTweet(HUMEDAD_OK);
    else if(estado == 5) sendTweet(GRACIAS);
    else sendTweet("Algo ha ido mal...");
  }
  suspenderMin(T_DESCANSO);
}
