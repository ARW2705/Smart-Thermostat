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
int addZone(bool isSelected, int deviceId) {
  for (int i=0; i < MAX_ZONES; ++i) {
    if (climate.zones[i] == NULL) {
      Zone* zone = new Zone();
      zone->deviceId = deviceId;
      zone->id = i;
      zone->temperature = INITIALIZE_T_H;
      zone->humidity = INITIALIZE_T_H;
      zone->lastUpdated = millis();
      zone->isRapid = false;
      strcpy(zone->locationName, "");
      climate.zones[i] = zone;
      if (isSelected) {
        selectZone(i);
      }
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
    if (climate.zones[i]->deviceId == deviceId) {
      int len = 0;
      while (newName[len] != NULL) {
        ++len;
      }
      if (len <= LCD_CHAR_LIMIT) {
        strcpy(climate.zones[i]->locationName, newName);
        emitClimateStatus();
      }
    }
  }
}

/*
 * Update a zone's sensor values
 * <Sensor input>
 * 
 * params: int, float, float
 * id - id of zone to be updated
 * temperature - current temperature
 * humidity - current humidity
 * 
 * return: int
 * - 1 if valid, 0 if id is valid, but zone invalid, -1 if id is invalid
 */
int updateZone(int id, float temperature, float humidity) {
  if (!isIdValid(id)) return -1;
  Zone* zone = climate.zones[id];
  if (zone != NULL && isSensorValid(temperature, humidity)) {
    zone->temperature = temperature;
    zone->humidity = humidity;
    zone->lastUpdated = millis();
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
    if (isSensorValid(zone->temperature, zone->humidity)
        && isSensorCurrent(zone->lastUpdated, zone->isRapid)) {
          return 1;
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
  if (validityCode == 1) {
    climate.zones[climate.selectedZoneIndex]->isRapid = false;
    climate.selectedZoneIndex = id;
    if (id) climate.isRapidSet = false;
    climate.zones[climate.selectedZoneIndex]->isRapid = true;
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

