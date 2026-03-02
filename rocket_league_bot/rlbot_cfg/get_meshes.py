import urllib.request
import zipfile
import os

urls = [
    "https://github.com/ZealanL/RocketSim/releases/download/v1.1.0/collision_meshes.zip",
    "https://github.com/ZealanL/RocketSim/releases/download/v1.0.0/collision_meshes.zip",
    "https://github.com/ZealanL/RocketSim/releases/latest/download/collision_meshes.zip",
]

for url in urls:
    try:
        print(f"Trying: {url}")
        urllib.request.urlretrieve(url, "cm.zip")
        size = os.path.getsize("cm.zip")
        print(f"Downloaded {size} bytes")
        if size > 1000:
            zipfile.ZipFile("cm.zip").extractall(".")
            print("Extracted! Done.")
            break
        else:
            print("File too small, trying next...")
    except Exception as e:
        print(f"Failed: {e}")