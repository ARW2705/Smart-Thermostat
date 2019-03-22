/*========================
      ZONE FUNCTIONS     
========================*/

/*
 * Create a new zone and add to climate status
 * if a free zone is available
 * 
 * params: none
 * 
 * return: int
 * - index if successful, -1 if max zones reached
 */
int addZone(int deviceId) {
  for (int i=0; i < MAX_ZONES; ++i) {
    if (climate.zones[i] == NULL) {
      Zone* zone = new Zone();
      zone->deviceId = deviceId;
      zone->id = i;
      zone->temperature = INITIALIZE_T_H;
      zone->lastTemperature = INITIALIZE_T_H;
      zone->humidity = INITIALIZE_T_H;
      zone->lastHumidity = INITIALIZE_T_H;
      zone->lastUpdated = millis();
      zone->isRapid = false;
      strcpy(zone->locationName, "<INIT>");
      climate.zones[i] = zone;
      Serial.println("New zone added");
      return i;
    }
  }
  return -1;
}

/*
 * Remove a zone by id
 * 
 * params: int
 * id of zone to be deleted (id is also index of pointer)
 * 
 * return: int
 * - 1 if valid, 0 if id is valid, but zone invalid, -1 if id is invalid
 */
int removeZone(int id) {
  if (!isIdValid(id)) return -1;
  Serial.println("Remove zone");
  delete climate.zones[id];
  return climate.zones[id] == NULL ? 1: 0;
}

/*
 * Update a zone's name by id
 * <User input>
 * 
 * params: int, char*
 * deviceId - device id of zone to be updated
 * newName - new name to be set
 * 
 * return: none
 */
void setZoneName(int deviceId, const char* newName) {
  for (int i=0; i < MAX_ZONES; ++i) {
    if (climate.zones[i] == NULL) {
      continue;
    } else if (climate.zones[i]->deviceId == deviceId) {
      int len = 0;
      while (newName[len] != NULL) {
        ++len;
      }
      if (len <= LCD_CHAR_LIMIT) {
        Serial.println("Zone name updated");
        strcpy(climate.zones[i]->locationName, newName);
      }
    }
  }
}

/*
 * Update a zone's sensor values
 * <Sensor input>
 * 
 * params: int, float, float, bool
 * id - index id of zone to be updated
 * temperature - current temperature
 * humidity - current humidity
 * isSelected - true if zone should become selected
 * 
 * return: int
 * - 1 if valid, 0 if id is valid, but zone invalid, -1 if id is invalid
 */
int updateZone(int id, float temperature, float humidity, bool isSelected) {
  if (!isIdValid(id)) return -1;
  Zone* zone = climate.zones[id];
  if (zone != NULL && isSensorValid(temperature, humidity)) {
    zone->temperature = temperature;
    zone->humidity = humidity;
    zone->lastUpdated = millis();
    if (isSelected) {
      selectZone(id);
    }
    queryEmitStatus(id);
    return 1;    
  }
  return 0;
}

/*
 * Check if zone has valid data
 * 
 * params: int
 * id - id of zone to check
 * 
 * return: int
 * - 1 if valid, 0 if id is valid, but zone invalid, -1 if id is invalid
 */
int isZoneValid(int id) {
  if (!isIdValid(id)) return -1;
  Zone* zone = climate.zones[id];
  if (zone != NULL) {
    bool isPlausible = isSensorValid(zone->temperature, zone->humidity);
    bool isCurrent = isSensorCurrent(zone->lastUpdated, zone->isRapid);
    if (isPlausible && isCurrent) return 1;
    if (!isPlausible) {
      Serial.println("Sensor values not valid:");
      Serial.print("Temperature: ");
      Serial.println(zone->temperature);
      Serial.print("Humidity: ");
      Serial.println(zone->humidity);
    }
    if (!isCurrent) {
      Serial.print("Sensor value time limit exceeded: ");
      int overBy = millis() - zone->lastUpdated + (zone->isRapid ? MAX_SINCE_LAST_RAPID_UPDATE: MAX_SINCE_LAST_UPDATE);
      Serial.println(overBy / 1000);
    }
  }
  return 0;
}

/*
 * Check if zone id is valid
 * 
 * params: int
 * id - id to check
 * 
 * return: bool
 * - true if id is valid
 */
bool isIdValid(int id) {
  return id > -1 && id < MAX_ZONES && climate.zones[id] != NULL;
}

/*
 * Select a zone to use for input
 * 
 * params: int
 * id - id of desired zone
 * 
 * return: int
 * - 1 if new zone selected, 0 if zone is invalid, -1 if id is invalid
 */
int selectZone(int id) {
  int validityCode = isZoneValid(id);
  Serial.print("Select zone: ");
  Serial.println(id);
  Serial.print("Zone valid: ");
  Serial.println(validityCode == 1 ? "True": "False");
  if (validityCode == 1) {
    climate.zones[climate.setZone]->isRapid = false;
    climate.setZone = id;
    climate.isRapidSet = id ? false: true;
    climate.zones[climate.setZone]->isRapid = true;
  }
  return validityCode;
}

/*
 * Select default zone as a back up
 * 
 * params: none
 * 
 * return: none
 */
void runDefault() {
  int validityCode = selectZone(0);
  if (validityCode < 1) {
    setAllCyclesOff();
  }
}

/*
 * Get the number of initialized zones
 * 
 * params: none
 * 
 * return: int
 * - number of zones that are not NULL
 */
int countZones() {
  int total = 0;
  for (int i=0; i < MAX_ZONES; ++i) {
    if (climate.zones[i] != NULL) ++total;
  }
  return total;
}

/*
 * Check if the difference between current temperature and last emitted 
 * temperature are outside of the emit difference threshold. If so, 
 * update last temperature or humidity and trigger a climate status emit
 * 
 * params: int
 * id - zone index to check
 * 
 * return: none
 */
void queryEmitStatus(int id) {
  Zone* zone = climate.zones[id];
  if (abs(zone->temperature - zone->lastTemperature) > EMIT_DIFF_THRESHOLD
      || abs(zone->humidity - zone->lastHumidity) > EMIT_DIFF_THRESHOLD) {
    zone->lastTemperature = zone->temperature;
    zone->lastHumidity = zone->humidity;
    emitClimateStatus();
  }
}

/*
 * Get index of remote device by its ID
 * 
 * params: int
 * deviceId - zone's unique device id
 * 
 * return: int
 * - index of device if found, otherwise -1
 */
int getIndexByDevice(int deviceId) {
  for (int i=0; i < MAX_ZONES; ++i) {
    if (climate.zones[i] != NULL && climate.zones[i]->deviceId == deviceId) {
      return i;
    }
  }
  return -1;
}

