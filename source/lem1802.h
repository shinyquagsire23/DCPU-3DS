#include <3ds.h>
#include "font.h"

void LEM_DrawScreen(void);
void DrawTextDCPU(gfxScreen_t screen, gfx3dSide_t side, char* str, s16 x, s16 y);
void drawStringDCPU(u8* fb, char* str, s16 x, s16 y, u16 w, u16 h);
int drawCharacterDCPU(u8* fb, char c, s16 x, s16 y, u16 w, u16 h);
