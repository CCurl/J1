// J1 white paper is here: https://excamera.com/files/j1.pdf

#include <stdio.h>

#define STK_SZ 16
#define MEM_SZ 8192

#define CELL_SZ 2
#define WORD unsigned short
#define CELL WORD

#define bool int
#define true 1
#define false 0

#define bitIncRSP (0x0001)
#define bitDecRSP (0x0002)
#define bitIncDSP (0x0004)
#define bitDecDSP (0x0008)
#define bitUnused (0x0010)
#define bitStore  (0x0020)
#define bitTtoR   (0x0040)
#define bitTtoN   (0x0080)
#define bitRtoPC  (0x1000)

#define opJMP     (0x0000)
#define opJMPZ    (0x2000)
#define opCALL    (0x4000)
#define opALU     (0x6000)
#define opLIT     (0x8000)

#define bitTgetsN (0x01 <<  8)

#define emitPort 1

#define MAKE_LIT(val) (0x8000 | val)
#define MAKE_JMP(to)  (0x0000 | to)
#define MAKE_JMPZ(to) (0x2000 | to)
#define MAKE_CALL(to) (0x4000 | to)
#define MAKE_ALU(val) (0x6000 | val)

#define T dstk[DSP]
#define N dstk[(DSP > 0) ? (DSP-1) : 0]
#define R rstk[RSP]

#define COMMA(val) the_memory[HERE++] = val

extern WORD the_memory[];

extern CELL dstk[STK_SZ+1];
extern int DSP;

extern CELL rstk[];
extern int RSP;

extern CELL PC;
extern CELL newT;

extern const WORD memory_size;
extern int running;
extern CELL HERE;

// ---------------------------------------------------------------------
void j1_init();
void setNewT(WORD OP);
void executeALU(WORD IR);
void j1_emu(CELL start);
void dumpStack(int sp, WORD *stk);
// ---------------------------------------------------------------------

