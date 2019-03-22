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
      && strcmp(climate.status, "RUN ON")
      && millis() - timer.coolDown > COOLDOWN_INTERVAL 
      && isZoneValid(climate.setZone)) {
    Serial.println("\nThermostat cycle pre-conditions met\n");
    float currentTemperature = climate.zones[climate.setZone]->temperature;
    // cooling cycle
    if (climate.setMode == 'C') {
      if (currentTemperature > climate.setTemperature) {
        Serial.println("\n*** Cooling conditions met ***\n");
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
        Serial.println("\n*** Heating conditions met ***\n");
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
      Serial.println("\n*** Fan only ***\n");
      setCycle(true);
    }
    // end all cycles
    else {
      setCycle(false);
      if (!strcmp(climate.status, "RUNNING")) runOn(true);
    }
  } else if (!strcmp(climate.status, "RUN ON")) {
    runOn(false);
  } else {
    Serial.println("\n*** Thermostat cycle pre-conditions >>NOT<< met! ***\n");
    if (!climate.isActive) {
      Serial.println("Thermostat is sleeping");
    }
    if (!strcmp(climate.status, "RUN ON")) {
      Serial.println("Thermostat is in run on");
    }
    if (millis() - timer.coolDown < COOLDOWN_INTERVAL) {
      Serial.print("\nCooldown remaining: ");
      int remaining = (COOLDOWN_INTERVAL - (millis() - timer.coolDown)) / 1000;
      Serial.print(remaining);
      Serial.println("s\n");
    }
    if (isZoneValid(climate.setZone) != 1) {
      Serial.print("Zone ");
      Serial.print(climate.setZone);
      Serial.print(" is invalid :: ");
      Serial.print(isZoneValid(climate.setZone));
      Serial.println();
    }
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
    Serial.println("Cycle ended - begin run on");
    strcpy(climate.status, "RUN ON");
    timer.runOn = millis();
    emitClimateStatus();
  } else if (!strcmp(climate.status, "RUN ON") && millis() - timer.runOn > RUN_ON_INTERVAL) {
    Serial.println("Run on expired");
    digitalWrite(FAN_RELAY, HIGH);
    strcpy(climate.status, "OFF");
    timer.coolDown = millis();
    emitClimateStatus();
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
      Serial.print("Begin cycle: ");
      Serial.println(climate.setMode);
      digitalWrite(AC_RELAY, climate.setMode == 'C' ? LOW: HIGH);
      digitalWrite(HEAT_RELAY, climate.setMode == 'H' ? LOW: HIGH);
      digitalWrite(FAN_RELAY, LOW);
      strcpy(climate.status, "RUNNING");
      emitClimateStatus();
    }
  } else {
    // always turn off the cooling or heating relay
    if (digitalRead(AC_RELAY) == LOW) digitalWrite(AC_RELAY, HIGH);
    if (digitalRead(HEAT_RELAY) == LOW) digitalWrite(HEAT_RELAY, HIGH);
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
  if (digitalRead(AC_RELAY) == LOW) digitalWrite(AC_RELAY, HIGH);
  if (digitalRead(HEAT_RELAY) == LOW) digitalWrite(HEAT_RELAY, HIGH);
  if (digitalRead(FAN_RELAY) == LOW) digitalWrite(FAN_RELAY, HIGH);
  strcpy(climate.status, "OFF");
  delay(10);
}

