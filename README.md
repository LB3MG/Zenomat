# Zenomat
__MQTT__ on __Sonoff WiFi switch__, with __captive config portal__.  
Saves config to EEPROM. Manages modes through onboard button. No added sugar.  

Version compatibility problems between different existing components forced me to commit Zenomat.  

__Zenomat__ relies on.
 * I used ArduinoIDE v.1.8.3
 * ESP8266 package from the ESP8266community in the ArduinoIDE boards manager (I used v.2.3.0).
 * WiFi Manager lib from tzapu available through Arduino IDEs Library manager. (v.0.12.0).
 * PubSub lib from Nick O'Leary through Library manager (v.2.6.0).  
 
## How it works
Hold the button down while powering on to enter bootloader for flashing, or uploading through ArduinoIDE. When you power it up for the first time, it creates the "Zenomat" access point. Connect to it via WiFi, and a captive portal appears allowing you to configure the device. 

You can set:
 * WiFi Network. SSID and password.
 * Boot state. Start with relay On or Off.
 * MQTT Broker.
 * MQTT port.
 * ~~Username~~
 * ~~Password~~
 * Unique ID.
 * MQTT topic.
 
 Pin 12 on the ESP8266 controls the relay. Pin 0 is the button, 13 is the status LED, and the extra pin on the Sonoff header is pin 14.


Also extra thanks to GIThub user tzapu for SonoffBoilerplate, and the WiFi lib. 
