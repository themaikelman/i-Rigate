//function for checking soil moisture against threshold
void moistureCheck() {
  static int counter = 1;//init static counter
  int nivelMedio = 0; // init soil moisture average
  
  if((millis() - lastMoistTime) / 1000 > (MOIST_SAMPLE_INTERVAL / MOIST_SAMPLES)) {
  
    blinkLED(MOISTLED, 5, 100); // blink fast midiendo los niveles
    
    for(int i = MOIST_SAMPLES - 1; i > 0; i--) {
      moistValues[i] = moistValues[i-1]; //move the first measurement to be the second one, and so forth until we reach the end of the array.   
    }
    
    analogWrite(CLAVOROJO, 255);
    delay(500); //Sleepy::loseSomeTime(100); // estabilizar la carga
    moistValues[0] = analogRead(CLAVONEGRO);//take a measurement and put it in the first place
    analogWrite(CLAVOROJO, 0);
    
    lastMoistTime = millis();
    
    int moistTotal = 0;//create a little local int for an average of the moistValues array
    for(int i = 0; i < MOIST_SAMPLES; i++) {//average the measurements (but not the nulls)
      moistTotal += moistValues[i];//in order to make the average we need to add them first 
    }
    
    if(counter<MOIST_SAMPLES) {
      nivelMedio = moistTotal/counter;
      counter++; //this will add to the counter each time we've gone through the function
    } else {
      nivelMedio = moistTotal/MOIST_SAMPLES;//here we are taking the total of the current light readings and finding the average by dividing by the array size
    }
  
    ///return values
    if ((nivelMedio < SECO)  &&  (lastMoistAvg < SECO)  && state < URGENT_SENT && (millis() > (lastTwitterTime + TWITTER_INTERVAL)) ) {
      // Serial.println("URGENT tweet");
      sendTweet(URGENT_WATER); // announce to Twitter
      state = URGENT_SENT; // remember this message
      
    } else if  ((nivelMedio < waterTarget)  &&  (lastMoistAvg >= waterTarget)   && state < WATER_SENT &&  (millis() > (lastTwitterTime + TWITTER_INTERVAL)) ) {
      // Serial.println("WATER tweet");
      sendTweet(WATER); // announce to Twitter
      state = WATER_SENT; // remember this message
      
    } else if (nivelMedio > waterTarget + HYSTERESIS) {
      state = MOISTURE_OK; // reset to messages not yet sent state
    }
    
    lastMoistAvg = nivelMedio; // record this moisture average for comparison the next time this function is called
    moistLight(nivelMedio);
    
    /*
    char texto[50];
    sprintf(texto, "Nivel %d Last %d Target %d State %d", nivelMedio, lastMoistAvg, waterTarget, state);
    Serial.println(texto);
    */
    
    if(state == URGENT_SENT) {
        riegaMe(RIEGO_URGENTE);
    } else if(state == WATER_SENT) {
        riegaMe(RIEGO_NORMAL);
    } else if(state == MOISTURE_OK) {
      if (millis() > (lastTwitterTime + 60000) ) {
         sendTweet(NO_WATER); // announce to Twitter
      }
      //Sleepy::loseSomeTime(120000); // dos minutos de bypass
      delay(120000);
    }
  }
}


//function for checking for watering events
void wateringCheck() {
  int moistAverage = 0; // init soil moisture average
  if((millis() - lastWaterTime) / 1000 > WATERED_INTERVAL) {
    
    blinkLED(STATUSLED, 5, 100); // blink fast midiendo los niveles
  
    analogWrite(CLAVOROJO, 255);
    delay(500); //Sleepy::loseSomeTime(100); // estabilizar la carga
    int waterVal = analogRead(CLAVONEGRO);//take a moisture measurement
    analogWrite(CLAVOROJO, 0);
    
    lastWaterTime = millis();

    if (waterVal >= lastWaterVal + WATERING_CRITERIA) { // if we've detected a watering event
      char* tweet = "";
      
      if (waterVal >= waterTarget  &&  lastWaterVal < waterTarget && (millis() > (lastTwitterTime + TWITTER_INTERVAL)) ) {
        // Serial.println("TY tweet");
        tweet = THANK_YOU;
        sendTweet(tweet);   // announce to Twitter

      } else if  (waterVal >= waterTarget  &&  lastWaterVal >= waterTarget && (millis() > (lastTwitterTime + TWITTER_INTERVAL)) ) {
        // Serial.println("OW tweet");
        tweet = OVER_WATERED;
        sendTweet(tweet);   // announce to Twitter

      } else if  (waterVal < waterTarget  &&  lastWaterVal < waterTarget && (millis() > (lastTwitterTime + TWITTER_INTERVAL)) ) {
        // Serial.println("UW tweet");
        tweet = UNDER_WATERED;
        sendTweet(tweet);   // announce to Twitter
      }
    }
    
    lastWaterVal = waterVal; // record the watering reading for comparison next time this function is called
  }
}

void riegaMe(int time) {
  // Serial.print("riega ");Serial.println(time * 1000);
  digitalWrite(MOISTLED, HIGH);
  digitalWrite(MOTORAGUA, HIGH);
  delay(time * 1000); //Sleepy::loseSomeTime(time * 1000);
  digitalWrite(MOTORAGUA, LOW);
  digitalWrite(MOISTLED, LOW);
  delay(time * 1000); //Sleepy::loseSomeTime(time * 1000);
}

