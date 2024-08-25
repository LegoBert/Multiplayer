#pragma once
//------------------------------------------------------------------------------
/**
	Space game application

	(C) 20222 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/app.h"
#include "render/window.h"
#include "enet/enet.h"
#include "render/input/inputserver.h"

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
	void SendInputToServer(Input::Keyboard* kbd, uint64_t currentTime);

	Display::Window* window;
	ENetHost* client = nullptr;
	ENetAddress address;
	ENetEvent event;
	ENetPeer* peer = nullptr;
};
} // namespace Game