/*==========================
      USER INTERFACE     
==========================*/

/*
 * Listen for analog inputs
 * 
 * params: none
 * 
 * return: none
 */
void listenForButton() {
  int input = analogRead(ANALOG_INPUT);
  // a button was pressed
  if (input > 50) {
    if (analog.analogIndex < SAMPLE_SIZE) {
      // add the input value to the sampling array
      analog.analogInputs[analog.analogIndex] = input;
      ++analog.analogIndex;
    } else {
      // button is being held, handle the input and reset sampling array
      handleAnalogInput(normalizeAnalogInputs());
      resetAnalogInput();
    }
  } else {
    if (analog.analogIndex) {
      // button was released, handle the input and reset sampling array
      handleAnalogInput(normalizeAnalogInputs());
    }
    resetAnalogInput();
  }
}

/*
 * Select the value with the most occurances
 * 
 * params: none
 * 
 * return: int
 * - return normalized input value
 */
int normalizeAnalogInputs() {
  // convert analog inputs to their closest midpoint values 
  int normal[8] = {200, 0, 400, 0, 600, 0, 800, 0};
  // a valid analog input should have at least 4 values above 50 to avoid errant analog signals
  for (int i=0; i < SAMPLE_SIZE; ++i) {
    int current = analog.analogInputs[i];
    for (int j=0; j < 4; ++j) {
      if (current > (normal[j * 2] - 100) && current < (normal[j * 2] + 100)) {
        ++normal[j * 2 + 1];
        break;
      }
    }
  }

  // return midpoint with the most occurrences
  int result = 0;
  int largest = 0;
  for (int k=0; k < 4; k++) {
    if (normal[k * 2 + 1] > largest) {
      result = normal[k * 2];
      largest = normal[k * 2 + 1];
    }
  }
  return result;
}

/*
 * Reset the analog sampling index and array
 * 
 * params: none
 * 
 * return: none
 */
void resetAnalogInput() {
  analog.analogIndex = 0;
  for (int i=0; i < SAMPLE_SIZE; i++) {
    analog.analogInputs[i] = 0;
  }
}

/*
 * Handle the normalized analog input
 * 
 * params: int
 * input - value of pressed analog button
 * 
 * return: none
 */
void handleAnalogInput(int input) {
  int tryCount = 0;
  int searchIndex = climate.setZone;
  switch(input) {
    case 200:
      // black button pressed 3.51kohm
      // toggle display on/off
      Serial.println("Black button");
      (screen.isDisplayOn) ? turnOffScreen(): turnOnScreen();
      break;
    case 400:
      // white button pressed 1.5kohm
      // change lcd page to next in sequence
      // home -> mode -> zone -> sleep -> error
      Serial.println("White button");
      switch(screen.currentPage) {
        case 'h':
          loadLCDPage('m');
          break;
        case 'm':
          loadLCDPage('z');
          break;
        case 'z':
          loadLCDPage('o');
          break;
        case 'o':
          loadLCDPage('e');
          break;
        case 'e':
          loadLCDPage('h');
          break;
        default:
          loadLCDPage('h');
          break;
      }
      break;
    case 600:
      // red button pressed 670ohm
      // increment values or toggle zone index
      Serial.println("Red button");
      // on home page - increment set temperature
      if (screen.currentPage == 'h' && climate.setTemperature < MAX_TEMP) {
        if (request.temperature == SENTINEL_TEMPERATURE) {
          request.temperature = climate.setTemperature + 1;
        } else if (request.temperature < MAX_TEMP) {
          ++request.temperature;
        }        
        request.isPending = true;
        loadLCDPage('h');
      } 
      // on mode page - cycle through mode options
      else if (screen.currentPage == 'm') {
        int index = 0;
        for (int i=0; i < 4; ++i) {
          if (climate.setMode == MODE_OPTIONS[i]) {
            index = i;
          }
        }
        index = index < 3 ? index + 1: 0;
        request.mode = MODE_OPTIONS[index];
        request.isPending = true;
        loadLCDPage('m');
      } 
      // on zone page - cycle through available zones
      else if (screen.currentPage == 'z') {
        do {
          if (searchIndex == MAX_ZONES - 1) {
            searchIndex = 0;
          } else {
            ++searchIndex;
          }
        } while (tryCount < MAX_ZONES && isZoneValid(searchIndex) != 1);
        screen.displayZone = searchIndex;
        loadZonePage();
      } else if (screen.currentPage == 'o') {
        request.setSleep = false;
        request.isPending = true;
        loadLCDPage('o');
      }
      break;
    case 800:
      // blue button pressed 270ohm
      // decrement values or toggle zone index
      Serial.println("Blue button");
      // on home page - decrement set temperature
      if (screen.currentPage == 'h' && climate.setTemperature > MIN_TEMP) {
        if (request.temperature == SENTINEL_TEMPERATURE) {
          request.temperature = climate.setTemperature - 1;
        } else if (request.temperature > MIN_TEMP) {
          --request.temperature;
        }
        request.isPending = true;
        loadLCDPage('h');
      } 
      // on mode page - cycle through mode options
      else if (screen.currentPage == 'm') {
        int index = 0;
        for (int i=0; i < 4; ++i) {
          if (climate.setMode == MODE_OPTIONS[i]) {
            index = i;
          }
        }
        index = index ? index - 1: 3;
        request.mode = MODE_OPTIONS[index];
        request.isPending = true;
        loadLCDPage('m');
      } 
      // on zone page - cycle through available zones
      else if (screen.currentPage == 'z') {
        do {
          if (searchIndex == 0) {
            searchIndex = MAX_ZONES - 1;
          } else {
            --searchIndex;
          }
          ++tryCount;
        } while (tryCount < MAX_ZONES && isZoneValid(searchIndex) != 1);
        screen.displayZone = searchIndex;
        loadZonePage();
      } else if (screen.currentPage == 'o') {
        request.setSleep = true;
        request.isPending = true;
        loadLCDPage('o');
      }
      break;
    default:
      break;
  }
  delay(10);
  timer.lcdTimeout = millis();
}

