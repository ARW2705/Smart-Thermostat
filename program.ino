/*==========================
      PRE-PROGRAMMED     
==========================*/

/*
 * If program is set to active, use the appropriate stored values
 * 
 * params: none
 * 
 * return: none
 */
void usePreprogrammedValues() {
  if (climate.isTimeSet) {
    if (program.isLoaded && program.isActive) {
      int programIndex = getProgramIndex();
      climate.setMode = program.mode;
      climate.setTemperature = program.schedule[programIndex + 2];
      climate.selectedZoneIndex = program.schedule[programIndex + 3];
    } 
  } else {
    // TODO emit time error message
    emitError("TimeError", "Internal time not set");
    syncTime();
  }
}

/*
 * Get the current index by schedule
 * 
 * params: none
 * 
 * return: int
 * - timebreak index to be used, index + 2 is temperature, index + 3 is zone 
 */
int getProgramIndex() {
  float systemTime = getComparisonTime(getHour(), getMinute());
  int day = getDay();

  int scheduled;
  for (int i=0; i < 4; ++i) {
    float progTime = getComparisonTime(
      program.schedule[(day * 16) + (i * 4)], program.schedule[(day * 16) + (i * 4) + 1]
    );
    // have not reached first timebreak for today, use last timebreak of previous day
    if (!i && progTime > systemTime) {
      return ((!day ? 6: day - 1) * 16) + (3 * 4);
    // if system time is later than program time, use that index
    // continue loop to find latest program time that is still less
    // than system time
    } else if (progTime < systemTime) {
      scheduled = (day * 16) + (i * 4);
    }
  }
  return scheduled;
}

/*
 * Determine if new program values are plausible
 * 
 * params: JsonObject&
 * root - reference to parsed JSON object
 * 
 * return: bool
 * - true if all values are plausible, false if any fail
 */
bool isProgramValid(JsonObject& root) {
  bool nameError = true;
  bool modeError = true;
  bool scheduleError = true;

  // check name length
  const char* name = root["name"];
  int len = 0;
  while (name[len] != NULL) {
    ++len;
  }
  nameError = len > PROGRAM_CHAR_LIMIT;

  // check valid mode char
  const char* setMode = root["mode"];
  modeError = charifyMode(setMode) == CHAR_SENTINEL;

  // check each index is within a valid range
  int hour, minute, temperature, zone;
  for (int i=0; i < PROGRAM_LENGTH; i+=4) {
     hour = root["program"][i];
     minute = root["program"][i + 1];
     temperature = root["program"][i + 2];
     zone = root["program"][i + 3];

     if ((hour < 0 || hour > 23)
         || (minute < 0 || minute > 59)
         || (temperature < MIN_TEMP || temperature > MAX_TEMP)
         || (!isIdValid(zone))) {
       scheduleError = true;
       break;     
     } else {
       scheduleError = false;
     }
  }

  if (nameError) {
    emitError("ProgramError", "Invalid name update");  
  }
  if (modeError) {
    emitError("ProgramError", "Invalid mode update");
  }
  if (scheduleError) {
    emitError("ProgramError", "Invalid program schedule values found");
  }

  return nameError && modeError && scheduleError;
}

