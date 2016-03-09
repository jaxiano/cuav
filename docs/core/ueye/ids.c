#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <time.h>
#include <ueye.h>
#include <ueye_deprecated.h>

HIDS hCam = 1;
time_t start, end;

void startOp()
{
	start = time(0);
}

double uptime()
{
	end = time(0);
	return (end - start);
}

void main()
{
	printf("Success-Code: %d\n", IS_SUCCESS);

	startOp();
	INT num_cam;
	is_GetNumberOfCameras(&num_cam);
	printf("# Cams Detected: %d\n", num_cam);
	printf("elapsed: %f\n", uptime());

	startOp();
	INT nRet = is_InitCamera(&hCam, NULL);
	printf("InitCamera\n");
	printf("elapsed: %f\n", uptime());

	if (nRet == IS_SUCCESS) {
            	nRet = is_ParameterSet( hCam, IS_PARAMETERSET_CMD_LOAD_EEPROM, NULL, 0 );
		printf("Status loading parameter %d\n", nRet);

/*		UINT nPixelClockDefault = 9;
		nRet = is_PixelClock(hCam, IS_PIXELCLOCK_CMD_SET, (void *)&nPixelClockDefault, sizeof(nPixelClockDefault));
		printf("Status is_pixelClock %d\n", nRet); */

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

/*		INT colorMode = IS_CM_BGR8_PACKED;
		nRet = is_SetColorMode(hCam, colorMode);
		printf("Status SetColorMode %d\n", nRet); */

/*		UINT formatID = 4;
		nRet = is_ImageFormat(hCam, IMGFRMT_CMD_SET_FORMAT, &formatID, 4);
		printf("Status ImageFormat %d\n", nRet); */

		char *pMem = NULL;
		int memID = 0;
		nRet = is_AllocImageMem(hCam, imageData.s32Width, imageData.s32Height, nBitsPerPixel, &pMem, &memID);
		printf("Status AllocImage %d\n", nRet);

/*	INT displayMode = IS_SET_DM_DIB;
	nRet = is_SetDisplayMode(hCam, displayMode);
	printf("Status displayMode %d\n", nRet); */

		nRet = is_SetImageMem(hCam, pMem, memID);
		printf("Status is_SetImageMem %d\n", nRet);

		nRet = is_SetExternalTrigger(hCam, IS_SET_TRIGGER_SOFTWARE);
		printf("Status is_SetExternalTrigger %d\n", nRet);

	size_t turns = 0;
	do{
		startOp();
		nRet = is_FreezeVideo(hCam, IS_WAIT);
		printf("Status is_FreezeVideo %d\n", nRet);
		printf("elapsed: %f\n", uptime());

		//wchar_t filename[100];
		//swprintf(filename,100, L"./image%d.png", turns);
		//wprintf(L"%s\n", filename);

		startOp();
		IMAGE_FILE_PARAMS ImageFileParams;
		//wcscpy(ImageFileParams.pwchFileName, L"image.png");
		wchar_t ws[100];
		char *filename = "./image.png";
		swprintf(ws, 100, L"%hs", filename);
		
		ImageFileParams.pwchFileName = &ws[0];
		ImageFileParams.pnImageID = NULL;
		ImageFileParams.ppcImageMem = NULL;
		ImageFileParams.nQuality = 50;
		ImageFileParams.nFileType = IS_IMG_PNG;
		nRet = is_ImageFile(hCam, IS_IMAGE_FILE_CMD_SAVE, (void *)&ImageFileParams, sizeof(ImageFileParams));
		printf("Status is_ImageFile %d\n", nRet);
		printf("elapsed: %f\n", uptime());

		sleep(1);
		turns++;
	}while(turns < 1);

		nRet = is_FreeImageMem(hCam, pMem, memID);
		printf("Status is_FreeImageMem %d\n", nRet);

		is_ExitCamera(hCam);
	}
	else{
		printf("Initialization failed with status: %d\n", nRet);
	}

}
