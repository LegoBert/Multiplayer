#pragma once
#include "render/model.h"
#include <iostream>
#include <chrono>
#include "render/physics.h"
#include "render/debugrender.h"
#include "enet/enet.h"
#include <proto.h>

namespace Render
{
    struct ParticleEmitter;
}

namespace Game
{

    struct Laser {
        Laser(glm::mat4 transform, uint64_t start_time)
            : transform(transform), origin(transform[3]), direction(transform[2]), start_time(start_time), end_time(start_time + 10000)
        {}

        uint32_t uuid;	        // Unique universal identifier of the laser.
        uint64_t start_time;	// The UNIX time in ms when the laser was created.
        uint64_t end_time;	    // The UNIX time in ms when the laser should die.
        glm::vec3 origin;		// Origin position of the laser.
        glm::vec4 direction;	// The quaternion direction of the laser.

        glm::mat4 transform = glm::mat4(1);
        float Speed = 5.0f;

        bool marked_for_deletion = false;

        void Update(float dt)
        {
            origin += (glm::vec3)direction * Speed * dt;
            transform[3] = glm::vec4(origin, 1);

            uint64_t current_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            if (current_time >= end_time) {
                marked_for_deletion = true;
            }
            CheckCollisions();
        }

        bool CheckCollisions()
        {
            Physics::RaycastPayload payload = Physics::Raycast(origin - (glm::vec3)direction * 0.5f, direction, 1);
            if (payload.hit)
            {
                marked_for_deletion = true;
            }

            // debug draw collision rays
            Debug::DrawLine(origin - (glm::vec3)direction * 0.5f, origin + (glm::vec3)direction * 0.5f, 1.0f, glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1), Debug::RenderMode::AlwaysOnTop);
            return payload.hit;
        }

        const glm::vec3 colliderEndPoints[8] = {
            glm::vec3(-1.10657, -0.480347, -0.346542),
            glm::vec3(1.10657, -0.480347, -0.346542),
            glm::vec3(-0.342382, 0.25109, -0.010299),
            glm::vec3(0.342382, 0.25109, -0.010299),
            glm::vec3(-0.285614, -0.10917, 0.869609),
            glm::vec3(0.285614, -0.10917, 0.869609),
            glm::vec3(-0.279064, -0.10917, -0.98846),
            glm::vec3(0.279064, -0.10917, -0.98846)
        };
    };

    struct SpaceShip
    {
        SpaceShip();
        ~SpaceShip() = default;

        glm::vec3 position = glm::vec3(0);
        glm::quat orientation = glm::identity<glm::quat>();
        glm::vec3 camPos = glm::vec3(0, 1.0f, -2.0f);
        glm::mat4 transform = glm::mat4(1);
        glm::vec3 linearVelocity = glm::vec3(0);

        const float normalSpeed = 1.0f;
        const float boostSpeed = normalSpeed * 2.0f;
        const float accelerationFactor = 1.0f;
        const float camOffsetY = 1.0f;
        const float cameraSmoothFactor = 10.0f;

        float currentSpeed = 0.0f;

        float rotationZ = 0;
        float rotXSmooth = 0;
        float rotYSmooth = 0;
        float rotZSmooth = 0;

        uint16_t bitmap = 0;
        uint32_t id;
        ENetPeer* peer;
        Protocol::Player player;

        Render::ModelId model;
        Physics::ColliderId collider;
        /*Render::ParticleEmitter* particleEmitterLeft;
        Render::ParticleEmitter* particleEmitterRight;*/
        float emitterOffset = -0.5f;

        void Update(float dt);

        bool CheckCollisions();

        void Teleport();

        glm::vec3 SpawnInRandomPosition(float radius);

        const glm::vec3 colliderEndPoints[8] = {
            glm::vec3(-1.10657, -0.480347, -0.346542),  // right wing
            glm::vec3(1.10657, -0.480347, -0.346542),  // left wing
            glm::vec3(-0.342382, 0.25109, -0.010299),   // right top
            glm::vec3(0.342382, 0.25109, -0.010299),   // left top
            glm::vec3(-0.285614, -0.10917, 0.869609), // right front
            glm::vec3(0.285614, -0.10917, 0.869609), // left front
            glm::vec3(-0.279064, -0.10917, -0.98846),   // right back
            glm::vec3(0.279064, -0.10917, -0.98846)   // right back
        };
    };

}