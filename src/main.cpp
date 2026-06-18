#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <DHT.h>
#include <SPI.h>
#include <ESP32Servo.h>

// PINY
#define SOIL1       34
#define SOIL2       35
#define DHTPIN      15
#define SERVO_PIN   13
#define BUTTON_PIN  14

// DISPLEJ
#define TFT_CS     -1
#define TFT_RST     4
#define TFT_DC      16

// SPI piny ESP32 VSPI
#define TFT_SCK     18
#define TFT_MOSI    23

#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
Servo servoOkno;

bool manualOpen = false;
bool autoOpen = false;
bool posledniStavOkna = false;

int lastButtonReading = HIGH;
int stableButtonState = HIGH;

unsigned long posledniCasTlacitka = 0;
const unsigned long debounceDelay = 50;

unsigned long posledniCasMereni = 0;
const unsigned long intervalMereni = 2000;

void nastavServo(int stupne) {
  servoOkno.write(stupne);
}

void vykresli(float t, float h, int hlina1, int hlina2, bool okno) {
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_BLUE);
  tft.setTextSize(3);

  tft.setCursor(10, 20);
  tft.print("Tepl: ");
  if (isnan(t)) {
    tft.print("ERR");
  } else {
    tft.print(t, 1);
    tft.print(" C");
  }

  tft.setCursor(10, 70);
  tft.print("Vlh:  ");
  if (isnan(h)) {
    tft.print("ERR");
  } else {
    tft.print(h, 0);
    tft.print(" %");
  }

  tft.setCursor(10, 120);
  tft.print("Hl1:  ");
  tft.print(hlina1);
  tft.print(" %");

  tft.setCursor(10, 170);
  tft.print("Hl2:  ");
  tft.print(hlina2);
  tft.print(" %");

  tft.setCursor(10, 215);
  tft.setTextSize(2);
  tft.print("Okno: ");
  tft.print(okno ? "OTEVRENO" : "ZAVRENO");
}

void setup() {
  Serial.begin(115200);

  analogSetAttenuation(ADC_11db);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  dht.begin();

  SPI.begin(TFT_SCK, -1, TFT_MOSI, TFT_CS);

  tft.init(240, 240, SPI_MODE3);
  tft.setRotation(2);
  tft.fillScreen(ST77XX_BLACK);

  servoOkno.setPeriodHertz(50);
  servoOkno.attach(SERVO_PIN, 544, 2400);
  nastavServo(0);

  tft.setTextColor(ST77XX_BLUE);
  tft.setTextSize(2);
  tft.setCursor(20, 100);
  tft.print("Startuji...");
}

void loop() {
  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonReading) {
    posledniCasTlacitka = millis();
  }

  if ((millis() - posledniCasTlacitka) > debounceDelay) {
    if (reading != stableButtonState) {
      stableButtonState = reading;

      if (stableButtonState == LOW) {
        manualOpen = !manualOpen;
      }
    }
  }

  lastButtonReading = reading;

  unsigned long aktualniCas = millis();

  if (aktualniCas - posledniCasMereni >= intervalMereni) {
    posledniCasMereni = aktualniCas;

    float t = dht.readTemperature();
    float h = dht.readHumidity();

    int rawS1 = analogRead(SOIL1);
    int rawS2 = analogRead(SOIL2);

    rawS1 = constrain(rawS1, 1500, 4095);
    rawS2 = constrain(rawS2, 1500, 4095);

    int h_hlina1 = map(rawS1, 4095, 1500, 0, 100);
    int h_hlina2 = map(rawS2, 4095, 1500, 0, 100);

    h_hlina1 = constrain(h_hlina1, 0, 100);
    h_hlina2 = constrain(h_hlina2, 0, 100);

    if (!isnan(t) && !isnan(h)) {
      if (t > 28.0 || h > 80.0) {
        autoOpen = true;
      } else if (t < 26.0 && h < 75.0) {
        autoOpen = false;
      }
    }

    bool aktualniStavOkna = manualOpen || autoOpen;

    if (aktualniStavOkna != posledniStavOkna) {
      posledniStavOkna = aktualniStavOkna;

      if (aktualniStavOkna) {
        nastavServo(90);
      } else {
        nastavServo(0);
      }
    }

    Serial.printf(
      "S1 raw: %d | S2 raw: %d | Hl1: %d %% | Hl2: %d %% | T: %.1f | H: %.0f | Okno: %s\n",
      rawS1,
      rawS2,
      h_hlina1,
      h_hlina2,
      t,
      h,
      aktualniStavOkna ? "OTEVRENO" : "ZAVRENO"
    );

    vykresli(t, h, h_hlina1, h_hlina2, aktualniStavOkna);
  }
}
