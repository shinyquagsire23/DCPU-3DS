#include "cpu.h"
#include "font_bin.h"

u16 REG_A = 0;
u16 REG_B = 0;
u16 REG_C = 0;
u16 REG_X = 0;
u16 REG_Y = 0;
u16 REG_Z = 0;
u16 REG_I = 0;
u16 REG_J = 0;
u16 REG_PC = 0;
u16 REG_SP = 0xFFFF;
u16 REG_EX = 0;
u16 REG_IA = 0;
u32 REG_DBG1 = 0;
u32 REG_DBG2 = 0;
u8 INT_ENABLED = 0;
u8 CLOCK_ENABLED = 0;
u8 CLOCK_INT_ENABLED = 0;
u16 CLOCK_TICKS = 0;
float DCPU_CLOCK_TICKS_PER_SECOND = 0;

u16 DCPU_Mem[0x10000];
u16* DCPU_Screen_Mem;
u8* DCPU_Font_Mem;
u8 DCPU_SCREEN_ON = 0;
char DCPU_Regs_Names[] = "ABCXYZIJ";

u16 DCPU_A_STORE;
u16 DCPU_B_STORE;

u16 *DCPU_A;
u16 *DCPU_B;
float DCPU_FREQ = 0;

#define DCPU_A_SIG ((s16)(*DCPU_A))
#define DCPU_B_SIG ((s16)(*DCPU_B))

u16 DCPU_INSTRUCTION = 0;
u16 DCPU_INSTRUCTION_SOFT = 0;
u16 REG_EX_TMP = 0;
u8 DCPU_SCREEN_UPDATE = 0; //Update screen once on boot
u16 LAST_PC = 0;

void DCPU_Tick(void)
{
	LAST_PC = REG_PC;
	DCPU_INSTRUCTION = NEXT_INSTRUCTION;
	DCPU_GetAB(NEXT_OPCODE);
	REG_DBG1 = DCPU_INSTRUCTION;
	REG_DBG2 = NEXT_OPCODE;
	//DCPU_FREQ = DCPU_CLOCK_TICKS_PER_SECOND;

	if(DCPU_SCREEN_ON)
	{
		//If we are writing to data within the DCPU's screen memory, schedule a redraw
		REG_DBG1 = DCPU_B;
		REG_DBG2 = &DCPU_Screen_Mem[0];
		//if(DCPU_B >= &DCPU_Screen_Mem[0] && DCPU_B <= &DCPU_Screen_Mem[32*12])
			//DCPU_SCREEN_UPDATE = 1;
	}

	switch(NEXT_OPCODE)
	{
		case 0x0:
			DCPU_HandleExtendedOpcode();
		break;
		case 0x1: //SET
			*DCPU_B = *DCPU_A;
		break;
		case 0x2: //ADD
			if(*DCPU_B + *DCPU_A > 0xFFFF)
				REG_EX = 1;
			*DCPU_B += *DCPU_A;
		break;

		case 0x3: //SUB
			if(*DCPU_B - *DCPU_A < 0)
				REG_EX = 0xFFFF;
			*DCPU_B -= *DCPU_A;
		break;

		case 0x4: //MUL
			REG_EX = (((*DCPU_B) * (*DCPU_A))>>16)&0xFFFF;
			*DCPU_B *= *DCPU_A;
		break;

		case 0x5: //MLI
			REG_EX = (((DCPU_B_SIG) * (DCPU_A_SIG))>>16)&0xFFFF;
			*DCPU_B = DCPU_B_SIG * DCPU_A_SIG;
		break;

		case 0x6: //DIV
			REG_EX = ((*DCPU_B << 16) / (*DCPU_A))&0xFFFF;
			if(*DCPU_A != 0)
				*DCPU_B /= *DCPU_A;
			else
			{
				*DCPU_B = 0;
				REG_EX = 0;
			}
		break;

		case 0x7: //DVI
			REG_EX = ((DCPU_B_SIG << 16) / (DCPU_A_SIG))&0xFFFF;
			if(*DCPU_A != 0)
				*DCPU_B = DCPU_B_SIG / DCPU_A_SIG;
			else
			{
				*DCPU_B = 0;
				REG_EX = 0;
			}
		break;
		case 0x8: //MOD
			if(*DCPU_A != 0)
				*DCPU_B %= *DCPU_A;
			else
			{
				*DCPU_B = 0;
				REG_EX = 0;
			}
		break;

		case 0x9: //MDI
			if(*DCPU_A != 0)
				*DCPU_B = DCPU_B_SIG % DCPU_A_SIG;
			else
			{
				*DCPU_B = 0;
				REG_EX = 0;
			}
		break;

		case 0xA: //AND
			*DCPU_B &= *DCPU_A;
		break;

		case 0xB: //BOR
			*DCPU_B |= *DCPU_A;
		break;

		case 0xC: //XOR
			*DCPU_B ^= *DCPU_A;
		break;

		case 0xD: //SHR
			REG_EX =  ((*DCPU_B << 16) >> *DCPU_A) & 0xffff;
			*DCPU_B >>= *DCPU_A;
		break;

		case 0xE: //ASR
			REG_EX =  ((DCPU_B_SIG << 16) >> *DCPU_A) & 0xffff;
			*DCPU_B = DCPU_B_SIG >> *DCPU_A;
		break;

		case 0xF: //SHL
			*DCPU_B <<= *DCPU_A;
		break;

		//TODO: Chained if's
		case 0x10: //IFB
			if(!(*DCPU_B & *DCPU_A != 0))
				REG_PC += DCPU_NextInstrLength(); 
		break;

		case 0x11: //IFC
			if(!(*DCPU_B & *DCPU_A) == 0)
				REG_PC += DCPU_NextInstrLength(); 
		break;

		case 0x12: //IFE
			if(!(*DCPU_B == *DCPU_A))
				REG_PC += DCPU_NextInstrLength(); 
		break;

		case 0x13: //IFN
			if(!(*DCPU_B != *DCPU_A))
				REG_PC += DCPU_NextInstrLength(); 
		break;

		case 0x14: //IFG
			if(!(*DCPU_B > *DCPU_A))
				REG_PC += DCPU_NextInstrLength(); 
		break;

		case 0x15: //IFA
			if(!(DCPU_B_SIG > DCPU_A_SIG))
				REG_PC += DCPU_NextInstrLength(); 
		break;

		case 0x16: //IFL
			if(!(*DCPU_B < *DCPU_A))
				REG_PC += DCPU_NextInstrLength(); 
		break;

		case 0x17: //IFU
			if(!(DCPU_B_SIG < DCPU_A_SIG))
				REG_PC += DCPU_NextInstrLength(); 
		break;

		case 0x1A: //ADX
			REG_EX_TMP = REG_EX;
			if(*DCPU_B + *DCPU_A + REG_EX > 0xFFFF)
				REG_EX = 1;
			*DCPU_B += *DCPU_A + REG_EX_TMP;
		break;

		case 0x1B: //SBX
			REG_EX_TMP = REG_EX;
			if(*DCPU_B - *DCPU_A + REG_EX < 0)
				REG_EX = 0xFFFF;
			*DCPU_B = *DCPU_B - *DCPU_A + REG_EX_TMP;
		break;

		case 0x1E: //STI
			*DCPU_B = *DCPU_A;
			REG_I++;
			REG_J++;
		break;

		case 0x1F: //STD
			*DCPU_B = *DCPU_A;
			REG_I--;
			REG_J--;
		break;
	}
}

void DCPU_HandleInterrupts(void)
{
	if(INT_ENABLED == 0)
		return;

	DCPU_Mem[--REG_SP] = REG_PC;
	DCPU_Mem[--REG_SP] = REG_A;
	REG_PC = REG_IA;
	while(INT_ENABLED)
		DCPU_Tick();

	REG_A = DCPU_Mem[REG_SP++];
	REG_PC = DCPU_Mem[REG_SP++];
}

void DCPU_HandleExtendedOpcode()
{
	int id = 0;
	int version = 0;
	int manufacturer = 0;

	switch(NEXT_SPECIAL_OPCODE)
	{
		case 0x1: //JSR
			DCPU_Mem[--REG_SP] = REG_PC;
			REG_PC = *DCPU_A;
		break;

		//TODO: Interrupts (da fak?)
		case 0x9: //IAG
			*DCPU_A = REG_IA;
		break;
		case 0xA: //IAS
			REG_IA = *DCPU_A;
		break;
		case 0xB: //RFI
			INT_ENABLED = 0;
		break;

		case 0x10: //HWN
			*DCPU_A = NUMBER_OF_CONNECTED_DEVICES;
		break;
		case 0x11: //HWQ
			switch(*DCPU_A)
			{
				case 0x0: //LEM1802
					id = 0x7349f615;
					version = 0x1802;
					manufacturer = 0x1c6c8b36; //NYA_ELEKTRISKA
				break;
							
				case 0x1: //Keyboard (3DS_CONTROLLER)
					id = 0x30cf7406;
					version = 0x1;
					manufacturer = 0x1A34B335; //NINTENDO_INC
				break;

				case 0x2: //Clock (3DS_CLOCK)
					id = 0x12d0b402;
					version = 0x1;
					manufacturer = 0x1A34B335; //NINTENDO_INC
				break;	


				case 0x3: //Speaker
					id = 0x02060001;
					version = 0x1;
					manufacturer = 0x5672746B; //VARTOK_HW
				break;						
			}
					
			REG_A = id & 0xFFFF;
			REG_B = (id & 0xFFFF0000)>>16;
			REG_C = version;
			REG_X = manufacturer & 0xFFFF;
			REG_Y = (manufacturer & 0xFFFF0000)>>16;
		break;
		case 0x12: //HWI
			switch(*DCPU_A)
			{
				case 0x0: //LEM1802
					switch(REG_A)
					{
						case 0: //MEM_MAP_SCREEN
							if(REG_B > 0)
							{
								DCPU_Screen_Mem = &DCPU_Mem[REG_B];
								DCPU_SCREEN_UPDATE = 1;
								DCPU_SCREEN_ON = 1;
							}
						break;	
						case 1: //MEM_MAP_FONT
							if(*DCPU_B > 0)
							{
								DCPU_Font_Mem = &DCPU_Mem[REG_B];
								DCPU_SCREEN_UPDATE = 1;
							}
						break;
						case 4: //MEM_DUMP_FONT
							memcpy(&DCPU_Mem[REG_B], &font_bin, 0x200);
						break;
						//TODO: Mem map and dump font and pal		
					}
				break;
								
				case 0x1: //Keyboard (3DS_CONTROLLER)
							
				break;
						
				case 0x2: //Clock (3DS_CLOCK)
					switch(REG_A)
					{
						case 0:
							CLOCK_TICKS = 0;
							if(REG_B > 0)
							{
								DCPU_CLOCK_TICKS_PER_SECOND = 60/REG_B;
								CLOCK_ENABLED = 1;
							}
							else
								CLOCK_ENABLED = 0;
						break;	
						case 1:
							REG_C = CLOCK_TICKS;
						break;
						case 2:
							CLOCK_INT_ENABLED = REG_B & 1;
						break;	
					}	
				break;	


				case 0x3: //Speaker
				//TODO
				break;	
			}
		break;
	}
}

//LUT for which A/B values reference NEXT_WORD (increasing REG_PC by 1)
//Used for IF commands to find instruction length to skip
int abLengths[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

int DCPU_NextInstrLength(void)
{
	int extCount = 0;
	int length = 0;
	DCPU_INSTRUCTION_SOFT = NEXT_INSTRUCTION_SOFT;

	u16 next = NEXT_OPCODE_SOFT;
	//Chained IFs
	if(next >= IFB && next <= IFU)
	{
		length += abLengths[NEXT_OPCODE_A_SOFT] + abLengths[NEXT_OPCODE_B_SOFT] + 1;
		extCount += length;
	}

	DCPU_INSTRUCTION_SOFT = NEXT_INSTRUCTION_SOFT;
	length += abLengths[NEXT_OPCODE_A_SOFT] + abLengths[NEXT_OPCODE_B_SOFT] + 1;
	return length;
}

void DCPU_GetAB(u16 opcode)
{
	switch(NEXT_OPCODE_A)
	{
		// REG
		case 0x0:
			DCPU_A = &REG_A;
		break;
		case 0x1:
			DCPU_A = &REG_B;
		break;
		case 0x2:
			DCPU_A = &REG_C;
		break;
		case 0x3:
			DCPU_A = &REG_X;
		break;
		case 0x4:
			DCPU_A = &REG_Y;
		break;
		case 0x5:
			DCPU_A = &REG_Z;
		break;
		case 0x6:
			DCPU_A = &REG_I;
		break;
		case 0x7:
			DCPU_A = &REG_J;
		break;

		// [REG]
		case 0x8:
			DCPU_A = &DCPU_Mem[REG_A];
		break;
		case 0x9:
			DCPU_A = &DCPU_Mem[REG_B];
		break;
		case 0xA:
			DCPU_A = &DCPU_Mem[REG_C];
		break;
		case 0xB:
			DCPU_A = &DCPU_Mem[REG_X];
		break;
		case 0xC:
			DCPU_A = &DCPU_Mem[REG_Y];
		break;
		case 0xD:
			DCPU_A = &DCPU_Mem[REG_Z];
		break;
		case 0xE:
			DCPU_A = &DCPU_Mem[REG_I];
		break;
		case 0xF:
			DCPU_A = &DCPU_Mem[REG_J];
		break;

		// [REG + Next Word] 
		case 0x10:
			DCPU_A = &DCPU_Mem[REG_A + NEXT_WORD];
		break;
		case 0x11:
			DCPU_A = &DCPU_Mem[REG_B + NEXT_WORD];
		break;
		case 0x12:
			DCPU_A = &DCPU_Mem[REG_C + NEXT_WORD];
		break;
		case 0x13:
			DCPU_A = &DCPU_Mem[REG_X + NEXT_WORD];
		break;
		case 0x14:
			DCPU_A = &DCPU_Mem[REG_Y + NEXT_WORD];
		break;
		case 0x15:
			DCPU_A = &DCPU_Mem[REG_Z + NEXT_WORD];
		break;
		case 0x16:
			DCPU_A = &DCPU_Mem[REG_I + NEXT_WORD];
		break;
		case 0x17:
			DCPU_A = &DCPU_Mem[REG_J + NEXT_WORD];
		break;

		/*case 0x18:
			DCPU_A = &DCPU_Mem[--REG_SP]; //PUSH
		break;*/
		case 0x18:
			DCPU_A = &DCPU_Mem[REG_SP++]; //POP
		break;

		case 0x19:
			DCPU_A = &DCPU_Mem[REG_SP]; //[SP]
		break;
		case 0x1A:
			DCPU_A = &DCPU_Mem[REG_SP + NEXT_WORD]; //[SP + NEXT_WORD]
		break;

		case 0x1B:
			DCPU_A = &REG_SP; //SP
		break;
		case 0x1C:
			DCPU_A = &REG_PC; //PC
		break;
		case 0x1D:
			DCPU_A = &REG_EX; //EX
		break;

		case 0x1E:
			DCPU_A = &DCPU_Mem[NEXT_WORD]; //[NEXT_WORD]
		break;
		case 0x1F:
			DCPU_A = &NEXT_WORD; //NEXT_WORD
		break;

		//Literal value of A as a signed 16-bit integer (minus 1). Has a range from -1 to 30.
		default:
			DCPU_A_STORE = (NEXT_OPCODE_A & 0x1F)-1;
			DCPU_A = &DCPU_A_STORE;
		break;
	}

	if(opcode == 0)
		return;

	switch(NEXT_OPCODE_B)
	{
		// REG
		case 0x0:
			DCPU_B = &REG_A;
		break;
		case 0x1:
			DCPU_B = &REG_B;
		break;
		case 0x2:
			DCPU_B = &REG_C;
		break;
		case 0x3:
			DCPU_B = &REG_X;
		break;
		case 0x4:
			DCPU_B = &REG_Y;
		break;
		case 0x5:
			DCPU_B = &REG_Z;
		break;
		case 0x6:
			DCPU_B = &REG_I;
		break;
		case 0x7:
			DCPU_B = &REG_J;
		break;

		// [REG]
		case 0x8:
			DCPU_B = &DCPU_Mem[REG_A];
		break;
		case 0x9:
			DCPU_B = &DCPU_Mem[REG_B];
		break;
		case 0xA:
			DCPU_B = &DCPU_Mem[REG_C];
		break;
		case 0xB:
			DCPU_B = &DCPU_Mem[REG_X];
		break;
		case 0xC:
			DCPU_B = &DCPU_Mem[REG_Y];
		break;
		case 0xD:
			DCPU_B = &DCPU_Mem[REG_Z];
		break;
		case 0xE:
			DCPU_B = &DCPU_Mem[REG_I];
		break;
		case 0xF:
			DCPU_B = &DCPU_Mem[REG_J];
		break;

		// [REG + Next Word] 
		case 0x10:
			DCPU_B = &DCPU_Mem[REG_A + NEXT_WORD];
		break;
		case 0x11:
			DCPU_B = &DCPU_Mem[REG_B + NEXT_WORD];
		break;
		case 0x12:
			DCPU_B = &DCPU_Mem[REG_C + NEXT_WORD];
		break;
		case 0x13:
			DCPU_B = &DCPU_Mem[REG_X + NEXT_WORD];
		break;
		case 0x14:
			DCPU_B = &DCPU_Mem[REG_Y + NEXT_WORD];
		break;
		case 0x15:
			DCPU_B = &DCPU_Mem[REG_Z + NEXT_WORD];
		break;
		case 0x16:
			DCPU_B = &DCPU_Mem[REG_I + NEXT_WORD];
		break;
		case 0x17:
			DCPU_B = &DCPU_Mem[REG_J + NEXT_WORD];
		break;

		case 0x18:
			DCPU_B = &DCPU_Mem[--REG_SP]; //PUSH
		break;
		/*case 0x18:
			DCPU_B = &DCPU_Mem[REG_SP++]; //POP
		break;*/

		case 0x19:
			DCPU_B = &DCPU_Mem[REG_SP]; //[SP]
		break;
		case 0x1A:
			DCPU_B = &DCPU_Mem[REG_SP + NEXT_WORD]; //[SP + NEXT_WORD]
		break;

		case 0x1B:
			DCPU_B = &REG_SP; //SP
		break;
		case 0x1C:
			DCPU_B = &REG_PC; //PC
		break;
		case 0x1D:
			DCPU_B = &REG_EX; //EX
		break;

		case 0x1E:
			DCPU_B = &DCPU_Mem[NEXT_WORD]; //[NEXT_WORD]
		break;
		case 0x1F:
			DCPU_B = &NEXT_WORD; //NEXT_WORD
		break;

		/*default:
			DCPU_B_STORE = ((s16)(NEXT_OPCODE_B)-1);
			DCPU_B = &DCPU_B_STORE;
		break;*/
	}
}
