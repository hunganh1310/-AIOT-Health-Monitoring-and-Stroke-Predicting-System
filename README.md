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
