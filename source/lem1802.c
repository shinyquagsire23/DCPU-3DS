#include "lem1802.h"
#include "cpu.h"
#include "draw.h"
#include "font.h"
#include "font_bin.h"
#include "gpspFont_bin.h"

void LEM_DrawScreen(void)
{
	int screenX = (400/2) - (128/2);
	int screenY = (240/2) - (96/2);
	int i = 0;
	int j = 0;
	int dx = screenX;
	int dy = TOP_HEIGHT-screenY-8;
	u16 fbWidth, fbHeight;
	u8* fb=gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &fbWidth, &fbHeight);
	drawFillRect(screenX, screenY, screenX+128, screenY+96, 0, 0, 0, gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL));
	for(i = 0; i < 12; i++)
	{
		for(j = 0; j < 32; j++)
		{
			u8 ch = (*(u8*)&DCPU_Screen_Mem[(i*32)+j]) & 0x7F;
			if(ch != 0)
				drawCharacterDCPU(fb,ch,dx,dy,fbHeight,fbWidth);
			dx += 4;
		}
		dy -= 8;
		dx = screenX;
	}

	
	//DrawTextDCPU(GFX_TOP, GFX_LEFT, str, TOP_HEIGHT-screenY-8, screenX);
}

void DrawTextDCPU(gfxScreen_t screen, gfx3dSide_t side, char* str, s16 x, s16 y)
{
	if(!str)return;

	u16 fbWidth, fbHeight;
	u8* fbAdr=gfxGetFramebuffer(screen, side, &fbWidth, &fbHeight);

	drawStringDCPU(fbAdr, str, y, x, fbHeight, fbWidth);
}

void drawStringDCPU(u8* fb, char* str, s16 x, s16 y, u16 w, u16 h)
{
	if(!fb || !str)return;
	int k; int dx=0, dy=0;
	int length=strlen(str);
	for(k=0;k<length;k++)
	{
		dx+=drawCharacterDCPU(fb,str[k],x+dx,y+dy,w,h);
		if(str[k]=='\n'){dx=0;dy-=10;}
	}
}

int drawCharacterDCPU(u8* fb, char c, s16 x, s16 y, u16 w, u16 h)
{
	u8* fb_orig = fb;

	if(x<0 || x+6>=w || y<-10 || y>=h+10)return 0;

	u8* charData=&DCPU_Font_Mem[(c*2*2)];

	int i, j;

	s16 cy=y, ch=16, cyo=0;

	if(y<0)
	{
		cy=0;cyo=-y;
		ch=8-cyo;
	}
	else if(y+ch>h)
		ch=h-y;

	fb+=(x*h+cy)*3;

	const u8 r=0xFF, g=0xFF, b=0xFF;
     /*for(i = 7; i >=0; i--)
	{
		for(j = 9; j >= 0; j--)
		{
			u8 v=(charData[j*2] & BIT(i));
			if(v)
			{
				fb[0]=b;
				fb[1]=g;
				fb[2]=r;
			}
			else //Copy color from 0,0
			{
				fb[0]=80;//fb_orig[0];
				fb[1]=32;//fb_orig[0];
				fb[2]=16;//fb_orig[0];
			}
			fb+=3;
			
		}
		fb+=(h*3)-(10*3);
	}*/
	u8 order[] = { 2, 3, 0, 1 };
	for(j = 3; j >= 0; j--)
	{
		for(i = 7; i >= 0; i--)
		{
			u8 n = (order[j]);
			u8 v = (charData[n] & BIT(i));
			if(v)
			{
				fb[0]=b;
				fb[1]=g;
				fb[2]=r;
			}
			fb += 3;
		}
		fb+=(h*3)-(8*3);
	}
	return 4;
}
