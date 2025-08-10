import requests
import zipfile
import io
import os

# Link dữ liệu
url = "https://zenodo.org/record/6807403/files/mimic_perform_non_af_csv.zip?download=1"

# Tạo thư mục lưu
save_dir = "mimic_perform_non_af_csv"
os.makedirs(save_dir, exist_ok=True)

response = requests.get(url)
if response.status_code == 200:
    with zipfile.ZipFile(io.BytesIO(response.content)) as zip_ref:
        zip_ref.extractall(save_dir)
