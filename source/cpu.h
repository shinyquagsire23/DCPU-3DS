#include <3ds.h>

void DCPU_Tick(void);
void DCPU_HandleInterrupts(void);
void DCPU_GetAB(u16 opcode);
int DCPU_NextInstrLength(void);
void DCPU_HandleExtendedOpcode(void);

extern u16 REG_A;
extern u16 REG_B;
extern u16 REG_C;
extern u16 REG_X;
extern u16 REG_Y;
extern u16 REG_Z;
extern u16 REG_I;
extern u16 REG_J;
extern u16 REG_PC;
extern u16 REG_SP;
extern u16 REG_EX;
extern u16 REG_IA;
extern u32 REG_DBG1;
extern u32 REG_DBG2;

extern u16 DCPU_Mem[0x10000];
extern u16 *DCPU_A;
extern u16 *DCPU_B;
extern u16 DCPU_INSTRUCTION;
extern u8 DCPU_SCREEN_UPDATE;
extern char DCPU_Regs_Names[];
extern u16* DCPU_Screen_Mem;
extern u8* DCPU_Font_Mem;
extern u16* DCPU_Palette_Mem;
extern u16 LAST_PC;
extern u8 DCPU_SCREEN_ON;
extern float DCPU_FREQ;
extern u8 INT_ENABLED;
extern u8 CLOCK_ENABLED;
extern u8 CLOCK_INT_ENABLED;
extern u16 CLOCK_TICKS;
extern float DCPU_CLOCK_TICKS_PER_SECOND;

#define TICKS_PER_MSEC 268123.480

#define NEXT_INSTRUCTION DCPU_Mem[REG_PC++]
#define NEXT_OPCODE DCPU_INSTRUCTION & 0x1F
#define NEXT_SPECIAL_OPCODE (DCPU_INSTRUCTION & 0x3E0) >> 5
#define NEXT_OPCODE_A (DCPU_INSTRUCTION & 0xFC00) >> 10
#define NEXT_OPCODE_B (DCPU_INSTRUCTION & 0x3E0) >> 5
#define NEXT_WORD DCPU_Mem[REG_PC++]

#define NEXT_INSTRUCTION_SOFT DCPU_Mem[REG_PC + extCount]
#define NEXT_OPCODE_SOFT DCPU_INSTRUCTION_SOFT & 0x1F
#define NEXT_OPCODE_A_SOFT (DCPU_INSTRUCTION_SOFT & 0xFC00) >> 10
#define NEXT_OPCODE_B_SOFT (DCPU_INSTRUCTION_SOFT & 0x3E0) >> 5

#define NUMBER_OF_CONNECTED_DEVICES 3

// Instructions (within 6 bit range, and not 0)
#define SPECIAL 0x0
#define SET 0x1
#define ADD 0x2
#define SUB 0x3
#define MUL 0x4
#define MLI 0x5
#define DIV 0x6
#define DVI 0x7
#define MOD 0x8
#define MDI 0x9
#define AND 0xA
#define BOR 0xB
#define XOR 0xC
#define SHR 0xD
#define ASR 0xE
#define SHL 0xF
#define IFB 0x10
#define IFC 0x11
#define IFE 0x12
#define IFN 0x13
#define IFG 0x14
#define IFA 0x15
#define IFL 0x16
#define IFU 0x17
#define ADX 0x1A
#define SBX 0x1B
#define STI 0x1E
#define STD 0x1F
