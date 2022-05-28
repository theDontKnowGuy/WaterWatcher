int waterSensorRead(){
  
    int sensorRead = digitalRead(SENSOR);
    if (sensorRead == 1) {
      lastSensorRead = millis();
      long diff = lastSensorRead - previousSensorRead;
      waterClicks[waterClicksInx++] = diff;
      if (waterClicksInx ==500 - 1) waterClicksInx == 0;
      logThis(2,String(diff));
      previousSensorRead = lastSensorRead;
    }
    LiveSignalPreviousMillis = millis();
    
    
  
  
  return 0;
}
