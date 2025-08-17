import json
import time
from collections import deque
import threading

import paho.mqtt.client as mqtt
import numpy as np
import pandas as pd
import joblib

# Import hàm extract_features từ code bạn đã viết
from feature_extraction import extract_features

# ========== Cấu hình ==========

# MQTT broker - Sửa IP để match với ESP32
MQTT_BROKER = "192.168.1.9"   # Phải match với mqtt_server trong ESP32
MQTT_PORT = 1883
MQTT_TOPIC = "ppg/patch"

FS = 125  # Tần số lấy mẫu của PPG (Hz)
WINDOW_SECONDS = 10  # Gom đủ 10 giây (1250 samples)
PATCH_SIZE = 250  # ESP32 gửi 250 samples/patch (2 giây)

# Model và preprocessing
MODEL_PATH = "rf_model.pkl"
SCALER_PATH = "scaler.pkl"
IMPUTER_PATH = "imputer.pkl"

# ========== Load model & preprocess ==========
print("🔄 Loading model and preprocess objects...")
try:
    rf_model = joblib.load(MODEL_PATH)
    scaler = joblib.load(SCALER_PATH)
    imputer = joblib.load(IMPUTER_PATH)
    print("✅ Models loaded successfully")
except Exception as e:
    print(f"❌ Error loading models: {e}")
    exit(1)

# Buffer lưu dữ liệu PPG - sử dụng thread-safe
ppg_buffer = deque(maxlen=FS * WINDOW_SECONDS * 2)  # Buffer lớn hơn để tránh mất data
buffer_lock = threading.Lock()


# ========== MQTT Callbacks ==========
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("✅ Connected to MQTT broker")
        client.subscribe(MQTT_TOPIC)
        print(f"📡 Subscribed to topic: {MQTT_TOPIC}")
    else:
        print(f"❌ Failed to connect to MQTT broker, return code {rc}")

def on_disconnect(client, userdata, rc):
    print(f"⚠️ Disconnected from MQTT broker (rc: {rc})")

def on_message(client, userdata, msg):
    global ppg_buffer
    try:
        payload = msg.payload.decode().strip()
        print(f"📥 Received patch: {len(payload)} bytes")

        # Parse CSV data từ ESP32: "timestamp,ppg_value"
        ppg_values = []
        lines = payload.splitlines()
        
        for line in lines:
            line = line.strip()
            if not line:
                continue
                
            parts = line.split(',')
            if len(parts) == 2:
                try:
                    timestamp = float(parts[0])
                    ppg_value = float(parts[1])
                    ppg_values.append(ppg_value)
                except ValueError as e:
                    print(f"⚠️ Invalid line: {line} - {e}")
                    continue

        # Thread-safe buffer update
        with buffer_lock:
            for value in ppg_values:
                ppg_buffer.append(value)
            
            current_buffer_size = len(ppg_buffer)
        
        print(f"📊 Buffer: {current_buffer_size}/{FS * WINDOW_SECONDS} samples (+{len(ppg_values)} new)")

        # Kiểm tra nếu đủ dữ liệu để phân tích (10 giây = 1250 samples)
        if current_buffer_size >= FS * WINDOW_SECONDS:
            # Lấy 1250 samples gần nhất để phân tích
            with buffer_lock:
                analysis_data = list(ppg_buffer)[-FS * WINDOW_SECONDS:]
            
            print(f"🔍 Processing {len(analysis_data)} samples for AF detection...")
            
            # Xử lý trong thread riêng để không block MQTT
            threading.Thread(
                target=process_ppg_window, 
                args=(analysis_data,), 
                daemon=True
            ).start()

    except Exception as e:
        print(f"❌ Error processing MQTT message: {e}")

# ========== Hàm xử lý 10 giây PPG ==========
def process_ppg_window(ppg_signal):
    try:
        print(f"⚙️ Analyzing {len(ppg_signal)} samples...")
        
        # Convert to numpy array
        ppg_array = np.array(ppg_signal)
        
        # Basic signal quality check
        if np.std(ppg_array) < 0.001:  # Signal quá phẳng
            print("⚠️ Poor signal quality - skipping analysis")
            return
        
        # Extract features
        features = extract_features(ppg_array)
        X_new = pd.DataFrame([features])
        
        print(f"📈 Extracted {len(features)} features")

        # Preprocess
        X_imputed = imputer.transform(X_new)
        X_scaled = scaler.transform(X_imputed)

        # Predict
        y_pred = rf_model.predict(X_scaled)
        y_proba = rf_model.predict_proba(X_scaled)[:, 1][0]

        result = "🟢 Normal" if y_pred[0] == 1 else "🔴 AF DETECTED"
        print(f"🎯 Result: {result} (Probability: {y_proba:.3f})")

        # Gửi cảnh báo nếu phát hiện AF
        if y_pred[0] == 1 and y_proba > 0.7:  # Threshold cao để tránh false alarm
            send_sms_alert(y_proba)
            
    except Exception as e:
        print(f"❌ Error in PPG analysis: {e}")

# ========== Main ==========
def main():
    # Tạo MQTT client
    client = mqtt.Client(client_id="PPG_AF_Detector")
    client.on_connect = on_connect
    client.on_disconnect = on_disconnect
    client.on_message = on_message
    
    # Kết nối đến broker
    try:
        print(f"🔌 Connecting to MQTT broker {MQTT_BROKER}:{MQTT_PORT}...")
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        
        print("🎯 Starting AF detection system...")
        print("📡 Waiting for PPG data from ESP32...")
        print("Press Ctrl+C to stop\n")
        
        # Bắt đầu loop
        client.loop_forever()
        
    except KeyboardInterrupt:
        print("\n🛑 Stopping AF detection system...")
        client.disconnect()
        
    except Exception as e:
        print(f"❌ Connection error: {e}")

if __name__ == "__main__":
    main()