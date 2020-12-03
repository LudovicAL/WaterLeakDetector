#include <WiFi.h>
#include <WebServer.h>
#include "ESP32_MailClient.h"

#define milliPerDay 86400000
#define milliPerHour 3600000
#define milliPerMinute 60000
#define milliPerSecond 1000

const char* wifiname = "ENTER-WIFI-NAME-HERE";
const char* password = "ENTER-WIFI-PASSWORD-HERE";

enum category {
  SUCCESS,
  DANGER,
  WARNING
};

String emailSenderAccount = "example_sender_account@gmail.com";
String emailSenderPassword = "email_sender_password";
String smtpServer = "smtp.gmail.com";
int smtpServerPort = 465;
String emailRecipientAccount = "example_receiver_account@gmail.com";

SMTPData smtpData;
bool emailSent = false;
WebServer server(80);

void setup() {
  Serial.begin(115200); // This is the serial communication speed to communicate with the ESP32, in bits per second:
  delay(3000);  //In milliseconds
  WiFi.mode(WIFI_STA);
  connectToWifi();
  startWebServer();
}

void loop() {
  //checkForWater();
  server.handleClient();
}

//Starts the web server
void startWebServer() {
  server.on("/", handle_OnConnect);
  server.on("/revertChangesButton", handle_onRevertChangesButton);
  server.on("/applyChangesButton", handle_onApplyChangesButton);
  server.on("/testButton", handle_onTestButton);
  server.onNotFound(handle_NotFound);
  server.begin();
  Serial.println("HTTP server started");
}

//A handle for the WebServer OnConnect event
void handle_OnConnect() {
  Serial.println("OnConnect");
  server.send(200, "text/html", SendHTML("", SUCCESS)); 
}

//A handle for the WebServer onRevertChangesButton event
void handle_onRevertChangesButton() {
  Serial.println("Revert changes button pressed");
  server.send(200, "text/html", SendHTML("Changes reverted", WARNING));
}

//A handle for the WebServer onApplyChangesButton event
void handle_onApplyChangesButton() {
  Serial.println("Apply changes button pressed");
  server.send(200, "text/html", SendHTML("Changes applied", SUCCESS));
}

//A handle for the WebServer onTestButton event
void handle_onTestButton() {
  if(sendEmail("Test from your ESP32", "This is a test email from your ESP32.")) {
    Serial.print("Email sent successfully");
    server.send(200, "text/html", SendHTML("Email sent successfully", SUCCESS));
  } else {
    Serial.println("Email failed to send");
    server.send(200, "text/html", SendHTML("Email failed to send", DANGER));
  }
}

//A handle for the WebServer NotFound event
void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

//Returns the device's uptime as readable string
String getUpTime() {
  unsigned long milli = millis();
  int days = milli / milliPerDay;
  milli -= days * milliPerDay;
  int hours = milli / milliPerHour;
  milli -= hours * milliPerHour;
  int minutes = milli / milliPerMinute;
  milli -= minutes * milliPerMinute;
  int seconds = milli / milliPerSecond;
  milli -= seconds * milliPerSecond; 
  return (String(days) + "d " + String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s");
}

//Returns a message to be displayed on top of the web page
String buildMessage(String message, category c) {
  String ptr = "";
  if (message.length() > 0) {
    ptr += "<!-- Message -->\n";
    ptr += "<div class=\"container\">\n";
    ptr += "    <div class=\"alert alert-";
    switch (c) {
      case SUCCESS:
        ptr += "success";
        break;
      case DANGER:
        ptr += "danger";
        break;
      case WARNING:
        ptr += "warning";
        break;
      default:
        Serial.println("Unrecognised category");
        ptr += "warning";
        break;
    }
    ptr += "\">\n";
    ptr += "        <strong>" + message + "</strong>\n";
    ptr += "    </div>\n";
    ptr += "</div>\n";
  }
  return ptr;
}

//Returns the wifi status as a string
String getWifiStatus() {
  switch (WiFi.status()) {
    case WL_NO_SHIELD:
      return "No shield (255)";
      break;
    case WL_IDLE_STATUS:
      return "Idle status (0)";
      break;
    case WL_NO_SSID_AVAIL:
      return "No SSID available (1)";
      break;
    case WL_SCAN_COMPLETED:
      return "Scan completed (2)";
      break;
    case WL_CONNECTED:
      return "Connected (3)";
      break;
    case WL_CONNECT_FAILED:
      return "Connection failed (4)";
      break;
    case WL_CONNECTION_LOST:
      return "Connection lost (5)";
      break;
    case WL_DISCONNECTED:
      return "Disconnected (6)";
      break;
    default:
      return "Status unknown";
      break;
  }
}

//Sends the WebServer HTML to the client
String SendHTML(String message, category c){
  String ptr = "<!DOCTYPE html>\n";
  ptr += "<html lang=\"en\">\n";
  ptr += "<head>";
  ptr += "    <title>Water Leak Detector</title>\n";
  ptr += "    <meta charset=\"utf-8\">\n";
  ptr += "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n";
  ptr += "    <link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.4.1/css/bootstrap.min.css\">\n";
  ptr += "    <script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.5.1/jquery.min.js\"></script>\n";
  ptr += "    <script src=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.4.1/js/bootstrap.min.js\"></script>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<!-- Title -->\n";
  ptr += "<div class=\"jumbotron text-center\">\n";
  ptr += "    <h1>Water Leak Detector</h1>\n";
  ptr += "</div>\n";
  ptr += buildMessage(message, c);
  ptr += "<!-- Inputs -->\n";
  ptr += "<div class=\"container\">\n";
  ptr += "    <h1>Inputs</h1>\n";
  ptr += "    <div class=\"panel panel-default\">\n";
  ptr += "        <!-- Wifi -->\n";
  ptr += "        <div class=\"panel-heading\">Wifi</div>\n";
  ptr += "        <div class=\"panel-body\">\n";
  ptr += "            <div class=\"form-group\">\n";
  ptr += "                <label for=\"wifiName\">WiFi name:</label>\n";
  ptr += "                <input type=\"text\" class=\"form-control\" id=\"wifiName\" value=\"" + WiFi.SSID() + "\">\n";
  ptr += "            </div>\n";
  ptr += "            <div class=\"form-group\">\n";
  ptr += "                <label for=\"wifiPassword\">WiFi password:</label>\n";
  ptr += "                <input type=\"password\" class=\"form-control\" id=\"wifiPassword\">\n";
  ptr += "                <a href=\"#tipPassword\" class=\"btn btn-info\" data-toggle=\"collapse\">Tip</a>\n";
  ptr += "                <div id=\"tipPassword\" class=\"alert alert-info collapse\">\n";
  ptr += "                    Yep, this password is sent as clear text to the device. No encryption here. This is the way it is.\n";
  ptr += "                </div>\n";
  ptr += "            </div>\n";
  ptr += "        </div>\n";
  ptr += "        <!-- Sender email -->\n";
  ptr += "        <div class=\"panel-heading\">Sender email</div>\n";
  ptr += "        <div class=\"panel-body\">\n";
  ptr += "            <div class=\"form-group\">\n";
  ptr += "                <label for=\"senderEmail\">Sender email addess:</label>\n";
  ptr += "                <input type=\"text\" class=\"form-control\" id=\"senderEmail\" value=\"" + emailSenderAccount + "\">\n";
  ptr += "                <a href=\"#tipSenderEmail\" class=\"btn btn-info\" data-toggle=\"collapse\">Tip</a>\n";
  ptr += "                <div id=\"tipSenderEmail\" class=\"alert alert-info collapse\">\n";
  ptr += "                    Create a new email account for this. If something goes wrong with the code, the account you use may get banned.\n";
  ptr += "                    If you decide to use Gmail, make sure you \"allow less secure apps\" in the account settings. Other email service provider may require that you activate a similar option for this to work.\n";
  ptr += "                </div>\n";
  ptr += "            </div>\n";
  ptr += "            <div class=\"form-group\">\n";
  ptr += "                <label for=\"senderEmailPassword\">Sender email password:</label>\n";
  ptr += "                <input type=\"password\" class=\"form-control\" id=\"senderEmailPassword\">\n";
  ptr += "                <a href=\"#tipSenderEmailPassword\" class=\"btn btn-info\" data-toggle=\"collapse\">Tip</a>\n";
  ptr += "                <div id=\"tipSenderEmailPassword\" class=\"alert alert-info collapse\">\n";
  ptr += "                    Again, this password is sent as clear text to the device. No encryption here. Deal with it.\n";
  ptr += "                </div>\n";
  ptr += "            </div>\n";
  ptr += "            <div class=\"form-group\">\n";
  ptr += "                <label for=\"senderSmtServerName\">Smt server name:</label>\n";
  ptr += "                <input type=\"text\" class=\"form-control\" id=\"senderSmtServerName\" value=\"" + smtpServer + "\">\n";
  ptr += "                <a href=\"#tipSenderSmtServerName\" class=\"btn btn-info\" data-toggle=\"collapse\">Tip</a>\n";
  ptr += "                <div id=\"tipSenderSmtServerName\" class=\"alert alert-info collapse\">\n";
  ptr += "                    <table class=\"table table-bordered\">\n";
  ptr += "                        <thead>\n";
  ptr += "                            <tr>\n";
  ptr += "                                <th>Email service provider</th>\n";
  ptr += "                                <th>Smt server name</th>\n";
  ptr += "                            </tr>\n";
  ptr += "                        </thead>\n";
  ptr += "                        <tbody>\n";
  ptr += "                            <tr>\n";
  ptr += "                                <td>Gmail</td>\n";
  ptr += "                                <td>smtp.gmail.com</td>\n";
  ptr += "                            </tr>\n";
  ptr += "                            <tr>\n";
  ptr += "                                <td>Outlook</td>\n";
  ptr += "                                <td>smtp.office365.com</td>\n";
  ptr += "                            </tr>\n";
  ptr += "                            <tr>\n";
  ptr += "                                <td>Live or Hotmail</td>\n";
  ptr += "                                <td>smtp.live.com</td>\n";
  ptr += "                            </tr>\n";
  ptr += "                            <tr>\n";
  ptr += "                                <td>Other</td>\n";
  ptr += "                                <td>You need to search for your email service provider SMTP Server settings</td>\n";
  ptr += "                            </tr>\n";
  ptr += "                        </tbody>\n";
  ptr += "                    </table>\n";
  ptr += "                </div>\n";
  ptr += "            </div>\n";
  ptr += "            <div class=\"form-group\">\n";
  ptr += "                <label for=\"senderSmtServerPort\">Smt server port:</label>\n";
  ptr += "                <input type=\"text\" class=\"form-control\" rows=\"5\" id=\"senderSmtServerPort\" value=\"" + String(smtpServerPort) + "\">\n";
  ptr += "                <a href=\"#tipSenderSmtServerPort\" class=\"btn btn-info\" data-toggle=\"collapse\">Tip</a>\n";
  ptr += "                <div id=\"tipSenderSmtServerPort\" class=\"alert alert-info collapse\">\n";
  ptr += "                    <table class=\"table table-bordered\">\n";
  ptr += "                        <thead>\n";
  ptr += "                            <tr>\n";
  ptr += "                                <th>Email service provider</th>\n";
  ptr += "                                <th>Smt server port</th>\n";
  ptr += "                            </tr>\n";
  ptr += "                        </thead>\n";
  ptr += "                        <tbody>\n";
  ptr += "                            <tr>\n";
  ptr += "                                <td>Gmail</td>\n";
  ptr += "                                <td>465</td>\n";
  ptr += "                            </tr>\n";
  ptr += "                            <tr>\n";
  ptr += "                                <td>Outlook</td>\n";
  ptr += "                                <td>587</td>\n";
  ptr += "                            </tr>\n";
  ptr += "                            <tr>\n";
  ptr += "                                <td>Live or Hotmail</td>\n";
  ptr += "                                <td>587</td>\n";
  ptr += "                            </tr>\n";
  ptr += "                            <tr>\n";
  ptr += "                                <td>Other</td>\n";
  ptr += "                                <td>You need to search for your email service provider SMTP Server settings</td>\n";
  ptr += "                            </tr>\n";
  ptr += "                        </tbody>\n";
  ptr += "                    </table>\n";
  ptr += "                </div>\n";
  ptr += "            </div>\n";
  ptr += "        </div>\n";
  ptr += "        <!-- Receiver emails -->\n";
  ptr += "        <div class=\"panel-heading\">Receiver email</div>\n";
  ptr += "        <div class=\"panel-body\">\n";
  ptr += "            <div class=\"form-group\">\n";
  ptr += "                <label for=\"receiverEmail\">Receiver email addess:</label>\n";
  ptr += "                <input type=\"text\" class=\"form-control\" id=\"receiverEmail\" value=\"" + emailRecipientAccount + "\">\n";
  ptr += "                <a href=\"#tipReceiverEmail\" class=\"btn btn-info\" data-toggle=\"collapse\">Tip</a>\n";
  ptr += "                <div id=\"tipReceiverEmail\" class=\"alert alert-info collapse\">\n";
  ptr += "                    To send SMS, refer to the following table:\n";
  ptr += "                    <table class=\"table table-bordered\">\n";
  ptr += "                        <thead>\n";
  ptr += "                            <tr>\n";
  ptr += "                                <th>Phone service provider</th>\n";
  ptr += "                                <th>SMS gateway domain</th>\n";
  ptr += "                            </tr>\n";
  ptr += "                        </thead>\n";
  ptr += "                        <tbody>\n";
  ptr += "                            <tr>\n";
  ptr += "                                <td>Bell</td>\n";
  ptr += "                                <td>\n";
  ptr += "                                    [insert 10-digit number]@txt.bellmobility.ca<br>\n";
  ptr += "                                    or<br>\n";
  ptr += "                                    [insert 10-digit number]@txt.bell.ca<br>\n";
  ptr += "                                </td>\n";
  ptr += "                            </tr>\n";
  ptr += "                            <tr>\n";
  ptr += "                                <td>Chatr</td>\n";
  ptr += "                                <td>[insert 10-digit number]@pcs.rogers.com</td>\n";
  ptr += "                            </tr>\n";
  ptr += "                            <tr>\n";
  ptr += "                                <td>Fido / Microcell</td>\n";
  ptr += "                                <td>[insert 10-digit number]@fido.ca</td>\n";
  ptr += "                            </tr>\n";
  ptr += "                            <tr>\n";
  ptr += "                                <td>Koodo</td>\n";
  ptr += "                                <td>[insert 10-digit number]@msg.koodomobile.com</td>\n";
  ptr += "                            </tr>\n";
  ptr += "                            <tr>\n";
  ptr += "                                <td>President’s Choice</td>\n";
  ptr += "                                <td>[insert 10-digit number]@txt.bell.ca</td>\n";
  ptr += "                            </tr>\n";
  ptr += "                            <tr>\n";
  ptr += "                                <td>Rogers Canada</td>\n";
  ptr += "                                <td>[insert 10-digit number]@pcs.rogers.com</td>\n";
  ptr += "                            </tr>\n";
  ptr += "                            <tr>\n";
  ptr += "                                <td>Sasktel</td>\n";
  ptr += "                                <td>[insert 10-digit number]@sms.Sasktel.com</td>\n";
  ptr += "                            </tr>\n";
  ptr += "                            <tr>\n";
  ptr += "                                <td>Solo Mobile</td>\n";
  ptr += "                                <td>[insert 10-digit number]@txt.bell.ca</td>\n";
  ptr += "                            </tr>\n";
  ptr += "                            <tr>\n";
  ptr += "                                <td>Telus</td>\n";
  ptr += "                                <td>[insert 10-digit number]@msg.telus.com</td>\n";
  ptr += "                            </tr>\n";
  ptr += "                            <tr>\n";
  ptr += "                                <td>Virgin Mobile Canada</td>\n";
  ptr += "                                <td>[insert 10-digit number]@vmobile.ca</td>\n";
  ptr += "                            </tr>\n";
  ptr += "                            <tr>\n";
  ptr += "                                <td>Other</td>\n";
  ptr += "                                <td>You need to search for your phone service provider SMS gateway domain</td>\n";
  ptr += "                            </tr>\n";
  ptr += "                        </tbody>\n";
  ptr += "                    </table>\n";
  ptr += "                </div>\n";
  ptr += "            </div>\n";
  ptr += "        </div>\n";
  ptr += "        <!-- Revert, apply and test buttons -->\n";
  ptr += "        <div class=\"panel-heading\">Revert, apply or test</div>\n";
  ptr += "        <div class=\"panel-body\">\n";
  ptr += "            <a type=\"button\" class=\"btn btn-danger\" href=\"/revertChangesButton\">Revert changes</a>\n";
  ptr += "            <a type=\"button\" class=\"btn btn-success\" href=\"/applyChangesButton\">Apply changes</a>\n";
  ptr += "            <a type=\"button\" class=\"btn btn-warning\" href=\"/testButton\">Test</a>\n";
  ptr += "        </div>\n";
  ptr += "    </div>\n";
  ptr += "</div>\n";
  ptr += "<!-- Outputs -->\n";
  ptr += "<div class=\"container\">\n";
  ptr += "    <h1>Outputs</h1>\n";
  ptr += "    <div class=\"panel panel-default\">\n";
  ptr += "        <ul class=\"list-group\">\n";
  ptr += "            <li class=\"list-group-item\">Web server ip address<span class=\"badge\">" + WiFi.localIP().toString() + "</span></li>\n";
  ptr += "            <li class=\"list-group-item\">Wifi status<span class=\"badge\">" + getWifiStatus() + "</span></li>\n";
  ptr += "            <li class=\"list-group-item\">ESP32 up time (overflows after about 50 days)<span class=\"badge\">" + getUpTime() + "</span></li>\n";
  ptr += "            <li class=\"list-group-item\">Water detector current value<span class=\"badge\">5%</span></li>\n";
  ptr += "        </ul>\n";
  ptr += "    </div>\n";
  ptr += "</div>\n";
  return ptr;
}

//Checks for water at the sensor
void checkForWater() {
  //if (waterDetected) {
    Serial.println("Water detected!");
    if (!emailSent) {
      if(sendEmail("Alert from your ESP32", "Water detected!")) {
        Serial.print("Email sent");
        emailSent = true;
      } else {
        Serial.println("Email failed to send");
      }
    }
  //}
}

//Sends an email
bool sendEmail(String emailSubject, String emailMessage){
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
      delay(3000); // Milliseconds
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: " + WiFi.localIP().toString());
  }
}
