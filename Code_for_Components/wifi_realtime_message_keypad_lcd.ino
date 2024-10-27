#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

#define rxPin 16
#define txPin 17
#define BAUD_RATE 9600

LiquidCrystal_I2C lcd(0x27, 16, 2);  

const String PHONE = "";
const char* ssid = "";        
const char* password = ""; 

String incomingMessage;
String paymentAmount;
String senderNumber;
String s3InvoiceUrl;  
bool receiptRequested = false; 

#define ROWS  4
#define COLS  4
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
uint8_t rowPins[ROWS] = {14, 27, 26, 25}; // GIOP14, GIOP27, GIOP26, GIOP25
uint8_t colPins[COLS] = {33, 32, 18, 19}; // GIOP33, GIOP32, GIOP18, GIOP19
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup() {
  Serial2.begin(BAUD_RATE, SERIAL_8N1, rxPin, txPin);
  Serial.begin(115200);

  lcd.init();  // Initialize LCD
  lcd.backlight(); // Turn on backlight

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());



}

void loop() {
  checkSMS();
  lcd.setCursor(0, 0);
  if (paymentAmount.length() > 0 && !receiptRequested) {
    // Display the payment amount on the LCD
    lcd.clear();
    lcd.print("Received: INR ");
    lcd.print(paymentAmount);
    lcd.setCursor(0, 1);
    lcd.print("Get receipt? A:");

    for (int i = 10; i > 0; i--) {
      delay(1000); // Wait for 1 second
      lcd.setCursor(0, 1);
      lcd.print("Time left: " + String(i) + "   "); // Clear previous text
      char key = keypad.getKey();
      if (key == 'A') {
        fetchS3InvoiceUrl();
        // If 'A' is pressed, ask for phone number
        lcd.clear();
        lcd.print("Enter Phone No:");
        i=500;
        String phoneNumber = getPhoneNumber();

        if (phoneNumber.length() > 0) {
          sendSMS(phoneNumber, String("Download payment from: ") + s3InvoiceUrl);
        }
        break; 
      }
    }
    delay(10000);
    
    paymentAmount = "";
    receiptRequested = false; // Reset flag
    lcd.clear();
    lcd.print("Waiting for SMS...");
  }
}


void checkSMS() {
  Serial2.println("AT+CMGF=1");  // Set SMS to text mode
  delay(1000);
  Serial2.println("AT+CNMI=1,2,0,0,0");  // Configure to show SMS immediately
  delay(1000);

  while (Serial2.available()) {
    incomingMessage += (char)Serial2.read();
  }

  
  if (incomingMessage.indexOf("INR") != -1) {
    parseAmount(incomingMessage);
    incomingMessage = "";  
  }
}

void parseAmount(String message) {
  Serial.println("Parsing message: " + message);
  int startIndex = message.indexOf("INR.") + 4;
  int endIndex = message.indexOf(" ", startIndex);
  
  if (startIndex != -1 && endIndex != -1) {
    paymentAmount = message.substring(startIndex, endIndex);
    Serial.println("Parsed Payment Amount: " + paymentAmount);
  }
}

void fetchS3InvoiceUrl() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String serverUrl = "http://192.168.29.81:3000/api/payments";  

    // Set content-type and start the request
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    // JSON payload
    String postData = "{\"amount\": " + paymentAmount + "}";  

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
      const char* invoiceUrl = doc["s3InvoiceUrl"];
      if (invoiceUrl) {
        s3InvoiceUrl = invoiceUrl;  // Store invoice URL
      } else {
        Serial.println("Error: 's3InvoiceUrl' not found in JSON response.");
      }
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  }
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

String getPhoneNumber() {
  lcd.setCursor(0,1);
  String phoneNumber = "";
  char key;
  while (phoneNumber.length() < 10) { // Assume 10 digit number
    key = keypad.getKey();
    if (key) {
      if (key >= '0' && key <= '9') { // Accept digits only
        phoneNumber += key;
        lcd.print(key); // Display digit on LCD
      }
      if (key == '#' && phoneNumber.length() > 0) { // End input on '#'
        lcd.clear();
        lcd.print("Sending recipet");
        break;
      }
    }
    delay(100); // Short delay to avoid excessive polling
  }
  return phoneNumber;
}