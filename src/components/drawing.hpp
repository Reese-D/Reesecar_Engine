#pragma once
#include "graphics_pipeline.hpp"

struct meshComponent
{
    std::vector<graphics_pipeline::Vertex> vertices;
    std::vector<uint16_t> indices;
};

struct instance2DComponent
{
    glm::vec2 position;
    uint16_t depthLevel{0}; //tiles should be 0, things displayed above them a 1, etc.
    glm::vec3 rotation;
    float scale{ 0.0f };
    //uint32_t texIndex{ 0 }; //if the instance supports multiple different skins?
};
