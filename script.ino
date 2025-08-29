#include <WiFi.h>
#include <HTTPClient.h>
#include <LiquidCrystal.h>

/* ----------- WiFi ----------- */
const char* SSID     = "realme 8";
const char* PASSWORD = "moaz123456";

/* -------- ThingSpeak -------- */
const char* WRITE_API_KEY = "YM57FG1NG7OW2N1D";  // only key needed for writes
const char* TS_UPDATE_URL = "http://api.thingspeak.com/update";

/* -------- Thresholds -------- */
const int FAN_ON_C  = 50;   // >50C -> fan ON
 const int ALERT_C=70 ;// 70 -> buzzer and fan

LiquidCrystal lcd(22, 23, 21, 19, 18, 2);

/* -------- Pins -------- */
const int TEMP_ANALOG_PIN = 34;  // analog input for temperature
const int FAN_PIN         = 4;   // fan control (via transistor/relay)
const int BUZZER_PIN      = 5;   // buzzer alert

/* ---------- helpers ---------- */
int readTemperatureAnalog() {
  int analogVal = analogRead(TEMP_ANALOG_PIN); // 0..4095
  float voltage = analogVal * (3.3 / 4095.0);  // convert ADC to volts

  // Reference point: ADC=250 -> 25°C
  float vRef = 250 * (3.3 / 4095.0);  // ≈ 0.451V

  // Each +10mV (0.01V) = +1°C
  float tempC = 25.0 + (voltage - vRef) / 0.01;

  // Clamp temperature
  if (tempC < 0) tempC = 0;
  if (tempC > 127) tempC = 127;

  return (int)tempC;
}


// function to upload data into cloud (thingspeak)
void thingspeakPublish(int temp, int state) {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = String(TS_UPDATE_URL) +
               "?api_key=" + WRITE_API_KEY +
               "&field1=" + String(temp) +
               "&field2=" + String(state);

  http.begin(url);
  int code = http.GET();
  // optional: Serial.println(code);
  http.end();
}
// used to write on lcd
void showOnLCD(int temp, int state) {
  // ---- First line: Temperature ----
  lcd.setCursor(0, 0);
  // Overwrite entire line with spaces first
  lcd.print("Temp:            "); // 16 chars total
  // Move cursor back to start of line
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temp);
  lcd.print((char)223); // degree symbol
  lcd.print("C");

  // ---- Second line: State ----
  lcd.setCursor(0, 1);
  // Overwrite entire line with spaces
  lcd.print("                "); // 16 spaces
  lcd.setCursor(0, 1);

  if (state == 2) {
    lcd.print("ALERT: >70C     "); // pad to 16 chars
  } else if (state == 1) {
    lcd.print("Fan ON (>50C)   "); // pad to 16 chars
  } else {
    lcd.print("Normal (<50C)   "); // pad to 16 chars
  }
}

int currdelay=0,clouddelay=0;

/* -------- Averaging buffer -------- */
// we get the average of some samples to be more accurate in sensor readings
#define NUM_SAMPLES 60
int samples[NUM_SAMPLES];      // circular buffer
int sampleIndex = 0;
int sumSamples = 0;

void setup() {
  Serial.begin(115200);

  // analog pin setup
  pinMode(TEMP_ANALOG_PIN, INPUT);

  // outputs
  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // LCD
  
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Booting...");

  // WiFi
  WiFi.begin(SSID, PASSWORD);
  lcd.setCursor(0,1);
  lcd.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }
  lcd.clear();
  lcd.print("WiFi connected");
  delay(4000);
}

void loop() {
  int temp = readTemperatureAnalog();   // read every 50 ms

  // update circular buffer
  sumSamples -= samples[sampleIndex];   // remove old value
  samples[sampleIndex] = temp;          // insert new value
  sumSamples += temp;                   // add new value
  sampleIndex = (sampleIndex + 1) % NUM_SAMPLES;

  currdelay += 50;
  clouddelay+=50;
  // every 3000 ms (60 readings) -> update LCD and logic
  if (currdelay >= 3000) {
    currdelay = 0;
    int avgTemp = sumSamples / NUM_SAMPLES;

    // decide state
    int state = 0;
    if (avgTemp > ALERT_C) {
      state = 2;
      digitalWrite(FAN_PIN, HIGH);
      digitalWrite(BUZZER_PIN, HIGH); // buzzer on
    } else if (avgTemp > FAN_ON_C) {
      state = 1;
      digitalWrite(FAN_PIN, HIGH);
      digitalWrite(BUZZER_PIN, LOW);
    } else {
      state = 0;
      digitalWrite(FAN_PIN, LOW);
      digitalWrite(BUZZER_PIN, LOW);
    }

    showOnLCD(avgTemp, state);
    if(clouddelay>16000){
thingspeakPublish(avgTemp, state);
clouddelay=0;
    }
    Serial.print("Temp=");
    Serial.print(avgTemp);
    Serial.print("C  |  State=");
    Serial.println(state);
  }

  delay(50);
}
