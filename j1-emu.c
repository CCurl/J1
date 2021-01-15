// J1 white paper is here: https://excamera.com/files/j1.pdf

#include <stdio.h>
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
	switch (OP) {
		case 0:
			newT = T;
			break;
		case 1:
			newT = N;
			break;
		case 2:
			newT = (T + N);
			break;
		case 3:
			newT = (T & N); // AND
			break;
		case 4:
			newT = (T | N); // OR
			break;
		case 5:
			newT = (T ^ N); // XOR
			break;
		case 6:
			newT = (~T); // NOT (aka - INVERT)
			break;
		case 7:
			newT = (N == T) ? 1 : 0;
			break;
		case 8:
			newT = (N < T) ? 1 : 0;
			break;
		case 9:
			newT = (N >> T);
			break;
		case 10:
			newT = (T-1);
			break;
		case 11:
			newT = R;
			break;
		case 12:
			newT = the_memory[T];
			break;
		case 13:
			newT = (N << T);
			break;
		case 14:
			newT = DSP;
			break;
		case 15:
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
void j1_emu(CELL start)
{
	int cycle = 0;
	running = 1;
	PC = start;

	while (running)
	{
		printf("\nPC: %-3d DSP: %-2d N: %-5d T: %-5d", PC, DSP, N, T);
		printf(" RSP: %-2d R: %-3d cycle: %-3d", RSP, R, cycle);
		running = (++cycle > 20) ? 0 : 1;
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
