// Support for project-specific IO ports

#include <stdio.h>

#define WORD unsigned short
#define emitPort 1
#define dotPort 2

// ---------------------------------------------------------------------
void writePort(WORD portNum, WORD val) {
	portNum = (portNum & 0x0FFF);
	if (portNum == emitPort) { printf("%c", val); }
	if (portNum == dotPort)  { printf(" %d", val); }
}

// ---------------------------------------------------------------------
WORD readPort(WORD portNum) {
	portNum = (portNum & 0x0FFF);
    printf("WARN: readPort(0x%04x) not implemented.", portNum);
    return 0;
}

