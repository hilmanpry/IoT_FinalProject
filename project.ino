#include <Wire.h>            
#include <Adafruit_MPU6050.h>            
#include <Adafruit_Sensor.h>            
#include <DHT.h>            
#include <Ultrasonic.h>            
#include <MAX30105.h> // Tambahkan pustaka MAX30105    
#include <WiFi.h> // Tambahkan pustaka WiFi  
  
// Konfigurasi WiFi  
const char* ssid = "asramapesan"; // Ganti dengan SSID WiFi Anda  
const char* password = "pesanX&XI"; // Ganti dengan password WiFi Anda  
  
// Pin komponen            
#define DHTPIN 23            
#define DHTTYPE DHT11            
#define PIR_PIN 5            
#define ULTRASONIC_TRIG 16            
#define ULTRASONIC_ECHO 4            
#define BUZZER_PIN 19            
#define RELAY_PIN 15            
    
// Inisialisasi sensor            
DHT dht(DHTPIN, DHTTYPE);            
Adafruit_MPU6050 mpu;            
Ultrasonic ultrasonic(ULTRASONIC_TRIG, ULTRASONIC_ECHO);            
MAX30105 particleSensor; // Buat objek untuk sensor MAX30102    
    
// Variabel global untuk data sensor            
float accelX, accelY, accelZ;            
float gyroX, gyroY, gyroZ;            
int pirState = LOW;            
float temperature, humidity;            
unsigned long lastMovementTime = 0;            
int movementCounter = 0;            
int restlessCounter = 0;            
float distance = 0;            
bool resetMPU = false; // Variabel untuk menandai reset MPU6050            
static float lastDistance = 0; // Variabel untuk menyimpan status jarak terakhir            
static bool objectDetected = false; // Menyimpan status objek terdeteksi          
bool buzzerActive = false; // Status buzzer      
  
void setup() {            
  // Inisialisasi Serial Monitor            
  Serial.begin(115200);            
    
  // Inisialisasi WiFi  
  WiFi.begin(ssid, password);  
  Serial.print("Menghubungkan ke WiFi");  
  while (WiFi.status() != WL_CONNECTED) {  
    delay(500);  
    Serial.print(".");  
  }  
  Serial.println(" Terhubung ke WiFi!");  
  
  // Inisialisasi DHT11            
  dht.begin();            
    
  // Inisialisasi MPU6050            
  if (!mpu.begin()) {            
    Serial.println("Gagal menemukan sensor MPU6050!");            
    while (1);            
  }            
  Serial.println("Sensor MPU6050 berhasil diinisialisasi.");            
    
  // Inisialisasi sensor MAX30102    
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {    
    Serial.println("Sensor MAX30102 tidak ditemukan!");    
    while (1);    
  }    
  Serial.println("Sensor MAX30102 berhasil diinisialisasi.");    
    
  // Atur pengaturan sensor MAX30102    
  particleSensor.setup(); // Atur pengaturan default    
    
  // Inisialisasi pin            
  pinMode(PIR_PIN, INPUT);            
  pinMode(BUZZER_PIN, OUTPUT);            
  pinMode(RELAY_PIN, OUTPUT);            
  digitalWrite(RELAY_PIN, LOW);            
  digitalWrite(BUZZER_PIN, LOW);            
}            
    
void loop() {           
  // Pembatas output            
  Serial.println("_______________||  INFORMASI  ||_________________");           
    
  // Membaca data dari sensor DHT11            
  temperature = dht.readTemperature();            
  humidity = dht.readHumidity();        
    
  // Menilai suhu dan kelembaban        
  String temperatureStatus = (temperature < 18 || temperature > 25) ? "TIDAK NORMAL" : "NORMAL";        
  String humidityStatus = (humidity < 30 || humidity > 60) ? "TIDAK NORMAL" : "NORMAL";        
    
  // Menampilkan kualitas tidur        
  String sleepQuality;        
  if (movementCounter > 10 || restlessCounter > 5) {        
    sleepQuality = "SANGAT GELISAH";        
  } else if (movementCounter > 5) {        
    sleepQuality = "GELISAH";        
  } else {        
    sleepQuality = "TENANG";        
  }        
    
  // Membaca data dari sensor PIR            
  pirState = digitalRead(PIR_PIN);            
  String presenceStatus = (pirState == HIGH) ? "TERDETEKSI" : "TIDAK TERDETEKSI";        
    
  // Menampilkan status objek        
  String objectStatus;        
  if (distance < 10) {        
    objectStatus = "SANGAT DEKAT";        
  } else if (distance < 20) {        
    objectStatus = "MULAI MENDEKAT";        
  } else {        
    objectStatus = "BELUM ADA";        
  }        
    
  // Menampilkan informasi        
  Serial.print("## KUALITAS TIDUR "); Serial.println(sleepQuality);        
  Serial.print("Suhu       : "); Serial.println(temperatureStatus);          
  Serial.print("Kelembaban : "); Serial.println(humidityStatus);          
  Serial.print("Keberadaan : "); Serial.println(presenceStatus);          
  Serial.print("Objek      : "); Serial.println(objectStatus);          
  Serial.println("__");        
    
  // Pembatas output            
  Serial.println("_____");            
    
  // Membaca data dari sensor MPU6050            
  sensors_event_t a, g, temp;            
  mpu.getEvent(&a, &g, &temp);            
  accelX = a.acceleration.x;            
  accelY = a.acceleration.y;            
  accelZ = a.acceleration.z;            
  gyroX = g.gyro.x;            
  gyroY = g.gyro.y;            
  gyroZ = g.gyro.z;            
    
  // Deteksi gerakan tubuh berdasarkan akselerometer            
  if (abs(accelX) > 5.5 || abs(accelY) > 5.5 || abs(accelZ - 9.8) > 5.5) {            
    movementCounter++;            
    lastMovementTime = millis();            
    Serial.println("Gerakan terdeteksi oleh MPU6050");            
  }            
    
  // Reset output MPU6050 jika tidak ada gerakan selama 60 detik            
  unsigned long currentTime = millis();            
  if (currentTime - lastMovementTime > 60000) { // Tidak ada gerakan selama 60 detik            
    resetMPU = true; // Tandai bahwa reset diperlukan            
    Serial.println("Reset output MPU6050 karena tidak ada gerakan.");            
  } else {            
    resetMPU = false; // Reset tidak diperlukan            
  }            
    
  // Jika reset diperlukan, set nilai pembacaan sensor ke nol            
  if (resetMPU) {            
    accelX = 0;            
    accelY = 0;            
    accelZ = 0;            
    gyroX = 0;            
    gyroY = 0;            
    gyroZ = 0;            
  }            
    
  // Pembatas output            
  Serial.println("_____");           
    
  // Membaca data dari sensor Ultrasonik            
  distance = ultrasonic.read();            
              
  // Informasi jika objek mendekat              
  if (distance < 20) {              
    // Cek apakah jarak telah berubah untuk menghindari output yang berulang            
    if (lastDistance != distance) {            
      Serial.print("Objek mulai mendekat! Jarak: ");             
      Serial.print(distance);             
      Serial.println(" cm");              
      lastDistance = distance; // Simpan jarak terakhir            
      objectDetected = true; // Tandai objek terdeteksi          
    }              
              
    // Mengaktifkan buzzer jika objek semakin mendekat              
    if (distance < 10) {              
      digitalWrite(BUZZER_PIN, HIGH); // Aktifkan buzzer jika jarak kurang dari 10 cm              
      buzzerActive = true; // Tandai buzzer aktif        
      Serial.print("Objek sangat dekat! Jarak: ");             
      Serial.print(distance);             
      Serial.println(" cm");             
    } else {              
      digitalWrite(BUZZER_PIN, LOW); // Matikan buzzer jika jarak lebih dari 10 cm              
      buzzerActive = false; // Tandai buzzer tidak aktif        
    }              
  } else {              
    digitalWrite(BUZZER_PIN, LOW); // Matikan buzzer jika tidak ada objek mendekat              
    if (objectDetected) { // Hanya tampilkan jika sebelumnya ada objek terdeteksi          
      Serial.println("Tidak ada objek mendekat.");            
      objectDetected = false; // Reset status objek terdeteksi          
    }          
    lastDistance = 0; // Reset jarak terakhir jika tidak ada objek            
  }               
    
  // Membaca data dari sensor MAX30102    
  long irValue = particleSensor.getIR(); // Membaca nilai IR    
  if (irValue > 50000) { // Jika ada deteksi jari    
    float heartRate = particleSensor.getHeartRate(); // Mendapatkan detak jantung    
    float spo2 = particleSensor.getSpO2(); // Mendapatkan kadar oksigen    
    Serial.print("Detak Jantung: "); Serial.print(heartRate); Serial.println(" bpm");    
    Serial.print("SpO2: "); Serial.print(spo2); Serial.println(" %");    
  } else {    
    Serial.println("Tidak ada jari terdeteksi.");    
  }    
    
  // Analisis kualitas tidur berdasarkan data sensor            
  // Resetting counters jika tidak ada gerakan selama 60 detik            
  if (currentTime - lastMovementTime > 60000) { // Tidak ada gerakan selama 60 detik            
    movementCounter = max(0, movementCounter - 1); // Kurangi movementCounter jika tidak ada gerakan            
    restlessCounter = max(0, restlessCounter - 1); // Kurangi restlessCounter jika tidak ada gerakan            
  }            
    
  // Logika untuk membatasi nilai movementCounter            
  if (movementCounter > 20) {            
    movementCounter = 20; // Batasi nilai maksimum            
  }            
    
  // Pembatas output            
  Serial.println("_______________||  DATA  ||_________________");            
    
  // Log data sensor            
  Serial.print("Accel X: "); Serial.print(accelX);            
  Serial.print(" | Accel Y: "); Serial.print(accelY);            
  Serial.print(" | Accel Z: "); Serial.println(accelZ);            
  Serial.print("Gyro X: "); Serial.print(gyroX);            
  Serial.print(" | Gyro Y: "); Serial.print(gyroY);            
  Serial.print(" | Gyro Z: "); Serial.println(gyroZ);            
  Serial.print("movementCounter : "); Serial.print(movementCounter); Serial.println(" hitungan");            
  Serial.print("restlessCounter : "); Serial.print(restlessCounter); Serial.println(" hitungan");            
  Serial.print("Reset MPU6050 : "); Serial.println(resetMPU ? "YES" : "NO");            
  Serial.print("Buzzer Aktif : "); Serial.println(buzzerActive ? "YA" : "TIDAK");            
  if (buzzerActive) {        
    Serial.println("Mengapa: Objek sangat dekat!");        
  }        
  Serial.print("Jarak Objek  : "); Serial.print(distance); Serial.println(" cm");            
  Serial.print("Suhu               : "); Serial.print(temperature); Serial.println(" °C");            
  Serial.print("Kelembaban         : "); Serial.print(humidity); Serial.println(" %");            
    
  // Status sensor        
  Serial.println("_______________||  SENSOR  ||_________________");         
  Serial.print("MPU6050 : "); Serial.println(mpu.begin() ? "OK" : "ERROR");        
  Serial.print("PIR     : "); Serial.println((pirState == HIGH) ? "OK" : "ERROR");        
  Serial.print("DHT11   : "); Serial.println((isnan(temperature) || isnan(humidity)) ? "ERROR" : "OK");        
  Serial.print("HCSR04  : "); Serial.println((distance >= 0) ? "OK" : "ERROR"); // Status HCSR04 berdasarkan pembacaan jarak      
  Serial.print("RELAY   : "); Serial.println((digitalRead(RELAY_PIN) == HIGH) ? "OK" : "ERROR"); // Status RELAY berdasarkan pembacaan pin      
    
  // Pembatas output            
  Serial.println("===================================================");            
    
  delay(5000); // Delay untuk pembacaan ulang            
}    
