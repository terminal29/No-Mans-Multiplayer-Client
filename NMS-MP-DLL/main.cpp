#include "main.h"

#define DEFAULT_PORT 5258
#define MILLIS(a) std::chrono::milliseconds(a)

//TODO: Move this into game_render.h
void render_overlay() {
	if (!game_render::overlay_init_state) {
//		game_render::init();
		return;
	}

	ImGuiIO& io = ImGui::GetIO();
	io.WantCaptureMouse = true;
	io.MouseDrawCursor = true;

	// If the window is in the foreground
	if (GetForegroundWindow() == WindowFromDC(wglGetCurrentDC()) && overlayOpenState) {
		io.KeysDown[VK_TAB] = GetAsyncKeyState(VK_TAB) == 0 ? false : true;                         
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
		memcpy(prevKeyStates, keyStates, sizeof(keyStates));
	}
	else {
		io.ClearInputCharacters();
		for (int i = 0; i < numKeys; i++) {
			keyStates[i] = false;
		}
		for (int i = 0; i < numControlKeys; i++) {
			keyControlStates[i] = false;
		}
	}
	// Create a new frame
	ImGui_NewFrame();
	ImGui::Begin("No Man's Multiplayer [Alpha 0.1]");
	ImGui::SetWindowSize({ 350,450 });
	
	// Status Display
	std::stringstream cStatus;
	cStatus << "Connection Status [" << (comms_client::is_connected ? "Connected]" : "Disconnected]") << std::endl;
	ImGui::TextColored({ 1,1,1,1 }, cStatus.str().c_str());

	// Set new name for your server-side character
	char temp_name[30];
	strcpy(temp_name, game_value::me.getName().c_str());
	ImGui::InputText("Name", temp_name, 29);
	game_value::me.setName(temp_name);

	// Set new ip to connect to
	char temp_ip[30];
	memset(temp_ip, '\0', 30 * sizeof(char));
	strcpy(temp_ip, comms_client::ip);
	ImGui::InputText("Server IP", temp_ip, 29);
	memcpy(comms_client::ip, temp_ip, 30 * sizeof(char));

	int temp_port = 0;
	memcpy(&temp_port, &comms_client::port, sizeof(temp_port));
	ImGui::DragInt("Port", &temp_port, 1.0f, 1024, 49151);
	comms_client::port = temp_port;

	// This is probably a really bad idea
	int opt = comms_client::want_connection ? 1 : 0;
	ImGui::DragInt("Join", &opt, 1.0f, 0, 1);
	comms_client::want_connection = opt == 1 ? true : false;
	ImGui::Text("Type 1 to join and 0 to disconnect.");
	ImGui::Text("(This is temporary)");

	// Players Display
	ImGui::TextColored({ 1,0,0,1 }, "Connected Players");
	std::vector<PlayerData> currentOthers = comms_client::safe_get_players();
	std::stringstream othersString;
	for (int i = 0; i < currentOthers.size(); i++) {
		othersString << currentOthers.at(i).getName() << std::endl;
		othersString << " " << currentOthers.at(i).getRegion() << std::endl;
		if (currentOthers.at(i).getRegion() == game_value::me.getRegion()) {
			othersString << " " << static_cast<int>(LinearDist3f(game_value::me.getPos(), currentOthers.at(i).getPos())) << " units away" << std::endl;
		}
	}
	ImGui::Text(othersString.str().c_str());
	ImGui::Spacing();

	// Region Display
	ImGui::TextColored({ 1,0,1,1 }, "Current Region");
	ImGui::Text(game_value::me.getRegion().c_str());
	ImGui::Spacing();

	// Position Display
	std::stringstream m;
	m << static_cast<int>(game_value::me.getPos()[0]) << ":" << static_cast<int>(game_value::me.getPos()[1]) << ":" << static_cast<int>(game_value::me.getPos()[2]);
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
	m << "Build Date: " << __DATE__ << " @ " << __TIME__;
	ImGui::Text(m.str().c_str());

	// Draw Overlay
	ImGui::End();
	ImGui::Render();
}

std::string get_time() {
	time_t t = std::time(nullptr);
	std::stringstream time;
	time << std::put_time(std::localtime(&t), "%H:%M:%S");
	return time.str();
}

std::string p_prfx() {
	std::stringstream prefix;
	prefix << "[" << get_time() << "] ";
	return prefix.str();
}

void ClientThread()
{
	while (true) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
		if(comms_client::want_connection) {
			bool connect_result = comms_client::start_client();
			if (connect_result) {
				comms_client::handle_client();
			}
			WSACleanup();
		}
	}
}

int gl3w_res = 0;
int WINAPI DetourWglSwapBuffers(int* context)
{
	// Init gl3w if it isn't already
	if (gl3w_res == 0) {
		gl3w_res = gl3wInit();
		// if it succeeded
		if (gl3w_res >= 0) {
			// Init ImGui
			ImGui_Init(wglGetCurrentDC());
			game_render::overlay_init_state = true;
		}
		else {
			return og_wglSwapBuffers(context);
		}
	}

	// Toggle overlay when we press F1
	// TODO: move to game_controls.h
	if (GetAsyncKeyState(VK_F1) != lastOverlayOpenKeyState) {
		lastOverlayOpenKeyState = GetAsyncKeyState(VK_F1);
		if (lastOverlayOpenKeyState != 0) { 
			overlayOpenState = !overlayOpenState;
		}
	}

	// Get the players position and velocity
	// TODO: move to game_value.h
	deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) - currentTime;
	currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
	memcpy(&playerPositionLast[0], &playerPosition[0], sizeof(playerPosition));
	game_value::get_player_position(&playerPosition[0]);

	// Get the players region name
	char regName[30];
	game_value::get_player_region(regName);

	// Update our vars
	game_value::me.setPos(&playerPosition[0]);
	game_value::me.setRegion(regName);

	// If the overlay is open and is init
	if (overlayOpenState && game_render::overlay_init_state) {
		render_overlay();
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	std::vector<PlayerData> othersCpy = comms_client::safe_get_players();
	for (int i = 0; i < othersCpy.size(); i++) {
		game_render::render_player_marker(othersCpy.at(i).getPos()[0], othersCpy.at(i).getPos()[1], othersCpy.at(i).getPos()[2]);
	}
	glDisable(GL_BLEND);

	// Pass back to original swapbuffer func
	og_wglSwapBuffers(context); 
	return 1;
}

DWORD WINAPI c_client_thread(LPVOID) {
	ClientThread();
	return 1;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
	if (reason == DLL_PROCESS_ATTACH) {
		// Load up all game values and work out offsets
		game_value::init();

		CreateThread(0, NULL, c_client_thread, NULL, 0, 0);
		//comms_client::client_comms_thread = std::thread(ClientThread);

		if (MH_Initialize() != MH_OK)
		{
			display_error("Cannot initialise Minhook. No Man's Multiplayer will not be able to the game.", true);
			return FALSE;
		}

		// Hook our functions
		if (MH_CreateHookApi(L"opengl32.dll", "wglSwapBuffers", DetourWglSwapBuffers, reinterpret_cast<LPVOID*>(&og_wglSwapBuffers)) != MH_OK) {
			display_error("Cannot find opengl32.dll in the programs memory space or cannot find wglSwapBuffers function address. No Man's Multiplayer will not be able to render to the game.", true);
			return FALSE;
		}

		// Enable Hooks!
		if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
		{
			display_error("OpenGL function hooks cannot be enabled. No Man's Multiplayer will not be able to render to the game.", true);
			return FALSE;
		}
	}

	if (reason == DLL_PROCESS_DETACH) {
		// TOOD: cleanup maybe
	}
	
	return TRUE;
}