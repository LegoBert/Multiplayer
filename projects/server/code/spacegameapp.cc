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

        std::vector<Laser*> Lasers;

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
                        SpawnSpaceShip(event.peer);
                        break;
                    case ENET_EVENT_TYPE_RECEIVE:
                        /*printf("A packet of length %u containing %s was received from %x:%u on channel %u.\n",
                            event.packet->dataLength,
                            event.packet->data,
                            event.peer->address.host,
                            event.peer->address.port,
                            event.channelID);*/
                        ProcessReceivedPacket(event.packet->data, event.packet->dataLength);
                        break;
                    case ENET_EVENT_TYPE_DISCONNECT:
                        printf("%x:%u disconnected.\n",
                            event.peer->address.host,
                            event.peer->address.port);
                        // Despawn ships here!!!
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

    void SpaceGameApp::SpawnSpaceShip(ENetPeer* peer) {
        SpaceShip ship;
        ship.id = Game::uuid++;
        ship.peer = peer;
        Physics::ColliderMeshId ShipCollider = Physics::LoadColliderMesh("assets/space/spaceship_physics.glb");
        ship.collider = Physics::CreateCollider(ShipCollider, ship.transform);
        ship.Teleport();
        SpaceGameApp::players.push_back(ship);
        SendClientConnectS2C(ship.id, peer);
        //Send gamestate 2 new player
        //inform other clients about new ship

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

    void SpaceGameApp::SendClientConnectS2C(uint16_t uuid,ENetPeer* peer) {
        flatbuffers::FlatBufferBuilder builder;
        auto idPacket = Protocol::CreateClientConnectS2C(builder, uuid, chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count());
        auto packetWrapper = Protocol::CreatePacketWrapper(builder, Protocol::PacketType_ClientConnectS2C, idPacket.Union());

        builder.Finish(packetWrapper);
        ENetPacket* packet = enet_packet_create(builder.GetBufferPointer(), builder.GetSize(), ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
        enet_peer_send(peer, 0, packet);
    }

    //void SpaceGameApp::SendGameStateS2C(ENetPeer* peer) {
    //    flatbuffers::FlatBufferBuilder builder;
    //    Protocol::GameStateS2CT state;
    //    for (SpaceShip player : SpaceGameApp::players) {
    //        state.players.push_back(Protocol::Player(player.id, player.position, player.linearVelocity, player., player.));
    //    }
    //    for (GLaser laser : SpaceGameApp::lasers) {
    //        //laser.uuid, laser.start_time, laser.end_time, laser.origin, laser.direction
    //        state.lasers.push_back(Protocol::Laser());
    //    }
    //    auto gameStatePacket = Protocol::CreateGameStateS2C(builder, state);
    //    auto packetWrapper = Protocol::CreatePacketWrapper(builder, Protocol::PacketType_GameStateS2C, gameStatePacket.Union());

    //    builder.Finish(packetWrapper);
    //    ENetPacket* packet = enet_packet_create(builder.GetBufferPointer(), builder.GetSize(), ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
    //    enet_peer_send(peer, 0, packet);
    //}

} // namespace Game