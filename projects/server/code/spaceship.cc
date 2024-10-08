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
    void SpaceShip::Update(float dt) {
        /*Mouse* mouse = Input::GetDefaultMouse();
        Keyboard* kbd = Input::GetDefaultKeyboard();*/

        Camera* cam = CameraManager::GetCamera(CAMERA_MAIN);
        //printf("Received InputC2S packet:  bitmap = %u\n", bitmap);
        if (bitmap & (1 << 0))  // 'W' is bit 0
        {
            if (bitmap & (1 << 8))  // 'Shift' is bit 8
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

        vec3 acceleration = (desiredVelocity - this->linearVelocity) * accelerationFactor;

        // Rotation calculations using bitmap
        float rotX = (bitmap & (1 << 5)) ? 1.0f : (bitmap & (1 << 6)) ? -1.0f : 0.0f;  // Left and Right
        float rotY = (bitmap & (1 << 3)) ? -1.0f : (bitmap & (1 << 4)) ? 1.0f : 0.0f;   // Up and Down
        float rotZ = (bitmap & (1 << 1)) ? -1.0f : (bitmap & (1 << 2)) ? 1.0f : 0.0f;   // A and D

        this->position += this->linearVelocity * dt * 10.0f;

        const float rotationSpeed = 1.8f * dt;
        rotXSmooth = mix(rotXSmooth, rotX * rotationSpeed, dt * cameraSmoothFactor);
        rotYSmooth = mix(rotYSmooth, rotY * rotationSpeed, dt * cameraSmoothFactor);
        rotZSmooth = mix(rotZSmooth, rotZ * rotationSpeed, dt * cameraSmoothFactor);
        quat localOrientation = quat(vec3(-rotYSmooth, rotXSmooth, rotZSmooth));
        this->orientation = this->orientation * localOrientation;
        this->rotationZ -= rotXSmooth;
        this->rotationZ = clamp(this->rotationZ, -45.0f, 45.0f);
        mat4 T = translate(this->position) * (mat4)this->orientation;
        this->transform = T * (mat4)quat(vec3(0, 0, rotationZ));
        this->rotationZ = mix(this->rotationZ, 0.0f, dt * cameraSmoothFactor);

        this->player = Protocol::Player(
            this->uuid,                                                                                         // Unique player ID
            Protocol::Vec3(this->position[0], this->position[1], this->position[2]),                            // Initial position (x, y, z)
            Protocol::Vec3(this->linearVelocity[0], this->linearVelocity[1], this->linearVelocity[2]),          // Initial velocity (x, y, z)
            Protocol::Vec3(acceleration[0], acceleration[1], acceleration[2]),                                     // Initial acceleration (x, y, z)
            Protocol::Vec4(this->orientation.x, this->orientation.y, this->orientation.z, this->orientation.w)  // Initial rotation (quaternion x, y, z, w)
        );

        // Update camera view transform
        vec3 desiredCamPos = this->position + vec3(this->transform * vec4(0, camOffsetY, -4.0f, 0));
        this->camPos = mix(this->camPos, desiredCamPos, dt * cameraSmoothFactor);
        cam->view = lookAt(this->camPos, this->camPos + vec3(this->transform[2]), vec3(this->transform[1]));
    }

    bool SpaceShip::CheckCollisions(std::vector<Laser>& lasers, std::vector<SpaceShip>& ships) {
        glm::mat4 rotation = (glm::mat4)orientation;
        // Check Rock collision
        for (int i = 0; i < 8; i++)
        {
            glm::vec3 pos = position;
            glm::vec3 dir = rotation * glm::vec4(glm::normalize(colliderEndPoints[i]), 0.0f);
            float len = glm::length(colliderEndPoints[i]);
            Physics::RaycastPayload payload = Physics::Raycast(position, dir, len);

            // debug draw collision rays
            Debug::DrawLine(pos, pos + dir * len, 1.0f, glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1), Debug::RenderMode::AlwaysOnTop);

            if (payload.hit)
            {
                Debug::DrawDebugText("HIT", payload.hitPoint, glm::vec4(1, 1, 1, 1));
                Teleport();
                return true;
            }
        }
        for (const Laser& laser : lasers) {
            // Convert quaternion to direction vector
            glm::vec3 laserRotation = laser.direction * glm::vec3(0, 0, 1);
            glm::vec3 p1 = laser.position - laserRotation * 0.5f;
            glm::vec3 p2 = laser.position + laserRotation * 0.5f;

            // Sphere data
            glm::vec3 sphereCenter = this->position;
            float sphereRadius = this->radius;

            // Vector from p1 to p2 (laser direction vector)
            glm::vec3 d = p2 - p1;

            // Vector from p1 to the sphere center
            glm::vec3 f = p1 - sphereCenter;

            // Coefficients of the quadratic equation
            float a = glm::dot(d, d);
            float b = 2.0f * glm::dot(f, d);
            float c = glm::dot(f, f) - sphereRadius * sphereRadius;

            // Discriminant of the quadratic equation
            float discriminant = b * b - 4 * a * c;

            if (discriminant >= 0) {
                // There is an intersection if discriminant is non-negative
                float t1 = (-b - sqrt(discriminant)) / (2 * a);
                float t2 = (-b + sqrt(discriminant)) / (2 * a);

                // Check if intersection occurs within the segment [0, 1]
                if ((t1 >= 0.0f && t1 <= 1.0f) || (t2 >= 0.0f && t2 <= 1.0f)) {
                    // Laser intersects the sphere
                    Teleport();
                    return true;
                    std::cout << "Laser hit the sphere!" << std::endl;
                }
            }
        }
        // Check Ship collision
        for (auto& ship : ships) {
            // Check if the ship is not the current ship
            if (&ship == this) {
                continue;
            }

            float distance = glm::distance(this->position, ship.position);
            if (distance < (ship.radius + this->radius)) {
                ship.Teleport();
                Teleport();
                return true;
            }
        }
        return false;
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
        this->player = this->player = Protocol::Player(
            this->uuid,                                                                                         // Unique player ID
            Protocol::Vec3(this->position[0], this->position[1], this->position[2]),                            // position (x, y, z)
            Protocol::Vec3(0, 0, 0),                                                                            // velocity (x, y, z)
            Protocol::Vec3(0, 0, 0),                                                                            // acceleration (x, y, z)
            Protocol::Vec4(this->orientation.x, this->orientation.y, this->orientation.z, this->orientation.w)  // rotation (quaternion x, y, z, w)
        );
    }

    glm::vec3 SpaceShip::SpawnInRandomPosition(float radius) {
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