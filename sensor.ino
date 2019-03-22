/*=======================
      SENSOR INPUTS     
=======================*/

/*
 * Read local temperature sensor and update status
 * 
 * params: none
 * 
 * reutrn: none
 */
void checkLocalSensor() {
  localSensor.requestTemperatures();
  delay(10);
  float tempC = localSensor.getTempC(LOCAL_SENSOR);
  float tempF = DallasTemperature::toFahrenheit(tempC);
  if (isSensorValid(tempF, 0)) {
    bool runDefault = climate.setZone == 0 && !climate.isRapidSet;
    updateZone(0, tempF, 0, runDefault);
    queryEmitStatus(0);
  } else {
    Serial.println("Invalid temperature sensor value");
  }
}

/*
 * Check if sensor values are plausible
 * 
 * params: float, float
 * temperature - temperature to check
 * humitiy - humidity percentage to check
 * 
 * return: bool
 * - true if values are valid and plausible
 */
bool isSensorValid(float temperature, float humidity) {
  return (!isnan(temperature) && !isnan(humidity)
          && SENSOR_MIN < temperature && temperature < SENSOR_MAX
          && -1 < humidity && humidity < 101);
}

/*
 * Check if sensor has been updated within the time limit
 * 
 * params: uint64_t, bool
 * lastUpdated - timestamp of last time sensor update its values
 * isRapid - determines which time limit to use
 * 
 * return: bool
 * - true if updated within time limit
 */
bool isSensorCurrent(uint64_t lastUpdated, bool isRapid) {
  int limit = isRapid ? MAX_SINCE_LAST_RAPID_UPDATE: MAX_SINCE_LAST_UPDATE;
  return (millis() - lastUpdated) < limit;
}

