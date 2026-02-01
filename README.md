# Smart Parking System

Sistem parkir pintar berbasis IoT dengan monitoring kapasitas real-time menggunakan teknologi RFID dan komunikasi serial antar mikrokontroler.

## Deskripsi Project

Smart Parking System adalah solusi parkir otomatis yang dirancang untuk mengelola kapasitas parkir secara efisien. Sistem ini menggunakan dua mikrokontroler yang berkomunikasi melalui serial communication untuk mengontrol pintu masuk dan keluar, dilengkapi dengan panel surya sebagai sumber energi alternatif.

## Fitur Utama

- **Monitoring Kapasitas Real-time** - Sistem counter otomatis untuk tracking jumlah kendaraan
- **Akses Control RFID** - Keamanan ganda dengan RFID di pintu masuk dan keluar
- **Komunikasi Serial** - Sinkronisasi data antara Arduino Mega (Master) dan Arduino Uno (Slave)
- **Indikator Visual & Audio** - LED merah/hijau dan buzzer untuk notifikasi status
- **Solar Powered** - Menggunakan panel surya untuk efisiensi energi
- **Dual Microcontroller** - Sistem terdistribusi untuk performa optimal

## Komponen Hardware

### Mikrokontroler
- **Arduino Mega 2560** - Master controller (Pintu Masuk)
- **Arduino Uno** - Slave controller (Pintu Keluar)

### Sensor & Aktuator
- **2x RFID Reader** (RC522/PN532)
  - 1x di pintu masuk
  - 1x di pintu keluar
- **1x LED Hijau** - Indikator akses diizinkan
- **1x LED Merah** - Indikator parkir penuh/akses ditolak
- **1x Buzzer** - Notifikasi audio
- **2x Ultrasonik** - Mendeteksi kendaraan lewat
- **2x Motor Power Window** - Motor Penggerak Palang Parkir
- **2x Driver Motor BTS** - Antarmuka antara mikrokontroller dan Motor DC
- **2x Limit Switch** - Pembatas Buka Tutup Palang Parkir
  
### Power Supply
- **Solar Panel** - Sumber energi utama
- **Battery** - Backup power
- **Charge Controller** - Regulator pengisian baterai

### Komponen Pendukung
- Kabel jumper
- Resistor
- PCB
- Housing

## Koneksi Pin

### Arduino Mega (master)
```bash
Motor Driver BTS #1
  - RPWM               → 4
  - LPWM               → 5
  - R_EN               → 6
  - L_EN               → 7
RFID #1
  - SDA (SS)           → 44
  - RST                → 49
  - SCK                → 52
  - MOSI               → 51
  - MISO               → 50
  - VCC                → 3.3V
  - GND                → GND
Ultrasonic Sensor #1
  - TRIG               → 31
  - ECHO               → 33
LED Merah              → 35
LED Hijau              → 37
Buzzer                 → 38
Komunikasi Serial
  - TX                → 0
  - RX                → 1
```
### Arduino Uno (Slave)
```bash
Motor Driver BTS #2
  - RPWM               → 5
  - LPWM               → 6
  - R_EN               → 7
  - L_EN               → 8
RFID #2
  - SDA (SS)           → 10
  - RST                → 9
  - SCK                → 13
  - MOSI               → 11
  - MISO               → 12
  - VCC                → 3.3V
  - GND                → GND
Ultrasonic Sensor #2
  - TRIG               → 4
  - ECHO               → 3
Komunikasi Serial
  - TX                 → 0
  - RX                 → 1
  ```
## Cara Kerja Sistem

### Logika Counter
1. **Inisialisasi**: Kapasitas maksimal parkir diset (misal: 10 slot)
2. **Kendaraan Masuk**: 
   - RFID terdeteksi di pintu masuk
   - Counter bertambah (+1)
   - LED hijau menyala, buzzer beep 1x
   - Data dikirim ke slave via serial
3. **Kendaraan Keluar**:
   - RFID terdeteksi di pintu keluar
   - Slave kirim sinyal ke master
   - Counter berkurang (-1)
   - LED hijau menyala, buzzer beep 2x
4. **Parkir Penuh**:
   - Counter = kapasitas maksimal
   - LED merah menyala
   - Buzzer beep panjang saat ada kartu terdeteksi
   - Akses ditolak

### Komunikasi Serial
- **Protocol**: UART (9600 baud)
- **Format Data**: 
  - `ENTRY` - Kendaraan masuk
  - `EXIT` - Kendaraan keluar
  - `STATUS:X` - Update counter (X = jumlah kendaraan)

## Struktur Folder

```
Smart_Parking_System/
├── Hardware/
│   ├── desain_3d.jpeg
│   ├── rangkaian.jpeg
│   └── rangkaian_real.jpeg
├── Software/
│   ├── code_master.ino     
│   ├── code_slave.ino       
│   └── flowchart.jpeg
└── README.md
```

## Instalasi & Setup

### Requirements
- Arduino IDE
- Library yang dibutuhkan:
  - `MFRC522` (untuk RFID)
  - `SPI` (sudah built-in)

### Langkah Instalasi

1. **Clone Repository**
   ```bash
   git clone https://github.com/username/Smart_Parking_System.git
   cd Smart_Parking_System
   ```

2. **Install Library**
   - Buka Arduino IDE
   - Pilih `Sketch` → `Include Library` → `Manage Libraries`
   - Search dan install library `MFRC522`

3. **Upload Code Master**
   - Buka file `Software/code_master.ino`
   - Pilih board: `Arduino Mega 2560`
   - Pilih port yang sesuai
   - Click Upload

4. **Upload Code Slave**
   - Buka file `Software/code_slave.ino`
   - Pilih board: `Arduino Uno`
   - Pilih port yang sesuai
   - Click Upload

5. **Koneksi Hardware**
   - Hubungkan TX Mega ke RX Uno
   - Hubungkan RX Mega ke TX Uno
   - Hubungkan GND kedua board
   - Pasang semua komponen sesuai diagram pin

6. **Power Up**
   - Hubungkan solar panel ke sistem
   - Pastikan battery terisi
   - System ready!

## Testing

### Test Case 1: Kendaraan Masuk
- Tempelkan kartu RFID di reader pintu masuk
- Cek: LED hijau menyala, buzzer beep 1x
- Verifikasi: Counter bertambah di serial monitor

### Test Case 2: Kendaraan Keluar
- Tempelkan kartu RFID di reader pintu keluar
- Cek: LED hijau menyala, buzzer beep 2x
- Verifikasi: Counter berkurang di serial monitor

### Test Case 3: Parkir Penuh
- Isi parkir hingga maksimal
- Tempelkan kartu baru di pintu masuk
- Cek: LED merah menyala, buzzer beep panjang
- Verifikasi: Akses ditolak

## Troubleshooting

| Masalah | Solusi |
|---------|--------|
| RFID tidak terdeteksi | Cek koneksi SPI, pastikan library terinstall |
| LED tidak menyala | Cek polaritas LED dan resistor |
| Komunikasi serial error | Pastikan TX-RX cross-connected dan baud rate sama |
| Counter tidak sinkron | Reset kedua board dan cek koneksi serial |
| Solar panel tidak charging | Cek charge controller dan koneksi battery |
