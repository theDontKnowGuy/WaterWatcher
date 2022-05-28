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
      String diffs = "WaterReads:";
      for (int i = 0; i < waterClicksInx; i++) {
        diffs = diffs + waterClicks[i] + "$";
      }
      logThis(0, diffs, 2);
      waterClicksInx = 0;
    }
    return;
  }

  if ((millis() - lastSensorRead) > 100) digitalWrite(LED_BUILTIN, LOW);

  return;
}
