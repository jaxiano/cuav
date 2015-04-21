# determine camera built
import json, os, zipfile

# allow for replaying of previous flights
import cuav.camera.fake_chameleon as chameleon
import settings

if os.getenv('FAKE_CHAMELEON'):
    if settings.camera == "chameleon":
       print("Loaded chameleon mock backend")
       import cuav.camera.fake_chameleon as chameleon
    elif settings.camera == "flea":
       print("Loaded flea mock backend")
       import cuav.camera.mock_flea as chameleon
else:
    if settings.camera == "flea":
       print("Loaded flea backend")
       import cuav.camera.flea as chameleon

def getCamera():
    return chameleon