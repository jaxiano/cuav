# determine camera built
import json, os, zipfile

# allow for replaying of previous flights
import cuav.camera.fake_chameleon as sensor
import settings

if os.getenv('FAKE_CHAMELEON'):
    if settings.camera == "chameleon":
        print("Loaded chameleon mock backend")
        import cuav.camera.fake_chameleon as sensor
    elif settings.camera == "flea":
        print("Loaded flea mock backend")
        import cuav.camera.flea as sensor
    elif settings.camera == "tau":
        print("Loaded tau mock backend")
        import cuav.camera.mock_tau as sensor
else:
    if settings.camera == "flea":
	print("Loaded flea backend")
        import cuav.camera.flea as sensor
    elif settings.camera == "tau":
	print("Loaded tau backend")
	import cuav.camera.tau as sensor

def getCamera():
    return sensor
