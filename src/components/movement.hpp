#pragma once
#include <glm/glm.hpp>

//Locations in 3d space
glm::vec3 comp_position;
glm::vec3 comp_rotation;
glm::vec3 comp_velocity;

//locations relative to a 2d grid in 3d space
//for now assume we'll only ever have 1 grid max, so we don't have to link entities to a specific grid
struct comp_gridPosition2D
{
    uint32_t x;
    uint32_t y;
};

struct comp_grid
{
    uint32_t width;
    uint32_t height;
};

unsigned float movementSpeed;

// struct gridPosition_s_2D
// {
//     std::vector<gridPosition2D> positions;
// }
