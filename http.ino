/*==========================
      HTTP FUNCTIONS     
==========================*/

/*
 * log into local server
 * 
 * params: none
 * 
 * return: none
 */
void login() {
  if (wifiMulti.run() == WL_CONNECTED) {
    // attempt to log in
    while (!credentials.isAuthed) {
      // compose login json string
      char loginValues[72];
      composeLoginMessage(loginValues);

      // send post request with formatted string
      http.begin(SERVER_ADDRESS, HTTP_PORT, "/users/login");
      http.addHeader("content-type", "application/json");
      int statusCode = http.POST(loginValues);
      Serial.print("HTTP response status: ");
      Serial.println(statusCode);
      String response = http.getString();
      int resLength = response.length() + 1;
      http.end();

      // on success
      if (statusCode == 200) {
        // store JWT and toggle authed
        char res[resLength];
        response.toCharArray(res, resLength);
        StaticJsonBuffer<200> jsonBuffer;
        JsonObject& root = jsonBuffer.parseObject(res);
        if (!root.success()) {
          Serial.println("parseObject failed, retrying\n");
          delay(5000);
        } else {
          strcpy(credentials.token, root["token"]);
          credentials.isAuthed = 1;
          Serial.print("Stored token");
        }
        Serial.print("Authed: ");
        Serial.println(credentials.isAuthed);
      } else {
        // wait to retry login
        Serial.println("Log in failed, retrying\n");
        delay(5000);
      }
    }
    Serial.println("Login successful\n");
  }
} // end login

/*
 * get unix time from server and set internal time
 * 
 * params: none
 * 
 * return: none
 * 
 */
void syncTime() {
  if (wifiMulti.run() == WL_CONNECTED && credentials.isAuthed) {
    Serial.println("Get DateTime from server...\n");
    char headerToken[207];
    strcpy(headerToken, "bearer ");
    strcat(headerToken, credentials.token);

    http.begin(SERVER_ADDRESS, HTTP_PORT, "/datetime");
    http.addHeader("content-type", "application/json");
    http.addHeader("authorization", headerToken);
    int statusCode = http.GET();
    Serial.print("HTTP response status: ");
    Serial.println(statusCode);
    String response = http.getString();
    int resLength = response.length() + 1;
    http.end();

    if (statusCode == 200) {
      char res[resLength];
      response.toCharArray(res, resLength);
      StaticJsonBuffer<200> jsonBuffer;
      JsonObject& root = jsonBuffer.parseObject(res);
      if (!root.success()) {
        Serial.println("parseObject failed");
        delay(1000);
      } else {
        time_t unix = root["unix"];
        setTime(unix);
        if (timeStatus() == timeSet) {
          Serial.print("Set time ");
          climate.isTimeSet = true;
        } else {
          Serial.println("Time not set ");
        }
        Serial.println(now());
      }
    }
  }
} // end syncTime

/*
 * Handle remote sensor post requests
 * 
 * params: none
 * 
 * return: none
 */
void handleSensorPostRequest() {
  int id, deviceId;
  float _t, _h;
  char rapid[20];
  String data = server.arg("plain");
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(data);
  if (!root.success()) {
    Serial.println("DHT HTTP request parsing failed");
    server.send(400);
    return;
  } else {
    id = root["id"];
    deviceId = root["deviceId"];
    _t = root["temperature"];
    _h = root["humidity"];
    strcpy(rapid, root["rapidTransmission"]);
  }
  Serial.println("Remote DHT Sensor Update:");
  Serial.println("=========================");
  Serial.print("Data from zone ");
  Serial.println(id);
  Serial.print("Temperature: ");
  Serial.println(_t);
  Serial.print("Humidity: ");
  Serial.println(_h);
  Serial.print("Rapid Transmission: ");
  Serial.println(rapid);

  String activeZone = (climate.selectedZoneIndex == id) ? "active": "inactive";
  Serial.print("Set rapid transmission to ");
  if (activeZone == "active") Serial.println(activeZone);
  else Serial.println("inactive");
  Serial.println("=========================\n");

  // if id is default - add zone to system
  if (id == -1) {
    id = addZone(false, deviceId);
  }

  int updated = updateZone(id, _t, _h);
  if (updated == 1) {
    server.send(200, "text/plain", activeZone);
  } else {
    server.send(200, "text/plain", "readfail");
  }
} // end handleSensorPostRequest

/*
 * Handle remote sensor post request with rapid transmission status
 * 
 * params: none
 * 
 * return: none
 */
void confirmRapidPostRequest() {
  String data = server.arg("plain");
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(data);
  if (!root.success()) {
    Serial.println("DHT HTTP request parsing failed");
    server.send(400);
    return;
  } else {
    if (root["id"] == climate.selectedZoneIndex) {
      climate.isRapidSet = true;
    }
  }
}

