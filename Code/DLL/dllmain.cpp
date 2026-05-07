#include "dllmain.h"
#include <psapi.h>

uintptr_t clientDll = 0;
uintptr_t engine2Dll = 0;

uintptr_t entityListOffset = 0;

bool inMatch = false;

Player* localPlayer = nullptr;

int screenHeight = 0;
int screenWidth = 0;
bool isCursorInWindow = false;

bool hideMenu = false;

float moveViewAnglesTolerance = 0.05;

int maxAimbotTimer = 500; // controls how smooth it is

bool enableAimbot = true;
bool useRightClick = true;
bool holdToUseAimbot = false;
bool headShots = true;
bool targetClosestToCrosshair = true;
float aimbotStrength = 10;

bool esp = true;
bool showPlayerNames = true;
bool hideEspInfo = true;
bool enableCrosshair = true;

bool targetSameTeam = false;

DWORD WINAPI Thread(LPVOID param)
{
	clientDll = (uintptr_t)GetModuleHandle(L"client.dll");
	engine2Dll = (uintptr_t)GetModuleHandle(L"engine2.dll");

	if (clientDll == 0 || engine2Dll == 0 || !HookPresent()) // hooking directx
	{
		FreeLibraryAndExitThread((HMODULE)param, 0);
		return 0;
	}

	if (!UpdateEntityListOffset())
	{
		FreeLibraryAndExitThread((HMODULE)param, 0);
		return 0;
	}

	Player* aimbotTargetPlayer = nullptr;

	int aimbotTimer = 0;
	bool aimbot = false;
	while (!GetAsyncKeyState(VK_INSERT)) // exit when ins key is pressed
	{
		if (GetAsyncKeyState(VK_F1) & 1)
		{
			hideMenu = !hideMenu;
		}

		DWORD aimbotKey = VK_LSHIFT;
		if (useRightClick) { aimbotKey = VK_RBUTTON; }

		if (!IsValidPlayer(localPlayer)) 
		{
			localPlayer = GetLocalPlayer();
			continue; 
		}

		inMatch = localPlayer->gameScene != 0;
		if (!inMatch) 
		{ 
			localPlayer = GetLocalPlayer();
			continue; 
		}

		aimbotTimer++;

		if (!isCursorInWindow || !enableAimbot) { aimbot = false; continue; }

		if (GetAsyncKeyState(aimbotKey) & 1)
		{
			aimbot = !aimbot;

			if (aimbot) { aimbotTargetPlayer = GetClosestPlayer(); }
			else { aimbotTargetPlayer = nullptr; }
		}

		if ((!holdToUseAimbot || (holdToUseAimbot && GetAsyncKeyState(aimbotKey))) && aimbot && aimbotTimer > maxAimbotTimer * localPlayer->zoom)
		{
			aimbotTimer = 0;

			if (!CanAimbotPlayer(aimbotTargetPlayer))
			{
				aimbot = false;
				aimbotTargetPlayer = nullptr;
				continue;
			}

			Aimbot(aimbotTargetPlayer);
		}
	}

	UnhookPresent();

	Sleep(100);

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	if (mainRenderTargetView) { mainRenderTargetView->Release(); mainRenderTargetView = NULL; }
	if (p_context) { p_context->Release(); p_context = NULL; }
	if (p_device) { p_device->Release(); p_device = NULL; }
	SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)(oWndProc));

	FreeLibraryAndExitThread((HMODULE)param, 0);
	return 0;
}

BOOL WINAPI DllMain(HINSTANCE hModule, DWORD  dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		CreateThread(0, 0, Thread, hModule, 0, 0);
	}

	return TRUE;
}

bool UpdateEntityListOffset()
{
	// entityListOffset
	// rax, QWORD PTR [rip+0x2381e52]
	int entityListSig[25] = { 0x48, 0x8B, 0x05, 0x1000, 0x1000, 0x1000, 0x1000, 0x4C, 0x8D, 0x2D, 0x1000, 0x1000, 0x1000, 0x1000, 0x48, 0x89, 0xBC, 0x24, 0x1000, 0x1000, 0x1000, 0x1000, 0x48, 0x8B, 0x98 };
	uintptr_t entityListSigAddress = FindClientDLLSig(entityListSig, 25);
	if (entityListSigAddress == 0) 
	{
		return false;
	}
	entityListOffset = *(unsigned int*)(entityListSigAddress + 3) + (entityListSigAddress + 7) - clientDll;

	return true;
}

uintptr_t FindClientDLLSig(int* sig, int sigLen)
{
	MODULEINFO modInfo = { 0 };
	uintptr_t clientSize = GetModuleInformation(GetCurrentProcess(), (HMODULE)clientDll, &modInfo, sizeof(modInfo));
	for (DWORD i = 0; i < modInfo.SizeOfImage; i++)
	{
		bool foundSig = false;
		for (int j = 0; j < sigLen; j++)
		{
			if (sig[j] == 0x1000)
			{
				continue;
			}

			if (*(unsigned char*)(clientDll + i + j) != sig[j])
			{
				break;
			}

			if (j == sigLen - 1)
			{
				foundSig = true;
			}
		}

		if (foundSig)
		{
			return clientDll + i;
		}
	}

	return 0;
}

void Draw() // called in DetourPresent()
{
	ImGuiIO& io = ImGui::GetIO();
	screenHeight = io.DisplaySize.y;
	screenWidth = io.DisplaySize.x;
	isCursorInWindow = IsCursorInWindow();

	if (!hideMenu)
	{
		ImU32 primaryTeamColor;
		ImU32 secondaryTeamColor;

		int team = inMatch && IsValidPlayer(localPlayer) ? localPlayer->team : -1;
		switch (team)
		{
		case Terrorist:
			primaryTeamColor = IM_COL32(230, 180, 90, 255);
			secondaryTeamColor = IM_COL32(130, 100, 50, 255);
			break;
		case CounterTerrorist:
			primaryTeamColor = IM_COL32(90, 150, 250, 255);
			secondaryTeamColor = IM_COL32(50, 100, 150, 255);
			break;
		default:
			primaryTeamColor = IM_COL32(150, 150, 150, 255);
			secondaryTeamColor = IM_COL32(80, 80, 80, 255);
			break;
		}

		ImGui::PushStyleColor(ImGuiCol_CheckMark, primaryTeamColor);
		ImGui::PushStyleColor(ImGuiCol_SliderGrab, primaryTeamColor);
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, primaryTeamColor);

		ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, secondaryTeamColor);
		ImGui::PushStyleColor(ImGuiCol_TitleBg, secondaryTeamColor);
		ImGui::PushStyleColor(ImGuiCol_TitleBgActive, secondaryTeamColor);
		ImGui::PushStyleColor(ImGuiCol_FrameBg, secondaryTeamColor);
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, secondaryTeamColor);

		ImGui::Begin("CS2 Jesso Cheats", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
		ImGui::SetWindowPos(ImVec2(0, 0));
		ImGui::SetWindowSize(ImVec2(400, 335), ImGuiCond_Always);

		ImGui::Text("Ins - uninject");
		ImGui::Text("F1 - hide this menu");
		if (useRightClick) { ImGui::Text("Right click - use aimbot"); }
		else { ImGui::Text("Left Shift - use aimbot"); }

		ImGui::Checkbox("Enable aimbot", &enableAimbot);
		ImGui::Checkbox("Right click to aimbot", &useRightClick);
		ImGui::Checkbox("Hold to use aimbot", &holdToUseAimbot);
		ImGui::Checkbox("Aim for heads", &headShots);
		if (targetClosestToCrosshair) { ImGui::Checkbox("Target player closest to crosshair", &targetClosestToCrosshair); }
		else { ImGui::Checkbox("Target player closest to you", &targetClosestToCrosshair); }
		ImGui::SliderFloat("Aimbot strength", &aimbotStrength, 0.01, 10, "%.2f");

		ImGui::Checkbox("ESP", &esp);
		ImGui::Checkbox("Show player names", &showPlayerNames);
		ImGui::Checkbox("Hide ESP info", &hideEspInfo);
		ImGui::Checkbox("Enable centered crosshair", &enableCrosshair);

		ImGui::Checkbox("ESP/Aimbot players on same team", &targetSameTeam);

		ImGui::End();

		ImGui::PopStyleColor(8);
	}

	if (esp)
	{
		if (!inMatch || !IsValidPlayer(localPlayer)) { return; }

		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
		ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0.0f, 0.0f, 0.0f, 0.0f });
		ImGui::Begin("invis window", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs);

		ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Always);
		ImGui::SetWindowSize(ImVec2(screenWidth, screenHeight), ImGuiCond_Always);

		ImGuiWindow* window = ImGui::GetCurrentWindow();
		ImDrawList* drawList = window->DrawList;

		if (enableCrosshair)
		{
			drawList->AddCircle(ImVec2(screenWidth / 2, screenHeight / 2), 3, IM_COL32(255, 0, 0, 255), 0, 2);
		}

		ESP(drawList);

		window->DrawList->PushClipRectFullScreen();
		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(2);
	}
}

bool IsCursorInWindow()
{
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	if (viewport == nullptr) { return false; }

	ImVec2 windowPos = viewport->Pos;
	ImVec2 windowSize = viewport->Size;
	ImVec2 cursorPos = ImGui::GetIO().MousePos;

	if (cursorPos.x < windowPos.x) { return false; }
	if (cursorPos.x > windowPos.x + windowSize.x) { return false; }

	if (cursorPos.y < windowPos.y) { return false; }
	if (cursorPos.y > windowPos.y + windowSize.y) { return false; }

	return true;
}

uintptr_t GetPlayerController(int index)
{
	if (index < 0 || index >= maxPlayerCount) { return 0; }

	uintptr_t* entitySystemPtr = (uintptr_t*)(clientDll + entityListOffset);
	if (entitySystemPtr == nullptr || (*entitySystemPtr) == 0) { return 0; }
	uintptr_t entitySystem = *entitySystemPtr;

	uintptr_t* entityListPtr = (uintptr_t*)(entitySystem + 0x10);
	if (entityListPtr == nullptr || (*entityListPtr) == 0) { return 0; }
	uintptr_t entityList = *entityListPtr;

	uintptr_t* playerControllerPtr = (uintptr_t*)(entityList + ((bytesBetweenControllers * index) + bytesBetweenControllers));
	if (playerControllerPtr == nullptr) { return 0; }
	uintptr_t playerController = *playerControllerPtr;

	return playerController;
}

Player* GetLocalPlayer()
{
	for (int i = 0; i < maxPlayerCount; i++) 
	{
		uintptr_t controller = GetPlayerController(i);
		if (controller != 0 && *(bool*)(controller + isLocalPlayerOffset))
		{
			return GetPlayer(i);
		}
	}

	return nullptr;
}

Player* GetPlayer(int index)
{
	if (index < 0 || index >= maxPlayerCount) { return nullptr; }

	uintptr_t playerController = GetPlayerController(index);
	if (playerController == 0) { return nullptr; }

	int pawnHandle = *(int*)(playerController + pawnHandleOffset);
	if (pawnHandle == 0) { return nullptr; }

	uintptr_t* entitySystemPtr = (uintptr_t*)(clientDll + entityListOffset);
	if (entitySystemPtr == nullptr || (*entitySystemPtr) == 0) { return 0; }
	uintptr_t entitySystem = *entitySystemPtr;

	uintptr_t* entityListPtr = (uintptr_t*)(entitySystem + 0x10 + (8 * ((pawnHandle & 0x7FFF) >> 9)));
	if (entityListPtr == nullptr || (*entityListPtr) == 0) { return nullptr; }
	uintptr_t entityList = *entityListPtr;

	Player** playerPtr = (Player**)(entityList + (bytesBetweenControllers * (pawnHandle & 0x1FF)));
	if (playerPtr == nullptr || (*playerPtr) == 0) { return nullptr; }
	Player* player = *playerPtr;

	return player;
}

bool IsValidPlayer(Player* player)
{
	if ((uintptr_t)player < 0x10000) { return false; }
	if (player->health < 1 || player->health > 100) { return false; }
	if (player->headHeight < 1 || player->headHeight > 100) { return false; }
	if (player->team != Terrorist && player->team != CounterTerrorist) { return false; }
	if (player->zoom == 0) { return false; }
	if (player->pos.x == 0 || player->pos.y == 0 || player->pos.z == 0) { return false; }

	return true;
}

bool CanAimbotPlayer(Player* player)
{
	if (!IsValidPlayer(player) || player == localPlayer) { return false; }
	if (!targetSameTeam && player->team == localPlayer->team) { return false; }

	return true;
}

void PredictPosition(Player* targetPlayer, Vector3& out)
{
	if (!IsValidPlayer(localPlayer) || !IsValidPlayer(targetPlayer)) { return; }

	Vector3 velocity = targetPlayer->velocity - localPlayer->velocity;

	out.x += velocity.x / 50;
	out.y += velocity.y / 50;
	out.z += velocity.z / 50;
}

Vector3 GetPlayerPosition(Player* player, bool getHeadPos)
{
	Vector3 result = { 0, 0, 0 };

	if (getHeadPos)
	{
		result.x = *(float*)(*(uintptr_t*)(player->gameScene + boneListOffset) + (bytesPerBone * headBoneIndex));
		result.y = *(float*)(*(uintptr_t*)(player->gameScene + boneListOffset) + (bytesPerBone * headBoneIndex) + 0x4);
		result.z = *(float*)(*(uintptr_t*)(player->gameScene + boneListOffset) + (bytesPerBone * headBoneIndex) + 0x8) - 72;

		if (player != localPlayer) 
		{
			result.z += 3;
		}
	}
	else
	{
		result = player->pos;
		result.z -= 5;
	}

	return result;
}

Vector2 GetPlayerScreenPos(Player* player, bool getHeadPos)
{
	Vector2 result = {};

	Vector3 localPlayerPos = GetPlayerPosition(localPlayer, true);
	Vector3 targetPlayerPos = GetPlayerPosition(player, getHeadPos);

	PredictPosition(player, targetPlayerPos);

	Vector3 diff = localPlayerPos - targetPlayerPos;
	float distance = sqrt((diff.x * diff.x) + (diff.y * diff.y) + (diff.z * diff.z));
	if (distance == 0) { return result; }

	float pitchToPlayer = -(asin((targetPlayerPos.z - localPlayerPos.z) / distance) * rToD);
	float yawToPlayer = (atan2(targetPlayerPos.y - localPlayerPos.y, targetPlayerPos.x - localPlayerPos.x) * rToD);

	float relativePitch = pitchToPlayer - localPlayer->pitch;
	float relativeYaw = localPlayer->yaw - yawToPlayer;

	if (relativeYaw > 180) { relativeYaw = -(360 - relativeYaw); }
	if (relativeYaw < -180) { relativeYaw = (360 + relativeYaw); }

	float xFov = (-0.015 * relativeYaw * relativeYaw) + (0.01 * abs(relativeYaw)) + 150; // https://www.desmos.com/calculator/tc3c5mtds2
	float yFov = (0.005 * relativePitch * relativePitch) + (-abs(relativePitch)) + 100; // https://www.desmos.com/calculator/amal9mekga

	if (xFov == 0 || yFov == 0) { return result; }

	float screenY = relativePitch / (yFov * 0.5 * localPlayer->zoom);
	screenY = (screenY + 1) / 2;
	screenY *= screenHeight;

	float screenX = relativeYaw / (xFov * 0.5 * localPlayer->zoom);

	screenX = (screenX + 1) / 2;
	screenX *= screenWidth;

	result.x = screenX;
	result.y = screenY;

	return result;
}

Player* GetClosestPlayer()
{
	if (!IsValidPlayer(localPlayer)) { return nullptr; }

	float minDistance = 999999999999.0f;

	Player* targetPlayer = 0;

	for (int i = 0; i < maxPlayerCount; i++)
	{
		Player* player = GetPlayer(i);
		if (!CanAimbotPlayer(player)) { continue; }

		Vector3 diffWorld = localPlayer->pos - player->pos;
		float distance = sqrt((diffWorld.x * diffWorld.x) + (diffWorld.y * diffWorld.y) + (diffWorld.z * diffWorld.z));

		if (targetClosestToCrosshair)
		{
			Vector2 crosshair = { screenWidth / 2, screenHeight / 2 };
			Vector2 diffScreen = crosshair - GetPlayerScreenPos(player, true);
			distance = sqrt((diffScreen.x * diffScreen.x) + (diffScreen.y * diffScreen.y));
		}

		if (distance < minDistance)
		{
			minDistance = distance;
			targetPlayer = player;
		}
	}

	return targetPlayer;
}

void MoveViewAngles(float targetPitch, float targetYaw, float speed, bool useTolerance)
{
	INPUT input = { 0 };
	input.type = INPUT_MOUSE;
	MOUSEINPUT mouseInput = { 0 };
	mouseInput.dwFlags = MOUSEEVENTF_MOVE;

	float deltaPitch = targetPitch - localPlayer->pitch;
	float deltaYaw = localPlayer->yaw - targetYaw;

	if (deltaYaw > 180) { deltaYaw -= 360; }
	if (deltaYaw < -180) { deltaYaw += 360; }

	float deltaY = deltaPitch * speed;
	float deltaX = deltaYaw * speed;

	if (deltaY > 0 && deltaY < 1) { deltaY = 1; }
	if (deltaY < 0 && deltaY > -1) { deltaY = -1; }
	if (deltaX > 0 && deltaX < 1) { deltaX = 1; }
	if (deltaX < 0 && deltaX > -1) { deltaX = -1; }

	if (useTolerance)
	{
		if (deltaPitch > -moveViewAnglesTolerance && deltaPitch < moveViewAnglesTolerance) { deltaY = 0; }
		if (deltaYaw > -moveViewAnglesTolerance && deltaYaw < moveViewAnglesTolerance) { deltaX = 0; }
	}

	mouseInput.dy = deltaY;
	mouseInput.dx = deltaX;

	input.mi = mouseInput;
	SendInput(1, &input, sizeof(INPUT));
}

void Aimbot(Player* targetPlayer)
{
	Vector3 localPlayerPos = GetPlayerPosition(localPlayer, true);
	Vector3 targetPlayerPos = GetPlayerPosition(targetPlayer, headShots);

	PredictPosition(targetPlayer, targetPlayerPos);

	Vector3 diff = localPlayerPos - targetPlayerPos;
	float distance = sqrt((diff.x * diff.x) + (diff.y * diff.y) + (diff.z * diff.z));
	if (distance == 0) { return; }

	float pitch = -(asin((targetPlayerPos.z - localPlayerPos.z) / distance) * rToD);
	float yaw = (atan2(targetPlayerPos.y - localPlayerPos.y, targetPlayerPos.x - localPlayerPos.x) * rToD);

	MoveViewAngles(pitch, yaw, aimbotStrength, true);
}

void ESP(ImDrawList* drawList)
{
	if (drawList == nullptr) { return; }

	for (int i = 0; i < maxPlayerCount; i++)
	{
		Player* player = GetPlayer(i);

		if (!IsValidPlayer(player) || player == localPlayer || (!targetSameTeam && player->team == localPlayer->team)) { continue; }

		Vector2 screenBodyPos = GetPlayerScreenPos(player, false);
		Vector2 screenHeadPos = GetPlayerScreenPos(player, true);

		Vector3 diff = localPlayer->pos - player->pos;
		float distance = sqrt((diff.x * diff.x) + (diff.y * diff.y) + (diff.z * diff.z));
		if (distance == 0) { continue; }

		float depth = (distance * 0.01 * localPlayer->zoom);
		float sizeX = (screenWidth * 0.05) / depth;
		float sizeY = ((screenHeight * 0.375) + player->headHeight) / depth;
		float headRadius = 30 / depth;

		if ((screenBodyPos.y < -sizeY || screenBodyPos.y > screenHeight) || (screenBodyPos.x < -sizeX || screenBodyPos.x > screenWidth)) { continue; }

		ImU32 color;
		if (player->health > 80) { color = IM_COL32(0, 255, 0, 255); }
		else if (player->health > 30) { color = IM_COL32(255, 255, 0, 255); }
		else { color = IM_COL32(255, 0, 0, 255); }

		if (!hideEspInfo)
		{
			std::string healthStr = "Health: " + std::to_string(player->health);
			drawList->AddText(ImVec2(screenBodyPos.x - sizeX, screenHeadPos.y - 35 - headRadius), color, healthStr.c_str());

			std::string distStr = "Distance: " + std::to_string((int)distance);
			drawList->AddText(ImVec2(screenBodyPos.x - sizeX, screenHeadPos.y - 25 - headRadius), color, distStr.c_str());
		}

		if (showPlayerNames)
		{
			const char* playerName = (const char*)(GetPlayerController(i) + playerNameOffset);
			drawList->AddText(ImVec2(screenBodyPos.x - sizeX, screenHeadPos.y - 15 - headRadius), color, playerName);
		}

		drawList->AddCircle(ImVec2(screenHeadPos.x, screenHeadPos.y), headRadius, color);

		drawList->AddRect(ImVec2(screenBodyPos.x - sizeX, screenBodyPos.y), ImVec2(screenBodyPos.x + sizeX, screenBodyPos.y + sizeY), color);
	}
}