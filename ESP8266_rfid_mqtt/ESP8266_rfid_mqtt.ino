/*
    ESP8266 RFID Reader - MQTT Sender
    Copyright 2016 Christian Moll <christian@chrmoll.de>
    Copyright 2018 Andreas Gremm

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Dieses Programm ist Freie Software: Sie können es unter den Bedingungen
    der GNU General Public License, wie von der Free Software Foundation,
    Version 3 der Lizenz oder (nach Ihrer Wahl) jeder neueren
    veröffentlichten Version, weiterverbreiten und/oder modifizieren.

    Dieses Programm wird in der Hoffnung, dass es nützlich sein wird, aber
    OHNE JEDE GEWÄHRLEISTUNG, bereitgestellt; sogar ohne die implizite
    Gewährleistung der MARKTFÄHIGKEIT oder EIGNUNG FÜR EINEN BESTIMMTEN ZWECK.
    Siehe die GNU General Public License für weitere Details.

    Sie sollten eine Kopie der GNU General Public License zusammen mit diesem
    Programm erhalten haben. Wenn nicht, siehe <http://www.gnu.org/licenses/>.
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

#include <PubSubClient.h>

#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN D4
#define RST_PIN D3

// Remove this include which sets the below constants to my own conveniance
#include </Users/andreas/Documents/git-github/non-git-local-includes/ESP8266_rfid_mqtt_local.h>

// The following constants need to be set in the program
/*
const char* host = "HOSTNAME";
const char* ssid = "SSID of WlAN";
const char* password = "PASSWORD to conect to ssid";

const char* brocker = "HOSTNAME or IP of MQTT brocker";
const char* mqttUser = "MQTT User Name";
const char* mqttPass = "MQTT Password";
const char* mqttClientId = "MQTT Client Name";
*/

int autoAlarmPin = D1;
int wohnzimmerAlarmPin = D2;
int irPin = A0;
int irValue;
bool wohnzimmerAlarm = false;
bool autoAlarm = false;
int motion;
bool buzzer = true;
long mqttConnectionLost = 0;

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

WiFiClient wifi;
PubSubClient mqtt(wifi);

MFRC522 rfid(SS_PIN, RST_PIN);

/*
void wificonnect();
void mqttconnect();
 */

void setup(void){
  pinMode(autoAlarmPin, OUTPUT);
  pinMode(wohnzimmerAlarmPin, OUTPUT);
  digitalWrite(autoAlarmPin, HIGH);
  digitalWrite(wohnzimmerAlarmPin, HIGH);
  pinMode(irPin, INPUT);


  SPI.begin();
  rfid.PCD_Init();
 
  Serial.begin(115200);
  Serial.println();
  Serial.println("Booting Sketch...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  mqtt.setServer(brocker, 1883);
  mqtt.setCallback(messageReceived);

  wificonnect();
  digitalWrite(autoAlarmPin, LOW);

  mqttconnect();
  digitalWrite(wohnzimmerAlarmPin, LOW);

  MDNS.begin(host);

  //Attach handles for different pages.
  httpUpdater.setup(&httpServer);

  httpServer.on("/", handleRoot);
  httpServer.on("/status",handleStatus);

  httpServer.begin();

  MDNS.addService("http", "tcp", 80);
  Serial.print("MFRC522 software version = ");
  Serial.println(rfid.PCD_ReadRegister(rfid.VersionReg),HEX);

  Serial.println("Up and running!");
}

void loop(void){
  if(!mqtt.connected()) {
    Serial.println("MQTT connection lost.");
    mqttConnectionLost++;
    mqttconnect();
  }

  httpServer.handleClient();
  mqtt.loop();
  handleRFID();
  handleIR();
}

void wificonnect() {
  while(WiFi.waitForConnectResult() != WL_CONNECTED){
    WiFi.begin(ssid, password);
    Serial.println("WiFi failed, retrying.");
    delay(500);
  }

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("\n WiFi connected!");
}

void mqttconnect() {
  while (!mqtt.connect(mqttClientId, mqttUser, mqttPass, "clientstatus/RFIDReader",1,1,"OFFLINE")) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\n MQTT connected!");
  mqtt.subscribe("alarm/wohnzimmer/motion");
  mqtt.subscribe("alarm/auto/motion");
  mqtt.publish("clientstatus/RFIDReader", "ONLINE");
}

void handleRoot() {
 // httpServer.send(200, "text/plain", "It works!!!");
  httpServer.send(200, "text/html", "<html><head></head><body><a href='/status'>Status</a><br /><a href='/update'>Update</a></body></html>");
}

void handleStatus() {
  bool rfidStatus = rfid.PCD_PerformSelfTest();
  rfid.PCD_Init();
  char theStatus[80];
  sprintf(theStatus, "RFID-Selftest: %s\nMQTT-Reconnect: %d\n",rfidStatus ? "True" : "False",mqttConnectionLost);
  httpServer.send(200, "text/plain", theStatus);
}

void handleIR() {
  irValue = analogRead(irPin);
  if (irValue > 100) {
    if (wohnzimmerAlarm and buzzer){
      buzzer = false;
      mqtt.publish("buzzer/wohnzimmer", "2");
    }
  } else {
    buzzer = true;
  }
}
void messageReceived(char * topic, unsigned char * payload, unsigned int length) {
/*
  Serial.print("incoming: ");
  Serial.print(topic);
  Serial.print(" - ");
  for (byte i = 0; i < length; i++) {
     Serial.print((const char)payload[i]);
  }
*/
  if (strcmp(topic,"alarm/wohnzimmer/motion") == 0) {
    if (strncmp((const char *)payload, "False", length) == 0) {
      digitalWrite(wohnzimmerAlarmPin, LOW);
      wohnzimmerAlarm = false;
    }
    if (strncmp((const char *)payload, "True", length) == 0) {
      digitalWrite(wohnzimmerAlarmPin, HIGH);
      wohnzimmerAlarm = true;
    }
  }

  if (strcmp(topic,"alarm/auto/motion") == 0) {
    if (strncmp((const char *)payload, "False", length) == 0) {
      digitalWrite(autoAlarmPin, LOW);
      autoAlarm = false;
    }
    if (strncmp((const char *)payload, "True", length) == 0) {
      digitalWrite(autoAlarmPin, HIGH);
      autoAlarm = true;
    }
  }
 // Serial.println();
}

void blinkLed(int led, int nr, bool ledstatus) {
  if (nr == 0) {
    return;
  }
  nr--;
  digitalWrite(led, !ledstatus);
  delay(200);
  digitalWrite(led, ledstatus);
  delay(200);
  blinkLed(led, nr, ledstatus);  
}

void handleRFID() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;
  String rfiduid = printHex(rfid.uid.uidByte, rfid.uid.size);
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  blinkLed(wohnzimmerAlarmPin, 3, wohnzimmerAlarm);
  
//  Serial.println(rfiduid);
  mqtt.publish("rfid_reader/uid", rfiduid.c_str());
  if (strcmp(rfiduid.c_str(),"c5d54c73") == 0) {
    if (wohnzimmerAlarm) {
      mqtt.publish("alarm/wohnzimmer/motion",(const uint8_t *)"False",5,true);
    } else {
      mqtt.publish("alarm/wohnzimmer/motion",(const uint8_t *)"True",4,true);
    }
  }
  if (strcmp(rfiduid.c_str(),"c6ebfe1f") == 0) {
    if (autoAlarm) {
      mqtt.publish("alarm/auto/motion",(const uint8_t *)"False",5,true);
    } else {
      mqtt.publish("alarm/auto/motion",(const uint8_t *)"True",4,true);
    }
  }
}

double mapDouble(double x, double in_min, double in_max, double out_min, double out_max)
{
  double temp = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  temp = (int) (4*temp + .5);
  return (double) temp/4;
}

String printHex(byte *buffer, byte bufferSize) {
  String id = "";
  for (byte i = 0; i < bufferSize; i++) {
    id += buffer[i] < 0x10 ? "0" : "";
    id += String(buffer[i], HEX);
  }
  return id;
}
