#include <WiFi.h>
#include <DHT.h>
 
// Wi-Fi credentials
const char* ssid = "Caandoy Residence";
const char* password = "@Admincaandoy1";
 
// Start web server
WiFiServer server(80);
 
// L298N Motor Driver Pins for Fan 1
const int ENA1 = 26;
const int IN1_1 = 27;
const int IN2_1 = 14;
 
// L298N Motor Driver Pins for Fan 2
const int ENA2 = 25;
const int IN1_2 = 33;
const int IN2_2 = 32;
 
int fanSpeed1 = 0;
int fanSpeed2 = 0;
bool autoMode = true; // Automatic mode enabled by default
 
// DHT Sensor
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
 
void setup() {
  Serial.begin(115200);
  dht.begin();
 
  pinMode(ENA1, OUTPUT);
  pinMode(IN1_1, OUTPUT);
  pinMode(IN2_1, OUTPUT);
 
  pinMode(ENA2, OUTPUT);
  pinMode(IN1_2, OUTPUT);
  pinMode(IN2_2, OUTPUT);
 
  digitalWrite(IN1_1, HIGH);
  digitalWrite(IN2_1, LOW);
  digitalWrite(IN1_2, HIGH);
  digitalWrite(IN2_2, LOW);
  analogWrite(ENA1, fanSpeed1);
  analogWrite(ENA2, fanSpeed2);
 
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 
  Serial.println("\nWiFi connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}
 
void loop() {
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New Client connected.");
    String request = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        request += c;
        if (c == '\n') { break; }
      }
    }
 
    if (request.indexOf("GET /fan1?value=") >= 0) {
      int valueIndex = request.indexOf("value=") + 6;
      String valueString = request.substring(valueIndex, request.indexOf(" HTTP/"));
      fanSpeed1 = valueString.toInt();
      analogWrite(ENA1, fanSpeed1);
      Serial.print("Fan 1 Speed Set to: ");
      Serial.println(fanSpeed1);
    }
 
    if (request.indexOf("GET /fan2?value=") >= 0) {
      int valueIndex = request.indexOf("value=") + 6;
      String valueString = request.substring(valueIndex, request.indexOf(" HTTP/"));
      fanSpeed2 = valueString.toInt();
      analogWrite(ENA2, fanSpeed2);
      Serial.print("Fan 2 Speed Set to: ");
      Serial.println(fanSpeed2);
    }
 
    if (request.indexOf("GET /toggleAutoMode") >= 0) {
      autoMode = !autoMode;
      Serial.print("Auto Mode: ");
      Serial.println(autoMode ? "ON" : "OFF");
    }
 
    if (request.indexOf("GET /temperature") >= 0) {
      float temperature = dht.readTemperature();
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/plain");
      client.println("Connection: close");
      client.println();
      client.println(String(temperature));
      client.stop();
      return;
    }
 
    float temperature = dht.readTemperature();
    if (autoMode) {
      fanSpeed1 = map(constrain(temperature, 25, 35), 25, 35, 0, 255);
      fanSpeed2 = fanSpeed1;
      analogWrite(ENA1, fanSpeed1);
      analogWrite(ENA2, fanSpeed2);
    }
 
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println("Connection: close");
    client.println();
    client.println("<!DOCTYPE html><html><head><title>Fan Speed Control</title>");
    client.println("<style>body { font-family: Arial, sans-serif; text-align: center; }");
    client.println(".slider { width: 80%; }</style></head>");
    client.println("<body>");
    client.println("<h2>ESP32 Web Server Fan Control</h2>");
    client.println("<p>Temperature: <span id='tempValue'>" + String(temperature) + "°C</span></p>");
    client.println("<button onclick='toggleMode()'>Toggle Mode (" + String(autoMode ? "Auto" : "Manual") + ")</button>");
    client.println("<p>Adjust Fan 1 Speed:</p>");
    client.println("<input type='range' min='0' max='255' value='" + String(fanSpeed1) + "' class='slider' id='fan1Slider' onchange='updateSpeed(1, this.value)'>");
    client.println("<p>Speed: <span id='speedValue1'>" + String(fanSpeed1) + "</span></p>");
    client.println("<p>Adjust Fan 2 Speed:</p>");
    client.println("<input type='range' min='0' max='255' value='" + String(fanSpeed2) + "' class='slider' id='fan2Slider' onchange='updateSpeed(2, this.value)'>");
    client.println("<p>Speed: <span id='speedValue2'>" + String(fanSpeed2) + "</span></p>");
    client.println("<script>");
    client.println("function updateSpeed(fan, value) {");
    client.println("if (!" + String(autoMode) + ") {");
    client.println("document.getElementById('speedValue' + fan).innerText = value;");
    client.println("var xhr = new XMLHttpRequest();");
    client.println("xhr.open('GET', '/fan' + fan + '?value=' + value, true);");
    client.println("xhr.send(); }}");
    client.println("function toggleMode() {");
    client.println("var xhr = new XMLHttpRequest();");
    client.println("xhr.open('GET', '/toggleAutoMode', true);");
    client.println("xhr.send(); location.reload(); }");
    client.println("setInterval(function() {");
    client.println("var xhttp = new XMLHttpRequest();");
    client.println("xhttp.onreadystatechange = function() { if (this.readyState == 4 && this.status == 200) { document.getElementById('tempValue').innerText = this.responseText + '°C'; } };");
    client.println("xhttp.open('GET', '/temperature', true);");
    client.println("xhttp.send(); }, 5000);");
    client.println("</script>");
    client.println("</body></html>");
    client.stop();
  }
}