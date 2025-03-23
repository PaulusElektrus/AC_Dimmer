/* Passwords */
#include "secrets.h"

/* Libraries */
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

/* JSON Library: https://github.com/bblanchon/ArduinoJson */
#include <ArduinoJson.h>

#ifdef DATABASE
#include <InfluxDbClient.h>
#endif

/* Timing */
#define REPORT_TIME 1000
#define UPDATE_TIME 500
/* Debug Mode*/
#undef DEBUG_MODE

float powerMeasured = 0.0;

/* JSON & Etherner */
DynamicJsonDocument jsonDoc(1024);
JsonObject          jsonData;
WiFiClient          client;
HTTPClient          http;

#ifdef DATABASE
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);
Point database(DEVICE);
#endif

void setup() 
{
  Serial.begin(115200);

  setupWLAN();
  setupDatabase();
}

void setupWLAN()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }

  #ifdef DEBUG_MODE
  Serial.print("\n\nW-Lan connected with IP: ");
  Serial.println(WiFi.localIP());
  #endif
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

void loop() {
  getData();
  serialReport();
  writeDatabase();
}

void getData() {
  static unsigned long lastReport = millis();

  if (millis() - lastReport >= UPDATE_TIME)
  {
    http.setTimeout(500);
    http.begin(client, SHELLY_IP);
    int httpCode = http.GET();

    if (httpCode == 200) {
      String payload = http.getString();
      DeserializationError error = deserializeJson(jsonDoc, payload);

      if (error) 
      {
        #ifdef DEBUG_MODE
        Serial.print("JSON Deserialization failed: ");  
        Serial.println(error.c_str());
        #endif
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
      #ifdef DEBUG_MODE
      Serial.print("HTTP GET failed, code: ");
      Serial.println(httpCode);
      #endif
      powerMeasured = 0.0;
    }
    Serial.println(powerMeasured);
    http.end();
    lastReport = millis();
  }
}

void serialReport()
{
  #ifdef DEBUG_MODE
  static unsigned long lastReport = millis();

  if (millis() - lastReport >= REPORT_TIME)
  {
    String output;
    output += "JSON:\n";
    for (JsonPair kv : jsonData) {
      output += kv.key().c_str();
      output += ": ";
      output += kv.value().as<String>();
      output += "\n";
    }
    Serial.println(output);
    lastReport = millis();
  }
  #endif
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
