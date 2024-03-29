void beginCC3000() {
  uint32_t ip = 0L, t;
  
  // Serial.print(F("Hello! Initializing CC3000..."));
  if(!cc3000.begin()) hang(F("failed. Check your wiring?"));
  
  // Serial.print(F("OK\r\nDeleting old connection profiles..."));
  if(!cc3000.deleteProfiles()) hang(F("failed."));

  // Serial.print(F("OK\r\nConnecting to network..."));
  /* NOTE: Secure connections are not available in 'Tiny' mode! */
  if(!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) hang(F("Failed!"));

  // Serial.print(F("OK\r\nRequesting address from DHCP server..."));
  for(t=millis(); !cc3000.checkDHCP() && ((millis() - t) < dhcpTimeout); delay(100));
  if(!cc3000.checkDHCP()) hang(F("failed"));
  // Serial.println(F("OK"));

  // Get initial time from time server (make a few tries if needed)
  for(uint8_t i=0; (i<5) && !(currentTime = getTime()); suspenderSec(5), i++);

  // Initialize crypto library
  Sha1.initHmac_P((uint8_t *)signingKey, sizeof(signingKey) - 1);
}

// Returns true on success, false on error
boolean sendTweet(char *twit) {
  tUltimoTwitter = millis();
    
  // 140 chars max! No checks made here.
  // Serial.println("---- SEND TWEET! ----");
  digitalWrite(TwittLED,HIGH); // light the Communications LED

  uint8_t                  *in, out, i;
  char                      nonce[9],       // 8 random digits + NUL
                            searchTime[11], // 32-bit int + NUL
                            b64[29];
  unsigned long             startTime, t, ip;
  static const char PROGMEM b64chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  startTime = millis();
  
  if(!cc3000.checkConnected() ) beginCC3000();
  
  sprintf(nonce, "%04x%04x", random() ^ currentTime, startTime ^ currentTime);
  sprintf(searchTime, "%ld", currentTime);
  
  // assemble a string for Twitter, appending a unique ID to prevent Twitter's repeat detection
  char *str1 = " [media:";
  char *str2;
  str2= (char*) calloc (5,sizeof(char)); // allocate memory to string 2
  itoa(ultimaMedia,str2,10); // turn serial number into a string
  char *str3 = " - obj:";
  char *str4;
  str4= (char*) calloc (5,sizeof(char)); // allocate memory to string 4
  itoa(objetivoRegulado,str4,10); // turn message counter into a string
  char *str5 = " - dosis:";
  char *str6;
  str6= (char*) calloc (5,sizeof(char)); // allocate memory to string 4
  itoa(dosis,str6,10); // turn message counter into a string
  char *str7 = "] ";
  char *message;
  // allocate memory for the message
  message = (char*) calloc(strlen(twit) + strlen(str1) + strlen(str2) + strlen(str3) + strlen(str4) + strlen(str5) + strlen(str6) + strlen(str7) + strlen(searchTime) + 1, sizeof(char));
  strcat(message, twit); // assemble (concatenate) the strings into a message
  strcat(message, str1);
  strcat(message, str2);   
  strcat(message, str3);
  strcat(message, str4);
  strcat(message, str5);
  strcat(message, str6);
  strcat(message, str7);
  strcat(message, searchTime);
  
  Sha1.print(F("POST&http%3A%2F%2F"));
  Sha1.print(host);
  urlEncode(Sha1, endpoint, true, false);
  Sha1.print(F("&oauth_consumer_key%3D"));
  Sha1.print(F2(consumer_key));
  Sha1.print(F("%26oauth_nonce%3D"));
  Sha1.print(nonce);
  Sha1.print(F("%26oauth_signature_method%3DHMAC-SHA1%26oauth_timestamp%3D"));
  Sha1.print(searchTime);
  Sha1.print(F("%26oauth_token%3D"));
  Sha1.print(F2(access_token));
  Sha1.print(F("%26oauth_version%3D1.0%26status%3D"));
  urlEncode(Sha1, message, false, true);
  
  for(in = Sha1.resultHmac(), out=0; ; in += 3) { // octets to sextets
    b64[out++] =   in[0] >> 2;
    b64[out++] = ((in[0] & 0x03) << 4) | (in[1] >> 4);
    if(out >= 26) break;
    b64[out++] = ((in[1] & 0x0f) << 2) | (in[2] >> 6);
    b64[out++] =   in[2] & 0x3f;
  }
  b64[out] = (in[1] & 0x0f) << 2;
  // Remap sextets to base64 ASCII chars
  for(i=0; i<=out; i++) b64[i] = pgm_read_byte(&b64chars[b64[i]]);
  b64[i++] = '=';
  b64[i++] = 0;

  // Hostname lookup
  // Serial.print(F("Locating Twitter server..."));
  cc3000.getHostByName((char *)host, &ip);

  // Connect to numeric IP
  // Serial.print(F("OK\r\nConnecting to server..."));
  t = millis();
  do {
    client = cc3000.connectTCP(ip, 80);
  } while((!client.connected()) && ((millis() - t) < connectTimeout));

  if(client.connected()) { // Success!
    // Unlike the hash prep, parameters in the HTTP request don't require sorting.
    client.fastrprint(F("POST "));
    client.fastrprint(F2(endpoint));
    client.fastrprint(F(" HTTP/1.1\r\nHost: "));
    client.fastrprint(host);
    client.fastrprint(F("\r\nUser-Agent: "));
    client.fastrprint(F2(agent));
    client.fastrprint(F("\r\nConnection: close\r\n"
                       "Content-Type: application/x-www-form-urlencoded;charset=UTF-8\r\n"
                       "Content-Length: "));
    client.print(7 + encodedLength(message));
    client.fastrprint(F("\r\nAuthorization: Oauth oauth_consumer_key=\""));
    client.fastrprint(F2(consumer_key));
    client.fastrprint(F("\", oauth_nonce=\""));
    client.fastrprint(nonce);
    client.fastrprint(F("\", oauth_signature=\""));
    urlEncode(client, b64, false, false);
    client.fastrprint(F("\", oauth_signature_method=\"HMAC-SHA1\", oauth_timestamp=\""));
    client.fastrprint(searchTime);
    client.fastrprint(F("\", oauth_token=\""));
    client.fastrprint(F2(access_token));
    client.fastrprint(F("\", oauth_version=\"1.0\"\r\n\r\nstatus="));
    urlEncode(client, message, false, false);
    
    unsigned long start = millis();
    while((!client.available()) && ((millis() - start) < responseTimeout)); // Esperar que haya respuesta
    
    // Serial.println(message);
    // Serial.println( strlen(message) );
    
    // Dirty trick: instead of parsing results, just look for opening
    // curly brace indicating the start of a successful JSON response.
    /*
    int c = 0;
    while(((c = timedRead()) > 0) && (c != '{'));
    */
    
    /*
    if(c == '{')   Serial.println(message);
    else if(c < 0) Serial.println(F("timeout"));
    else           Serial.println(F("error (invalid Twitter credentials?)"));
    */
    // Serial.println(c);
     
    client.close();
    cc3000.disconnect();
    digitalWrite(TwittLED,LOW); // light the Communications LED
    return true;
  } else {
    // Serial.println(F("Couldn't contact server"));
    blinkLED(TwittLED,10,50);
    digitalWrite(TwittLED,LOW);
    return false;
  }
}

// Read from client stream with a 5 second timeout.  Although an
// essentially identical method already exists in the Stream() class,
// it's declared private there...so this is a local copy.
int timedRead(void) {
  unsigned long start = millis();
  while((!client.available()) && ((millis() - start) < responseTimeout));
  return client.read();  // -1 on timeout
}

// For URL-encoding functions below
static const char PROGMEM hexChar[] = "0123456789ABCDEF";

// Returns would-be length of encoded string, without actually encoding
int encodedLength(char *src) {
  uint8_t c;
  int     len = 0;

  while((c = *src++)) {
    len += (((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) || ((c >= '0') && (c <= '9')) || strchr_P(PSTR("-_.~"), c)) ? 1 : 3;
  }

  return len;
}


/*
// check and attempt to create a DHCP leased IP address
void dhcpChecker() {
  boolean prevState = ipState; // record the current state
  ipState = cc3000.checkDHCP(); // poll for an updated state
  if (prevState != ipState) { // if this is a new state then report it
    switch (ipState) {
    case false:
      Serial.println("DHCP not ready");
      break;
    case true: 
      {
        Serial.println("DHCP OK!");
        // We have a new DHCP lease, so print the info
        displayConnectionDetails();
        break;
      }
    }
  }
} 

void printResponse() {
  // Serial.print(F("OK\r\nAwaiting response..."));
  /*
  Serial.println(F("-------------------------------------"));
  unsigned long lastRead = millis();
  while (client.connected() && (millis() - lastRead < responseTimeout)) {
    while (client.available()) {
      char c = client.read();
      Serial.print(c);
      lastRead = millis();
    }
  Serial.println(F(""));
  }
  Serial.println(F("-------------------------------------"));
}
*/
