*Vietnamese below*
# AIOT Health Monitoring and Stroke Predicting System

## 📌 Introduction
The **AIOT Health Monitoring and Stroke Predicting System** project aims to build a real-time health monitoring system focusing on detecting Atrial Fibrillation (AF) — a key factor in **early stroke prediction**.

The system combines **IoT + Machine Learning** to collect biometric data, analyze it, and send alerts immediately when abnormalities are detected.

---

## 🛠️ Hardware
- **PPG Sensor**: MAX30102  
- **Microcontroller**: ESP32-WROOM-32  
- **OLED Display**: SSD1306 (displaying heart rate & connection status)  
- **Power Supply**: Lithium battery + charging circuit  
- **Connectivity**: WiFi

---

## ☁️ Management and Monitoring Platform
- **ThingsBoard**: Device management, dashboard display, and PPG data storage
- **MQTT**: Communication between ESP32 and the server

---

## 🤖 Machine Learning (ML)
- **Algorithm**: Random Forest  
- **Goal**: Extract features from PPG signals and classify **AF / Non-AF**
- **Sampling Frequency**: 125 Hz
- **Training Dataset**: [MIMIC-III Waveform Database – Perform subset](https://physionet.org/)  
- **Preprocessing**:
  - PPG signal noise filtering
  - Data normalization (`scaler`)
  - Missing value handling (`imputer`)

---

## 📡 Alerts
- When **AF** is detected, the system sends an **SMS alert** to the user via **Twilio**.

---

## 🚀 How to Run
### 1. ESP32
- Connect MAX30102 and SSD1306 to ESP32
- Flash the `PPG.ino` code
- Update WiFi & MQTT Broker credentials in the code

### 2. Server (PC / Raspberry Pi)
- Install dependencies:
```bash
pip install paho-mqtt joblib pandas numpy scikit-learn twilio
```
- Run the script:
```bash
python af_monitor.py
```

---

## 📷 Demo
![Demo Video](demo.gif)  
*(GIF demonstrating system operation)*

---

## 📌 Notes
- Ensure the PPG sampling frequency on ESP32 matches `FS` in `af_monitor.py`
- A **Twilio** account is required to send SMS alerts
- **ThingsBoard** and the MQTT Broker must be configured before running

---

## 📜 License
This project is intended for research and educational purposes only. It is **not** for medical diagnosis use.

(Vietnamese)
# AIOT Health Monitoring and Stroke Predicting System

## 📌 Giới thiệu
Dự án **AIOT Health Monitoring and Stroke Predicting System** nhằm xây dựng một hệ thống giám sát sức khỏe thời gian thực, tập trung vào việc phát hiện rung nhĩ (AF – Atrial Fibrillation), một yếu tố quan trọng giúp **dự báo sớm nguy cơ đột quỵ**.

Hệ thống sử dụng **IoT + Machine Learning** để thu thập dữ liệu sinh học, phân tích và gửi cảnh báo ngay khi phát hiện bất thường.

---

## 🛠️ Phần cứng
- **Cảm biến PPG**: MAX30102  
- **Vi điều khiển**: ESP32-WROOM-32  
- **Màn hình OLED**: SSD1306 (hiển thị nhịp tim & trạng thái kết nối)  
- **Nguồn**: Pin Lithium + mạch sạc  
- **Kết nối**: WiFi

---

## ☁️ Nền tảng quản lý và theo dõi
- **ThingsBoard**: Quản lý thiết bị, hiển thị dashboard, lưu trữ dữ liệu PPG
- **MQTT**: Giao tiếp giữa ESP32 và server

---

## 🤖 Máy học (ML)
- **Thuật toán**: Random Forest  
- **Mục tiêu**: Trích xuất đặc trưng từ tín hiệu PPG và phân loại **AF / Non-AF**
- **Tần số lấy mẫu**: 125 Hz
- **Bộ dữ liệu huấn luyện**: [MIMIC-III Waveform Database – Perform subset](https://physionet.org/)  
- **Tiền xử lý**:
  - Lọc nhiễu tín hiệu PPG
  - Chuẩn hóa dữ liệu (`scaler`)
  - Xử lý giá trị thiếu (`imputer`)

---

## 📡 Cảnh báo
- Khi phát hiện **AF**, hệ thống sẽ gửi **SMS cảnh báo** đến người dùng thông qua dịch vụ **Twilio**.

---

## 🚀 Cách chạy
### 1. ESP32
- Kết nối MAX30102 và SSD1306 với ESP32
- Flash code `PPG.ino`
- Cập nhật thông tin WiFi & MQTT Broker trong code

### 2. Server (PC / Raspberry Pi)
- Cài thư viện:
```bash
pip install paho-mqtt joblib pandas numpy scikit-learn twilio
```
- Chạy script:
```bash
python af_monitor.py
```

---

## 📷 Demo
![Demo Video](demo.gif)  
*(GIF demo hệ thống hoạt động)*

---

## 📌 Ghi chú
- Đảm bảo tần số lấy mẫu PPG của ESP32 khớp với `FS` trong `af_monitor.py`
- Cần tài khoản **Twilio** để gửi SMS
- Cần cấu hình **ThingsBoard** và MQTT Broker trước khi chạy

---

## 📜 Giấy phép
Dự án này sử dụng cho mục đích nghiên cứu và học tập. Không sử dụng cho mục đích chẩn đoán y tế.
