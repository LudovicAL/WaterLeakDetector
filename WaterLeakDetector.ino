#include <WiFi.h>
#include "ESP32_MailClient.h"

#define emailSenderAccount "example_sender_account@gmail.com"
#define emailSenderPassword "email_sender_password"
#define emailRecipientAccount "example_recipient_account@gmail.com"
#define smtpServer "smtp.gmail.com"
#define smtpServerPort 465
#define emailSubject "Alert from your ESP32"

const long timeoutTime = 2000;
const long intervalBetweenReadings = 5000;  //In milliseconds
const char* wifiname = "REPLACE_WITH_YOUR_SSID";
const char* password = "REPLACE_WITH_YOUR_PASSWORD";

unsigned long currentTime = millis();
unsigned long previousTime = 0;
SMTPData smtpData;
bool emailSent = false;
WiFiServer server(80);  //Server port number
WiFiClient client;
String header;  //Stores the HTTP requests to the server


void setup() {
  Serial.begin(115200); // initialize serial communication at X bits per second:
  delay(1000);  //In milliseconds
  WiFi.mode(WIFI_STA);
  connectToWifi();
}

void loop() {
  currentTime = millis();
  checkForWater();
  checkWebServer();
  if ((currentTime - previousTime) > intervalBetweenReadings) {
    previousTime = currentTime;
  }
}

//Checks the web server
void checkWebServer() {
  client = server.available();  //Listens for incoming clients
  if (client) { //If a new client connects
    Serial.println("New client connected");
    String currentLine = "";  //Holds incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  //Loops while the client's connected
      currentTime = millis();
      if (client.available()) { //If there's bytes to read from the client
        char c = client.read(); //Reads a byte, then
        Serial.write(c); //Prints it out the serial monitor
        header += c;
        if (c == '\n') { //If the byte is a newline character
          if (currentLine.length() == 0) { //If the current line is blank, you got two newline characters in a row. That's the end of the client HTTP request, so send a response
            client.println("HTTP/1.1 200 OK");  //HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            client.println("Content-type:text/html"); //And a content-type so the client knows what's coming
            client.println("Connection: close");
            client.println();  //Then a blank line

            //See https://randomnerdtutorials.com/esp32-web-server-arduino-ide/ for this part

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println("<body><h1>ESP32 Web Server</h1>");

            //See https://randomnerdtutorials.com/esp32-web-server-arduino-ide/ for this part

            client.println("</body></html>");
            client.println(); //The HTTP response ends with another blank line
            break;  //Breaks out of the while loop
          } else { //If you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  //If you got anything else but a carriage return character,
          currentLine += c;      //Adds it to the end of the currentLine
        }
      }
    }
    header = "";  //Clears the header variable
    client.stop();  //Closes the connection
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

//Checks for water at the sensor
void checkForWater() {
  if ((currentTime - previousTime) > intervalBetweenReadings) {
    //if (waterDetected) {
      Serial.println("Water detected!");
      if (!emailSent) {
        if(sendEmail("Water detected!")) {
          Serial.print("Email sent");
          emailSent = true;
        } else {
          Serial.println("Email failed to send");
        }
      }
    //}
  }
}

//Sends an email
bool sendEmail(String emailMessage){
  smtpData.setLogin(smtpServer, smtpServerPort, emailSenderAccount, emailSenderPassword);
  smtpData.setSender("ESP32", emailSenderAccount);
  smtpData.setPriority("High");
  smtpData.setSubject(emailSubject);
  smtpData.setMessage(emailMessage, true);
  smtpData.addRecipient(emailRecipientAccount);
  smtpData.setSendCallback(emailCallback);
  if (!MailClient.sendMail(smtpData)) {
    Serial.println("Error sending Email, " + MailClient.smtpErrorReason());
    return false;
  }
  smtpData.empty();
  return true;
}

//Callback function to get the email status
void emailCallback(SendStatus msg) {
  //Print the current status
  Serial.println("Email status: " + msg.info());
  //Do something when complete
  if (msg.success()) {
    Serial.println("Email sent");
  }
}

//Connects the device to the WiFi
void connectToWifi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to wifi: ");
    Serial.println(wifiname);
    WiFi.begin(wifiname, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000); // Milliseconds
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
}
