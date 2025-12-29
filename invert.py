# invert_image.py

from PIL import Image, ImageOps
import os
import sys

def invert_image(input_path, output_path):
    try:
        img = Image.open(input_path)

        # Convert to RGB if needed (handles RGBA, L, etc.)
        if img.mode not in ("RGB", "RGBA"):
            img = img.convert("RGB")

        # Handle alpha channel properly
        if img.mode == "RGBA":
            r, g, b, a = img.split()
            rgb = Image.merge("RGB", (r, g, b))
            inverted_rgb = ImageOps.invert(rgb)
            inverted = Image.merge("RGBA", (*inverted_rgb.split(), a))
        else:
            inverted = ImageOps.invert(img)

        inverted.save(output_path)
        print(f"Saved inverted image as: {output_path}")

    except FileNotFoundError:
        print("Error: logo.png not found in current directory.")
        sys.exit(1)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

def main():
    input_file = "logo.png"
    output_file = "logo_inverted.png"

    cwd = os.getcwd()
    input_path = os.path.join(cwd, input_file)
    output_path = os.path.join(cwd, output_file)

    invert_image(input_path, output_path)

if __name__ == "__main__":
    main()
