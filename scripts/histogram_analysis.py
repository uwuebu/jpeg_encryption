import cv2
import numpy as np
import matplotlib.pyplot as plt
from scipy.stats import chisquare

def histogram_analysis_ycbcr(image_path):
    img = cv2.imread(image_path, cv2.IMREAD_COLOR)
    if img is None:
        print("Не вдалося завантажити зображення.")
        return

    # Перетворення BGR → YCrCb (де Y - яскравість, Cr - хрома червона, Cb - хрома синя)
    img_ycrcb = cv2.cvtColor(img, cv2.COLOR_BGR2YCrCb)
    channels = ('Y (Luminance)', 'Cr (Chroma Red)', 'Cb (Chroma Blue)')
    num_bins = 256
    uniform_freq = img.shape[0] * img.shape[1] / num_bins

    for i, channel_name in enumerate(channels):
        channel_data = img_ycrcb[:, :, i].flatten()

        # Розрахунок гістограми
        hist, _ = np.histogram(channel_data, bins=num_bins, range=(0, 256))

        # Хі-квадрат тест
        chi2_stat, _ = chisquare(hist, f_exp=np.full(num_bins, uniform_freq))

        # Максимальне відхилення
        max_dev = np.max(np.abs(hist - uniform_freq))

        # Нерегулярне відхилення
        irregular_dev = np.sum(np.abs(hist - uniform_freq)) / num_bins

        # Відхилення від рівномірної гістограми (нормоване)
        deviation_uniform = np.linalg.norm(hist - uniform_freq) / num_bins

        print(f"\nАналіз гістограми для {channel_name}:")
        print(f"  Хі-квадрат: {chi2_stat:.2f}")
        print(f"  Максимальне відхилення: {max_dev:.2f}")
        print(f"  Нерегулярне відхилення: {irregular_dev:.2f}")
        print(f"  Відхилення від рівномірної гістограми: {deviation_uniform:.4f}")

        # Побудова гістограми
        plt.figure(figsize=(6, 4))
        plt.title(f"Гістограма ({channel_name})")
        plt.xlabel("Яскравість пікселя")
        plt.ylabel("Частота")
        plt.bar(range(num_bins), hist, color='gray')  # Гістограма в градаціях сірого
        plt.grid(True)
        plt.tight_layout()
        plt.show()

# Приклад використання
histogram_analysis_ycbcr("file_enc.jpg")
