// J1 white paper is here: https://excamera.com/files/j1.pdf

#include "j1.h"

WORD the_memory[MEM_SZ];
CELL dstk[STK_SZ+1], DSP;
CELL rstk[STK_SZ+1], RSP;
CELL PC, running = 0, debugOn = false;
long cycle;

void push(CELL val) { if (DSP < STK_SZ) { dstk[++DSP] = val; } }
CELL pop() { return (0 < DSP) ? dstk[DSP--] : 0; }
void setDebugMode(bool isOn) { debugOn = isOn; }

// ---------------------------------------------------------------------
void j1_init() {
	PC = DSP = RSP = running = 0;
}

// ---------------------------------------------------------------------
WORD deriveNewT(WORD IR) {
	switch ((IR >> 8) & 0x0F) {
		case tpTgetsT: return T;
		case tpTgetsN: return N;
		case tpTplusN: return (T + N);
		case tpTandN:  return (T & N);
		case tpTorN:   return (T | N);
		case tpTxorN:  return (T ^ N);
		case tpNotT:   return (~T);
		case tpTeqN:   return (N == T) ? 1 : 0;
		case tpTltN:   return (N < T) ? 1 : 0;
		case tpSHR:    return (N >> T);
		case tpDecT:   return (T-1);
		case tpTgetsR: return R;
		case tpFetch:  return the_memory[T];
		case tpSHL:    return (N << T);
		case tpDepth:  return DSP;
		case tpNuLtT:  return 0;
	}
	return 0;
}

// ---------------------------------------------------------------------
void executeALU(WORD IR) {
	CELL currentT = T;
	CELL currentN = N;
	CELL currentR = R;
	CELL newT = deriveNewT(IR);

	if (debugOn) {
		dumpStack(DSP, dstk);
		writePort_StringF(" newT=[%d] ", newT);
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
		} else {
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
		WORD IR = the_memory[PC++];
		if (debugOn) { dumpState(false, IR); }

		// The top 3 bits identify the class of instruction ...
		// 1xxx => LIT  (1xxx xxxx xxxx xxxx) (IR & 0x8000) == 0x8000
		// 011x => ALU  (011x xxxx xxxx xxxx) (IR & 0xE000) == 0x6000
		// 010x => CALL (010x xxxx xxxx xxxx) (IR & 0xE000) == 0x4000
		// 001x => JMPZ (001x xxxx xxxx xxxx) (IR & 0xE000) == 0x2000
		// 000x => JMP  (000x xxxx xxxx xxxx) (IR & 0xE000) == 0x0000

		if ((IR & opLIT) == opLIT) {
			push(IR & 0x7FFF);
		} else if ((IR & INSTR_MASK) == opALU) {
			executeALU(IR);
		} else if ((IR & INSTR_MASK) == opCALL) {
			RSP += (RSP < STK_SZ) ? 1 : 0;
			R = PC;
			PC = (IR & ADDR_MASK);
		} else if ((IR & INSTR_MASK) == opJMPZ) {
			PC = (T == 0) ? (IR & ADDR_MASK) : PC;
			DSP--;
		} else if ((IR & INSTR_MASK) == opJMP) {
			PC = IR & ADDR_MASK;
		}

		if (maxCycles && (++cycle >= maxCycles)) { return; }
		if (RSP < 0) { RSP = 0; return; }
	}
}
