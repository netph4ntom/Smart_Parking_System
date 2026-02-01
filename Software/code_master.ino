//==================================
// ARDUINO MEGA - MASTER CONTROLLER
// PINTU MASUK + MANAJEMEN KAPASITAS
//==================================
#include <SPI.h>
#include <MFRC522.h>
// ===== KONFIGURASI SISTEM =====
#define KAPASITAS_MAKSIMAL 5 // Ubah sesuai kebutuhan
// ===== RFID =====
#define SS_PIN 44
#define RST_PIN 49
MFRC522 mfrc522(SS_PIN, RST_PIN);
// ===== BTS7960 (Motor Driver Pintu Masuk) =====
#define RPWM 4
#define LPWM 5
#define R_EN 6
#define L_EN 7
// ===== LIMIT SWITCH =====
#define LIMIT_ATAS 27
#define LIMIT_BAWAH 29
// ===== LED DAN BUZZER =====
#define LED_HIJAU 37 // Parkir tersedia
#define LED_MERAH 35 // Parkir penuh / akses ditolak
#define BUZZER 38
// ===== ULTRASONIC =====
#define TRIG_PIN 31
#define ECHO_PIN 33
#define JARAK_AMAN 50 // cm
// ===== MOTOR SPEED =====
#define SPEED_ATAS 60
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
int counterParkir = 0;
bool sedangProses = false;
unsigned long lastCardTime = 0;
const unsigned long CARD_DEBOUNCE = 2000; // 2 detik debounce kartu
// ===== BUFFER SERIAL =====
String serialBuffer = "";
void setup() {
 // Serial (TX0/RX0 - pin 0 dan 1) untuk komunikasi dengan Arduino Uno
 Serial.begin(9600);

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
 // Init LED & Buzzer
 pinMode(LED_HIJAU, OUTPUT);
 pinMode(LED_MERAH, OUTPUT);
 pinMode(BUZZER, OUTPUT);
 // Kondisi Awal
 motorStop();
 // updateStatusLED();
 // Delay untuk stabilisasi serial
 delay(2000);
 Serial.println("========================================");
 Serial.println(" SISTEM PARKIR OTOMATIS - MASTER");
 Serial.println(" PINTU MASUK + MANAJEMEN KAPASITAS");
 Serial.println("========================================");
 Serial.print("Kapasitas Maksimal: ");
 Serial.println(KAPASITAS_MAKSIMAL);
 tampilkanStatus();
 Serial.println("\nSistem siap. Menunggu kartu RFID...\n");
}
void loop() {
 // Handler komunikasi Serial dari Uno
 handleSerialFromUno();

 // Handler RFID pintu masuk
 handleRFIDMasuk();
}
// ================= HANDLER RFID PINTU MASUK =================
void handleRFIDMasuk() {
 // Skip jika sedang proses
 if (sedangProses) return;

 // Cek kartu RFID
 if (!mfrc522.PICC_IsNewCardPresent()) return;
 if (!mfrc522.PICC_ReadCardSerial()) return;
 // Debounce kartu (hindari baca ganda)
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
 // Cek kapasitas parkir
 if (counterParkir >= KAPASITAS_MAKSIMAL) {
 Serial.println(">>> AKSES DITOLAK - PARKIR PENUH <<<");
 aksesDitolak(true); // true = parkir penuh
 } else {
 Serial.println(">>> AKSES DITERIMA <<<");
 aksesIzinkan();

 sedangProses = true;
 bukaPalangMasuk();
 tungguKendaraanLewat();

 // Kendaraan sudah masuk, increment counter
 counterParkir++;
 Serial.print("Counter: ");
 Serial.print(counterParkir);
 Serial.print("/");
 Serial.println(KAPASITAS_MAKSIMAL);

 tutupPalangMasuk();
 sedangProses = false;

 // updateStatusLED();
 tampilkanStatus();
 Serial.println("Sistem siap. Silakan tempelkan kartu...\n");
 }
 } else {
 Serial.println(">>> AKSES DITOLAK - KARTU TIDAK VALID <<<");
 aksesDitolak(false); // false = kartu tidak valid
 }
 mfrc522.PICC_HaltA();
}
// ================= HANDLER SERIAL DARI UNO =================
void handleSerialFromUno() {
 while (Serial.available()) {
 char c = Serial.read();

 if (c == '\n') {
 serialBuffer.trim();

 if (serialBuffer == "REQ_IN") {
 // Uno request izin masuk (seharusnya tidak terjadi, Uno adalah pintu keluar)
 Serial.println("[UNO] Request masuk (tidak valid untuk Uno)");
 Serial.println("DENY");

 } else if (serialBuffer == "CONFIRM_OUT") {
 // Kendaraan keluar, decrement counter
 if (counterParkir > 0) {
 counterParkir--;
 Serial.println("[UNO] Kendaraan keluar");
 Serial.print("Counter: ");
 Serial.print(counterParkir);
 Serial.print("/");
 Serial.println(KAPASITAS_MAKSIMAL);

 // updateStatusLED();
 tampilkanStatus();
 }
 Serial.println("ACK");

 } else if (serialBuffer.length() > 0) {
 Serial.print("[UNO] Pesan tidak dikenal: ");
 Serial.println(serialBuffer);
 }

 serialBuffer = "";
 } else {
 serialBuffer += c;
 }
 }
}
// ================= LOGIKA LED & BUZZER =================
// void updateStatusLED() {
// if (counterParkir >= KAPASITAS_MAKSIMAL) {
// digitalWrite(LED_HIJAU, LOW);
// digitalWrite(LED_MERAH, HIGH);
// } else {
// digitalWrite(LED_HIJAU, HIGH);
// digitalWrite(LED_MERAH, LOW);
// }
// }
void aksesIzinkan() {
 digitalWrite(LED_MERAH, LOW);
 digitalWrite(LED_HIJAU, HIGH);

 // Buzzer beep 1x
 digitalWrite(BUZZER, HIGH);
 delay(200);
 digitalWrite(LED_MERAH, LOW);
 digitalWrite(LED_HIJAU, LOW);
 digitalWrite(BUZZER, LOW);
}
void aksesDitolak(bool parkirPenuh) {
 digitalWrite(LED_HIJAU, LOW);
 digitalWrite(LED_MERAH, HIGH);

 if (parkirPenuh) {
 // Buzzer panjang untuk parkir penuh
 digitalWrite(BUZZER, HIGH);
 delay(1000);
 digitalWrite(BUZZER, LOW);
 } else {
 // Buzzer 2x untuk kartu tidak valid
 for (int i = 0; i < 2; i++) {
 digitalWrite(BUZZER, HIGH);
 delay(150);
 digitalWrite(BUZZER, LOW);
 delay(150);
 }
 }
 digitalWrite(LED_HIJAU, LOW);
 digitalWrite(LED_MERAH, LOW);
}
// ================= FUNGSI MOTOR PINTU MASUK =================
void bukaPalangMasuk() {
 Serial.println("Membuka palang masuk...");
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
 Serial.println("Palang masuk terbuka.");
}
void tutupPalangMasuk() {
 Serial.println("Menutup palang masuk...");
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
 Serial.println("Palang masuk tertutup.");
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
 Serial.println("Menunggu kendaraan masuk...");

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
 Serial.println("Kendaraan berhasil masuk!");
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
// ================= DISPLAY STATUS =================
void tampilkanStatus() {
 Serial.println("----------------------------------------");
 Serial.print("Status Parkir: ");
 Serial.print(counterParkir);
 Serial.print("/");
 Serial.print(KAPASITAS_MAKSIMAL);
 Serial.print(" (");
 Serial.print(KAPASITAS_MAKSIMAL - counterParkir);
 Serial.println(" slot tersisa)");

 if (counterParkir >= KAPASITAS_MAKSIMAL) {
 Serial.println("Status: PENUH");
 } else {
 Serial.println("Status: TERSEDIA");
 }
 Serial.println("----------------------------------------");
}
