//function for checking soil moisture against threshold
void medirHumedad() {
  int nivelMedio = tomarMediaHumedad();
  
  // Cambiar de estado
  if ( nivelMedio < SECO ) {
    estado = REVISAR_DEPOSITO;
  } else if  ( nivelMedio > (waterTarget + HYSTERESIS) ) {
    estado = HUMEDAD_ALTA;
  } else if  ( (nivelMedio < (waterTarget + HYSTERESIS)) && (nivelMedio > (waterTarget - HYSTERESIS)) ) {
    estado = HUMEDAD_OK;
  } else if ( nivelMedio < waterTarget ) {
    estado = RIEGAME;
    regar();
  }
  
  ultimaMedia = nivelMedio;
    
  /*
  char texto[50];
  sprintf(texto, "Nivel %d Last %d Target %d State %d", nivelMedio, ultimaMedia, waterTarget, state);
  Serial.println(texto);
  */
}

void regar() {
  activarRiego(T_RIEGO);
  Sleepy::loseSomeTime(T_CALAR_AGUA);
  int nivelMedio = tomarMediaHumedad();
  
  // Revisar el estado
  if ( nivelMedio < SECO ) {
    estado = REVISAR_DEPOSITO;
  } else if  (nivelMedio > waterTarget + HYSTERESIS) {
    estado = GRACIAS;
  } else if ( nivelMedio < waterTarget - HYSTERESIS ) {
    estado = RIEGAME;
    regar();
  }
}

int tomarMediaHumedad() {
  static int counter = 1;
  // Tomar un total de MOIST_SAMPLES de la humedad
  while(counter < MUESTRAS_HUMEDAD) {
    if(millis() > (tUltimaMedida + T_MUESTRAS)) {
      // Mover las medidas vaciando la ultima
      for(int i = MUESTRAS_HUMEDAD - 1; i > 0; i--) {
        moistValues[i] = moistValues[i-1]; 
      }
      
      // Tomar una nueva medida
      analogWrite(ClavoROJO, 255);
      delay(500); // estabilizar la carga
      moistValues[0] = analogRead(ClavoNEGRO);//take a measurement and put it in the first place
      analogWrite(ClavoROJO, 0);
      
      counter++;
      tUltimaMedida = millis();
    }
  }

  // Calcular la media
  int moistTotal = 0;
  for(int i = 0; i < MUESTRAS_HUMEDAD; i++) {
    moistTotal += moistValues[i];
  }
  int media = moistTotal/MUESTRAS_HUMEDAD;
  
  // Mostrar en el LED la medida tomada
  humedad2LED(media);
  
  return media;
}

void activarRiego(int time) {
  // Serial.print("riega ");Serial.println(time * 1000);
  digitalWrite(MotorLED, HIGH);
  digitalWrite(MotorAGUA, HIGH);
  Sleepy::loseSomeTime(time * 1000);
  digitalWrite(MotorAGUA, LOW);
  digitalWrite(MotorLED, LOW);
}