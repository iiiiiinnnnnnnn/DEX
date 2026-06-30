#include <windows.h>
#include <stdio.h>
#include <iostream>

#include "Logger.h"

// ƒƒOo—Í
void Logger::Print(const char* format, ...)
{
	char message[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(message, sizeof(message), format, args);
	va_end(args);

	//OutputDebugStringA(message);
	std::cout << message << std::endl;
}
