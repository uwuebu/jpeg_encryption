import cv2
import numpy as np

def calculate_ycbcr_image_correlation(img_path1, img_path2):
    # Load both images in BGR format
    img1 = cv2.imread(img_path1, cv2.IMREAD_COLOR)
    img2 = cv2.imread(img_path2, cv2.IMREAD_COLOR)

    if img1 is None or img2 is None:
        print("Failed to load one or both images.")
        return

    if img1.shape != img2.shape:
        print("Images must have the same dimensions and number of channels.")
        return

    # Convert both images to YCrCb (OpenCV format)
    img1_ycrcb = cv2.cvtColor(img1, cv2.COLOR_BGR2YCrCb).astype(np.float32)
    img2_ycrcb = cv2.cvtColor(img2, cv2.COLOR_BGR2YCrCb).astype(np.float32)

    channels = ('Y (Luminance)', 'Cr (Chroma Red)', 'Cb (Chroma Blue)')

    for i, name in enumerate(channels):
        ch1 = img1_ycrcb[:, :, i].flatten()
        ch2 = img2_ycrcb[:, :, i].flatten()
        corr = np.corrcoef(ch1, ch2)[0, 1]
        print(f"{name} channel correlation between images: {corr:.4f}")

# Example usage
calculate_ycbcr_image_correlation("file_original.jpg", "file_enc.jpg")
