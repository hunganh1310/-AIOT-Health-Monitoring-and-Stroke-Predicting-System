#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

MAX30105 particleSensor;

// OLED SSD1315 128x64
//#define SCREEN_WIDTH 128
//#define SCREEN_HEIGHT 64
//Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Pin đọc điện áp pin qua chia áp (VD: GPIO34)
#define BATTERY_PIN 34
// Giá trị điện áp max của pin Li-ion (4.2V)
#define BATTERY_MAX 4.20
// Giá trị điện áp min khi cạn (3.0V)
#define BATTERY_MIN 3.00

#define MAX_BRIGHTNESS 255

const char* localMqttServer = "192.168.1.9"; // IP máy tính của bạn
const int localMqttPort = 1883;               // Port broker máy tính
const char* localMqttClientId = "esp32_ppg";
const char* localMqttUsername = "";           // Nếu broker không yêu cầu user/pass thì để trống
const char* localMqttPassword = "";

// Tạo kết nối riêng cho broker máy tính
WiFiClient localEspClient;
PubSubClient localClient(localEspClient);

const char* ssid = "FPT";
const char* password = "80812004";
const char* mqttServer = "nxchieu.duckdns.org";
const int mqttPort = 11883;
const char* mqttClientId = "jy2fl048z8fdaj4gxdc9";
const char* mqttUsername = "3j84yqow1tugy0kjbiix";
const char* mqttPassword = "xzzad47wbsittxqno50c";

// Thu thập dữ liệu 10s (125 Hz)
const int sampleRate = 125;
const int patchDurationSec = 10;
const int patchSize = sampleRate * patchDurationSec; // 1250 mẫu
float ppgBuffer[patchSize];
int sampleCount = 0;
unsigned long lastSampleTime = 0;

bool patchReady = false;

const int sampleIntervalMs = 8; // 125 Hz = 8ms/mẫu
unsigned long sampleIndex = 0;

WiFiClient espClient;
PubSubClient client(espClient);

// ESP32 có đủ RAM để sử dụng 32-bit data
uint32_t irBuffer[100]; //infrared LED sensor data
uint32_t redBuffer[100];  //red LED sensor data

int32_t bufferLength; //data length
int32_t spo2; //SPO2 value
int8_t validSPO2; //indicator to show if the SPO2 calculation is valid
int32_t heartRate; //heart rate value
int8_t validHeartRate; //indicator to show if the heart rate calculation is valid
String status;

// Thêm biến để tính Average BPM
float avgHeartRate = 0;
int validReadings = 0;
unsigned long lastUpdate = 0;
unsigned long lastMqttSend = 0; // Thêm biến để kiểm soát tần suất gửi MQTT

// ESP32 built-in LED pin
#define LED_BUILTIN 2

void setupWiFi() {
  Serial.print("🔌 Kết nối WiFi...");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi đã kết nối");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal Strength: ");
    Serial.println(WiFi.RSSI());
  } else {
    Serial.println("\n❌ WiFi kết nối thất bại!");
  }
}

/*void reconnectLocalMQTT() {
  while (!localClient.connected()) {
    Serial.print("🔄 Kết nối MQTT Local... ");
    if (localClient.connect(localMqttClientId, localMqttUsername, localMqttPassword)) {
      Serial.println("✅ MQTT Local đã kết nối");
    } else {
      Serial.print("❌ Lỗi MQTT Local: ");
      Serial.print(localClient.state());
      Serial.println(" -> thử lại sau 2s");
      delay(2000);
    }
  }
}*/

void reconnectMQTT() {
  int attempts = 0;
  while (!client.connected() && attempts < 3) {
    Serial.print("🔄 Kết nối MQTT... ");
    
    if (client.connect(mqttClientId, mqttUsername, mqttPassword)) {
      Serial.println("✅ MQTT đã kết nối");
      
      // Test gửi một message để kiểm tra kết nối
      String testPayload = "{\"test\": \"connection_ok\", \"timestamp\": " + String(millis()) + "}";
      boolean result = client.publish("v1/devices/me/telemetry", testPayload.c_str());
      Serial.print("📤 Test message sent: ");
      Serial.println(result ? "SUCCESS" : "FAILED");
      
    } else {
      Serial.print("❌ Lỗi MQTT: ");
      Serial.print(client.state());
      Serial.println(" -> thử lại sau 2s");
      delay(2000);
      attempts++;
    }
  }
  
  if (!client.connected()) {
    Serial.println("❌ MQTT kết nối thất bại sau 3 lần thử!");
  }
}

// Hàm đọc điện áp pin thực tế
float readBatteryVoltage() {
  int adcValue = analogRead(BATTERY_PIN);
  float voltage = (adcValue / 4095.0) * 3.3 * 2; // Giả sử dùng mạch chia áp 1:1
  return voltage;
}

void sendPatchInChunks() {
  const int chunkSize = 250; // 250 mẫu/gói
  int numChunks = patchSize / chunkSize;

  for (int c = 0; c < numChunks; c++) {
    String csvPayload = "";
    for (int i = 0; i < chunkSize; i++) {
      int idx = c * chunkSize + i;
      csvPayload += String(ppgBuffer[idx], 5);
      if (i < chunkSize - 1) csvPayload += ",";
    }

    client.publish("ppg/patch", csvPayload.c_str());
    Serial.printf("Sent chunk %d/%d\n", c+1, numChunks);

    delay(2000); // gửi theo chu kỳ giống MQTT monitor (2 giây/gói)
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("\n=== ESP32 MAX30102 Heart Rate and SpO2 Monitor ===");
  
  setupWiFi();
  
  client.setServer(mqttServer, mqttPort);
  client.setKeepAlive(60);
  client.setSocketTimeout(5); // Timeout 5 giây
  
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Khởi tạo I2C cho ESP32
  Wire.begin(21, 22); // SDA = GPIO21, SCL = GPIO22 (default cho ESP32)
  
  /*if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("❌ SSD1315 không tìm thấy"));
    for(;;); // Dừng lại nếu không tìm thấy
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Starting...");
  display.display();*/

  // Khởi tạo sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //400kHz speed
  {
    Serial.println("❌ MAX30102 was not found. Please check wiring/power.");
  } else {
    Serial.println("✅ MAX30102 Found!");
  }

  Serial.println("Place your finger on the sensor with steady pressure.");
  Serial.println("Starting in 3 seconds...");
  delay(3000);

  // Cấu hình sensor tối ưu cho ESP32
  byte ledBrightness = 50; //Options: 0=Off to 255=50mA
  byte sampleAverage = 4; //Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 2; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  byte sampleRate = 100; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411; //Options: 69, 118, 215, 411
  int adcRange = 4096; //Options: 2048, 4096, 8192, 16384

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
  
  // Tắt các chế độ không cần thiết
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED
  
  Serial.println("=== Monitoring Started ===");
  Serial.println("BPM | Avg BPM | SpO2 | Battery | Status | MQTT");
  Serial.println("------------------------------------------------");
}

void loop(){
  // Kiểm tra kết nối WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi mất kết nối, đang kết nối lại...");
    setupWiFi();
  }
  
  // Kiểm tra và kết nối lại MQTT nếu cần
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  bufferLength = 100; //buffer length 100 = 4 seconds of samples at 25sps

  // Đọc 100 mẫu đầu tiên
  for (byte i = 0 ; i < bufferLength ; i++)
  {
    while (particleSensor.available() == false) //chờ data mới
      particleSensor.check(); //kiểm tra sensor

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample();
  }

  // Tính heart rate và SpO2 sau 100 mẫu đầu
  maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

  // Vòng lặp chính - cập nhật mỗi giây
  while (1)
  {
    // Kiểm tra kết nối trong vòng lặp
    if (!client.connected() && millis() - lastMqttSend > 10000) {
      Serial.println("⚠️  MQTT mất kết nối trong loop");
      break; // Thoát vòng lặp để reconnect
    }
    client.loop();

    // Dịch chuyển data: bỏ 25 mẫu cũ, giữ 75 mẫu cuối
    for (byte i = 25; i < 100; i++)
    {
      redBuffer[i - 25] = redBuffer[i];
      irBuffer[i - 25] = irBuffer[i];
    }

    // Đọc 25 mẫu mới
    for (byte i = 75; i < 100; i++)
    {
      while (particleSensor.available() == false)
        particleSensor.check();

      redBuffer[i] = particleSensor.getRed();
      irBuffer[i] = particleSensor.getIR();
      particleSensor.nextSample();
    }

    // Tính toán lại HR và SpO2
    maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

    // Cập nhật Average BPM (chia 2 để có giá trị chính xác)
    heartRate = heartRate / 2;
    if (validHeartRate == 1 && heartRate > 40 && heartRate < 200) {
      validReadings++;
      avgHeartRate = ((avgHeartRate * (validReadings - 1)) + heartRate) / validReadings;
      
      // Giới hạn số lượng readings để tránh overflow
      if (validReadings > 20) {
        validReadings = 20;
      }
    }
    //Gửi Broker cho học máy
    // Thu thập mẫu
    unsigned long now = millis();
    if (now - lastSampleTime >= sampleIntervalMs) {
      lastSampleTime = now;
      long irValue = particleSensor.getIR();

      if (sampleCount < patchSize) {
        ppgBuffer[sampleCount++] = irValue / 100000.0; // scale
      }

      if (sampleCount >= patchSize) {
        patchReady = true;
        sampleCount = 0;
      }
    }

    // Khi đủ patch
    if (patchReady) {
      patchReady = false;

      // Kiểm tra valid (tất cả mẫu > 50000)
      bool valid = true;
      for (int i = 0; i < patchSize; i++) {
        if (ppgBuffer[i] * 100000.0 <= 50000) { // scale lại để so sánh
          valid = false;
          break;
        }
      }

      if (valid) {
        sendPatchInChunks();
      } else {
        Serial.println("Patch skipped (no finger detected).");
      }
    }
    // Đọc điện áp pin thực tế
    int batteryPercent = 95;

    // In kết quả và gửi MQTT mỗi 3 giây (giảm tần suất để ổn định)
    if (millis() - lastUpdate > 3000) {
      lastUpdate = millis();
      
      // Hiển thị BPM
      Serial.print("BPM: ");
      if (validHeartRate == 1 && heartRate > 40 && heartRate < 200) {
        Serial.print(heartRate);
      } else {
        Serial.print("--");
      }
      Serial.print(" | ");
      
      // Hiển thị Average BPM
      Serial.print("Avg: ");
      if (validReadings > 0) {
        Serial.print(avgHeartRate, 1);
      } else {
        Serial.print("--");
      }
      Serial.print(" | ");
      
      // Hiển thị SpO2
      Serial.print("SpO2: ");
      if (validSPO2 == 1 && spo2 > 80 && spo2 <= 100) {
        Serial.print(spo2);
        Serial.print("%");
      } else {
        Serial.print("--");
      }
      Serial.print(" | ");
      
      // Hiển thị battery
      Serial.print("Bat: ");
      Serial.print(batteryPercent);
      Serial.print("% | ");
      
      // Hiển thị trạng thái
      if (irBuffer[99] < 50000) {
        status = "No finger detected";
        Serial.print(status);
        // Reset average khi không có ngón tay
        avgHeartRate = 0;
        validReadings = 0;
      } else if (validHeartRate == 1 && validSPO2 == 1) {
        status = "Good signal";
        Serial.print(status);
      } else {
        status = "Poor signal - hold steady";
        Serial.print(status);
      }

      // Gửi dữ liệu MQTT
      boolean mqttSuccess = false;
      if (client.connected()) {
        String payload;
        
        if (irBuffer[99] < 50000) {
          payload = "{\"BPM\":0, \"AvgBPM\":0, \"SpO2\":0, \"Battery\":" + String(batteryPercent) + 
                   ", \"Status\":\"No finger detected\", \"timestamp\":" + String(millis()) + "}";
        } else {
          // Đảm bảo dữ liệu hợp lệ trước khi gửi
          int sendBPM = (validHeartRate == 1 && heartRate > 40 && heartRate < 200) ? heartRate : 0;
          int sendSpO2 = (validSPO2 == 1 && spo2 > 80 && spo2 <= 100) ? spo2 : 0;
          float sendAvgBPM = (validReadings > 0) ? avgHeartRate : 0;
          
          payload = "{\"BPM\":" + String(sendBPM) + 
                   ", \"AvgBPM\":" + String(sendAvgBPM, 1) + 
                   ", \"SpO2\":" + String(sendSpO2) + 
                   ", \"Battery\":" + String(batteryPercent) + 
                   ", \"Status\":\"" + status + "\"" +
                   ", \"timestamp\":" + String(millis()) + "}";
        }
        
        mqttSuccess = client.publish("v1/devices/me/telemetry", payload.c_str());
        lastMqttSend = millis();
        
        Serial.print(" | MQTT: ");
        Serial.println(mqttSuccess ? "✅ OK" : "❌ FAIL");
        
        if (!mqttSuccess) {
          Serial.println("🔄 MQTT publish failed, checking connection...");
        }
      } else {
        Serial.println(" | MQTT: ❌ DISCONNECTED");
      }
    }

    // Cập nhật OLED
    /*display.clearDisplay();
    display.setCursor(0,0);
    
    if(irBuffer[99] < 50000) {
      display.print("BPM: -- \n");
      display.print("Avg BPM: -- \n");
      display.print("SpO2: -- \n");
      Serial.print(status);
    } else {
      display.print("BPM: ");
      display.println((validHeartRate == 1 && heartRate > 40 && heartRate < 200) ? heartRate : 0);
      display.print("Avg BPM: ");
      display.println((validReadings > 0) ? avgHeartRate : 0, 1);
      display.print("SpO2: ");
      display.println((validSPO2 == 1 && spo2 > 80 && spo2 <= 100) ? spo2 : 0);
      Serial.print(status);
    }
    
    display.print("Pin: ");
    display.print(batteryPercent);
    display.println("%");
    display.print("WiFi: ");
    display.println(WiFi.status() == WL_CONNECTED ? "OK" : "FAIL");
    display.print("MQTT: ");
    display.println(client.connected() ? "OK" : "FAIL");
    display.display();*/
    
    // Ngắt nghỉ ngắn để tránh watchdog timer reset trên ESP32
    yield();
    delay(100);
  }
}