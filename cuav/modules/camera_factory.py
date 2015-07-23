# determine camera built
import json, os, zipfile

# allow for replaying of previous flights
import settings

if os.getenv('FAKE_CHAMELEON'):
    if settings.camera == "tau":
        print("Loaded tau mock backend")
        import cuav.camera.mock_tau as sensor
    else:
        print("Loaded flea mock backend")
        import cuav.camera.mock_flea as sensor
else:
    if settings.camera == "tau":
	print("Loaded tau backend")
	import cuav.camera.tau as sensor
    else:
	print("Loaded flea backend")
        import cuav.camera.flea as sensor

def getCamera():
    return sensor
