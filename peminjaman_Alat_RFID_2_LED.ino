#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ======================
// Konfigurasi WiFi & Server
// ======================
const char* ssid     = "PUMA KEMBAR";
const char* password = "kembar69";
// Ganti dengan IP laptop/server lokal kamu, atau domain jika sudah online
const char* serverUrl = "http://192.168.18.5/monitoring-alat/simpan_rfid.php"; 

// ======================
// RFID RC522 (SPI)
// ======================
#define SS_PIN   5
#define RST_PIN  4
MFRC522 rfid(SS_PIN, RST_PIN);

// ======================
// LED Indikator
// ======================
#define LED_MERAH 26
#define LED_HIJAU 27

void setup() {
  Serial.begin(115200);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(LED_HIJAU, OUTPUT);

  digitalWrite(LED_MERAH, LOW);
  digitalWrite(LED_HIJAU, LOW);

  // Inisialisasi SPI & RFID
  SPI.begin(18, 19, 23, 5);
  rfid.PCD_Init();

  // Koneksi ke WiFi
  WiFi.begin(ssid, password);
  Serial.print("Menghubungkan ke WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Terhubung!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Sistem Siap. Tempelkan kartu untuk peminjaman...");
}

void loop() {
  // cek kartu
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    delay(50);
    return;
  }

  // Ambil UID kartu dalam format String Hex
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) {
      uid += "0";
    }
    uid += String(rfid.uid.uidByte[i], HEX);
    if (i != rfid.uid.size - 1) {
      uid += " ";
    }
  }
  uid.toUpperCase();

  Serial.print("UID Kartu Terdeteksi: ");
  Serial.println(uid);

  // Kirim data ke MySQL via HTTP POST
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client; // Tambahkan ini untuk mengelola koneksi TCP
    HTTPClient http;
    
    //http.begin(serverUrl);
    // Gunakan objek client di dalam http.begin
    if (http.begin(client, serverUrl)) {
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      // Mengirimkan parameter 'uid_kartu' ke file PHP
      String httpRequestData = "uid_kartu=" + uid;
      int httpResponseCode = http.POST(httpRequestData);

      if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.print("Respon Server [");
        Serial.print(httpResponseCode);
        Serial.print("]: ");
        Serial.println(response);

        // ======================
        // LED Berdasarkan Status
        // ======================

        // Jika peminjaman berhasil
        if (response.indexOf("berhasil dipinjam") >= 0) {

          Serial.println("Peminjaman Berhasil");

          digitalWrite(LED_MERAH, HIGH);
          delay(2000);
          digitalWrite(LED_MERAH, LOW);
        }

        // Jika pengembalian berhasil
        else if (response.indexOf("berhasil dikembalikan") >= 0) {

          Serial.println("Pengembalian Berhasil");

          digitalWrite(LED_HIJAU, HIGH);
          delay(2000);
          digitalWrite(LED_HIJAU, LOW);
        }

        else {
          Serial.println("Status tidak dikenali");
        }
      } else {
        Serial.print("Error saat mengirim POST: ");
        Serial.println(httpResponseCode);
        // Tambahkan ini untuk tahu deskripsi error teks-nya
        Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpResponseCode).c_str());
      }
      
      http.end();
    } else {
      Serial.println("[HTTP] Tidak dapat terhubung ke server (Format URL salah/Server mati)");
    }
  } else {
    Serial.println("Koneksi WiFi Terputus!");
  }

  // stop komunikasi RFID
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  // tunggu kartu dijauhkan
  while (rfid.PICC_IsNewCardPresent()) {
      delay(50);
  }

  delay(2000); 
}