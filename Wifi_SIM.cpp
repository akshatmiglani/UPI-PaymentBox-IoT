#include <WiFi.h>
#include <HTTPClient.h>

const String PHONE="+919601131088"
#define rxPin 4
#define txPin 2
#define BAUD_RATE 9600
HardwareSerial sim800(1);

String smsStatus,senderNumber,receivedDate,msg;

const char* ssid = "Wifi";        
const char* password = "Akshat001"; 

void setup(){
    sim800.begin(BAUD_RATE, SERIAL_8N1, rxPin, txPin);

    smsStatus = "";
    senderNumber="";
    receivedDate="";
    msg="";

    delay(1000);

     WiFi.begin(ssid, password);
    Serial.println("Connecting to WiFi...");
    
    // Wait until connected to WiFi
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    
    // Print ESP32's IP address
    Serial.println("\nConnected to WiFi!");
    Serial.print("ESP32 IP Address: ");
    Serial.println(WiFi.localIP());


}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String serverUrl = "http://192.168.222.39:3000/api/payments";  

    // Set content-type and start the request
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    // JSON payload
    String postData = "{\"amount\": 100}";  

    // Send HTTP POST request
    int httpResponseCode = http.POST(postData);

    if (httpResponseCode > 0) {
      String response = http.getString(); // Get the JSON response

      // Parse JSON to extract the 'path' containing S3 receipt link
      int pathIndex = response.indexOf("\"path\":\"");
      if (pathIndex != -1) {
        int linkStart = pathIndex + 8;
        int linkEnd = response.indexOf("\"", linkStart);
        String receiptLink = response.substring(linkStart, linkEnd);

        // Send SMS with receipt link
        sendSMS("+1234567890", "Download receipt from: " + receiptLink); 
      } else {
        Serial.println("Error: 'path' not found in JSON response.");
      }
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  }

  delay(100000000); 
}

void sendSMS(String phoneNumber, String message) {
  sim800.println("AT");  
  delay(1000);

  sim800.println("AT+CMGF=1");  
  delay(1000);

  sim800.print("AT+CMGS=\"");  
  sim800.print(phoneNumber);   
  sim800.println("\"");
  delay(1000);

  sim800.print(message);  
  delay(1000);

  sim800.write(26);  // CTRL+Z to send the SMS
  delay(5000);  

  String smsStatus = "";
  while (sim800.available()) {
    smsStatus += (char)sim800.read();
  }
  Serial.println(smsStatus);  // Debugging SMS status
}