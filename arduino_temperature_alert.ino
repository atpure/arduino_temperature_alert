#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
//#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define BUZZER 16 // D0
#define EEPROM_SIZE 1

int print_interval = 15; // x 200ms
int print_interval_count = print_interval;
int high_temp_check = 5; // x 200ms
int high_temp_check_count = high_temp_check;
int led_interval = 5; // x 200ms
int led_interval_count = led_interval;
float alert_temperature = 30;
float recovery_threthold = 1;
float tempA, tempO, tempO_max;
boolean high_temp = false;
boolean alert_on = false;
boolean buzz_on = false;
boolean led_on = false;
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

  if (val > 0 && val <= 300) {
    alert_temperature = val;
  }
  Serial.print("Alert Temperature: ");
  Serial.println(alert_temperature);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
  }
}

void loop() {
  tempA = mlx.readAmbientTempC();
  tempO = mlx.readObjectTempC();

  if (tempO > tempO_max) {
    tempO_max = tempO;
  }

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
          client.println("(1 ~ 300)");
          client.println("<br><br>");
          client.print("Max Temperature: ");
          client.println(tempO_max);
          client.println("<br>");
          client.println("<h1>Temperature</h1>");
          client.print("<h2>");
          client.print(tempO);
          client.print("<h2>");
          client.println("<br>");
          client.println("</body>");
          client.println("</html>");

    // give the web browser time to receive the data
    delay(1);
    Serial.println("client disconnected");
  }


  if (--print_interval_count < 1 ) {
    print_interval_count = print_interval;
    Serial.print("Object Temperature = ");Serial.println(tempO);
  }

  if (tempO > alert_temperature || (alert_on && (tempO > alert_temperature - recovery_threthold))) {
    digitalWrite(LED_BUILTIN, LOW);
    high_temp = true;

    if (--high_temp_check_count > 0) { // An alarm is triggered when high temperature is maintained for more than 'high_temp_check' seconds.
      ;
    } else {
      alert_on = true;
      if (buzz_on) {
          digitalWrite(BUZZER, LOW);
          buzz_on = false;
      } else {
          digitalWrite(BUZZER, HIGH);
          buzz_on = true;
      }
    }

    led_interval_count = led_interval;
    led_on = true;
    delay(200);
  } else {
    if (high_temp) {
      digitalWrite(LED_BUILTIN, HIGH);
      led_on = false;
      high_temp = false;
      alert_on = false;
    }
    if (--led_interval_count > 0) {
      ;
    } else {
      if (led_on) {
          digitalWrite(LED_BUILTIN, HIGH);
          led_on = false;
          led_interval_count = led_interval;
      } else {
          digitalWrite(LED_BUILTIN, LOW);
          led_on = true;
          led_interval_count = led_interval;
      }
    }

    high_temp_check_count = high_temp_check;
    digitalWrite(BUZZER, LOW);
    buzz_on = false;
    delay(200);
  }

  // OLED Display
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,4);
  display.println("Target");

  display.setTextSize(2);
  display.setCursor(50,0);
  display.println(tempO,1);

  display.setCursor(110,0);
  display.println("C");

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,17);
  display.println("Max");

  display.setTextSize(1);
  display.setCursor(50,17);
  display.println(tempO_max,1);

  display.setCursor(110,17);
  display.println("C");

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,25);
  display.println("Alert");

  display.setTextSize(1);
  display.setCursor(50,25);
  display.println(alert_temperature,1);

  display.setCursor(110,25);
  display.println("C");

  display.display();
}

void SaveValue(int temp)
{
  if (temp > 0 && temp <= 300) {
    Serial.println("New Value Saved");
    if (alert_temperature != temp) {
      alert_temperature = temp;
      EEPROM.write(address, temp);
      EEPROM.commit();
    }
  }
}
