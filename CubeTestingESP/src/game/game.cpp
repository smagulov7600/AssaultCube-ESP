#define NOMINMAX // TO PREVENT WINDOWS.H FROM DEFINING MIN AND MAX (IT WILL BREAK STD::MAX)

#include "game.h"
#include "../math/math.h"
#include "DirectXMath.h"
#include "../window/window.hpp"

#include <iostream>
#include <iomanip> // For debugging purposes (std::hex)
#include <algorithm>  // For std::max
#include <sstream>

std::vector<std::string> processes = { "ac_client.exe" };
memify mem(processes);

namespace offset {
	uint32_t client, dwLocalPlayer = 0x0017E0A8, dwViewMatrix = 0x0057DFD0, dwEntityList = 0x18AC04;
	uint32_t playerName = 0x205, playerHealth = 0xEC, playerPosition = 0x2C, isDead = 0x0318;
}
int HEAD_CIRCLE_SIZE = 15;
ImColor boxColor = ImColor(205, 116, 255);
ImColor headColor = ImColor(1, 136, 255);


void AssaultCube::Setup()
{
	auto process_name = mem.GetProcessName();
	using namespace offset;
	client = mem.GetBase(process_name);

	if (client) {
		std::cout << process_name << ": " << std::hex << client << std::endl;
	} else {
		std::cout << "client not found\n";
	}
}

void AssaultCube::RunESP(HWND windowHandle)
{
	if (!offset::client) {
		std::cout << "client not found\n";
		return;
	}

	uint32_t entityList = mem.Read<uint32_t>(offset::client + offset::dwEntityList);
	if (!entityList) {
		//std::cout << "entity List not found\n";
		return;
	}

	auto view_matrix = mem.Read<Matrix>(offset::dwViewMatrix);

	auto player_count = mem.Read<int>(offset::client + 0x18AC0C);
	uint32_t localPlayer = mem.Read<uint32_t>(entityList + offset::dwLocalPlayer);

	for (int i = 0; i < player_count; i++) {
		uint32_t player = mem.Read<uint32_t>(entityList + (i * 0x4));
		if (player == localPlayer) {
			//std::cout << "Local player\n";
			continue;
		}

		int healthValue = mem.Read<int>(player + offset::playerHealth);

		if (mem.Read<bool>(player + offset::isDead) or healthValue <= 0) {
			continue;
		}

		float x = mem.Read<float>(player + 0x28);
		float y = mem.Read<float>(player + 0x2C);
		float z = mem.Read<float>(player + 0x30);
		Vec3 enemyPos = Vec3(x, y, z);

		float x2 = mem.Read<float>(player + 0x4);
		float y2 = mem.Read<float>(player + 0x8);
		float z2 = mem.Read<float>(player + 0xC);
		Vec3 headPos = Vec3(x2, y2, z2);

		Vec2 vScreen, vHead;
		RECT windowRect;
		int systemWidth = 0;
		int systemHeight = 0;
		if (GetWindowRect(windowHandle, &windowRect)) {
			systemWidth = windowRect.right - windowRect.left;
			systemHeight = windowRect.bottom - windowRect.top;
		}

		if (world_to_screen(enemyPos, &vScreen, view_matrix, systemWidth, systemHeight)) {
			if (world_to_screen(headPos, &vHead, view_matrix, systemWidth, systemHeight)) {
				//ImGui::GetForegroundDrawList()->AddCircle({ vHead.x, vHead.y }, HEAD_CIRCLE_SIZE, headColor); // TURN THIS ON IF YOU WANT TO RENDER HEADS
				
				float rectHeight = vHead.y - vScreen.y;
				float rectWidth = rectHeight / 2.4f;

				float midpointY = (vHead.y + vScreen.y) / 2;

				// Calculate corners of the rectangle
				ImVec2 rectMin(vScreen.x - rectWidth / 2, midpointY - rectHeight / 2);
				ImVec2 rectMax(vScreen.x + rectWidth / 2, midpointY + rectHeight / 2);

				ImGui::GetForegroundDrawList()->AddRect(rectMin, rectMax, boxColor);

				char buffer[16] = {};
				mem.ReadRaw(player + offset::playerName, buffer, sizeof(buffer));

				float centerX = rectMin.x + rectWidth / 2;
				ImVec2 nTextPos(rectMin.x + 5, rectMax.y - rectHeight / 2 - 8);
				ImGui::GetForegroundDrawList()->AddText(nTextPos, boxColor, buffer);

				std::string health = std::to_string(healthValue) + "/100";
				const char* buffer2 = health.c_str();

				ImVec2 textSize = ImGui::CalcTextSize(buffer2);
				ImVec2 hTextPos(centerX - textSize.x / 2, rectMax.y - textSize.y);
				ImGui::GetForegroundDrawList()->AddText(hTextPos, boxColor, buffer2);
			}
		}
	}
}