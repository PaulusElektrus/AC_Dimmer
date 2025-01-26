/* Passwords */
#include "secrets.h"

/* Libraries */
#include <WiFi.h>
#include <InfluxDbClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

/* JSON */
DynamicJsonDocument doc(1024);

#ifdef DATABASE
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);
Point database(DEVICE);
#endif

void setup() 
{
  Serial.begin(115200);
  Serial.println("\n\nPV-Overshoot-Terminator\n\n");

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

  Serial.println("\n\nStarting Overhoot Termination Loop\n\n");
}

void loop() {
  get_data();
  sleep(1);
}

void get_data() {
  HTTPClient http;
  http.setTimeout(1000);
  http.begin(SHELLY_IP);

  int httpCode = http.GET();
  if (httpCode == 200) {
    String payload = http.getString();

    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.print("JSON Deserialization failed: ");
      Serial.println(error.c_str());
    } else {
      JsonObject root = doc.as<JsonObject>();

      String output;
      for (JsonPair kv : root) {
        output += kv.key().c_str();
        output += ": ";
        output += kv.value().as<String>();
        output += "\n";
      }
      Serial.println(output);
    }
  } else {
    Serial.print("HTTP GET failed, code: ");
    Serial.println(httpCode);
  }

  http.end();
}
