import cv2
import numpy as np

def mse(img1, img2):
    return np.mean((img1 - img2) ** 2)

def mae(img1, img2):
    return np.mean(np.abs(img1 - img2))

def psnr(img1, img2):
    mse_val = mse(img1, img2)
    if mse_val == 0:
        return float('inf')  # Ідеально однакові зображення
    max_pixel = 255.0
    return 20 * np.log10(max_pixel / np.sqrt(mse_val))

def encryption_quality_metrics_ycbcr(original_path, encrypted_path):
    # Завантаження зображень
    orig = cv2.imread(original_path, cv2.IMREAD_COLOR)
    enc = cv2.imread(encrypted_path, cv2.IMREAD_COLOR)

    if orig is None or enc is None:
        print("Помилка: одне з зображень не завантажено.")
        return

    if orig.shape != enc.shape:
        print("Помилка: зображення мають різні розміри або кількість каналів.")
        return

    # Перетворення в YCrCb та float32
    orig = cv2.cvtColor(orig, cv2.COLOR_BGR2YCrCb).astype(np.float32)
    enc = cv2.cvtColor(enc, cv2.COLOR_BGR2YCrCb).astype(np.float32)

    channels = ('Y (Luminance)', 'Cr (Chroma Red)', 'Cb (Chroma Blue)')

    for i, name in enumerate(channels):
        orig_ch = orig[:, :, i]
        enc_ch = enc[:, :, i]

        mse_val = mse(orig_ch, enc_ch)
        mae_val = mae(orig_ch, enc_ch)
        psnr_val = psnr(orig_ch, enc_ch)

        print(f"\nМетрики для каналу {name}:")
        print(f"  MSE  (середньоквадратична похибка):      {mse_val:.2f}")
        print(f"  MAE  (середня абсолютна похибка):        {mae_val:.2f}")
        print(f"  PSNR (пікове відношення сигнал/шум):     {psnr_val:.2f} дБ")

# Приклад використання
encryption_quality_metrics_ycbcr("file_original.jpg", "file_enc.jpg")
