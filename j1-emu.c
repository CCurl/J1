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
bool debugOn = false;
void dumpState(bool);

inline void push(CELL val) {
	if (++DSP > STK_SZ) { DSP = STK_SZ; }
	dstk[DSP] = val;
}

inline CELL pop() {
	if (--DSP < 1) { DSP = 0; }
	return dstk[DSP+1];
}

// ---------------------------------------------------------------------
void j1_init()
{
	DSP = 0;
	RSP = 0;
	PC = 0;
	running = 0;
}

// ---------------------------------------------------------------------
WORD deriveNewT(WORD IR) {
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
	WORD currentT = T;
	WORD currentN = N;
	WORD currentR = R;
	WORD newT = deriveNewT(IR);

	if (debugOn) {
		dumpStack(DSP, dstk);
		writePort_StringF(" newT=[%d] ", newT);
		disIR(IR, NULL);
	}
	
	if (IR & bitIncRSP) { RSP += (RSP < STK_SZ) ? 1 : 0; }
	if (IR & bitDecRSP) { RSP -= 1; }
	if (IR & bitIncDSP) { DSP += (DSP < STK_SZ) ? 1 : 0; }
	if (IR & bitDecDSP) { DSP -= (DSP > 0)      ? 1 : 0; }

	if (IR & bitRtoPC)  { PC = currentR; }
	if (IR & bitTtoR)   { R  = currentT; }
	if (IR & bitTtoN)   { N  = currentT; }
	if (IR & bitStore)  {
		if ((0 <= currentT) && (currentT < MEM_SZ)) {
			the_memory[currentT] = currentN;
		}
		else {
			writePort(currentT, currentN);
		}
	}

	T = newT;
}

// ---------------------------------------------------------------------
void j1_emu(WORD start, long maxCycles)
{
	cycle = 0;
	PC = start;
	while (true)
	{
		if (debugOn) { dumpState(false); }
        WORD IR = the_memory[PC++];

		// The top 3 bits identify the class of operation ...
		// 1xxx => LIT  (1xxx xxxx xxxx xxxx) (IR & 0x8000) == 0x8000
		// 000x => JMP  (000x xxxx xxxx xxxx) (IR & 0xE000) == 0x0000
		// 001x => JMPZ (001x xxxx xxxx xxxx) (IR & 0xE000) == 0x2000
		// 010x => CALL (010x xxxx xxxx xxxx) (IR & 0xE000) == 0x4000
		// 011x => ALU  (011x xxxx xxxx xxxx) (IR & 0xE000) == 0x6000

		if ((IR & opLIT) == opLIT) {
			push(IR & 0x7FFF);
			if (debugOn) { printf(" - LIT %d", T); }
		} else if ((IR & INSTR_MASK) == opJMP) {
			PC = IR & ADDR_MASK;
			if (debugOn) { printf(" - JMP %d", PC); }
		} else if ((IR & INSTR_MASK) == opJMPZ) {
			if (debugOn) { printf(" - JMPZ %d", IR & ADDR_MASK); }
			PC = (T == 0) ? (IR & ADDR_MASK) : PC;
			DSP--;
		} else if ((IR & INSTR_MASK) == opCALL) {
			if (debugOn) { printf(" - CALL %d", IR & ADDR_MASK); }
			RSP += (RSP < STK_SZ) ? 1 : 0;
			R = PC;
			PC = (IR & ADDR_MASK);
		} else if ((IR & INSTR_MASK) == opALU) {
			if (debugOn) { printf(" - ALU: "); }
			executeALU(IR);
		}

		if (maxCycles && (++cycle >= maxCycles)) { return; }
		if (RSP < 0) {
			RSP = 0;
			return;
		}
	}
}

void setDebugMode(bool isOn) {
	debugOn = isOn;
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
	if (aluOp == aluTgetsT) { strcat(output, "T"); }
	if (aluOp == aluTgetsN) { strcat(output, "N"); }
	if (aluOp == aluTgetsR) { strcat(output, "R"); }
	if (aluOp == aluTplusN) { strcat(output, "T+N"); }
	if (aluOp == aluTandN)  { strcat(output, "T&N"); }
	if (aluOp == aluTorN)   { strcat(output, "T|N"); }
	if (aluOp == aluTxorN)  { strcat(output, "T^N"); }
	if (aluOp == aluNotT)   { strcat(output, "~T"); }
	if (aluOp == aluTeqN)   { strcat(output, "N==T"); }
	if (aluOp == aluTltN)   { strcat(output, "N<T"); }
	if (aluOp == aluSHR)    { strcat(output, "N>>T"); }
	if (aluOp == aluDecT)   { strcat(output, "T-1"); }
	if (aluOp == aluFetch)  { strcat(output, "[T]"); }
	if (aluOp == aluSHL)    { strcat(output, "N<<T"); }
	if (aluOp == aluDepth)  { strcat(output, "dsp"); }
	if (aluOp == aluNuLtT)  { strcat(output, "Nu<T"); }

	if (IR & bitRtoPC)   { strcat(output, "   R->PC"); }
	if (IR & bitStore)   { strcat(output, "   N->[T]"); }
	if (IR & bitIncRSP)  { strcat(output, "   r+1"); }
	if (IR & bitDecRSP)  { strcat(output, "   r-1"); }
	if (IR & bitTtoR)    { strcat(output, "   T->R"); }
	if (IR & bitTtoN)    { strcat(output, "   T->N"); }
	if (IR & bitUnused)  { strcat(output, "   (unused)"); }
	if (IR & bitDecDSP)  { strcat(output, "   d-1"); }
	if (IR & bitIncDSP)  { strcat(output, "   d+1"); }
}

void disIR(WORD IR, char *output) {
	char buf[128];
	WORD arg;
	sprintf(buf, "Unknown IR %04X", IR);
	if ((IR & opLIT) == opLIT) {
		arg = (IR & 0x7FFF);
		sprintf(buf, "%-8s %-5d   # (0x%04X)", "LIT", arg, arg);
	} else if ((IR & INSTR_MASK) == opJMP) {
		arg = (IR & ADDR_MASK);
		sprintf(buf, "%-8s %-5d   # (0x%04X)", "JMP", arg, arg);
	} else if ((IR & INSTR_MASK) == opJMPZ) {
		arg = (IR & ADDR_MASK);
		sprintf(buf, "%-8s %-5d   # (0x%04X)", "JMPZ", arg, arg);
	} else if ((IR & INSTR_MASK) == opCALL) {
		arg = (IR & ADDR_MASK);
		sprintf(buf, "%-8s %-5d   # (0x%04X)", "CALL", arg, arg);
	} else if ((IR & INSTR_MASK) == opALU) {
		arg = (IR & ADDR_MASK);
		sprintf(buf, "%-8s %-5d   # (0x%04X)", "ALU", arg, arg);
		disALU(IR, buf);
	}
	if (output) {
		strcpy(output, buf);
	} else {
		writePort_String("\n");
		writePort_String(buf);
	}
}
