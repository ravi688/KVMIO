from PIL import Image
import numpy as np
import sys

def rgb_to_nv12(rgb_img: np.ndarray) -> bytes:
    """Convert RGB -> NV12 (YUV420)"""
    R, G, B = rgb_img[:,:,0].astype(np.float32), rgb_img[:,:,1].astype(np.float32), rgb_img[:,:,2].astype(np.float32)
    Y = 0.299*R + 0.587*G + 0.114*B
    U = -0.169*R - 0.331*G + 0.5*B + 128
    V = 0.5*R - 0.419*G - 0.081*B + 128

    H, W = Y.shape
    # Downsample U and V (2x2)
    U_sub = U.reshape(H//2, 2, W//2, 2).mean(axis=(1,3))
    V_sub = V.reshape(H//2, 2, W//2, 2).mean(axis=(1,3))

    UV = np.empty((H//2, W), dtype=np.uint8)
    UV[:, 0::2] = U_sub.astype(np.uint8)
    UV[:, 1::2] = V_sub.astype(np.uint8)

    nv12 = np.concatenate([Y.astype(np.uint8).flatten(), UV.flatten()])
    return nv12.tobytes()


def rgb_to_yuv422(rgb_img: np.ndarray) -> bytes:
    """Convert RGB -> YUV422 (packed YUYV)"""
    R, G, B = rgb_img[:,:,0].astype(np.float32), rgb_img[:,:,1].astype(np.float32), rgb_img[:,:,2].astype(np.float32)
    Y = 0.299*R + 0.587*G + 0.114*B
    U = -0.169*R - 0.331*G + 0.5*B + 128
    V = 0.5*R - 0.419*G - 0.081*B + 128

    H, W = Y.shape
    if W % 2 != 0:
        Y = Y[:, :-1]; U = U[:, :-1]; V = V[:, :-1]; W -= 1

    yuv422 = np.empty((H, W*2), dtype=np.uint8)
    yuv422[:, 0::4] = Y[:, 0::2].astype(np.uint8)
    yuv422[:, 1::4] = U[:, 0::2].astype(np.uint8)
    yuv422[:, 2::4] = Y[:, 1::2].astype(np.uint8)
    yuv422[:, 3::4] = V[:, 0::2].astype(np.uint8)

    return yuv422.tobytes()


def save_image_as_format(img_path: str, out_path: str, fmt: str):
    img = Image.open(img_path).convert("RGB")
    w, h = img.size
    if w % 2 != 0:
        img = img.crop((0, 0, w & ~1, h & ~1))
    rgb = np.array(img)

    fmt = fmt.lower()
    if fmt == "rgb":
        data = rgb.astype(np.uint8).tobytes()
    elif fmt == "nv12":
        data = rgb_to_nv12(rgb)
    elif fmt in ["yuv422", "yuv 4:2:2", "yuyv"]:
        data = rgb_to_yuv422(rgb)
    else:
        raise ValueError(f"Unsupported format: {fmt}")

    with open(out_path, "wb") as f:
        f.write(data)

    print(f"Image saved as {fmt} to {out_path}")


if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: python convert_image.py <input.jpg> <output.raw> <format>")
        print("Supported formats: rgb, nv12, yuv422")
        sys.exit(1)

    _, input_path, output_path, target_format = sys.argv
    save_image_as_format(input_path, output_path, target_format)
