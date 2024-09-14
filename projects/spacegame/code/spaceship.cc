#include "config.h"
#include "spaceship.h"
#include "render/input/inputserver.h"
#include "render/cameramanager.h"
#include "render/physics.h"
#include "render/debugrender.h"
#include "render/particlesystem.h"
#include "proto.h"
#include <random>
#include <cmath>

using namespace Input;
using namespace glm;
using namespace Render;

namespace Game
{
    SpaceShip::SpaceShip()
    {
        uint32_t numParticles = 2048;
        this->particleEmitterLeft = new ParticleEmitter(numParticles);
        this->particleEmitterLeft->data = {
            .origin = glm::vec4(this->position + (vec3(this->transform[2]) * emitterOffset),1),
            .dir = glm::vec4(glm::vec3(-this->transform[2]), 0),
            .startColor = glm::vec4(0.38f, 0.76f, 0.95f, 1.0f) * 2.0f,
            .endColor = glm::vec4(0,0,0,1.0f),
            .numParticles = numParticles,
            .theta = glm::radians(0.0f),
            .startSpeed = 1.2f,
            .endSpeed = 0.0f,
            .startScale = 0.025f,
            .endScale = 0.0f,
            .decayTime = 2.58f,
            .randomTimeOffsetDist = 2.58f,
            .looping = 1,
            .emitterType = 1,
            .discRadius = 0.020f
        };
        this->particleEmitterRight = new ParticleEmitter(numParticles);
        this->particleEmitterRight->data = this->particleEmitterLeft->data;

        ParticleSystem::Instance()->AddEmitter(this->particleEmitterLeft);
        ParticleSystem::Instance()->AddEmitter(this->particleEmitterRight);
    }

    SpaceShip::SpaceShip(int32_t uuid, glm::vec3 pos, glm::vec3 vel, glm::vec3 acc, glm::quat ori) {
        this->uuid = uuid;
        this->position = pos;
        this->linearVelocity = vel;
        this->orientation = ori;
        this->transform = translate(this->position) * (mat4)this->orientation;
        // Particles
        uint32_t numParticles = 2048;
        this->particleEmitterLeft = new ParticleEmitter(numParticles);
        this->particleEmitterLeft->data = {
            .origin = glm::vec4(this->position + (vec3(this->transform[2]) * emitterOffset),1),
            .dir = glm::vec4(glm::vec3(-this->transform[2]), 0),
            .startColor = glm::vec4(0.38f, 0.76f, 0.95f, 1.0f) * 2.0f,
            .endColor = glm::vec4(0,0,0,1.0f),
            .numParticles = numParticles,
            .theta = glm::radians(0.0f),
            .startSpeed = 1.2f,
            .endSpeed = 0.0f,
            .startScale = 0.025f,
            .endScale = 0.0f,
            .decayTime = 2.58f,
            .randomTimeOffsetDist = 2.58f,
            .looping = 1,
            .emitterType = 1,
            .discRadius = 0.020f
        };
        this->particleEmitterRight = new ParticleEmitter(numParticles);
        this->particleEmitterRight->data = this->particleEmitterLeft->data;
        ParticleSystem::Instance()->AddEmitter(this->particleEmitterLeft);
        ParticleSystem::Instance()->AddEmitter(this->particleEmitterRight);
    }

    SpaceShip::~SpaceShip() {
    
    }

    void SpaceShip::Update(float dt)
    {
        Mouse* mouse = Input::GetDefaultMouse();
        Keyboard* kbd = Input::GetDefaultKeyboard();

        Camera* cam = CameraManager::GetCamera(CAMERA_MAIN);

        if (kbd->held[Key::W])
        {
            if (kbd->held[Key::Shift])
                this->currentSpeed = mix(this->currentSpeed, this->boostSpeed, std::min(1.0f, dt * 30.0f));
            else
                this->currentSpeed = mix(this->currentSpeed, this->normalSpeed, std::min(1.0f, dt * 90.0f));
        }
        else
        {
            this->currentSpeed = 0;
        }
        vec3 desiredVelocity = vec3(0, 0, this->currentSpeed);
        desiredVelocity = this->transform * vec4(desiredVelocity, 0.0f);

        this->linearVelocity = mix(this->linearVelocity, desiredVelocity, dt * accelerationFactor);

        float rotX = kbd->held[Key::Left] ? 1.0f : kbd->held[Key::Right] ? -1.0f : 0.0f;
        float rotY = kbd->held[Key::Up] ? -1.0f : kbd->held[Key::Down] ? 1.0f : 0.0f;
        float rotZ = kbd->held[Key::A] ? -1.0f : kbd->held[Key::D] ? 1.0f : 0.0f;

        //this->position += this->linearVelocity * dt * 10.0f;

        const float rotationSpeed = 1.8f * dt;
        rotXSmooth = mix(rotXSmooth, rotX * rotationSpeed, dt * cameraSmoothFactor);
        rotYSmooth = mix(rotYSmooth, rotY * rotationSpeed, dt * cameraSmoothFactor);
        rotZSmooth = mix(rotZSmooth, rotZ * rotationSpeed, dt * cameraSmoothFactor);
        quat localOrientation = quat(vec3(-rotYSmooth, rotXSmooth, rotZSmooth));
        //this->orientation = this->orientation * localOrientation;
        //this->rotationZ -= rotXSmooth;
        //this->rotationZ = clamp(this->rotationZ, -45.0f, 45.0f);
        mat4 T = translate(this->position) * (mat4)this->orientation;
        //this->transform = T * (mat4)quat(vec3(0, 0, rotationZ));
        this->rotationZ = mix(this->rotationZ, 0.0f, dt * cameraSmoothFactor);

        // update camera view transform
        vec3 desiredCamPos = this->position + vec3(this->transform * vec4(0, camOffsetY, -4.0f, 0));
        this->camPos = mix(this->camPos, desiredCamPos, dt * cameraSmoothFactor);
        cam->view = lookAt(this->camPos, this->camPos + vec3(this->transform[2]), vec3(this->transform[1]));

        const float thrusterPosOffset = 0.365f;
        this->particleEmitterLeft->data.origin = glm::vec4(vec3(this->position + (vec3(this->transform[0]) * -thrusterPosOffset)) + (vec3(this->transform[2]) * emitterOffset), 1);
        this->particleEmitterLeft->data.dir = glm::vec4(glm::vec3(-this->transform[2]), 0);
        this->particleEmitterRight->data.origin = glm::vec4(vec3(this->position + (vec3(this->transform[0]) * thrusterPosOffset)) + (vec3(this->transform[2]) * emitterOffset), 1);
        this->particleEmitterRight->data.dir = glm::vec4(glm::vec3(-this->transform[2]), 0);

        float t = (currentSpeed / this->normalSpeed);
        this->particleEmitterLeft->data.startSpeed = 1.2 + (3.0f * t);
        this->particleEmitterLeft->data.endSpeed = 0.0f + (3.0f * t);
        this->particleEmitterRight->data.startSpeed = 1.2 + (3.0f * t);
        this->particleEmitterRight->data.endSpeed = 0.0f + (3.0f * t);
        //this->particleEmitter->data.decayTime = 0.16f;//+ (0.01f  * t);
        //this->particleEmitter->data.randomTimeOffsetDist = 0.06f;/// +(0.01f * t);
    }

    bool SpaceShip::CheckCollisions()
    {
        glm::mat4 rotation = (glm::mat4)orientation;
        bool hit = false;
        for (int i = 0; i < 8; i++)
        {
            glm::vec3 pos = position;
            glm::vec3 dir = rotation * glm::vec4(glm::normalize(colliderEndPoints[i]), 0.0f);
            float len = glm::length(colliderEndPoints[i]);
            Physics::RaycastPayload payload = Physics::Raycast(position, dir, len);

            // debug draw collision rays
            //Debug::DrawLine(pos, pos + dir * len, 1.0f, glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1), Debug::RenderMode::AlwaysOnTop);

            if (payload.hit)
            {
                Debug::DrawDebugText("HIT", payload.hitPoint, glm::vec4(1, 1, 1, 1));
                //Teleport();
                hit = true;
            }
        }
        return hit;
    }

    void SpaceShip::Teleport()
    {
        //Reset Velocity
        currentSpeed = 0.0f;
        linearVelocity = glm::vec3(0);

        //Spawn in a new position
        position = SpawnInRandomPosition(50);

        //Reset orientation to look at the center of the map
        glm::vec3 directionToCenter = glm::normalize(glm::vec3(0.0f, 0.0f, 0.0f) - position);
        glm::quat newOrientation = glm::quatLookAt(-directionToCenter, glm::vec3(0.0f, -1.0f, 0.0f));
        orientation = newOrientation;
    }

    glm::vec3
        SpaceShip::SpawnInRandomPosition(float radius) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::uniform_real_distribution<float> distTheta(0.0f, 2.0f * 3.14f);

        // Generate a random point on the surface of a sphere
        float theta = distTheta(gen);
        float phi = acos(2.0f * dist(gen) - 1.0f);
        float x = radius * sin(phi) * cos(theta);
        float y = radius * sin(phi) * sin(theta);
        float z = radius * cos(phi);

        // Return the spawned position
        return { x, y, z };
    }

}