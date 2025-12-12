#pragma once
#include <string>

#include "SFML/Graphics.hpp"

class Component
{
public:
    virtual ~Component() = default;
    std::string name;

protected:
    Component(const std::string& n) : name(n) {}
};

