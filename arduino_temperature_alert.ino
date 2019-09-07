#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
//#include <ESP8266WebServer.h>
#include <EEPROM.h>

#define BUZZER 16 // D0
#define EEPROM_SIZE 1

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

  if (val > 0 && val <= 50) {
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

    String req = client.readStringUntil('\r');
    Serial.println(req);
    client.flush();
    
    if (req.indexOf("/?alert_temp") > 0) {
      String newVal = req.substring(req.indexOf("=")+1, req.indexOf("HTTP")-1);
      Serial.println(newVal);
      SaveValue(newVal.toInt());
    }


          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
//          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          client.println("<body>");
          client.print("Alert Temperature: ");
          client.println(alert_temperature);
          client.println("<br>");
          client.println(F("  <form method=\"get\">"));
          client.println(F("  <input type='text' name='alert_temp' id='alert_temp' size=2 autofocus> "));
          client.println(F(" <input type=\"button\" name=\"button\" value=\"SET\" onclick=\"submit();\">"));
          client.println(F("  </form>"));
          client.println("(1 ~ 50)");
          client.println("<br>");
          client.println("<h1>Temperature</h1>");
          client.println(tempO);
          client.println("<br>");
          client.println("</body>");
          client.println("</html>");                    

    // give the web browser time to receive the data
    delay(1);
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
        digitalWrite(BUZZER, HIGH);
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

void SaveValue(int temp)
{
  if (temp > 0 && temp <= 50) {
    Serial.println("New Value Saved");
    alert_temperature = temp;
    EEPROM.write(address, temp);
    EEPROM.commit();
  }
}
