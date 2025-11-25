// J1 white paper is here: https://excamera.com/files/j1.pdf

#include "j1.h"

extern CELL PC;

void dumpState(bool lastPC, WORD IR) {
	printf("\nPC: %04X  DSP: %-2d N: %-5d T: %-5d", PC, DSP, N, T);
	printf(" RSP: %-2d R: %-3d cycle: %-4ld", RSP, R);
	printf(" IR: %04X", the_memory[PC - ((lastPC) ? 1 : 0)]);
	disIR(IR, NULL);
}

void dumpStack(int sp, WORD *stk) {
	printf("( ");
	for (int i = 1; i <= sp; i++) {
		printf("%d ", stk[i]);
	}
	printf(")");
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
		printf("%s", "\n");
		printf("%s", buf);
	}
}
