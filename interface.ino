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
    if (analog.analogIndex >= MIN_SAMPLE_SIZE) {
      // button was released, handle the input if the minimum sample size is present
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
  switch(input) {
    case 200:
      // black button pressed 3.51kohm
      // toggle display on/off
      (screen.isDisplayOn) ? turnOffScreen(): turnOnScreen();
      break;
    case 400:
      // white button pressed 1.5kohm
      // change lcd page
      switch(screen.currentPage) {
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
          loadLCDPage('m');
          break;
        default:
          loadLCDPage('m');
          break;
      }
      break;
    case 600:
      // red button pressed 670ohm
      // increment values or toggle zone index
      if (screen.currentPage == 'm' && climate.setTemperature < MAX_TEMP) {
        request.temperature = climate.setTemperature + 1;
        request.isPending = true;
        timer.request = millis();
        loadLCDPage('m');
      } else if (screen.currentPage == 'z') {
        int searchIndex = climate.selectedZoneIndex;
        do {
          if (searchIndex == MAX_ZONES - 1) {
            searchIndex = 0;
          } else {
            ++searchIndex;
          }
        } while (!isZoneValid(searchIndex));
        loadZonePage(searchIndex);
      } else if (screen.currentPage == 'o') {
        request.setSleep = false;
        request.isPending = true;
        timer.request = millis();
        loadLCDPage('o');
      }
      break;
    case 800:
      // blue button pressed 270ohm
      // increment values or toggle zone index
      if (screen.currentPage == 'm' && climate.setTemperature > MIN_TEMP) {
        request.temperature = climate.setTemperature - 1;
        request.isPending = true;
        timer.request = millis();
        loadLCDPage('m');
      } else if (screen.currentPage == 'z') {
        int searchIndex = climate.selectedZoneIndex;
        do {
          if (searchIndex == 0) {
            searchIndex = MAX_ZONES - 1;
          } else {
            --searchIndex;
          }
        } while (!isZoneValid(searchIndex));
        loadZonePage(searchIndex);
      } else if (screen.currentPage == 'o') {
        request.setSleep = true;
        request.isPending = true;
        timer.request = millis();
        loadLCDPage('o');
      }
      break;
    default:
      break;
  }
  delay(10);
  timer.lcdTimeout = millis();
}

