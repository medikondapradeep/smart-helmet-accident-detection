#include <Wire.h> 
#include "ADXL345.h" 
#include <TinyGPSPlus.h> 
#include <WiFi.h> 
#include <HTTPClient.h> 
#include <ArduinoJson.h> 
#include <Arduino.h> 
#include <Preferences.h> 
Preferences preferences;
#define LED_PIN 2 
#define BATTERY_PIN 34 // ADC pin where voltage divider is connected 
#define FIRMWARE_VERSION "v1.3.2" 
float maxVoltage = 3.3;
float minVoltage = 2.7;
float voltage;
bool state_pin = 1;
unsigned long restartInterval = 2 * 60 * 1000; // 3 minutes
unsigned long lastRestartTime = 0;
unsigned long lastHeartbeat = 0;
const unsigned long HEARTBEAT_INTERVAL = 20000; // 20 seconds
// Accelerometer 
ADXL345 accelerometer;
#define ADXL345_ADDRESS 0x53 
// GPS on Serial2 
#define GPS_RX 18 // Connect to GPS TX 
#define GPS_TX 19 // Connect to GPS RX 
HardwareSerial GPSserial(2);
TinyGPSPlus gps;
unsigned long lastUpdateId = 0;
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
String BOT_TOKEN = "YOUR_BOT_TOKEN";
String CHAT_ID_1 = "YOUR_CHAT_ID";
String CHAT_ID_2 = "YOUR_CHAT_I
void setup(void) 
{ 
 Serial.begin(115200);
 delay(1000);
 lastRestartTime = millis();
 pinMode(LED_PIN, OUTPUT);
 preferences.begin("telegram", false); // namespace = "telegram"
 lastUpdateId = preferences.getULong("last_id", 0); // default to 0
 Serial.println("Connecting to Wi-Fi...");
 blinkWiFiConnecting();
 WiFi.begin(ssid, password);
 while (WiFi.status() != WL_CONNECTED) { 
 delay(500);
 Serial.print(".");
 }Serial.println("\nConnected to WiFi!");
 indicateWiFiConnected();
 Wire.begin(21, 22); // SDA, SCL
 Serial.println("Initializing ADXL345 with FIFO Trigger Mode...");
 if (!accelerometer.begin()) { 
 Serial.println("Could not find a valid ADXL345 sensor, check wiring!");
 while (1);
 } 
 // ADXL345 Config 
 accelerometer.setFreeFallThreshold(0.35);
 accelerometer.setFreeFallDuration(0.1);
 accelerometer.setActivityThreshold(2.0);
 accelerometer.setInactivityThreshold(2.0);
 accelerometer.setTimeInactivity(5);
 accelerometer.setActivityXYZ(1);
 accelerometer.setInactivityXYZ(1);
 accelerometer.useInterrupt(ADXL345_INT1);
 accelerometer.setRange(ADXL345_RANGE_16G);
 accelerometer.setDataRate(ADXL345_DATARATE_100HZ);
 // FIFO Trigger Mode 
 Wire.beginTransmission(ADXL345_ADDRESS);
 Wire.write(0x38); // FIFO_CTL
 Wire.write(0b11111111); // Trigger mode, INT1, 31 post-trigger samples 
 if (Wire.endTransmission() != 0) { 
 Serial.println("FIFO rearm failed (I2C issue)");
 }Serial.println("Initializing GPS...");
 GPSserial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
 Serial.println("Setup complete.\n");
} 
void loop(void) { 
 checkBatteryAndSend();
 uint32_t heap = ESP.getFreeHeap();
 if(heap<225000) 
 { 
 ESP.restart();
 } 
 Vector norm = accelerometer.readNormalize();
 Activites activ = accelerometer.readActivites();
 smartDelay(200); // While reading GPS serial
 if (activ.isFreeFall || activ.isActivity) { 
 blinkActivityDetected();
 if (activ.isActivity) Serial.println("Activity Detected");
 if (activ.isFreeFall) Serial.println("Free Fall Detected!");
 Serial.println("Reading FIFO Buffer (32 entries):");
 for (int i = 0; i < 32; i++) {
 Vector norm = accelerometer.readNormalize();
 Serial.printf("Sample %d: X = %.2f | Y = %.2f | Z = %.2f\n", 
 i+1, norm.XAxis, norm.YAxis, norm.ZAxis);
 delay(10);
 } 
 // Clear FIFO 
Wire.beginTransmission(ADXL345_ADDRESS);
 Wire.write(0x38);
 Wire.write(0x00);
 Wire.endTransmission();
 // Re-arm FIFO Trigger Mode 
 Wire.beginTransmission(ADXL345_ADDRESS);
 Wire.write(0x38);
 Wire.write(0b11111111);
 Wire.endTransmission();
 Serial.println("FIFO cleared and rearmed.\n");
 if (gps.location.isValid()) { 
 Serial.println(F("Latitude Longitude Date Time "));
 Serial.println(F(" (deg) (deg) Age "));
 Serial.println(F("-----------------------------------------"));
 printFloat(gps.location.lat(), gps.location.isValid(), 11, 6);
 printFloat(gps.location.lng(), gps.location.isValid(), 12, 6);
 printDateTime(gps.date, gps.time);
 Serial.println();
 } else { 
 Serial.println("GPS data not available. Waiting for fix...");
 } 
 char msg[512];
 snprintf(msg, sizeof(msg), 
 "SOS ALERT!\n" 
 "%s" "%s" "%s" "%s" "%s", 
 activ.isActivity ? "Activity Detected.\n" : "", 
 activ.isFreeFall ? "Free Fall Detected.\n" : "",gps.location.isValid() ? "굗굙굘 Location:\n" : "", 
 gps.location.isValid() ? 
 ("Lat: " + String(gps.location.lat(), 6) + "\nLng: " + String(gps.location.lng(), 6) + 
"\n").c_str() : "", 
 gps.location.isValid() ? 
 ("舏 https://maps.google.com/?q=" + String(gps.location.lat(), 6) + "," + 
String(gps.location.lng(), 6)).c_str() 
 : " GPS location not available." 
 ); 
 sendTelegramToAll(String(msg));
 } 
 checkTelegramCommands();
 digitalWrite(LED_PIN, state_pin);
 state_pin = !state_pin;
} 
// Read GPS during delays 
static void smartDelay(unsigned long ms){ 
 unsigned long start = millis();
 while (millis() - start < ms) { 
 while (GPSserial.available()) 
 gps.encode(GPSserial.read());
 } 
} 
static void printFloat(float val, bool valid, int len, int prec) { 
 if (!valid) { 
 while (len-- > 1) Serial.print('*');
 Serial.print(' ');
} else { 
 Serial.print(val, prec);
 int vi = abs((int)val);
 int flen = prec + (val < 0.0 ? 2 : 1);
 flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
 for (int i=flen; i<len; ++i)
 Serial.print(' ');
 } 
 smartDelay(0);
} 
static void printInt(unsigned long val, bool valid, int len) { 
 char sz[32] = "*****************";
 if (valid) sprintf(sz, "%ld", val);
 sz[len] = 0;
 for (int i=strlen(sz); i<len; ++i) sz[i] = ' ';
 if (len > 0) sz[len-1] = ' ';
 Serial.print(sz);
 smartDelay(0);
} 
static void printDateTime(TinyGPSDate &d, TinyGPSTime &t) { 
 int year = d.year(), month = d.month(), day = d.day();
 int hour = t.hour(), minute = t.minute(), second = t.second();
 if (!d.isValid() || !t.isValid()) { 
 Serial.print(F("********** ******** "));
 printInt(d.age(), false, 5);
 return;
 } 
// UTC to IST 
 minute += 30; hour += 5;
 if (minute >= 60) { minute -= 60; hour++; }
 if (hour >= 24){ 
 hour -= 24;
 static const int daysInMonth[] = {31,28,31,30,31,30,31,31,30,31,30,31};
 int dim = daysInMonth[month - 1];
 if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) dim = 29;
 day++;
 if (day > dim) { day = 1; month++; if (month > 12) { month = 1; year++; } }
 
 char sz[32];
 sprintf(sz, "%02d/%02d/%02d %02d:%02d:%02d ", month, day, year, hour, minute, second);
 Serial.print(sz);
 printInt(d.age(), d.isValid(), 5);
} 
void sendTelegramToAll(String msg) { 
 digitalWrite(LED_PIN, LOW);
 sendTelegram(msg, CHAT_ID_1);
 sendTelegram(msg, CHAT_ID_2);
 digitalWrite(LED_PIN, HIGH);
} 
void sendTelegram(String msg, String chat_id) { 
 HTTPClient http;
 String url = "https://api.telegram.org/bot" + BOT_TOKEN + "/sendMessage";
 http.begin(url);
 http.addHeader("Content-Type", "application/json");
String payload = "{\"chat_id\":\"" + chat_id + "\",\"text\":\"" + msg + 
"\",\"parse_mode\":\"Markdown\"}";
 http.setTimeout(5000); // 5 seconds max
 int responseCode = http.POST(payload);
 http.end();
} 
String urlencode(String str) { 
 String encoded = "";
 char c;
 char code0, code1;
 for (int i = 0; i < str.length(); i++) { 
 c = str.charAt(i);
 if (isalnum(c)) { 
 encoded += c;
 } 
else { 
 code1 = (c & 0xf) + '0';
 if ((c & 0xf) > 9) code1 = (c & 0xf) - 10 + 'A';
 code0 = ((c >> 4) & 0xf) + '0';
 if (((c >> 4) & 0xf) > 9) code0 = ((c >> 4) & 0xf) - 10 + 'A';
 encoded += '%';
 encoded += code0;
 encoded += code1;
 } 
 } 
 return encoded;
}void blinkWiFiConnecting() { 
 for (int i = 0; i < 5; i++) {
 digitalWrite(LED_PIN, HIGH);
 delay(500);
 digitalWrite(LED_PIN, LOW);
 delay(500);
 } 
} 
void indicateWiFiConnected() { 
 digitalWrite(LED_PIN, HIGH); // Solid ON
} 
void blinkActivityDetected() { 
 for (int i = 0; i < 3; i++) {
 digitalWrite(LED_PIN, HIGH);
 delay(100);
 digitalWrite(LED_PIN, LOW);
 delay(100);
 } 
} 
void blinkTelegramSending() { 
 for (int i = 0; i < 3; i++) {
 digitalWrite(LED_PIN, HIGH);
 delay(200);
 digitalWrite(LED_PIN, LOW);
 delay(200);
 } 
}void checkTelegramCommands() { 
 HTTPClient http;
 String url = "https://api.telegram.org/bot" + BOT_TOKEN + "/getUpdates?offset=" + 
String(lastUpdateId + 1);
 http.begin(url);
 http.setTimeout(5000); // 5 seconds max
 int httpCode = http.GET();
 if (httpCode == 200) { 
 String payload = http.getString();
 DynamicJsonDocument doc(2048);
 deserializeJson(doc, payload);
 JsonArray results = doc["result"].as<JsonArray>();
 for (JsonObject update : results) { 
// lastUpdateId = update["update_id"];
 String text = update["message"]["text"];
 String chatId = update["message"]["chat"]["id"].as<String>();
 text.toLowerCase();
 int batteryVoltage = readBatteryVoltage();
 if (text == "location") { 
 if (gps.location.isValid()) { 
 String locMsg = "굗굙굘 Current Location:\n";
 locMsg += "Lat: " + String(gps.location.lat(), 6) + "\n";
 locMsg += "Lng: " + String(gps.location.lng(), 6) + "\n"; 
 locMsg += "舏 https://maps.google.com/?q=" + String(gps.location.lat(), 6) + "," + 
String(gps.location.lng(), 6)+ " %\n";
 locMsg += "꺆꺈꺇 Battery: " + String(batteryVoltage) + " %\n";
 sendTelegram(locMsg, chatId);
 } else { 
 sendTelegram(" GPS fix not available. Try again in a moment.", chatId);
 } 
 } 
 else if (text == "restart" || text == "reboot" || text == "esp restart") 
 { 
 String statusMsg = "脥� System Restarting in 2 sec.\n";
 // Save update ID to preferences before restarting 
 lastUpdateId = update["update_id"];
 preferences.putULong("last_id", lastUpdateId);
 sendTelegram(statusMsg, chatId);
 delay(2000);
 ESP.restart();
 return;
 } 
 else if (text == "status") { 
 String statusMsg = "脥� System is online.\n";
 statusMsg += "WiFi: " + WiFi.SSID() + "\n";
 statusMsg += "IP: " + WiFi.localIP().toString() + "\n";
 statusMsg += "Battery: " + String(batteryVoltage) + " %\n";
 sendTelegram(statusMsg, chatId);
 } 
 else if (text == "hi" || text == "hello" || text == "hey" || text == "hlo") { 
 String statusMsg = "脥� Smart Helmet is online and functioning.\n";
 statusMsg += "脥긧긨긩긪 WiFi: " + WiFi.SSID() + "\n";
 statusMsg += "곸곹곺곻과곽 IP Address: " + WiFi.localIP().toString() + "\n";statusMsg += "Battery: " + String(batteryVoltage) + " %\n";
 if (gps.location.isValid()) { 
 statusMsg += "굗굙굘 Last Known Location:\n";
 statusMsg += "Lat: " + String(gps.location.lat(), 6) + "\n";
 statusMsg += "Lng: " + String(gps.location.lng(), 6) + "\n";
 statusMsg += "舏 https://maps.google.com/?q=" + String(gps.location.lat(), 6) + "," + 
String(gps.location.lng(), 6);
 } else { 
 statusMsg += " GPS fix not available.";
 } 
 
 sendTelegram(statusMsg, chatId);
 } 
 else if(text == "battery" || text == "bat" || text == "power") 
 { 
 String batteryMsg = "꺆꺈꺇 Battery Percentage: " + String(batteryVoltage) + "%";
 sendTelegram(batteryMsg, chatId);
 } 
 else if(text == "voltage") 
 { 
 String batvoltage = "꺆꺈꺇 Battery Voltage: " + String(voltage, 2) + " V";
 sendTelegram(batvoltage, chatId);
 } 
 else if (text == "info" || text == "system info") { 
 String info = "脥� *System Info Report:*\n\n"; 
 info += "궯궰궱궲궳 *MCU:*\n";
 info += "• Chip: " + String(ESP.getChipModel()) + "\n";
56 | P a g e
 info += "• Rev: " + String(ESP.getChipRevision()) + " | Cores: " + 
String(ESP.getChipCores()) + "\n";
 info += "• Flash: " + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB @ " + 
String(ESP.getFlashChipSpeed() / 1000000) + " MHz\n\n";
 info += "• *Firmware Version:* " + String(FIRMWARE_VERSION) + "\n\n";
 // Network 
 info += "脥긧긨긩긪 *Network:*\n";
 info += "• WiFi: " + WiFi.SSID() + "\n";
 info += "• IP: " + WiFi.localIP().toString() + "\n";
 info += "• RSSI: " + String(WiFi.RSSI()) + " dBm\n";
 info += "• MAC: " + WiFi.macAddress() + "\n\n";
 // Battery 
 info += "꺆꺈꺇 *Battery:*\n";
 info += "• Battery Percentage: " + String(batteryVoltage) + "%"+ "\n";
 info += "• Battery Voltage: " + String(voltage, 2) + " V"+ "\n\n";
 // Location 
 if (gps.location.isValid()) { 
 info += "굗굙굘 *GPS Location:*\n";
 info += "• Lat: " + String(gps.location.lat(), 6) + "\n";
 info += "• Lng: " + String(gps.location.lng(), 6) + "\n";
 info += "• 舏 https://maps.google.com/?q=" + String(gps.location.lat(), 6) + "," + 
String(gps.location.lng(), 6) + "\n\n";
 } else { 
 info += "굗굙굘 *GPS:* No fix\n\n";
 } 
 // Memory 
 info += "귑귒귓귔귕귖 *Memory:*\n";
info += "• Free Heap: " + String(ESP.getFreeHeap()) + " bytes\n\n"; 
 sendTelegram(info, chatId);
 } 
// lastUpdateId = update["update_id"];
 lastUpdateId = update["update_id"];
 preferences.putULong("last_id", lastUpdateId);
 } 
 } 
 http.end();
} 
float readBatteryVoltage() { 
 int adcValue = analogRead(BATTERY_PIN);
 voltage = adcValue * (3.3 / 4095.0); // Convert ADC to voltage
 int batteryPercentage = ((voltage - minVoltage) / (maxVoltage - minVoltage)) * 100;
 return batteryPercentage;
} 
void checkBatteryAndSend() { 
 int battVolt = readBatteryVoltage();
// Serial.print("Battery Voltage: ");
// Serial.print(voltage);
// Serial.println(" V");
 if (voltage < minVoltage) { 
 String msg = " Low Battery Alert!\nBattery Voltage: " + String(battVolt, 2) + " V";
 sendTelegramToAll(msg); // Reuse your function
 } 
}
