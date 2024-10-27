#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const String PHONE = "";
#define rxPin 16
#define txPin 17
#define BAUD_RATE 9600

const char* ssid = "";        
const char* password = ""; 

void setup(){
    Serial2.begin(BAUD_RATE, SERIAL_8N1, rxPin, txPin);
    Serial.begin(115200);

    delay(1000);

    WiFi.begin(ssid, password);
    
    // Wait until connected to WiFi
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }

    delay(10000);
    
    // Print ESP32's IP address
    Serial.println("\nConnected to WiFi!");
    Serial.print("ESP32 IP Address: ");
    Serial.println(WiFi.localIP());
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String serverUrl = "";  

    // Set content-type and start the request
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    // JSON payload
    String postData = "{\"amount\": 100}";  

    // Send HTTP POST request
    int httpResponseCode = http.POST(postData);

    if (httpResponseCode > 0) {
      String response = http.getString(); // Get the JSON response
      Serial.println("Response: " + response); // Debugging

      // Use ArduinoJson to parse JSON response
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, response);
      
      if (error) {
        Serial.print("JSON Parsing failed: ");
        Serial.println(error.c_str());
        return;
      }

      // Extract 's3InvoiceUrl'
      const char* s3InvoiceUrl = doc["s3InvoiceUrl"];
      if (s3InvoiceUrl) {
        // Send SMS with receipt link
        sendSMS(PHONE, String("Download receipt from: ") + s3InvoiceUrl);
      } else {
        Serial.println("Error: 's3InvoiceUrl' not found in JSON response.");
      }
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  }

  delay(60000); // Delay between checks to prevent spamming the server
}

void sendSMS(String phoneNumber, String message) {
  Serial2.println("AT");  
  delay(1000);

  Serial2.println("AT+CMGF=1");  
  delay(1000);

  Serial2.print("AT+CMGS=\"");  
  Serial2.print(phoneNumber);   
  Serial2.println("\"");
  delay(1000);

  Serial2.print(message);  
  delay(1000);

  Serial2.write(26);  // CTRL+Z to send the SMS
  delay(5000);  

  String smsStatus = "";
  while (Serial2.available()) {
    smsStatus += (char)Serial2.read();
  }
  Serial.println(smsStatus);  // Debugging SMS status
}
