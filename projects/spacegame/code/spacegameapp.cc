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
#include <proto.h>
#include <iostream>

using namespace Display;
using namespace Render;
using namespace std;


namespace Game
{

    //------------------------------------------------------------------------------
    
    SpaceGameApp::SpaceGameApp(){ }

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
        cam->view = glm::lookAt(glm::vec3(50,0,0), glm::vec3(0), glm::vec3(0,1,0));

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
        for (int i = 0; i < 100; i++)
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
        }

        // Setup asteroids far
        for (int i = 0; i < 50; i++)
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
        }

        // Setup skybox
        std::vector<const char*> skybox
        {
            "assets/space/bg.png",
            "assets/space/bg.png",
            "assets/space/bg.png",
            "assets/space/bg.png",
            "assets/space/bg.png",
            "assets/space/bg.png"
        };
        TextureResourceId skyboxId = TextureResource::LoadCubemap("skybox", skybox, true);
        RenderDevice::SetSkybox(skyboxId);

        Input::Keyboard* kbd = Input::GetDefaultKeyboard();

        const int numLights = 40;
        Render::PointLightId lights[numLights];
        // Setup lights
        for (int i = 0; i < numLights; i++)
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
        }

        //SpaceShip ship;
        ////ship.model = LoadModel("assets/space/spaceship.glb");
        ///**/Physics::ColliderMeshId ShipCollider = Physics::LoadColliderMesh("assets/space/spaceship_physics.glb");
        //ship.collider = Physics::CreateCollider(ShipCollider, ship.transform);
        //spaceShips.push_back(ship);

        ModelId shipModel = LoadModel("assets/space/spaceship.glb");
        ModelId laserModel = LoadModel("assets/space/laser.glb");

        std::clock_t c_start = std::clock();
        double dt = 0.01667f;

        //ENet setup
        if (enet_initialize() != 0)
        {
            fprintf(stderr, "An error Occured while initializing ENet!\n");
        }

        atexit(enet_deinitialize);

        client = enet_host_create(NULL, 1, 1, 0, 0);

        if (client == NULL)
        {
            fprintf(stderr, "An error occurred while trying to create ENet client!\n");
        }

        // game loop
        while (this->window->IsOpen())
        {
            /// <Event>
            while (enet_host_service(client, &event, 0) > 0)
            {
                switch (event.type)
                {
                    case ENET_EVENT_TYPE_RECEIVE:
                        /*printf("A packet of length %u containing %s was received from %x:%u on channel %u.\n",
                        event.packet->dataLength,
                        event.packet->data,
                        event.peer->address.host,
                        event.peer->address.port,
                        event.channelID);*/
                        ProcessReceivedPacket(event.packet->data, event.packet->dataLength);
                        break;
                }
            }
            /// </Event>

            auto timeStart = std::chrono::steady_clock::now();
            glClear(GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);

            this->window->Update();

            if (kbd->pressed[Input::Key::Code::End])
            {
                ShaderResource::ReloadShaders();
            }

            if (peer && peer->state == ENET_PEER_STATE_CONNECTED)
            {
                SendInputToServer(kbd, duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count());
            }
            
            ////Shoot laser
            //uint64_t start_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            //if (kbd->pressed[Input::Key::Code::Space])
            //{
            //    Lasers.push_back(new Laser(ship.transform, start_time));
            //}

            /*ship.Update(dt);
            ship.CheckCollisions();*/

            // Draw some debug text
            Debug::DrawDebugText("Center", glm::vec3(0), { 1,0,0,1 });
            printf("amount of players in clients vector %u\n", SpaceGameApp::spaceShips.size());

            // Store all drawcalls in the render device
            for (auto const& asteroid : asteroids)
            {
                RenderDevice::Draw(std::get<0>(asteroid), std::get<2>(asteroid));
            }

            // Update and draw all lasers
            for (Laser& laser : SpaceGameApp::lasers)
            {
                //Update here
                RenderDevice::Draw(laserModel, laser.transform);
            }

            // Update and draw all ships
            for (SpaceShip& ship : SpaceGameApp::spaceShips)
            {
                //Update here
                if(SpaceGameApp::playerID == ship.uuid)
                    ship.Update(dt);
                RenderDevice::Draw(shipModel, ship.transform);
                //printf("ship id: %u pos: %f %f %f \n", ship.uuid, ship.position.x, ship.position.y, ship.position.z);
                ship.CheckCollisions();
            }

            // Execute the entire rendering pipeline
            RenderDevice::Render(this->window, dt);

            // transfer new frame to window
            this->window->SwapBuffers();

            auto timeEnd = std::chrono::steady_clock::now();
            dt = std::min(0.04, std::chrono::duration<double>(timeEnd - timeStart).count());

            if (kbd->pressed[Input::Key::Code::Escape])
                this->Exit();
        }

        //Disconnect
        if (peer != nullptr && peer->state == ENET_PEER_STATE_CONNECTED) {
            enet_peer_disconnect(peer, 0);
            while (enet_host_service(client, &event, 3000) > 0)
            {
                switch (event.type)
                {
                case ENET_EVENT_TYPE_RECEIVE:
                    enet_packet_destroy(event.packet);
                    break;
                case ENET_EVENT_TYPE_DISCONNECT:
                    puts("Disconnection succeeded.");
                    break;
                }
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
            ImVec2 windowSize(200, 105);
            ImGui::SetNextWindowSize(windowSize);
            ImGui::Begin("Server Connection");

            static char ip[16] = "192.168.1.103";
            static int port = 7777;

            ImGui::InputText("IP-address", ip, sizeof(ip));
            ImGui::InputInt("Port", &port, 1);

            if (ImGui::Button("Connect"))
            {
                // Check if the peer is already connected
                if (peer && peer->state == ENET_PEER_STATE_CONNECTED)
                {
                    puts("Already connected to the server.");
                }
                else
                {
                    printf("Connecting to %s\n", ip);
                    enet_address_set_host(&address, ip);
                    address.port = port;

                    peer = enet_host_connect(client, &address, 1, 0);
                    if (peer == NULL)
                    {
                        fprintf(stderr, "No available peers for initiating an ENEt connection! \n");
                    }

                    //Check if server has contacted us
                    if (enet_host_service(client, &event, 5000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT)
                    {
                        puts("Connection successful");
                    }
                    else
                    {
                        enet_peer_reset(peer);
                        puts("Connection failed.");
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Disconnect") && peer != nullptr && peer->state == ENET_PEER_STATE_CONNECTED)
            {
                enet_peer_disconnect(peer, 0);
                while (enet_host_service(client, &event, 100) > 0)
                {
                    switch (event.type)
                    {
                    case ENET_EVENT_TYPE_RECEIVE:
                        enet_packet_destroy(event.packet);
                        break;
                    case ENET_EVENT_TYPE_DISCONNECT:
                        puts("Disconnection succeeded.");
                        SpaceGameApp::spaceShips.clear();
                        SpaceGameApp::lasers.clear();
                        playerID = -1;
                        break;
                    }
                }
            }

            ImGui::End();
        }
    }

    void SpaceGameApp::SendInputToServer(Input::Keyboard* kbd, uint64_t currentTime) {
        uint16_t bitmap = 0;
        // Iterate over the keys to build the bitmap of pressed keys
        if (kbd->held[Input::Key::W]){
            bitmap |= (1 << 0);
        }
        if (kbd->held[Input::Key::A]) {
            bitmap |= (1 << 1);
        }
        if (kbd->held[Input::Key::D]) {
            bitmap |= (1 << 2);
        }
        if (kbd->held[Input::Key::Up]) {
            bitmap |= (1 << 3);
        }
        if (kbd->held[Input::Key::Down]) {
            bitmap |= (1 << 4);
        }
        if (kbd->held[Input::Key::Left]) {
            bitmap |= (1 << 5);
        }
        if (kbd->held[Input::Key::Right]) {
            bitmap |= (1 << 6);
        }
        if (kbd->held[Input::Key::Space]) {
            bitmap |= (1 << 7);
        }
        if (kbd->held[Input::Key::Shift]) {
            bitmap |= (1 << 8);
        }

        flatbuffers::FlatBufferBuilder builder;
        auto inputPacket = Protocol::CreateInputC2S(builder, currentTime, bitmap);
        auto packetWrapper = Protocol::CreatePacketWrapper(builder, Protocol::PacketType_InputC2S, inputPacket.Union());

        builder.Finish(packetWrapper);
        ENetPacket* packet = enet_packet_create(builder.GetBufferPointer(), builder.GetSize(), ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
        enet_peer_send(peer, 0, packet);
    }

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
            case Protocol::PacketType_ClientConnectS2C:
            {
                // Deserialize InputC2S packet
                const auto packet = packetWrapper->packet_as_ClientConnectS2C();
                if (packet)
                {
                    printf("id recived: %u\n", packet->uuid());
                    SpaceGameApp::playerID = packet->uuid();
                }
                break;
            }
            case Protocol::PacketType_GameStateS2C:
            {
                const auto packet = packetWrapper->packet_as_GameStateS2C();
                if (packet)
                {
                    printf("Gamestate recived\n");
                    auto players = packet->players();
                    auto lasers = packet->lasers();
                    for (auto p : *players) {
                        const auto& position = p->position();
                        const auto& velocity = p->velocity();
                        const auto& acceleration = p->acceleration();
                        const auto& orientation = p->direction();
                        SpaceGameApp::spaceShips.push_back(SpaceShip(
                            p->uuid(),
                            glm::vec3(position.x(), position.y(), position.z()),
                            glm::vec3(velocity.x(), velocity.y(), velocity.z()),
                            glm::vec3(acceleration.x(), acceleration.y(), acceleration.z()),
                            glm::quat(orientation.w(), orientation.x(), orientation.y(), orientation.z())
                        ));
                    }
                }
                break;
            }
            case Protocol::PacketType_SpawnPlayerS2C:
            {
                const auto packet = packetWrapper->packet_as_SpawnPlayerS2C();
                if (packet)
                {
                    printf("Spawn ship with id: %u\n", packet->player()->uuid());
                    const auto& position = packet->player()->position();
                    const auto& velocity = packet->player()->velocity();
                    const auto& acceleration = packet->player()->acceleration();
                    const auto& orientation = packet->player()->direction();
                    SpaceGameApp::spaceShips.push_back(SpaceShip(
                        packet->player()->uuid(),
                        glm::vec3(position.x(), position.y(), position.z()),
                        glm::vec3(velocity.x(), velocity.y(), velocity.z()),
                        glm::vec3(acceleration.x(), acceleration.y(), acceleration.z()),
                        glm::quat(orientation.w(), orientation.x(), orientation.y(), orientation.z())
                    ));
                }
                break;
            }
            case Protocol::PacketType_DespawnPlayerS2C:
            {
                const auto packet = packetWrapper->packet_as_DespawnPlayerS2C();
                if (packet)
                {
                    printf("despawn ship with id: %u\n", packet->uuid());
                    /*for (int i = 0; i < SpaceGameApp::spaceShips.size(); i++) {
                        if (packet->uuid() == SpaceGameApp::spaceShips[i].uuid) {
                            SpaceGameApp::spaceShips.erase(SpaceGameApp::spaceShips.begin() + i);
                            break;
                        }
                    }*/
                }
                break;
            }
            default:
                printf("Received unknown packet type.\n");
                break;
        }
    }

} // namespace Game