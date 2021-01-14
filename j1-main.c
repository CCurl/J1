#include <winbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CELL_SZ 2
#define MEM_SZ 8192
#define WORD unsigned short
#define CELL WORD

#define bool int
#define true 1
#define false 0

void j1_init();
void j1_emu(CELL start);
extern WORD the_memory[];
extern WORD HERE;

char base_fn[32];
bool run_saved = true;
bool is_temp = false;
bool auto_run = false;

#define BUF_SZ 1024
char tib[BUF_SZ];
char *toIn;
FILE *input_fp = NULL;
FILE *input_stack[16];
int input_SP = 0;
int isBye = 0;

#define COMMA(val) the_memory[HERE++] = (val)
#define MAKE_LIT(val) (0x8000 | val)
#define MAKE_JMP(to)  (0x0000 | to)
#define MAKE_JMPZ(to) (0x2000 | to)
#define MAKE_CALL(to) (0x4000 | to)
#define MAKE_ALU(val) (0x6000 | val)

WORD curr_op = 0;

char peekChar() {
	return *(toIn);
}

char getChar() {
	return (*toIn) ? *(toIn++) : 0;
}

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

int getWord(char *word) {
	char ch = skipWS();
	int len = 0;
	while (ch > 0x20) {
		*(word++) = ch;
		ch = getChar();
	}
	*word = 0;
	return len;
}

int isNumber(char *word, WORD *value) {
	*value = 0;
	return false;
}

void parseWord(char *word) {
	WORD num = 0;
	if (isNumber(word, &num)) {
		COMMA(MAKE_LIT(num));
		return;
	}
	if (strcmp(word, "XXX") == 0) {
		// do something ...
		return;
	}
}

void parseLine(char *line) {
	char word[32];
	toIn = line;
	while (true) {
		int len = getWord(word);
		if (len) {
			if (strcmp(word, "\\") == 0) { return; }
			if (strcmp(word, "//") == 0) { return; }
			parseWord(word);
		} else {
			return;
		}
	}
}

void getLine(char *line) {
}

void doCompile() {
	COMMA(MAKE_LIT(0xFFFF));
	COMMA(MAKE_LIT(4));
	COMMA(MAKE_LIT(0));
	COMMA(MAKE_JMPZ(1));
	COMMA(MAKE_JMP(0));
}

// ---------------------------------------------------------------------
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
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
int main (int argc, char **argv)
{
	strcpy(base_fn, "j1");

	for (int i = 1; i < argc; i++) {
		char *cp = argv[i];
		if (*cp == '-') { parse_arg(++cp); }
	}

	j1_init();
	doCompile();
	j1_emu(0);

	printf("\nj1 exiting");
	return 0;
}
