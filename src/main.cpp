#include "components/movement.hpp"
#include "components/drawing.hpp"
#include "entity_registry.hpp"
#include "systems/gridSystem.hpp"
#include "GameEngine.hpp"

int main()
{
    auto registry = std::make_shared<entity_registry>();
        
    //Grid
    uint32_t gridEntityID = registry->createEntity();
    
    auto grid = new gridComponent();
    grid->width = 10;
    grid->height = 10;

    //TODO make gridComponent and its mesh work together
    auto gridMesh = GridSystem::setupGridMesh(10);

    registry->addComponent<meshComponent>(gridMesh, gridEntityID);
    registry->addComponent<gridComponent>(grid, gridEntityID);

    //Unit (red triangle for the moment)
    auto unitEntityID = registry->createEntity();
    auto unitGridPosition = new gridPosition2DComponent();
    unitGridPosition->x = 5;
    unitGridPosition->y = 5;
    auto unitMovementSpeed = new movementSpeedComponent();
    unitMovementSpeed->movementSpeed = 10.0f;
    auto unitMesh = new meshComponent();
    unitMesh->vertices.push_back({{-0.05f, -0.05f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}});
    unitMesh->vertices.push_back({{0.05f, -0.05f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}});
    unitMesh->vertices.push_back({{0.05f, 0.05f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}});
    unitMesh->indices.push_back(0);
    unitMesh->indices.push_back(1);
    unitMesh->indices.push_back(2);

    registry->addComponent(unitGridPosition, unitEntityID); //unused, to add with instancing
    registry->addComponent(unitMovementSpeed, unitEntityID);//TODO add support
    registry->addComponent(unitMesh, unitEntityID);

    //Building (blue triangle for the moment)
    auto buildingEntityID = registry->createEntity();
    auto buildingGridPosition = new gridPosition2DComponent();
    buildingGridPosition->x = 4;
    buildingGridPosition->y = 5;
    auto buildingMesh = new meshComponent();
    buildingMesh->vertices.push_back({{-0.10f, -0.10f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}});
    buildingMesh->vertices.push_back({{-0.05f, -0.10f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}});
    buildingMesh->vertices.push_back({{-0.05f, -0.05f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}});
    buildingMesh->indices.push_back(0);
    buildingMesh->indices.push_back(1);
    buildingMesh->indices.push_back(2);

    registry->addComponent(buildingGridPosition, buildingEntityID); //unused, to add with instancing
    registry->addComponent(buildingMesh, buildingEntityID);
        
    GameEngine app(registry);
    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


