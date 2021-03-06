/*  Zenomat FW for Sonoff WiFi Switch
 *  ToDo: Auth for MQTT
 *        Auth for ota
 *  
 *  lb3mg@radiokameratene.no
 */

#include <ESP8266WiFi.h> //ESP communitys from Boards man.
#include <PubSubClient.h> //Nick O'Learys from Lib manager
#include <WiFiManager.h>  //tzapus from Lib manager
#include <EEPROM.h>
#include <ArduinoOTA.h>

WiFiClient wclient;
PubSubClient mqtt(wclient);

#include <Ticker.h>
Ticker periodic;

#define   SONOFF_BUTTON             0
#define   SONOFF_INPUT              14
#define   SONOFF_LED                13
#define   SONOFF_RELAY_PIN          12

#define EEPROM_SALT 14579
#define HOSTNAME "Zenomat"

const int CMD_WAIT = 0;
const int CMD_BUTTON_CHANGE = 1;

int cmd = CMD_WAIT;
//int relayState = HIGH;

//inverted button state
int buttonState = HIGH;

volatile unsigned long startPress = 0;

long lastMsg = 0;
char msg[50];
int value = 0;

typedef struct {
  char  bootState[6]      = "off";
  char  mqttHostname[32]  = "test.mosquitto.org";
  char  mqttPort[6]       = "1883";
  char  mqttUsername[16]    = "";
  char  mqttPassword[16]    = "";
  char  mqttClientID[24]  = "zenomatID";
  char  mqttTopic[32]     = "zenomat/relay";
  char  otaPassword[16]    = "";
  int   salt              = EEPROM_SALT;
} ZenomatSettings;

ZenomatSettings settings;

void reconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.println("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt.connect(settings.mqttClientID)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      mqtt.publish("zenomat/chatter", "Zenomat online");
      // ... and resubscribe
      mqtt.subscribe(settings.mqttTopic);
      Serial.println(settings.mqttTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      delay(5000); // Wait 5 seconds before retrying
    }
  }
}

void toggleLED(){
  int state = digitalRead(SONOFF_LED);
  digitalWrite(SONOFF_LED, !state);
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("WiFi config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  periodic.attach(0.2, toggleLED);
}

void updateMQTT(){
  int state = digitalRead(SONOFF_RELAY_PIN);
  String stateString; //Treated myself to a string object.
  if (state == 1){
    stateString = "On";
    } else {
    stateString = "Off";
  }
  int len=stateString.length();
  len+=1;//off by one bugfix
  char stateChar[len];
  stateString.toCharArray(stateChar,len);
  
  char topic[50];
  sprintf(topic, "%s/status", settings.mqttTopic);
  mqtt.publish(topic, stateChar);
}

void setRelayState(int state) {
  digitalWrite(SONOFF_RELAY_PIN, state);
  digitalWrite(SONOFF_LED, !state); //LED active low, opposit of relay.
  updateMQTT();
}

void turnOn() {
  setRelayState(1);
}

void turnOff() {
  setRelayState(0);
}

void toggleState() {
  cmd = CMD_BUTTON_CHANGE;
}

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}


void toggle() {
  Serial.println("toggle state");
  Serial.println(digitalRead(SONOFF_RELAY_PIN));
  bool relayState = (digitalRead(SONOFF_RELAY_PIN)==HIGH);
  setRelayState(!relayState);
}

void restart() {
  turnOff();
  ESP.reset();
  delay(1000);
}

void reset() {
  turnOff(); //Turn off relay
  //resetEEpromSettings()
  //reset wifi credentials
  WiFi.disconnect();
  delay(1000);
  ESP.reset();
  delay(1000);
}

void resetEEpromSettings(){
  ZenomatSettings defaults;
  settings = defaults;
  EEPROM.begin(512);
  EEPROM.put(0, settings);
  EEPROM.end();
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(SONOFF_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
    turnOn();
  } else {
    digitalWrite(SONOFF_LED, HIGH);  // Turn the LED off by making the voltage HIGH
    turnOff();
  }

}

void setup() {
  Serial.begin(115200);

  //set led pin as output
  pinMode(SONOFF_LED, OUTPUT);
  // blink 1 sec intervall in AP mode, try to connect
  periodic.attach(0.6, toggleLED);


  const char *hostname = HOSTNAME;

  WiFiManager wifiManager;
  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //timeout - this will quit WiFiManager
  //if it's not configured in 3 minutes, causing a restart
  wifiManager.setConfigPortalTimeout(180);

  //custom params
  EEPROM.begin(512);
  EEPROM.get(0, settings);
  EEPROM.end();

  if (settings.salt != EEPROM_SALT) {
    Serial.println("Invalid settings in EEPROM, trying with defaults");
    ZenomatSettings defaults;
    settings = defaults;
  }

// this does some gymnastics to work around what I guess is a bug in WiFimanager
// To get valid HTML from WiFiManagerParameter, I need to format like this:
// "><br/>Boot state. on/off<br/"
  WiFiManagerParameter custom_boot_state("boot-state", "on/off on boot", settings.bootState, 33, "><br/>Boot state. on/off<br/");
  wifiManager.addParameter(&custom_boot_state);

  Serial.println(settings.bootState);
  Serial.println(settings.mqttHostname);
  Serial.println(settings.mqttPort);
  Serial.println(settings.mqttClientID);
  Serial.println(settings.mqttTopic);
  
  WiFiManagerParameter custom_mqtt_client_id("mqtt-client-id", "Client ID", settings.mqttClientID, 24, "><br/>Unique ID<br/");
  wifiManager.addParameter(&custom_mqtt_client_id);
  
  WiFiManagerParameter custom_mqtt_config_text("<p/><b>MQTT config</b> <br/>No url to disable.<br/>");
  wifiManager.addParameter(&custom_mqtt_config_text);

  WiFiManagerParameter custom_mqtt_hostname("mqtt-hostname", "Hostname", settings.mqttHostname, 33, "><br/>MQTT broker<br/");
  wifiManager.addParameter(&custom_mqtt_hostname);

  WiFiManagerParameter custom_mqtt_port("mqtt-port", "port", settings.mqttPort, 6, "><br/>Port<br/");
  wifiManager.addParameter(&custom_mqtt_port);
/*  
  WiFiManagerParameter custom_mqtt_login_text("<p/>Leave blank if no MQTT login<br/>");
  wifiManager.addParameter(&custom_mqtt_login_text);
  
  WiFiManagerParameter custom_mqtt_username("mqtt-user", "MQTT username", settings.mqttUsername, 16, "><br/>Username<br/");
  wifiManager.addParameter(&custom_mqtt_username);

  WiFiManagerParameter custom_mqtt_password("mqtt-pass", "MQTT password", settings.mqttPassword, 16, "><br/>Password<br/");
  wifiManager.addParameter(&custom_mqtt_password);

  WiFiManagerParameter custom_ota_password_text("<p/>Leave blank if no OTA password<br/>");
  wifiManager.addParameter(&custom_ota_password_text);
  
  WiFiManagerParameter custom_ota_password("ota-pass", "OTA password", settings.otaPassword, 16, "><br/>OTA Password<br/");
  wifiManager.addParameter(&custom_ota_password);
//*/

  WiFiManagerParameter custom_mqtt_topic("mqtt-topic", "Topic", settings.mqttTopic, 33, "><br/>MQTT topic<br/");
  wifiManager.addParameter(&custom_mqtt_topic);

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  if (!wifiManager.autoConnect(hostname)) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }
  
  //save the custom parameters to EEPROM
  if (shouldSaveConfig) {
    Serial.println("Saving config");

    strcpy(settings.bootState, custom_boot_state.getValue());
    strcpy(settings.mqttHostname, custom_mqtt_hostname.getValue());
    strcpy(settings.mqttPort, custom_mqtt_port.getValue());
    strcpy(settings.mqttClientID, custom_mqtt_client_id.getValue());
    strcpy(settings.mqttTopic, custom_mqtt_topic.getValue());
/*
    strcpy(settings.mqttUsername, custom_mqtt_username.getValue());
    strcpy(settings.mqttPassword, custom_mqtt_password.getValue());
    strcpy(settings.otaPassword, custom_ota_password.getValue());
//*/
    
    Serial.println(settings.bootState);
    EEPROM.begin(512);
    EEPROM.put(0, settings);
    EEPROM.end();
  }
    
    //config mqtt
    mqtt.setServer(settings.mqttHostname, atoi(settings.mqttPort));
    mqtt.setCallback(mqttCallback);

    //OTA
    ArduinoOTA.onStart([]() {
      Serial.println("Start OTA");
    });
    ArduinoOTA.onEnd([]() {
      Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.setHostname(settings.mqttClientID);
    //ArduinoOTA.setPassword(settings.otaPassword);
    ArduinoOTA.begin();
  
    //if you get here you have connected to the WiFi
    Serial.println("connected!");
    periodic.detach();
  
    //setup button
    pinMode(SONOFF_BUTTON, INPUT);
    attachInterrupt(SONOFF_BUTTON, toggleState, CHANGE);

  //setup relay
  pinMode(SONOFF_RELAY_PIN, OUTPUT);

   //TODO this should move to last state maybe
  if (strcmp(settings.bootState, "on") == 0) {
    turnOn();
  } else {
    turnOff();
  }

  Serial.println("done setup");
}

void loop() {
//ota loop
  ArduinoOTA.handle();

//mqtt loop
if (!mqtt.connected()) {
    reconnect();
  }
  mqtt.loop();

  long now = millis();
  if (now - lastMsg > 120000) {
    lastMsg = now;
    ++value;
    snprintf (msg, 75, "Ping #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    mqtt.publish("zenomat/chatter", msg);
  }

  switch (cmd) {
    case CMD_WAIT:
      break;
    case CMD_BUTTON_CHANGE:
      int currentState = digitalRead(SONOFF_BUTTON);
      if (currentState != buttonState) {
        if (buttonState == LOW && currentState == HIGH) {
          long duration = millis() - startPress;
          if (duration < 1000) { //less than a sec for toggle relay
            Serial.println("short press - toggle relay");
            toggle();
          } else if (duration < 6000) { //5 sec for reset
            Serial.println("medium press - reset");
            restart();
          } else if (duration < 30000) { //10sec for flush settings
            Serial.println("long press - reset settings");
            reset();
          }
        } else if (buttonState == HIGH && currentState == LOW) {
          startPress = millis();
        }
        buttonState = currentState;
      }
      break;
  }
}
