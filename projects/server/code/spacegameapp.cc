//------------------------------------------------------------------------------
// spacegameapp.cc
// (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "config.h"
#include "spacegameapp.h"
#include <cstring>
#include "imgui.h"
#include "render/renderdevice.h"
#include "render/shaderresource.h"
#include <vector>
#include "render/textureresource.h"
#include "render/model.h"
#include "render/cameramanager.h"
#include "render/lightserver.h"
#include "render/debugrender.h"
#include "core/random.h"
#include "render/input/inputserver.h"
#include "core/cvar.h"
#include "render/physics.h"
#include <chrono>
#include "spaceship.h"
#include <vector>
#include "enet/enet.h"
#include <proto.h>
#include <utility>

using namespace Display;
using namespace Render;
using namespace std;

namespace Game
{

    //------------------------------------------------------------------------------

    SpaceGameApp::SpaceGameApp() { }

    //------------------------------------------------------------------------------

    SpaceGameApp::~SpaceGameApp() { }

    //------------------------------------------------------------------------------

    bool SpaceGameApp::Open()
    {
        App::Open();
        this->window = new Display::Window;
        this->window->SetSize(2500/2, 2000/2);

        if (this->window->Open())
        {
            // set clear color to gray
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

            RenderDevice::Init();

            // set ui rendering function
            this->window->SetUiRender([this]()
                {
                    this->RenderUI();
                });

            return true;
        }
        return false;
    }

    //------------------------------------------------------------------------------
    
    void SpaceGameApp::Run()
    {
        int w;
        int h;
        this->window->GetSize(w, h);
        glm::mat4 projection = glm::perspective(glm::radians(90.0f), float(w) / float(h), 0.01f, 1000.f);
        Camera* cam = CameraManager::GetCamera(CAMERA_MAIN);
        cam->projection = projection;

        // load all resources
        ModelId models[6] = {
            LoadModel("assets/space/Asteroid_1.glb"),
            LoadModel("assets/space/Asteroid_2.glb"),
            LoadModel("assets/space/Asteroid_3.glb"),
            LoadModel("assets/space/Asteroid_4.glb"),
            LoadModel("assets/space/Asteroid_5.glb"),
            LoadModel("assets/space/Asteroid_6.glb")
        };
        Physics::ColliderMeshId colliderMeshes[6] = {
            Physics::LoadColliderMesh("assets/space/Asteroid_1_physics.glb"),
            Physics::LoadColliderMesh("assets/space/Asteroid_2_physics.glb"),
            Physics::LoadColliderMesh("assets/space/Asteroid_3_physics.glb"),
            Physics::LoadColliderMesh("assets/space/Asteroid_4_physics.glb"),
            Physics::LoadColliderMesh("assets/space/Asteroid_5_physics.glb"),
            Physics::LoadColliderMesh("assets/space/Asteroid_6_physics.glb")
        };

        std::vector<std::tuple<ModelId, Physics::ColliderId, glm::mat4>> asteroids;

        // Setup asteroids near
        /*for (int i = 0; i < 100; i++)
        {
            std::tuple<ModelId, Physics::ColliderId, glm::mat4> asteroid;
            size_t resourceIndex = (size_t)(Core::FastRandom() % 6);
            std::get<0>(asteroid) = models[resourceIndex];
            float span = 20.0f;
            glm::vec3 translation = glm::vec3(
                Core::RandomFloatNTP() * span,
                Core::RandomFloatNTP() * span,
                Core::RandomFloatNTP() * span
            );
            glm::vec3 rotationAxis = normalize(translation);
            float rotation = translation.x;
            glm::mat4 transform = glm::rotate(rotation, rotationAxis) * glm::translate(translation);
            std::get<1>(asteroid) = Physics::CreateCollider(colliderMeshes[resourceIndex], transform);
            std::get<2>(asteroid) = transform;
            asteroids.push_back(asteroid);
        }*/

        // Setup asteroids far
        /*for (int i = 0; i < 50; i++)
        {
            std::tuple<ModelId, Physics::ColliderId, glm::mat4> asteroid;
            size_t resourceIndex = (size_t)(Core::FastRandom() % 6);
            std::get<0>(asteroid) = models[resourceIndex];
            float span = 80.0f;
            glm::vec3 translation = glm::vec3(
                Core::RandomFloatNTP() * span,
                Core::RandomFloatNTP() * span,
                Core::RandomFloatNTP() * span
            );
            glm::vec3 rotationAxis = normalize(translation);
            float rotation = translation.x;
            glm::mat4 transform = glm::rotate(rotation, rotationAxis) * glm::translate(translation);
            std::get<1>(asteroid) = Physics::CreateCollider(colliderMeshes[resourceIndex], transform);
            std::get<2>(asteroid) = transform;
            asteroids.push_back(asteroid);
        }*/

        // Setup skybox
        /*std::vector<const char*> skybox
        {
            "assets/space/bg.png",
            "assets/space/bg.png",
            "assets/space/bg.png",
            "assets/space/bg.png",
            "assets/space/bg.png",
            "assets/space/bg.png"
        };
        TextureResourceId skyboxId = TextureResource::LoadCubemap("skybox", skybox, true);
        RenderDevice::SetSkybox(skyboxId);*/

        Input::Keyboard* kbd = Input::GetDefaultKeyboard();

        const int numLights = 40;
        Render::PointLightId lights[numLights];
        // Setup lights
        /*for (int i = 0; i < numLights; i++)
        {
            glm::vec3 translation = glm::vec3(
                Core::RandomFloatNTP() * 20.0f,
                Core::RandomFloatNTP() * 20.0f,
                Core::RandomFloatNTP() * 20.0f
            );
            glm::vec3 color = glm::vec3(
                Core::RandomFloat(),
                Core::RandomFloat(),
                Core::RandomFloat()
            );
            lights[i] = Render::LightServer::CreatePointLight(translation, color, Core::RandomFloat() * 4.0f, 1.0f + (15 + Core::RandomFloat() * 10.0f));
        }*/

        /*SpaceShip ship;
        ship.model = LoadModel("assets/space/spaceship.glb");
        Physics::ColliderMeshId ShipCollider = Physics::LoadColliderMesh("assets/space/spaceship_physics.glb");
        ship.collider = Physics::CreateCollider(ShipCollider, ship.transform);*/


        ModelId laserModel = LoadModel("assets/space/laser.glb");

        std::clock_t c_start = std::clock();
        double dt = 0.01667f;

        //ENet
        if (enet_initialize() != 0) {
            fprintf(stderr, "An error Occured while initializing ENet!\n");
        }
        atexit(enet_deinitialize);

        ENetAddress address;
        ENetEvent event;
        ENetHost* server;

        address.host = ENET_HOST_ANY;
        address.port = 7777;
        
        int playerLimit = 32;
        server = enet_host_create(&address, playerLimit, 1, 0, 0);

        if (server == NULL) {
            fprintf(stderr, "An error occurred while trying to create the server! \n");
        }

        // game loop
        while (this->window->IsOpen())
        {
            //<Event>
            while (enet_host_service(server, &event, 0) > 0)
            {
                switch (event.type)
                {
                    case ENET_EVENT_TYPE_CONNECT:
                        printf("A new client connected from %x:%u.\n",
                            event.peer->address.host,
                            event.peer->address.port);
                        SendClientConnectS2C(uuid, event.peer);
                        SpawnSpaceShip(uuid, event.peer);
                        uuid++;
                        SendGameStateS2C(SpaceGameApp::spaceShips, SpaceGameApp::lasers, event.peer);
                        break;
                    case ENET_EVENT_TYPE_RECEIVE:
                        ProcessReceivedPacket(event.packet->data, event.packet->dataLength);
                        break;
                    case ENET_EVENT_TYPE_DISCONNECT:
                        printf("%x:%u disconnected.\n",
                            event.peer->address.host,
                            event.peer->address.port);
                        ENetPeer* peerToDespawn = event.peer;
                        //for (int i = 0; i < SpaceGameApp::peers.size(); i++) {
                        //    if (event.peer == SpaceGameApp::peers[i]) {
                        //        SpaceGameApp::peers.erase(SpaceGameApp::peers.begin() + i);
                        //        //printf("peers: %u\n", SpaceGameApp::peers.size());
                        //        break;
                        //    }
                        //}
                        //for (int i = 0; i < SpaceGameApp::spaceShips.size(); i++) {
                        //    if (event.peer == SpaceGameApp::spaceShips[i].peer) {
                        //        SendDespawnPlayerS2C(SpaceGameApp::spaceShips[i].id, SpaceGameApp::peers);
                        //        SpaceGameApp::spaceShips.erase(SpaceGameApp::spaceShips.begin() + i);
                        //        //printf("spaceShips: %u\n", SpaceGameApp::spaceShips.size());
                        //        break;
                        //    }
                        //}
                        printf("spaceShips: %u\n", SpaceGameApp::spaceShips.size());
                        auto it = std::find_if(spaceShips.begin(), spaceShips.end(), [peerToDespawn](const SpaceShip& player) {
                            return player.peer == peerToDespawn;
                        });

                        if (it != spaceShips.end())
                            spaceShips.erase(it);
                        printf("spaceShips: %u\n", SpaceGameApp::spaceShips.size());
                        break;
                }
            }
            //</Event>
            
            auto timeStart = std::chrono::steady_clock::now();
            glClear(GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);

            this->window->Update();
            
            /*if (kbd->pressed[Input::Key::Code::End])
            {
                ShaderResource::ReloadShaders();
            }*/

            //ship.Update(dt);
            //ship.CheckCollisions();

            // Store all drawcalls in the render device
            /*for (auto const& asteroid : asteroids)
            {
                RenderDevice::Draw(std::get<0>(asteroid), std::get<2>(asteroid));
            }*/

            // Update and draw all lasers
            //for (Laser* laser : Lasers)
            //{
            //    laser->Update(dt);
            //    if (laser->marked_for_deletion)
            //        delete laser;
            //    /*else
            //        RenderDevice::Draw(laserModel, laser->transform);*/
            //}
            //Lasers.erase(std::remove_if(Lasers.begin(), Lasers.end(), [](Laser* laser) { return laser->marked_for_deletion; }), Lasers.end());

            //Physics::SetTransform(ship.collider, ship.transform);
            //RenderDevice::Draw(ship.model, ship.transform);

            // Execute the entire rendering pipeline
            //RenderDevice::Render(this->window, dt);

            // transfer new frame to window
            this->window->SwapBuffers();

            auto timeEnd = std::chrono::steady_clock::now();
            dt = std::min(0.04, std::chrono::duration<double>(timeEnd - timeStart).count());
            

            if (kbd->pressed[Input::Key::Code::Escape]) {
                enet_host_destroy(server);
                this->Exit();
            }
                
        }
    }

    //------------------------------------------------------------------------------

    void SpaceGameApp::Exit()
    {
        this->window->Close();
    }

    //------------------------------------------------------------------------------

    void SpaceGameApp::RenderUI()
    {
        if (this->window->IsOpen())
        {
            ImGui::Begin("Debug");
            Core::CVar* r_draw_light_spheres = Core::CVarGet("r_draw_light_spheres");
            int drawLightSpheres = Core::CVarReadInt(r_draw_light_spheres);
            if (ImGui::Checkbox("Draw Light Spheres", (bool*)&drawLightSpheres))
                Core::CVarWriteInt(r_draw_light_spheres, drawLightSpheres);

            Core::CVar* r_draw_light_sphere_id = Core::CVarGet("r_draw_light_sphere_id");
            int lightSphereId = Core::CVarReadInt(r_draw_light_sphere_id);
            if (ImGui::InputInt("LightSphereId", (int*)&lightSphereId))
                Core::CVarWriteInt(r_draw_light_sphere_id, lightSphereId);
            
            ImGui::End();

            Debug::DispatchDebugTextDrawing();
        }
    }

    //------------------------------------------------------------------------------

    void SpaceGameApp::SpawnSpaceShip(uint32_t uuid, ENetPeer* peer) {
        //Create a new SpaceShip instance
        SpaceShip ship;
        ship.id = uuid;
        ship.peer = peer;
        Physics::ColliderMeshId ShipCollider = Physics::LoadColliderMesh("assets/space/spaceship_physics.glb");
        ship.collider = Physics::CreateCollider(ShipCollider, ship.transform);
        ship.Teleport();
        // Create a new player object for network synchronization
        ship.player = Protocol::Player(
            uuid,                                                                                           // Unique player ID
            Protocol::Vec3(ship.position[0], ship.position[1], ship.position[2]),                           // Initial position (x, y, z)
            Protocol::Vec3(0, 0, 0),                                                                        // Initial velocity (x, y, z)
            Protocol::Vec3(0, 0, 0),                                                                        // Initial acceleration (x, y, z)
            Protocol::Vec4(ship.orientation.x, ship.orientation.y, ship.orientation.z, ship.orientation.w)  // Initial rotation (quaternion x, y, z, w)
        );
        SpaceGameApp::spaceShips.push_back(ship);
        // Send a message to all connected peers, informing them of the new player's spawn+
        SendSpawnPlayerS2C(&ship.player, SpaceGameApp::peers);
        // Add the peer to the list of connected peers in the game
        SpaceGameApp::peers.push_back(peer);
    }

    //<ENet functions>
    void SpaceGameApp::ProcessReceivedPacket(const void* data, size_t dataLength)
    {
        // Ensure the data length is valid
        if (dataLength < sizeof(uint16_t)) {
            printf("Received packet is too short.\n");
            return;
        }

        // Create a FlatBuffers buffer from the received data
        auto packetWrapper = Protocol::GetPacketWrapper(data);

        // Check the type of packet received
        switch (packetWrapper->packet_type())
        {
            case Protocol::PacketType_InputC2S:
            {
                // Deserialize InputC2S packet
                const auto inputPacket = packetWrapper->packet_as_InputC2S();
                if (inputPacket)
                {
                    //ship->bitmap = inputPacket->bitmap();
                }
                break;
            }
            default:
                printf("Received unknown packet type.\n");
                break;
        }
    }

    void SpaceGameApp::SendClientConnectS2C(uint16_t uuid, ENetPeer* peer) {
        flatbuffers::FlatBufferBuilder builder;
        auto idPacket = Protocol::CreateClientConnectS2C(builder, uuid, chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count());
        auto packetWrapper = Protocol::CreatePacketWrapper(builder, Protocol::PacketType_ClientConnectS2C, idPacket.Union());

        builder.Finish(packetWrapper);
        ENetPacket* packet = enet_packet_create(builder.GetBufferPointer(), builder.GetSize(), ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
        enet_peer_send(peer, 0, packet);
    }

    void SpaceGameApp::SendGameStateS2C(std::vector<SpaceShip>& spaceShips, std::vector<Laser>& lasers, ENetPeer* peer) {
        flatbuffers::FlatBufferBuilder builder(1024);

        std::vector<Protocol::Player> p;
        std::vector<Protocol::Laser> l;

        for (auto& spaceShip : spaceShips) {
            Protocol::Player player = spaceShip.player;
            p.push_back(player);
        }
        for (auto& laser : lasers) {
            //l.push_back();
        }

        auto playersVec = builder.CreateVectorOfStructs(p.data(), p.size());
        auto lasersVec = builder.CreateVectorOfStructs(l.data(), l.size());

        auto gameState = Protocol::CreateGameStateS2C(builder, playersVec, lasersVec);
        auto packetWrapper = Protocol::CreatePacketWrapper(builder, Protocol::PacketType_GameStateS2C, gameState.Union());

        builder.Finish(packetWrapper);

        ENetPacket* enetPacket = enet_packet_create(builder.GetBufferPointer(), builder.GetSize(), ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(peer, 0, enetPacket);
    }

    void SpaceGameApp::SendSpawnPlayerS2C(Protocol::Player* player, vector<ENetPeer*> peers) {
        flatbuffers::FlatBufferBuilder builder;
        auto telPacket = Protocol::CreateSpawnPlayerS2C(builder, player);
        auto packetWrapper = Protocol::CreatePacketWrapper(builder, Protocol::PacketType_SpawnPlayerS2C, telPacket.Union());

        builder.Finish(packetWrapper);
        ENetPacket* packet = enet_packet_create(builder.GetBufferPointer(), builder.GetSize(), ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
        for (ENetPeer* peer : peers)
            enet_peer_send(peer, 0, packet);
    }

    void SpaceGameApp::SendDespawnPlayerS2C(uint32_t uuid, vector<ENetPeer*> peers) {
        flatbuffers::FlatBufferBuilder builder;
        auto telPacket = Protocol::CreateDespawnPlayerS2C(builder, uuid);
        auto packetWrapper = Protocol::CreatePacketWrapper(builder, Protocol::PacketType_DespawnPlayerS2C, telPacket.Union());

        builder.Finish(packetWrapper);
        ENetPacket* packet = enet_packet_create(builder.GetBufferPointer(), builder.GetSize(), ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
        for (ENetPeer* peer : peers)
            enet_peer_send(peer, 0, packet);
    }

    void SendUpdatePlayerS2C(const Protocol::Player* player, uint64_t time, vector<ENetPeer*> peers) {
        flatbuffers::FlatBufferBuilder builder;
        auto telPacket = Protocol::CreateUpdatePlayerS2C(builder, time, player);
        auto packetWrapper = Protocol::CreatePacketWrapper(builder, Protocol::PacketType_UpdatePlayerS2C, telPacket.Union());

        builder.Finish(packetWrapper);
        ENetPacket* packet = enet_packet_create(builder.GetBufferPointer(), builder.GetSize(), ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
        for (ENetPeer* peer : peers)
            enet_peer_send(peer, 0, packet);
    }

    void SendTeleportPlayerS2C(const Protocol::Player* player, uint64_t time, vector<ENetPeer*> peers) {
        flatbuffers::FlatBufferBuilder builder;
        auto telPacket = Protocol::CreateTeleportPlayerS2C(builder, time, player);
        auto packetWrapper = Protocol::CreatePacketWrapper(builder, Protocol::PacketType_TeleportPlayerS2C, telPacket.Union());

        builder.Finish(packetWrapper);
        ENetPacket* packet = enet_packet_create(builder.GetBufferPointer(), builder.GetSize(), ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
        for (ENetPeer* peer : peers)
            enet_peer_send(peer, 0, packet);
    }

    void SendSpawnLaserS2C(const Protocol::Laser* laser, vector<ENetPeer*> peers) {
        flatbuffers::FlatBufferBuilder builder;
        auto telPacket = Protocol::CreateSpawnLaserS2C(builder, laser);
        auto packetWrapper = Protocol::CreatePacketWrapper(builder, Protocol::PacketType_SpawnLaserS2C, telPacket.Union());

        builder.Finish(packetWrapper);
        ENetPacket* packet = enet_packet_create(builder.GetBufferPointer(), builder.GetSize(), ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
        for (ENetPeer* peer : peers)
            enet_peer_send(peer, 0, packet);
    }

    void SendDespawnLaserS2C(uint32_t uuid, vector<ENetPeer*> peers) {
        flatbuffers::FlatBufferBuilder builder;
        auto telPacket = Protocol::CreateDespawnLaserS2C(builder, uuid);
        auto packetWrapper = Protocol::CreatePacketWrapper(builder, Protocol::PacketType_DespawnLaserS2C, telPacket.Union());

        builder.Finish(packetWrapper);
        ENetPacket* packet = enet_packet_create(builder.GetBufferPointer(), builder.GetSize(), ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
        for(ENetPeer* peer : peers)
            enet_peer_send(peer, 0, packet);
    }

} // namespace Game