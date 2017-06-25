# Zenomat
__MQTT__ on __Sonoff WiFi switch__, with __captive config portal__.  
Saves config to EEPROM. Manages modes through onboard button. No added sugar.  

Version compatibility problems between different existing components forced me to commit Zenomat.  
I took [tzapus code](https://github.com/tzapu, tzapus code) and ran with it. Wifimanager and Sonoffboilerplate. 


__Zenomat__ relies on these components. Versions stated is what I used when writing and compiling the latest zenomat.ino, so that should work for you too. Other versions might also work fresh out of box, or with minor tweaking.
 * The Sonoff Wi-Fi smart switch hardware.
 * A USB to UART cable for flashing. Pinout below.
 * ArduinoIDE (v.1.8.3).
 * ESP8266 package from the ESP8266community in the ArduinoIDE boards manager (v.2.3.0).
 * WiFi Manager lib from tzapu available through Arduino IDEs Library manager. (v.0.12.0).
 * PubSub lib from Nick O'Leary through Library manager (v.2.6.0).


## Pinout
__Mains *or* USB__ Unplug your USB prog cable before testing with mains! Screw this up, and you *will* be sorry!!
There are probably several different versions of the board, but is what I got off of eBay. The unpopulated and unnamed header next to the button on the PCB is a UART. Pin one is a square on the PCB. So, starting with number 1, the pins are:
 * VCC (3.3v)
 * RX
 * TX
 * GND
 * GPIO 14

Pin 12 on the ESP8266 controls the relay. Pin 0 is the button, 13 is the status LED, and the extra pin on the Sonoff header is pin 14.


## How it works
Hold the button down while powering on to enter bootloader for flashing, or uploading through ArduinoIDE. When you power it up for the first time, it creates the "Zenomat" access point. Connect to it via WiFi, and a captive portal appears allowing you to configure the device. After configuring WiFi and MQTT, the Zenomat will connect with the broker via WiFi on boot. Use the button to enter config mode again when needed.

You can set:
 * WiFi Network. SSID and password.
 * Boot state. Start with relay On or Off.
 * MQTT Broker.
 * MQTT port.
 * ~~Username~~
 * ~~Password~~
 * Unique ID.
 * MQTT topic.
 
 ## Button:
  * Press less than a sec, toggle relay and publish update to topic.
  * Between 1 and 6 sec, turns off relay and resets the ESP.
  * Between 6 and 30 sec, turns off relay and resets WiFi settings then starts the captive portal.  



*Good luck with your project!  
73 de LB3MG  
Bernt-Egil*
