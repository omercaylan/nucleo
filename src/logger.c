#include "logger.h"

#define bufEND (buffer + loggerBUFFER_SIZE)

int bufCapacity();
void cleanStringAt(char *ptr);

static char __attribute__ ((section(".logger"))) buffer[loggerBUFFER_SIZE];
// Pointer to the current position in the buffer where the next string will be written.
static char *bufCursor;

void loggerInit()
{
	// Clean the buffer.
	for (bufCursor = buffer; bufCursor < bufEND; bufCursor++) {
		*bufCursor = 0;
	}
	bufCursor = buffer;
}

void loggerPrintf(const char *format, ...)
{
	// Print to the buffer.
	va_list args;
	va_start(args, format);
	// n is number of characters that would be printed, without \0
	int n = vsnprintf(bufCursor, bufCapacity(), format, args);
	va_end(args);

	if (bufCapacity() >= (n + 1)) {
		// If the string fit the buffer, then update the pointer, clean next
		// string, update pointer if needed and return.
		bufCursor += n + 1;
		cleanStringAt(bufCursor);
		return;
	}
	// The string doesn't fit. Clean the printed string and print string at the
	// beginning of the buffer.
	cleanStringAt(bufCursor);
	bufCursor = buffer;

	va_start(args, format);
	n = vsnprintf(bufCursor, bufCapacity(), format, args);
	va_end(args);

	// If the string doesn't fit then tough luck.
	bufCursor += n + 1;
	cleanStringAt(bufCursor);
}

void cleanStringAt(char *bufPtr)
{
	for (; bufPtr < bufEND; bufPtr++) {
		if (*bufPtr == '\0') {
			break;
		}
		*bufPtr = '\0';
	}
}

// Remaining space in the buffer.
int bufCapacity()
{
	return bufEND - bufCursor;
}
