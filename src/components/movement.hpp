#pragma once
#include <glm/glm.hpp>

//Locations in 3d space
struct positionComponent
{
    glm::vec3 position;
};

struct rotationComponent
{
    glm::vec3 rotation;
};

struct velocityComponent
{
    glm::vec3 velocity;
};

//locations relative to a 2d grid in 3d space
//for now assume we'll only ever have 1 grid max, so we don't have to link entities to a specific grid
struct gridPosition2DComponent
{
    uint32_t x;
    uint32_t y;
};

struct gridComponent
{
    uint32_t width;
    uint32_t height;
};

struct movementSpeedComponent
{
    float movementSpeed;
};
