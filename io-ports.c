// Support for project-specific IO ports

#include <stdio.h>
#include <stdarg.h>

#ifndef emitPort
#define emitPort   1
#define dotPort    2
#endif

// ---------------------------------------------------------------------
void writePort_String(const char *str);
void writePort_StringF(const char *fmt, ...);

// ---------------------------------------------------------------------
unsigned short readPort(unsigned short portNum) {
	portNum = (portNum & 0x0FFF);
    writePort_StringF("WARN: readPort(0x%04x) not implemented.", portNum);
    return 0;
}

// ---------------------------------------------------------------------
void writePort(unsigned short portNum, unsigned short val) {
	portNum = (portNum & 0x0FFF);
	if (portNum == emitPort) { putc(val, stdout); }
	if (portNum == dotPort)  { writePort_StringF(" %d", val); }
}

// ---------------------------------------------------------------------
void writePort_String(const char *str)
{
	while (*str) {
		writePort(emitPort, *(str++));
	}
}

// ---------------------------------------------------------------------
void writePort_StringF(const char *fmt, ...)
{
	char buf[64];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
	writePort_String(buf);
}
