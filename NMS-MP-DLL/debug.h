#pragma once
#include <Windows.h>
#include <string>

std::string debug_last_error;

void display_error(const char* errorStr, bool important) {
	if (important) {
		MessageBox(NULL, errorStr, "Error", MB_OK);
	}
	debug_last_error = std::string(errorStr);
}