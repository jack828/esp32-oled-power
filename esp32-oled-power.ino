#include "credentials.h"
#include "font.h"
#include "images.h"
#include <WiFi.h>
#include <SSD1306Wire.h>
#include <OLEDDisplayUi.h>

#define LED_PIN 16

SSD1306Wire display(0x3c, 5, 4);
OLEDDisplayUi ui(&display);

uint64_t setupMillis;


void titleOverlay(OLEDDisplay *display, OLEDDisplayUiState *state) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(Monospaced_plain_10);
  display->drawString(0, 0, (char *)state->userData);
}

void nameOverlay(OLEDDisplay *display, OLEDDisplayUiState *state) {
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(Monospaced_plain_10);
  display->drawString(128, 0, "Power");
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
    } }
};

int LOADING_STAGES_COUNT = sizeof(loadingStages) / sizeof(LoadingStage);

// This array keeps function pointers to all frames
// frames are the single views that slide in
FrameCallback frames[] = { drawFrame1 };

// how many frames are there?
int FRAME_COUNT = sizeof(frames) / sizeof(FrameCallback);

// Overlays are statically drawn on top of a frame eg. a clock
OverlayCallback overlays[] = { titleOverlay, nameOverlay };
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
}


void loop() {
  int remainingTimeBudget = ui.update();

  if (remainingTimeBudget > 0) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi not connected!");
    }
    // You can do some work here
  }
}