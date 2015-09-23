
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <ueye.h>
#include <ueye_deprecated.h>

#define MAX_HEIGHT 2048
#define MAX_WIDTH 2048

HIDS hCam = 1;
char *pMem = NULL;
int memID = 0;
static bool initialized = false;

int open_camera(unsigned int new_height, unsigned int new_width)
{
	if (initialized == true)
	    return;

	int num_cam;
	is_GetNumberOfCameras(&num_cam);
	printf("# Cams Detected: %d\n", num_cam);

	int nRet; // = is_InitCamera(&hCam, NULL);
	do{
	    sleep(2);
	    nRet = is_InitCamera(&hCam, NULL);
	} while(nRet != IS_SUCCESS);
	initialized = true;

        nRet = is_ParameterSet( hCam, IS_PARAMETERSET_CMD_LOAD_EEPROM, NULL, 0 );
	printf("Status loading parameter %d\n", nRet);

	SENSORINFO DataFromSensor;
	if (is_GetSensorInfo(hCam, &DataFromSensor) == IS_SUCCESS) {
		printf("nColorMode = %d, dim = %d x %d\n", DataFromSensor.nColorMode, DataFromSensor.nMaxWidth, DataFromSensor.nMaxHeight);
	}
	else
		printf("is_GetSensorInfo Failed\n");

	INT nBitsPerPixel;
	switch(is_SetColorMode(hCam, IS_GET_COLOR_MODE)) {
		case IS_SET_CM_RGB32:
			nBitsPerPixel = 32;
			break;
		case IS_SET_CM_RGB24:
			nBitsPerPixel = 24;
			break;
		case IS_SET_CM_RGB16:
		case IS_SET_CM_UYVY:
			nBitsPerPixel = 16;
			break;
		case IS_SET_CM_RGB15:
			nBitsPerPixel = 15;
			break;
		case IS_SET_CM_Y8:
		case IS_SET_CM_RGB8:
		case IS_SET_CM_BAYER:
		default:
			nBitsPerPixel = 8;
	}
	printf("BitsPerPixel = %d\n", nBitsPerPixel);

	IS_RECT imageData;
	INT x,y,width,height;
	nRet = is_AOI(hCam, IS_AOI_IMAGE_GET_AOI, (void *)&imageData, sizeof(imageData));
	if (nRet == IS_SUCCESS) {
		x = imageData.s32X;
		y = imageData.s32Y;
		width = imageData.s32Width;
		height = imageData.s32Height;
		printf("AOI position x=%d, y=%d, width=%d, height=%d \n", x, y, width, height);
	}

	nRet = is_AllocImageMem(hCam, imageData.s32Width, imageData.s32Height, nBitsPerPixel, &pMem, &memID);
	printf("Status AllocImage %d\n", nRet);

	nRet = is_SetImageMem(hCam, pMem, memID);
	printf("Status is_SetImageMem %d\n", nRet);

	nRet = is_SetExternalTrigger(hCam, IS_SET_TRIGGER_SOFTWARE);
	printf("Status is_SetExternalTrigger %d\n", nRet);
		
	nRet = is_StopLiveVideo(hCam, IS_WAIT);
	printf("Status is_StopLiveVideo %d\n", nRet);

	return nRet;
}

void capture(char *filename)
{
    wchar_t ws[100];
    swprintf(ws, 100, L"%hs", filename);

    int nRet = is_FreezeVideo(hCam, IS_WAIT);
    printf("Status is_FreezeVideo %d\n", nRet);

    IMAGE_FILE_PARAMS ImageFileParams;
    ImageFileParams.pwchFileName = &ws[0];
    ImageFileParams.pnImageID = NULL;
    ImageFileParams.ppcImageMem = NULL;
    ImageFileParams.nQuality = 0;
    ImageFileParams.nFileType = IS_IMG_PNG;
    nRet = is_ImageFile(hCam, IS_IMAGE_FILE_CMD_SAVE, (void *)&ImageFileParams, sizeof(ImageFileParams));
    printf("Status is_ImageFile %d\n", nRet);
}

void close_camera()
{
//    int nRet = is_FreeImageMem(hCam, pMem, memID);
//    printf("Status is_FreeImageMem %d\n", nRet);
//
//    is_ExitCamera(hCam);
}


