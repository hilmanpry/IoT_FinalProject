#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL6WrK2GT9k"
#define BLYNK_TEMPLATE_NAME "PantauTuru"
#define BLYNK_AUTH_TOKEN "aXrmkei28nqF2LKoL31UIani__NRwc7c"
#define PERMENIT 1
#define PERJAM 60
#define PERHARI 1440
#define APIKEY "6d0e8e669dc8bb87edfc2129b47ae1c0"
#define KODE_SENSOR1 "1"
#define KODE_SENSOR2 "2"
#define KODE_SENSOR3 "31"

#include <WiFi.h>
#include <HTTPClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Ultrasonic.h>
BlynkTimer timer;

// Informasi koneksi WiFi dan Blynk
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "STTNF-Rooftop";
char pass[] = "Excellent!@#";
String apikey = APIKEY;
const char* serverName = "https://www.pemantauan.com/submission/";

#define DHTPIN 23
#define DHTTYPE DHT11
#define PIR_PIN 5
#define ULTRASONIC_TRIG 16
#define ULTRASONIC_ECHO 4
#define BUZZER_PIN 19
#define RELAY_PIN 15
#define VIRTUAL_TEMPERATURE V8
#define VIRTUAL_HUMIDITY V9

// Sensor inisialisasi
DHT dht(DHTPIN, DHTTYPE);
Adafruit_MPU6050 mpu;
Ultrasonic ultrasonic(ULTRASONIC_TRIG, ULTRASONIC_ECHO);

// Variabel global
float accelX, accelY, accelZ;
float temperature, humidity;
unsigned long lastMovementTime = 0;
int movementCounter = 0;
bool buzzerActive = false;
unsigned long counting;

void setup() {
  Serial.begin(115200);

  // Koneksi ke WiFi dan Blynk
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Menghubungkan ke WiFi...");
  }
  Serial.println("WiFi terhubung.");
  Blynk.begin(auth, ssid, pass);

  // Inisialisasi sensor
  dht.begin();
  if (!mpu.begin()) {
    Serial.println("Gagal menemukan sensor MPU6050!");
    while (1);
  }
  Serial.println("Sensor MPU6050 berhasil diinisialisasi.");

  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  timer.setInterval(1000L, check);
}

void loop() {
  Blynk.run();
  timer.run();

  // Membaca sensor PIR
  int pirState = digitalRead(PIR_PIN);
  if (pirState == HIGH) {
    Serial.println("Gerakan terdeteksi.");
  }

  // Membaca sensor ultrasonik
  float distance = ultrasonic.read();
  if (distance < 20) {
    digitalWrite(BUZZER_PIN, HIGH);
    buzzerActive = true;
    Serial.println("Objek terlalu dekat!");
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerActive = false;
  }

  // Membaca data akselerometer
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  accelX = a.acceleration.x;
  accelY = a.acceleration.y;
  accelZ = a.acceleration.z;

  if (abs(accelX) > 5.5 || abs(accelY) > 5.5 || abs(accelZ - 9.8) > 5.5) {
    movementCounter++;
    lastMovementTime = millis();
    Serial.println("Gerakan terdeteksi oleh MPU6050");
  }

  // Logika gerakan
  unsigned long currentTime = millis();
  if (currentTime - lastMovementTime > 60000) {
    movementCounter = 0;
  }

  // Log data
  Serial.print("Gerakan: "); Serial.println(movementCounter);
  Serial.print("Buzzer Aktif: "); Serial.println(buzzerActive ? "YA" : "TIDAK");
}

void check() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  if (!isnan(temperature) && !isnan(humidity)) {
    Blynk.virtualWrite(VIRTUAL_TEMPERATURE, temperature);
    Blynk.virtualWrite(VIRTUAL_HUMIDITY, humidity);
    aktifkanPemantauan(PERMENIT, temperature, humidity, movementCounter);
  }
}

void aktifkanPemantauan(int frekuensi, float value1, float value2, float value3) {
  String obyek1 = KODE_SENSOR1;
  String obyek2 = KODE_SENSOR2;
  String obyek3 = KODE_SENSOR3;

  if ((millis() - counting) > frekuensi * 60000) {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(serverName);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      String httpRequestData = "apikey=" + apikey;
      httpRequestData += "&obyek1=" + obyek1 + "&value1=" + value1;
      httpRequestData += "&obyek2=" + obyek2 + "&value2=" + value2;
      httpRequestData += "&obyek3=" + obyek3 + "&value3=" + value3;
      int httpResponseCode = http.POST(httpRequestData);

      if (httpResponseCode > 0) {
        Serial.printf("Data terkirim. Respon: %d\n", httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);
      } else {
        Serial.printf("Gagal mengirim data. Error: %s\n", http.errorToString(httpResponseCode).c_str());
      }
      http.end();
    } else {
      Serial.println("WiFi tidak terhubung.");
    }
    counting = millis();
  }
}
