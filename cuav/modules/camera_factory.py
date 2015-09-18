# determine camera built
import json, os, zipfile

# allow for replaying of previous flights
import settings

if os.getenv('FAKE_CHAMELEON'):
    if settings.camera == "tau":
        print("Loaded tau mock backend")
        import cuav.camera.mock_tau as sensor
    elif settings.camera == "flea":
        print("Loaded flea mock backend")
        import cuav.camera.mock_flea as sensor
    elif settings.camera == "ids":
        print("Loaded ids mock backend")
        import cuav.camera.mock_ids as sensor
    else:
        print("Camera not supported: %s" % settings.camera)
else:
    if settings.camera == "tau":
        print("Loaded tau backend")
        import cuav.camera.tau as sensor
    elif settings.camera == "flea":
        print("Loaded flea backend")
        import cuav.camera.flea as sensor
    elif settings.camera == "ids":
        print("Loaded ids backend")
        import cuav.camera.ids as sensor
    else:
        print("Camera not supported: %s" % settings.camera)

def getCamera():
    return sensor
