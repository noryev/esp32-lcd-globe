from flask import Flask, send_file
from PIL import Image
import requests
import shutil
import os
import numpy as np

app = Flask(__name__)

def download_image():
    response = requests.get("https://epic.gsfc.nasa.gov/api/natural")
    data = response.json()
    most_recent = data[0]
    year, month, day = most_recent["date"].split(" ")[0].split("-")
    name = most_recent["image"] + ".png"
    archive = f"https://epic.gsfc.nasa.gov/archive/natural/{year}/{month}/{day}/png/"
    source = archive + name
    destination = os.path.join(os.path.expanduser("~"), "Desktop", name)

    response = requests.get(source, stream=True)
    with open(destination, 'wb') as out_file:
        shutil.copyfileobj(response.raw, out_file)
    del response

    return destination

@app.route("/image.raw")
def image():
    image_path = download_image()
    img = Image.open(image_path)

    img = img.resize((240, 240), Image.LANCZOS)

    # Convert image to RGB565 format
    np_img = np.array(img)
    r = (np_img[:, :, 0] >> 3).astype(np.uint16)
    g = (np_img[:, :, 1] >> 2).astype(np.uint16)
    b = (np_img[:, :, 2] >> 3).astype(np.uint16)
    rgb565 = ((r << 11) | (g << 5) | b)

    # Save RGB565 pixel data as raw file
    raw_filename = os.path.join(os.path.expanduser("~"), "Desktop", "image.raw")
    rgb565.tofile(raw_filename)

    return send_file(raw_filename, mimetype='application/octet-stream')

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8000)
