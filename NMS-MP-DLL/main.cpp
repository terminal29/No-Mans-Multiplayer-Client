
#include "main.h"

void getCurrentPlayerPosition(float* out_floatArr) {
	float p[3];

	memcpy(&p[0], (float*)((char*)nmsAddr + posXOffs), sizeof(float));
	memcpy(&p[1], (float*)((char*)nmsAddr + posXOffs + sizeof(float)), sizeof(float));
	memcpy(&p[2], (float*)((char*)nmsAddr + posXOffs + 2*sizeof(float)), sizeof(float));

	memcpy(out_floatArr, &p[0], sizeof(p));
}

void getCurrentRegionName(char* out_sysName) {
	int* sysAddr;
	int sysNameLength = 30;
	__try {
		memcpy(&sysAddr, (void*)((char*)nmsAddr + playerSystemBase), sizeof(sysAddr));
		memcpy(&sysAddr, (void*)((char*)sysAddr + psOffsets[0]), sizeof(sysAddr));
		memcpy(&sysAddr, (void*)((char*)sysAddr + psOffsets[1]), sizeof(sysAddr));
		memcpy(&sysAddr, (void*)((char*)sysAddr + psOffsets[2]), sizeof(sysAddr));
		memcpy(&sysAddr, (void*)((char*)sysAddr + psOffsets[3]), sizeof(sysAddr));
		memcpy(out_sysName, (char*)((char*)sysAddr + psOffsets[4]), sizeof(char) * (sysNameLength - 1));
		out_sysName[sysNameLength - 1] = '\0';

		// This is here because while the game is loading in the system name is junk so we need to limit it ourselves or it will never terminate.
	}__except(EXCEPTION_EXECUTE_HANDLER){
		// Name probably hasn't been loaded yet. User began the execution at the difficulty select screen, the bastard!
		strcpy(out_sysName, "nil");
	}
}

void safeSetOthers(std::vector<PlayerData> newOthers) {
	std::lock_guard<std::mutex> lock(mtx);
	others = newOthers;
}

std::vector<PlayerData> safeGetOthers() {
	std::lock_guard<std::mutex> lock(mtx);
	std::vector<PlayerData> othersCopy = others;
	return othersCopy;
}

DWORD WINAPI ClientThread(LPVOID)
{
#ifdef DO_SERVER

	// Attempt to connect to the server
	me.setName("InfiniteLlamas"); // TODO: Replace this with their selection
	zmq::context_t context(1);
	zmq::socket_t socket(context, ZMQ_REQ);
	std::cout << "Connecting to server on tcp://127.0.0.1:5258" << std::endl;
	socket.connect("tcp://127.0.0.1:5258");
	while (true) {
		Sleep(50);
		// Send a copy of our PlayerData to the server
		std::string outgoingString = me.getJSONString();
		zmq::message_t outgoing(outgoingString.size());
		memcpy(outgoing.data(), outgoingString.c_str(), outgoingString.size());
		socket.send(outgoing);

		// Get the reply.
		zmq::message_t incoming;
		socket.recv(&incoming);
		std::string incomingString = std::string(static_cast<char*>(incoming.data()), incoming.size());
		connectStatus = true;

		// Parse the reply
		Json::Reader reader;
		Json::Value jResponse;
		bool parseResult = reader.parse(incomingString, jResponse);
		if (parseResult) {
			if (jResponse.isMember("nil")) { 
				//The last message we sent was invalid for whatever reason
			}
			else if (jResponse.isMember("you") && jResponse.isMember("others")) {
				if (me.getId() != jResponse["you"]["id"].asInt()) { 
					//Server has assigned us a new id
					std::cout << "My new ID is " << jResponse["you"]["id"].asInt() << std::endl;
					me.setId(jResponse["you"]["id"].asInt());
				}
				// TODO: optimise this
				std::vector<PlayerData> players;
				Json::Value jPlayers = jResponse["others"];
				for (Json::ValueIterator iter = jPlayers.begin(); iter != jPlayers.end(); iter++) {
					PlayerData otherPlayer;
					int parseResult = PlayerData::fromJSONString(&otherPlayer, iter->toStyledString());
					if (parseResult) {
						players.push_back(otherPlayer);
					}
				}
				safeSetOthers(players);
			}
			else {
				// We connected to the wrong server? idk this should never happen
			}
		}
		else {
			// Do nothing. The server's response is invalid, so just recycle last frames local list.
		}
	}
#endif DO_SERVER
	return 1;
}

bool _overlayIsInit = false;
void drawOverlay() {

	if (!_overlayIsInit) {
		int g = gl3wInit();
		if (g != 0) {
			std::stringstream l;
			l << "Gl3w failed to init. Overlay probably will not render or your game will crash. Err: " << g;
			MessageBoxA(NULL, l.str().c_str(), "Error", MB_OK);
		}
		nms_window = wglGetCurrentDC();
		ImGui_Init(nms_window);
		_overlayIsInit = true;
	}
	// Initialize gl3w
	
		ImGuiIO& io = ImGui::GetIO();
		io.WantCaptureMouse = true;
		io.MouseDrawCursor = true;
		
		HWND window = WindowFromDC(nms_window);
				
		RECT windowRect;
		GetWindowRect(window, &windowRect);
		wWidth = windowRect.right - windowRect.left;
		wHeight = windowRect.bottom - windowRect.top;

				

		io.MousePos = ImVec2(50, 50);
			



	// Create a new frame
	ImGui_NewFrame();
	ImGui::Begin("No Man's Multiplayer");
	ImGui::SetWindowSize({ 0,0 });

	// If the client thread is disabled
#ifndef DO_SERVER
	ImGui::TextColored({ 1,0,0,1 }, "Client thread disabled");
#else
	// Status Display
	std::stringstream cStatus;
	cStatus << "Connection Status [" << (connectStatus ? "Connected]" : "Disconnected]") << std::endl;
	ImGui::TextColored({ 1,1,1,1 }, cStatus.str().c_str());
	ImGui::Spacing();

	// Players Display
	ImGui::TextColored({ 1,0,0,1 }, "Connected Players");
	std::vector<PlayerData> currentOthers = safeGetOthers();
	std::stringstream othersString;
	for (int i = 0; i < currentOthers.size(); i++) {
		othersString << currentOthers.at(i).getName() << std::endl;
		othersString << " " << currentOthers.at(i).getRegion() << std::endl;
		if (currentOthers.at(i).getRegion() == me.getRegion()) {
			othersString << " " << static_cast<int>(LinearDist3f(me.getPos(), currentOthers.at(i).getPos())) << " units away" << std::endl;
		}
	}
	ImGui::Text(othersString.str().c_str());
	ImGui::Spacing();


#endif DO_SERVER

	// Region Display
	ImGui::TextColored({ 1,0,1,1 }, "Current Region");
	ImGui::Text(me.getRegion().c_str());
	ImGui::Spacing();
	
	// Position Display
	std::stringstream m;
	m << static_cast<int>(me.getPos()[0]) << ":" << static_cast<int>(me.getPos()[1]) << ":" << static_cast<int>(me.getPos()[2]);
	ImGui::TextColored({ 0,1,1,1 }, "Position (x:y:z)");
	ImGui::Text(m.str().c_str());
	ImGui::Spacing();
	
	// Velocity Display
	m.flush();
	m.str("");
	ImGui::TextColored({ 1,1,0,1 }, "Linear Velocity");
	float LinearVel = LinearDist3f(playerPosition, playerPositionLast) / ((float)deltaTime.count() / 1000);
	m << static_cast<int>(LinearVel) << " units/sec";
	ImGui::Text(m.str().c_str());
	ImGui::Spacing();

	// Debug
	m.flush();
	m.str("");
	ImGui::TextColored({ 1,1,0,1 }, "Debug");
	m << mousePos[0] << ":" << mousePos[1] << std::endl;
	m << wWidth << ":" << wHeight << std::endl;
	ImGui::Text(m.str().c_str());

	
	// Draw Overlay
	ImGui::End();
	ImGui::Render();
}

int WINAPI DetourWglSwapBuffers(int* context)
{
	// Toggle overlay when we press F1
	if (GetAsyncKeyState(VK_F1) != lastOverlayOpenKeyState) {
		lastOverlayOpenKeyState = GetAsyncKeyState(VK_F1);
		if (lastOverlayOpenKeyState != 0) { 
			overlayOpenState = !overlayOpenState;
		}
	}


	if (GetAsyncKeyState(VK_F2) != 0) {

		glUseProgram(0);

		GLchar* vertexsource = "#version 150	\
		in  vec3 in_Position;						\
		in  vec3 in_Color;							\
		out vec3 ex_Color;							\
		void main(void) {							\
			gl_Position = gl_ModelViewProjectionMatrix * vec4(in_Position.x, in_Position.y, in_Position.z, 1.0);	\
			ex_Color = in_Color;					\
		}";

		GLchar* fragmentsource = "#version 150	\
			precision highp float;					\
													\
		in  vec3 ex_Color;							\
		out vec4 gl_FragColor;						\
													\
		void main(void) {							\
			gl_FragColor = vec4(ex_Color, 1.0);		\
		}";

		/* We're going to create a simple diamond made from lines */
		const GLfloat diamond[4][3] = {
			{ 0.0, 100.0, 0.0}, /* Top point */
			{ 100.0,  0.0, 0.0}, /* Right point */
			{ 0.0, -100.0, 0.0}, /* Bottom point */
			{ -100.0,  0.0 , 0.0} }; /* Left point */

		const GLfloat colors[4][3] = {
			{ 1.0,  0.0,  0.0 }, /* Red */
			{ 0.0,  1.0,  0.0 }, /* Green */
			{ 0.0,  0.0,  1.0 }, /* Blue */
			{ 1.0,  1.0,  1.0 } }; /* White */


		int i; /* Simple iterator */
		GLuint vao, vbo[2]; /* Create handles for our Vertex Array Object and two Vertex Buffer Objects */
								   /* These pointers will receive the contents of our shader source code files */

		/* These are handles used to reference the shaders */
		GLuint vertexshader, fragmentshader;

		/* This is a handle to the shader program */
		GLuint shaderprogram;

		/* Allocate and assign a Vertex Array Object to our handle */
		glGenVertexArrays(1, &vao);

		/* Bind our Vertex Array Object as the current used object */
		glBindVertexArray(vao);

		/* Allocate and assign two Vertex Buffer Objects to our handle */
		glGenBuffers(2, vbo);

		/* Bind our first VBO as being the active buffer and storing vertex attributes (coordinates) */
		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);

		/* Copy the vertex data from diamond to our buffer */
		/* 8 * sizeof(GLfloat) is the size of the diamond array, since it contains 8 GLfloat values */
		glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLfloat), diamond, GL_STATIC_DRAW);

		/* Specify that our coordinate data is going into attribute index 0, and contains two floats per vertex */
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		/* Enable attribute index 0 as being used */
		glEnableVertexAttribArray(0);

		/* Bind our second VBO as being the active buffer and storing vertex attributes (colors) */
		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);

		/* Copy the color data from colors to our buffer */
		/* 12 * sizeof(GLfloat) is the size of the colors array, since it contains 12 GLfloat values */
		glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLfloat), colors, GL_STATIC_DRAW);

		/* Specify that our color data is going into attribute index 1, and contains three floats per vertex */
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		/* Enable attribute index 1 as being used */
		glEnableVertexAttribArray(1);

		/* Create an empty vertex shader handle */
		vertexshader = glCreateShader(GL_VERTEX_SHADER);

		/* Send the vertex shader source code to GL */
		/* Note that the source code is NULL character terminated. */
		/* GL will automatically detect that therefore the length info can be 0 in this case (the last parameter) */
		glShaderSource(vertexshader, 1, (const GLchar**)&vertexsource, 0);

		/* Compile the vertex shader */
		glCompileShader(vertexshader);

		

		/* Create an empty fragment shader handle */
		fragmentshader = glCreateShader(GL_FRAGMENT_SHADER);

		/* Send the fragment shader source code to GL */
		/* Note that the source code is NULL character terminated. */
		/* GL will automatically detect that therefore the length info can be 0 in this case (the last parameter) */
		glShaderSource(fragmentshader, 1, (const GLchar**)&fragmentsource, 0);

		/* Compile the fragment shader */
		glCompileShader(fragmentshader);

		/* If we reached this point it means the vertex and fragment shaders compiled and are syntax error free. */
		/* We must link them together to make a GL shader program */
		/* GL shader programs are monolithic. It is a single piece made of 1 vertex shader and 1 fragment shader. */
		/* Assign our program handle a "name" */
		shaderprogram = glCreateProgram();

		/* Attach our shaders to our program */
		glAttachShader(shaderprogram, vertexshader);
		glAttachShader(shaderprogram, fragmentshader);

		/* Bind attribute index 0 (coordinates) to in_Position and attribute index 1 (color) to in_Color */
		/* Attribute locations must be setup before calling glLinkProgram. */
		glBindAttribLocation(shaderprogram, 0, "in_Position");
		glBindAttribLocation(shaderprogram, 1, "in_Color");

		/* Link our program */
		/* At this stage, the vertex and fragment programs are inspected, optimized and a binary code is generated for the shader. */
		/* The binary code is uploaded to the GPU, if there is no error. */
		glLinkProgram(shaderprogram);

		/* Again, we must check and make sure that it linked. If it fails, it would mean either there is a mismatch between the vertex */
		/* and fragment shaders. It might be that you have surpassed your GPU's abilities. Perhaps too many ALU operations or */
		/* too many texel fetch instructions or too many interpolators or dynamic loops. */

		

		/* Load the shader into the rendering pipeline */
		glUseProgram(shaderprogram);

		/* Loop our display increasing the number of shown vertexes each time.
		* Start with 2 vertexes (a line) and increase to 3 (a triangle) and 4 (a diamond) */
			/* Invoke glDrawArrays telling that our data is a line loop and we want to draw 2-4 vertexes */
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		/* Cleanup all the things we bound and allocated */
		glUseProgram(0);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDetachShader(shaderprogram, vertexshader);
		glDetachShader(shaderprogram, fragmentshader);
		glDeleteProgram(shaderprogram);
		glDeleteShader(vertexshader);
		glDeleteShader(fragmentshader);
		glDeleteBuffers(2, vbo);
		glDeleteVertexArrays(1, &vao);

	}




	// Get the players position and velocity
	deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) - currentTime;
	currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
	memcpy(&playerPositionLast[0], &playerPosition[0], sizeof(playerPosition));
	getCurrentPlayerPosition(&playerPosition[0]);

	// Get the players region name
	char regName[30];
	getCurrentRegionName(regName);

	// Update our vars
	me.setPos(&playerPosition[0]);
	me.setRegion(regName);

	// Render our overlay
	if (overlayOpenState) {
		drawOverlay();
	}
	haveRenderedInWorldThisFrame = false;
	// Pass back to original swapbuffer func
	return og_wglSwapBuffers(context); 
}

int WINAPI DetourGlDrawArrays(GLenum mode, GLint first, GLsizei count) {
	og_glDrawArrays(mode, first, count);


	
	return 0;// og_glDrawArrays(mode, first, count);
}

LRESULT CALLBACK MousePosHook(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode >= 0) {
		if (wParam == WM_MOUSEMOVE) {
			mousePos[0] = LOWORD(lParam);
			mousePos[1] = HIWORD(lParam);
		}
	}
	return CallNextHookEx(0, nCode, wParam, lParam);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
	if (reason == DLL_PROCESS_ATTACH) { // If we just injected our dll
		// Get handle and proc addr
		nmsAddr = (__int64*)GetModuleHandle(NULL);
		nmsHandle = GetCurrentProcess();
		HHOOK mousehook = SetWindowsHookEx(WH_MOUSE_LL, MousePosHook, NULL, 0);
#ifdef DO_SERVER
		// Init client thread.
		t_handle = CreateThread(0, NULL, ClientThread, NULL, 0, 0);


#endif DO_SERVER


#ifdef DO_OVERLAY


		//Init MH to hook into opengl to render our overlays
		if (MH_Initialize() != MH_OK)
		{
			MessageBoxA(NULL, "Cannot initialize Minhook. NMM will not be able to render to the game!", "Error", MB_OK);
			return FALSE;
		}

		// Hook our functions
		if (MH_CreateHookApi(L"opengl32.dll", "wglSwapBuffers", DetourWglSwapBuffers, reinterpret_cast<LPVOID*>(&og_wglSwapBuffers)) != MH_OK) {
			MessageBoxA(NULL, "Cannot find opengl32.dll in the programs memory space or cannot find wglSwapBuffers function address. NMM will not be able to render to the game!", "Error", MB_OK);
			return FALSE;
		}

		if (MH_CreateHookApi(L"opengl32.dll", "glDrawArrays", DetourGlDrawArrays, reinterpret_cast<LPVOID*>(&og_glDrawArrays)) != MH_OK) {
			MessageBoxA(NULL, "Cannot find opengl32.dll in the programs memory space or cannot find glDrawArrays function address. NMM will not be able to render in-world, but the overlay should still work.", "Error", MB_OK);
		}

		// Enable Hooks!
		if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
		{
			MessageBoxA(NULL, "Function hooks cannot be enabled. NMM will not be able to render to the game!", "Error", MB_OK);
			return FALSE;
		}
#endif DO_OVERLAY
	}

	if (reason == DLL_PROCESS_DETACH) {
#ifdef DO_SERVER
		// Kill client thread
		TerminateThread(t_handle, EXIT_SUCCESS);
#endif DO_SERVER
	}

	return TRUE;
}