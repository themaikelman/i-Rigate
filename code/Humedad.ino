
void medirHumedad() {
  int nivelMedio = tomarMediaHumedad();
  objetivoRegulado = analogRead(PotenciometroPIN);
  
  // Cambiar de estado
  if ( nivelMedio < SECO ) {
    estado = 1;
    blinkLED(MotorLED, 10, 50); // Para mostrar que no se riega
  } else if  ( nivelMedio > (objetivoRegulado + HYSTERESIS) ) {
    estado = 3;
    blinkLED(MotorLED, 10, 50); // Para mostrar que no se riega
  } else if  ( (nivelMedio < (objetivoRegulado + HYSTERESIS)) && (nivelMedio > (objetivoRegulado - HYSTERESIS)) ) {
    estado = 4;
    blinkLED(MotorLED, 10, 50); // Para mostrar que no se riega
  } else if ( nivelMedio < objetivoRegulado ) {
    estado = 2;
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
  dosis++;
  activarRiego(T_RIEGO);
  suspenderSec(T_CALAR_AGUA);
  int nivelMedio = tomarMediaHumedad();
  objetivoRegulado = analogRead(PotenciometroPIN);
  
  // Revisar el estado
  if ( nivelMedio < SECO ) {
    estado = 1;
    blinkLED(MotorLED, 10, 50); // Para mostrar que no se riega
  } else if  (nivelMedio > objetivoRegulado + HYSTERESIS) {
    estado = 5;
    blinkLED(MotorLED, 10, 50); // Para mostrar que no se riega
  } else if ( nivelMedio < objetivoRegulado - HYSTERESIS ) {
    estado = 2;
    regar();
  }
  
}

int tomarMediaHumedad() {
  int counter = 1;
  // Tomar un total de MUESTRAS_HUMEDAD de la humedad
  while(counter < MUESTRAS_HUMEDAD) {
    if(millis() > (tUltimaMedida + (T_MUESTRAS * 1000L))) {
      blinkLED(WaterLED, 6, 50);humedad2LED(ultimaMedia);
      
      // Mover las medidas vaciando la ultima
      for(int i = MUESTRAS_HUMEDAD - 1; i > 0; i--) {
        muestras[i] = muestras[i-1]; 
      }
      
      // Tomar una nueva medida
      analogWrite(ClavoROJO, 255);
      delay(500); // estabilizar la carga
      muestras[0] = analogRead(ClavoNEGRO);//take a measurement and put it in the first place
      analogWrite(ClavoROJO, 0);
      
      counter++;
      tUltimaMedida = millis();
    }
  }

  // Calcular la media
  int valorTotal = 0;
  for(int i = 0; i < MUESTRAS_HUMEDAD; i++) {
    valorTotal += muestras[i];
  }
  int media = valorTotal/MUESTRAS_HUMEDAD;
  
  // Mostrar en el LED la medida tomada
  humedad2LED(media);
  
  return media;
}

void activarRiego(int time) {
  // Serial.print("riega ");Serial.println(time * 1000);
  digitalWrite(MotorLED, HIGH);
  digitalWrite(MotorAGUA, HIGH);
  suspenderSec(time);
  digitalWrite(MotorAGUA, LOW);
  digitalWrite(MotorLED, LOW);
}
