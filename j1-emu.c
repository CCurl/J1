// J1 white paper is here: https://excamera.com/files/j1.pdf

#include "j1.h"

WORD the_memory[MEM_SZ];
CELL dstk[STK_SZ+1], DSP;
CELL rstk[STK_SZ+1], RSP;
CELL PC, debugOn = false;

void push(CELL val) { if (DSP < STK_SZ) { dstk[++DSP] = val; } }
CELL pop() { return (0 < DSP) ? dstk[DSP--] : 0; }
void rpush(CELL val) { if (RSP < STK_SZ) { rstk[++RSP] = val; } }

void storeWord(WORD addr, WORD val) {
	if (BTWI(addr, 0, MEM_SZ-1)) { the_memory[addr] = val; }
	else if ((addr & 0x0FFF) == emitPort) { putc(val%0xff, stdout); }
	else if ((addr & 0x0FFF) == dotPort)  { printf(" %d", val); }
}

WORD readWord(WORD addr) {
	if (BTWI(addr, 0, MEM_SZ-1)) { return the_memory[addr]; }
	else {
		printf("WARN: readWord(0x%04x) not implemented.", addr & 0x0FFF);
		return 0;
	}
}

WORD deriveNewT(WORD op) {
	switch (op & 0x0F) {
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
		case tpFetch:  return readWord(T);
		case tpSHL:    return (N << T);
		case tpDepth:  return DSP;
		default: return 0; // tpNullT
	}
}

void j1_emu(WORD PC) {
	while (1) {
		WORD IR = the_memory[PC++];
		if ((IR & opLIT) == opLIT) {
			push(IR & 0x7FFF);
		} else if ((IR & INSTR_MASK) == opALU) {
			CELL currentT = T, currentN = N;
			CELL currentR = R, newT = deriveNewT(IR>>8);
			if (IR & bitIncRSP) { if (RSP < STK_SZ) { RSP++; } }
			if (IR & bitDecRSP) { RSP -= 1; }
			if (IR & bitIncDSP) { if (DSP < STK_SZ) { DSP++; } }
			if (IR & bitDecDSP) { if (0 < DSP) { DSP--; } }
			if (IR & bitRtoPC)  { PC = currentR; }
			if (IR & bitTtoR)   { R  = currentT; }
			if (IR & bitTtoN)   { N  = currentT; }
			if (IR & bitStore)  { storeWord(currentT, currentN); }
			T = newT;
		} else if ((IR & INSTR_MASK) == opCALL) {
			rpush(PC);
			PC = (IR & ADDR_MASK);
		} else if ((IR & INSTR_MASK) == opJMPZ) {
			if (pop() == 0) PC = (IR & ADDR_MASK);
		} else if ((IR & INSTR_MASK) == opJMP) {
			PC = IR & ADDR_MASK;
		}
		if (RSP < 0) { RSP = 0; return; }
	}
}

void main(int argc, char *argv[]) {
	FILE *fp = fopen("j1.bin", "rb");
	if (!fp) {
		printf(" ERROR: unable to open file 'j1.bin'\n");
	} else {
		PC = DSP = RSP = 0;
		fread(the_memory, 2, MEM_SZ, fp);
		fclose(fp);
		j1_emu(0);
	}
}
