#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "Wifi";        
const char* password = "Akshat001"; 

void setup() {
  Serial.begin(115200);
  
  // Connect to WiFi
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
  if(WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Your API endpoint (replace with your server's IP address)
    String serverUrl = "http://192.168.222.39:3000/api/payments";  
    
    // Specify content-type header
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");
    
    // Create JSON payload
    String postData = "{\"amount\": 100}";  // Modify as per your requirement
    
    // Send HTTP POST request
    int httpResponseCode = http.POST(postData);
    
    // Check response code
    if (httpResponseCode > 0) {
      String response = http.getString(); // Get the response to the request
      Serial.println(httpResponseCode);   // Print HTTP response code
      Serial.println(response);           // Print response payload
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }

    // End the connection
    http.end();
  }
  
  delay(10000); // Wait for 10 seconds before sending another request
}
