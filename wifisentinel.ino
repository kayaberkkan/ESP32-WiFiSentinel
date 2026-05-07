/*
 * Project: ESP32 WiFi Sentinel
 * Author: https://github.com/kayaberkkan
 * Hardware:
 *   - ESP32 Dev Module
 *   - SSD1306 I2C OLED (SDA: 21, SCL: 22)
 *   - Rotary Encoder (CLK: 18, DT: 19, SW: 23)
 */

#include "AiEsp32RotaryEncoder.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <Wire.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define ROTARY_ENCODER_A_PIN 18
#define ROTARY_ENCODER_B_PIN 19
#define ROTARY_ENCODER_BUTTON_PIN 23
#define ROTARY_ENCODER_VCC_PIN -1
#define ROTARY_ENCODER_STEPS 4

AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(
    ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN,
    ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);

enum AppState {
  MAIN_MENU,
  WIFI_RADAR,
  RISK_REPORT,
  CLONE_REPORT,
  DEVICE_INFO,
  SETTINGS,
  CLONE_DETAIL
};
AppState currentState = MAIN_MENU;
const char *menuItems[] = {"WiFi Radar", "Risk Score", "Clone Check",
                           "Device Info", "Settings"};
const int menuCount = 5;
int currentMenuIndex = 0;
int settingsRow = 0;
unsigned long lastScanTime = 0;
bool isScanning = false;
enum RiskSens { RS_LOW, RS_MEDIUM, RS_HIGH };
enum CloneSens { CS_RELAXED, CS_NORMAL, CS_AGGRESSIVE };
enum ScanInt { SI_2S, SI_5S, SI_10S };

struct Config {
  RiskSens riskSensitivity;
  CloneSens cloneSensitivity;
  ScanInt scanInterval;
} sysConfig = {RS_MEDIUM, CS_NORMAL, SI_5S};

struct ScanResult {
  int totalAPs = 0;
  int openNetworks = 0;
  int strongestRSSI = -100;
  int riskScore = 0;
  String riskLevel = "N/A";
  bool cloneDetected = false;
  bool suspiciousClone = false;
  String clonedSSID = "";
  int cloneCount = 0;
  int rssiDiff = 0;
} lastScan;

const char *riskStrings[] = {"LOW", "MEDIUM", "HIGH"};
const char *cloneStrings[] = {"RELAXED", "NORMAL", "AGGRESSIVE"};
const char *scanStrings[] = {"2s", "5s", "10s"};

unsigned long getScanIntervalMs() {
  switch (sysConfig.scanInterval) {
  case SI_2S:
    return 2000;
  case SI_5S:
    return 5000;
  case SI_10S:
    return 10000;
  default:
    return 5000;
  }
}
void drawHeader(String title) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(title);
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
}
void drawFooter(String hint) {
  display.drawLine(0, 54, 128, 54, SSD1306_WHITE);
  display.setCursor(0, 56);
  display.print(hint);
  display.display();
}
void IRAM_ATTR readRotarySensor() { rotaryEncoder.readEncoder_ISR(); }

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    for (;;)
      ;
  }

  drawHeader("ESP32 WiFi Sentinel");
  display.setCursor(15, 30);
  display.println("Booting System...");
  display.display();
  delay(1500);

  rotaryEncoder.begin();
  rotaryEncoder.setup(readRotarySensor);
  rotaryEncoder.setBoundaries(0, menuCount - 1, true);
  rotaryEncoder.setAcceleration(250);
}

void loop() {
  if (rotaryEncoder.encoderChanged())
    handleEncoderChange();
  if (rotaryEncoder.isEncoderButtonClicked())
    handleButtonClick();

  switch (currentState) {
  case MAIN_MENU:
    drawMenu();
    break;
  case WIFI_RADAR:
    handleWiFiRadar();
    break;
  case RISK_REPORT:
    drawRiskScreen();
    break;
  case CLONE_REPORT:
    drawCloneScreen();
    break;
  case DEVICE_INFO:
    drawDeviceInfo();
    break;
  case SETTINGS:
    drawSettings();
    break;
  case CLONE_DETAIL:
    drawCloneDetailScreen();
    break;
  }
}
void handleEncoderChange() {
  if (currentState == MAIN_MENU)
    currentMenuIndex = rotaryEncoder.readEncoder();
  else if (currentState == SETTINGS)
    settingsRow = rotaryEncoder.readEncoder();
}
void handleWiFiRadar() {
  if (millis() - lastScanTime > getScanIntervalMs() || lastScanTime == 0) {
    performWiFiScan();
    lastScanTime = millis();
  }
  drawRadarScreen();
}
void performWiFiScan() {
  isScanning = true;
  drawHeader("WIFI RADAR");
  display.setCursor(30, 30);
  display.print("SCANNING...");
  display.display();

  int n = WiFi.scanNetworks();
  isScanning = false;
  lastScan.totalAPs = n;
  lastScan.openNetworks = 0;
  lastScan.strongestRSSI = -100;

  for (int i = 0; i < n; ++i) {
    if (WiFi.encryptionType(i) == WIFI_AUTH_OPEN)
      lastScan.openNetworks++;
    if (WiFi.RSSI(i) > lastScan.strongestRSSI)
      lastScan.strongestRSSI = WiFi.RSSI(i);
  }

  analyzeCloneGroups(n);
  WiFi.scanDelete();

  lastScan.riskScore = calculateRiskScore(
      lastScan.totalAPs, lastScan.openNetworks, lastScan.strongestRSSI,
      lastScan.cloneDetected, lastScan.suspiciousClone);
  lastScan.riskLevel = getRiskLevel(lastScan.riskScore);
}
int calculateRiskScore(int apCount, int openCount, int rssi, bool clone,
                       bool susp) {
  int score = openCount * 20;
  if (apCount > 25)
    score += 25;
  else if (apCount > 15)
    score += 15;
  if (rssi > -40)
    score += 15;
  if (clone)
    score += (susp ? 50 : 20);
  if (sysConfig.riskSensitivity == RS_HIGH)
    score += 15;
  if (sysConfig.riskSensitivity == RS_LOW)
    score -= 15;
  return max(0, min(score, 100));
}
void analyzeCloneGroups(int n) {
  lastScan.cloneDetected = false;
  lastScan.suspiciousClone = false;
  lastScan.clonedSSID = "";
  for (int i = 0; i < n; i++) {
    String ssid = WiFi.SSID(i);
    if (ssid == "" || ssid == lastScan.clonedSSID)
      continue;
    int count = 1;
    int maxR = WiFi.RSSI(i);
    int minR = WiFi.RSSI(i);
    for (int j = i + 1; j < n; j++) {
      if (ssid == WiFi.SSID(j)) {
        count++;
        int r = WiFi.RSSI(j);
        if (r > maxR)
          maxR = r;
        if (r < minR)
          minR = r;
      }
    }
    if (count >= 2) {
      lastScan.cloneDetected = true;
      int diff = abs(maxR - minR);
      int threshold =
          (sysConfig.cloneSensitivity == CS_AGGRESSIVE)
              ? 15
              : (sysConfig.cloneSensitivity == CS_RELAXED ? 35 : 25);
      if (maxR > -45 && diff > threshold) {
        lastScan.suspiciousClone = true;
        lastScan.clonedSSID = ssid;
        lastScan.cloneCount = count;
        lastScan.rssiDiff = diff;
        return;
      }
      if (lastScan.clonedSSID == "") {
        lastScan.clonedSSID = ssid;
        lastScan.cloneCount = count;
        lastScan.rssiDiff = diff;
      }
    }
  }
}
String getRiskLevel(int score) {
  if (score <= 30)
    return "LOW";
  if (score <= 60)
    return "MEDIUM";
  return "HIGH";
}
void drawMenu() {
  drawHeader("MAIN MENU");
  for (int i = 0; i < menuCount; i++) {
    display.setCursor(5, 14 + (i * 8));
    if (i == currentMenuIndex) {
      display.fillRect(0, 13 + (i * 8), 128, 9, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }
    display.println(menuItems[i]);
  }
  drawFooter("Rotate:Nav | Click:OK");
}
void drawRadarScreen() {
  drawHeader("WIFI RADAR");
  display.setCursor(0, 14);
  display.print("AP Count: ");
  display.println(lastScan.totalAPs);
  display.setCursor(0, 24);
  display.print("Open Net: ");
  display.println(lastScan.openNetworks);
  display.setCursor(0, 34);
  display.print("Risk Lv : ");
  display.println(lastScan.riskLevel);
  display.setCursor(0, 44);
  display.print("Clones  : ");
  display.println(lastScan.cloneDetected ? "YES" : "NO");
  String hint =
      isScanning
          ? "SCANNING..."
          : "Next in " +
                String((getScanIntervalMs() - (millis() - lastScanTime)) /
                       1000) +
                "s";
  drawFooter(hint);
}
void drawRiskScreen() {
  drawHeader("RISK REPORT");
  display.setCursor(0, 14);
  display.print("Score: ");
  display.print(lastScan.riskScore);
  display.print(" (");
  display.print(lastScan.riskLevel);
  display.println(")");
  display.setCursor(0, 24);
  display.println("Factors:");
  if (lastScan.openNetworks > 0)
    display.println("- Open WiFi Active");
  if (lastScan.cloneDetected)
    display.println("- Clone Detected");
  drawFooter("[Click] to Return");
}
void drawCloneScreen() {
  drawHeader("CLONE CHECK");
  if (!lastScan.cloneDetected) {
    display.setCursor(0, 25);
    display.print("No clones found.");
  } else {
    display.setCursor(0, 14);
    display.print("Target: ");
    display.println(lastScan.clonedSSID.substring(0, 10));
    display.setCursor(0, 24);
    display.print("Stat  : ");
    display.println(lastScan.suspiciousClone ? "SUSPECT" : "CLEAN");
    display.setCursor(0, 34);
    display.print("Diff  : ");
    display.print(lastScan.rssiDiff);
    display.println(" dBm");
  }
  drawFooter("[Click] for Details");
}
void drawDeviceInfo() {
  drawHeader("DEVICE INFO");
  display.setCursor(0, 14);
  display.print("IP : ");
  display.println(WiFi.localIP());
  display.setCursor(0, 24);
  display.print("MAC: ");
  display.println(WiFi.macAddress().substring(0, 12));
  display.setCursor(0, 34);
  display.print("CH : ");
  display.println(WiFi.channel());
  display.setCursor(0, 44);
  display.print("UP : ");
  display.print(millis() / 1000);
  display.println("s");
  drawFooter("[Click] to Return");
}
void drawSettings() {
  drawHeader("SETTINGS");
  const char *opt[] = {"Risk", "Clone", "Interval", "EXIT"};
  for (int i = 0; i < 4; i++) {
    display.setCursor(5, 14 + (i * 10));
    if (settingsRow == i)
      display.print("> ");
    display.print(opt[i]);
    display.print(": ");
    if (i == 0)
      display.print(riskStrings[sysConfig.riskSensitivity]);
    else if (i == 1)
      display.print(cloneStrings[sysConfig.cloneSensitivity]);
    else if (i == 2)
      display.print(scanStrings[sysConfig.scanInterval]);
  }
  drawFooter("Click: Edit | Exit");
}
void drawCloneDetailScreen() {
  drawHeader("CLONE DETAIL");
  if (lastScan.cloneDetected) {
    display.setCursor(0, 14);
    display.print("SSID : ");
    display.println(lastScan.clonedSSID.substring(0, 12));
    display.setCursor(0, 24);
    display.print("Count: ");
    display.println(lastScan.cloneCount);
    display.setCursor(0, 34);
    display.print("Diff : ");
    display.print(lastScan.rssiDiff);
    display.println(" dBm");
    display.setCursor(0, 44);
    display.print("Stat : ");
    display.println(lastScan.suspiciousClone ? "SUSPECT" : "NORMAL");
  } else {
    display.setCursor(0, 25);
    display.print("No clones found.");
  }
  drawFooter("[Click] to Return");
}
void handleButtonClick() {
  static unsigned long lastClickTime = 0;
  if (millis() - lastClickTime < 300)
    return;
  lastClickTime = millis();

  if (currentState == MAIN_MENU) {
    lastScanTime = 0;
    switch (currentMenuIndex) {
    case 0:
      currentState = WIFI_RADAR;
      break;
    case 1:
      currentState = RISK_REPORT;
      break;
    case 2:
      currentState = CLONE_REPORT;
      break;
    case 3:
      currentState = DEVICE_INFO;
      break;
    case 4:
      currentState = SETTINGS;
      settingsRow = 0;
      rotaryEncoder.setBoundaries(0, 3, true);
      break;
    }
  } else if (currentState == SETTINGS) {
    if (settingsRow == 0)
      sysConfig.riskSensitivity =
          (RiskSens)((sysConfig.riskSensitivity + 1) % 3);
    else if (settingsRow == 1)
      sysConfig.cloneSensitivity =
          (CloneSens)((sysConfig.cloneSensitivity + 1) % 3);
    else if (settingsRow == 2)
      sysConfig.scanInterval = (ScanInt)((sysConfig.scanInterval + 1) % 3);
    else if (settingsRow == 3) {
      currentState = MAIN_MENU;
      rotaryEncoder.setBoundaries(0, menuCount - 1, true);
      rotaryEncoder.setEncoderValue(4);
    }
  } else if (currentState == WIFI_RADAR) {
    currentState = CLONE_DETAIL;
  } else {
    currentState = MAIN_MENU;
    rotaryEncoder.setBoundaries(0, menuCount - 1, true);
    rotaryEncoder.setEncoderValue(currentMenuIndex);
  }
}