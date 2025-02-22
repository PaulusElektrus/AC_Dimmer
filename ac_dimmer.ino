/* Passwords */
#include "secrets.h"

/* Libraries */
#include <WiFi.h>
#include <InfluxDbClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <RBDdimmer.h>

#define VENTILATOR_RELAIS     17
#define HEATER_RELAIS         16
#define ZEROCROSS_PIN         25
#define DIMMER_PIN            26

#define SMALL_POWER_THRESHOLD -15.0
#define BIG_POWER_THRESHOLD   50.0
#define VENTILATOR_POWER      -25.0
#define HEATER_POWER          -30.0
#define INSTANT_OFF           500.0
#define MAX_HEATER_SET        95
#define DIMMER_DELAY          100
#define REPORT_TIME           1

enum heaterStates
{
  OFF_STATE, 
  VENTILATOR,
  HEATER,
  ERROR
};

/* Global Variables */
heaterStates  state = OFF_STATE;
float         powerMeasured = 0.0;
int           powerSet      = 1;

bool          ventilatorState = OFF;
bool          heaterState     = OFF;

/* JSON */
DynamicJsonDocument jsonDoc(1024);
JsonObject          jsonData;
HTTPClient          http;

/* Dimmer */
dimmerLamp dimmer(DIMMER_PIN, ZEROCROSS_PIN);

#ifdef DATABASE
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);
Point database(DEVICE);
#endif

void setup() 
{
  Serial.begin(115200);
  Serial.println("\n\nPV-Overshoot-Terminator\n\n");

  setupWLAN();
  setupDatabase();
  setupPinout();

  Serial.println("\n\nStarting Overhoot Termination Loop\n\n");
}

void setupWLAN()
{
  Serial.print("Connect W-Lan: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }

  Serial.print("\n\nW-Lan connected with IP: ");
  Serial.println(WiFi.localIP());
}

void setupDatabase()
{
#ifdef DATABASE
  database.addTag("device", DEVICE);
  database.addTag("SSID", WiFi.SSID());

  if (client.validateConnection()) {
    Serial.print("\n\nConnected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("\n\nInfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
#endif
}

void setupPinout()
{
  pinMode(VENTILATOR_RELAIS, OUTPUT);
  pinMode(HEATER_RELAIS, OUTPUT);
  dimmer.begin(NORMAL_MODE, OFF);
  ventilatorOff();
}

void loop() {
  getData();
  setHeater();
  serialReport();
  writeDatabase();
  delay(500);
}

void getData() {
  http.setTimeout(500);
  http.begin(SHELLY_IP);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    DeserializationError error = deserializeJson(jsonDoc, payload);

    if (error) 
    {
      Serial.print("JSON Deserialization failed: ");  
      Serial.println(error.c_str());
      powerMeasured = 0.0;
    } 
    else 
    {
      jsonData = jsonDoc.as<JsonObject>();
      powerMeasured = jsonData["total_act_power"];
    }
  } 
  else 
  {
    Serial.print("HTTP GET failed, code: ");
    Serial.println(httpCode);
    powerMeasured = 0.0;
  }

  http.end();
}

void setHeater()
{
  if (powerMeasured > INSTANT_OFF)
  {
    ventilatorOff();
  }

  switch (state)
  {
  case OFF_STATE:
    if (powerMeasured < VENTILATOR_POWER)
    {
      ventilatorOn();
    }
    break;
  
  case  VENTILATOR:
    if (powerMeasured > SMALL_POWER_THRESHOLD)
    {
      ventilatorOff();
    }
    else if (powerMeasured < HEATER_POWER)
    {
      heaterOn();
    }
    break;

  case HEATER:
    if (powerMeasured > SMALL_POWER_THRESHOLD)
    {
      if (powerSet <= 1)
      {
        heaterOff();
        state = VENTILATOR;
        break;
      }
      if (powerMeasured > BIG_POWER_THRESHOLD)
      {
        if (powerSet <= 5)
        {
          ventilatorOff();
          break;
        }
        powerSet -= 5;
        dimmer.setPower(powerSet);
        break;
      }
      powerSet--;
      dimmer.setPower(powerSet);
    }
    else if (powerMeasured < HEATER_POWER)
    {
      if (powerSet < MAX_HEATER_SET)
      {
        powerSet++;
        dimmer.setPower(powerSet);
      }
    }
    break;

  case ERROR:

  default:
    ventilatorOff();
    break;
  }
}

void serialReport()
{
  String output;
  /* output += "JSON:\n"
  for (JsonPair kv : jsonData) {
    output += kv.key().c_str();
    output += ": ";
    output += kv.value().as<String>();
    output += "\n";
  } */
  output += "\nActual Power: ";
  output += powerMeasured;
  output += "\nVentilator State: ";
  output += ventilatorState;
  output += "\nHeater State: ";
  output += heaterState;
  output += "\nPower Set: ";
  output += powerSet;
  output += " %\n";
  Serial.println(output);
}

void ventilatorOn()
{
  digitalWrite(VENTILATOR_RELAIS, LOW);
  ventilatorState = ON;
  state = VENTILATOR;
}

void ventilatorOff()
{
  heaterOff();
  digitalWrite(VENTILATOR_RELAIS, HIGH);
  ventilatorState = OFF;
  state = OFF_STATE;
}

void heaterOn()
{
  ventilatorOn();
  digitalWrite(HEATER_RELAIS, LOW);
  delay(DIMMER_DELAY);
  powerSet = 1;
  dimmer.setPower(powerSet);
  dimmer.setState(ON);
  heaterState = ON;
  state = HEATER;
}

void heaterOff()
{
  dimmer.setState(OFF);
  delay(DIMMER_DELAY);
  powerSet = 1;
  dimmer.setPower(powerSet);
  digitalWrite(HEATER_RELAIS, HIGH);
  heaterState = OFF;
}

void writeDatabase()
{
  #ifdef DATABASE
  database.clearFields();

  for (JsonPair kv : jsonData) {
    database.addField(kv.key().c_str(), kv.value().as<float>());
  }
  
  // Write point
  if (!client.writePoint(database)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }
  #endif
}
