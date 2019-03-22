/*======================================

  Smart Thermostat

  NodeMCU 1.0
 
======================================*/

/*========== Library Imports ==========*/
#include <cmath>
#include <cstring>
#include <queue>
#include <functional>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <Time.h>
#include <WebSocketsClient.h>
#include "AuthConstants.h"

/* END IMPORTS */


/*========== Thermostat CONSTANT Definitions ==========*/
// timer intervals in milliseconds
const int CLOCK_SYNC_INTERVAL = 24 * 60 * 60 * 1000; // Sync clock to internet time every 24 hours
const int COOLDOWN_INTERVAL = 2 * 60 * 1000; // Cool down time interval after HVAC cycle has completed or Mode has been changed
const int IO_INTERVAL = 5 * 1000; // Sample local temperature sensor every 5 seconds
const int REQUEST_INTERVAL = 3 * 1000; // Delay between values being input to operating values being updated
const int LCD_CYCLE_INTERVAL = 2 * 1000; // Refresh lcd to reflect changed values - user made changes will not wait for cycle
const int LCD_TIMEOUT_INTERVAL = 30 * 1000; // Turn off lcd 30 seconds after last request application
const int PROGRAM_RUN_INTERVAL = 10 * 1000; // Delay between pre-program and thermostat cycle updates
const int DEFAULT_TIMESTAMP = 5 * 60 * 1000; // Time interval between monitored zone checks
const int RUN_ON_INTERVAL = 30 * 1000; // Fan run on interval after heat/ac cycle has finished
const int EMIT_COOLDOWN = 500; // Force half second delay between emits to prevent repeated emit events of the same data
const int HEARTBEAT_INTERVAL = 15 * 1000; // Websocket keep-alive message interval
const int MONITOR_INTERVAL = 60 * 1000;
const int RECONNECT_INTERVAL = 5 * 1000;
const int LOGIN_RECONNECT_INTERVAL = 60 * 1000;

// system limit constants
const float MIN_TEMP = 60; // Minimum set temperature in degrees Fahrenheit
const float MAX_TEMP = 80; // Maximum set temperature in degrees Fahrenheit
const int SAMPLE_SIZE = 10; // Number of inputs to read from the analog input for local interface push buttons
const int MIN_SAMPLE_SIZE = 2;
const int MAX_ZONES = 10; // Max number of zones allowed
const int TEMPERATURE_THRESHOLD = 2; // Threshold outside of which an HVAC cycle will start e.g. set temp 70, mode cool, cycle won't start until actual temp is 72
const float TEMPERATURE_UPDATE_THRESHOLD = 0.5; // Threshold outside of which a new websocket message will be sent
const int EMIT_DIFF_THRESHOLD = 1; // difference threshold between last emitted zone value and current 
const int MAX_RETRY = 20;

// temperature and humidity sensor constants
const int INITIALIZE_T_H = -99; // Value to initialize temperature and humidity variables

// program constants
const int PROGRAM_LENGTH = 112; // Data points for a complete program
const int PROGRAM_CHAR_LIMIT = 10;

// mode character constants
const char MODE_OPTIONS[4] = {'O', 'F', 'C', 'H'};
const char CHAR_SENTINEL = 'E';

// request constants
const int SENTINEL_TEMPERATURE = -100;
const char SENTINEL_MODE = '\0';
const int SENTINEL_ZONE = -1;

// sensor constants
const int ONE_WIRE_BUS = 0; // GPIO 0 NodeMCU pin D3 - DS18B20 bus
const int TEMPERATURE_PRECISION = 9; // DS18B20 temperature sensor resolution
DeviceAddress LOCAL_SENSOR = {0x28, 0xFF, 0x05, 0x47, 0x21, 0x17, 0x04, 0x20}; // Memory address of DS18B20 sensor
const int SENSOR_MAX = 130; // Sanity check value to prevent abnormal values from being used
const int SENSOR_MIN = -40;
const int MAX_SINCE_LAST_UPDATE = 6 * 60 * 1000; // Max time since last temperature sent from sensor on normal interval
const int MAX_SINCE_LAST_RAPID_UPDATE = 60 * 1000; // Max time since last temperature sent from sensor that is being used for the thermostat cycle

// relay pins
const int FAN_RELAY = 12; // NodeMCU D6 pin - HVAC fan ground trigger
const int AC_RELAY = 14; // NodeMCU D5 pin - HVAC ac system ground trigger
const int HEAT_RELAY = 13; // NodeMCU D7 pin - HVAC heater ground trigger

// Local interface
const int ANALOG_INPUT = A0; // Local interface - 4 buttons

// LCD values: NodeMCU D2 pin GPIO 4 - SDA , NodeMCU D1 pin GPIO 5 - SCL
const int i2cAddr = 0x3F; // I2C memory address
const int en = 2;
const int rw = 1;
const int rs = 0;
const int d4 = 4;
const int d5 = 5;
const int d6 = 6;
const int d7 = 7;
const int bl = 3;
const int LCD_CHAR_LIMIT = 17; // 16 character limit plus null terminator
const char LCD_BLANK_LINE[17] = "                "; // blank line for lcd

/* END THERMOSTAT CONSTANT DEFINITIONS */


/*========== Communication CONSTANT Definitions ==========*/
// wifi router connection
const char* ROUTER_SSID = Authorization::ROUTER_SSID; // Wifi id
const char* ROUTER_PASSWORD = Authorization::ROUTER_PASS; // Wifi password

// nodejs server connection
const char* SERVER_ADDRESS = Authorization::SERVER_ADDR; // Local proxy server address
const char* USERNAME = Authorization::SERVER_USER; // Local proxy server login username
const char* PASSWORD = Authorization::SERVER_PASS; // Local proxy server login password
const int HTTP_PORT = 3001; // Local proxy server server http port
const int WEBSOCKET_PORT = 3575; // Local proxy server server socket.io port

// remote dht sensor server
const int AP_PORT = 2772; // Nodemcu webserver port for remote sensors

/* END COMMUNICATION CONSTANT DEFINITIONS */

/* END CONSTANT DEFINITIONS */


/*========== Global VARIABLE Definitions ==========*/
// debugger
bool DEBUG = true; // true starts serial debug

/* END VARIABLE DEFINITIONS */

/* END GLOBAL VARIABLE DEFINITIONS */


/*========== Structure Definitions ==========*/
struct Analog {
  int analogIndex = 0; // current analog sampling array index
  int analogInputs[SAMPLE_SIZE]; // analog sampling array
};

struct Credentials {
  char token[200]; // json web token
  int isAuthed = 0; // set to 1 when authorization is complete
};

struct Error {
  bool hasError = false; // true if an error has been set
  char errorName[LCD_CHAR_LIMIT]; // error type
  char errorDetail[LCD_CHAR_LIMIT]; // error detail message
};

struct Program {
  bool isActive = false; // pre-programmed values will be used for operating values
  bool isLoaded = false; // true if all fields initialized
  char name[PROGRAM_CHAR_LIMIT + 1]; // text name of program - max 10 characters
  char mode; // either COOL or HEAT in a program
  int schedule[PROGRAM_LENGTH]; // array with the following order: 4 ints per time period [hour, minute, temperature, zone], 4 time periods per day, 7 days in schedule
  char queryId[25]; // query id for stored program
};

struct Request {
  bool isPending = false; // true if all operation changes have been applied, false if not
  bool setSleep = false; // set climate control to sleep mode
  char mode = SENTINEL_MODE; // temporary set mode - wait for delay duration in case new inputs are made
  int zone = SENTINEL_ZONE; // temporary selected zone - wait for delay duration in case new inputs are made
  int temperature = SENTINEL_TEMPERATURE; // temporary set temperature - wait for delay duration in case new inputs are made
};

struct Screen {
  bool isDisplayOn = false; // current state of lcd
  char currentPage = 'h'; // current displayed page id
  int displayZone = 0; // current climate control zone to display
};

struct Task {
  const char* errorName; // store error name for error emit
  const char* errorMessage; // store error message for error emit
  bool fullUpdate; // store bool for program update confirmation
  int id; // task id to determine which function to call
          // 0 - echoPing
          // 1 - emitProgramId
          // 2 - emitError
          // 3 - emitClimateStatus
          // 4 - emitProgramUpdateConfirmation
};

struct Timeout {
  uint64_t button; // local interface button delay timer
  uint64_t coolDown; // hardware cool down timer
  uint64_t sensor; // local dht sensor check interval timer
  uint64_t socketEmit; // emit cool down to prevent duplicate messages
  uint64_t heartbeat; // heartbeat timer start
  uint64_t lcdRefresh; // cycle values in display
  uint64_t lcdTimeout; // timer to display dimming after 5 minutes
  uint64_t programDelay; // timer to apply pre-programmed values to operating values
  uint64_t request; // operating condition request timer
  uint64_t output; // timer to check and update thermostat output
  uint64_t timeSync; // sync internal time to server time once per 24 hours
  uint64_t checkDefault; // periodically check if monitored zone is still valid - run default cycle if not - return to monitored zone if it becomes available again
  uint64_t runOn; // fan run on countdown after heat/ac cycle has completed
  uint64_t monitor;
  uint64_t reconnect;
  uint64_t retryConnection;
};

struct ConnectionUtility {
  bool isInit = true;
  std::queue<struct Task> emitQueue; // queue of pending websocket emits
  int maxQueue = 20;
  bool isWifiConnected = false;
  bool isLoggedIn = false;
  bool isSocketConnected = false;
};

struct Zone {
  char locationName[LCD_CHAR_LIMIT]; // where zone sensor is located
  int deviceId; // unique id for each zone - to be used for user id
  int id; // internal id for each zone - to be used for system addressing
  float temperature; // in 0.01 of a degree
  float lastTemperature; // last temperature that triggered an emit
  float humidity; // in 0.01 of a %
  float lastHumidity; // last humidity that triggered an emit
  uint64_t lastUpdated; // check if monitored sensor has sent a timely update respecting its set transmission rate
  bool isRapid; // true if sensor has been set for rapid tranmission, false if set for long tranmission
};

struct Climate {
  bool isF = true; // true sets dht to read fahrenheit
  bool isLoaded = false; // true if all values are initialized
  bool isTimeSet = false; // true when time has been synced with server
  bool isRapidSet = false; // true is selected zone has confirmed it's set to rapid transmission
  bool isActive = true; // true if sleep mode is inactive
  int localSensorFailed = 0; // count of failed reads from local sensor backup
  char setMode = 'O'; // current desired mode
  char status[8] = "OFF"; // current thermostat operating status, either "RUNNING" OR "OFF"
  int setTemperature = 70; // current desired temperature in degrees
  int setZone = 0; // index of selected zone to monitor
  Zone* zones[MAX_ZONES]; // array of pointers to zone structs
};

/* END STRUCTURE DEFINITIONS */


/*========== Object/Structure Initialization ==========*/
LiquidCrystal_I2C lcd(i2cAddr, en, rw, rs, d4, d5, d6, d7, bl, POSITIVE); // init LCD
OneWire oneWire(ONE_WIRE_BUS); // One wire bus setup
DallasTemperature localSensor(&oneWire); // DS18B20 temperature sensor
ESP8266WiFiMulti wifiMulti; // wifi connection
ESP8266WebServer server(AP_PORT); // webserver for remote sensors
HTTPClient http; // http client
WebSocketsClient io; // socket.io client
Credentials credentials; // login credentials
Climate climate; // primary climate status and operation data
Timeout timer; // various timer timestamps
Request request; // updated user inputs pending application
Program program; // a pre-programmed thermostat operating schedule
Screen screen; // lcd operation data
Error error; // error message struct
ConnectionUtility connectUtil; // websocket utility 
Analog analog; // analog input utilities

/* END OBJECT/STRUCTURE INITIALIZATIONS */


/*========== Function Declarations ==========*/
// http tab
void login();
void syncTime();
void handleSensorPostRequest();
void confirmRapidPostRequest();

// interface tab
void listenForButton();
int normalizeAnalogInputs();
void resetAnalogInput();
void handleAnalogInput(int);

// lcd tab
void turnOffScreen();
void turnOnScreen();
void loadLCDPage(char);
void loadMainPage();
void loadModePage();
void loadZonePage(int);
void loadErrorPage();
void printConnectNotification(char);
void printToLCD(const char*, const char*);

// message_text tab
void composeLoginMessage(char*);
void composeClimateStatusMessage(char*);
void composeProgramDataMessage(char* message);
void stringifyMode(char, char*);
char charifyMode(const char*);
void stringifyProgOnOff(char, char*);
int getStringLength(int);

// output tab
void runThermostatCycle();
void setCycle(bool);
void runOn(bool);
void setAllCyclesOff();

// program tab
void usePreprogrammedValues();
int getProgramIndex();
bool isProgramValid(JsonObject& root);
void updateProgram(JsonObject&);

// request tab
void updateClimateSettings(JsonObject&);
void processPendingRequests();
void resetRequest();

// sensor tab
void checkLocalSensor();
bool isSensorValid(float, float);
bool isSensorCurrent(uint64_t, bool);

// system
void connectWifi();
bool isClimateControlLoaded();
void initClimateControl();
void initDefaultZone();
void printStatus();

// time tab
float getComparisonTime();
int getDay();
int getHour();
int getMinute();

// websocket tab
void ioEvent(WStype_t, uint8_t*, size_t);
void echoPing();
void emitError(const char*, const char*);
void emitClimateStatus();
void emitProgramId();
void emitProgramUpdateConfirmation(bool);
bool isEmitCooldownExpired();
void processQueuedEmits();
bool isEmitQueueEmpty();
void emitHeartbeat();

// zone tab
int addZone(bool, int);
int removeZone(int);
void setZoneName(int, const char*);
int updateZone(int, float, float, bool);
int isZoneValid(int);
bool isIdValid(int);
void runDefault();
int selectZone(int);
int countZones();
void queryEmitStatus(int);
int getIndexByDevice(int);


/* END FUNCTION DEFINITIONS */

/* END DEFINITIONS */


/*========== Initial Setup ==========*/
void setup() {
  // begin debugger if DEBUG is true
  Serial.begin(9600);
  if (!DEBUG) Serial.end();
  delay(10);
  Serial.setDebugOutput(DEBUG);
  Serial.println("\n\n");

  // initialize relay pins
  pinMode(FAN_RELAY, OUTPUT);
  pinMode(AC_RELAY, OUTPUT);
  pinMode(HEAT_RELAY, OUTPUT);
  
  // relay activated when pin is sent low - start relays deactivated
  digitalWrite(FAN_RELAY, HIGH);
  digitalWrite(AC_RELAY, HIGH);
  digitalWrite(HEAT_RELAY, HIGH);

  // alive status led blink
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // initialize lcd
  lcd.begin(16, 2);
  lcd.home();
  lcd.print(" System Loading ");
  screen.isDisplayOn = true;
  delay(10);

  // connect to wifi
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  wifiMulti.addAP(ROUTER_SSID, ROUTER_PASSWORD);

  // define remote dht sensor handler and start webserver
  server.on("/sensor/update", HTTP_POST, handleSensorPostRequest);
  server.on("/sensor/confirm-rapid", HTTP_POST, confirmRapidPostRequest);
  server.begin();
  Serial.println("HTTP server started\n");

  // define websocket handler
  io.beginSocketIO(SERVER_ADDRESS, WEBSOCKET_PORT);
  io.onEvent(ioEvent);
  io.setReconnectInterval(RECONNECT_INTERVAL);

  // begin monitoring local temperature sensor
  localSensor.begin();
  localSensor.getDeviceCount();
  localSensor.setResolution(LOCAL_SENSOR, TEMPERATURE_PRECISION);

  initClimateControl();
  while(!isClimateControlLoaded()) {
    delay(100);
  }

  // initialize timers
  uint64_t start = millis();
  timer.button = start;
  timer.coolDown = start;
  timer.sensor = start;
  timer.output = start;
  timer.heartbeat = start;
  timer.lcdRefresh = start;
  timer.lcdTimeout = start;
  timer.request = start;
  timer.timeSync = start;
  timer.socketEmit = start;
  timer.checkDefault = start;
  timer.runOn = 0;
  timer.heartbeat = start;
  timer.monitor = start;
  timer.retryConnection = start;

  connectWifi();
  if (connectUtil.isWifiConnected) login();
  if (connectUtil.isLoggedIn) syncTime();

  // show loading complete on lcd
  char line1[LCD_CHAR_LIMIT] = "";
  char line2[LCD_CHAR_LIMIT] = "";
  if (!connectUtil.isWifiConnected) {
    timer.retryConnection = millis();
    Serial.println("No Wifi connection");
    strcat(line1, " WiFi not found ");
    strcat(line2, LCD_BLANK_LINE);
  } else if (!connectUtil.isLoggedIn) {
    Serial.println("Could not log in");
    strcat(line1, "Could not log in");
    strcat(line2, LCD_BLANK_LINE);
  } else {
    Serial.println("Setup complete");
    strcat(line1, " Setup complete ");
    strcat(line2, LCD_BLANK_LINE);
  }
  printToLCD(line1, line2);

  delay(100);
} // end setup

void loop() {

  // attempt WiFi and server reconnect if disconnected
  if (!connectUtil.isWifiConnected && millis() - timer.retryConnection > RECONNECT_INTERVAL) {
    connectWifi(); 
    timer.retryConnection = millis(); 
  } else if (!connectUtil.isLoggedIn && millis() - timer.retryConnection > LOGIN_RECONNECT_INTERVAL) {
    login();
    timer.retryConnection = millis(); 
  } 
  
  // handle pending websocket emits
  processQueuedEmits();
  delay(10);
  
  // websocket listener
  io.loop();
  delay(10);
  
  // http server listener
  server.handleClient();
  delay(10);

  // listen for local user input
  listenForButton();
  delay(10);

  // websocket heartbeat
  if (millis() - timer.heartbeat > HEARTBEAT_INTERVAL) {
    digitalWrite(LED_BUILTIN, LOW);
    emitHeartbeat();
    timer.heartbeat = millis();
    digitalWrite(LED_BUILTIN, HIGH);
  }

  // check local sensor
  if (millis() - timer.sensor > IO_INTERVAL) {
    checkLocalSensor();
    timer.sensor = millis();  
  }

  // update operating values from program
  if (millis() - timer.programDelay > PROGRAM_RUN_INTERVAL) {
    usePreprogrammedValues();
    timer.programDelay = millis();
  }

  // run thermostat output cycle
  if (millis() - timer.output > IO_INTERVAL) {
    runThermostatCycle();
    timer.output = millis();
  }

  // refresh lcd
  if (screen.isDisplayOn && millis() - timer.lcdRefresh > LCD_CYCLE_INTERVAL) {
    // turn screen off if timed out
    if (screen.isDisplayOn && millis() - timer.lcdTimeout > LCD_TIMEOUT_INTERVAL) {
      turnOffScreen();
    } else if (screen.isDisplayOn) {
      loadLCDPage(screen.currentPage);
    }
  }

  // process user requests
  if (millis() - timer.request > REQUEST_INTERVAL) {
    if (request.isPending) processPendingRequests();
    timer.request = millis();
  }

  // sync internal time with internet
  if (millis() - timer.timeSync > CLOCK_SYNC_INTERVAL) {
    Serial.println("Sync internal clock");
    climate.isTimeSet = false;
    syncTime();
    if (climate.isTimeSet) {
      timer.timeSync = millis();
    } else {
      timer.timeSync = millis() - CLOCK_SYNC_INTERVAL + 5000;
    }
  }

  // print system status
  if (DEBUG && millis() - timer.monitor > MONITOR_INTERVAL) {
    Serial.println("DEBUG print status");
    printStatus();
    timer.monitor = millis();
  }

  delay(100);
}
