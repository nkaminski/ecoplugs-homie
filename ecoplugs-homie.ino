/* EcoPlugs_ng: A custom firmware for enabling ESP8266 based
 * smart plugs utilizing the EcoPlugs design to be operated over MQTT. 
 * 
 * Tested units:
 * Woods "Wi-On" models 50049 and 50050
 * 
 * Copyright 2018 Nash Kaminski, released under MIT license.
 * 
 * Portions of this file from homie-esp8266(https://github.com/marvinroger/homie-esp8266)
 * Copyright 2015 Marvin Roger, released under MIT license
 */

#include <Homie.h>

#define DEVICE_BRAND "EcoPlugs_ng"
#define DEVICE_FIRMWARE "1.0.0"
const unsigned int PIN_LED = 2;
const unsigned int PIN_BUTTON = 13;
const unsigned int PIN_RELAY = 15;

unsigned long buttonDownTime = 0;
byte lastButtonState = 1;
byte buttonPressHandled = 0;

HomieNode switchNode("power", "switch");

void onHomieEvent(const HomieEvent& event) {
  if (event.type == HomieEventType::MQTT_READY){
    byte state = digitalRead(PIN_RELAY);
    switchNode.setProperty("on").send(state ? "true" : "false");
  }
}

bool switchOnHandler(HomieRange range, String value) {
  if (value != "true" && value != "false") return false;
  bool on = (value == "true");
  digitalWrite(PIN_RELAY, on ? HIGH : LOW);
  switchNode.setProperty("on").send(value);
  Homie.getLogger() << "Relay is " << (on ? "on" : "off") << endl;

  return true;
}

void toggleRelay() {
  bool on = digitalRead(PIN_RELAY) == HIGH;
  digitalWrite(PIN_RELAY, on ? LOW : HIGH);
  switchNode.setProperty("on").send(on ? "false" : "true");
  Homie.getLogger() << "Relay is " << (on ? "off" : "on") << endl;
}

void loopHandler() {
  byte buttonState = digitalRead(PIN_BUTTON);
  if ( buttonState != lastButtonState ) {
    if (buttonState == LOW) {
      buttonDownTime     = millis();
      buttonPressHandled = 0;
    }
    else {
      unsigned long dt = millis() - buttonDownTime;
      if ( dt >= 90 && dt <= 900 && buttonPressHandled == 0 ) {
        toggleRelay();
        buttonPressHandled = 1;
      }
    }
    lastButtonState = buttonState;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_BUTTON, INPUT);
  digitalWrite(PIN_RELAY, LOW);

  //Wait for the button to be released
  while(digitalRead(PIN_BUTTON) == LOW);
  
  Homie_setBrand(DEVICE_BRAND);
  Homie_setFirmware(DEVICE_BRAND, DEVICE_FIRMWARE);
  Homie.setLedPin(PIN_LED, LOW).setResetTrigger(PIN_BUTTON, LOW, 5000);
  Homie.onEvent(onHomieEvent);
  
  switchNode.advertise("on").settable(switchOnHandler);

  Homie.setup();
}

void loop() {
  Homie.loop();
  loopHandler();
}
