// Libraries
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")

// Standard includes
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <memory>
#include <chrono>
#include <mutex>
#include <iomanip>
#include <ctime>

// OS-Specific Includes
#include <gl/gl3w.h>
#include <Windows.h>
#include <tlhelp32.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <gl/glcorearb.h>

// Third party includes
#include <Minhook.h>
#include "imgui.h"
#include "imgui_impl.h"
#include <zmq.hpp>
#include <json/json.h>
#include <glm/common.hpp>
#include <glm/matrix.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include "lodepng.h"

// First party includes :)
#include <PlayerData.h>
#include "game_value.h"
#include "game_render.h"
#include "debug.h"
#include <Communication.h>

#pragma comment(lib, "ws2_32.lib")

// Define types for our functions we wish to detour
typedef BOOL(WINAPI *td_wglSwapBuffers)(int*);

// Pointers for calling original functions.
td_wglSwapBuffers og_wglSwapBuffers = NULL;

// Overridden functions
int WINAPI DetourWglSwapBuffers(int* context);

// Function that is run on our client-server thread
void ClientThread();

// Our dll's main function which is run when it is injected, and when NMS quits.
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved);

int overlayOpenState = 0;
int lastOverlayOpenKeyState = 0;
int w = 0, h = 0;
int overlayX = 250, overlayY = 350;

// Keycodes for the overlay
// TODO: move to game_control.h
bool	 prevKeyStates[]{ false, false, false, false, false, false, false, false, false, false, false, false };
bool	  keyStates[] = { false, false, false, false, false, false, false, false, false, false, false, false };
int	        vkCodes[] = { 0x30,  0x31,  0x32,  0x33,  0x34,  0x35,  0x36,  0x37,  0x38,  0x39,  VK_OEM_PERIOD , VK_OEM_1 };
char       keyChars[] = {'0',	 '1',	'2',   '3',   '4',   '5',   '6',   '7',   '8',   '9',   '.', ':'};
int numKeys = 12;
int	        vkCodesControl[] = {  VK_BACK };
bool      keyControlStates[] = { false };
bool  prevKeyControlStates[] = { false };
int numControlKeys = 1;

//Game Variables, Memory addresses, and other working variables
float playerPosition[3];
float playerPositionLast[3];

// Used for working out velocity and display framerate
std::chrono::milliseconds deltaTime;
std::chrono::milliseconds currentTime;

// Other various small funcs
float LinearDist3f(float* a, float* b) {
	return std::sqrtf(std::powf(a[0] - b[0], 2.0f) + std::powf(a[1] - b[1], 2.0f) + std::powf(a[2] - b[2], 2.0f));
}

// From https://www.khronos.org/opengl/wiki/Load_OpenGL_Functions#Function_Prototypes
void *GetAnyGLFuncAddress(const char *name)
{
	void *p = (void *)wglGetProcAddress(name);
	if (p == 0 ||
		(p == (void*)0x1) || (p == (void*)0x2) || (p == (void*)0x3) ||
		(p == (void*)-1))
	{
		HMODULE module = LoadLibraryA("opengl32.dll");
		p = (void *)GetProcAddress(module, name);
	}

	return p;
}
