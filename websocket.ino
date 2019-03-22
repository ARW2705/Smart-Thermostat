/*=====================
      WEBSOCKET     
======================*/

/*
 * Handle websocket events
 * 
 * params: none
 * 
 * return: none
 */
void ioEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      if (connectUtil.isSocketConnected) {
        Serial.print("\n[ws] Disconnected!\n\n");
        connectUtil.isSocketConnected = false;
      }
      break;
    case WStype_CONNECTED:
      Serial.printf("\n[ws] Connected to url %s\n\n", payload);
      if (!connectUtil.isSocketConnected) printConnectNotification('s');
      connectUtil.isSocketConnected = true;
      io.sendTXT("5");
      break;
    case WStype_TEXT:
      {
        Serial.print("\n[ws] Incoming message\n\n");
        connectUtil.isSocketConnected = true;
        char* charPayload = (char*) payload;
        if (*charPayload == '3' || (!strcmp(charPayload, "40"))) break;

        // parse message
        int messageStart = strcspn(charPayload, "\"") + 1;
        int messageEnd = strcspn(charPayload, ",") - 1;
        int messageSize = messageEnd - messageStart;
        char* socketMessage = (char*) malloc(sizeof(char) * (messageSize + 1));
        for (int i=0; i < messageSize; ++i) {
          socketMessage[i] = charPayload[messageStart + i];
        }
        socketMessage[messageSize] = '\0';

        // parse json
        int jsonStart = strcspn(charPayload, "{");
        int jsonEnd = strcspn(charPayload, "\0") - 1;
        int dataSize = jsonEnd - jsonStart;
        char* charJson = (char*) malloc(sizeof(char) * (dataSize + 1));
        for (int i=0; i < dataSize; ++i) {
          charJson[i] = charPayload[jsonStart + i];
        }
        charJson[dataSize] = '\0';

        StaticJsonBuffer<2000> jsonBuffer;
        JsonObject& root = jsonBuffer.parseObject(charJson);
        if (!root.success()) {
          Serial.println("WStype_TEXT parseObject failed");
          Serial.println(socketMessage);
        } else {
          if (!strcmp(socketMessage, "proxy-request-ping-thermostat")) {
            // send alive status
            Serial.println("proxy-request-ping-thermostat");
            echoPing();
            /*----------------------------------------------------------*/
          } else if (!strcmp(socketMessage, "proxy-request-get-climate-data")) {
            // generate climate data
            Serial.println("proxy-request-get-climate-data");
            emitClimateStatus();
            /*----------------------------------------------------------*/
          } else if (!strcmp(socketMessage, "proxy-request-get-thermostat-program-id")) {
            // generate program data
            Serial.println("proxy-request-get-thermostat-program-id");
            emitProgramId();
            /*----------------------------------------------------------*/
          } else if (!strcmp(socketMessage, "proxy-request-update-climate-settings")) {
            // set climate settings to user specifications
            Serial.println("proxy-request-update-climate-settings");
            updateClimateSettings(root);
            /*----------------------------------------------------------*/
          } else if (!strcmp(socketMessage, "proxy-request-update-program")) {
            // load a new program, activate if isActive is true
            Serial.println("proxy-request-update-program");
            updateProgram(root);
            /*----------------------------------------------------------*/
          } else if (!strcmp(socketMessage, "proxy-request-toggle-program")) {
            // turn program on/off
            Serial.println("proxy-request-toggle-program");
            toggleProgram(root["isActive"]);
            /*----------------------------------------------------------*/
          } else if (!strcmp(socketMessage, "proxy-request-update-zone-name")) {
            // update zone name
            Serial.println("proxy-request-update-zone-name");
            int setId = root["deviceId"];
            const char* setName = root["name"];
            setZoneName(setId, setName);
            /*----------------------------------------------------------*/
          } else {
            Serial.print("Socket message ");
            Serial.print(socketMessage);
            Serial.println(" is not recognized");
          }
        }
        free(socketMessage);
        free(charJson);
      }
      break;
    case WStype_BIN:
      Serial.printf("\n[ws] Get binary length: %u\n\n", length);
      hexdump(payload, length);
      break;
    case WStype_ERROR:
      Serial.printf("\n[ws] Error: %s\n\n", payload);
      break;
  }
} // end ioEvent

/*
 * Send confirmation that thermostat is alive and accepting messages
 * 
 * params: none
 * 
 * return: none
 */
void echoPing() {
  if (connectUtil.isSocketConnected && isEmitCooldownExpired()) {
    Serial.println("response-ping-thermostat");
    io.sendTXT("42[\"response-ping-thermostat\": {}]");
    timer.socketEmit = millis();
  } else if (connectUtil.emitQueue.front().id != 0) {
    Serial.println("Queuing: response-ping-thermostat");
    Task t;
    t.id = 0;
    connectUtil.emitQueue.push(t);
  }
}

/*
 * Send the program's id (MongoDB object id value)
 * 
 * params: none
 * 
 * return: none
 */
void emitProgramId() {
  if (connectUtil.isSocketConnected && isEmitCooldownExpired()) {
    Serial.println("response-get-thermostat-program-id");
    char message[100];
    strcpy(message, "42[\"response-get-thermostat-program-id\": {\"queryId\": \"");
    strcat(message, program.queryId);
    strcat(message, "\"}]");
    io.sendTXT(message);
    timer.socketEmit = millis();
  } else if (connectUtil.emitQueue.front().id != 1) {
    Serial.println("Queuing: response-get-thermostat-program-id");
    Task t;
    t.id = 1;
    connectUtil.emitQueue.push(t);
  }
}

/*
 * Send error message
 * 
 * params: const char*, const char*
 * errorName - the name of the error type
 * errorMessage - details about the error
 * 
 * return: none
 */
void emitError(const char* errorName, const char* errorMessage) {
  if (connectUtil.isSocketConnected) {
    if (isEmitCooldownExpired()) {
      Serial.println("request-post-error");
      char* message = (char*) malloc(sizeof(char) * 200);
      composeErrorMessage(errorName, errorMessage, message);
      io.sendTXT(message);
      free(message);
      timer.socketEmit = millis();
    } else if (connectUtil.emitQueue.front().id != 2) {
      Serial.println("Queuing: request-post-error");
      Task t;
      t.errorName = errorName;
      t.errorMessage = errorMessage;
      t.id = 2;
      connectUtil.emitQueue.push(t);
    }
  }
}

/*
 * Emit the current climate status overview
 * 
 * params: none
 * 
 * return: none
 */
void emitClimateStatus() {
  if (connectUtil.isSocketConnected && isEmitCooldownExpired()) {
    Serial.println("request-post-climate-data");
    int zones = countZones();
    char* message = (char*) malloc(sizeof(char) * (100 * zones + 200));
    composeClimateStatusMessage(message);
    io.sendTXT(message);
    free(message);
    timer.socketEmit = millis();
  } else if (connectUtil.emitQueue.front().id != 3) {
    Serial.println("Queuing: request-post-climate-data");
    Task t;
    t.id = 3;
    connectUtil.emitQueue.push(t);
  }
}

/*
 * Emit confirmation of update to program with new values
 * 
 * params: bool
 * fullUpdate - if true, send full program, otherwise only send active state
 * 
 * return: none
 */
void emitProgramUpdateConfirmation(bool fullUpdate) {
  if (connectUtil.isSocketConnected && isEmitCooldownExpired()) {
    if (fullUpdate) {
      Serial.println("reponse-update-program");
      char* message = (char*) malloc(sizeof(char) * (800));
      composeProgramDataMessage(message);
      io.sendTXT(message);
      free(message);
    } else {
      Serial.println("response-toggle-program");
      char message[100];
      strcpy(message, "42[\"response-toggle-program\": {\"isActive\": ");
      strcat(message, program.isActive ? "true": "false");
      strcat(message, "}, \"queryId\": \"");
      strcat(message, program.queryId);
      strcat(message, "\"}]");
      io.sendTXT(message);
    }
    timer.socketEmit = millis();
  } else if (connectUtil.emitQueue.front().id != 4) {
    Serial.println("Queuing: response-update-program or response-toggle-program");
    Task t;
    t.fullUpdate = fullUpdate;
    t.id = 4;
    connectUtil.emitQueue.push(t);
  }
}

/*
 * Check if websocket emit cooldown has expired
 * Without a cooldown, websocket ends up firing 
 * mulitple times for the same message
 * 
 * params: none
 * 
 * return: bool
 * - true if emit cooldown has expired
 */
bool isEmitCooldownExpired() {
  return millis() - timer.socketEmit > EMIT_COOLDOWN;
}

/*
 * Get the next queued emit and call the appropriate function
 * 
 * params: none
 * 
 * return: none
 */
void processQueuedEmits() {
  if (connectUtil.isSocketConnected && !isEmitQueueEmpty() && isEmitCooldownExpired()) {
    Serial.println("Emitting from queue");
    Task nextTask = connectUtil.emitQueue.front();
    switch(nextTask.id) {
      case 0:
        echoPing();
        break;
      case 1:
        emitProgramId();
        break;
      case 2:
        emitError(nextTask.errorName, nextTask.errorMessage);
        break;
      case 3:
        emitClimateStatus();
        break;
      case 4:
        emitProgramUpdateConfirmation(nextTask.fullUpdate);
        break;
      default:
        char error[] = "Queued message has invalid ID: ";
        char id[3];
        sprintf(id, "%d", nextTask.id);
        strcat(error, id);
        emitError("InternalError", error);
        break;
    }
    connectUtil.emitQueue.pop();
    timer.socketEmit = millis();
  }
}

/*
 * Helper function to check if socket emit queue is empty
 * 
 * params: none
 * 
 * return: bool
 * - true if queue is empty
 */
bool isEmitQueueEmpty() {
  return connectUtil.emitQueue.empty();
}

/*
 * Emit heartbeat to keep webocket connection open
 * 
 * params: none
 * 
 * return: none
 */
void emitHeartbeat() {
  if (connectUtil.isSocketConnected) {
    io.sendTXT("2");
  }
}

