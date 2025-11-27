#include "../../include/Item.h"
#include "../../include/Player.h"
#include <iostream>

Item::Item(const std::string& name, ItemType type, const std::string& description)
    : m_name(name), m_type(type), m_description(description), m_value(0) {}

Item::~Item() {}
