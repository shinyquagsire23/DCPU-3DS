#include "main.h"

#include <3ds.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "draw.h"
#include "mem.h"
#include "font.h"
#include "cpu.h"
#include "lem1802.h"
#include "font_bin.h"

u8* fileBuffer;

int main(int argc, char** argv)
{
	// Initialize services
	srvInit();			// mandatory
	aptInit();			// mandatory
	hidInit(NULL);	// input (buttons, screen)
	gfxInit();			// graphics
	fsInit();
	sdmcArchive = (FS_archive){0x9, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FSUSER_OpenArchive(NULL, &sdmcArchive);

	gfxSet3D(true);
	loadROM();

	gspWaitForVBlank(); //wait to let the app register itself

	screenTopLeft = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL); 
	screenBottom = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL); 
	clearScreen(screenTopLeft, GFX_TOP,16,32,80); 
	clearScreen(screenBottom, GFX_BOTTOM,16,32,80);
	gfxFlushBuffers();
	gfxSwapBuffers();
	screenTopLeft = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL); 
	screenBottom = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL); 
	clearScreen(screenTopLeft, GFX_TOP,16,32,80); 
	clearScreen(screenBottom, GFX_BOTTOM,16,32,80);

	// Main loop
	u64 lastTick = osGetTime();
	u64 lastTickFps = osGetTime();
	float millis = 0.0f;
	float millisFps = 0.0f;
	int targetFps = 15;
	int cycles = 0;
	int drawAlways = 0;
	int step = 1;
	DCPU_Tick();
	DCPU_Font_Mem = font_bin;
	while (aptMainLoop())
	{
		millis = osGetTime() - lastTick;
		millisFps = osGetTime() - lastTickFps;
		/*if(millis >= 1000)
		{
			
			DCPU_FREQ = (cycles / (millis/1000))/1000; //Get frequency in kHZ
			lastTick = osGetTime();
			cycles = 0;
			DCPU_SCREEN_UPDATE = 1;
		}*/

		if(millis >= 1000 / DCPU_CLOCK_TICKS_PER_SECOND && CLOCK_ENABLED)
		{
			if(CLOCK_INT_ENABLED)
				INT_ENABLED = 1;
			CLOCK_TICKS++;
			DCPU_FREQ = (cycles / (millis/1000))/1000; //Get frequency in kHZ
			lastTick = osGetTime();
			cycles = 0;
			//DCPU_SCREEN_UPDATE = 1;
		}

		if(millisFps >= 1000 / targetFps)
		{
			// Read which buttons are currently pressed 
			hidScanInput();
			kDown = hidKeysDown();
			kHeld = hidKeysHeld();

			DCPU_SCREEN_UPDATE = 1;
			lastTickFps = osGetTime();
		}

		if (kDown & KEY_X | step)
		{
			DCPU_Tick();
			DCPU_HandleInterrupts();
		}
		cycles++;
		
		// If START is pressed, break loop and quit
		if (kHeld & KEY_START){
			break;
		}

		if (kDown & KEY_A)
			drawAlways = !drawAlways;

		if (kDown & KEY_Y)
			step = !step;

		if(DCPU_SCREEN_UPDATE | drawAlways | !step)
		{
			gspWaitForVBlank();
			drawStuff();	
			//For now we update on every frame, but do partial draws to make it fast(ish)
			DCPU_SCREEN_UPDATE = 0;
		}
	}

	// Exit services
	fsExit();
	gfxExit();
	hidExit();
	aptExit();
	srvExit();
	
	// Return to hbmenu
	return 0;
}

void drawStuff(void)
{
	//Init screen buffers
	screenTopLeft = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL); 
	screenTopRight = gfxGetFramebuffer(GFX_TOP, GFX_RIGHT, NULL, NULL);
	screenBottom = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL); 
	/* Clear Screen */
	//clearScreen(screenBottom, GFX_BOTTOM,16,32,80);
	//clearScreen(screenTopLeft, GFX_TOP,16,32,80); 
	//clearScreen(screenTopRight, GFX_TOP);

	drawStack();
	if(DCPU_SCREEN_ON)
		LEM_DrawScreen();

	//Screenshot - taken from blargSnes
	if(kHeld & KEY_B)
	{
		u32 timestamp = (u32)(svcGetSystemTick() / 446872);
		char file[256];
		snprintf(file, 256, "/dcpu%08d.bmp", timestamp);
		first = TakeScreenshot(file);
	}

	// Flush and swap framebuffers
	gfxFlushBuffers();
	gfxSwapBuffers();
}

void drawStack()
{
	drawRegister(REG_A, 0);
	drawRegister(REG_B, 1);
	drawRegister(REG_C, 2);
	drawRegister(REG_X, 3);
	drawRegister(REG_Y, 4);
	drawRegister(REG_Z, 5);
	drawRegister(REG_I, 6);
	drawRegister(REG_J, 7);
	stackNum += 3+1;

	drawRegister(LAST_PC, 8);
	drawRegister(REG_SP, 9);
	stackNum++;

	drawRegister(REG_EX, 10);
	drawRegister(REG_IA, 11);
	stackNum++;

	drawRegisterFloat(DCPU_FREQ, 12);
	stackNum += 2;

	drawRegister(REG_DBG1, 13);
	stackNum += 2;
	drawRegister(REG_DBG2, 14);

	stackNum = 0;

	if(DCPU_SCREEN_ON)
	{
		//DCPU_Screen_Mem[0] = 'F';
		char str[256];
		memcpy(&str[0], &DCPU_Screen_Mem[0], 254);
		str[255] = 0;
		gfxDrawText(GFX_BOTTOM, GFX_LEFT, first ? &fontPurple : &fontGreen, str, 0, 0);
	}
}

void drawRegister(u32 regval, u8 regnum)
{
	char str[256];
	if(regnum < 8)
	{
		sprintf(str, " W: %04x", regval);
		str[1] = DCPU_Regs_Names[regnum];
	}
	else
	{
		switch(regnum)
		{
			case 8:
			sprintf(str, "PC: %04x", regval);
			break;
			case 9:
			sprintf(str, "SP: %04x", regval);
			break;
			case 10:
			sprintf(str, "EX: %04x",regval);
			break;
			case 11:
			sprintf(str, "IA: %04x", regval);
			break;
			case 12:
			sprintf(str, "FR: %f", regval);
			break;
			default:
			sprintf(str, "DB: %08x", regval);
			break;
			
		}
	}
	gfxDrawText(GFX_BOTTOM, GFX_LEFT, first ? &fontPurple : &fontGreen, str, BOTTOM_SCREEN_Y-3-(10*((stackNum/3)+1)), 3+(10*((6+1)*(stackNum%3))));
	stackNum++;
}

void drawRegisterFloat(float regval, u8 regnum)
{
	char str[256];
	if(regnum < 8)
	{
		sprintf(str, " W: %04x", regval);
		str[1] = DCPU_Regs_Names[regnum];
	}
	else
	{
		switch(regnum)
		{
			case 8:
			sprintf(str, "PC: %04x", regval);
			break;
			case 9:
			sprintf(str, "SP: %04x", regval);
			break;
			case 10:
			sprintf(str, "EX: %04x",regval);
			break;
			case 11:
			sprintf(str, "IA: %04x", regval);
			break;
			case 12:
			sprintf(str, "FR: %f", regval);
			break;
			default:
			sprintf(str, "DB: %08x", regval);
			break;
			
		}
	}
	gfxDrawText(GFX_BOTTOM, GFX_LEFT, first ? &fontPurple : &fontGreen, str, BOTTOM_SCREEN_Y-3-(10*((stackNum/3)+1)), 3+(10*((6+1)*(stackNum%3))));
	stackNum++;
}

void text_print(char* str)
{
	gfxDrawText(GFX_TOP, GFX_LEFT, NULL, str, 140, 150);
}

//Taken from blargSnes
bool TakeScreenshot(char* path)
{
	int x, y;
	
	Handle file;
	FS_path filePath;
	filePath.type = PATH_CHAR;
	filePath.size = strlen(path) + 1;
	filePath.data = (u8*)path;
	
	Result res = FSUSER_OpenFile(NULL, &file, sdmcArchive, filePath, FS_OPEN_CREATE|FS_OPEN_WRITE, FS_ATTRIBUTE_NONE);
	if (res) 
		return false;
		
	u32 byteswritten;
	
	u32 bitmapsize = 400*480*3;
	u8* tempbuf = (u8*)MemAlloc(0x36 + bitmapsize);
	memset(tempbuf, 0, 0x36 + bitmapsize);
	
	FSFILE_SetSize(file, (u16)(0x36 + bitmapsize));
	
	*(u16*)&tempbuf[0x0] = 0x4D42;
	*(u32*)&tempbuf[0x2] = 0x36 + bitmapsize;
	*(u32*)&tempbuf[0xA] = 0x36;
	*(u32*)&tempbuf[0xE] = 0x28;
	*(u32*)&tempbuf[0x12] = 400; // width
	*(u32*)&tempbuf[0x16] = 480; // height
	*(u32*)&tempbuf[0x1A] = 0x00180001;
	*(u32*)&tempbuf[0x22] = bitmapsize;
	
	u8* framebuf = (u8*)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
	for (y = 0; y < 240; y++)
	{
		for (x = 0; x < 400; x++)
		{
			int si = ((239 - y) + (x * 240)) * 3;
			int di = 0x36 + (x + ((479 - y) * 400)) * 3;
			
			tempbuf[di++] = framebuf[si++];
			tempbuf[di++] = framebuf[si++];
			tempbuf[di++] = framebuf[si++];
		}
	}
	
	framebuf = (u8*)gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
	for (y = 0; y < 240; y++)
	{
		for (x = 0; x < 320; x++)
		{
			int si = ((239 - y) + (x * 240)) * 3;
			int di = 0x36 + ((x+40) + ((239 - y) * 400)) * 3;
			
			tempbuf[di++] = framebuf[si++];
			tempbuf[di++] = framebuf[si++];
			tempbuf[di++] = framebuf[si++];
		}
	}
	
	FSFILE_Write(file, &byteswritten, 0, (u32*)tempbuf, 0x36 + bitmapsize, 0x10001);
	
	FSFILE_Close(file);
	MemFree(tempbuf);
	REG_B = 1;
	return true;
}

void setA(u16 val)
{
	REG_A = val;
}

void loadROM()
{
	Handle fileHandle;
	char* path = "/dcpu/derp.dcpx";
	FS_path filePath;
	filePath.type = PATH_CHAR;
	filePath.size = strlen(path) + 1;
	filePath.data = (u8*)path;

	Result ret = FSUSER_OpenFile(NULL, &fileHandle, sdmcArchive, filePath, FS_OPEN_READ, FS_ATTRIBUTE_NONE);
	if(!ret)
	{
		u64 size;
		u32 bytesRead;
		ret = FSFILE_GetSize(fileHandle, &size);
		if(ret)
			return;	

		fileBuffer = linearAlloc(size);
		if(!fileBuffer)
			return;

		ret = FSFILE_Read(fileHandle, &bytesRead, 0x0, fileBuffer, size);
		if(ret || size != bytesRead)
			return;

		ret = FSFILE_Close(fileHandle);

		// Copy all the data we need to the DCPU memory 
		// (only 0x10000 words worth, the rest of the file is ignored)
		memcpy(&DCPU_Mem[0], &fileBuffer[0], 0x10000*2);


	}
}

