/*==============================
      REQUEST FUNCTIONS     
==============================*/

/*
 * User has remotely made changes to the operating values
 * 
 * params: JsonObject&
 * root - base of JSON object
 * 
 * return: none
 */
void updateClimateSettings(JsonObject& root) {
  // get values from root
  const char* setMode = root["mode"];
  int setZone = root["zone"];
  int setTemperature = root["targetTemperature"];
  bool sleep = root["sleep"];

  // set errors or default true
  bool zoneError = !isIdValid(setZone);
  bool temperatureError = setTemperature <= MAX_TEMP && setTemperature >= MIN_TEMP;
  bool modeError = charifyMode(setMode) == CHAR_SENTINEL;

  // check errors
  if (modeError) {
    char error[20] = "Invalid mode: '";
    error[15] = *setMode;
    error[16] = '\'';
    emitError("RequestError", error); 
  }
  if (zoneError) {
    char error[20] = "Invalid zone: ";
    char zoneNum[2];
    sprintf(zoneNum, "%d", setZone);
    strcat(error, zoneNum);
    emitError("RequestError", error);
  }
  if (temperatureError) {
    char error[23] = "Invalid temperature: ";
    char tempNum[3];
    sprintf(tempNum, "%d", setTemperature);
    strcat(error, tempNum);
    emitError("RequestError", error);
  }
  
  if (!modeError && !zoneError && !temperatureError) {
    request.temperature = setTemperature;
    request.mode = *setMode;
    request.zone = setZone;
    request.setSleep = sleep;
    request.isPending = true;
  }
}

/*
 * Assign program values from incoming JSON
 * 
 * params: JsonObject&
 * root - reference to parsed JSON object
 * 
 * return: none
 */
void updateProgram(JsonObject& root) {
  if (isProgramValid(root)) {
    // set name
    strcpy(program.name, root["name"]);
    
    // set mode
    const char* _mode = root["mode"];
    program.mode = charifyMode(_mode);
  
    // set active
    program.isActive = root["isActive"];
  
    // set program
    for (int i=0; i < PROGRAM_LENGTH; ++i) {
      program.schedule[i] = root["program"][i];
    }

    program.isLoaded = true;
    emitProgramUpdateConfirmation(true);
  }
}

/*
 * Set program to active or not active
 * 
 * params: bool
 * isActive - true if program should be active
 * 
 * return: none
 */
void toggleProgram(bool isActive) {
  program.isActive = isActive;
  emitProgramUpdateConfirmation(false);
}

/*
 * Apply pending user (local/remote) requests after debounce time
 * 
 * params: none
 * 
 * return: none
 */
void processPendingRequests() {
  climate.isActive = request.setSleep;
  program.isActive = false;
  if (request.temperature != SENTINEL_TEMPERATURE) {
    climate.setTemperature = request.temperature;
  }
  if (request.mode != SENTINEL_MODE) {
    climate.setMode = request.mode;
  }
  if (request.zone != SENTINEL_ZONE && isZoneValid(request.zone) == 1) {
    climate.selectedZoneIndex = request.zone;
  }
  resetRequest();
}

/*
 * Reset request struct to default
 * 
 * params: none
 * 
 * return: none
 */
void resetRequest() {
  request.isPending = false;
  request.temperature = SENTINEL_TEMPERATURE;
  request.mode = SENTINEL_MODE;
  request.zone = SENTINEL_ZONE;
  emitClimateStatus();
}

