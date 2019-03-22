/*=========================
      LOCAL DISPLAY     
=========================*/

/*
 * Clear the LCD and turn off backlight
 * 
 * params: none
 * 
 * return: none
 */
void turnOffScreen() {
  if (screen.isDisplayOn) {
    loadLCDPage('h');
    printToLCD(LCD_BLANK_LINE, LCD_BLANK_LINE);
    screen.isDisplayOn = false;
    lcd.noBacklight();
    Serial.println("Screen off");
  }
}

/*
 * Turn LCD on, restart timeout timer, set refresh timer to trigger immediately
 */
void turnOnScreen() {
  timer.lcdRefresh = millis();
  if (!screen.isDisplayOn) {
    screen.isDisplayOn = true;
    lcd.clear();
    lcd.noCursor();
    lcd.backlight();
    timer.lcdRefresh -= LCD_CYCLE_INTERVAL;
    timer.lcdTimeout = millis();
    Serial.println("Screen on");
  }
}

/*
 * LCD page router
 * 
 * params: char
 * page - desired page
 * 
 * return: none
 */
void loadLCDPage(char page) {
  turnOnScreen();
  switch(page) {
    case 'h':
      loadMainPage();
      break;
    case 'm':
      loadModePage();
      break;
    case 'z':
      loadZonePage();
      break;
    case 'o':
      loadOnOffPage();
      break;
    case 'e':
      loadErrorPage();
      break;
    // TODO implement program page
    // TODO implement settings page
    default:
      loadMainPage();
      break;
  }
}

/*
 * Load the main LCD page
 * 
 * params: none
 * 
 * return: none
 * 
 * example:
 * 
 *  |70F COOL ProgOFF|
 *  |Currently  70.0F|
 */
void loadMainPage() {
  char line1[LCD_CHAR_LIMIT] = "";
  char line2[LCD_CHAR_LIMIT] = "Currently  ";
  
  char setTemperature[3];
  char setMode[5];
  char progOnOff[8];
  char currentTemperature [5];

  sprintf(setTemperature, "%d", request.temperature == SENTINEL_TEMPERATURE ? climate.setTemperature: request.temperature);
  strcpy(line1, setTemperature);
  strcat(line1, "F ");
  stringifyMode(request.mode == SENTINEL_MODE ? climate.setMode: request.mode, setMode);
  strcat(line1, setMode);
  strcat(line1, (setMode[0] == 'F' || setMode[0] == 'O') ? "  ": " ");
  stringifyProgOnOff(program.isActive, progOnOff);
  strcat(line1, progOnOff);
  
  sprintf(currentTemperature, "%.1f", climate.zones[climate.setZone]->temperature);
  strcat(line2, currentTemperature);
  strcat(line2, "F");

  printToLCD(line1, line2);

  screen.currentPage = 'h';
}

/*
 * Load mode display and selection page
 * 
 * params: none
 * 
 * return: none
 * 
 * example:
 * 
 *   | Set Mode: COOL | 
 *   | Change to HEAT |
 */
void loadModePage() {
  char line1[LCD_CHAR_LIMIT] = " Set Mode: ";
  char line2[LCD_CHAR_LIMIT] = " Change to ";

  char _mode[5];
  stringifyMode(climate.setMode, _mode);
  strcat(line1, _mode);
  strcat(line1, climate.setMode == 'F' || climate.setMode == 'O' ? "  ": " ");

  char _newMode[5];
  char& useMode = request.mode == SENTINEL_MODE ? climate.setMode: request.mode;
  stringifyMode(useMode, _newMode);
  strcat(line2, _newMode);
  strcat(line2, useMode == 'F' || useMode == 'O' ? "  ": " ");

  printToLCD(line1, line2);

  screen.currentPage = 'm';
}

/*
 * Load zone info page by index
 * 
 * params: none
 * 
 * return: none
 * 
 * example:
 * 
 *   |     SYSTEM     |
 *   |70.0F      50.0%|
 */
void loadZonePage() {
  char line1[LCD_CHAR_LIMIT] = "";
  char line2[LCD_CHAR_LIMIT] = "";

  int validityCode = isZoneValid(screen.displayZone);
  if (validityCode != 1) {
    strcat(line1, "Invalid Index ");
    char index[2];
    sprintf(index, "%d", screen.displayZone);
    strcat(line1, index);
    strcat(line1, " ");
    strcat(line2, "                ");
    printToLCD(line1, line2);
    return;
  }

  char zoneTemperature[5];
  char zoneHumidity[5];

  int nameLength = getStringLength(screen.displayZone);
  int startSpaces = (LCD_CHAR_LIMIT - nameLength - 1) / 2;
  int endSpaces = LCD_CHAR_LIMIT - 1 - nameLength - startSpaces;
  for (int i=0; i < startSpaces; ++i) strcat(line1, " ");
  strcat(line1, climate.zones[screen.displayZone]->locationName);
  for (int i=0; i < endSpaces; ++i) strcat(line1, " ");

  Serial.println(screen.displayZone);
  Serial.println(climate.zones[screen.displayZone]->temperature);

  sprintf(zoneTemperature, "%.1f", climate.zones[screen.displayZone]->temperature);
  sprintf(zoneHumidity, "%.1f", climate.zones[screen.displayZone]->humidity);
  strcat(line2, zoneTemperature);
  strcat(line2, "F      ");
  if (climate.zones[screen.displayZone]->humidity < 10) strcat(line2, " ");
  strcat(line2, zoneHumidity);
  strcat(line2, "%");

  printToLCD(line1, line2);

  screen.currentPage = 'z';
}

/*
 * Toggle system sleep
 * 
 * params: none
 * 
 * return: none
 * 
 * example:
 * 
 *   |  System sleep  |
 *   |  BLUE to sleep |
 */
void loadOnOffPage() {
  char line1[LCD_CHAR_LIMIT] = "  System Sleep  ";
  char line2[LCD_CHAR_LIMIT] = "";
  if (climate.isActive && request.setSleep) {
    strcat(line2, "Starting sleep..");
  } else if (climate.isActive) {
    strcat(line2, "  BLUE to sleep ");
  } else {
    strcat(line2, "RED to end sleep"); 
  }
  
  printToLCD(line1, line2);

  screen.currentPage = 'o';
}

/*
 * Display an error
 * 
 * params: const char*, const char*
 * errorName - type of error
 * errorDetail - a message about the error
 * 
 * return: none
 * 
 * example:
 * 
 *   |ProgramError    |
 *   |Internal time...|
 */
void loadErrorPage() {
  char line1[LCD_CHAR_LIMIT] = "";
  char line2[LCD_CHAR_LIMIT] = "";

  // TODO implement scrolling detail line for long messages
  if (error.hasError) {
    strcat(line1, error.errorName);
    strcat(line2, error.errorDetail);
  } else {
    strcat(line1, "   No  errors   ");
    strcat(line2, LCD_BLANK_LINE);
  }

  printToLCD(line1, line2);

  screen.currentPage = 'e';
}

/*
 * Print a notification message on connection
 * 
 * params: char
 * connection - the type of connection made: 'w' = wifi, 'l' = loggedin, 's' = websocket
 * 
 * return: none
 * 
 * example: 
 * 
 *   |Login Successful|
 *   |                |
 */
void printConnectNotification(char connection) {
  char line1[LCD_CHAR_LIMIT] = "";
  char line2[LCD_CHAR_LIMIT] = "";
  if (connection == 'w') {
    strcat(line1, " WiFi Connected ");
    strcat(line2, LCD_BLANK_LINE);
  } else if (connection = 'l') {
    strcat(line1, "Login Successful");
    strcat(line2, LCD_BLANK_LINE);
  } else if (connection = 's') {
    strcat(line1, " Communication  ");
    strcat(line2, "  Established   ");
  } else {
    return;
  }
  timer.lcdRefresh = millis();
  printToLCD(line1, line2);
}

/*
 * Print strings to LCD
 * 
 * params: const char*, const char*
 * line1 - top line to print
 * line2 - bottom line to print
 * 
 * return: none
 */
void printToLCD(const char* line1, const char* line2) {
  delay(100);

  lcd.setCursor(0, 0);
  lcd.print(line1);

  lcd.setCursor(0, 1);
  lcd.print(line2);
}

