#pragma once

#include "../components/drawing.hpp"

class GridSystem
{
public:
    static meshComponent* setupGridMesh(uint16_t gridSize)
    {
        auto mesh = new meshComponent();
        float startingValue = -1.0f, x = -1.0f, y = -1.0f;
        float increment = 2.0f / gridSize ;
        int index = 0;
        for(int i = 0; i < gridSize; i++){
            for(int j = 0; j < gridSize; j++){
                mesh->vertices.push_back({{x, y, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}});
                mesh->vertices.push_back({{x+increment, y, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}});
                mesh->vertices.push_back({{x+increment, y+increment, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}});
                mesh->vertices.push_back({{x, y+increment, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}});

                mesh->indices.push_back(0+index);
                mesh->indices.push_back(1+index);
                mesh->indices.push_back(2+index);
                mesh->indices.push_back(2+index);
                mesh->indices.push_back(3+index);
                mesh->indices.push_back(0+index);
                index += 4;
                x += increment;
            }
            y += increment;
            x = startingValue;
        }
        return mesh;
    }
};
