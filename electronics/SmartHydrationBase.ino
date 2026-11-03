#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include "HX711.h"
#include <WiFi.h>
#include <PubSubClient.h>

// =====================================================
// PIN DEFINITIONS
// =====================================================

#define HX711_DT 18
#define HX711_SCK 19

#define DHT_PIN 15
#define DHT_TYPE DHT22

#define OLED_SDA 21
#define OLED_SCL 22

#define RGB_R 25
#define RGB_G 26
#define RGB_B 27

#define CAL_BUTTON 32

// =====================================================
// WIFI AND MQTT SETTINGS
// =====================================================

const char* ssid = "Wokwi-GUEST";
const char* password = "";

const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

// Keep this topic identical when configuring Unity.
const char* mqtt_topic =
  "smart_hydration_base/yuvasri/sensor_data";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// =====================================================
// OBJECTS
// =====================================================

HX711 scale;
DHT dht(DHT_PIN, DHT_TYPE);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  -1
);

// =====================================================
// CALIBRATION
// =====================================================

float calibrationFactor = 0.410000;
float emptyWeight = 280.37;

// =====================================================
// WATER VARIABLES
// =====================================================

float currentWeight = 0;
float currentWater = 0;

float previousWater = 0;
float consumedMl = 0;

bool waterInitialized = false;

// =====================================================
// TEMPERATURE / GOAL
// =====================================================

float temperatureC = 25.0;

float defaultGoalMl = 2500;
float userGoalMl = 0;

float effectiveGoalMl = 2500;
float remainingMl = 2500;
float percentComplete = 0;

// =====================================================
// TIME
// =====================================================

unsigned long startMillis = 0;
int elapsedMinutes = 0;

// 1 real second = 5 simulated minutes
const int ACTIVE_MINUTES = 960;

// =====================================================
// STATUS
// =====================================================

String pacingStatus = "SIGNIFICANTLY BEHIND";

// =====================================================
// BUTTON
// =====================================================

bool lastButtonState = HIGH;

// =====================================================
// RGB LED - COMMON ANODE
// =====================================================

void setRGB(int r, int g, int b)
{
  analogWrite(RGB_R, 255 - r);
  analogWrite(RGB_G, 255 - g);
  analogWrite(RGB_B, 255 - b);
}

// =====================================================
// TEMPERATURE GOAL
// =====================================================

void calculateGoal()
{
  if (temperatureC < 20)
  {
    defaultGoalMl = 2000;
  }
  else if (temperatureC <= 28)
  {
    defaultGoalMl = 2500;
  }
  else
  {
    defaultGoalMl = 3200;
  }

  if (userGoalMl > 0)
  {
    effectiveGoalMl = userGoalMl;
  }
  else
  {
    effectiveGoalMl = defaultGoalMl;
  }
}

// =====================================================
// PACING STATUS
// =====================================================

void calculatePacing()
{
  if (effectiveGoalMl <= 0)
  {
    pacingStatus = "SIGNIFICANTLY BEHIND";
    return;
  }

  float expectedMl =
    effectiveGoalMl *
    ((float)elapsedMinutes / ACTIVE_MINUTES);

  if (expectedMl < 1)
  {
    pacingStatus = "ON TRACK";
    return;
  }

  float ratio = consumedMl / expectedMl;

  if (ratio >= 0.9 && ratio <= 1.3)
  {
    pacingStatus = "ON TRACK";
  }
  else if (ratio >= 0.6 && ratio < 0.9)
  {
    pacingStatus = "SLIGHTLY BEHIND";
  }
  else if (ratio < 0.6)
  {
    pacingStatus = "SIGNIFICANTLY BEHIND";
  }
  else
  {
    pacingStatus = "TOO FAST";
  }
}

// =====================================================
// RGB STATUS
// =====================================================

void updateRGB()
{
  if (pacingStatus == "ON TRACK")
  {
    setRGB(0, 255, 0);
  }
  else if (pacingStatus == "SLIGHTLY BEHIND")
  {
    setRGB(255, 180, 0);
  }
  else if (pacingStatus == "SIGNIFICANTLY BEHIND")
  {
    setRGB(255, 0, 0);
  }
  else if (pacingStatus == "TOO FAST")
  {
    setRGB(0, 0, 255);
  }
  else
  {
    setRGB(0, 0, 0);
  }
}

// =====================================================
// OLED NORMAL DASHBOARD
// =====================================================

void updateOLED()
{
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.print("SMART HYDRATION");

  display.setCursor(0, 10);
  display.print("Goal: ");
  display.print(effectiveGoalMl, 0);
  display.print(" mL");

  display.setCursor(0, 20);
  display.print("Had: ");
  display.print(consumedMl, 0);
  display.print(" mL");

  remainingMl = effectiveGoalMl - consumedMl;

  if (remainingMl < 0)
  {
    remainingMl = 0;
  }

  display.setCursor(0, 30);
  display.print("Remain: ");
  display.print(remainingMl, 0);
  display.print(" mL");

  if (effectiveGoalMl > 0)
  {
    percentComplete =
      (consumedMl / effectiveGoalMl) * 100.0;
  }
  else
  {
    percentComplete = 0;
  }

  if (percentComplete > 100)
  {
    percentComplete = 100;
  }

  if (percentComplete < 0)
  {
    percentComplete = 0;
  }

  display.setCursor(0, 40);
  display.print("Progress: ");
  display.print(percentComplete, 0);
  display.print("%");

  display.setCursor(0, 50);

  if (pacingStatus == "ON TRACK")
  {
    display.print("Status: ON TRACK");
  }
  else if (pacingStatus == "SLIGHTLY BEHIND")
  {
    display.print("Status: SLIGHTLY");
  }
  else if (pacingStatus == "TOO FAST")
  {
    display.print("Status: TOO FAST");
  }
  else
  {
    display.print("Status: BEHIND");
  }

  display.display();
}

// =====================================================
// OLED CALIBRATION SCREEN
// =====================================================

void showCalibrationScreen()
{
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(20, 10);
  display.println("CALIBRATING...");

  display.setCursor(15, 25);
  display.println("Please keep bottle");

  display.setCursor(25, 40);
  display.println("EMPTY & STILL");

  display.display();
}

// =====================================================
// OLED SAVED SCREEN
// =====================================================

void showCalibrationSaved()
{
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(15, 10);
  display.println("CALIBRATION");

  display.setCursor(30, 25);
  display.println("SAVED!");

  display.setCursor(10, 40);
  display.print("Empty: ");
  display.print(emptyWeight, 1);
  display.println(" g");

  display.display();
  delay(1500);
}

// =====================================================
// CALIBRATION
// =====================================================

void calibrateBottle()
{
  Serial.println();
  Serial.println("================================");
  Serial.println("BUTTON PRESSED!");
  Serial.println("CALIBRATING...");
  Serial.println("Keep bottle EMPTY and STILL");
  Serial.println("================================");

  showCalibrationScreen();
  delay(1000);

  if (scale.is_ready())
  {
    float newWeight = scale.get_units(20);

    if (newWeight < 0)
    {
      newWeight = 0;
    }

    emptyWeight = newWeight;

    Serial.print("Empty bottle weight = ");
    Serial.print(emptyWeight, 2);
    Serial.println(" g");

    Serial.println("CALIBRATION SAVED!");

    consumedMl = 0;
    previousWater = 0;
    waterInitialized = false;

    showCalibrationSaved();
  }
  else
  {
    Serial.println("ERROR: HX711 NOT READY!");

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    display.setCursor(10, 20);
    display.println("HX711 ERROR!");

    display.setCursor(10, 35);
    display.println("Check sensor");

    display.display();
    delay(1500);
  }
}

// =====================================================
// READ BUTTON
// =====================================================

void checkButton()
{
  bool buttonState = digitalRead(CAL_BUTTON);

  if (lastButtonState == HIGH &&
      buttonState == LOW)
  {
    calibrateBottle();
  }

  lastButtonState = buttonState;
}

// =====================================================
// READ HX711
// =====================================================

void readWeight()
{
  if (!scale.is_ready())
  {
    return;
  }

  currentWeight = scale.get_units(10);

  if (currentWeight < 0)
  {
    currentWeight = 0;
  }

  currentWater = currentWeight - emptyWeight;

  if (currentWater < 0)
  {
    currentWater = 0;
  }

  if (!waterInitialized)
  {
    previousWater = currentWater;
    waterInitialized = true;
    return;
  }

  float difference = previousWater - currentWater;

  // WATER CONSUMED
  if (difference > 5)
  {
    consumedMl += difference;

    Serial.println();
    Serial.println("WATER CONSUMED!");

    Serial.print("Amount consumed = ");
    Serial.print(difference, 2);
    Serial.println(" mL");

    Serial.print("TOTAL CONSUMED = ");
    Serial.print(consumedMl, 2);
    Serial.println(" mL");
  }

  // REFILL DETECTED
  float refillDifference = currentWater - previousWater;

  if (refillDifference > 5)
  {
    Serial.println();
    Serial.println("REFILL DETECTED!");

    Serial.print("Water added = ");
    Serial.print(refillDifference, 2);
    Serial.println(" mL");
  }

  previousWater = currentWater;
}

// =====================================================
// READ TEMPERATURE
// =====================================================

void readTemperature()
{
  float temp = dht.readTemperature();

  if (!isnan(temp))
  {
    temperatureC = temp;
  }

  calculateGoal();
}

// =====================================================
// SERIAL DASHBOARD
// =====================================================

void printDashboard()
{
  Serial.println("--------------------------------");

  Serial.print("Temperature: ");
  Serial.print(temperatureC, 1);
  Serial.println(" C");

  Serial.print("Goal: ");
  Serial.print(effectiveGoalMl, 0);
  Serial.println(" mL");

  Serial.print("Bottle weight: ");
  Serial.print(currentWeight, 2);
  Serial.println(" g");

  Serial.print("Water in bottle: ");
  Serial.print(currentWater, 2);
  Serial.println(" mL");

  Serial.print("Consumed: ");
  Serial.print(consumedMl, 2);
  Serial.println(" mL");

  Serial.print("Remaining: ");
  Serial.print(remainingMl, 2);
  Serial.println(" mL");

  Serial.print("Progress: ");
  Serial.print(percentComplete, 1);
  Serial.println("%");

  Serial.print("Simulated time: ");
  Serial.print(elapsedMinutes);
  Serial.println(" min");

  Serial.print("STATUS: ");
  Serial.println(pacingStatus);

  Serial.println("--------------------------------");
}

// =====================================================
// MQTT PUBLISH SENSOR DATA
// =====================================================

void publishSensorData()
{
  // Recalculate values before sending.
  remainingMl = effectiveGoalMl - consumedMl;

  if (remainingMl < 0)
  {
    remainingMl = 0;
  }

  if (effectiveGoalMl > 0)
  {
    percentComplete =
      (consumedMl / effectiveGoalMl) * 100.0;
  }
  else
  {
    percentComplete = 0;
  }

  if (percentComplete > 100)
  {
    percentComplete = 100;
  }

  if (percentComplete < 0)
  {
    percentComplete = 0;
  }

  // JSON payload for Unity.
  // Water-level percentage is omitted because
  // maximum bottle capacity is not yet defined.
  String payload = "{";

  payload += "\"currentWaterMl\":";
  payload += String(currentWater, 2);

  payload += ",\"consumedMl\":";
  payload += String(consumedMl, 2);

  payload += ",\"temperatureC\":";
  payload += String(temperatureC, 1);

  payload += ",\"goalMl\":";
  payload += String(effectiveGoalMl, 0);

  payload += ",\"remainingMl\":";
  payload += String(remainingMl, 2);

  payload += ",\"percentComplete\":";
  payload += String(percentComplete, 1);

  payload += ",\"elapsedMinutes\":";
  payload += String(elapsedMinutes);

  payload += ",\"pacingStatus\":\"";
  payload += pacingStatus;
  payload += "\"";

  payload += "}";

  if (mqttClient.connected())
  {
    bool published = mqttClient.publish(
      mqtt_topic,
      payload.c_str()
    );

    if (published)
    {
      Serial.print("MQTT SENT: ");
      Serial.println(payload);
    }
    else
    {
      Serial.println("MQTT publish failed.");
    }
  }
}

// =====================================================
// WIFI CONNECTION
// =====================================================

void connectWiFi()
{
  Serial.print("Connecting to Wokwi WiFi");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// =====================================================
// MQTT CONNECTION
// =====================================================

void reconnectMQTT()
{
  while (!mqttClient.connected())
  {
    Serial.print("Connecting to MQTT broker...");

    String clientId = "SmartHydrationBase-";
    clientId += String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str()))
    {
      Serial.println("MQTT connected!");
      Serial.print("Publishing topic: ");
      Serial.println(mqtt_topic);
    }
    else
    {
      Serial.print("MQTT connection failed, state = ");
      Serial.println(mqttClient.state());

      delay(2000);
    }
  }
}

// =====================================================
// SETUP
// =====================================================

void setup()
{
  Serial.begin(115200);

  pinMode(CAL_BUTTON, INPUT_PULLUP);

  pinMode(RGB_R, OUTPUT);
  pinMode(RGB_G, OUTPUT);
  pinMode(RGB_B, OUTPUT);

  setRGB(0, 0, 0);

  dht.begin();

  scale.begin(HX711_DT, HX711_SCK);
  scale.set_scale(calibrationFactor);

  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        0x3C))
  {
    Serial.println("OLED FAILED!");
  }
  else
  {
    Serial.println("OLED READY");
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(10, 15);
  display.println("SMART HYDRATION");

  display.setCursor(25, 30);
  display.println("BASE");

  display.setCursor(25, 45);
  display.println("STARTING...");

  display.display();

  delay(2000);

  startMillis = millis();

  Serial.println();
  Serial.println("================================");
  Serial.println(" SMART HYDRATION BASE");
  Serial.println(" MQTT INTEGRATION");
  Serial.println("================================");

  // Initialize MQTT settings.
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setBufferSize(512);

  // Connect to Wokwi virtual Wi-Fi.
  connectWiFi();

  // Connect to the MQTT broker.
  reconnectMQTT();
}

// =====================================================
// LOOP
// =====================================================

void loop()
{
  // Keep network connections active.
  if (WiFi.status() != WL_CONNECTED)
  {
    connectWiFi();
  }

  if (!mqttClient.connected())
  {
    reconnectMQTT();
  }

  mqttClient.loop();

  // BUTTON
  checkButton();

  // TEMPERATURE
  readTemperature();

  // WEIGHT AND WATER
  readWeight();

  // SIMULATED TIME
  // 1 real second = 5 simulated minutes.
  unsigned long elapsedMillis = millis() - startMillis;
  elapsedMinutes = elapsedMillis / 200;

  // PACING
  calculatePacing();

  // RGB
  updateRGB();

  // OLED
  updateOLED();

  // SERIAL DASHBOARD AND MQTT DATA
  static unsigned long lastPublish = 0;

  if (millis() - lastPublish >= 2000)
  {
    lastPublish = millis();

    printDashboard();
    publishSensorData();
  }

  delay(100);
}
