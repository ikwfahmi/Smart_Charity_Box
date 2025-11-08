#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// ------------------- WIFI + TELEGRAM --------------------
const char* ssid = "NAME WIFI HERE";
const char* password = "PASSWORD WIFI HERE";

#define BOTtoken "BOT TOKEN HERE"
#define CHAT_ID "CHAT ID HERE"

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOTtoken, secured_client);

// ------------------- SENSOR WARNA + LCD --------------------
#define S2 2
#define S3 4
#define sensorOut 5
#define IR_SENSOR_PIN 15
#define BUZZER_PIN 19

LiquidCrystal_I2C lcd(0x27, 16, 2);

int redFrequency = 0;
int greenFrequency = 0;
int blueFrequency = 0;

String lastColor = "";

// ============================================================
//                      SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();

  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(sensorOut, INPUT);
  pinMode(IR_SENSOR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  lcd.setCursor(0, 0);
  lcd.print("Sensor Warna Aktif");
  lcd.setCursor(0, 1);
  lcd.print("Menunggu objek...");

  // -------- KONEKSI WIFI ---------
  WiFi.begin(ssid, password);
  Serial.print("Menghubungkan WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Terhubung!");

  // -------- TELEGRAM HTTPS --------
  secured_client.setInsecure();
  Serial.println("Telegram Bot Siap!");
}

// ============================================================
//                 LOOP UTAMA
// ============================================================
void loop() {
  int irStatus = digitalRead(IR_SENSOR_PIN);

  if (irStatus == LOW) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Objek Terdeteksi");

    // Baca RED
    digitalWrite(S2, LOW);
    digitalWrite(S3, LOW);
    redFrequency = pulseIn(sensorOut, LOW);

    // Baca GREEN
    digitalWrite(S2, HIGH);
    digitalWrite(S3, HIGH);
    greenFrequency = pulseIn(sensorOut, LOW);

    // Baca BLUE
    digitalWrite(S2, LOW);
    digitalWrite(S3, HIGH);
    blueFrequency = pulseIn(sensorOut, LOW);

    // Deteksi warna
    String currentColor =
      detectColor(redFrequency, greenFrequency, blueFrequency);

    // Jika warna berubah → tampilkan + kirim Telegram
    if (currentColor != lastColor) {
      tampilkanWarna(currentColor);
      kirimKeTelegram(currentColor);
      lastColor = currentColor;
    }

  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Menunggu objek...");
    digitalWrite(BUZZER_PIN, LOW);
  }

  delay(300);
}

// ============================================================
//             FUNCTION DETEKSI WARNA
// ============================================================
String detectColor(int r, int g, int b) {
  int minVal = min(r, min(g, b));
  int maxVal = max(r, max(g, b));
  int diff = maxVal - minVal;

  if (diff < 20) return "Putih";
  if (r == minVal) return "Merah";
  if (g == minVal) return "Hijau";
  if (b == minVal) return "Biru";
  return "Tidak dikenal";
}

// ============================================================
//            TAMPILKAN DI LCD + BUZZER
// ============================================================
void tampilkanWarna(String warna) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Warna: ");
  lcd.print(warna);

  Serial.print("Warna: ");
  Serial.println(warna);

  if (warna == "Merah") {
    for (int i = 0; i < 3; i++) { tone(BUZZER_PIN, 800, 150); delay(200); }
    noTone(BUZZER_PIN);
  }
  else if (warna == "Hijau") {
    tone(BUZZER_PIN, 1200, 400); delay(500);
    tone(BUZZER_PIN, 900, 300); delay(400);
    noTone(BUZZER_PIN);
  }
  else if (warna == "Biru") {
    for (int f = 1000; f <= 1800; f += 200) { tone(BUZZER_PIN, f, 100); delay(150); }
    for (int f = 1800; f >= 1000; f -= 200) { tone(BUZZER_PIN, f, 100); delay(150); }
    noTone(BUZZER_PIN);
  }
  else {
    noTone(BUZZER_PIN);
  }
}

// ============================================================
//                    KIRIM KE TELEGRAM
// ============================================================
void kirimKeTelegram(String warna) {
  String pesan = 
    "🎨 *Warna Terdeteksi!* \n"
    "=====================\n\n"
    "🔹 Warna: *" + warna + "*\n"
    "🔸 Red: " + String(redFrequency) + "\n"
    "🔸 Green: " + String(greenFrequency) + "\n"
    "🔸 Blue: " + String(blueFrequency) + "\n\n"
    "✅ Laporan dikirim dari ESP32";

  bot.sendMessage(CHAT_ID, pesan, "Markdown");
  Serial.println("Pesan terkirim ke Telegram!");
}
