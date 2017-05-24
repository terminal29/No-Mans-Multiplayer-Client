#pragma once
#include <Windows.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//			Namespace for storing all game memory addresses & functions that return game variables
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace game_value
{
	bool has_init = false;

	char game_exe_name[8] = "NMS.exe";
	HDC game_hdc;
	HWND game_hwnd;
	HANDLE game_handle;
	__int64* game_begin_address;

	__int64 x_position_base_address = 0x1907608;

	int system_pointer_num_offset = 5;
	__int64 system_base_address = 0x01D9D1D0;
	__int64 system_pointer_offsets[5] = { 0x658, 0x4B0, 0x5D8, 0x460, 0x350 };

	static void init() {
		game_begin_address = (__int64*)GetModuleHandle(NULL);
		game_handle = GetCurrentProcess();
		game_hdc = wglGetCurrentDC();
		game_hwnd = WindowFromDC(game_hdc);
		has_init = true;
	}
	static void get_player_position(float* position_out) {
		if (!has_init)
			return;
		float p[3];

		memcpy(&p[0], (float*)((char*)game_begin_address + x_position_base_address), sizeof(float));
		memcpy(&p[1], (float*)((char*)game_begin_address + x_position_base_address + sizeof(float)), sizeof(float));
		memcpy(&p[2], (float*)((char*)game_begin_address + x_position_base_address + 2 * sizeof(float)), sizeof(float));

		memcpy(position_out, &p[0], sizeof(p));
	}
	static void get_player_rotation_vectors(float* rotation_vectors_out) {
		if (!has_init)
			return;
		float p[9];
		//13,14,15 16,17,18 19,20,21

		memcpy(&p[0], (float*)((char*)game_begin_address + x_position_base_address + 13 * sizeof(float)), sizeof(float));
		memcpy(&p[1], (float*)((char*)game_begin_address + x_position_base_address + 14 * sizeof(float)), sizeof(float));
		memcpy(&p[2], (float*)((char*)game_begin_address + x_position_base_address + 15 * sizeof(float)), sizeof(float));

		memcpy(&p[3], (float*)((char*)game_begin_address + x_position_base_address + 16 * sizeof(float)), sizeof(float));
		memcpy(&p[4], (float*)((char*)game_begin_address + x_position_base_address + 17 * sizeof(float)), sizeof(float));
		memcpy(&p[5], (float*)((char*)game_begin_address + x_position_base_address + 18 * sizeof(float)), sizeof(float));

		memcpy(&p[6], (float*)((char*)game_begin_address + x_position_base_address + 19 * sizeof(float)), sizeof(float));
		memcpy(&p[7], (float*)((char*)game_begin_address + x_position_base_address + 20 * sizeof(float)), sizeof(float));
		memcpy(&p[8], (float*)((char*)game_begin_address + x_position_base_address + 21 * sizeof(float)), sizeof(float));

		memcpy(rotation_vectors_out, &p[0], sizeof(p));
	}
	static void get_player_region(char* region_name_out) {
		if (!has_init)
			return;
		int* sys_address;
		int sys_name_length = 30;
		__try {
			memcpy(&sys_address, (void*)((char*)game_begin_address + system_base_address), sizeof(sys_address));
			memcpy(&sys_address, (void*)((char*)sys_address + system_pointer_offsets[0]), sizeof(sys_address));
			memcpy(&sys_address, (void*)((char*)sys_address + system_pointer_offsets[1]), sizeof(sys_address));
			memcpy(&sys_address, (void*)((char*)sys_address + system_pointer_offsets[2]), sizeof(sys_address));
			memcpy(&sys_address, (void*)((char*)sys_address + system_pointer_offsets[3]), sizeof(sys_address));
			memcpy(region_name_out, (char*)((char*)sys_address + system_pointer_offsets[4]), sizeof(char) * (sys_name_length - 1));
			region_name_out[sys_name_length - 1] = '\0';
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			strcpy(region_name_out, "nil");
		}
	}

	

}