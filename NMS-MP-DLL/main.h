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

// First party includes :)
#include <PlayerData.h>

static PlayerData me;

#define DO_OVERLAY // Render the overlay
#define DO_SERVER // Init the client-server backend

// Functions to retrieve various bits from the game memory, ie. the juicy stuff.
void getCurrentPlayerPosition(float* in);
void getCurrentRegionName(char* out_regName);

// Define types for our functions we wish to detour
typedef BOOL(WINAPI *td_wglSwapBuffers)(int*);
typedef BOOL(WINAPI *td_glDrawArrays)(GLenum, GLint, GLsizei);

// Pointers for calling original functions.
td_wglSwapBuffers og_wglSwapBuffers = NULL;
td_glDrawArrays og_glDrawArrays = NULL;

// Overridden functions
int WINAPI DetourWglSwapBuffers(int* context);
int WINAPI DetourGlDrawArrays(GLenum mode, GLint first, GLsizei count);
bool haveRenderedInWorldThisFrame = false;

// Client-Server variables
HANDLE t_handle;
HANDLE t_mouse;
HHOOK mousehook;
bool connectStatus = false;
std::vector<PlayerData> others;
std::mutex mtx;

// So we don't end up with accessing modified vectors
void safeSetOthers(std::vector<PlayerData> newOthers);
std::vector<PlayerData> safeGetOthers();

// Function that is run on our client-server thread
DWORD WINAPI ClientThread(LPVOID);

// Our dll's main function which is run when it is injected, and when NMS quits.
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved);

//Overlay variables
void drawOverlay();
int overlayOpenState = 0;
int lastOverlayOpenKeyState = 0;
HDC nms_window;

//Game Variables, Memory addresses, and other working variables
HANDLE nmsHandle;
__int64* nmsAddr;
char nmsExeName[] = "NMS.exe";
__int64 posXOffs = 0x1907608;
__int64 playerSystemBase = 0x01D9D1D0;
__int64 psOffsets[5] = { 0x658, 0x4B0, 0x5D8, 0x460, 0x350 };
int psNumOffsets = 5;
int playerSystemPtr = 0;
char playerSystemName[30] = "";
float playerPosition[3];
float playerPositionLast[3];
int mousePos[2] = { 0,0 };
int wWidth = 0, wHeight = 0;

// Used for working out velocity and display framerate
std::chrono::milliseconds deltaTime;
std::chrono::milliseconds currentTime;

// Other various small funcs
float LinearDist3f(float* a, float* b) {
	return std::sqrtf(std::powf(a[0] - b[0], 2.0f) + std::powf(a[1] - b[1], 2.0f) + std::powf(a[2] - b[2], 2.0f));
}
