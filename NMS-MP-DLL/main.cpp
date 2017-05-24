
#include "main.h"

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
	zmq::context_t context(1);
	

	while (true) {

		// Wait until the user asks to connect to a server
		while (tryConnect == 0) {
			Sleep(1000);
		}

		zmq::socket_t socket(context, ZMQ_REQ);
		// Tell ZMQ to give up if we lose connection for 1 second
		socket.setsockopt(ZMQ_RCVTIMEO, 1000);
		socket.setsockopt(ZMQ_LINGER, 0);

		// Get ip that the user entered
		std::stringstream m;
		m << "tcp://" << ip;

		try {
			socket.connect(m.str().c_str());
			connectStatus = true;
		}
		catch (zmq::error_t t) {
			connectStatus = false;
		}
		
		while (connectStatus && tryConnect) {

			// Give the server a break between packets
			Sleep(50);

			// Update our name in case the user has entered something new
			me.setName(name);

			// Construct our Player Data string
			std::string outgoingString = me.getJSONString();

			// Create message with length
			zmq::message_t outgoing(outgoingString.size());

			// Copy Player Data into message
			memcpy(outgoing.data(), outgoingString.c_str(), outgoingString.size());

			int success = false;
			// Attempt to send it to the server
				try {
					success = socket.send(outgoing);
				}
				catch (zmq::error_t t) {
					connectStatus = false;
					break;
				}
				if (success == false) {
					connectStatus = false;
					break;
				}
			//

			// Create incoming message
			zmq::message_t incoming;
			
			// Attempt to receive it
				try {
					success = socket.recv(&incoming);
				}
				catch (zmq::error_t t) {
					connectStatus = false;
					break;
				}
				if (success == false) {
					connectStatus = false;
					break;
				}
			// Copy data to a string
			std::string incomingString = std::string(static_cast<char*>(incoming.data()), incoming.size());

			// Set up Json reader
			Json::Reader reader;
			Json::Value jResponse;

			// Read server response
			bool parseResult = reader.parse(incomingString, jResponse);
			if (parseResult) {

				// If the servers response is 'nil'
				if (jResponse.isMember("nil")) {
					//The last message we sent was invalid for whatever reason
				}

				// If the servers response is valid
				else if (jResponse.isMember("you") && jResponse.isMember("others")) {

					//Get our new ID if we have a new one
					if (me.getId() != jResponse["you"]["id"].asInt()) {
						me.setId(jResponse["you"]["id"].asInt());
					}

					// TODO: optimise this
					// Iterate through all others in the list and add them to this current frame's players
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
			} // parseResult
		} // isConnected
		socket.close();

	} // true
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

int g = 0;
int WINAPI DetourWglSwapBuffers(int* context)
{
	if (!g) {
		g = gl3wInit();
		ImGui_Init(wglGetCurrentDC());
		_overlayIsInit = true;
	}

	// Toggle overlay when we press F1
	if (GetAsyncKeyState(VK_F1) != lastOverlayOpenKeyState) {
		lastOverlayOpenKeyState = GetAsyncKeyState(VK_F1);
		if (lastOverlayOpenKeyState != 0) { 
			overlayOpenState = !overlayOpenState;
		}
	}


	if (overlayOpenState) {
		
		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_SCISSOR_TEST);
		
		game_render::render_player_marker(39751, -4830, 122724);

		/*
		std::vector<PlayerData> othersCpy = safeGetOthers();
		for (int i = 0; i < othersCpy.size(); i++) {
		//	drawTest(pQ, othersCpy.at(i).getPos()[0], othersCpy.at(i).getPos()[1], othersCpy.at(i).getPos()[2]);
		}*/
	}




	// Get the players position and velocity
	deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) - currentTime;
	currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
	memcpy(&playerPositionLast[0], &playerPosition[0], sizeof(playerPosition));
	game_value::get_player_position(&playerPosition[0]);

	// Get the players region name
	char regName[30];
	game_value::get_player_region(regName);

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
	if (reason == DLL_PROCESS_ATTACH) {
		game_value::init();

#ifdef DO_SERVER
		t_handle = CreateThread(0, NULL, ClientThread, NULL, 0, 0);
#endif DO_SERVER
#ifdef DO_OVERLAY
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