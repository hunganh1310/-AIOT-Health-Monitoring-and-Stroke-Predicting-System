#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "MAX30105.h"

MAX30105 particleSensor;

//////////////////////
// WiFi + MQTT config
//////////////////////
const char *ssid = "FPT";
const char *password = "80812004";
const char *mqtt_server = "192.168.1.9";
const int mqtt_port = 1883;
const char *mqtt_topic = "ppg/patch";

WiFiClient espClient;
PubSubClient client(espClient);

//////////////////////
// Sampling config
//////////////////////
#define FS 125               // Hz
#define PATCH_SIZE 250       // samples (2s)

unsigned long sampleInterval = 1000 / FS;
unsigned long lastSampleTime = 0;

long patchBuffer[PATCH_SIZE];
int patchIndex = 0;

//////////////////////
// WiFi + MQTT setup
//////////////////////
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("🔌 Connecting to WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("🔄 Attempting MQTT connection...");
    if (client.connect("ESP32_PPG_Client")) {
      Serial.println("✅ connected");
    } else {
      Serial.print("❌ failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5s");
      delay(5000);
    }
  }
}

//////////////////////
// Setup
//////////////////////
void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  // Khởi động WiFi
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  
  // Tăng buffer size cho MQTT client
  client.setBufferSize(8192);  // Tăng từ 256 bytes mặc định lên 8KB
  
  // Khởi động MAX30102
  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("❌ MAX30102 not found. Please check wiring.");
    while (1);
  }
  
  // Config MAX30102
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);  // LED Red thấp
  particleSensor.setPulseAmplitudeGreen(0);   // Tắt Green
  
  Serial.println("🚀 Setup completed");
}

//////////////////////
// Loop
//////////////////////
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  unsigned long now = millis();
  if (now - lastSampleTime >= sampleInterval) {
    lastSampleTime = now;
    
    long irValue = particleSensor.getIR();
    patchBuffer[patchIndex++] = irValue;
    
    // Debug: In giá trị IR mỗi 50 samples
    if (patchIndex % 50 == 0) {
      Serial.print("📊 Sample ");
      Serial.print(patchIndex);
      Serial.print("/");
      Serial.print(PATCH_SIZE);
      Serial.print(" - IR: ");
      Serial.println(irValue);
    }
    
    if (patchIndex >= PATCH_SIZE) {
      Serial.println("📦 Preparing patch...");
      
      // Kiểm tra điều kiện lọc nhiễu: tất cả samples phải > 50000
      bool validPatch = true;
      int invalidCount = 0;
      
      for (int i = 0; i < PATCH_SIZE; i++) {
        if (patchBuffer[i] <= 50000) {
          validPatch = false;
          invalidCount++;
        }
      }
      
      if (!validPatch) {
        Serial.print("🚫 Patch rejected - ");
        Serial.print(invalidCount);
        Serial.print(" samples ≤ 50000 (");
        Serial.print((invalidCount * 100.0) / PATCH_SIZE, 1);
        Serial.println("%)");
        patchIndex = 0;  // reset buffer
        return;
      }
      
      Serial.println("✅ Valid patch - all samples > 50000");
      
      // Gom thành payload CSV với timestamp và giá trị PPG
      String payload = "";
      payload.reserve(PATCH_SIZE * 20); // Reserve memory trước
      
      for (int i = 0; i < PATCH_SIZE; i++) {
        float timestamp = i * (1.0 / FS);  // timestamp = index * (1/125) giây
        float scaled = patchBuffer[i] / 100000.0;  // chuẩn hóa PPG value
        
        payload += String(timestamp, 3) + "," + String(scaled, 12) + "\n";
      }
      
      // Debug thông tin payload
      Serial.print("📏 Payload size: ");
      Serial.print(payload.length());
      Serial.println(" bytes");
      
      // Kiểm tra kết nối trước khi publish
      if (!client.connected()) {
        Serial.println("⚠️ MQTT disconnected, reconnecting...");
        reconnect();
      }
      
      // Publish lên MQTT với retain = false
      Serial.println("📤 Publishing to MQTT...");
      bool result = client.publish(mqtt_topic, payload.c_str(), false);
      
      if (result) {
        Serial.println("✅ Patch sent to MQTT successfully");
      } else {
        Serial.println("❌ Failed to send patch");
        Serial.print("🔍 MQTT state: ");
        Serial.println(client.state());
        Serial.print("🔍 WiFi status: ");
        Serial.println(WiFi.status());
        
        // Thử reconnect nếu failed
        if (!client.connected()) {
          reconnect();
        }
      }
      
      patchIndex = 0;  // reset buffer
      Serial.println("🔄 Buffer reset, collecting new patch...\n");
    }
  }
}