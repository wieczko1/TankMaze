#pragma once
#include <string>

class Component
{
public:
    virtual ~Component() = default;
    std::string name;

protected:
    Component(const std::string& n) : name(n) {}
};

