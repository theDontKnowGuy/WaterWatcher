/*
   WaterWatcher by theDontKnowGuy

   Monitor fertelizer pump for
   Version 1.0 - Basic functionality works
*/

#include <Arduino.h>
#include "secret.h"


#define RELEASE true
#define SERVER
const int FW_VERSION = 2022060501;
int DEBUGLEVEL = 2;     // set between 0 and 5. This value will be overridden by dynamic network configuration json if it has a higher value


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////// Firmware update over the air (FOTA) SECTION///////////////////////////////////////////????//////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#include <HTTPUpdate.h>                                                                /// year_month_day_counternumber 2019 is the year, 04 is the month, 17 is the day 01 is the in day release
const char *fwUrlBase = "https://raw.githubusercontent.com/theDontKnowGuy/WaterWatcher/master/fota/"; /// put your server URL where the *.bin & version files are saved in your http ( Apache? ) server


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////// NETWORK SECTION/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// defined in secrets.h:    const char* ssid =    "my Wifi SSID";
// defined in secrets.h:    const char* password = "password";

#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

typedef struct
{
  int resultCode;
  String header;
  String body;
  int headerLength;
  int bodyLength;
} NetworkResponse;

const int httpsPort = 443;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////// WEBSERVER SECTION///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#if defined(SERVER)

const bool isServer = true;
#include <WebServer.h>
WebServer server(80);
#else
const bool isServer = false;
#endif

typedef struct
{
  String endpointDescription;
  String descriptor;
  char host[100];
  int port;
  String URI;
  bool isSSL = false;
  String successString;
} endpoint;

#include <ESPmDNS.h>
char *serverMDNSname = "GreenPlanet"; //clients will look for http://GreenPlanet.local

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////// LOGGING SECTION/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool log2Serial = true; //move to false in production to save some time


//char loggerHost[100] = "192.168.1.200";
//String logTarget =     "/MyRFDevicesHub/MyRFDevicesHubLogger.php"; /// leave empty if no local logging server (will only Serial.print logs)
////String logTarget =     "/logs"; /// leave empty if no local logging server (will only Serial.print logs)
//int loggerHostPort = 80;

RTC_DATA_ATTR int loggingType = 3;
RTC_DATA_ATTR int loggingCounter = 0;
RTC_DATA_ATTR char c_logTarget[200];
//RTC_DATA_ATTR char loggerHost[100] = "api.thingspeak.com";
RTC_DATA_ATTR char loggerHost[100] = "maker.ifttt.com";
//RTC_DATA_ATTR char loggerHost[100] = "192.168.1.200";
//defined in secrets.h :   String logTarget =     "channels/<channel code here>/bulk_update.csv";
//defined in secrets.h :   String logTarget = "/trigger/GreenPlanet2/with/key/xxxxxxx-xxx";
RTC_DATA_ATTR int loggerHostPort = 443;
String write_api_key = "";

//RTC_DATA_ATTR int loggingType = 1;
//RTC_DATA_ATTR char c_logTarget[200];

String logBuffer = "";
String networkLogBuffer = "";
unsigned long previousTimeStamp = millis(), totalLifes, LiveSignalPreviousMillis = millis(), lastMessageTiming = 0;
int maxLogAge = 200; //  how many sec to keep log before attempting to send.
int logAge = 0, LivePulseLedStatus = 0;
int failedLogging2NetworkCounter = 0;
int addFakeSec = -1;
String firstLogTimeStamp = "";

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////// REMOTE CONFIGURATION SECTION ///////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int ConfigurationVersion = 0;
RTC_DATA_ATTR char dataUpdateHost[100];
RTC_DATA_ATTR int dataUpdatePort;
String dataUpdateURI;
RTC_DATA_ATTR char c_dataUpdateURI[200];

char *serverDataUpdateHost = "raw.githubusercontent.com";
int serverDataUpdatePort = 443;

#if (RELEASE)
String serverDataUpdateURI = "/theDontKnowGuy/WaterWatcher/master/configuration/WaterWatcher.json";
String dataUpdateURI_fallback = "/theDontKnowGuy/WaterWatcher/master/configuration/WaterWatcher.json"; /// see example json file in github. leave
#else
String serverDataUpdateURI = "/theDontKnowGuy/WaterWatcher/master/configuration/WaterWatcher_dev.json";
String dataUpdateURI_fallback = "/theDontKnowGuy/WaterWatcher/master/configuration/WaterWatcher_dev.json"; /// see example json file in github. leave
#endif

char *dataUpdateHost_fallback = "raw.githubusercontent.com";
int dataUpdatePort_fallback = 443;
String dataUpdateURI_fallback_local = "/GreenPlanet/GreenPlanetConfig.json";                               /// see example json file in github. leave value empty if no local server

int ServerConfigurationRefreshRate = 60;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////// MAINTENANCE AND HARDWARE SECTION ///////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int red = 2;
int green = 17;
int blue = 12;

int maintenanceRebootHour = 4; // the hub will reboot once a day at aproximitly this UTC hour (default 4 am), provided it was running ~24 hours

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////// JSON SECTION ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino_JSON.h>
JSONVar eyeConfig;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////// TIME AND CLOCK SECTION /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <time.h>

const char *ntpServer = "pool.ntp.org";
long gmtOffset_sec = 7200;
int daylightOffset_sec = 0;
struct tm timeinfo;
bool ihaveTime = false;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////// MQTT SECTION ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//char* MQTTtopicDevices = "MyRFDeviceHub";
//char* MQTTtopicControl = "MyRFDeviceHub/control";
//char* MQTTendpointID = "Hub1";
//char* MQTTHost = "192.168.1.200";
//int MQTTPort = 1883;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////// IR SECTION//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//#include <IRremote.h>

#include <IRrecv.h>
#include <IRsend.h>
#include <IRac.h>
#include <IRtext.h>
#include <IRutils.h>

long learningTH = 15000;

uint16_t kIrLed = 14;
const uint16_t kRecvPin = 15;
IRac ac(kIrLed);  // Create a A/C object using GPIO to sending messages with.

const uint16_t kCaptureBufferSize = 2048;
const uint16_t kMinUnknownSize = 12;
const uint8_t kTimeout = 50;

int maxSignalLength = 200;
IRrecv irrecv(kRecvPin, kCaptureBufferSize, kTimeout, true);
decode_results results; // Somewhere to store the results

uint16_t ACcode[200];
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////// DHT SECTION  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <dhtnew.h>
#define DHTleg 4
DHTNEW DHTsensor(DHTleg);
float DHTt = 0;
float DHTh = 0;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////// EEPROM SECTION  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <EEPROM.h>
int maxEEPROMMessageLength = 1000;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////// WATCHGDOG SECTIO1N //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const int wdtTimeout = 60000; //time in ms to trigger the watchdog // NOTE: less than that, on upgrade it barks. alternatively, kill dog before update
hw_timer_t *timer = NULL;
RTC_DATA_ATTR byte bootCount = 0;
RTC_DATA_ATTR time_t rightNow;
RTC_DATA_ATTR uint64_t Mics = 0;
//unsigned long chrono;

void IRAM_ATTR resetModule()
{
  ets_printf("reboot for freeze\n");
  esp_restart();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////// DEEPSLEEP SECTION //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
int WakeUpSensorThreshold = 50;    /* Greater the value, more the sensitivity */
touch_pad_t touchPin;
int delayBetweenExecs = 3;
int sleepAfterExec = 1800;
int sleepTime = 1800;
int sleepAfterPanic = 7200;
int sleepRandFactor = 120;

//#include "esp_system.h"
esp_sleep_wakeup_cause_t wakeup_reason;
RTC_DATA_ATTR int RTCpanicStateCode = 0;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////// THIS PROGRAM SPECIFIC SECTION //////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

String deviceID;                    // = "Unknown";  // will be overwritten by dynamically loaded configuration file. if you see this on the hub - update configuration to new MAC
String deviceGroup = "New Devices"; // will be overwritten by dynamically loaded configuration file. if you see this on the hub - associate new device to a group
String deviceLocation = "default";  // same idea
String memberInOperationPlans = ""; // same idea
String MACID;                       // MAC address converted to long


int inxParticipatingPlans = 0;
int inxParticipatingIRCodes = 0;
int recessTime = 160; // between executions, not to repeat same exection until clock procceeds

#define NOTINITIATED -1
#define RESTING 0
#define INIT 1
#define DONE_INIT 2
#define RUNNING_UNSTABLE 3
#define RUNNING_STABLE 4
#define POTINTIAL_LEAK 5
#define LEAK 6



int waterStatus = -1;

int restingInterval = 10000 ; //if no read after this time probably no water, resting
int maxStdDev = 50; // beyond this reads are too noisy
long InitTS = 0;
long initPeriod = 1000 * 60;

int waterClicks[500];
int waterClicksInx = 0;
#define SENSOR 15
long lastSensorRead = 0 , previousSensorRead;
int logChunkSize = 10;
int tooLongInterval = 3000;
int tooShortInterval = 100;
int minNormalAvgWater = 1100;
int maxAcceptableStdDev = 50;
int leakAvgWater = 1000;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////// SERVER CONFIGURATION /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(SERVER)

String serverConfiguration = "";

#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////// END OF DECLERATION /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void IRAM_ATTR waterSensorRead();
int networklogThis(String message, bool asProxy = false);

void setup()
{
  pinMode(red, OUTPUT);  pinMode(green, OUTPUT);  pinMode(blue, OUTPUT); pinMode(kIrLed, OUTPUT); digitalWrite(kIrLed, LOW);
  digitalWrite(blue, HIGH); vTaskDelay(100); digitalWrite(blue, LOW); digitalWrite(red, LOW); digitalWrite(green, LOW);

  // Hardeware Watchdog
  timer = timerBegin(0, 80, true);                  //timer 0, div 80
  timerAttachInterrupt(timer, &resetModule, true);  //attach callback
  timerAlarmWrite(timer, wdtTimeout * 1000, false); //set time in us
  timerAlarmEnable(timer);                          //enable interrupt

  MACID = mac2long(WiFi.macAddress());

  if (log2Serial)
    Serial.begin(115200);

#if (RELEASE)
#else
  logThis(1, "********** NOT A RELEASE VERSION ******************* NOT A RELEASE VERSION ******************* NOT A RELEASE VERSION ********* ", 2);
#endif
  logThis(3, "Starting GreenPlanet Device by the DontKnowGuy", 2);
  logThis(2, "Firmware version " + String(FW_VERSION) + ". Unique device identifier: " + MACID, 2);

  EEPROM.begin(4096);
  checkPanicMode();

  if (initiateNetwork() > 0)
  {
    networkReset();
  }

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();
  ihaveTime = true;
  updateTime(0);        ////??????????
  timerWrite(timer, 0); //reset timer (feed watchdog)

  LiveSignalPreviousMillis = millis();
  bootCount++;
  logThis(0, "This is boot No. " + String(bootCount), 3);
  if (bootCount == 1)
  {
    digitalWrite(red, HIGH); vTaskDelay(100); digitalWrite(red, LOW); vTaskDelay(50); digitalWrite(red, HIGH); vTaskDelay(100); digitalWrite(red, LOW); vTaskDelay(50); digitalWrite(red, HIGH); vTaskDelay(100); digitalWrite(red, LOW); vTaskDelay(50);
  }
  if (bootCount > 48)
    ESP.restart();

  Serial.printf("\n" D_STR_IRRECVDUMP_STARTUP "\n", kRecvPin);
  if (isServer) irrecv.enableIRIn(); // Start the receiver

  parseConfiguration(loadConfiguration());

  logThis(1, "This is device " + String(deviceID), 3);

#if defined(SERVER)
  logThis(1, "I am a server", 2);
  startWebServer();
#endif

  wakeupReason();
  DHTsensor.read();
  DHTt = DHTsensor.getTemperature();
  DHTh = DHTsensor.getHumidity();

  logThis(1, "Temperature: " + String(DHTt) + " Humidity: " + String(DHTh));
  logThis(3, "Initialization Completed.", 3);
  digitalWrite(blue, LOW); // system live indicator

  if (networklogThis(networkLogBuffer) == 0)
  {
    networkLogBuffer = "";
    logAge = 0;
  }

#if defined(SERVER)

  xTaskCreatePinnedToCore(
    serverOtherFunctions, "serverOtherFunctions" // A name just for humans
    ,
    7000 // This stack size can be checked & adjusted by reading the Stack Highwater
    ,
    NULL, 3 // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,
    NULL, 1);

  xTaskCreatePinnedToCore(
    webServerFunction, "webServerFunction" // A name just for humans
    ,
    7000 // This stack size can be checked & adjusted by reading the Stack Highwater
    ,
    NULL, 0 // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,
    NULL, 0);

#else
  ;
  if (wakeup_reason == ESP_SLEEP_WAKEUP_TOUCHPAD) {
    logThis(1, "Executing Plan becuase woke up by touchpad", 2);  // on wakeup by touchpad - play ir plan for testing
    execPlan(3);
  }
  gotoSleep(calcTime2Sleep()); ///is this order right ????????
#endif


  attachInterrupt(SENSOR, waterSensorRead, RISING);



} //setup

#if defined(SERVER)
void webServerFunction(void *pvParameters) {

  (void) pvParameters;
  for (;;) {
    server.handleClient();
    blinkLiveLed();
    vTaskDelay(10 / portTICK_RATE_MS);
    timerWrite(timer, 0); //reset timer (feed watchdog)
  }
}

void serverOtherFunctions(void *pvParameters) {

  (void) pvParameters;
  for (;;) {
    // waterSensorRead();
    timerWrite(timer, 0); //reset timer (feed watchdog)
    vTaskDelay(10 / portTICK_RATE_MS);
  }
}
#endif

void loop()
{
  //    waterSensorRead();

  // vTaskDelay(10 / portTICK_RATE_MS);
  //timerWrite(timer, 0);
}
