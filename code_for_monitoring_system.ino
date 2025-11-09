#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <ThingSpeak.h>

#define DHTPIN D4
#define DHTTYPE DHT11
#define DUSTPIN A0
#define SENSOR_LED_PIN D2
#define LED1_PIN D5
#define LED2_PIN D6
#define LED3_PIN D7

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 20, 4);

const char* ssid = "iot";
const char* password = "project1w34";
unsigned long myChannelNumber = 2898189;
const char* myWriteAPIKey = "0J20BDH543XBKAUI";

WiFiClient client;

const float VOC_TYPICAL = 0.9;
const float SENSITIVITY_UG_PER_V = 200.0;
const float ADC_MAX_VOLTAGE = 3.3;
const float ADC_MAX_READING = 1023.0;

void setup() {
  lcd.init();
  lcd.backlight();
  lcd.print("Initializing...");
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  lcd.setCursor(0, 1);
  lcd.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
  lcd.setCursor(0, 1);
  lcd.print("WiFi Connected ");
  ThingSpeak.begin(client);
  dht.begin();

  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);
  pinMode(SENSOR_LED_PIN, OUTPUT);
  digitalWrite(SENSOR_LED_PIN, HIGH);
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  digitalWrite(LED3_PIN, LOW);
}

float readGP2Y1010() {
  digitalWrite(SENSOR_LED_PIN, LOW);
  delayMicroseconds(280);
  int raw = analogRead(DUSTPIN);
  delayMicroseconds(40);
  digitalWrite(SENSOR_LED_PIN, HIGH);
  float voltage = (raw / ADC_MAX_READING) * ADC_MAX_VOLTAGE;
  float deltaV = voltage - VOC_TYPICAL;
  if (deltaV < 0) deltaV = 0.0;
  float dust_ugm3 = deltaV * SENSITIVITY_UG_PER_V;
  return dust_ugm3;
}

void loop() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  float dust_ugm3 = readGP2Y1010();
  int dustRounded = (int)round(dust_ugm3);

  Serial.print("Dust (ug/m3): ");
  Serial.println(dust_ugm3);
  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.print(" C  Humidity: ");
  Serial.print(hum);
  Serial.println(" %");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.printf("Temperature: %.1fC", temp);
  lcd.setCursor(0, 1);
  lcd.printf("Humidity: %.1f%%", hum);
  lcd.setCursor(0, 2);
  lcd.printf("Dust: %d ug/m3", dustRounded);

  if (dustRounded > 0 && dustRounded <= 100) {
    digitalWrite(LED1_PIN, HIGH);
    digitalWrite(LED2_PIN, LOW);
    digitalWrite(LED3_PIN, LOW);
  } 
  else if (dustRounded > 100 && dustRounded <= 200) {
    digitalWrite(LED1_PIN, LOW);
    digitalWrite(LED2_PIN, HIGH);
    digitalWrite(LED3_PIN, LOW);
  }
  else if (dustRounded > 200) {
    digitalWrite(LED1_PIN, LOW);
    digitalWrite(LED2_PIN, LOW);
    digitalWrite(LED3_PIN, HIGH);
  } else {
    digitalWrite(LED1_PIN, LOW);
    digitalWrite(LED2_PIN, LOW);
    digitalWrite(LED3_PIN, LOW);
  }

  ThingSpeak.setField(1, temp);
  ThingSpeak.setField(2, hum);
  ThingSpeak.setField(3, dustRounded);

  int httpCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if (httpCode == 200) {
    Serial.println("Data successfully sent to ThingSpeak");
  } else {
    Serial.print("Error sending data to ThingSpeak, code: ");
    Serial.println(httpCode);
  }

  delay(20000);
}
