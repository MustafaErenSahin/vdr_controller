# ESP32 AM32 ESC ve Değişken Çaplı Rotor (VDR) Kontrolcüsü

AM32 ESC telemetrisi ve servo kontrolü kullanarak fırçasız doğru akım (BLDC) motoru ve değişken çaplı rotor (VDR) mekanizmasını yöneten ESP32 tabanlı gömülü yazılım.

---

## Donanım Bağlantıları (Pinout)

| Çevre Birimi | ESP32 Pini | Sinyal / Protokol | Açıklama |
| :--- | :--- | :--- | :--- |
| **ESC Gaz Sinyali** | GPIO 18 | 50 Hz PWM | 1000 µs – 2000 µs (Governor hedef sinyali) |
| **Çap Servosu** | GPIO 19 | 50 Hz PWM | 500 µs – 2500 µs (0° – 180° kanat açısı) |
| **ESC Telemetri** | GPIO 16 (RX2) | Donanımsal UART | 115200 Baud, 8N1 (10-Byte KISS Protokolü) |

---

## Gereksinimler ve Kurulum (Arduino IDE)

Kodu karta derleyip yüklemeden önce Arduino IDE üzerinde aşağıdaki iki bileşenin kurulu olması gerekir:

### 1. ESP32 Kart Desteği (Boards Manager)
1. Arduino IDE menüsünden **Tools (Araçlar)** → **Board (Kart)** → **Boards Manager... (Kart Yöneticisi)** seçeneğine gidin.
2. Arama kutusuna `esp32` yazın.
3. **esp32 by Espressif Systems** paketini bulun ve **Install (Kur)** butonuna basın.
4. Kurulumdan sonra **Tools** → **Board** → **esp32** menüsünden kullandığınız kartı (örneğin `ESP32 Dev Module`) seçin.

### 2. Servo Kütüphanesi (Library Manager)
1. Menüden **Sketch (Taslak)** → **Include Library (Kütüphane Ekle)** → **Manage Libraries... (Kütüphaneleri Yönet)** yolunu izleyin.
2. Arama kutusuna `ESP32Servo` yazın.
3. **ESP32Servo** (Kevin Harrington) kütüphanesini seçip **Install (Kur)** butonuna tıklayın.

---

## Seri Port Komutları (115200 Baud)

Arduino IDE üzerinden **Serial Monitor** veya **Serial Plotter** ekranını açın. İletişim hızını **115200 Baud** ve satır sonunu **Newline** olarak ayarlayın.

- `T<değer>` : Gaz / Governor hedef sinyalini ayarlar (Örn: `T1400` → 1000 ile 2000 µs arası).
- `C<açı>`   : Rotor çapını değiştiren servonun açısını ayarlar (Örn: `C90` → 0 ile 180 derece arası).
- `STOP`     : Acil durdurma emri. Gazı anında güvenli seviyeye (1000 µs) çeker ve motoru durdurur.