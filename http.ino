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
  if (wifiMulti.run() == WL_CONNECTED || !credentials.isAuthed) {
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

