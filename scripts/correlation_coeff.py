import cv2
import numpy as np

def calculate_ycbcr_adjacent_pixel_correlation(image_path):
    # Load image in BGR (default in OpenCV)
    img = cv2.imread(image_path, cv2.IMREAD_COLOR)
    if img is None:
        print("Failed to load image.")
        return

    # Convert to YCrCb color space (OpenCV uses YCrCb, where Cr=chrominance red, Cb=chrominance blue)
    img_ycrcb = cv2.cvtColor(img, cv2.COLOR_BGR2YCrCb)
    img_ycrcb = img_ycrcb.astype(np.float32)

    channels = ('Y (Luminance)', 'Cr (Chroma Red)', 'Cb (Chroma Blue)')
    directions = ['Horizontal', 'Vertical']

    for i, name in enumerate(channels):
        channel = img_ycrcb[:, :, i]

        # Horizontal pairs
        x_h = channel[:, :-1].flatten()
        y_h = channel[:, 1:].flatten()
        corr_h = np.corrcoef(x_h, y_h)[0, 1]

        # Vertical pairs
        x_v = channel[:-1, :].flatten()
        y_v = channel[1:, :].flatten()
        corr_v = np.corrcoef(x_v, y_v)[0, 1]

        print(f"{name}:")
        print(f"  Horizontal correlation: {corr_h:.4f}")
        print(f"  Vertical correlation:   {corr_v:.4f}")

# Example usage
calculate_ycbcr_adjacent_pixel_correlation("file_enc.jpg")
# calculate_ycbcr_adjacent_pixel_correlation("lena.jpg")
