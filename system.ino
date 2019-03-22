/*============================
      SYSTEM FUNCTIONS     
============================*/

/*
 * Connect client to WiFi
 * 
 * params: none
 * 
 * return: none
 */
void connectWifi() {
  Serial.print("Connecting to ");
  Serial.print(ROUTER_SSID);
  Serial.println(" ...\n");
  int i = 0;
  while (wifiMulti.run() != WL_CONNECTED) {
    if (i > MAX_RETRY) {
      Serial.println("Cannot connect to wifi\n");
      connectUtil.isWifiConnected = false;
      return;
    }
    delay(1000);
    Serial.print(++i);
    Serial.print(' ');
  }
  connectUtil.isWifiConnected = true;
  printConnectNotification('w');
  Serial.print("\n\n");
  Serial.println("Connection established!");
  Serial.print("Connected to: ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  delay(10);
}

/*
 * Check if initial climate settings have been initialized
 * 
 * params: none
 * 
 * return: bool
 * - true if time has been set and a valid local sensor value has been recorded
 */
bool isClimateControlLoaded() {
  bool loaded = true;
  if (climate.zones[0]->temperature == INITIALIZE_T_H) {
    Serial.println("Local sensor not valid");
    checkLocalSensor();
    loaded = false;
  }
  climate.isLoaded = loaded;
  if (loaded) Serial.println("Climate control is loaded");
  return loaded;
}

/*
 * Call functions needed to initialize climate control
 * 
 * params: none
 * 
 * return: none
 */
void initClimateControl() {
  initDefaultZone();

  checkLocalSensor();
}

/*
 * Create the default system zone for the local sensor
 * 
 * params: none
 * 
 * return: none
 */
void initDefaultZone() {
  // add a default zone for the local sensor data
  int newZone = addZone(0);
  if (newZone != -1) {
    char defaultName[7] = "SYSTEM";
    setZoneName(0, defaultName);
  }
}

/*
 * Serial print multiple climate status values
 * 
 * params: none
 * 
 * return: none
 */
void printStatus() {
  Serial.print("\n\n====== System Status ======\n\n");
  Serial.println("Current Conditions:");
  
  Serial.print("Temperature: ");
  Serial.println(climate.zones[climate.setZone]->temperature);
  
  Serial.print("Humidity: ");
  Serial.println(climate.zones[climate.setZone]->humidity);
  
  Serial.print("Last updated: ");
  int sinceUpdate = (millis() - climate.zones[climate.setZone]->lastUpdated) / 1000;
  Serial.print(sinceUpdate);
  Serial.println("s");

  Serial.print("\n/*-------------------------------------*/\n\n");

  Serial.println("Current Settings:");
  
  Serial.print("Set Temperature: ");
  Serial.println(climate.setTemperature);

  Serial.print("Set Mode: ");
  Serial.println(climate.setMode);

  Serial.print("Set Zone: ");
  Serial.println(climate.setZone);

  Serial.print("Status: ");
  Serial.println(climate.status);

  Serial.print("\n/*-------------------------------------*/\n\n");

  Serial.print("Websocket ");
  Serial.println(connectUtil.isSocketConnected ? "connected": "disconnected");
  Serial.print("WiFi: ");
  Serial.println(wifiMulti.run() == WL_CONNECTED ? "connected": "disconnected");
  Serial.print("Pending websocket emits: ");
  Serial.println(isEmitQueueEmpty() ? "False": "true");

  Serial.print("\n/*-------------------------------------*/\n\n");

  Serial.println("Zones: \n");
  for (int i=0; i < MAX_ZONES; ++i) {
    if (climate.zones[i] != NULL) {
      Zone* toPrint = climate.zones[i];
      Serial.print("Location: ");
      Serial.println(toPrint->locationName);
      Serial.print("Device Id: ");
      Serial.println(toPrint->deviceId);
      Serial.print("Index: ");
      Serial.println(toPrint->id);
      Serial.print("Temperature: ");
      Serial.println(toPrint->temperature);
      Serial.print("Transmission: ");
      Serial.println(toPrint->isRapid ? "RAPID": "LONG");
      Serial.println();
    }
  }

  Serial.print("/*-------------------------------------*/\n\n");

  Serial.print("====== END Status ======\n\n");
  
  Serial.flush();
  delay(100);
}

