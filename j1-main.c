#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "j1.h"

char base_fn[32];
bool save_output = true;
bool debug_flag = false;
int exitStatus = 0;
long maxCycles = 0;

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
char getChar() {
	return (*toIn) ? *(toIn++) : 0;
}

// ---------------------------------------------------------------------
char skipWS() {
	char ch = getChar();
	while (ch) {
		if (ch > 0x20) {
			return ch;
		} else {
			ch = getChar();
		}
	}
	return ch;
}

// ---------------------------------------------------------------------
int getWord(char *word) {
	char ch = skipWS();
	int len = 0;
	while (ch > 0x20) {
		*(word++) = ch;
		++len;
		ch = getChar();
	}
	*word = 0;
	return len;
}

// ---------------------------------------------------------------------
int isNumber(char *word, WORD *value) {
	int base = 10;
	bool isNeg = false;
	short num = 0;
	*value = 0;

	if ((strlen(word) == 3) && (*word == '\'') && (*(word+2) == '\'')) {
		*value = *(word+1);
		return true;
	}
	if (*word == '$') {
		base = 16;
		++word;
	}
	if (*word == '#') {
		base = 10;
		++word;
	}
	if (*word == '%') {
		base = 2;
		++word;
	}
	char ch = *(word++);
	while (ch) {
		short chNum = -1;
		if ((base == 2) && ('0' <= ch) && (ch <= '1')) {
			chNum = ch - '0';
		}
		if ((base == 10) && ('0' <= ch) && (ch <= '9')) {
			chNum = ch - '0';
		}
		if (base == 16) {
			if (('0' <= ch) && (ch <= '9')) {
				chNum = ch - '0';
			}
			if (('A' <= ch) && (ch <= 'F')) {
				chNum = ch - 'A' + 10;
			}
			if (('a' <= ch) && (ch <= 'f')) {
				chNum = ch - 'a' + 10;
			}
		}
		if (chNum < 0) { return false; }

		num = (num*base) + chNum;
		ch = *(word++);
	}
	if (isNeg) {
		num = -num;
	}
	*value = (WORD)num;
	return true;
}

// ---------------------------------------------------------------------
void defineWord(char *name) {
	DICT_T *p = &words[numWords++];
	strcpy(p->name, name);
	p->xt = HERE;
	p->flags = 0;
	p->len = 0;
	if (debug_flag) writePort_StringF("\nDefined [%s] at #%d", name, numWords);
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
	if (debug_flag) writePort_StringF("\n[%s] (HERE=%d), LAST_OP=%04X", word, HERE, LAST_OP);
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
			LAST_OP &= 0xEFFD;
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
			if (debug_flag) writePort_StringF("\nchanged op at %d to JMP", HERE-1);
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
			if (debug_flag) writePort_StringF("\nAdded %04X to ALU op at %d", (bitDecDSP|bitRtoPC), HERE-1);
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
	if (strcmp(word, "ALU") == 0) {
		if (debug_flag) writePort_StringF(" ALU->%d", HERE);
		op = opALU;
		COMMA(op);
		return;
	}
	if (strcmp(word, "T'<-T") == 0) {
		LAST_OP |= aluTgetsT;
		return;
	}
	if (strcmp(word, "T'<-N") == 0) {
		LAST_OP |= aluTgetsN;
		return;
	}
	if (strcmp(word, "T'<-R") == 0) {
		LAST_OP |= aluTgetsR;
		return;
	}
	if (strcmp(word, "T'<-(T+N)") == 0) {
		LAST_OP |= aluTplusN;
		return;
	}
	if (strcmp(word, "T'<-(TandN)") == 0) {
		LAST_OP |= aluTandN;
		return;
	}
	if (strcmp(word, "T'<-(TorN)") == 0) {
		LAST_OP |= aluTorN;
		return;
	}
	if (strcmp(word, "T'<-(TxorN)") == 0) {
		LAST_OP |= aluTxorN;
		return;
	}
	if (strcmp(word, "T'<-(notT)") == 0) {
		LAST_OP |= aluNotT;
		return;
	}
	if (strcmp(word, "T'<-(T=N)") == 0) {
		LAST_OP |= aluTeqN;
		return;
	}
	if (strcmp(word, "T'<-(T<N)") == 0) {
		LAST_OP |= aluTltN;
		return;
	}
	if (strcmp(word, "T'<-(N>>T)") == 0) {
		LAST_OP |= aluSHR;
		return;
	}
	if (strcmp(word, "T'<-(T-1)") == 0) {
		LAST_OP |= aluDecT;
		return;
	}
	if (strcmp(word, "T'<-[T]") == 0) {
		LAST_OP |= aluFetch;
		return;
	}
	if (strcmp(word, "T'<-(N<<T)") == 0) {
		LAST_OP |= aluSHL;
		return;
	}
	if (strcmp(word, "T'<-depth") == 0) {
		LAST_OP |= aluDepth;
		return;
	}
	if (strcmp(word, "T'<-(Nu<T)") == 0) {
		LAST_OP |= aluNuLtT;
		return;
	}
	if (strcmp(word, "N->[T]") == 0) {
		LAST_OP |= bitStore;
		return;
	}
	if (strcmp(word, "R->PC") == 0) {
		LAST_OP |= bitRtoPC;
		return;
	}
	if (strcmp(word, "T->N") == 0) {
		LAST_OP |= bitTtoN;
		return;
	}
	if (strcmp(word, "T->R") == 0) {
		LAST_OP |= bitTtoR;
		return;
	}
	if (strcmp(word, "++RSP") == 0) {
		LAST_OP |= bitIncRSP;
		return;
	}
	if (strcmp(word, "--RSP") == 0) {
		LAST_OP |= bitDecRSP;
		return;
	}
	if (strcmp(word, "++DSP") == 0) {
		LAST_OP |= bitIncDSP;
		return;
	}
	if (strcmp(word, "--DSP") == 0) {
		LAST_OP |= bitDecDSP;
		return;
	}
	if (strcmp(word, ">r") == 0) {
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
	writePort_StringF("\nERROR: unknown word: [%s]\n", word);
	exitStatus = 1;
}

// ---------------------------------------------------------------------
void parseLine(char *line) {
	char word[32];
	toIn = line;
	while (true) {
		int len = getWord(word);
		// writePort_StringF("[%s]", word);
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
void doDisassemble(bool toFile) {
	FILE *fp = NULL;
	if (toFile) {
		char fn[32];
		sprintf(fn, "%s.lst", base_fn);
		fp = fopen(fn, "wt");
		if (!fp) {
			writePort_StringF("\nUnable to create listing file '%s'.", fn);
			return;
		}
		fprintf(fp, "; HERE: 0x%04X (%d)\n", HERE, HERE);
	}

	char buf[256];
	for (int i = 0; i < numWords; i++) {
		DICT_T *p = &words[i];
		sprintf(buf, "; %2d: XT: %04X, Len: %2d, Flags: %02X, Name: %s\n", i,
			p->xt, p->len, p->flags, p->name);
		fp ? fprintf(fp, "%s", buf) : writePort_String(buf);
	}

	for (int i = 0; i < HERE; i++) {
		WORD ir = the_memory[i];
		disIR(ir, buf);
		if (fp) {
			fprintf(fp, "\n%04X: %04X    ", i, ir);
			fprintf(fp, "%s", buf);
		} else {
			writePort_StringF("\n%04X: %04X    ", i, ir);
			writePort_String(buf);
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
		fwrite(the_memory, 1, MEM_SZ, fp);
		fclose(fp);
	} else {
		writePort_StringF(" ERROR: unable to open file '%s'", fn);
	}
}

// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
void parse_arg(char *arg) 
{
	if (*arg == 'f') strcpy(base_fn, arg+2);
	if (*arg == 't') save_output = false;
	if (*arg == 'd') debug_flag  = true;
	if (*arg == 'c') {
		char buf[24];
		strcpy(buf, arg+2);
		maxCycles = atol(buf);
	}

	if (*arg == '?') {
		writePort_StringF("usage: j1 [options]\n");
		writePort_StringF("\t -f:baseFn     (default: 'j1')\n");
		writePort_StringF("\t -t     (temp:  default: false)\n");
		writePort_StringF("\t -d     (debug: default: false)\n");
		writePort_StringF("\t -c:maxCycles  (default: 0)\n");
		writePort_StringF("\nNotes ...");
		writePort_StringF("\n\n    -f:baseFn defines the base filename for the files in the working set.");
		writePort_StringF(  "\n    -t identfies that J1 should not write a .LST or .BIN file");
		writePort_StringF(  "\n    -c limits the number of CPU cycles, 0 => unlimited");

		exit(0);
	}
}

// ---------------------------------------------------------------------
int main (int argc, char **argv)
{
	strcpy(base_fn, "j1");

	for (int i = 1; i < argc; i++) {
		char *cp = argv[i];
		if (*cp == '-') { parse_arg(++cp); }
	}

	j1_init();
	COMMA(MAKE_JMP(0));

	char fn[32];
	sprintf(fn, "%s.src", base_fn);
	FILE *fp = fopen(fn, "rt");
	if (fp) {
		doCompile(fp);
		fclose(fp);
	} else {
		writePort_StringF("ERROR: unable to open '%s'", fn);
		return 1;
	}

	if (numWords) { the_memory[0] = MAKE_JMP(words[numWords-1].xt); }
	if (save_output) { doDisassemble(true); }

	j1_emu(0, maxCycles);

	if (debug_flag) {
		doDisassemble(false);
		writePort_String("\ndata stack: ");
		dumpStack(DSP, dstk);
		writePort_String("\nreturn stack: ");
		dumpStack(RSP, rstk);
	} 

	if (save_output) { saveImage(); }
	return exitStatus;
}
