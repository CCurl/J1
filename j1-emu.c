// J1 white paper is here: https://excamera.com/files/j1.pdf

#include <stdio.h>
#include <string.h>
#include "j1.h"

WORD the_memory[MEM_SZ];

CELL dstk[STK_SZ+1];
int DSP = 0;

CELL rstk[STK_SZ+1];
int RSP = 0;

CELL PC;

const WORD memory_size = MEM_SZ;
int running = 0;

// ---------------------------------------------------------------------
void j1_init()
{
	DSP = 0;
	RSP = 0;
	PC = 0;
	running = 0;
}

// ---------------------------------------------------------------------
WORD getTprime(WORD OP) {
	int op = (OP & 0x0F00) >> 8;
	switch (OP) {
		case 0: // #define tPrime_T 0x00
			return T;
		case 1: // #define tPrime_N 0x01
			return N;
		case 2: // #define tPrime_ADD 0x02
			return (T + N);
		case 3: // #define tPrime_AND 0x03
			return (T & N); // AND
		case 4: // #define tPrime_OR 0x04
			return (T | N); // OR
		case 5: // #define tPrime_XOR 0x05
			return (T ^ N); // XOR
		case 6: // #define tPrime_NOT 0x06
			return (~T); // NOT (aka - INVERT)
		case 7: // #define tPrime_EQ 0x07
			return (N == T) ? 1 : 0;
		case 8: // #define tPrime_LT 0x08
			return (N < T) ? 1 : 0;
		case 9: // #define tPrime_SHR 0x09
			return (N >> T);
		case 10: // #define tPrime_DEC 0x0A
			return (T-1);
		case 11: // #define tPrime_R 0x0B
			return R;
		case 12: // #define tPrime_FETCH 0x0C
			return the_memory[T];
		case 13: // #define tPrime_SHL 0x0D
			return (N << T);
		case 14: // #define tPrime_DEPTH 0x0E
			return DSP;
		case 15: // #define tPrime_NuT 0x0F
			return 0; // what is (Nu<T) ?
	}
	return 0;
}

// ---------------------------------------------------------------------
void executeALU(WORD IR) {
	// Lower 13 bits ...
	// R->PC               [12:12] xxx1 xxxx xxxx xxxx (IR >> 12) & 0x0001
	// T'                  [11:08] xxxx NNNN xxxx xxxx (IR >> 11) & 0x000F
	// T->N                [07:07] xxxx xxxx 1xxx xxxx (IR >>  7) & 0x0001
	// T->R                [06:06] xxxx xxxx x1xx xxxx (IR >>  6) & 0x0001
	// STORE (N->[T])      [05:05] xxxx xxxx xx1x xxxx (IR >>  5) & 0x0001
	// UNUSED              [04:04] xxxx xxxx xxx1 xxxx (IR >>  4) & 0x0010
	// DSP -= 1            [03:02] xxxx xxxx xxxx 1xxx (IR >>  2) & 0x0003 == 0x0003
	// DSP += 1            [03:02] xxxx xxxx xxxx x1xx (IR >>  2) & 0x0001 == 0x0001
	// RSP -= 1            [01:00] xxxx xxxx xxxx xx1x (IR & 0x0003) == 0x0003
	// RSP += 1            [01:00] xxxx xxxx xxxx xxx1 (IR & 0x0003) == 0x0001
	WORD tPrime = getTprime((IR & 0x0F00) >> 8);
	
	if (IR & bitRtoPC) { PC = R; }
	if (IR & bitStore) {
		if ((0 <= T) && (T < MEM_SZ)) {
			the_memory[T] = N;
		} else {
			int portNum = T & 0xFF;			
			if (portNum == emitPort) { printf("%c", N); }
			if (portNum == dotPort)  { printf(" %d", N); }
		}
		DSP--;
		tPrime = N;
	}
	if (IR & bitIncRSP) { RSP++; }
	if (IR & bitDecRSP) { RSP--; }
	if (IR & bitTtoR) { R = T; }
	if (IR & bitTtoN) { N = T; }
	if (IR & bitIncDSP) { DSP++; }
	if (IR & bitDecDSP) { DSP--; }

	if (DSP < 1)      { DSP = 0; }
	if (DSP > STK_SZ) { DSP = STK_SZ; }

	T = tPrime;
}

// ---------------------------------------------------------------------
void j1_emu(CELL start, int maxCycles)
{
	int cycle = 0;
	running = 1;
	PC = start;
	while (running)
	{
		// printf("\nPC: %04X  DSP: %-2d N: %-5d T: %-5d", PC, DSP, N, T);
		// printf(" RSP: %-2d R: %-3d cycle: %-3d", RSP, R, cycle);
		// printf(" IR: %04X", the_memory[PC]);
        WORD IR = the_memory[PC++];

		// The top 3 bits identify the class of operation ...
		// 1xxx => LIT  (1xxx xxxx xxxx xxxx) (IR & 0x8000) == 0x8000
		// 000x => JMP  (000x xxxx xxxx xxxx) (IR & 0xE000) == 0x0000
		// 001x => JMPZ (001x xxxx xxxx xxxx) (IR & 0xE000) == 0x2000
		// 010x => CALL (010x xxxx xxxx xxxx) (IR & 0xE000) == 0x4000
		// 011x => ALU  (011x xxxx xxxx xxxx) (IR & 0xE000) == 0x6000

		if ((IR & opLIT) == opLIT) {                  // LITERAL
			if (DSP < STK_SZ) { DSP++; }
			T = (IR & 0x7FFF);
			// printf(" - LIT %d", T);
		} else if ((IR & INSTR_MASK) == opJMP) {      // JMP
			PC = IR & ADDR_MASK;
			// printf(" - JMP %d", PC);
		} else if ((IR & INSTR_MASK) == opJMPZ) {      // JMPZ (0BRANCH)
			// printf(" - JMPZ %d", IR & ADDR_MASK);
			PC = (T == 0) ? (IR & ADDR_MASK) : PC;
			DSP--;
		} else if ((IR & INSTR_MASK) == opCALL) {      // CALL
			// printf(" - CALL %d", IR & ADDR_MASK);
			RSP += (RSP < STK_SZ) ? 1 : 0;
			R = PC;
			PC = (IR & ADDR_MASK);
		} else if ((IR & INSTR_MASK) == opALU) {      // ALU
			// printf(" - ALU: ");
			executeALU(IR);
		}

		if (maxCycles && (++cycle >= maxCycles)) { running = false; }
		if (RSP < 0) {
			RSP = 0;
			running = false;
		}
	}
}

void dumpStack(int sp, WORD *stk) {
	printf("(");
	for (int i = 1; i <= sp; i++) {
		printf(" %d", stk[i]);
	}
	printf(" )");
}

void disALU(WORD IR, char *output) {
	strcat(output, "\n    ");

	WORD aluOp = IR & 0x0F00;
	if (aluOp == aluTgetsT) { strcat(output, " T'<-T"); }
	if (aluOp == aluTgetsN) { strcat(output, " T'<-N"); }
	if (aluOp == aluTgetsR) { strcat(output, " T'<-R"); }
	if (aluOp == aluTplusN) { strcat(output, " T'<-(T+N)"); }
	if (aluOp == aluTandN)  { strcat(output, " T'<-(TandN)"); }
	if (aluOp == aluTorN)   { strcat(output, " T'<-(TorN)"); }
	if (aluOp == aluTxorN)  { strcat(output, " T'<-(TxorN)"); }
	if (aluOp == aluNotT)   { strcat(output, " T'<-(notT)"); }
	if (aluOp == aluTeqN)   { strcat(output, " T'<-(T==N)"); }
	if (aluOp == aluTltN)   { strcat(output, " T'<-(T<N)"); }
	if (aluOp == aluSHR)    { strcat(output, " T'<-(N>>T)"); }
	if (aluOp == aluDecT)   { strcat(output, " T'<-(T-1)"); }
	if (aluOp == aluFetch)  { strcat(output, " T'<-[T]"); }
	if (aluOp == aluSHL)    { strcat(output, " T'<-(N<<T)"); }
	if (aluOp == aluDepth)  { strcat(output, " T'<-Depth"); }
	if (aluOp == alu15)     { strcat(output, " T'<-(Nu<T)"); }

	if (IR & bitRtoPC)   { strcat(output, "   R->PC"); }
	if (IR & bitStore)   { strcat(output, "   N->[T]"); }
	if (IR & bitIncRSP)  { strcat(output, "   ++RSP"); }
	if (IR & bitDecRSP)  { strcat(output, "   --RSP"); }
	if (IR & bitTtoR)    { strcat(output, "   T->R"); }
	if (IR & bitTtoN)    { strcat(output, "   T->N"); }
	// if (IR & bitUnused)  { strcat(output, "   (unused)"); }
	if (IR & bitDecDSP)  { strcat(output, "   --DSP"); }
	if (IR & bitIncDSP)  { strcat(output, "   ++DSP"); }
}

void disIR(WORD IR, char *output) {
	char buf[256];
	sprintf(buf, "Unknown IR %04X", IR);
	if ((IR & opLIT) == opLIT) {
		WORD val = (IR & 0x7FFF);
		sprintf(buf, "%-8s %-5d   # (0x%04X)", "LIT", val, val);
	} else if ((IR & INSTR_MASK) == opJMP) {
		WORD addr = (IR & ADDR_MASK);
		sprintf(buf, "%-8s %-5d   # (0x%04X)", "JMP", addr, addr);
	} else if ((IR & INSTR_MASK) == opJMPZ) {
		WORD addr = (IR & ADDR_MASK);
		sprintf(buf, "%-8s %-5d   # (0x%04X)", "JMPZ", addr, addr);
	} else if ((IR & INSTR_MASK) == opCALL) {
		WORD addr = (IR & ADDR_MASK);
		sprintf(buf, "%-8s %-5d   # (0x%04X)", "CALL", addr, addr);
	} else if ((IR & INSTR_MASK) == opALU) {
		WORD val = (IR & ADDR_MASK);
		sprintf(buf, "%-8s %-5d   # (0x%04X)", "ALU", val, val);
		disALU(IR, buf);
	}
	if (output) {
		strcpy(output, buf);
	} else {
		printf("\n%s", buf);
	}
}
