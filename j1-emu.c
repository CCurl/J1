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

int running = 0;
long cycle;
void dumpState(bool);

// ---------------------------------------------------------------------
void j1_init()
{
	DSP = 0;
	RSP = 0;
	PC = 0;
	running = 0;
}

// ---------------------------------------------------------------------
WORD getTprime(WORD IR) {
	int op = (IR & 0x0F00) >> 8;
	switch (op) {
		case tpTgetsT:
			return T;
		case tpTgetsN:
			return N;
		case tpTplusN:
			return (T + N);
		case tpTandN:
			return (T & N);
		case tpTorN:
			return (T | N);
		case tpTxorN:
			return (T ^ N);
		case tpNotT:
			return (~T);
		case tpTeqN:
			return (N == T) ? 1 : 0;
		case tpTltN:
			return (N < T) ? 1 : 0;
		case tpSHR:
			return (N >> T);
		case tpDecT:
			return (T-1);
		case tpTgetsR:
			return R;
		case tpFetch:
			return the_memory[T];
		case tpSHL:
			return (N << T);
		case tpDepth:
			return DSP;
		case tpNuLtT:         // what is (Nu<T) ?
			return 0;
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
	WORD tPrime = getTprime(IR);
	// disIR(IR, NULL);
	
	if (IR & bitRtoPC)  { PC = R; }
	if (IR & bitIncRSP) { RSP++; }
	if (IR & bitDecRSP) { RSP--; }
	if (IR & bitTtoR)   { R = T; }

	if (IR & bitStore) {
		if ((0 <= T) && (T < MEM_SZ)) the_memory[T] = N;
		else writePort(T, N);
		DSP--;
		tPrime = N;
	}
	if (IR & bitTtoN) { N = T; }

	if (IR & bitIncDSP) { DSP++; }
	if (IR & bitDecDSP) { DSP--; }
	if (DSP < 0)        { DSP = 0; }
	if (DSP > STK_SZ)   { DSP = STK_SZ; }

	T = tPrime;
}

// ---------------------------------------------------------------------
void j1_emu(CELL start, long maxCycles)
{
	cycle = 0;
	running = 1;
	PC = start;
	while (running)
	{
		// dumpState(false);
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

void dumpState(bool lastPC) {
		writePort_StringF("\nPC: %04X  DSP: %-2d N: %-5d T: %-5d", PC, DSP, N, T);
		writePort_StringF(" RSP: %-2d R: %-3d cycle: %-4ld", RSP, R, cycle);
		writePort_StringF(" IR: %04X", the_memory[PC - ((lastPC) ? 1 : 0)]);
}

void dumpStack(int sp, WORD *stk) {
	writePort(emitPort, '(');
	for (int i = 1; i <= sp; i++) {
		writePort(dotPort, stk[i]);
	}
	writePort_String(" )");
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
	if (aluOp == aluNuLtT)  { strcat(output, " T'<-(Nu<T)"); }

	if (IR & bitRtoPC)   { strcat(output, "   R->PC"); }
	if (IR & bitStore)   { strcat(output, "   N->[T]"); }
	if (IR & bitIncRSP)  { strcat(output, "   ++RSP"); }
	if (IR & bitDecRSP)  { strcat(output, "   --RSP"); }
	if (IR & bitTtoR)    { strcat(output, "   T->R"); }
	if (IR & bitTtoN)    { strcat(output, "   T->N"); }
	if (IR & bitUnused)  { strcat(output, "   (unused)"); }
	if (IR & bitDecDSP)  { strcat(output, "   --DSP"); }
	if (IR & bitIncDSP)  { strcat(output, "   ++DSP"); }
}

void disIR(WORD IR, char *output) {
	char buf[128];
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
		writePort_String("\n");
		writePort_String(buf);
	}
}
