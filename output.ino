/*=====================================
      THERMOSTAT OUTPUT OPERATION     
=====================================*/

/*
 * Set thermostat outputs depending on current operating conditions
 * 
 * params: none
 * 
 * return: none
 */
void runThermostatCycle() {
  // check if cool down timer has expired
  if (climate.isActive 
      && !strcmp(climate.status, "RUN ON")
      && millis() - timer.coolDown > COOLDOWN_INTERVAL 
      && isZoneValid(climate.selectedZoneIndex)) {
    float currentTemperature = climate.zones[climate.selectedZoneIndex]->temperature;
    // cooling cycle
    if (climate.setMode == 'C') {
      if (currentTemperature > climate.setTemperature) {
        // start cooling cycle only if current temperature has exceeded the set threshold
        if (currentTemperature > climate.setTemperature + TEMPERATURE_THRESHOLD) {
          setCycle(true);
        }
      // if current temperature has reached the set temperature, end cooling cycle
      } else {
        setCycle(false);
        runOn(true);
      }
    }
    // heating cycle
    else if (climate.setMode == 'H') {
      if (currentTemperature < climate.setTemperature) {
        // start heating cycle only if current temperature has exceeded the set threshold
        if (currentTemperature < climate.setTemperature - TEMPERATURE_THRESHOLD) {
          setCycle(true);
        }
      // if current temperature has reached the set temperature, end heating cycle
      } else {
        setCycle(false);
        runOn(true);
      }
    }
    // ventilation cycle
    else if (climate.setMode == 'F') {
      setCycle(true);
    }
    // end all cycles
    else {
      setCycle(false);
    }
  } else if (!strcmp(climate.status, "RUN ON")) {
    runOn(false);
  } else {
    setAllCyclesOff();
  }
}

/*
 * After target temperature was reached, execute a fan run on period
 * 
 * params: bool
 * isStart - true if run on is to be started, false to check timer
 * 
 * return: none
 */
void runOn(bool isStart) {
  if (isStart && !strcmp(climate.status, "RUNNING")) {
    strcpy(climate.status, "RUN ON");
    timer.runOn = millis();
  } else if (millis() - timer.runOn > RUN_ON_INTERVAL) {
    digitalWrite(FAN_RELAY, HIGH);
    strcpy(climate.status, "OFF");
    timer.coolDown = millis();
  }
}

/*
 * Start or stop a thermostat cycle
 * 
 * params: bool
 * isStart - true to start a cycle, false to end a cycle
 * 
 * return: none
 */
void setCycle(bool isStart) {
  if (isStart) {
    // only start a cycle if the system status was previously not running
    if (!strcmp(climate.status, "OFF")) {
      digitalWrite(AC_RELAY, climate.setMode == 'C' ? LOW: HIGH);
      digitalWrite(HEAT_RELAY, climate.setMode == 'H' ? LOW: HIGH);
      digitalWrite(FAN_RELAY, LOW);
      strcpy(climate.status, "RUNNING");
    }
  } else {
    // always turn off the cooling or heating relay
    digitalWrite(AC_RELAY, HIGH);
    digitalWrite(HEAT_RELAY, HIGH);
  }
}

/*
 * Turn all output relays off without affecting cooldown timer or using run on
 * 
 * params: none
 * 
 * return: none
 */
void setAllCyclesOff() {
  digitalWrite(AC_RELAY, HIGH);
  digitalWrite(HEAT_RELAY, HIGH);
  digitalWrite(FAN_RELAY, HIGH);
  strcpy(climate.status, "OFF");
}

