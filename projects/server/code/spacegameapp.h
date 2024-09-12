#pragma once
//------------------------------------------------------------------------------
/**
	Space game application

	(C) 20222 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/app.h"
#include "render/window.h"
#include "spaceship.h"
#include "enet/enet.h"
#include <proto.h>

namespace Game
{
class SpaceGameApp : public Core::App
{
public:
	/// constructor
	SpaceGameApp();
	/// destructor
	~SpaceGameApp();

	/// open app
	bool Open();
	/// run app
	void Run();
	/// exit app
	void Exit();
private:
	/// show some ui things
	void RenderUI();
	void ProcessReceivedPacket(const void* data, size_t dataLength);
	void SpawnSpaceShip(uint32_t uuid, ENetPeer* peer);

	void SendClientConnectS2C(uint16_t uuid, ENetPeer* peer);
	void SendGameStateS2C(std::vector<SpaceShip>& spaceShips, std::vector<Laser>& lasers, ENetPeer* peer);
	void SendSpawnPlayerS2C(Protocol::Player* player, std::vector<ENetPeer*> peers);
	void SendDespawnPlayerS2C(uint32_t uuid, std::vector<ENetPeer*> peers);


	Display::Window* window;

	std::vector<ENetPeer*> peers = {};
	std::vector<SpaceShip> spaceShips = {};
	std::vector<Laser> lasers = {};
};

static unsigned int uuid = 0;

} // namespace Game