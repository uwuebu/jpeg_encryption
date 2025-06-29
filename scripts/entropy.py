import cv2
import numpy as np
from skimage.util import view_as_windows
from scipy.stats import entropy
import matplotlib.pyplot as plt

def compute_entropy(hist):
    """Обчислює ентропію з гістограми піксельних значень."""
    hist = hist[hist > 0]  # ігноруємо нулі
    prob = hist / np.sum(hist)
    return -np.sum(prob * np.log2(prob))

def global_entropy(channel):
    """Глобальна ентропія каналу."""
    hist, _ = np.histogram(channel.flatten(), bins=256, range=(0, 256))
    return compute_entropy(hist)

def local_entropy(channel, window_size=8):
    """Локальна ентропія для кожного вікна."""
    windows = view_as_windows(channel, (window_size, window_size), step=window_size)
    entropies = np.zeros(windows.shape[:2])

    for i in range(entropies.shape[0]):
        for j in range(entropies.shape[1]):
            window = windows[i, j].flatten()
            hist, _ = np.histogram(window, bins=256, range=(0, 256))
            entropies[i, j] = compute_entropy(hist)

    return entropies

def entropy_analysis_ycbcr(image_path):
    img = cv2.imread(image_path, cv2.IMREAD_COLOR)
    if img is None:
        print("Не вдалося завантажити зображення.")
        return

    # Перетворення в YCrCb (де Y - яскравість, Cr - хрома червона, Cb - хрома синя)
    img_ycrcb = cv2.cvtColor(img, cv2.COLOR_BGR2YCrCb)
    channels = ('Y (Luminance)', 'Cr (Chroma Red)', 'Cb (Chroma Blue)')

    for i, name in enumerate(channels):
        channel = img_ycrcb[:, :, i]

        global_ent = global_entropy(channel)
        local_entropies = local_entropy(channel, window_size=8)
        avg_local_ent = np.mean(local_entropies)

        print(f"\nАналіз ентропії для {name} каналу:")
        print(f"  Глобальна ентропія: {global_ent:.4f}")
        print(f"  Середня локальна ентропія: {avg_local_ent:.4f}")

        # Побудова карти локальної ентропії
        plt.figure(figsize=(5, 4))
        plt.imshow(local_entropies, cmap='hot', interpolation='nearest')
        plt.colorbar(label='Локальна ентропія')
        plt.title(f"Карта локальної ентропії ({name})")
        plt.axis('off')
        plt.tight_layout()
        plt.show()

# Приклад використання
entropy_analysis_ycbcr("file_enc.jpg")
