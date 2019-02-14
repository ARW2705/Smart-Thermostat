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
  if (screen.isDisplayOn) printToLCD(LCD_BLANK_LINE, LCD_BLANK_LINE);
  screen.isDisplayOn = false;
  lcd.noBacklight();
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
    case 'm':
      loadMainPage();
      break;
    case 'z':
      loadZonePage(climate.selectedZoneIndex);
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
 */
void loadMainPage() {
  /*
   * 70F COOL ProgOFF
   * Currently  70.0F
   */
  char line1[LCD_CHAR_LIMIT] = "";
  char line2[LCD_CHAR_LIMIT] = "Currently  ";
  
  char setTemperature[3];
  char setMode[5];
  char progOnOff[8];
  char currentTemperature [5];

  sprintf(setTemperature, "%d", climate.setTemperature);
  strcpy(line1, setTemperature);
  strcat(line1, "F ");
  stringifyMode(climate.setMode, setMode);
  strcat(line1, setMode);
  strcat(line1, (climate.setMode == 'F' || climate.setMode == 'O') ? "  ": " ");
  stringifyProgOnOff(program.isActive, progOnOff);
  strcat(line1, progOnOff);
  
  sprintf(currentTemperature, ".1f", climate.zones[climate.selectedZoneIndex]);
  strcat(line2, currentTemperature);
  strcat(line2, "F");

  printToLCD(line1, line2);

  screen.currentPage = 'm';
}

/*
 * 
 */
void loadZonePage(int zoneIndex) {
  /*
   *      SYSTEM
   * 70.0F      50.0%
   */   
  char line1[LCD_CHAR_LIMIT] = "";
  char line2[LCD_CHAR_LIMIT] = "";

  int validityCode = isZoneValid(zoneIndex);
  if (validityCode != 1) {
    strcat(line1, "Invalid Index ");
    char index[2];
    sprintf(index, "%d", zoneIndex);
    strcat(line1, index);
    strcat(line1, " ");
    strcat(line2, "                ");
    printToLCD(line1, line2);
    return;
  }

  char zoneTemperature[5];
  char zoneHumidity[5];

  int nameLength = getStringLength(zoneIndex);
  int startSpaces = (LCD_CHAR_LIMIT - nameLength - 1) / 2;
  int endSpaces = LCD_CHAR_LIMIT - 1 - nameLength - startSpaces;
  for (int i=0; i < startSpaces; ++i) strcat(line1, " ");
  strcat(line1, climate.zones[zoneIndex]->locationName);
  for (int i=0; i < endSpaces; ++i) strcat(line1, " ");

  sprintf(line2, "%.1f", climate.zones[zoneIndex]->temperature);
  sprintf(line2, "%.1f", climate.zones[zoneIndex]->humidity);
  strcat(line2, zoneTemperature);
  strcat(line2, "F      ");
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
 */
void loadOnOffPage() {
  /*
   *   System sleep  
   *   BLUE to sleep
   */
  char line1[LCD_CHAR_LIMIT] = "  System Sleep  ";
  char line2[LCD_CHAR_LIMIT] = "";
  if (request.setSleep) {
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
 */
void loadErrorPage() {
  char line1[LCD_CHAR_LIMIT] = "";
  char line2[LCD_CHAR_LIMIT] = "";

  strcat(line1, error.errorName);
  strcat(line2, error.errorDetail);

  printToLCD(line1, line2);

  screen.currentPage = 'e';
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

