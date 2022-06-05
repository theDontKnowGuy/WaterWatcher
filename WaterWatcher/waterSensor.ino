void IRAM_ATTR waterSensorRead() {

  if (digitalRead(SENSOR)) {

    long diff = millis() - lastSensorRead;

    if (diff > tooLongInterval) {
      if (diff > restingInterval) waterStatus = RESTING;
      lastSensorRead = millis();
      return;
    }

    if (diff < tooShortInterval) return;

    if (waterStatus == RESTING || waterStatus == NOTINITIATED ) {
      waterStatus == INIT;
      InitTS = millis();
    }

    waterClicks[waterClicksInx++] = diff;

    lastSensorRead = millis();

    digitalWrite(LED_BUILTIN, HIGH);

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


      if ((waterStatus == INIT) && (millis() - InitTS < initPeriod)) waterStatus = DONE_INIT;

      if ((waterStatus == DONE_INIT) && (stddev < maxStdDev)) {

        if (avgWater > minNormalAvgWater) waterStatus = RUNNING_STABLE;
        
        if (avgWater < leakAvgWater) waterStatus = LEAK;
        else waterStatus = POTINTIAL_LEAK;

        if (avgWater > maxAcceptableStdDev ) waterStatus = RUNNING_UNSTABLE;
        }

      String waterLogging = "stats#" + String(minWater) + "#" + String(maxWater) + "#" + String(waterClicksInx) + "#" + String(avgWater)  + "#" + String(varWater) + "#" + String(stddev) + "#" + waterStatus;

      logThis(0, waterLogging, 2);
      waterClicksInx = 0;




    }
    return;
  }

  if ((millis() - lastSensorRead) > 100) digitalWrite(LED_BUILTIN, LOW);

  return;
}
