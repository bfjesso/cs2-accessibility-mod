#pragma once
#include "mathstructs.h"

// offsets valid for the April 30th, 2026 game update

// controller offsets
const unsigned int pawnHandleOffset = 0x6BC;
const unsigned int playerNameOffset = 0x6F0;
const unsigned int isLocalPlayerOffset = 0x780;
const int maxPlayerCount = 64;
const unsigned int bytesBetweenControllers = 0x70;

// bone offsets
const unsigned int boneListOffset = 0x1D0;
const unsigned int bytesPerBone = 0x20;
const int headBoneIndex = 7;

// player offsets
const unsigned int gameSceneOffset = 0x330;
const unsigned int healthOffset = 0x34C;
const unsigned int velocityOffset = 0x3FC;
const unsigned int teamOffset = 0xBB0;
const unsigned int headHeightOffset = 0xD7C;
const unsigned int pitchOffset = 0x1298;
const unsigned int yawOffset = 0x129C;
const unsigned int zoomOffset = 0x1388;
const unsigned int posOffset = 0x1390;

const float maxHeadHeight = 72;

enum Team
{
	Terrorist = 6,
	CounterTerrorist = 3
};

struct Player
{
	char pad1[gameSceneOffset];
	uintptr_t gameScene;
	
	char pad2[healthOffset - gameSceneOffset - sizeof(gameScene)];
	int health;

	char pad3[velocityOffset - healthOffset - sizeof(health)];
	Vector3 velocity;

	char pad4[teamOffset - velocityOffset - sizeof(velocity)];
	int team; // 6 = terrorist, 3 = counter terrorist

	char pad5[headHeightOffset - teamOffset - sizeof(team)];
	float headHeight; // 72 when standing, 54 when crouched

	char pad6[pitchOffset - headHeightOffset - sizeof(headHeight)];
	float pitch; // -89 when looking up, 89 when looking down. this seems to only exist for the local player
	float yaw;

	char pad7[zoomOffset - yawOffset - sizeof(yaw)];
	float zoom; // 1 when not zoomed, 0.444 first zoom with awp, 0.111 last zoom with awp

	char pad8[posOffset - zoomOffset - sizeof(zoom)];
	Vector3 pos;
};