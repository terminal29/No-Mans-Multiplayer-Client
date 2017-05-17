
#include "main.h"

void getCurrentPlayerPosition(float* out_floatArr) {
	float p[3];

	memcpy(&p[0], (float*)((char*)nmsAddr + posXOffs), sizeof(float));
	memcpy(&p[1], (float*)((char*)nmsAddr + posXOffs + sizeof(float)), sizeof(float));
	memcpy(&p[2], (float*)((char*)nmsAddr + posXOffs + 2*sizeof(float)), sizeof(float));

	memcpy(out_floatArr, &p[0], sizeof(p));
}

void getCurrentPlayerRotations(float* out_floatArr) {
	float p[9];
	//13,14,15 16,17,18 19,20,21

	memcpy(&p[0], (float*)((char*)nmsAddr + posXOffs + 13 * sizeof(float)), sizeof(float));
	memcpy(&p[1], (float*)((char*)nmsAddr + posXOffs + 14 * sizeof(float)), sizeof(float));
	memcpy(&p[2], (float*)((char*)nmsAddr + posXOffs + 15 * sizeof(float)), sizeof(float));
	memcpy(&p[3], (float*)((char*)nmsAddr + posXOffs + 16 * sizeof(float)), sizeof(float));
	memcpy(&p[4], (float*)((char*)nmsAddr + posXOffs + 17 * sizeof(float)), sizeof(float));
	memcpy(&p[5], (float*)((char*)nmsAddr + posXOffs + 18 * sizeof(float)), sizeof(float));
	memcpy(&p[6], (float*)((char*)nmsAddr + posXOffs + 19 * sizeof(float)), sizeof(float));
	memcpy(&p[7], (float*)((char*)nmsAddr + posXOffs + 20 * sizeof(float)), sizeof(float));
	memcpy(&p[8], (float*)((char*)nmsAddr + posXOffs + 21 * sizeof(float)), sizeof(float));

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
	while (tryConnect == 0) {
		Sleep(1000);
	}
	// Attempt to connect to the server
	zmq::context_t context(1);
	zmq::socket_t socket(context, ZMQ_REQ);
	std::stringstream m;
	m << "tcp://" << ip;
	socket.connect(m.str().c_str());
	while (true) {
		Sleep(50);
		// Send a copy of our PlayerData to the server
		me.setName(name);
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
		return;
	}
	
	ImGuiIO& io = ImGui::GetIO();
	io.WantCaptureMouse = true;
	io.MouseDrawCursor = true;

	io.KeysDown[VK_TAB] = GetAsyncKeyState(VK_TAB) == 0 ? false : true;                         // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
	io.KeysDown[VK_LEFT] = GetAsyncKeyState(VK_LEFT) == 0 ? false : true;
	io.KeysDown[VK_RIGHT] = GetAsyncKeyState(VK_RIGHT) == 0 ? false : true;
	io.KeysDown[VK_UP] = GetAsyncKeyState(VK_UP) == 0 ? false : true;
	io.KeysDown[VK_DOWN] = GetAsyncKeyState(VK_DOWN) == 0 ? false : true;
	io.KeysDown[VK_RETURN] = GetAsyncKeyState(VK_RETURN) == 0 ? false : true;
	io.KeysDown[VK_ESCAPE] = GetAsyncKeyState(VK_ESCAPE) == 0 ? false : true;

	for (int i = 0; i < numControlKeys; i++) {
		keyControlStates[i] = !GetAsyncKeyState(vkCodesControl[i]) == 0;
	}
	for (int i = 0; i < numControlKeys; i++) {
		if (keyControlStates[i] && keyControlStates[i] != prevKeyControlStates[i]) { //If it is down and we just pressed it
			io.KeysDown[vkCodesControl[i]] = true;
		}
		else {
			io.KeysDown[vkCodesControl[i]] = false;
		}
	}
	memcpy(prevKeyControlStates, keyControlStates, sizeof(keyControlStates));

	for (int i = 0; i < numKeys; i++) {
		keyStates[i] = !GetAsyncKeyState(vkCodes[i]) == 0;
	}

	for (int i = 0; i < numKeys; i++) {
		if (keyStates[i] && keyStates[i] != prevKeyStates[i]) { //If it is down and we just pressed it
			io.AddInputCharacter(keyChars[i]);
		}
	}
	memcpy(prevKeyStates,keyStates, sizeof(keyStates));


	// Create a new frame
	ImGui_NewFrame();
	ImGui::Begin("No Man's Multiplayer");
	ImGui::SetWindowSize({ 250,350 });

	// If the client thread is disabled
#ifndef DO_SERVER
	ImGui::TextColored({ 1,0,0,1 }, "Client thread disabled");
#else
	// Status Display
	std::stringstream cStatus;
	cStatus << "Connection Status [" << (connectStatus ? "Connected]" : "Disconnected]") << std::endl;
	ImGui::TextColored({ 1,1,1,1 }, cStatus.str().c_str());
	
	ImGui::InputText("Name", &name[0], 30);

	ImGui::InputText("IP:Port", ip, 30);
	ImGui::SliderInt("<<", &tryConnect, 0, 1);


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
	m << "Build: " << __DATE__ << " @ " << __TIME__;
	ImGui::Text(m.str().c_str());

	
	// Draw Overlay
	ImGui::End();
	ImGui::Render();
}

void drawTest(float* pQ, float x, float y, float z) {

	HWND hwnd = WindowFromDC(nms_window);
	RECT windowRect;
	GetWindowRect(hwnd, &windowRect);
	w = windowRect.right - windowRect.left;
	h = windowRect.bottom - windowRect.top;

	glm::vec3 Xax = { pQ[0], pQ[1], pQ[2] };
	glm::vec3 Yax = { -pQ[3], -pQ[4], -pQ[5] };
	glm::vec3 Zax = { -pQ[6], -pQ[7], -pQ[8] };
	glm::vec3 pos = { me.getPos()[0], me.getPos()[1], me.getPos()[2] };

	glm::mat4 View = { Xax.x, Yax.x, Zax.x, 0,
		Xax.y, Yax.y, Zax.y, 0,
		Xax.z, Yax.z, Zax.z, 0,
		-glm::dot(pos, Xax), -glm::dot(pos, Yax), -glm::dot(pos, Zax), 1 };


	glm::mat4 projectionMatrix = glm::perspective(
		glm::radians(45.0f),         // The horizontal Field of View, in degrees : the amount of "zoom". Think "camera lens". Usually between 90° (extra wide) and 30° (quite zoomed in)
		(float)w / h, // Aspect Ratio. Depends on the size of your window. Notice that 4/3 == 800/600 == 1280/960, sounds familiar ?
		0.1f,        // Near clipping plane. Keep as big as possible, or you'll get precision issues.
		1000.0f       // Far clipping plane. Keep as little as possible.
	);

	// Model matrix : an identity matrix (model will be at the origin)
	glm::mat4 Model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
	//Model = glm::lookAt(glm::vec3(x, y, z), glm::vec3(me.getPos()[0], me.getPos()[1], me.getPos()[2]), glm::vec3(0, 1, 0));

	//Model = glm::translate(Model, glm::vec3(0, 0, -1));
	// Our ModelViewProjection : multiplication of our 3 matrices
	glm::mat4 modelviewMatrix = View * Model; // Remember, matrix multiplication is the other way around

	GLchar* vertexsource = "#version 410 compatibility\n	\
		uniform mat4 MV;							\
		uniform mat4 P;								\
		uniform sampler2D tex;						\
		in  vec3 in_Position;						\
		in  vec2 uv;								\
		out vec4 ex_Color;							\
		void main(void) {							\
			gl_Position = P * (MV * vec4(0.0, 0.0, 0.0, 1.0) + vec4(in_Position.x, in_Position.y, 0.0, 0.0));\
			ex_Color = texture(tex,uv);				\
		}";

	GLchar* fragmentsource = "#version 410 compatibility\n	\
			precision highp float;					\
													\
		in  vec4 ex_Color;							\
		out vec4 gl_FragColor;						\
													\
		void main(void) {							\
			gl_FragColor = ex_Color;		\
		}";

	int i; /* Simple iterator */
	GLuint vao, vbo[2]; /* Create handles for our Vertex Array Object and two Vertex Buffer Objects */
	int IsCompiled_VS, IsCompiled_FS;
	int IsLinked;
	int maxLength;
	char *vertexInfoLog;
	char *fragmentInfoLog;
	char *shaderProgramInfoLog;

	float scale = 0.75;

	/* We're going to create a simple diamond made from lines */
	const GLfloat diamond[4][2] = {
		{ -scale, -scale }, /* Top point */
		{ scale, -scale }, /* Right point */
		{ scale, scale }, /* Bottom point */
		{ -scale, scale } }; /* Left point */

	const GLfloat uv[4][3] = {
		{ 0.0,  0.0},
		{ 0.0,  1.0},
		{ 1.0,  1.0},
		{ 0.0,  1.0} };

	GLuint Atlas_Tex;
	glGenTextures(1, &Atlas_Tex);
	int width = 64, height = 64;
	unsigned char image[64*64];
	//std::fill_n(image, 64 * 64, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, Atlas_Tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);

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
	glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(GLfloat), diamond, GL_STATIC_DRAW);

	/* Specify that our coordinate data is going into attribute index 0, and contains two floats per vertex */
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	/* Enable attribute index 0 as being used */
	glEnableVertexAttribArray(0);

	/* Bind our second VBO as being the active buffer and storing vertex attributes (colors) */
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);

	/* Copy the color data from colors to our buffer */
	/* 12 * sizeof(GLfloat) is the size of the colors array, since it contains 12 GLfloat values */
	glBufferData(GL_ARRAY_BUFFER, sizeof(uv), uv, GL_STATIC_DRAW);

	/* Specify that our color data is going into attribute index 1, and contains three floats per vertex */
	glVertexAttribPointer(1, 3, GL_INT, GL_FALSE, 0, 0);

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

	glGetShaderiv(vertexshader, GL_COMPILE_STATUS, &IsCompiled_VS);
	if (IsCompiled_VS == FALSE)
	{
		glGetShaderiv(vertexshader, GL_INFO_LOG_LENGTH, &maxLength);

		/* The maxLength includes the NULL character */
		vertexInfoLog = (char *)malloc(maxLength);

		glGetShaderInfoLog(vertexshader, maxLength, &maxLength, vertexInfoLog);

		/* Handle the error in an appropriate way such as displaying a message or writing to a log file. */
		/* In this simple program, we'll just leave */
		free(vertexInfoLog);
		MessageBoxA(NULL, vertexInfoLog, NULL, MB_OK);
	}

	/* Create an empty fragment shader handle */
	fragmentshader = glCreateShader(GL_FRAGMENT_SHADER);

	/* Send the fragment shader source code to GL */
	/* Note that the source code is NULL character terminated. */
	/* GL will automatically detect that therefore the length info can be 0 in this case (the last parameter) */
	glShaderSource(fragmentshader, 1, (const GLchar**)&fragmentsource, 0);

	/* Compile the fragment shader */
	glCompileShader(fragmentshader);

	glGetShaderiv(fragmentshader, GL_COMPILE_STATUS, &IsCompiled_FS);
	if (IsCompiled_FS == FALSE)
	{
		glGetShaderiv(fragmentshader, GL_INFO_LOG_LENGTH, &maxLength);

		/* The maxLength includes the NULL character */
		fragmentInfoLog = (char *)malloc(maxLength);

		glGetShaderInfoLog(fragmentshader, maxLength, &maxLength, fragmentInfoLog);

		MessageBoxA(NULL, fragmentInfoLog, NULL, MB_OK);
		/* Handle the error in an appropriate way such as displaying a message or writing to a log file. */
		/* In this simple program, we'll just leave */
		free(fragmentInfoLog);
	}

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
	glBindAttribLocation(shaderprogram, 1, "uv");

	/* Link our program */
	/* At this stage, the vertex and fragment programs are inspected, optimized and a binary code is generated for the shader. */
	/* The binary code is uploaded to the GPU, if there is no error. */
	glLinkProgram(shaderprogram);

	/* Again, we must check and make sure that it linked. If it fails, it would mean either there is a mismatch between the vertex */
	/* and fragment shaders. It might be that you have surpassed your GPU's abilities. Perhaps too many ALU operations or */
	/* too many texel fetch instructions or too many interpolators or dynamic loops. */

	glGetProgramiv(shaderprogram, GL_LINK_STATUS, (int *)&IsLinked);
	if (IsLinked == FALSE)
	{
		/* Noticed that glGetProgramiv is used to get the length for a shader program, not glGetShaderiv. */
		glGetProgramiv(shaderprogram, GL_INFO_LOG_LENGTH, &maxLength);

		/* The maxLength includes the NULL character */
		shaderProgramInfoLog = (char *)malloc(maxLength);

		/* Notice that glGetProgramInfoLog, not glGetShaderInfoLog. */
		glGetProgramInfoLog(shaderprogram, maxLength, &maxLength, shaderProgramInfoLog);

		/* Handle the error in an appropriate way such as displaying a message or writing to a log file. */
		/* In this simple program, we'll just leave */
		free(shaderProgramInfoLog);
		MessageBoxA(NULL, shaderProgramInfoLog, NULL, MB_OK);
	}

	/* Load the shader into the rendering pipeline */
	glUseProgram(shaderprogram);

	// Update the uniforms AFTER glUseProgram
	GLuint modelview_handle = glGetUniformLocation(shaderprogram, "MV");
	GLuint projection_handle = glGetUniformLocation(shaderprogram, "P");
	glUniformMatrix4fv(modelview_handle, 1, GL_FALSE, &modelviewMatrix[0][0]);
	glUniformMatrix4fv(projection_handle, 1, GL_FALSE, &projectionMatrix[0][0]);

	//Load in our texture
	GLuint texture_handle = glGetUniformLocation(shaderprogram, "tex");
	glUniform1i(texture_handle, 0);

	/* Invoke glDrawArrays telling that our data is a line loop and we want to draw 2-4 vertexes */
	glDrawArrays(GL_QUADS, 0, 4);


	/* Cleanup all the things we bound and allocated */
	//glUseProgram(0);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDetachShader(shaderprogram, vertexshader);
	glDetachShader(shaderprogram, fragmentshader);
	glDeleteProgram(shaderprogram);
	glDeleteShader(vertexshader);
	glDeleteShader(fragmentshader);
	glDeleteBuffers(2, vbo);
	glDeleteVertexArrays(1, &vao);
	//	free(vertexsource);
	//	free(fragmentsource);
}

int g = 0;
int WINAPI DetourWglSwapBuffers(int* context)
{
	if (!g) {
		g = gl3wInit();
		nms_window = wglGetCurrentDC();
		ImGui_Init(nms_window);
		_overlayIsInit = true;
	}


	// Toggle overlay when we press F1
	if (GetAsyncKeyState(VK_F1) != lastOverlayOpenKeyState) {
		lastOverlayOpenKeyState = GetAsyncKeyState(VK_F1);
		if (lastOverlayOpenKeyState != 0) { 
			overlayOpenState = !overlayOpenState;
		}
	}


	if (_overlayIsInit) {
		
		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_SCISSOR_TEST);
		
	
		float pQ[9];
		getCurrentPlayerRotations(&pQ[0]);

		std::vector<PlayerData> othersCpy = safeGetOthers();
		for (int i = 0; i < othersCpy.size(); i++) {
			drawTest(pQ, othersCpy.at(i).getPos()[0], othersCpy.at(i).getPos()[1], othersCpy.at(i).getPos()[2]);
		}
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


BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
	if (reason == DLL_PROCESS_ATTACH) { // If we just injected our dll
		// Get handle and proc addr
		nmsAddr = (__int64*)GetModuleHandle(NULL);
		nmsHandle = GetCurrentProcess();
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