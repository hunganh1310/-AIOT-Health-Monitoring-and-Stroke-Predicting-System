import requests
import zipfile
import io
import os

# Link dữ liệu
url = "https://zenodo.org/record/6807403/files/mimic_perform_non_af_csv.zip?download=1"

# Tạo thư mục lưu
save_dir = "mimic_perform_non_af_csv"
os.makedirs(save_dir, exist_ok=True)

print("🔽 Đang tải dữ liệu từ Zenodo...")
response = requests.get(url)
if response.status_code == 200:
    print("✅ Tải thành công, đang giải nén...")
    with zipfile.ZipFile(io.BytesIO(response.content)) as zip_ref:
        zip_ref.extractall(save_dir)
    print(f"🎉 Giải nén xong! Dữ liệu nằm trong thư mục: {save_dir}")
else:
    print(f"❌ Lỗi khi tải dữ liệu: {response.status_code}")
