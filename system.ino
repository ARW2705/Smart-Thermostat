/*============================
      SYSTEM FUNCTIONS     
============================*/

bool isClimateControlLoaded() {
  bool loaded;
  if (climate.isTimeSet) {
    loaded = false;
    syncTime();
  }
  if (!isZoneValid(0)) {
    loaded = false;
    if (climate.zones[0] != NULL) {
      removeZone(0);
    } 
    initDefaultZone();
  }
  if (climate.zones[0]->temperature == INITIALIZE_T_H) {
    loaded = false;
    checkLocalSensor();
  }
  climate.isLoaded = loaded;
  return loaded;
}

void initClimateControl() {
  syncTime();
  
  initDefaultZone();

  checkLocalSensor();
}

void initDefaultZone() {
  // add a default zone for the local sensor data
  int newZone = addZone(true, 0);
  if (newZone != -1) {
    char defaultName[7] = "SYSTEM";
    setZoneName(0, defaultName);
  }
}

