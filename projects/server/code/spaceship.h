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
        Laser(uint32_t uuid, uint64_t start_time, uint64_t duration, glm::vec3 pos, glm::quat direction)
            : uuid(uuid), position(pos), direction(direction), start_time(start_time), end_time(start_time + duration)
        {
            transform = glm::translate(pos) * glm::mat4_cast(direction);
        }

        uint32_t uuid;	        // Unique universal identifier of the laser.
        uint64_t start_time;	// The UNIX time in ms when the laser was created.
        uint64_t end_time;	    // The UNIX time in ms when the laser should die.
        glm::vec3 position;		// the position of the laser.
        glm::quat direction;	// The quaternion direction of the laser.

        glm::mat4 transform = glm::mat4(1);
        float Speed = 10.0f;
        bool marked_for_deletion = false;

        /*void Update(float dt)
        {
            position += (glm::vec3)direction * Speed * dt;
            transform[3] = glm::vec4(position, 1);

            uint64_t current_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            if (current_time >= end_time) {
                marked_for_deletion = true;
            }

            // debug draw collision rays
            Debug::DrawLine(position - (glm::vec3)direction * 0.5f, position + (glm::vec3)direction * 0.5f, 1.0f, glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1), Debug::RenderMode::AlwaysOnTop);
        }

        bool CheckCollisions()
        {
            Physics::RaycastPayload payload = Physics::Raycast(position - (glm::vec3)direction * 0.5f, direction, 1);
            if (payload.hit)
            {
                marked_for_deletion = true;
            }

            return payload.hit;
        }*/

        void Update(float dt)
        {
            // Get the forward vector (Z-axis) from the quaternion direction
            glm::vec3 forward = direction * glm::vec3(0, 0, 1);
            // Move the laser in the direction it's facing
            position += forward * Speed * dt;
            // Update the transformation matrix with the new position and direction
            transform = glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(direction);

            uint64_t current_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            if (current_time >= end_time) {
                marked_for_deletion = true;
            }

            // Debug draw collision rays using the forward vector from the quaternion
            Debug::DrawLine(position - forward * 0.5f, position + forward * 0.5f, 1.0f, glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1), Debug::RenderMode::AlwaysOnTop);
        }

        bool CheckCollisions()
        {
            // Get the forward vector (Z-axis) from the quaternion
            glm::vec3 forward = direction * glm::vec3(0, 0, 1);

            // Perform a raycast in the direction the laser is facing
            Physics::RaycastPayload payload = Physics::Raycast(position - forward * 0.5f, forward, 1);
            if (payload.hit) {
                marked_for_deletion = true;
            }

            return payload.hit;
        }
    };

    struct SpaceShip
    {
        SpaceShip() = default;
        ~SpaceShip() = default;
        SpaceShip& operator=(const SpaceShip& other) {
            if (this != &other) {
                position = other.position;
                orientation = other.orientation;
                camPos = other.camPos;
                transform = other.transform;
                linearVelocity = other.linearVelocity;
                currentSpeed = other.currentSpeed;
                // Copy other relevant fields as needed
                uuid = other.uuid;
                peer = other.peer; // Consider how you want to manage peer ownership
                player = other.player; // Assuming Protocol::Player has a proper copy assignment operator
            }
            return *this;
        }

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

        uint64_t lastFireTime = 0;  // Stores the time of the last fired shot
        float fireRate = 200.0f;    // Fire rate in milliseconds (e.g., 200 ms between shots)

        float radius = 0.75f;

        uint16_t bitmap = 0;
        uint32_t uuid;
        ENetPeer* peer;
        Protocol::Player player;

        void Update(float dt);

        bool CheckCollisions(std::vector<Laser>& lasers, std::vector<SpaceShip>& ships);

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