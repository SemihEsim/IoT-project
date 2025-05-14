#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#include <DHT.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// WiFi bilgileri

const char *WIFI_SSID= "GalaxyA24"; // WiFi SSID bilgisi
const char  *WIFI_PASSWORD ="987654321"; // WiFi şifresi

// Firebase bilgileri
#define FIREBASE_HOST "evdekisen2-default-rtdb.firebaseio.com" // Firebase host adresi
#define FIREBASE_AUTH "i3jEp5jPwp9Ji7f5vXx4KSGgBL5EO6PRCj2iNduA" // Firebase kimlik doğrulama anahtarı

// DHT sensörü ayarları
#define DHTPIN D4 // DHT11 sensörünün bağlı olduğu pin
#define DHTTYPE DHT11 // Kullanılan DHT sensör tipi
DHT dht(DHTPIN, DHTTYPE); // DHT sensör nesnesi

// Ses sensörü ayarları
#define MIC_DIGITAL_PIN D5 // LM393 ses sensörünün dijital çıkış pini

// NTP istemcisi için ayarlar
WiFiUDP ntpUDP; // WiFi üzerinden UDP iletişimi için nesne
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600 * 3); // Türkiye UTC+3 için NTP istemcisi

unsigned long previousMillis = 0; // Zamanlayıcı için önceki milisaniye değeri
const long interval = 3600000; // 1 saatlik aralık (3600000 ms)

void setup() {
  Serial.begin(9600); // Seri haberleşmeyi başlat

  // WiFi bağlantısı
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); // WiFi ağ bilgileri ile bağlan
  while (WiFi.status() != WL_CONNECTED) { // WiFi bağlantısı tamamlanana kadar bekle
    delay(1000);
    Serial.println("WiFi'ye bağlanılıyor...");
  }
  Serial.println("WiFi bağlantısı başarılı!");

  // Firebase bağlantısını başlat
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  // NTP istemcisini başlat
  timeClient.begin();

  // DHT sensörünü başlat
  dht.begin();

  // Ses sensörü pinini giriş olarak ayarla
  pinMode(MIC_DIGITAL_PIN, INPUT);

  // Güncel zamanı al
  timeClient.update();
  String currentDateTime = getFormattedDateTime();

  // Başlangıçta sıcaklık ve nem verilerini gönder
  sendTemperatureAndHumidity();
}

void loop() {
  // Güncel zamanı NTP istemciden al
  timeClient.update();
  String currentDateTime = getFormattedDateTime();

  // Ses sensörünü kontrol et
  int micValue = digitalRead(MIC_DIGITAL_PIN); // Ses sensöründen değer oku

  if (micValue == HIGH) { // Eğer ses algılanmışsa
    String micPath = "/sensorData/microphone/"; // Firebase mikrofon veri yolu
    micPath += currentDateTime; // Zaman bilgisi ile birlikte yolu oluştur
    micPath += "/notification"; // Bildirim için tam yolu tamamla

    Firebase.setString(micPath, "Ses Algılandı"); // Firebase'e "Ses Algılandı" gönder

    if (Firebase.failed()) { // Eğer gönderim başarısız olursa
      Serial.println("Ses verisi gönderim hatası: " + Firebase.error());
    } else {
      Serial.println("Ses algılandı ve veriler gönderildi");
    }

    delay(1000); // Gürültü algılandıktan sonra kısa bir süre bekle
  }

  // Sıcaklık ve nem verilerini periyodik olarak gönder
  unsigned long currentMillis = millis(); // Geçen süreyi kontrol et
  if (currentMillis - previousMillis >= interval) { // Eğer belirtilen süre geçmişse
    previousMillis = currentMillis; // Zamanlayıcıyı güncelle
    sendTemperatureAndHumidity(); // Sıcaklık ve nem verilerini gönder
  }
}

// Sıcaklık ve nem verisini Firebase'e gönderme fonksiyonu
void sendTemperatureAndHumidity() {
  float temperature = dht.readTemperature(); // DHT sensöründen sıcaklık verisini al
  float humidity = dht.readHumidity(); // DHT sensöründen nem verisini al

  if (isnan(temperature) || isnan(humidity)) { // Eğer sensörden veri okunamıyorsa
    Serial.println("DHT sensöründen veri okunamadı!");
    return; // Fonksiyonu sonlandır
  }

  String currentDateTime = getFormattedDateTime(); // Güncel zamanı al

  // Veriyi Firebase'e yazmak için yollar oluştur
  String dataPath = "/sensorData/Dht/" + currentDateTime;
  String temperaturePath = dataPath + "/sicaklik";
  String humidityPath = dataPath + "/nem";
  String webPath = "/sensor";
  String webtemperaturePath = webPath + "/temperature";
  String webhumidityPath = webPath + "/humidity";

  Firebase.setFloat(temperaturePath, temperature); // Sıcaklık verisini gönder
  Firebase.setFloat(humidityPath, humidity); // Nem verisini gönder
  Firebase.setFloat(webtemperaturePath, temperature); // Web için sıcaklık verisini gönder
  Firebase.setFloat(webhumidityPath, humidity); // Web için nem verisini gönder

  if (Firebase.failed()) { // Eğer veri gönderimi başarısız olursa
    Serial.println("Sıcaklık ve nem verisi gönderim hatası: " + Firebase.error());
  } else {
    Serial.println("Sıcaklık ve nem verisi gönderildi: " + currentDateTime);
  }
}

// Tarih ve saati "YYYY-MM-DDTHH:MM:SS" formatında döndür
String getFormattedDateTime() {
  time_t rawTime = timeClient.getEpochTime(); // Epoch zamanını al
  struct tm *timeInfo = localtime(&rawTime); // Yerel zamana dönüştür

  char buffer[20]; // Tarih ve saat için tampon
  sprintf(buffer, "%04d-%02d-%02dT%02d:%02d:%02d",
          timeInfo->tm_year + 1900, // Yıl
          timeInfo->tm_mon + 1, // Ay
          timeInfo->tm_mday, // Gün
          timeInfo->tm_hour, // Saat
          timeInfo->tm_min, // Dakika
          timeInfo->tm_sec); // Saniye
  return String(buffer); // Formatlanmış zamanı döndür
}
