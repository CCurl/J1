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
#define aluFetch  (0x0C00)
#define aluSHL    (0x0D00)
#define aluDepth  (0x0E00)
#define aluNuLtT  (0x0F00)

#define tpTgetsT (0x00)
#define tpTgetsN (0x01)
#define tpTplusN (0x02)
#define tpTandN  (0x03)
#define tpTorN   (0x04)
#define tpTxorN  (0x05)
#define tpNotT   (0x06)
#define tpTeqN   (0x07)
#define tpTltN   (0x08)
#define tpSHR    (0x09)
#define tpDecT   (0x0A)
#define tpTgetsR (0x0B)
#define tpFetch  (0x0C)
#define tpSHL    (0x0D)
#define tpDepth  (0x0E)
#define tpNuLtT  (0x0F)

#define setALUcode(op, code) (op |= ((code & 0x0f) << 8))

#define MAKE_LIT(val)   (0x8000 | val)
#define MAKE_JMP(addr)  (0x0000 | addr)
#define MAKE_JMPZ(addr) (0x2000 | addr)
#define MAKE_CALL(addr) (0x4000 | addr)
#define MAKE_ALU(val)   (0x6000 | val)

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

extern CELL dstk[];
extern int DSP;

extern CELL rstk[];
extern int RSP;

extern CELL PC;
extern CELL tPrime;

extern const WORD memory_size;
extern int running;
extern CELL HERE;

// ---------------------------------------------------------------------
void j1_init();
void executeALU(WORD IR);
void j1_emu(CELL start, long maxCycles);
void dumpState(bool lastPC);
void dumpStack(int sp, WORD *stk);
void disIR(WORD IR, char *output);
void writePort(WORD portNum, WORD val);
// ---------------------------------------------------------------------

