#include <HX711_ADC.h>
#include <WiFi.h>  // Include the WiFi library

// Network credentials
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// Pins for HX711 load cell
const int HX711_dout = 4;
const int HX711_sck = 5;

// Pins for HC-SR04 distance sensor
const int trigPin = 9;
const int echoPin = 10;

// Pins for MQ-4 gas sensor
const int MQ4_AO_Pin = A0;
const int MQ4_DO_Pin = 8;

// HX711 load cell
HX711_ADC LoadCell(HX711_dout, HX711_sck);

const int calVal_eepromAdress = 0;
unsigned long t = 0;

// WiFi server
WiFiServer server(80);

void setup() {
  Serial.begin(57600);

  // Initialize HX711 load cell
  LoadCell.begin();
  float calibrationValue = 696.0;
  EEPROM.get(calVal_eepromAdress, calibrationValue);

  unsigned long stabilizingtime = 2000;
  boolean _tare = true;
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU > HX711 wiring and pin designations");
    while (1);
  } else {
    LoadCell.setCalFactor(calibrationValue);
    Serial.println("HX711 Startup is complete");
  }

  // Initialize HC-SR04 pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Initialize MQ-4 sensor pins
  pinMode(MQ4_DO_Pin, INPUT);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to ");
  Serial.print(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Start the server
  server.begin();
}

void loop() {
  static boolean newDataReady = 0;
  const int serialPrintInterval = 0;

  // Read and process distance measurement from HC-SR04
  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  int distance = duration * 0.034 / 2;

  // Read analog value from MQ-4
  int mq4_analogValue = analogRead(MQ4_AO_Pin);
  float mq4_voltage = mq4_analogValue * (5.0 / 1023.0);

  // Read digital output from MQ-4
  int mq4_digitalValue = digitalRead(MQ4_DO_Pin);

  // Check for new data from HX711
  if (LoadCell.update()) newDataReady = true;

  // Get smoothed value from HX711
  float weight = 0;
  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      weight = LoadCell.getData();
      newDataReady = 0;
      t = millis();
    }
  }

  // Determine bin status
  String binStatus = "Empty";
  if (weight <= 100) {
    binStatus = "Empty";
  } else if (weight <= 3500) {
    binStatus = "Medium";
  } else if (weight <= 7000) {
    binStatus = "Full";
  } else {
    binStatus = "Overloaded";
  }

  // Determine gas level
  String gasStatus = "No Gas";
  if (mq4_analogValue < 100) {
    gasStatus = "No Gas";
  } else if (mq4_analogValue < 300) {
    gasStatus = "Low Gas";
  } else if (mq4_analogValue < 600) {
    gasStatus = "Medium Gas";
  } else {
    gasStatus = "High Gas";
  }

  // Handle HTTP requests
  WiFiClient client = server.available();
  if (client) {
    String request = client.readStringUntil('\r');
    client.flush();

    // Respond with JSON data
    String response = "{";
    response += "\"binStatus\":\"" + binStatus + "\",";
    response += "\"weight\":" + String(weight) + ",";
    response += "\"gasStatus\":\"" + gasStatus + "\"";
    response += "}";

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();
    client.println(response);
    delay(1);
    client.stop();
  }

  // Wait for a bit before the next loop iteration
  delay(1000);
}
