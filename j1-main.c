#include <winbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define CELL_SZ 2
#define MEM_SZ 8192
#define WORD unsigned short
#define CELL WORD

#define bool int
#define true 1
#define false 0

void j1_init();
void j1_emu(CELL start);

char base_fn[32];
bool run_saved = true;
bool is_temp = false;
bool auto_run = false;

#define BUF_SZ 1024
char input_buf[BUF_SZ];
FILE *input_fp = NULL;
FILE *input_stack[16];
int input_SP = 0;
int isBye = 0;


// ---------------------------------------------------------------------
void parse_arg(char *arg) 
{
	// -f:baseFn
	if (*arg == 'f') strcpy(base_fn, arg+2);

	// -b (bootstrap)
	if (*arg == 'b') run_saved = false;

	// -t (is-temp)
	if (*arg == 't') is_temp = true;

	// -a (auto-run)
	if (*arg == 'a') auto_run = true;

	if (*arg == '?')
	{
		printf("usage: mforth [options]\n");
		printf("\t-f:baseFn (default: 'mforth')\n");
		printf("\t-b        (bootstrap, default: false)\n");
		printf("\t-t        (is-temp,   default: false)\n");
		printf("\t-a        (auto-run,  default: false)\n");
		printf("\nNotes ...");
		
		printf("\n\n    -f:baseFn defines the base filename for the files in the working set.");
		printf("\nThis is used by the other options.");

		printf("\n\n    -b tells mforth to start with an empty dictionary. Then mforth");
		printf("\nreads the <baseFn>.SRC file as initial input. You can define an empty");
		printf("\n<baseFn>.SRC file to drop into the REPL with no words at all. I have");
		printf("\nprovided the empty.SRC file for that purpose. Use 'mforth -f:empty -b'");

		printf("\n\n    -t tells mforth that it should not save its state when closing. By");
		printf("\ndefault, on exit (bye), mforth will create a set of files <baseFn>.[TXT|INF|BIN]");
		printf("\nthat allow mforth to remember the current dictionary the next time it is run.");

		printf("\n\n    -a tells mforth to automatically run the last word in the working set");
		printf("\ndictionary and then exit. In this case, -t is automatically set to true. One would");
		printf("\nuse this option to run a stand-alone program.");
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

    for (int i = 1; i < argc; i++)
    {
        char *cp = argv[i];
        if (*cp == '-') parse_arg(++cp);
    }

	j1_init();
	j1_emu(0);

	printf("j1 exiting");
    return 0;
}
