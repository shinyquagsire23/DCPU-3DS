#include <3ds.h>

#define TOP_SCREEN_X 400
#define TOP_SCREEN_Y 240

#define BOTTOM_SCREEN_X 320
#define BOTTOM_SCREEN_Y 240

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

void reset(void);
void drawPaddle(int x, int y, u8 ai);
void drawBall(int x, int y);
void drawStuff(void);
bool TakeScreenshot(char* path);
void lol();
void text_print();
void setA(u16 val);
void loadROM();

u8* screenTopLeft;
u8* screenTopRight;
u8* screenBottom;

FS_archive sdmcArchive;

u32 kDown;
u32 kHeld;


int screenWait = 60*2;
int first = 0;

int stackNum = 0;

int depthSeparation = 2;
#define CONFIG_3D_SLIDERSTATE (*(float*)0x1FF81080)
