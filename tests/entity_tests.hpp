#include "../src/entity_registry.hpp"
#include "../src/components/movement.hpp"

#include <cassert>
#include <iostream>

class entity_tests
{
public:
    int run()
    {
        entity_registry myRegistry{};
        auto entityID = myRegistry.createEntity();

        //add a component to our entity
        auto testPosition = new gridPosition2DComponent();
        testPosition->x = 12;
        testPosition->y = 15;
        myRegistry.addComponent(testPosition, entityID);

        std::cout << "testing entity registry..." << std::endl;
        //make sure we can access it properly from our entity and component list
        assert(((gridPosition2DComponent*)myRegistry.getComponents<gridPosition2DComponent>()[0].getData())->x == testPosition->x);
        assert(((gridPosition2DComponent*)myRegistry.getComponents(entityID)[0]->getData())->x == testPosition->x);
        std::cout << "finished testing entity registry" << std::endl;
        return 0;
    }
};
