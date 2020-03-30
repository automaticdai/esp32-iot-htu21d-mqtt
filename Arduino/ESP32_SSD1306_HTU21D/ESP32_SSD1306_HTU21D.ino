#include <WiFi.h>
#include <Wire.h>
#include "time.h"
#include "htu/SparkFunHTU21D.h"
#include "oled/SSD1306Wire.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

/************************** Pins **********************************************/
#define SDA_OLED (4)
#define SCL_OLED (15)
#define RST_OLED (16)


/************************** WiFi Access Point *********************************/
// Please input the SSID and password of WiFi
#define WLAN_SSID       ""
#define WLAN_PASS       ""

WiFiClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;


/***************************** MQTT Setup *************************************/
#define MQTT_SERVER      ""
#define MQTT_SERVERPORT  1883                   // use 8883 for SSL
#define MQTT_USERNAME    "esp32-node"
#define MQTT_KEY         "key"


/******************************** OLED ****************************************/
SSD1306Wire *display;
char S1[30];
char S2[30];
char S3[30];

void drawFrame(OLEDDisplay *display, int16_t x, int16_t y) {
  display->clear();
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(x, y, S1);
  display->drawString(x, y + 15, S2);
  display->drawString(x, y + 30, S3);
  display->display();
}


/****************************** MQTT ******************************************/
// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_SERVERPORT, MQTT_USERNAME, MQTT_KEY);

Adafruit_MQTT_Publish mqtt_pub_time = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME "/Timestamp");
Adafruit_MQTT_Publish mqtt_pub_temp = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME "/Temp");
Adafruit_MQTT_Publish mqtt_pub_humi = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME "/Humid");

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}


/****************************** NTP *******************************************/
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;

/*
  %a Abbreviated weekday name
  %A Full weekday name
  %b Abbreviated month name
  %B Full month name
  %c Date and time representation for your locale
  %d Day of month as a decimal number (01-31)
  %H Hour in 24-hour format (00-23)
  %I Hour in 12-hour format (01-12)
  %j Day of year as decimal number (001-366)
  %m Month as decimal number (01-12)
  %M Minute as decimal number (00-59)
  %p Current locale's A.M./P.M. indicator for 12-hour clock
  %S Second as decimal number (00-59)
  %U Week of year as decimal number,  Sunday as first day of week (00-51)
  %w Weekday as decimal number (0-6; Sunday is 0)
  %W Week of year as decimal number, Monday as first day of week (00-51)
  %x Date representation for current locale
  %X Time representation for current locale
  %y Year without century, as decimal number (00-99)
  %Y Year with century, as decimal number
  %z %Z Time-zone name or abbreviation, (no characters if time zone is unknown)
  %% Percent sign
  You can include text literals (such as spaces and colons) to make a neater display or for padding between adjoining columns.
  You can suppress the display of leading zeroes  by using the "#" character  (%#d, %#H, %#I, %#j, %#m, %#M, %#S, %#U, %#w, %#W, %#y, %#Y)
*/
void updateLocalTime(struct tm &timeinfo) {
  while (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time. Retry in one second.");
    delay(1000);
  }
}


/***************************** Setup ******************************************/
HTU21D htu;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting connecting WiFi.");
  delay(10);

  /* connect to WiFi */
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  /* initial display */
  display = new SSD1306Wire(0x3c, SDA_OLED, SCL_OLED, RST_OLED, GEOMETRY_128_64);
  display->init();
  display->flipScreenVertically();
  display->setFont(ArialMT_Plain_10);
  display->drawString(0, 0, "OLED initial done!");
  display->display();

  htu.begin(Wire, SDA_OLED, SCL_OLED);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}


char time_str[30];

/***************************** Main Loop **************************************/
void loop() {
  struct tm timeinfo;
  updateLocalTime(timeinfo);

  int second = timeinfo.tm_sec;
  int minute = timeinfo.tm_min;
  int hour = timeinfo.tm_hour;
  int day = timeinfo.tm_mday;
  int month = timeinfo.tm_mon + 1;
  int year = timeinfo.tm_year + 1900;
  int weekday = timeinfo.tm_wday +1;
  
  sprintf(time_str, "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
  
  float humd = htu.readHumidity();
  float temp = htu.readTemperature();

  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  Serial.println(">>>>>>>>>>");
  mqtt_pub_time.publish(time_str);
  mqtt_pub_humi.publish(humd);
  mqtt_pub_temp.publish(temp);

  sprintf(S1, "Time: %s", time_str);
  sprintf(S2, "Temperature: %0.1f C", temp);
  sprintf(S3, "Humidity: %0.1f %%", humd);

  Serial.println(time_str);
  Serial.print("Temperature:");
  Serial.print(temp, 1);
  Serial.print("C");
  Serial.print(" Humidity:");
  Serial.print(humd, 1);
  Serial.print("%");

  drawFrame(display, 0, 10);

  Serial.println();
  Serial.println("<<<<<<<<<<");
  delay(10000);
}
