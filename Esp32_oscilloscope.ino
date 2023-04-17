/*

   Esp32_oscilloscope.ino

   This file is part of Esp32_oscilloscope project: https://github.com/BojanJurca/Esp32_oscilloscope
   Esp32 oscilloscope is also fully included in Esp32_web_ftp_telnet_server_template project: https://github.com/BojanJurca/Esp32_web_ftp_telnet_server_template
  
   Esp32 oscilloscope is built upon Esp32_web_ftp_telnet_server_template. As a stand-alone project it uses only those 
   parts of Esp32_web_ftp_telnet_server_template that are necessary to run Esp32 oscilloscope.

   See https://github.com/BojanJurca/Esp32_web_ftp_telnet_server_template for details on how the servers work.

   Copy all files in the package into Esp32_oscilloscope directory, compile them with Arduino (with FAT partition scheme) and run on ESP32.
   
   October, 23, 2022, Bojan Jurca

*/

// Compile this code with Arduino for:
// - one of ESP32 boards (Tools | Board) and 
// - one of FAT partition schemes (Tools | Partition scheme) and
// - at least 80 MHz CPU (Tools | CPU Frequency)


#include <WiFi.h>
#include <soc/rtc_wdt.h>  // disable watchdog - it gets occasionally triggered when loaded heavily


// When file system is used a disc will be created and formatted. You can later FTP .html and other files to /var/www/html directory, 
// which is the root directory of a HTTP server.
// Some ESP32 boards do not have a flash disk. In this case just comment the following #include line and Oscilloscope.html will be stored in program memory.
// This is where HTTP server will fetch it from.

#include "file_system.h"


// #define network parameters before #including network.h
#define DEFAULT_STA_SSID                          "pachara 2.4G"
#define DEFAULT_STA_PASSWORD                      "0895968635"
#define DEFAULT_AP_SSID                           "" // HOSTNAME - leave empty if you don't want to use AP
#define DEFAULT_AP_PASSWORD                       "" // "YOUR_AP_PASSWORD" - at least 8 characters
// ... add other #definitions from network.h
#include "version_of_servers.h"
#define HOSTNAME                                  "Oscilloscope" // choose your own
#define MACHINETYPE                               "ESP32 Dev Modue" // your board

#include "network.h"

// #define what kind of user management you want before #including user_management.h
#define USER_MANAGEMENT NO_USER_MANAGEMENT // UNIX_LIKE_USER_MANAGEMENT // HARDCODED_USER_MANAGEMENT
#include "user_management.h"                      // file_system.h is needed prior to #including user_management.h in case of UNIX_LIKE_USER_MANAGEMENT

#include "httpServer.hpp"                         // file_system.h is needed prior to #including httpServer.h if you want server also to serve .html and other files

#ifdef __FILE_SYSTEM__
  #include "ftpServer.hpp"                        // file_system.h is also necessary to use ftpServer.h
#else
  #include "progMem.h"                            // for ESP32 boards without file system oscilloscope.html is also available in PROGMEM
#endif

// oscilloscope
#include "oscilloscope.h"

String httpRequestHandler (char *httpRequest, httpConnection *hcn);
void wsRequestHandler (char *wsRequest, WebSocket *webSocket);


#include <soc/gpio_sig_map.h>
#include <soc/io_mux_reg.h>

//Ultrasonic                 
const int trigPin = 4;
const int echoPin = 5;

//AH10 Temp Humidity sensor
#include <Adafruit_AHTX0.h>
Adafruit_AHTX0 aht;

//2.4 TFT LCD Touch Screen
//#include <SPI.h>
//#include <TFT_eSPI.h>
//TFT_eSPI tft = TFT_eSPI();

//Line Notify
#include <HTTPClient.h>
String access_token = "74dKDVRwqaYqyCyfhP2R44bO50XCiz3DHtoxAZ3tM88";
unsigned long previousMillis = 0;
int interval = 600000; // send a message every 3 minutes 180000
String input;

const int BUTTON_PIN = 0;
bool buttonState = false;
bool lastButtonState = false;

//I2C LCD 16*02
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);


void setup () {  

  // disable watchdog if you can afford it - watchdog gets occasionally triggered when loaded heavily
  // rtc_wdt_protect_off ();
  // rtc_wdt_disable ();
  // disableCore0WDT ();
  // disableCore1WDT ();
  // disableLoopWDT ();

  
  Serial.begin (115200);
  Serial.println (String (MACHINETYPE) + " (" + ESP.getCpuFreqMHz () + " MHz) " + HOSTNAME + " SDK: " + ESP.getSdkVersion () + " " + VERSION_OF_SERVERS + ", compiled at: " + String (__DATE__) + " " + String (__TIME__));

  //Start Ultrasonic
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);


  //Start AH10
  Wire.begin(19, 23);
  if (!aht.begin()) {
    Serial.println("Error: Failed to initialize AHT10 sensor");
  }

  //I2C LCD
  lcd.init();
  //lcd.noBacklight();   // ปิด backlight
  lcd.backlight();       // เปิด backlight 

  
  //Start TFT Touch Screen
//  tft.init(); // Initialize the display
//  tft.fillScreen(TFT_BLACK);  // fill screen with black color
//  tft.drawFastHLine(0, 30,  tft.width(), TFT_WHITE);   // draw horizontal white line at position (0, 30)
//
//  tft.setTextColor(TFT_WHITE, TFT_BLACK);  // set text color to white and black background
//  tft.setTextSize(1);                 // text size = 1
//  tft.setCursor(4, 0);               // move cursor to position (4, 0) pixel
//  tft.print("LEARNING KIT");
//  tft.setCursor(19, 15);              // move cursor to position (19, 15) pixel
//  tft.print("IP : ");
//  tft.print("");
//  if( !aht.begin())
//  {  // connection error or device address wrong!
//    tft.setTextColor(TFT_RED, TFT_BLACK);   // set text color to red and black background
//    tft.setTextSize(2);         // text size = 2
//    tft.setCursor(5, 76);       // move cursor to position (5, 76) pixel
//    tft.print("Connection");
//    tft.setCursor(35, 100);     // move cursor to position (35, 100) pixel
//    tft.print("Error");
//    while(1);  // stay here
//  }
//  tft.drawFastHLine(0, 76,  tft.width(), TFT_WHITE);  // draw horizontal white line at position (0, 76)
//  tft.drawFastHLine(0, 122,  tft.width(), TFT_WHITE);  // draw horizontal white line at position (0, 122)
//  tft.setTextColor(TFT_RED, TFT_BLACK);     // set text color to red and black background
//  tft.setCursor(25, 39);              // move cursor to position (25, 39) pixel
//  tft.print("TEMPERATURE =");
//  tft.setTextColor(TFT_CYAN, TFT_BLACK);  // set text color to cyan and black background
//  tft.setCursor(34, 85);              // move cursor to position (34, 85) pixel
//  tft.print("HUMIDITY =");
//  tft.setTextColor(TFT_GREEN, TFT_BLACK);  // set text color to green and black background
//  tft.setCursor(34, 131);              // move cursor to position (34, 131) pixel
//  tft.print("Distance =");
//  tft.setTextSize(2);




ledcSetup (0, 1000, 10);
ledcAttachPin (22, 0); // built-in LED
ledcWrite (0, 307); // 1/3 duty cycle

// PIN_INPUT_ENABLE (GPIO_PIN_MUX_REG [22]);

  
  #ifdef __FILE_SYSTEM__
  
    // FFat.format (); Serial.printf ("\nFormatting FAT file system ...\n\n"); // format flash disk to reset everithing and start from the scratch
    mountFileSystem (true);                                                 // this is the first thing to do - all configuration files reside on the file system

  #endif

  // initializeUsers ();                                                    // creates user management files with root and webadmin users, if they don't exist yet, not needed for NO_USER_MANAGEMENT 

  // deleteFile ("/network/interfaces");                                    // contation STA(tion) configuration       - deleting this file would cause creating default one
  // deleteFile ("/etc/wpa_supplicant/wpa_supplicant.conf");                // contation STA(tion) credentials         - deleting this file would cause creating default one
  // deleteFile ("/etc/dhcpcd.conf");                                       // contains A(ccess) P(oint) configuration - deleting this file would cause creating default one
  // deleteFile ("/etc/hostapd/hostapd.conf");                              // contains A(ccess) P(oint) credentials   - deleting this file would cause creating default one
  startWiFi ();                                                             // starts WiFi according to configuration files, creates configuration files if they don't exist

  // start web server 
  httpServer *httpSrv = new httpServer (httpRequestHandler,                 // a callback function that will handle HTTP requests that are not handled by webServer itself
                                        wsRequestHandler,                   // a callback function that will handle WS requests, NULL to ignore WS requests
                                        (char *) "0.0.0.0",                 // start HTTP server on all available IP addresses
                                        80,                                 // default HTTP port
                                        NULL);                              // we won't use firewallCallback function for HTTP server
  if (!httpSrv && httpSrv->state () != httpServer::RUNNING) dmesg ("[httpServer] did not start.");

  #ifdef __FILE_SYSTEM__

    // start FTP server
    ftpServer *ftpSrv = new ftpServer ((char *) "0.0.0.0",                    // start FTP server on all available ip addresses
                                       21,                                    // default FTP port
                                       NULL);                                 // we won't use firewallCallback function for FTP server
    if (ftpSrv && ftpSrv->state () != ftpServer::RUNNING) dmesg ("[ftpServer] did not start.");

  #endif
  
  // initialize GPIOs you are going to use here:
  // ...

              // ----- examples - delete this code when if it is not needed -----
              #undef LED_BUILTIN
              #define LED_BUILTIN 2
              Serial.printf ("\nGenerating 1 KHz PWM signal on built-in LED pin %i, just for demonstration purposes.\n"
                             "Please, delete this code when if it is no longer needed.\n\n", LED_BUILTIN);
              ledcSetup (0, 1000, 10);        // channel, freqency, resolution_bits
              ledcAttachPin (LED_BUILTIN, 0); // GPIO, channel
              ledcWrite (0, 307);             // channel, 1/3 duty cycle (307 out of 1024 (10 bit resolution))
              int state = 1;
              while(state){
                if (WiFi.status() != WL_CONNECTED){
                  lcd.setCursor(0, 0);
                  lcd.print("Waiting WiFi.");
                  Serial.print(".");
                } else {
                  state = 0;
                  sendLineNotify("IP WEB SOCKET: " + String((char *) WiFi.localIP ().toString ().c_str ()));
                  lcd.setCursor(0, 0);
                  lcd.print(String((char *) WiFi.localIP ().toString ().c_str ()));
                }
              }
              
                       
}


void loop () {
  lcd.setCursor(0, 1);
  buttonState = digitalRead(BUTTON_PIN);
  if (buttonState != lastButtonState) {
    if (buttonState == LOW) {
      interval += 1000;
    } else {
      interval += 1000;
    }
    lastButtonState = buttonState;
  }

  
  //Ultrasonic Read
  long duration, distance;
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = duration / 58.2;
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  lcd.print("D:");
  //lcd.print(distance);
  if (distance < 10){
    lcd.setCursor(2, 1);
    lcd.print(distance);
    lcd.print("  ");
  } else if (distance < 100){
    lcd.setCursor(2, 1);
    lcd.print(distance);
    lcd.print(" ");
  } else {
    lcd.setCursor(2, 1);
    lcd.print(distance);
  }

//  tft.setTextColor(0xFD00, TFT_BLACK);  // set text color to orange and black background
//  tft.setCursor(3, 146);
//  tft.print(distance);
//  tft.setCursor(91, 146);
//  tft.print("cm");

  //Potentiometor Read
  float potenValue = analogRead(33);
  Serial.print("Voltage: ");
  float voltage = map(potenValue, 0, 4096, 0, 5);
  Serial.println(voltage);

  //Photoresistor Read
  int photoValue = analogRead(35);
  Serial.print("LDR: ");
  Serial.println(photoValue);
  
  //AHT10 Read
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);// populate temp and humidity objects with fresh data
  Serial.print("Temperature: "); 
  Serial.print(temp.temperature); 
  Serial.println(" degrees C");
  lcd.setCursor(5, 1);
  lcd.print(" T:");
  lcd.print(int(temp.temperature));
  Serial.print("Humidity: "); 
  Serial.print(humidity.relative_humidity); 
  Serial.println("% rH");
  lcd.setCursor(10, 1);
  lcd.print(" H:");
  lcd.print(int(humidity.relative_humidity));
  lcd.print("%");
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    sendLineNotify("อุณหภูมิตอนนี้ : " + String(temp.temperature));
    sendLineNotify("ความชื้นตอนนี้ : " + String(humidity.relative_humidity));
  }
//  tft.setTextColor(TFT_YELLOW, TFT_BLACK);  // set text color to yellow and black background
//  tft.setCursor(11, 54);
//  tft.print(temp.temperature);
//  tft.drawCircle(89, 56, 2, TFT_YELLOW);  // print degree symbol ( ° )
//  tft.setCursor(95, 54);
//  tft.print("C");
//  tft.setTextColor(TFT_MAGENTA, TFT_BLACK);  // set text color to magenta and black background
//  tft.setCursor(23, 100);
//  tft.print(humidity.relative_humidity);

  

}

void sendLineNotify(String message) {
  HTTPClient http;
  http.begin("https://notify-api.line.me/api/notify");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.addHeader("Authorization", "Bearer " + access_token);
  String data = "message=" + message;
  int httpResponseCode = http.POST(data);
  if (httpResponseCode > 0) {
    Serial.print("Line Notify message sent. HTTP response code: ");
    Serial.println(httpResponseCode);
  } else {
    Serial.print("Line Notify message send failed. HTTP error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();

}


String httpRequestHandler (char *httpRequest, httpConnection *hcn) { 
  // httpServer will add HTTP header to the String that this callback function returns and send everithing to the Web browser (this callback function is suppose to return only the content part of HTTP reply)

  #define httpRequestStartsWith(X) (strstr(httpRequest,X)==httpRequest)

  #ifdef __FILE_SYSTEM__

    // if HTTP request is GET /oscilloscope.html HTTP server will fetch the file but let us redirect GET / and GET /index.html to it as well
    if (httpRequestStartsWith ("GET / ") || httpRequestStartsWith ("GET /index.html ")) {
      hcn->setHttpReplyHeaderField ("Location", "/oscilloscope.html");
      hcn->setHttpReplyStatus ((char *) "307 temporary redirect"); // change reply status to 307 and set Location so client browser will know where to go next
      return "Redirecting ..."; // whatever
    }

  #else

    if (httpRequestStartsWith ("GET / ") || httpRequestStartsWith ("GET /index.html ") || httpRequestStartsWith ("GET /oscilloscope.html ")) {
      return F (oscilloscopeHtml);  
    }

  #endif

  // if the request is GET /oscilloscope.html we don't have to interfere - web server will read the file from file system
  return ""; // HTTP request has not been handled by httpRequestHandler - let the httpServer handle it itself
}

void wsRequestHandler (char *wsRequest, WebSocket *webSocket) {
  
  #define wsRequestStartsWith(X) (strstr(wsRequest,X)==wsRequest)

  if (wsRequestStartsWith ("GET /runOscilloscope")) runOscilloscope (webSocket); // used by oscilloscope.html
}
