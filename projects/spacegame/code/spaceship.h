#pragma once
#include "render/model.h"
#include <iostream>
#include <chrono>
#include "render/physics.h"
#include "render/debugrender.h"

namespace Render
{
    struct ParticleEmitter;
}

namespace Game
{

    struct Laser {
        Laser(uint32_t uuid, uint64_t start_time, uint64_t end_time, glm::vec3 pos, glm::quat direction)
            : uuid(uuid), position(pos), direction(direction), start_time(start_time), end_time(end_time)
        {
            transform = glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(direction);
        }

        uint32_t uuid;	        // Unique universal identifier of the laser.
        uint64_t start_time;	// The UNIX time in ms when the laser was created.
        uint64_t end_time;	    // The UNIX time in ms when the laser should die.
        glm::vec3 position;		// the position of the laser.
        glm::quat direction;	// The quaternion direction of the laser.

        glm::mat4 transform = glm::mat4(1);
        float Speed = 10.0f;
        bool marked_for_deletion = false;

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
        SpaceShip();
        SpaceShip(int32_t uuid , glm::vec3 pos, glm::vec3 vel, glm::vec3 acc, glm::quat ori);
        ~SpaceShip();
        SpaceShip& operator=(const SpaceShip& other) {
            if (this != &other) {
                position = other.position;
                orientation = other.orientation;
                camPos = other.camPos;
                transform = other.transform;
                linearVelocity = other.linearVelocity;
                currentSpeed = other.currentSpeed;
                rotationZ = other.rotationZ;
                rotXSmooth = other.rotXSmooth;
                rotYSmooth = other.rotYSmooth;
                rotZSmooth = other.rotZSmooth;
                uuid = other.uuid;
                // Handle particle emitters if necessary
                // Assuming we need to copy the pointers (make sure to manage ownership correctly)
                particleEmitterLeft = other.particleEmitterLeft;
                particleEmitterRight = other.particleEmitterRight;
            }
            return *this;
        }

        glm::vec3 position = glm::vec3(0);
        glm::quat orientation = glm::identity<glm::quat>();
        glm::vec3 camPos = glm::vec3(0, 1.0f, -2.0f);
        glm::mat4 transform = glm::mat4(1);
        glm::vec3 linearVelocity = glm::vec3(0);

        uint64_t lastUpdateTime;
        glm::vec3 lastPosition;
        glm::quat lastOrientation;
        glm::vec3 lastVelocity;
        glm::vec3 lastAcceleration;

        glm::vec3 predictedPosition;
        glm::quat predictedOrientation;

        float timeSinceLastPacket = 0;

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

        uint32_t uuid;

        Render::ParticleEmitter* particleEmitterLeft;
        Render::ParticleEmitter* particleEmitterRight;
        float emitterOffset = -0.5f;

        void Update(float dt, uint32_t id);

        void UpdateThrusters(float dt);

        glm::vec3 PredictPosition();

        void ApplyInterpolation(float dt);

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