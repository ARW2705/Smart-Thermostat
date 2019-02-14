/*=======================================
      MESSAGE COMPOSITION FUNCTIONS     
=======================================*/

/*
 * Compose a Json-formatted string for login post request
 * 
 * params: char array
 * loginValues - pointer to outgoing message
 * 
 * return: none
 * 
 * result:
 * "{"username": "<USERNAME>", "password": "<PASSWORD>"}"
 */
void composeLoginMessage(char* loginValues) {
  strcpy(loginValues, "{\"username\":\"");
  strcat(loginValues, USERNAME);
  strcat(loginValues, "\", \"password\":\"");
  strcat(loginValues, PASSWORD);
  strcat(loginValues, "\"}");
  delay(10);
} // end composeLoginMessage

/*
 * Compose a Json-formatted string with all climate control values
 * 
 * params: char*
 * message - pointer to outgoing message
 * 
 * return: none
 * 
 * example:
 * 42["request-post-climate-data",{
 *   "setTemperature": 70,
 *   "status": "RUNNING",
 *   "setMode": "COOL",
 *   "setZone": 2,
 *   "units": "F",
 *   "zones": [
 *     {
 *       "locationName": "Living Room",
 *       "deviceId": 4,
 *       "zone": 2,
 *       "temperature": 72.1,
 *       "humidity": 15.5,
 *     },
 *     {
 *       "locationName": "Bedroom",
 *       "deviceId": 2,
 *       "zone": 1,
 *       "temperature": 70.4, 
 *       "humidity": 18.0
 *     }
 *   ]
 * }
 */
void composeClimateStatusMessage(char* message) {
  strcpy(message, "42[\"request-post-climate-data\",{");
  
  char _setTemperature[3];
  sprintf(_setTemperature, "%d", climate.setTemperature);
  strcat(message, "\"setTemperature\": ");
  strcat(message, _setTemperature);
  strcat(message, ",");

  strcat(message, "\"status\": \"");
  strcat(message, climate.status);
  strcat(message, "\",");

  char _mode[5];
  stringifyMode(climate.setMode, _mode);
  strcat(message, "\"setMode\": \"");
  strcat(message, _mode);
  strcat(message, "\",");

  char _zone[2];
  sprintf(_zone, "%d", climate.selectedZoneIndex);
  strcat(message, "\"setZone\": ");
  strcat(message, _zone);
  strcat(message, ",");

  strcat(message, "\"units\": \"");
  strcat(message, climate.isF ? "F": "C");
  strcat(message, "\",");

  strcat(message, "\"zones\": [");
  for (int i=0; i < MAX_ZONES; ++i) {
    if (climate.zones[i] != NULL) {
      strcat(message, "{\"locationName\": \"");
      strcat(message, climate.zones[i]->locationName);
      strcat(message, "\",");

      char _deviceId[4];
      sprintf(_deviceId, "%d", climate.zones[i]->deviceId);
      strcat(message, "\"deviceId\": ");
      strcat(message, _deviceId);
      strcat(message, ",");

      char _zone[2];
      sprintf(_zone, "%d", i);
      strcat(message, "\"zone\": ");
      strcat(message, _zone);
      strcat(message, ",");

      char _temperature[5];
      sprintf(_temperature, "%.1f", climate.zones[i]->temperature);
      strcat(message, "\"temperature\": ");
      strcat(message, _temperature);
      strcat(message, ",");

      char _humidity[5];
      sprintf(_humidity, "%.1f", climate.zones[i]->humidity);
      strcat(message, "\"humidity\": ");
      strcat(message, _humidity);
      strcat(message, "}");
    }
  }
  strcat(message, "]}]");
}

/*
 * Compose a Json-formatted string with program values
 * 
 * params: char*
 * message - pointer to outgoing message
 * 
 * return: none
 * 
 * example:
 * 42["response-update-program",{
 *   "isActive": false,
 *   "isLoaded": true,
 *   "name": "Hot Summer",
 *   "mode": "COOL",
 *   "id": "<24 character Object ID>",
 *   "program": [
 *     <112 data points>
 *   ]
 * }]
 */
void composeProgramDataMessage(char* message) {
  strcpy(message, "42[\"response-update-program\",{");
  strcat(message, "\"isActive\": ");
  strcat(message, program.isActive ? "true": "false");
  strcat(message, ", \"isLoaded\": ");
  strcat(message, program.isLoaded ? "true": "false");
  strcat(message, ", \"name\": \"");
  strcat(message, program.name);
  strcat(message, "\", \"mode\": \"");
  char strMode[5];
  stringifyMode(program.mode, strMode);
  strcat(message, strMode);
  strcat(message, "\", \"id\": \"");
  strcat(message, program.id);
  strcat(message, "\", \"program\": [");
  for (int i=0; i < PROGRAM_LENGTH - 1; ++i) {
    char strNum[4];
    sprintf(strNum, "%d,", program.schedule[i]);
    strcat(message, strNum);
  }
  char strNum[3];
  sprintf(strNum, "%d", program.schedule[PROGRAM_LENGTH - 1]);
  strcat(message, strNum);
  strcat(message, "]}]");
}

/*
 * Compose a Json-formatted string with error type and details message
 * 
 * params: const char*, const char*, char*
 * errorName - the predefined error type
 * errorMessage - details about the error
 * message - pointer to outgoing message
 * 
 * return: none
 * 
 * example:
 * 42["request-post-error",{
 *   "error": "RequestError",
 *   "message": "Invalid mode: 'E'"
 * }]
 */
void composeErrorMessage(const char* errorName, const char* errorMessage, char* message) {
  strcpy(message, "42[\"request-post-error\",{");
  strcat(message, "\"error\": \"");
  strcat(message, errorName);
  strcat(message, "\",\"message\": \"");
  strcat(message, errorMessage);
  strcat(message, "\"}]");
}

/*
 * Convert character to full word
 * 
 * params: char, char*
 * mode - input mode character to convert
 * formatted - pointer to char array to contain word
 * 
 * return: none
 */
void stringifyMode(char mode, char* formatted) {
  switch(mode) {
    case 'C':
      strcpy(formatted, "COOL");
      break;
    case 'H':
      strcpy(formatted, "HEAT");
      break;
    case 'F':
      strcpy(formatted, "FAN");
      break;
    case 'O':
      strcpy(formatted, "OFF");
      break;
    default:
      strcpy(formatted, "ERR");
      break;
  }
}

char charifyMode(const char* inputMode) {
  for (int i=0; i < 4; ++i) {
    if (*inputMode == MODE_OPTIONS[i]) return *inputMode;
  }
  return CHAR_SENTINEL;
}

/*
 * Convert program active bool value to string
 * 
 * params: char, char*
 * programOn - true if program is active, false if not
 * formatted - pointer to char array to contain word
 */
void stringifyProgOnOff(char programOn, char* formatted) {
  (programOn) ? strcpy(formatted, " progON"): strcpy(formatted, "progOFF");
}

/*
 * 
 */
int getStringLength(int zoneIndex) {
  int validityCode = isZoneValid(zoneIndex);
  if (validityCode == 1) {
    int strlength = 0;
    for (int i=0; i < LCD_CHAR_LIMIT; ++i) {
      if (climate.zones[zoneIndex]->locationName[i] != NULL) {
        ++strlength;
      } else {
        return strlength;
      }
    }
  }
  return validityCode;
}

