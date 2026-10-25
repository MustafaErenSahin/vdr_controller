#include <ESP32Servo.h>

// --- PIN TANIMLARI ---
const int ESC_PIN = 18;         // ESC Gaz Çıkışı (Artık doğrudan Governor Setpoint sinyali)
const int SERVO_PIN = 19;       // Çap Servosu Sinyal Çıkışı
const int TLM_RX_PIN = 16;      // AM32 ESC Telemetry Girişi (ESP32 RX2)

// --- MOTOR VE TELEMETRİ BİLGİLERİ ---
const int MOTOR_POLE_PAIRS = 7; // 14 Kutuplu motor için 7 çift

// --- GAZ / GOVERNOR HEDEF SINIRLARI (MİKROSANİYE) ---
const int THROTTLE_MIN = 1000;  // Motor durma / Arming / %0 Governor hedefi
const int THROTTLE_MAX = 2000;  // Maksimum devir / %100 Governor hedefi

// --- SERVO SINIRLARI ---
const int SERVO_MIN_US = 500;
const int SERVO_MAX_US = 2500;
const int SERVO_MIN_ANGLE = 0;
const int SERVO_MAX_ANGLE = 180;

// --- NESNELER ---
Servo escMotor;
Servo diameterServo;

// --- DURUM DEĞİŞKENLERİ ---
float actualRPM = 0.0;
int currentThrottle = THROTTLE_MIN; // ESC'ye gönderilen anlık Governor setpoint'i
int currentAngle = SERVO_MIN_ANGLE;

// Zamanlayıcı
unsigned long lastPlotTime = 0;
const unsigned long PLOT_INTERVAL_MS = 50; // 20 Hz telemetri/grafik çıktısı

// --- AM32 TELEMETRİ OKUMA (SADECE İZLEME VE DOĞRULAMA AMAÇLI) ---
void readESCTelemetry() {
  while (Serial2.available() >= 10) {
    byte buffer[10];
    for (int i = 0; i < 10; i++) {
      buffer[i] = Serial2.read();
    }
    
    // Byte 7-8: Elektriksel RPM / 100
    uint16_t erpmRaw = (buffer[7] << 8) | buffer[8];
    float erpm = erpmRaw * 100.0;
    actualRPM = erpm / MOTOR_POLE_PAIRS;
  }
}

void setup() {
  Serial.begin(115200);                               // PC / Serial Plotter
  Serial2.begin(115200, SERIAL_8N1, TLM_RX_PIN, -1); // ESC Telemetri hattı

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  escMotor.setPeriodHertz(50);
  diameterServo.setPeriodHertz(50);

  escMotor.attach(ESC_PIN, THROTTLE_MIN, THROTTLE_MAX);
  diameterServo.attach(SERVO_PIN, SERVO_MIN_US, SERVO_MAX_US);

  // Başlangıç Güvenliği: ESC Arming ve minimum çap açısı
  escMotor.writeMicroseconds(THROTTLE_MIN);
  diameterServo.write(currentAngle);
  delay(3000); // AM32 açılış melodisi ve arming için bekleme

  Serial.println("--- AM32 DAHILI GOVERNOR MODU AKTIF ---");
  Serial.println("Komutlar: T1400 (Hedef Gaz), C90 (Cap Acisi), STOP");
}

void loop() {
  // 1. Bilgisayardan Komut Dinleme
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.equalsIgnoreCase("STOP")) {
      currentThrottle = THROTTLE_MIN;
      escMotor.writeMicroseconds(THROTTLE_MIN);
    } 
    // T: Doğrudan Hedef Gaz / Governor Setpoint (1000 - 2000 us)
    else if (input.charAt(0) == 'T' || input.charAt(0) == 't') {
      int requestedThrottle = input.substring(1).toInt();
      currentThrottle = constrain(requestedThrottle, THROTTLE_MIN, THROTTLE_MAX);
      escMotor.writeMicroseconds(currentThrottle);
    } 
    // C: Kanat Çap Açısı (0 - 180 derece)
    else if (input.charAt(0) == 'C' || input.charAt(0) == 'c') {
      currentAngle = constrain(input.substring(1).toInt(), SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);
      diameterServo.write(currentAngle);
    }
  }

  // 2. Canlı Devri Oku (Kapalı çevrim kararı vermez; sadece ekrana yansıtır)
  readESCTelemetry();

  // 3. Serial Plotter Çıktısı (Yük altındaki devir kararlılığını izlemek için)
  if (millis() - lastPlotTime >= PLOT_INTERVAL_MS) {
    lastPlotTime = millis();
    Serial.print("Hedef_Gaz_us:");
    Serial.print(currentThrottle);
    Serial.print(" ");
    Serial.print("Gercek_RPM:");
    Serial.println(actualRPM, 0);
  }
}
