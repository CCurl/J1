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
CELL newT;

const WORD memory_size = MEM_SZ;
int running = 0;
CELL HERE = 0;

// ---------------------------------------------------------------------
void j1_init()
{
	DSP = 0;
	RSP = 0;
	PC = 0;
	running = 0;
}

// ---------------------------------------------------------------------
void setNewT(WORD OP) {
	int op = (OP & 0x0F00) >> 8;
	switch (OP) {
		case 0: // #define newT_T 0x00
			newT = T;
			break;
		case 1: // #define newT_N 0x01
			newT = N;
			break;
		case 2: // #define newT_ADD 0x02
			newT = (T + N);
			break;
		case 3: // #define newT_AND 0x03
			newT = (T & N); // AND
			break;
		case 4: // #define newT_OR 0x04
			newT = (T | N); // OR
			break;
		case 5: // #define newT_XOR 0x05
			newT = (T ^ N); // XOR
			break;
		case 6: // #define newT_NOT 0x06
			newT = (~T); // NOT (aka - INVERT)
			break;
		case 7: // #define newT_EQ 0x07
			newT = (N == T) ? 1 : 0;
			break;
		case 8: // #define newT_LT 0x08
			newT = (N < T) ? 1 : 0;
			break;
		case 9: // #define newT_SHR 0x09
			newT = (N >> T);
			break;
		case 10: // #define newT_DEC 0x0A
			newT = (T-1);
			break;
		case 11: // #define newT_R 0x0B
			newT = R;
			break;
		case 12: // #define newT_FETCH 0x0C
			newT = the_memory[T];
			break;
		case 13: // #define newT_SHL 0x0D
			newT = (N << T);
			break;
		case 14: // #define newT_DEPTH 0x0E
			newT = DSP;
			break;
		case 15: // #define newT_NuT 0x0F
			newT = 0; // what is (Nu<T) ?
			break;
	}
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
	// DSP -= 1            [03:02] xxxx xxxx xxxx 11xx (IR >>  2) & 0x0003 == 0x0003
	// DSP (no change)     [03:02] xxxx xxxx xxxx 10xx (IR >>  2) & 0x0002 == 0x0002
	// DSP += 1            [03:02] xxxx xxxx xxxx 01xx (IR >>  2) & 0x0001 == 0x0001
	// DSP (no change)     [03:02] xxxx xxxx xxxx 00xx (IR >>  2) & 0x0000 == 0x0000
	// RSP -= 1            [01:00] xxxx xxxx xxxx xx11 (IR & 0x0003) == 0x0003
	// RSP (no change)     [01:00] xxxx xxxx xxxx xx10 (IR & 0x0003) == 0x0002
	// RSP += 1            [01:00] xxxx xxxx xxxx xx01 (IR & 0x0003) == 0x0001
	// RSP (no change)     [01:00] xxxx xxxx xxxx xx00 (IR & 0x0003) == 0x0000
	WORD origN = N;
	WORD origT = T;
	setNewT((IR >> 8) & 0x0F);
	
	if (IR & bitRtoPC) { PC = R; }
	if (IR & bitDecDSP) { DSP--; }
	if (IR & bitIncDSP) { DSP++; }
	if (IR & bitDecRSP) { RSP--; }
	if (IR & bitIncRSP) { RSP++; }
	if (IR & bitTtoN) { N = origT; }
	if (IR & bitTtoR) { R = origT; }
	if (IR & bitStore) {
		if ((0 <= origT) && (origT < MEM_SZ)) {
			the_memory[origT] = origN;
		} else {
			int portNum = origT & 0xFF;			
			switch (portNum)
			{
			case emitPort:
				printf("%c", origN);
				break;
			
			case 2:
				printf("%d", origN);
				break;
			
			default:
				printf("-invalid port #%d-", portNum);
			}
		}
		DSP -= (DSP > 0) ? 1 : 0;
		newT = T;
	}
	T = newT;
}

// ---------------------------------------------------------------------
void j1_emu(CELL start, int maxCycles)
{
	int cycle = 0;
	running = 1;
	PC = start;

	while (running)
	{
		if (maxCycles) {
			running = (++cycle >= maxCycles) ? 0 : 1;
		}
		printf("\nPC: %-4d DSP: %-2d N: %-5d T: %-5d", PC, DSP, N, T);
		printf(" RSP: %-2d R: %-3d cycle: %-3d", RSP, R, cycle);
        WORD IR = the_memory[PC++];
		printf(" IR: %04X", IR);

		// The top 3 bits identify the class of operation ...
		// 1xxx => LIT  (1xxx xxxx xxxx xxxx) (IR & 0x8000) != 0x0000
		// 000x => JMP  (000x xxxx xxxx xxxx) (IR & 0x6000) == 0x0000
		// 001x => JMPZ (001x xxxx xxxx xxxx) (IR & 0x6000) == 0x2000
		// 010x => CALL (010x xxxx xxxx xxxx) (IR & 0x6000) == 0x4000
		// 011x => ALU  (011x xxxx xxxx xxxx) (IR & 0x6000) == 0x6000
		// (0110000000000000)
		// (0000000000000011)

		if ((IR & 0x8000) != 0x0000) {             // LITERAL
			DSP += (DSP < STK_SZ) ? 1 : 0;
			T = (IR & 0x7FFF);
			printf(" - LIT %d", T);
		} else if ((IR & 0x6000) == 0x0000) {      // JMP
			PC = IR & 0x1FFF;
			printf(" - JMP %d", PC);
		} else if ((IR & 0x6000) == 0x2000) {      // JMPZ (0BRANCH)
			printf(" - JMPZ %d", IR & 0x1FFF);
			PC = (T == 0) ? (IR & 0x1FFF) : PC;
			DSP -= (DSP > 0) ? 1 : 0;
		} else if ((IR & 0x6000) == 0x4000) {      // CALL
			printf(" - CALL %d", IR & 0x1FFF);
			RSP += (RSP < STK_SZ) ? 1 : 0;
			R = PC;
			PC = (IR & 0x1FFF);
		} else if ((IR & 0x6000) == 0x6000) {      // ALU
			printf(" - ALU: ");
			executeALU(IR);
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
	strcat(output, "\n        ");

	WORD aluOp = IR & 0x0F00;
	if (aluOp == aluTgetsT) { strcat(output, " T<-T"); }
	if (aluOp == aluTgetsN) { strcat(output, " T<-N"); }
	if (aluOp == aluTplusN) { strcat(output, " T<-(T+N)"); }
	if (aluOp == aluTandN)  { strcat(output, " T<-(TandN)"); }
	if (aluOp == aluTorN)   { strcat(output, " T<-(TorN)"); }
	if (aluOp == aluTxorN)  { strcat(output, " T<-(TxorN)"); }
	if (aluOp == aluNotT)   { strcat(output, " T<-(notT)"); }
	if (aluOp == aluTeqN)   { strcat(output, " T<-(T==N)"); }
	if (aluOp == aluTltN)   { strcat(output, " T<-(T<N)"); }
	if (aluOp == aluSHR)    { strcat(output, " T<-(N>>T)"); }
	if (aluOp == alu10) { strcat(output, " T<-(XXX)"); }
	if (aluOp == alu11) { strcat(output, " T<-(XXX)"); }
	if (aluOp == alu12) { strcat(output, " T<-(XXX)"); }
	if (aluOp == alu13) { strcat(output, " T<-(XXX)"); }
	if (aluOp == alu14) { strcat(output, " T<-(XXX)"); }
	if (aluOp == alu15) { strcat(output, " T<-(XXX)"); }

	if (IR & bitTtoN)    { strcat(output, " T->N"); }
	if (IR & bitTtoR)    { strcat(output, " T->R"); }
	if (IR & bitStore)   { strcat(output, " N->[T]"); }
	if (IR & bitUnused)  { strcat(output, " (unused)"); }
	if (IR & bitDecDSP)  { strcat(output, " --DSP"); }
	if (IR & bitIncDSP)  { strcat(output, " ++DSP"); }
	if (IR & bitIncRSP)  { strcat(output, " ++RSP"); }
	if (IR & bitRtoPC)   { strcat(output, " R->PC"); }
	if (IR & bitDecRSP)  { strcat(output, " --RSP"); }
}

void disIR(WORD IR, char *output) {
	char buf[256];
	sprintf(buf, "Unknown IR %04X", IR);
	if ((IR & opLIT) == opLIT) {
		WORD val = IR & 0x7FFF;
		sprintf(buf, "%-8s %-5d   # (0x%04X)", "LIT", val, val);
	} else if ((IR & 0x7000) == 0) {
		WORD val = IR & (0x1FFF);
		sprintf(buf, "%-8s %-5d   # (0x%04X)", "JMP", val, val);
	} else if ((IR & 0x6000) == 0x2000) {
		WORD val = IR & (0x1FFF);
		sprintf(buf, "%-8s %-5d   # (0x%04X)", "JMPZ", val, val);
	} else if ((IR & 0x6000) == 0x4000) {
		WORD val = IR & (0x1FFF);
		sprintf(buf, "%-8s %-5d   # (0x%04X)", "CALL", val, val);
	} else if ((IR & 0x6000) == 0x6000) {
		WORD val = IR & (0x1FFF);
		sprintf(buf, "%-8s %-5d   # (0x%04X)", "ALU", val, val);
		disALU(IR, buf);
	}
	if (output) {
		strcpy(output, buf);
	} else {
		printf("\n%s", buf);
	}
}
