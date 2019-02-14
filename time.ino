/*============================
      DATETIME FUNCTIONS     
============================*/

/*
 * Get comparison time
 * 
 * params: none
 * 
 * return: float
 * - float with hours as whole numbers and minutes as decimal
 */
float getComparisonTime(int hours, int minutes) {
  return hours + (static_cast<float>(minutes) / 60);
}

/*
 * Get current weekday
 * sunday = 0
 * saturday = 6
 * 
 * params: none
 * 
 * return: int
 * - current weekday
 */
int getDay() {
  return weekday(now()) - 1;
}

/*
 * Get current hour
 * 
 * params: none
 * 
 * return: int
 * - current hour 0-23
 */
int getHour() {
  return hour(now());
}

/*
 * Get current minute
 * 
 * params: none
 * 
 * return: int
 * - current minute 0-59
 */
int getMinute() {
  return minute(now());
}

