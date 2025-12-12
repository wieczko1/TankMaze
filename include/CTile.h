#pragma once
#include "Component.h"

class CTile : public Component
{
public:
    enum class Type
    {
        NONE,
        MAP_BORDER,
        WALL,
        WALKABLE
    };

    int id;
    Type type;

    CTile(int id, Type type)
        : Component("Tile"), id(id), type(type)
    {
    }
};