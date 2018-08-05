// Libraries
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <BlynkSimpleEsp8266.h>
// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "Your Token From Blynk App";
// Your WiFi credentials.
// Set password to "" for open networks.
#define WIFI_SSID "Your Wifi Service Set Identifier (SSID)"
#define WIFI_PASSPHRASE "Your Wifi Password"
// Adafruit IO
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "Your Adafruit Username"
#define AIO_KEY         "Your Adafruit Key"
// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// Store the MQTT server, client ID, username, and password in flash memory.
const char MQTT_SERVER[] = AIO_SERVER;
// Set a unique MQTT client ID using the AIO key + the date and time the sketch
// was compiled (so this should be unique across multiple devices for a user,
// alternatively you can manually set this to a GUID or other random value).
const char MQTT_CLIENTID[] = AIO_KEY __DATE__ __TIME__;
const char MQTT_USERNAME[] = AIO_USERNAME;
const char MQTT_PASSWORD[] = AIO_KEY;
// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, AIO_SERVERPORT, MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD);
// Setup feed for arrival time
const char ARRIVAL_TIME_FEED[] = AIO_USERNAME "/feeds/arrival-time";
Adafruit_MQTT_Publish arrival_time = Adafruit_MQTT_Publish(&mqtt, ARRIVAL_TIME_FEED);
// Setup feed for number of crashes
const char NUMBER_OF_CRASHES_FEED[] = AIO_USERNAME "/feeds/number-of-crashes";
Adafruit_MQTT_Publish number_of_crashes = Adafruit_MQTT_Publish(&mqtt, NUMBER_OF_CRASHES_FEED);
// Functions
void connect();
void notifyOnButtonPress();
// Pins configuration
const int speakerPin = D3;
const int trigPin = D2;
const int echoPin = D1;
const int buttonPin = D5;
// Global variables
long duration;
int distance;
int safetyDistance;
// Used to deliver data to the adafruit interface
bool emergency = false;
unsigned long currentTime = 0;
unsigned long elapsedTime = 0;
void setup() {
  Serial.begin(9600);
  delay(10);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSPHRASE);
  
  // wait for WiFi connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    delay(10);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  // connect to adafruit io
  connect();
  Blynk.begin(auth, WIFI_SSID, WIFI_PASSPHRASE);
  // Pins setup
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(speakerPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
}
void loop() {
  // Initialize the supersonic sensor.
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Supersonic sensor data
  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;
  safetyDistance = distance;
  // if we are too close to hit the wall
  if (safetyDistance <= 20) {
    // make warning sound
    tone(speakerPin, 1000, 1000);
    
    // ping adafruit io a few times to make sure we remain connected
      if (! mqtt.ping(3)) {
        // reconnect to adafruit io
        if (! mqtt.connected())
          connect();
      }
        // Publish data
        if (! number_of_crashes.publish(1))
          Serial.println("Failed to publish number of crashes");
        else
          Serial.println("Number of crashes published!");
  }
  Blynk.run();
  notifyOnButtonPress();
  delay(500);
}
// Check if the emergency button is pressed and publish data to adafruit.io
void notifyOnButtonPress()
{
  // Invert state, since button is "Active LOW"
  int isButtonPressed = !digitalRead(D5);
  if (isButtonPressed) {
    if (emergency == false)
    {
      emergency = true;
      currentTime = millis();
      // Note:
      //   We allow 1 notification per 5 seconds for now.
      Blynk.notify("EMERGENCY - GRRANDPA JUST PUSH HELP BUTTON!!!");
    }
    else
    {
      elapsedTime = millis() - currentTime;
      //convert from milliseconds to seconds
      elapsedTime = elapsedTime / 1000;
      Serial.println(elapsedTime);
      // ping adafruit io a few times to make sure we remain connected
      if (! mqtt.ping(3)) {
        // reconnect to adafruit io
        if (! mqtt.connected())
          connect();
      }
        // Publish data
        if (! arrival_time.publish((int) elapsedTime))
          Serial.println("Failed to publish arrival time");
        else
          Serial.println("Arrival time published!");
        
        emergency = false;
      }
    }
  }
  // connect to adafruit.io via MQTT
  void connect() {
    Serial.print("Connecting to Adafruit IO... ");
    int8_t ret;
    while ((ret = mqtt.connect()) != 0) {
      switch (ret) {
        case 1: Serial.println("Wrong protocol"); break;
        case 2: Serial.println("ID rejected"); break;
        case 3: Serial.println("Server unavail"); break;
        case 4: Serial.println("Bad user/pass"); break;
        case 5: Serial.println("Not authed"); break;
        case 6: Serial.println("Failed to subscribe"); break;
        default: Serial.println("Connection failed"); break;
      }
      if (ret >= 0) {
        mqtt.disconnect();
      }
      Serial.println("Retrying connection...");
      delay(5000);
    }
    Serial.println("Adafruit IO Connected!");
  }