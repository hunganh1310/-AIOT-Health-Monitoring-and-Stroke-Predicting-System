import json
import time
from collections import deque

import paho.mqtt.client as mqtt
import numpy as np
import pandas as pd
import joblib
from twilio.rest import Client

# Import hàm extract_features từ code bạn đã viết
from feature_extraction import extract_features

# ========== Cấu hình ==========

# MQTT broker
MQTT_BROKER = "192.168.1.100"   # IP broker hoặc tên miền
MQTT_PORT = 1883
MQTT_TOPIC = "ppg/data"         # topic ESP32 publish

FS = 125  # Tần số lấy mẫu của PPG (Hz)
WINDOW_SECONDS = 10  # Gom đủ 10 giây

# Twilio SMS
TWILIO_SID = "YOUR_TWILIO_SID_HERE"
TWILIO_AUTH_TOKEN = "TOKEN_HERE"
TWILIO_FROM = "+84889506638"  # số gửi
TWILIO_TO = "+848889506638"   # số nhận cảnh báo

# Model và preprocessing
MODEL_PATH = "rf_model.pkl"
SCALER_PATH = "scaler.pkl"
IMPUTER_PATH = "imputer.pkl"

# ========== Load model & preprocess ==========
print("Loading model and preprocess objects...")
rf_model = joblib.load(MODEL_PATH)
scaler = joblib.load(SCALER_PATH)
imputer = joblib.load(IMPUTER_PATH)

# Buffer lưu dữ liệu PPG
ppg_buffer = deque(maxlen=FS * WINDOW_SECONDS)

# Twilio client
sms_client = Client(TWILIO_SID, TWILIO_AUTH_TOKEN)

# ========== Hàm gửi cảnh báo ==========
def send_sms_alert(probability):
    msg = f"⚠️ Cảnh báo: Phát hiện nguy cơ AF! Xác suất = {probability:.2f}"
    sms_client.messages.create(
        body=msg,
        from_=TWILIO_FROM,
        to=TWILIO_TO
    )
    print("SMS alert sent!")

# ========== Xử lý khi nhận dữ liệu MQTT ==========
def on_message(client, userdata, msg):
    try:
        payload = msg.payload.decode()
        data = json.loads(payload)

        # Giả sử ESP32 gửi {"ppg": 512}
        ppg_value = float(data["ppg"])
        ppg_buffer.append(ppg_value)

        if len(ppg_buffer) >= FS * WINDOW_SECONDS:
            process_ppg_window(list(ppg_buffer))
            ppg_buffer.clear()

    except Exception as e:
        print("Error processing MQTT message:", e)

# ========== Hàm xử lý 10 giây PPG ==========
def process_ppg_window(ppg_signal):
    print(f"Processing {len(ppg_signal)} samples...")

    # Extract features
    features = extract_features(np.array(ppg_signal))
    X_new = pd.DataFrame([features])

    # Preprocess
    X_imputed = imputer.transform(X_new)
    X_scaled = scaler.transform(X_imputed)

    # Predict
    y_pred = rf_model.predict(X_scaled)
    y_proba = rf_model.predict_proba(X_scaled)[:, 1][0]

    print(f"Prediction: {'AF' if y_pred[0] == 1 else 'Non-AF'} (P = {y_proba:.3f})")

    if y_pred[0] == 1:
        send_sms_alert(y_proba)

# ========== Kết nối MQTT ==========
client = mqtt.Client()
client.on_message = on_message
client.connect(MQTT_BROKER, MQTT_PORT, 60)
client.subscribe(MQTT_TOPIC)

print("Listening for PPG data...")
client.loop_forever()
