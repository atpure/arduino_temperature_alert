#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
//#include <ESP8266WebServer.h>
#include <EEPROM.h>

#define BUZZER 16 // D0
#define EEPROM_SIZE 1


// size of buffer used to capture HTTP requests
#define REQ_BUF_SZ   90
// size of buffer that stores the incoming string
#define TXT_BUF_SZ   50
char HTTP_req[REQ_BUF_SZ] = {0}; // buffered HTTP request stored as null terminated string
char req_index = 0;              // index into HTTP_req buffer
char txt_buf[TXT_BUF_SZ] = {0};  // buffer to save text to

int print_interval = 15; // x 200ms
int interval_count = print_interval;
double alert_temperature = 30;
double tempA, tempO;
boolean buzz = false;
int address = 0;
byte val;

const char *ssid = "TEMP_ALERT";
const char *password = "tempalert";

Adafruit_MLX90614 mlx = Adafruit_MLX90614();

WiFiServer server(80);

void setup() {
  Serial.begin(9600);
  mlx.begin();
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  //server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");

  EEPROM.begin(EEPROM_SIZE);
  val = EEPROM.read(address);

  if (val > 0 && val < 50) {
    alert_temperature = val;
  }
  Serial.print("Alert Temperature: ");
  Serial.println(alert_temperature);
}

void loop() {
  tempA = mlx.readAmbientTempC();
  tempO = mlx.readObjectTempC();

  WiFiClient client = server.available();

  if (client) {
    Serial.println("new client");   
    // an http request ends with a blank line
    bool currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);      
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply                
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          client.print("Alert Temperature: ");
          client.println(alert_temperature);
          client.println("<br>");
          client.println(F("  <form method=\"get\">"));
          client.print(F("  <input type='text' name='alert_temp' id='alert_temp' size=2 autofocus> "));
          client.println(F(" <input type=\"button\" name=\"button\" value=\"SET\" onclick=\"SaveValue();\">"));
          client.println(F("  </form>"));
          client.println("<br>");
          client.println("<h1>Temperature</h1>");
          client.print(tempO);
          client.println("<br>");
          client.println("</html>");                    
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
  

  if (--interval_count < 1 ) {
    interval_count = print_interval;
    Serial.print("Object Temperature = ");Serial.println(tempO);
  }
  
  if (tempO > alert_temperature) {
    digitalWrite(LED_BUILTIN, LOW);
    if (buzz == false) {
        digitalWrite(BUZZER, LOW);
        buzz = true;
    } else {
        digitalWrite(BUZZER, LOW);
        buzz = false;
    }
    delay(200);
  } else {
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(BUZZER, LOW);
    buzz = false;
    delay(200);
  }
}
