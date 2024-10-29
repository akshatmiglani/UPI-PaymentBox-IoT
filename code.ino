#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include "DFRobotDFPlayerMini.h"

#define rxPin 4
#define txPin 2
#define BAUD_RATE 9600

LiquidCrystal_I2C lcd(0x27, 16, 2);  
DFRobotDFPlayerMini dfPlayer;
#define FPSerial Serial1

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
  FPSerial.begin(9600, SERIAL_8N1, 16, 17);

  lcd.init();  // Initialize LCD
  lcd.backlight(); // Turn on backlight
  lcd.clear();
  lcd.print("Initializing...");

  delay(100);

  lcd.clear();
  lcd.print("Connecting MP3..");


  // Start communication with DFPlayer Mini
  if (dfPlayer.begin(FPSerial)) {
    dfPlayer.volume(29); // Set volume (0 to 30)
    dfPlayer.play(33); // Play startup sound message
    delay(3000); // Adjust delay based on audio length
  } else {
    lcd.clear();
    lcd.print("DFPlayer Error");
    while (true); // Halt execution
  }

  lcd.clear();
  delay(5000);

  
  lcd.clear();
  lcd.print("Wifi Setup...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }

  defaultMsgOnLCD();



}

void loop(){
  checkSMS();
  lcd.setCursor(0, 0);
  
  if (paymentAmount.length() > 0 && !receiptRequested) {
    
    lcd.clear();
    lcd.print("Received: INR ");
    lcd.print(paymentAmount);

    processAmount(paymentAmount.toFloat());

    lcd.setCursor(0, 1);
    lcd.print("Get receipt? A:");

    // Start the countdown for 10 seconds
    for (int i = 40; i > 0; i--) {
      delay(1000); // Wait for 1 second
      lcd.setCursor(0, 1);
      lcd.print("Time left: " + String(i) + "   "); // Clear previous text
      char key = keypad.getKey();
      if (key == 'A') {
        fetchS3InvoiceUrl();
        // If 'A' is pressed, ask for phone number
        lcd.clear();
        lcd.print("Enter Phone No:");
        String phoneNumber = getPhoneNumber();

        if (phoneNumber.length() > 0) {
          sendSMS(phoneNumber, String("Download payment from: ") + s3InvoiceUrl);
        }
        break; 
      }
    }

  
    paymentAmount = "";
    receiptRequested = false; 
    defaultMsgOnLCD

  }

}

void defaultMsgOnLCD(){
  lcd.clear();
  lcd.print("PAYMENT BOX");
  lcd.setCursor(0, 1);
  lcd.print("IS ONLINE");
}


void checkSMS() {
  Serial2.println("AT+CMGF=1");  // Set SMS to text mode
  delay(1000);
  Serial2.println("AT+CNMI=1,2,0,0,0");  // Configure to show SMS immediately
  delay(1000);

  while (Serial2.available()) {
    incomingMessage += (char)Serial2.read();
  }

  // Check if incoming message contains "INR"
  if (incomingMessage.indexOf("INR") != -1) {
    parseAmount(incomingMessage);
    incomingMessage = "";  // Clear message after parsing
  }
}

void parseAmount(String message) {
  // Example: "VPA "akshatmiglani.am-1@okhdfcbank" linked to A/C No."XXXXXX1088" is Dr with INR.88.0"
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
    String serverUrl = "http://IP:3000/api/payments";  

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
    delay(100); 
  }
  return phoneNumber;
}


void processAmount(float amount) {
  Serial.println("Processing Amount: " + String(amount, 2));

  // Split amount into rupees and paise
  long rupees = (long)amount;
  int paise = (int)((amount - rupees) * 100);

  // Display on LCD
  lcd.clear();
  lcd.print("Received: ");
  lcd.setCursor(0, 1);
  lcd.print("INR " + String(rupees) + "." + String(paise));

  // Play audio notification
  playAmountAudio(rupees, paise);

}

void playAmountAudio(long rupees, int paise) {
  // Play "You have received"
  
  dfPlayer.play(getAudioFileNumber("you have received"));
  waitForAudioToFinish();
  delay(1000);

  // Play "rupees"
  // dfPlayer.play(getAudioFileNumber("rupees"));
  // waitForAudioToFinish();
  // delay(500);

  // Convert rupees to words
  String rupeesInWords = convertNumberToWords(rupees);
  Serial.println("Rupees in Words: " + rupeesInWords);

  // Convert paise to words
  String paiseInWords = "";
  if (paise > 0) {
    paiseInWords = convertNumberToWords(paise);
    Serial.println("Paise in Words: " + paiseInWords);
  }

  // Split words into arrays and play audio
  playWords(rupeesInWords, "rupees");
  if (paise > 0) {
    playWords(paiseInWords, "paise");
  }
}

void playWords(String words, String unit) {
  String wordArray[20];
  int wordCount = splitString(words, ' ', wordArray);

  // Play words in sequence
  for (int i = 0; i < wordCount; i++) {
    int fileNumber = getAudioFileNumber(wordArray[i]);
    if (fileNumber != -1) {
      dfPlayer.play(fileNumber);
      waitForAudioToFinish();
      delay(50);
    }
  }

  // Play unit (e.g., "rupees" or "paise")
  dfPlayer.play(getAudioFileNumber(unit));
  waitForAudioToFinish();

  delay(500);
}

int splitString(String str, char delimiter, String *tokens) {
  int index = 0;
  int startIndex = 0;
  int endIndex = str.indexOf(delimiter);

  while (endIndex >= 0) {
    tokens[index++] = str.substring(startIndex, endIndex);
    startIndex = endIndex + 1;
    endIndex = str.indexOf(delimiter, startIndex);
  }
  tokens[index++] = str.substring(startIndex);
  return index;
}

String convertNumberToWords(long number) {
  if (number == 0) return "zero";

  String words = "";

  if (number >= 1000) {
    long thousands = number / 1000;
    words += numberToWordsBelowThousand(thousands) + " thousand ";
    number %= 1000;
  }
  if (number > 0) {
    words += numberToWordsBelowThousand(number);
  }

  words.trim();  // Trim any extra spaces at the beginning or end
  return words;
}

String numberToWordsBelowThousand(long number) {
  String words = "";

  if (number >= 100) {
    long hundreds = number / 100;
    words += numberToWordsBelowHundred(hundreds) + " hundred ";
    number %= 100;
  }
  if (number > 0) {
    words += numberToWordsBelowHundred(number);
  }
  return words;
}

String numberToWordsBelowHundred(int number) {
  String ones[] = {"", "one", "two", "three", "four", "five", "six", "seven",
                   "eight", "nine", "ten", "eleven", "twelve", "thirteen",
                   "fourteen", "fifteen", "sixteen", "seventeen", "eighteen",
                   "nineteen", "twenty"};
  String tens[] = {"", "", "twenty", "thirty", "forty", "fifty", "sixty",
                   "seventy", "eighty", "ninety"};
  String words = "";

  if (number <= 20) {
    words += ones[number] + " ";
  } else {
    int ten = number / 10;
    int one = number % 10;
    words += tens[ten] + " ";
    if (one > 0) {
      words += ones[one] + " ";
    }
  }
  return words;
}

int getAudioFileNumber(String word) {
  word.toLowerCase();
  word.trim();

  // Map words to file numbers (assuming additional audio files for phrases)
  if (word == "one") return 1;
  else if (word == "two") return 2;
  else if (word == "three") return 3;
  else if (word == "four") return 4;
  else if (word == "five") return 5;
  else if (word == "six") return 6;
  else if (word == "seven") return 7;
  else if (word == "eight") return 8;
  else if (word == "nine") return 9;
  else if (word == "ten") return 10;
  else if (word == "eleven") return 11;
  else if (word == "twelve") return 12;
  else if (word == "thirteen") return 13;
  else if (word == "fourteen") return 14;
  else if (word == "fifteen") return 15;
  else if (word == "sixteen") return 16;
  else if (word == "seventeen") return 17;
  else if (word == "eighteen") return 18;
  else if (word == "nineteen") return 19;
  else if (word == "twenty") return 20;
  else if (word == "thirty") return 21;
  else if (word == "forty") return 22;
  else if (word == "fifty") return 23;
  else if (word == "sixty") return 24;
  else if (word == "seventy") return 25;
  else if (word == "eighty") return 26;
  else if (word == "ninety") return 27;
  else if (word == "hundred") return 28;
  else if (word == "thousand") return 29;
  else if (word == "you have received") return 32; 
  else if (word == "rupees") return 31;
  else if (word == "paise") return 30;
  else return -1; // Word not found
}

void waitForAudioToFinish() {
  while (true) {
    if (dfPlayer.available()) { // Check if DFPlayer Mini has any new data available
      uint8_t type = dfPlayer.readType(); // Read the type of the message
      if (type == DFPlayerPlayFinished) { 
        break;
      }
    } 
  }
  delay(500);
}