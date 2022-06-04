void IRAM_ATTR waterSensorRead() {

  if (digitalRead(SENSOR)) {

    long diff = millis() - lastSensorRead;

    if (diff > tooLongInterval) {
      lastSensorRead = millis();;
      return;
    }

    if (diff < tooShortInterval) return;

    waterClicks[waterClicksInx++] = diff;

    lastSensorRead = millis();

    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println(diff);

    if (waterClicksInx == logChunkSize)
    {
      if (DEBUGLEVEL > 3) {
        String diffs = "WaterReads:";
        for (int i = 0; i < waterClicksInx; i++) {
          diffs = diffs + waterClicks[i] + "$";
        }
      }

      int minWater = 9999; int maxWater = 0; int avgWater = 0; int varWater = 0; int stddev = 0; int totalWater = 0; int varWaterAcc = 0;

      for (int i = 0; i < waterClicksInx; i++) {
        maxWater = (waterClicks[i] > maxWater) ? waterClicks[i] : maxWater;
        minWater = (waterClicks[i] < minWater) ? waterClicks[i] : minWater;
        totalWater = totalWater + waterClicks[i];
      }
      avgWater = totalWater / waterClicksInx;

      for (int i = 0; i < waterClicksInx; i++) {
        varWaterAcc = varWaterAcc + (waterClicks[i] - avgWater) * (waterClicks[i] - avgWater) ;
      }
      varWater = varWaterAcc / waterClicksInx;
      stddev = sqrt(varWater);

      String waterLogging = "min " + String(minWater) + " max " + String(maxWater) + " size " + String(waterClicksInx) + " avg " + String(avgWater)  + " var " + String(varWater) + " stddev " + String(stddev);
      Serial.println(waterLogging);

      logThis(0, waterLogging, 2);
      waterClicksInx = 0;
    }
    return;
  }

  if ((millis() - lastSensorRead) > 100) digitalWrite(LED_BUILTIN, LOW);

  return;
}
