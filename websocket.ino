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
      Serial.print("[ws] Disconnected!\n\n");
      socketUtil.isConnected = 0;
      break;
    case WStype_CONNECTED:
      Serial.printf("[ws] Connected to url %s\n\n", payload);
      socketUtil.isConnected = 1;
      io.sendTXT("5");
      break;
    case WStype_TEXT:
      {
        Serial.print("[ws] Incoming message\n\n");
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
            echoPing();
            
          } else if (!strcmp(socketMessage, "proxy-request-get-climate-data")) {
            // generate climate data
            emitClimateStatus();

          } else if (!strcmp(socketMessage, "proxy-request-get-program-id")) {
            // generate program data
            emitProgramId();
            
          } else if (!strcmp(socketMessage, "proxy-request-update-climate-settings")) {
            // set climate settings to user specifications
            updateClimateSettings(root);
            
          } else if (!strcmp(socketMessage, "proxy-request-update-program")) {
            // load a new program, activate if isActive is true
            updateProgram(root);
            
          } else if (!strcmp(socketMessage, "proxy-request-toggle-program")) {
            toggleProgram(root["isActive"]);
            
          } else if (!strcmp(socketMessage, "proxy-request-update-zone-name")) {
            // update zone name
            int setId = root["deviceId"];
            const char* setName = root["name"];
            setZoneName(setId, setName);
            
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
      Serial.printf("[ws] Get binary length: %u\n\n", length);
      hexdump(payload, length);
      break;
    case WStype_ERROR:
      Serial.printf("[ws] Error: %s\n\n", payload);
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
  if (isEmitCooldownExpired()) {
    io.sendTXT("42[\"response-ping-thermostat\": {}]");
    timer.socketEmit = millis();
  } else {
    Task t;
    t.id = 0;
    socketUtil.emitQueue.push(t);
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
  if (isEmitCooldownExpired()) {
    char message[72];
    strcpy(message, "42[\"response-get-program-id\": {\"id\": \"");
    strcat(message, program.id);
    strcat(message, "\"}]");
    io.sendTXT(message);
    timer.socketEmit = millis();
  } else {
    Task t;
    t.id = 1;
    socketUtil.emitQueue.push(t);
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
  if (isEmitCooldownExpired()) {
    char* message = (char*) malloc(sizeof(char) * 200);
    composeErrorMessage(errorName, errorMessage, message);
    io.sendTXT(message);
    free(message);
    timer.socketEmit = millis();
  } else {
    Task t;
    t.errorName = errorName;
    t.errorMessage = errorMessage;
    t.id = 2;
    socketUtil.emitQueue.push(t);
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
  if (isEmitCooldownExpired()) {
    int zones = countZones();
    char* message = (char*) malloc(sizeof(char) * (100 * zones + 100));
    composeClimateStatusMessage(message);
    io.sendTXT(message);
    free(message);
    timer.socketEmit = millis();
  } else {
    Task t;
    t.id = 3;
    socketUtil.emitQueue.push(t);
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
  if (isEmitCooldownExpired()) {
    if (fullUpdate) {
      char* message = (char*) malloc(sizeof(char) * (100));
      composeProgramDataMessage(message);
      io.sendTXT(message);
      free(message);
    } else {
      char message[50];
      strcpy(message, "42[\"response-update-program\": {\"isActive\": ");
      strcat(message, program.isActive ? "true": "false");
      strcat(message, "}]");
      io.sendTXT(message);
    }
    timer.socketEmit = millis();
  } else {
    Task t;
    t.fullUpdate = fullUpdate;
    t.id = 4;
    socketUtil.emitQueue.push(t);
  }
}

bool isEmitCooldownExpired() {
  return millis() - timer.socketEmit > EMIT_COOLDOWN;
}

void processQueuedEmits() {
  if (!isEmitQueueEmpty && isEmitCooldownExpired()) {
    Task nextTask = socketUtil.emitQueue.front();
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
    socketUtil.emitQueue.pop();
    timer.socketEmit = millis();
  }
}

bool isEmitQueueEmpty() {
  return socketUtil.emitQueue.empty();
}

