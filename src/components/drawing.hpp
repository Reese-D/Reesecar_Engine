#pragma once
#include "graphics_pipeline.hpp"
#include <glm/fwd.hpp>
#include <vector>

struct meshComponent
{
    std::vector<graphics_pipeline::Vertex> vertices;
    std::vector<uint16_t> indices;
};

struct instance2DComponent
{
    glm::vec2 position;
    uint8_t depthLevel{0}; //tiles should be 0, things displayed above them a 1, etc.
    glm::float32 rotation; //2D, so we only rotate on the Z axis.
    glm::float32 scale{ 1.0f };
    //uint32_t texIndex{ 0 }; //if the instance supports multiple different skins?
};
