#include <ESP8266WiFi.h>                                    // WiFi library  
#include <ESP8266WebServer.h>                               // WEB server library  
#include <ESP8266HTTPUpdateServer.h>                        // Firmware update library
#include <NTPClient.h>                                      // NTP library
#include <WiFiUdp.h>                                        // NTP update library

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");               // define NTP Client to get time

const char* ssid = "NETGEAR";                               // SSID WiFi
const char* password = "12345678_1Q";                       // password WiFi
#define OTAUSER         ""                                  // login page OTA
#define OTAPASSWORD     ""                                  // password page OTA
#define OTAPATH         "/firmware"                         // url OTA
#define SERVERPORT      80                                  // http port
ESP8266WebServer HttpServer(SERVERPORT);
ESP8266HTTPUpdateServer httpUpdater;

const char* confFile = "def_t.txt";                         // config file
const int LEDpin = 2;                                       // GPIO LED pin
bool LED_on = false;                                        // LED State
String outputLightState = "OFF";                            // variables to store the current light state
const int lightPin = 5;                                     // light relay GPIO pin
String IPaddress;                                           // IP address to request URL
short h_on = 8, m_on = 0, h_off = 23, m_off = 0;            // hours and minutes light turn on and turn off
short autoSwitch = 1;                                       // auto switch light

void setup(void) {
    pinMode(LEDpin, OUTPUT);                                // configures pin as output
    pinMode(lightPin, OUTPUT);
    digitalWrite(LEDpin, LOW);

    WiFi.begin(ssid, password);                             // waiting WiFi connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(250);
        if (LED_on) {
            LED_on = false;
            digitalWrite(LEDpin, LOW);
        }
        else {
            LED_on = true;
            digitalWrite(LEDpin, HIGH);
        }
    }
    digitalWrite(LEDpin, HIGH);

    IPaddress = WiFi.localIP().toString();

    timeClient.begin();                                     // Initialize a NTPClient to get time
    timeClient.setTimeOffset(18000);                        // GMT +5 = 18000

    if (!SPIFFS.begin()) {                                  // init SPIFFS
        return;
    }
    if (SPIFFS.exists(confFile)) {                          // read config
        File f = SPIFFS.open(confFile, "r");
        h_off = f.read();
        m_off = f.read();
        h_on = f.read();
        m_on = f.read();
        autoSwitch = f.read();
        f.close();
    }
    else
    {
        ConfigFileUpdate();                                 // create default config, if doesn't exist
    }

    httpUpdater.setup(&HttpServer, OTAPATH, OTAUSER, OTAPASSWORD);
    HttpServer.onNotFound(handleNotFound);
    HttpServer.on("/", handleRootPath);                     // main page
    HttpServer.on("/h_on_i", handle_h_on_i);                // light on increment hours
    HttpServer.on("/h_on_d", handle_h_on_d);                // light on decrement hours
    HttpServer.on("/m_on_i", handle_m_on_i);                // light on increment minutes
    HttpServer.on("/m_on_d", handle_m_on_d);                // light on decrement minutes
    HttpServer.on("/h_off_i", handle_h_off_i);              // light off increment hours
    HttpServer.on("/h_off_d", handle_h_off_d);              // light off decrement hours
    HttpServer.on("/m_off_i", handle_m_off_i);              // light off increment minutes
    HttpServer.on("/m_off_d", handle_m_off_d);              // light off decrement minutes
    HttpServer.on("/light_on", handle_light_on);            // manual light on
    HttpServer.on("/light_off", handle_light_off);          // manual light off
    HttpServer.on("/auto_on", handle_auto_on);              // auto switch on
    HttpServer.on("/auto_off", handle_auto_off);            // auto switch off
    HttpServer.begin();
}

void loop(void) {
    timeClient.update();
    HttpServer.handleClient();
    if (autoSwitch != 0) {
        TimerEvent();
    }
    delay(25);
    if (LED_on) {
        LED_on = false;
        digitalWrite(LEDpin, LOW);
    }
    else {
        LED_on = true;
        digitalWrite(LEDpin, HIGH);
    }
}

void TimerEvent()
{
    if ((timeClient.getHours() >= h_on) or ((timeClient.getHours() == h_on) and (timeClient.getMinutes() >= m_on))) {
        outputLightState = "ON";
        digitalWrite(lightPin, HIGH);
    }
    if ((timeClient.getHours() >= h_off) or ((timeClient.getHours() == h_off) and (timeClient.getMinutes() >= m_off))) {
        outputLightState = "OFF";
        digitalWrite(lightPin, LOW);
    }
    if ((timeClient.getHours() < h_on) or ((timeClient.getHours() == h_on) and (timeClient.getMinutes() < m_on))) {
        outputLightState = "OFF";
        digitalWrite(lightPin, LOW);
    }
}

void ConfigFileUpdate() {
    File f = SPIFFS.open(confFile, "w");
    f.write(h_off);
    f.write(m_off);
    f.write(h_on);
    f.write(m_on);
    f.write(autoSwitch);
    f.close();
}

void handle_h_on_i() {
    h_on++;
    if (h_on > 23) { h_on = 0; }
    ConfigFileUpdate();
    HttpServer.send(200, "text/html", HTML(0));
}

void handle_h_on_d() {
    h_on--;
    if (h_on < 0) { h_on = 23; }
    ConfigFileUpdate();
    HttpServer.send(200, "text/html", HTML(0));
}

void handle_m_on_i() {
    m_on++;
    if (m_on > 59) {
        m_on = 0;
        h_on++;
        if (h_on > 23) { h_on = 0; }
    }
    ConfigFileUpdate();
    HttpServer.send(200, "text/html", HTML(0));
}

void handle_m_on_d() {
    m_on--;
    if (m_on < 0) {
        m_on = 59;
        h_on--;
        if (h_on < 0) { h_on = 23; }
    }
    ConfigFileUpdate();
    HttpServer.send(200, "text/html", HTML(0));
}

void handle_h_off_i() {
    h_off++;
    if (h_off > 23) { h_off = 0; }
    ConfigFileUpdate();
    HttpServer.send(200, "text/html", HTML(0));
}

void handle_h_off_d() {
    h_off--;
    if (h_off < 0) { h_off = 23; }
    ConfigFileUpdate();
    HttpServer.send(200, "text/html", HTML(0));
}

void handle_m_off_i() {
    m_off++;
    if (m_off > 59) {
        m_off = 0;
        h_off++;
        if (h_off > 23) { h_off = 0; }
    }
    ConfigFileUpdate();
    HttpServer.send(200, "text/html", HTML(0));
}

void handle_m_off_d() {
    m_off--;
    if (m_off < 0) {
        m_off = 59;
        h_off--;
        if (h_off < 0) { h_off = 23; }
    }
    ConfigFileUpdate();
    HttpServer.send(200, "text/html", HTML(0));
}

void handle_light_on() {
    autoSwitch = 0;
    outputLightState = "ON";
    digitalWrite(lightPin, HIGH);
    HttpServer.send(200, "text/html", HTML(0));
}

void handle_light_off() {
    autoSwitch = 0;
    outputLightState = "OFF";
    digitalWrite(lightPin, LOW);
    HttpServer.send(200, "text/html", HTML(0));
}

void handle_auto_on() {
    autoSwitch = 1;
    HttpServer.send(200, "text/html", HTML(0));
}

void handle_auto_off() {
    autoSwitch = 0;
    HttpServer.send(200, "text/html", HTML(0));
}

void handleNotFound() {
    HttpServer.send(404, "text/html", "<head><meta http-equiv=\"Refresh\" content=\"1; URL=http://" + IPaddress + "\">\n</head><body>404</body></html>\n");
}

void handleRootPath() {
    HttpServer.send(200, "text/html", HTML(10));
}

String HTML(short time_ms) {
    String msg = "<!DOCTYPE html> <html>\n";
    msg += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n";
    msg += "<link rel=\"icon\" href=\"data:,\">\n";
    msg += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}";
    msg += "h1{color: #0F3376; padding: 2vh;} p{font-size: 1.5rem;}";
    msg += ".button{display: inline-block; background-color: #999999; border: none; border-radius: 20px; color: white; padding: 4px 12px; text-decoration: none; font-size: 20px; margin: 2px; cursor: pointer;}";
    msg += ".buttonOn{background-color: #00aa00;}.buttonOff{background-color: #aa0000;}</style>\n";
    msg += "<meta http-equiv=\"Refresh\" content=\"" + String(time_ms) + "; URL=http://" + IPaddress + "\">\n</head>\n";
    msg += "<body><h1>Light: <strong> " + outputLightState + "</h1>\n";
    msg += "<p>Time : ";
    if (String(timeClient.getHours()).length() < 2) { msg += "0"; }
    msg += String(timeClient.getHours());
    msg += " : ";
    if (String(timeClient.getMinutes()).length() < 2) { msg += "0"; }
    msg += String(timeClient.getMinutes());
    msg += "</p>\n<p>On :";
    if (String(h_on).length() < 2) { msg += "0"; }
    msg += String(h_on) + " : ";
    if (String(m_on).length() < 2) { msg += "0"; } 
    msg += String(m_on) + "</p>\n";
    msg += "<p><a href = \"/h_on_i\"><button class = \"button\">+</button></a>\n";
    msg += "<a href = \"/h_on_d\"><button class = \"button\">-</button></a>\n";
    msg += "<a href = \"/m_on_i\"><button class = \"button\">+</button></a>\n";
    msg += "<a href = \"/m_on_d\"><button class = \"button\">-</button></a></p>\n";
    msg += "<p>Off :";
    if (String(h_off).length() < 2) { msg += "0"; }
    msg += String(h_off) + " : ";
    if (String(m_off).length() < 2) { msg += "0"; }
    msg += String(m_off) + "</p>\n";
    msg += "<p><a href = \"/h_off_i\"><button class = \"button\">+</button></a>\n";
    msg += "<a href = \"/h_off_d\"><button class = \"button\">-</button></a>\n";
    msg += "<a href = \"/m_off_i\"><button class = \"button\">+</button></a>\n";
    msg += "<a href = \"/m_off_d\"><button class = \"button\">-</button></a></p>\n";
    if (autoSwitch == 0) {
        msg += "<p>AutoMode: <a href = \"/auto_on\"><button class=\"button buttonOn\">ON</button></a></p>\n";
    }
    else {
        msg += "<p>AutoMode: <a href = \"/auto_off\"><button class=\"button buttonOff\">OFF</button></a></p>\n";
    }
    if (outputLightState == "OFF") {
        msg += "<p>Light: <a href = \"/light_on\"><button class=\"button buttonOn\">ON</button></a></p>\n";
    }
    else {
        msg += "<p>Light: <a href = \"/light_off\"><button class=\"button buttonOff\">OFF</button></a></p>\n";
    }
    msg += "<a href = \"/firmware\"><button class = \"button\">Firmware</button></a>\n";
    msg += "</body></html>\n";
    return msg;
}