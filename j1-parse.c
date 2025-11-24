#include "j1.h"

char base_fn[32];
bool save_output = true;
bool debug_flag = false;
int exitStatus = 0;
long maxCycles = 0;

WORD the_memory[MEM_SZ];
CELL dstk[STK_SZ+1], DSP;
CELL rstk[STK_SZ+1], RSP;
CELL PC, debugOn = false;
long cycle;

#define LAST_OP the_memory[HERE-1]
#define COMMA(val) the_memory[HERE++] = val

#define TIB_SZ 1024
char tib[TIB_SZ];
char *toIn;

DICT_T words[2048];
WORD numWords = 0;

WORD HERE = 0;
WORD STATE = 0;

// ---------------------------------------------------------------------
void j1_init() { PC = DSP = RSP = 0; }
void push(CELL val) { if (DSP < STK_SZ) { dstk[++DSP] = val; } }
CELL pop() { return (0 < DSP) ? dstk[DSP--] : 0; }

// ---------------------------------------------------------------------
int getWord(char *word) {
	int len = 0;
	while (BTWI(*toIn,1,32)) { ++toIn; }
	while (BTWI(*toIn,33,126)) { word[len++] = *(toIn++); }
	word[len] = 0;
	return len;
}

// ---------------------------------------------------------------------
int isNumber(char *w, WORD *value) {
	WORD n=0, b=10, isNeg=0;
	if ((w[0]==39) && (w[2]==39) && (w[3]==0)) { *value=w[1]; return 1; }
	if (w[0]=='#') { b=10; w++; }
	if (w[0]=='$') { b=16; w++; }
	if (w[0]=='%') { b=2; w++; }
	if (w[0]=='-') { isNeg=1; w++; }
	if (w[0]==0) { return 0; }
	while (*w) {
		int c = *(w++);
		int x = BTWI(c,'0','9') ? c-'0' : 99;
		if (BTWI(c,'A','F')) { x = (c-'A'+10); }
		if (BTWI(c,'a','f')) { x = (c-'a'+10); }
		if (BTWI(x, 0, b-1)) { n = (n*b)+x; } else { return 0; }
	}
	*value = (isNeg ? -n : n);
	return 1;
}

// ---------------------------------------------------------------------
void defineWord(char *name) {
	DICT_T *p = &words[numWords++];
	strcpy(p->name, name);
	p->xt = HERE;
	p->flags = 0;
	p->len = 0;
	if (debug_flag) printf("\nDefined (%d) [%s] at addr %02X", numWords-1, name, HERE);
}

// ---------------------------------------------------------------------
DICT_T *findWord(char *word) {
	for (int i = numWords-1; i >= 0; i--) {
		if (strcmp(words[i].name, word) == 0) {
			return &words[i];
		}
	}
	return NULL;
}

// ---------------------------------------------------------------------
void parseWord(char *word) {
	// if (debug_flag) printf("\n[%s] (HERE=%d), LAST_OP=%04X", word, HERE, LAST_OP);
	WORD num = 0;
	WORD op = LAST_OP;
	if (isNumber(word, &num)) {
		if ((num & 0x8000) == 0) {
			op = MAKE_LIT(num);
			COMMA(op);
		} else {
			// For numbers larger than 0x7FFF
			num = ~num;
			COMMA(MAKE_LIT(num));
			COMMA(MAKE_ALU(0x0030));
		}
		return;
	}
	DICT_T *w = findWord(word);
	if (w) {
		// INLINE is only for words where the last operation is ALU
		if (w->flags & 0x01) {
			WORD a = w->xt;
			for (WORD i = 0; i < w->len; i++) {
				COMMA(the_memory[a++]);
			}
			// Clear the R->PC and --RSP bits
			LAST_OP &= ~(bitRtoPC|bitDecRSP);
		} else if (w->flags & 0x08) {
			// MACRO
			COMMA(w->macroVal);
		} else {
			op = MAKE_CALL(w->xt);
			COMMA(op);
		}
		return;
	}
	if (strcmp(word, ":") == 0) {
		getWord(word);
		defineWord(word);
		STATE = 1;
		return;
	}
	if (strcmp(word, "MACRO") == 0) {
		DICT_T *w = &words[numWords-1];
		if (w->len == 1) {
			op = the_memory[--HERE]; 
			the_memory[HERE] = 0; 
			op &= ~(bitRtoPC|bitDecRSP);
			w->xt = 0x0000;
			w->flags = 0x08;
			w->macroVal = op;
			if (debug_flag) { printf("MACRO: [%s], val=%02X", w->name, w->macroVal); }
		} else {
			printf("\nWARN: [%s] length must be 1 for MACRO", w->name);
		}
		return;
	}
	if (strcmp(word, "INLINE") == 0) {
		if ((LAST_OP & 0xE000) == opALU) {
			words[numWords-1].flags |= 1;
		}
		return;
	}
	if (strcmp(word, ";") == 0) {
		STATE = 0;
		words[numWords-1].len = (HERE - words[numWords-1].xt);
		// Change last operation to JMP if CALL
		if ((LAST_OP & 0xE000) == opCALL) {
			LAST_OP = (LAST_OP & 0x1FFF) | opJMP;
			if (debug_flag) printf("\nchanged op at %d to JMP", HERE-1);
			return;
		}
		bool canAddRet = true;
		if ((LAST_OP & 0xE000) != opALU) canAddRet = false; // not ALU
		if ((LAST_OP & bitRtoPC) != 0)   canAddRet = false; // R->PC already set
		if ((LAST_OP & bitIncRSP) != 0)  canAddRet = false; // R++ set
		if ((LAST_OP & bitDecRSP) != 0)  canAddRet = false; // R-- already set
		if (canAddRet) {
			LAST_OP |= bitRtoPC;
			LAST_OP |= bitDecRSP;
			if (debug_flag) printf("\nAdded %04X to ALU op at %d", (bitDecDSP|bitRtoPC), HERE-1);
			return;
		}
		// cannot include in previous op :(
		op = opALU;
		op |= bitRtoPC;
		op |= bitDecRSP;
		COMMA(op);
		words[numWords-1].len = (HERE - words[numWords-1].xt);
		return;
	}
	if (strcmp(word, "alu") == 0) {
		if (debug_flag) printf(" putting ALU %04X to [%d]", HERE);
		op = MAKE_ALU(pop());
		COMMA(op);
		return;
	}
	if (strcmp(word, "T")      == 0) { push(aluTgetsT); return; }
	if (strcmp(word, "N")      == 0) { push(aluTgetsN); return; }
	if (strcmp(word, "rT")     == 0) { push(aluTgetsR); return; }
	if (strcmp(word, "T+N")    == 0) { push(aluTplusN); return; }
	if (strcmp(word, "T&N")    == 0) { push(aluTandN);  return; }
	if (strcmp(word, "T|N")    == 0) { push(aluTorN);   return; }	
	if (strcmp(word, "T^N")    == 0) { push(aluTxorN);  return; }
	if (strcmp(word, "~T")     == 0) { push(aluNotT);   return; }
	if (strcmp(word, "N==T")   == 0) { push(aluTeqN);   return; }
	if (strcmp(word, "N<T")    == 0) { push(aluTltN);   return; }
	if (strcmp(word, "N>>T")   == 0) { push(aluSHR);    return; }
	if (strcmp(word, "N<<T")   == 0) { push(aluSHL);    return; }
	if (strcmp(word, "T-1")    == 0) { push(aluDecT);   return; }
	if (strcmp(word, "[T]")    == 0) { push(aluFetch);  return; }
	if (strcmp(word, "dsp")    == 0) { push(aluDepth);  return; }
	if (strcmp(word, "Nu<T")   == 0) { push(aluNuLtT);  return; }
	if (strcmp(word, "N->[T]") == 0) { T |= bitStore;   return; }
	if (strcmp(word, "R->PC")  == 0) { T |= bitRtoPC;   return; }
	if (strcmp(word, "T->N")   == 0) { T |= bitTtoN;    return; }
	if (strcmp(word, "T->R")   == 0) { T |= bitTtoR;    return; }
	if (strcmp(word, "r+1")    == 0) { T |= bitIncRSP;  return; }
	if (strcmp(word, "r-1")    == 0) { T |= bitDecRSP;  return; }
	if (strcmp(word, "d+1")    == 0) { T |= bitIncDSP;  return; }
	if (strcmp(word, "d-1")    == 0) { T |= bitDecDSP;  return; }
	if (strcmp(word, ">r")     == 0) {
		op = MAKE_ALU(aluTgetsN|bitTtoR|bitIncRSP|bitDecDSP);
		COMMA(op);
		return;
	}
	if (strcmp(word, "r>") == 0) {
		op = MAKE_ALU(aluTgetsR|bitDecRSP|bitIncDSP);
		COMMA(op);
		return;
	}
	if (strcmp(word, "r@") == 0) {
		op = MAKE_ALU(aluTgetsR|bitIncDSP);
		COMMA(op);
		return;
	}
	if (strcmp(word, "XXX") == 0) {
		// do something ...
		return;
	}
	printf("\nERROR: unknown word: [%s]\n", word);
	exitStatus = 1;
}

// ---------------------------------------------------------------------
void parseLine(char *line) {
	char word[32];
	toIn = line;
	while (true) {
		int len = getWord(word);
		// printf("[%s]", word);
		if (len) {
			if (strcmp(word, "\\") == 0) { return; }
			if (strcmp(word, "//") == 0) { return; }
			parseWord(word);
		} else {
			return;
		}
	}
}

// ---------------------------------------------------------------------
void doCompile(FILE *fp) {
	char *inSave = toIn;
	while (true) {
		if (fgets(tib, TIB_SZ, fp) == tib) {
			parseLine(tib);
		} else {
			toIn = inSave;
			return;
		}
	}
}

// ---------------------------------------------------------------------
int load(char *base) {
	char fn[32];
	sprintf(fn, "%s.src", base_fn);
	FILE *fp = fopen(fn, "rt");
	if (fp) {
		doCompile(fp);
		fclose(fp);
	} else {
		printf("ERROR: unable to open '%s'\n", fn);
		return 1;
	}
}

// ---------------------------------------------------------------------
void doDisassemble(bool toFile) {
	FILE *fp = NULL;
	if (toFile) {
		char fn[32];
		sprintf(fn, "%s.lst", base_fn);
		fp = fopen(fn, "wt");
		if (!fp) {
			printf("\nUnable to create listing file '%s'.", fn);
			return;
		}
		fprintf(fp, "; HERE: 0x%04X (%d)\n", HERE, HERE);
	}

	char buf[256];
	for (int i = 0; i < numWords; i++) {
		DICT_T *p = &words[i];
		sprintf(buf, "; %2d: XT: %04X, Len: %2d, Flags: %02X, MacroVal: %02X, Name: %s\n", i,
			p->xt, p->len, p->flags, p->macroVal, p->name);
		(fp) ? fprintf(fp, "%s", buf) : printf("%s", buf);
	}

	for (int i = 0; i < HERE; i++) {
		WORD ir = the_memory[i];
		disIR(ir, buf);
		if (fp) {
			fprintf(fp, "\n%04X: %04X    ", i, ir);
			fprintf(fp, "%s", buf);
		} else {
			printf("\n%04X: %04X    ", i, ir);
			printf("%s", buf);
		}
	}
	if (fp) { fclose(fp); }
}

// ---------------------------------------------------------------------
void saveImage() {
	char fn[32];
	sprintf(fn, "%s.bin", base_fn);
	FILE *fp = fopen(fn, "wb");
	if (fp) {
		fwrite(the_memory, 2, MEM_SZ, fp);
		fclose(fp);
	} else {
		printf(" ERROR: unable to open file '%s'", fn);
	}
}

// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
int main (int argc, char **argv)
{
	strcpy(base_fn, "j1");

	j1_init();
	COMMA(MAKE_JMP(0));

	load(base_fn);

	if (numWords) { the_memory[0] = MAKE_JMP(words[numWords-1].xt); }
	if (save_output) {
		doDisassemble(true);
		saveImage();
	}
	return exitStatus;
}
