#include <winbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "j1.h"

char base_fn[32];
bool run_saved = true;
bool is_temp = false;
bool auto_run = false;

#define LAST_OP the_memory[HERE-1]

#define TIB_SZ 1024
char tib[TIB_SZ];
char *toIn;
FILE *input_fp = NULL;
FILE *input_stack[16];
int input_SP = 0;
int isBye = 0;

DICT_T words[256];
int numWords = 0;

WORD STATE = 0;

// ---------------------------------------------------------------------
char peekChar() {
	return *(toIn);
}

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

void defineWord(char *name) {
	DICT_T *p = &words[numWords++];
	strcpy(p->name, name);
	p->xt = HERE;
	p->flags = 0;
	p->len = strlen(name);
	printf("\nDefined [%s] at #%d", name, numWords);
}

// ---------------------------------------------------------------------
DICT_T *findWord(char *word) {
	for (int i = 0; i< numWords; i++) {
		if (strcmp(words[i].name, word) == 0) {
			return &words[i];
		}
	}
	return NULL;
}

// ---------------------------------------------------------------------
void parseWord(char *word) {
	printf("\n[%s] (HERE=%d), LAST_OP=%04X", word, HERE, LAST_OP);
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
		op = MAKE_CALL(w->xt);
		COMMA(op);
		return;
	}
	if (strcmp(word, ":") == 0) {
		getWord(word);
		defineWord(word);
		STATE = 1;
		return;
	}
	if (strcmp(word, ";") == 0) {
		STATE = 0;
		// Change last operation to JMP if CALL
		if ((LAST_OP & opLIT) == 0) {
			if ((LAST_OP & 0x6000) == opCALL) {
				LAST_OP = (LAST_OP & 0x1FFF) | opJMP;
				printf("\nchanged op at %d to JMP", HERE-1);
				return;
			}
		}
		bool canAddRet = true;
		if ((LAST_OP & opLIT) != 0)      canAddRet = false; // not LITERAL
		if ((LAST_OP & 0x6000) != opALU) canAddRet = false; // not ALU
		if ((LAST_OP & bitRtoPC) != 0)   canAddRet = false; // R->PC already set
		if ((LAST_OP & bitIncRSP) != 0)  canAddRet = false; // R++ set
		if ((LAST_OP & bitDecRSP) != 0)  canAddRet = false; // R-- already set
		if (canAddRet) {
			LAST_OP |= bitRtoPC;
			LAST_OP |= bitDecRSP;
			printf("\nAdded %04X to ALU op at %d", (bitDecDSP|bitRtoPC), HERE-1);
			return;
		}
		// cannot include in previous op :(
		op = opALU;
		op |= bitRtoPC;
		op |= bitDecRSP;
		COMMA(op);
		return;
	}
	if (strcmp(word, "ALU") == 0) {
		printf(" ALU->%d", HERE);
		op = opALU;
		COMMA(op);
		return;
	}
	if (strcmp(word, "T<-N") == 0) {
		LAST_OP |= aluTgetsN;
		return;
	}
	if (strcmp(word, "T<-T") == 0) {
		LAST_OP |= aluTgetsT;
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
	if (strcmp(word, "XXX") == 0) {
		// do something ...
		return;
	}
	printf("\nERROR: unknown word: [%s]\n", word);
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
void getLine(char *line) {
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
			printf("\nUnable to create listing file '%s'.", fn);
			return;
		}
		fprintf(fp, "; HERE: %04X\n", HERE);
		printf("\nWriting listing file '%s' ...", fn);
	}

	char buf[256];
	for (int i = 0; i < numWords; i++) {
		DICT_T *p = &words[i];
		sprintf(buf, "; %2d: XT: %04X, Len: %2d, Flags: %02X, Name: %s\n", i,
			p->xt, p->len, p->flags, p->name);
		fp ? fprintf(fp, "%s", buf) : printf("%s", buf);
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
	if (fp) {
		fclose(fp);
	}
}

// ---------------------------------------------------------------------
void saveImage() {
	char fn[32];
	sprintf(fn, "%s.bin", base_fn);
	printf("\nWriting image to %s ...", fn);
	FILE *fp = fopen(fn, "wb");
	if (fp) {
		fwrite(the_memory, 1, memory_size, fp);
		fclose(fp);
		printf(" done");
	} else {
		printf(" ERROR: unable to open file");
	}
}

// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
void parse_arg(char *arg) 
{
	// -f:baseFn
	if (*arg == 'f') strcpy(base_fn, arg+2);

	// -a (auto-run)
	if (*arg == 'a') auto_run = true;

	if (*arg == '?') {
		printf("usage: j1 [options]\n");
		printf("\t-f:baseFn (default: 'j1')\n");
		printf("\nNotes ...");
		printf("\n\n    -f:baseFn defines the base filename for the files in the working set.");

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

	// disIR(MAKE_LIT(66), NULL);
	// disIR(MAKE_JMP(67), NULL);
	// disIR(MAKE_JMPZ(68), NULL);
	// disIR(MAKE_CALL(69), NULL);
	// disIR(0x7FFF, NULL);

	j1_init();
	COMMA(MAKE_JMP(0));

	char fn[32];
	sprintf(fn, "%s.src", base_fn);
	FILE *fp = fopen(fn, "rt");
	if (fp) {
		doCompile(fp);
		fclose(fp);
	}

	the_memory[0] = MAKE_JMP(words[numWords-1].xt);
	doDisassemble(true);
	j1_emu(0, 20);

	printf("\ndata stack:");
	dumpStack(DSP, dstk);
	printf("\nreturn stack:");
	dumpStack(RSP, rstk);

	saveImage();
	printf("\nexiting");
	return 0;
}
