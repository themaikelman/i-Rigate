void suspenderMin(int t) {
  for (int i = 0; i < t; ++i) {
    Sleepy::loseSomeTime(60L * 1000L);
  }
}

void suspenderSec(int t) {
  for (int i = 0; i < t; ++i) {
    Sleepy::loseSomeTime(1000L);
  }
}

// setting the moisture LED
void humedad2LED (int humedad) {
  if (humedad < SECO) {
    blinkLED(WaterLED, 10, 50); // blink fast when soil is very dry
    analogWrite(WaterLED, 8);
  } else if (humedad < objetivoRegulado) {
    blinkLED(WaterLED, 5, 100); // blink slowly when watering is needed
    analogWrite(WaterLED, 24);
  } else {
    analogWrite(WaterLED,humedad/4); // otherwise display a steady LED with brightness mapped to moisture
  }
}

// Gestion del error de comunicaciones
void hang(const __FlashStringHelper *str) {
  // Serial.print(str);
  blinkLED(TwittLED,100,10);
  cc3000.reboot();
}

// Funcion de parpadeo
void blinkLED(byte targetPin, int numBlinks, int blinkRate) {
  for (int i=0; i<numBlinks; i++) {
    digitalWrite(targetPin, HIGH);
    delay(blinkRate); // milliseconds
    digitalWrite(targetPin, LOW);
    delay(blinkRate);
  }
}

// URL-encoding
// Se codifica doblemente en el caso del Oauth
void urlEncode(
  Print      &p,       // EthernetClient, Sha1, etc.
  const char *src,     // String to be encoded
  boolean     progmem, // If true, string is in PROGMEM (else RAM)
  boolean     x2)      // If true, "double encode" parenthesis
{
  uint8_t c;

  while((c = (progmem ? pgm_read_byte(src) : *src))) {
    if(((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) ||
       ((c >= '0') && (c <= '9')) || strchr_P(PSTR("-_.~"), c)) {
      p.write(c);
    } else {
      if(x2) p.print("%25");
      else   p.write('%');
      p.write(pgm_read_byte(&hexChar[c >> 4]));
      p.write(pgm_read_byte(&hexChar[c & 15]));
    }
    src++;
  }
}


// Minimalist time server query; adapted from Adafruit Gutenbird sketch,
// which in turn has roots in Arduino UdpNTPClient tutorial.
unsigned long getTime(void) {
  uint8_t       buf[48];
  unsigned long ip, startTime, t = 0L;

  // Serial.print(F("Locating time server..."));

  // Hostname to IP lookup; use NTP pool (rotates through servers)
  if(cc3000.getHostByName("pool.ntp.org", &ip)) {
    static const char PROGMEM
      timeReqA[] = { 227,  0,  6, 236 },
      timeReqB[] = {  49, 78, 49,  52 };

    // Serial.println(F("found\r\nConnecting to time server..."));
    startTime = millis();
    do {
      client = cc3000.connectUDP(ip, 123);
    } while((!client.connected()) && ((millis() - startTime) < connectTimeout));

    if(client.connected()) {
      // Serial.print(F("connected!\r\nIssuing request..."));
      memset(buf, 0, sizeof(buf));
      memcpy_P( buf    , timeReqA, sizeof(timeReqA));
      memcpy_P(&buf[12], timeReqB, sizeof(timeReqB));
      client.write(buf, sizeof(buf));

      // Serial.print(F("OK\r\nAwaiting response..."));
      memset(buf, 0, sizeof(buf));
      startTime = millis();
      
      while((!client.available()) && ((millis() - startTime) < responseTimeout));
      
      if(client.available()) {
        client.read(buf, sizeof(buf));
        t = (((unsigned long)buf[40] << 24) |
             ((unsigned long)buf[41] << 16) |
             ((unsigned long)buf[42] <<  8) |
              (unsigned long)buf[43]) - 2208988800UL;
        // Serial.println(F("success!"));
      }
      client.close();
    // } else {
      // Serial.print(F("not connected"));
    }
  }

  if(!t) Serial.println(F("error"));

  return t;
}
