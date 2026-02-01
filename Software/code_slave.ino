//==================================
// ARDUINO UNO - SLAVE CONTROLLER
// PINTU KELUAR
//==================================
#include <SPI.h>
#include <MFRC522.h>
// ===== RFID =====
#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);
// ===== BTS7960 (Motor Driver Pintu Keluar) =====
#define RPWM 5
#define LPWM 6
#define R_EN 7
#define L_EN 8
// ===== LIMIT SWITCH =====
#define LIMIT_ATAS A0
#define LIMIT_BAWAH A1
// // ===== LED DAN BUZZER =====
// #define LED_HIJAU 2
// #define LED_MERAH 3
// #define BUZZER 4
// ===== ULTRASONIC =====
#define TRIG_PIN 4
#define ECHO_PIN 3
#define JARAK_AMAN 50 // cm
// ===== MOTOR SPEED =====
#define SPEED_ATAS 72
#define SPEED_BAWAH 20
// ===== UID VALID =====
#define JUMLAH_UID 6
byte authorizedUID[][4] = {
 {0x19, 0x5A, 0xB5, 0x02},
 {0xD2, 0xA2, 0x0F, 0x05},
 {0xEA, 0x24, 0x2D, 0x02},
 {0x65, 0xFE, 0x77, 0x85},
 {0x55, 0x6E, 0x26, 0x85},
 {0x45, 0x10, 0xD5, 0x85}
};
// ===== VARIABEL GLOBAL =====
bool sedangProses = false;
unsigned long lastCardTime = 0;
const unsigned long CARD_DEBOUNCE = 2000; // 2 detik debounce kartu
// ===== BUFFER SERIAL =====
String serialBuffer = "";
void setup() {
 Serial.begin(9600); // Komunikasi dengan Arduino Mega

 // Init RFID
 SPI.begin();
 mfrc522.PCD_Init();
 // Init Motor
 pinMode(RPWM, OUTPUT);
 pinMode(LPWM, OUTPUT);
 pinMode(R_EN, OUTPUT);
 pinMode(L_EN, OUTPUT);
 // Init Limit Switch
 pinMode(LIMIT_ATAS, INPUT_PULLUP);
 pinMode(LIMIT_BAWAH, INPUT_PULLUP);
 // Init Ultrasonic
 pinMode(TRIG_PIN, OUTPUT);
 pinMode(ECHO_PIN, INPUT);
 // // Init LED & Buzzer
 // pinMode(LED_HIJAU, OUTPUT);
 // pinMode(LED_MERAH, OUTPUT);
 // pinMode(BUZZER, OUTPUT);
 // Kondisi Awal
 motorStop();
 // digitalWrite(LED_HIJAU, HIGH); // Pintu keluar selalu siap
 // digitalWrite(LED_MERAH, LOW);
 // Delay untuk sinkronisasi dengan Mega
 delay(1000);
 Serial.println("========================================");
 Serial.println(" SISTEM PARKIR OTOMATIS - SLAVE");
 Serial.println(" PINTU KELUAR");
 Serial.println("========================================");
 Serial.println("\nSistem siap. Menunggu kartu RFID...\n");
}
void loop() {
 // Handler komunikasi Serial dari Mega
 handleSerialFromMega();

 // Handler RFID pintu keluar
 handleRFIDKeluar();
}
// ================= HANDLER RFID PINTU KELUAR =================
void handleRFIDKeluar() {
 // Skip jika sedang proses
 if (sedangProses) return;

 // Cek kartu RFID
 if (!mfrc522.PICC_IsNewCardPresent()) return;
 if (!mfrc522.PICC_ReadCardSerial()) return;
 // Debounce kartu
 unsigned long currentTime = millis();
 if (currentTime - lastCardTime < CARD_DEBOUNCE) {
 mfrc522.PICC_HaltA();
 return;
 }
 lastCardTime = currentTime;
 // Tampilkan UID
 Serial.print("UID Terdeteksi: ");
 for (byte i = 0; i < mfrc522.uid.size; i++) {
 Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
 Serial.print(mfrc522.uid.uidByte[i], HEX);
 }
 Serial.println();
 // Validasi kartu
 if (isAuthorized(mfrc522.uid.uidByte, mfrc522.uid.size)) {
 Serial.println(">>> AKSES KELUAR DITERIMA <<<");
 // aksesIzinkan();

 sedangProses = true;
 bukaPalangKeluar();
 tungguKendaraanLewat();

 // Notifikasi ke Mega bahwa kendaraan sudah keluar
 Serial.println("CONFIRM_OUT");
 waitForAck();

 tutupPalangKeluar();
 sedangProses = false;

 Serial.println("Sistem siap. Silakan tempelkan kartu...\n");

 } else {
 Serial.println(">>> AKSES DITOLAK - KARTU TIDAK VALID <<<");
 // aksesDitolak();
 }
 mfrc522.PICC_HaltA();
}
// ================= HANDLER SERIAL DARI MEGA =================
void handleSerialFromMega() {
 while (Serial.available()) {
 char c = Serial.read();

 if (c == '\n') {
 serialBuffer.trim();

 if (serialBuffer == "ACK") {
 // Acknowledgement dari Mega
 Serial.println("[MEGA] ACK diterima");

 } else if (serialBuffer == "ALLOW") {
 // Untuk future implementation jika Uno juga handle masuk
 Serial.println("[MEGA] Akses diizinkan");

 } else if (serialBuffer == "DENY") {
 // Untuk future implementation
 Serial.println("[MEGA] Akses ditolak");

 } else {
 Serial.print("[MEGA] Pesan tidak dikenal: ");
 Serial.println(serialBuffer);
 }

 serialBuffer = "";
 } else {
 serialBuffer += c;
 }
 }
}
void waitForAck() {
 unsigned long startTime = millis();
 String tempBuffer = "";

 while (millis() - startTime < 2000) { // Timeout 2 detik
 while (Serial.available()) {
 char c = Serial.read();
 if (c == '\n') {
 tempBuffer.trim();
 if (tempBuffer == "ACK") {
 Serial.println("[MEGA] Konfirmasi diterima");
 return;
 }
 tempBuffer = "";
 } else {
 tempBuffer += c;
 }
 }
 delay(10);
 }

 Serial.println("[MEGA] Timeout menunggu ACK");
}
// ================= LOGIKA LED & BUZZER =================
// void aksesIzinkan() {
// digitalWrite(LED_MERAH, LOW);
// digitalWrite(LED_HIJAU, HIGH);

// // Buzzer beep 1x
// digitalWrite(BUZZER, HIGH);
// delay(200);
// digitalWrite(BUZZER, LOW);
// }
// void aksesDitolak() {
// digitalWrite(LED_HIJAU, LOW);

// // LED Merah berkedip dan Buzzer 3x
// for (int i = 0; i < 3; i++) {
// digitalWrite(BUZZER, HIGH);
// digitalWrite(LED_MERAH, HIGH);
// delay(150);
// digitalWrite(BUZZER, LOW);
// digitalWrite(LED_MERAH, LOW);
// delay(150);
// }

// // Kembali ke mode siap
// digitalWrite(LED_HIJAU, HIGH);
// digitalWrite(LED_MERAH, LOW);
// }
// ================= FUNGSI MOTOR PINTU KELUAR =================
void bukaPalangKeluar() {
 Serial.println("Membuka palang keluar...");
 digitalWrite(R_EN, HIGH);
 digitalWrite(L_EN, HIGH);
 // Cek apakah sudah terbuka
 if (digitalRead(LIMIT_ATAS) == HIGH) {
 Serial.println("Palang sudah terbuka!");
 motorStop();
 return;
 }
 // Buka palang
 unsigned long startTime = millis();
 while (digitalRead(LIMIT_ATAS) == LOW) {
 analogWrite(RPWM, SPEED_ATAS);
 analogWrite(LPWM, 0);

 if (millis() - startTime > 15000) {
 Serial.println("TIMEOUT membuka palang!");
 break;
 }
 delay(10);
 }

 motorStop();
 Serial.println("Palang keluar terbuka.");
}
void tutupPalangKeluar() {
 Serial.println("Menutup palang keluar...");
 digitalWrite(R_EN, HIGH);
 digitalWrite(L_EN, HIGH);
 // Cek apakah sudah tertutup
 if (digitalRead(LIMIT_BAWAH) == HIGH) {
 Serial.println("Palang sudah tertutup!");
 motorStop();
 return;
 }
 // Tutup palang
 unsigned long startTime = millis();
 while (digitalRead(LIMIT_BAWAH) == LOW) {
 analogWrite(RPWM, 0);
 analogWrite(LPWM, SPEED_BAWAH);

 if (millis() - startTime > 15000) {
 Serial.println("TIMEOUT menutup palang!");
 break;
 }
 delay(10);
 }

 motorStop();
 Serial.println("Palang keluar tertutup.");
}
void motorStop() {
 analogWrite(RPWM, 0);
 analogWrite(LPWM, 0);
 digitalWrite(R_EN, LOW);
 digitalWrite(L_EN, LOW);
}
// ================= SENSOR ULTRASONIC =================
long bacaJarak() {
 digitalWrite(TRIG_PIN, LOW);
 delayMicroseconds(2);
 digitalWrite(TRIG_PIN, HIGH);
 delayMicroseconds(10);
 digitalWrite(TRIG_PIN, LOW);

 long duration = pulseIn(ECHO_PIN, HIGH, 30000);
 if (duration == 0) return 999;

 long jarak = duration * 0.034 / 2;
 return (jarak > 400) ? 999 : jarak;
}
void tungguKendaraanLewat() {
 Serial.println("Menunggu kendaraan keluar...");

 bool kendaraanTerdeteksi = false;
 unsigned long waktuMulai = millis();
 unsigned long waktuStabil = 0;

 while (true) {
 long jarak = bacaJarak();

 // Kendaraan masuk area deteksi
 if (jarak < JARAK_AMAN) {
 kendaraanTerdeteksi = true;
 waktuStabil = millis();
 }

 // Kendaraan sudah lewat dan stabil 2 detik
 if (kendaraanTerdeteksi && jarak >= JARAK_AMAN) {
 if (millis() - waktuStabil > 2000) {
 Serial.println("Kendaraan berhasil keluar!");
 break;
 }
 }

 // Timeout 30 detik
 if (millis() - waktuMulai > 30000) {
 Serial.println("TIMEOUT menunggu kendaraan!");
 break;
 }

 delay(200);
 }
}
// ================= RFID HELPER =================
bool isAuthorized(byte *uid, byte uidSize) {
 for (byte i = 0; i < JUMLAH_UID; i++) {
 bool match = true;
 for (byte j = 0; j < uidSize; j++) {
 if (uid[j] != authorizedUID[i][j]) {
 match = false;
 break;
 }
 }
 if (match) return true;
 }
 return false;
}
