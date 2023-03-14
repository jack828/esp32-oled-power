#include "definitions.h"
#include "credentials.h"
#include "font.h"
#include "images.h"
#include "influx.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <SSD1306Wire.h>
#include <OLEDDisplayUi.h>

#define LED_PIN 16

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "uk.pool.ntp.org", 0, 1000);

SSD1306Wire display(0x3c, 5, 4);
OLEDDisplayUi ui(&display);

uint64_t setupMillis;

void titleOverlay(OLEDDisplay *display, OLEDDisplayUiState *state) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(Monospaced_plain_10);
  display->drawString(0, 0, (char *)state->userData);
}

void timeOverlay(OLEDDisplay *display, OLEDDisplayUiState *state) {
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(Monospaced_plain_10);
  display->drawString(128, 0, timeClient.getFormattedTime());
}

void drawFrame1(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y) {
  state->userData = (void *)"Device";
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(Monospaced_plain_10);
  static char tempBuf[237];
  display->drawStringf(0 + x, 10 + y, tempBuf, "RSSI:      %d", WiFi.RSSI());
  display->drawStringf(0 + x, 20 + y, tempBuf, "Uptime:    %ld", millis() - setupMillis);
  display->drawStringf(0 + x, 30 + y, tempBuf, "Free Heap: %d", ESP.getFreeHeap());
  display->drawStringf(0 + x, 40 + y, tempBuf, "Heap Frag: %.2f%", (100 - ESP.getMaxAllocHeap() * 100.0 / ESP.getFreeHeap()));
}

void drawFrame2(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y) {
  state->userData = (void *)"Storage";
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(Monospaced_plain_10);
  display->drawString(0 + x, 10 + y, "Capacity: ");
  display->drawString(0 + x, 20 + y, "Free:     ");
  display->drawString(0 + x, 30 + y, "Used:     ");
  display->drawString(0 + x, 40 + y, "% Free:   ");
}

LoadingStage loadingStages[] = {
  { .process = "Connecting to WiFi",
    .callback = []() {
      /* <WIFI> */
      Serial.print(F("[ WIFI ] Connecting"));
      WiFi.mode(WIFI_STA);
      WiFi.begin(WIFI_SSID, WIFI_PSK);

      uint8_t retryCount = 0;
      do {
        Serial.print(F("."));
        digitalWrite(LED_PIN, LOW);
        delay(250);
        digitalWrite(LED_PIN, HIGH);
        delay(250);
        digitalWrite(LED_PIN, LOW);
        delay(250);
        digitalWrite(LED_PIN, HIGH);
        delay(250);
        if (retryCount++ > 20) {
          Serial.println(
            F("\n[ WIFI ] ERROR: Could not connect to wifi, rebooting..."));
          ESP.restart();
        }
      } while (WiFi.status() != WL_CONNECTED);

      Serial.print(F("\n[ WIFI ] connected, SSID: "));
      Serial.print(WiFi.SSID());
      Serial.print(F(", IP: "));
      Serial.println(WiFi.localIP());
      Serial.println();
      /* </WIFI> */
    } },
  { .process = "Initialising NTP", .callback = []() {
     timeClient.begin();
     while (!timeClient.update()) {
       timeClient.forceUpdate();
     }
     Serial.println(timeClient.getFormattedTime());
   } },
  { .process = "Connecting to InfluxDB", .callback = []() {
     setupInfluxOptions();
     boolean influxOk = validateInfluxConnection();
     if (!influxOk) {
       // literally no point in being here
       ESP.restart();
     }
   } }
};

int LOADING_STAGES_COUNT = sizeof(loadingStages) / sizeof(LoadingStage);

// This array keeps function pointers to all frames
// frames are the single views that slide in
FrameCallback frames[] = { drawFrame1 };

// how many frames are there?
int FRAME_COUNT = sizeof(frames) / sizeof(FrameCallback);

// Overlays are statically drawn on top of a frame eg. a clock
OverlayCallback overlays[] = { titleOverlay, timeOverlay };
int OVERLAY_COUNT = sizeof(overlays) / sizeof(OverlayCallback);


void initialiseUi() {
  // The ESP is capable of rendering 60fps in 80Mhz mode
  // but that won't give you much time for anything else
  // run it in 160Mhz mode or just set it to 30 fps
  ui.setTargetFPS(60);

  ui.setActiveSymbol(activeSymbol);
  ui.setInactiveSymbol(inactiveSymbol);

  ui.setIndicatorPosition(BOTTOM);

  ui.setIndicatorDirection(LEFT_RIGHT);

  ui.setFrameAnimation(SLIDE_LEFT);

  ui.setFrames(frames, FRAME_COUNT);

  ui.setOverlays(overlays, OVERLAY_COUNT);

  ui.init();

  // If needed
  display.flipScreenVertically();

  ui.runLoadingProcess(loadingStages, LOADING_STAGES_COUNT);
}

void setup() {
  Serial.begin(115200);
  setupMillis = millis();
  pinMode(LED_PIN, OUTPUT);

  initialiseUi();

  // Only create the tasks after all setup is done, and we're ready
  xTaskCreate(
    wifiKeepAlive,
    "wifiKeepAlive",  // Task name
    16192,            // Stack size (bytes)
    NULL,             // Parameter
    1,                // Task priority
    NULL              // Task handle
  );
}

/**
 * Task: monitor the WiFi connection and keep it alive
 *
 * When a WiFi connection is established, this task will check it every 10
 * seconds to make sure it's still alive.
 *
 * If not, a reconnect is attempted. If this fails to finish within the timeout,
 * the ESP32 will wait for it to recover and try again.
 */
void wifiKeepAlive(void *parameter) {
  uint8_t failCount = 0;
  for (;;) {
    Serial.print(F("[ WIFI ] Keep alive "));
    bool wifiConnected = (WiFi.RSSI() == 0) || WiFi.status() == WL_CONNECTED;
    Serial.println(wifiConnected ? F("OK") : F("NOT OK"));
    if (wifiConnected) {
      bool influxOk = validateInfluxConnection();
      if (!influxOk) {
        Serial.println(F("[ NODE ] Rebooting..."));
        Serial.flush();
        ESP.restart();
      }
      vTaskDelay(10000 / portTICK_PERIOD_MS);
      continue;
    }

    Serial.println(F("[ WIFI ] Connecting"));
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PSK);

    uint64_t startAttemptTime = millis();

    // Keep looping while we're not connected and haven't reached the timeout
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT_MS) {
      digitalWrite(LED_PIN, HIGH);
      delay(250);
      digitalWrite(LED_PIN, LOW);
      delay(250);
    }

    // When we couldn't make a WiFi connection (or the timeout expired)
    // sleep for a while and then retry.
    if (WiFi.status() != WL_CONNECTED) {
      Serial.print(F("[ WIFI ] Failed"));
      Serial.println(failCount);
      if (failCount++ > 5) {
        Serial.println(
          F("\n[ WIFI ] ERROR: Could not connect to wifi, rebooting..."));
        ESP.restart();
      }
      vTaskDelay(WIFI_RECOVER_TIME_MS / portTICK_PERIOD_MS);
      continue;
    }

    Serial.print(F("\n[ WIFI ] Connected: "));
    Serial.println(WiFi.localIP());
  }
}

void loop() {
  int remainingTimeBudget = ui.update();

  if (remainingTimeBudget > 0) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi not connected!");
    }
    // You can do some work here
    while (!timeClient.update()) {
      timeClient.forceUpdate();
    }
  }
}
