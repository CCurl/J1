// J1 white paper is here: https://excamera.com/files/j1.pdf


#define STK_SZ 16
#define MEM_SZ 8192

#define CELL_SZ 2
#define WORD unsigned short
#define CELL WORD
#define byte unsigned char  

#define bool int
#define true 1
#define false 0

#define INSTR_MASK 0xE000
#define ADDR_MASK  0x1FFF

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

#define aluTgetsT (0x0000)
#define aluTgetsN (0x0100)
#define aluTplusN (0x0200)
#define aluTandN  (0x0300)
#define aluTorN   (0x0400)
#define aluTxorN  (0x0500)
#define aluNotT   (0x0600)
#define aluTeqN   (0x0700)
#define aluTltN   (0x0800)
#define aluSHR    (0x0900)
#define aluDecT   (0x0A00)
#define aluTgetsR (0x0B00)
#define alu12     (0x0C00)
#define alu13     (0x0D00)
#define alu14     (0x0E00)
#define alu15     (0x0F00)

#define setALUcode(op, code) (op |= ((code & 0x0f) << 8))

#define emitPort 1
#define dotPort 2

#define MAKE_LIT(val) (0x8000 | val)
#define MAKE_JMP(to)  (0x0000 | to)
#define MAKE_JMPZ(to) (0x2000 | to)
#define MAKE_CALL(to) (0x4000 | to)
#define MAKE_ALU(val) (0x6000 | val)

#define T dstk[DSP]
#define N dstk[(DSP > 0) ? (DSP-1) : 0]
#define R rstk[RSP]

#define COMMA(val) the_memory[HERE++] = val

typedef struct {
	char name[24];
	byte flags;
	byte len;
	WORD xt;
} DICT_T;

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
void j1_emu(CELL start, int maxCycles);
void dumpStack(int sp, WORD *stk);
void disIR(WORD IR, char *output);
// ---------------------------------------------------------------------

