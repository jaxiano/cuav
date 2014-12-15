# determine camera built
import json, os

# allow for replaying of previous flights
import cuav.camera.fake_chameleon as chameleon

with open("cuav/data/build_config.json", "r") as jsonFile:
    build_config = json.load(jsonFile)


if os.getenv('FAKE_CHAMELEON'):
    if build_config["camera"] == "chameleon":
       print("Loaded chameleon mock backend")
       import cuav.camera.fake_chameleon as chameleon
    elif build_config["camera"] == "flea":
       print("Loaded flea mock backend")
       import cuav.camera.mock_flea as chameleon
else:
    if build_config["camera"] == "flea":
       print("Loaded flea backend")
       import cuav.camera.flea as chameleon

def getCamera():
    return chameleon