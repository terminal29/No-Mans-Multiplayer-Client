#pragma once
#include <windows.h>
#include <gl/gl3w.h>
#include <imgui.h>
#include <string>
#include <sstream>
#include <chrono>

IMGUI_API bool        ImGui_Init(HDC hdc);
IMGUI_API void        ImGui_Shutdown();
IMGUI_API void        ImGui_NewFrame();

static GLuint       g_FontTexture = 0;
static int          g_ShaderHandle = 0, g_VertHandle = 0, g_FragHandle = 0;
static int          g_AttribLocationTex = 0, g_AttribLocationProjMtx = 0;
static int          g_AttribLocationPosition = 0, g_AttribLocationUV = 0, g_AttribLocationColor = 0;
static unsigned int g_VboHandle = 0, g_VaoHandle = 0, g_ElementsHandle = 0;
static HDC window_HDC;
static long double g_time = 0;