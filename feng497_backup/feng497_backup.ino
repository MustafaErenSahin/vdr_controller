#include <ESP32Servo.h>

// --- PIN TANIMLARI ---
const int ESC_PIN = 18;         // ESC Gaz Çıkışı
const int SERVO_PIN = 19;       // Çap Servosu Sinyal Çıkışı
const int TLM_RX_PIN = 16;      // AM32 ESC Telemetry Girişi (ESP32 RX2)

// --- MOTOR TEKNİK BİLGİLERİ ---
const int MOTOR_POLE_PAIRS = 7; // 14 Kutuplu motor için kutup çifti sayısı (14 / 2)
const float MIN_ACTIVE_RPM = 300.0; // Kapalı çevrim kontrol için güvenli minimum devir eşiği

// --- GAZ (THROTTLE) SINIRLARI (ELEKTRİKSEL MİKROSANİYE DEĞERLERİ) ---
const int THROTTLE_MIN = 1000;      // Sıfır gaz, durma ve arming darbesi
const int THROTTLE_MAX = 2000;      // Maksimum donanım tavan darbesi
const int THROTTLE_SAFE_MIN = 1050; // Kapalı çevrimde motorun durmaması/bayılmaması için alt sınır
const int THROTTLE_SAFE_MAX = 1950; // ESC tavan doygunluğunu önleyici üst emniyet sınırı

// --- SERVO SINIRLARI (ELEKTRİKSEL VE MEKANİK) ---
const int SERVO_MIN_US = 500;       // Servonun elektriksel minimum puls genişliği (us)
const int SERVO_MAX_US = 2500;      // Servonun elektriksel maksimum puls genişliği (us)
const int SERVO_MIN_ANGLE = 0;      // Mekanizmanın fiziksel minimum açı sınırı (derece)
const int SERVO_MAX_ANGLE = 180;    // Mekanizmanın fiziksel maksimum açı sınırı (derece)

// --- SERVO VE ESC NESNELERİ ---
Servo escMotor;
Servo diameterServo;

// --- KONTROL VE DURUM DEĞİŞKENLERİ ---
float targetRPM = 0.0;
float actualRPM = 0.0;
int currentThrottle = THROTTLE_MIN;
int currentAngle = SERVO_MIN_ANGLE;

// PI Kontrol Parametreleri
float Kp = 0.08;   // Oransal kazanç
float Ki = 0.02;   // İntegral kazancı
float integralError = 0.0;
const float INTEGRAL_LIMIT = 200.0; // Anti-windup sınırı

// Zamanlama Değişkenleri
unsigned long lastControlTime = 0;
unsigned long lastPlotTime = 0;
const unsigned long CONTROL_INTERVAL_MS = 20; // 50 Hz kontrol çevrimi
const unsigned long PLOT_INTERVAL_MS = 50;    // 20 Hz grafik çıktı çevrimi

// --- AM32 KISS TELEMETRİ OKUMA ---
// 10 Byte standart KISS paketi; Byte 7-8: (eRPM / 100)
void readESCTelemetry() {
  while (Serial2.available() >= 10) {
    byte buffer[10];
    for (int i = 0; i < 10; i++) {
      buffer[i] = Serial2.read();
    }
    
    uint16_t erpmRaw = (buffer[7] << 8) | buffer[8];
    float erpm = erpmRaw * 100.0;
    actualRPM = erpm / MOTOR_POLE_PAIRS;
  }
}

void setup() {
  Serial.begin(115200);                               // Bilgisayar / Serial Plotter
  Serial2.begin(115200, SERIAL_8N1, TLM_RX_PIN, -1); // ESC Telemetri Portu

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  escMotor.setPeriodHertz(50);
  diameterServo.setPeriodHertz(50);

  // Sinyal bacaklarını ve elektriksel sınırları ata
  escMotor.attach(ESC_PIN, THROTTLE_MIN, THROTTLE_MAX);
  diameterServo.attach(SERVO_PIN, SERVO_MIN_US, SERVO_MAX_US);

  // Başlangıç Güvenlik Durumu (Sıfır gaz arming ve minimum açı)
  escMotor.writeMicroseconds(THROTTLE_MIN);
  diameterServo.write(currentAngle);
  delay(3000); // ESC arming ve bip sesleri için bekleme payı
}

void loop() {
  // 1. Seri Port Komutlarını Dinle
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.equalsIgnoreCase("STOP")) {
      targetRPM = 0;
      integralError = 0;
      currentThrottle = THROTTLE_MIN;
      escMotor.writeMicroseconds(THROTTLE_MIN); // Doğrudan donanımsal emniyet tabanı
    } 
    else if (input.charAt(0) == 'R' || input.charAt(0) == 'r') {
      targetRPM = input.substring(1).toFloat();
    } 
    else if (input.charAt(0) == 'C' || input.charAt(0) == 'c') {
      currentAngle = constrain(input.substring(1).toInt(), SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);
      diameterServo.write(currentAngle);
    }
  }

  // 2. ESC Telemetrisini Güncelle
  readESCTelemetry();

  // 3. PI Kapalı Çevrim Kontrol Çevrimi (50 Hz)
  if (millis() - lastControlTime >= CONTROL_INTERVAL_MS) {
    lastControlTime = millis();

    if (targetRPM > MIN_ACTIVE_RPM) {
      float error = targetRPM - actualRPM;
      integralError += error * (CONTROL_INTERVAL_MS / 1000.0);
      integralError = constrain(integralError, -INTEGRAL_LIMIT, INTEGRAL_LIMIT);

      float outputCorrection = (Kp * error) + (Ki * integralError);
      
      currentThrottle = constrain((int)(currentThrottle + outputCorrection), THROTTLE_SAFE_MIN, THROTTLE_SAFE_MAX);
      escMotor.writeMicroseconds(currentThrottle);
    } else {
      currentThrottle = THROTTLE_MIN;
      integralError = 0;
      escMotor.writeMicroseconds(THROTTLE_MIN);
    }
  }

  // 4. Canlı Telemetri ve Plotter Çıktısı (20 Hz)
  if (millis() - lastPlotTime >= PLOT_INTERVAL_MS) {
    lastPlotTime = millis();
    Serial.print("Hedef_RPM:");
    Serial.print(targetRPM);
    Serial.print(" ");
    Serial.print("Gercek_RPM:");
    Serial.print(actualRPM, 0);
    Serial.print(" ");
    Serial.print("Gaz_us:");
    Serial.println(currentThrottle);
  }
}
