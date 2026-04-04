#include "GhostData.hpp"
#include <fstream>

using namespace geode::prelude;

bool GhostData::saveToFile(const std::string& path) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    
    // Simple binary save (we'll improve later)
    size_t size = frames.size();
    file.write(reinterpret_cast<const char*>(&percentage), sizeof(percentage));
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));
    file.write(reinterpret_cast<const char*>(frames.data()), sizeof(GhostFrame) * size);
    
    return true;
}

std::optional<GhostData> GhostData::loadFromFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return std::nullopt;
    
    GhostData data;
    size_t size;
    file.read(reinterpret_cast<char*>(&data.percentage), sizeof(data.percentage));
    file.read(reinterpret_cast<char*>(&size), sizeof(size));
    
    data.frames.resize(size);
    file.read(reinterpret_cast<char*>(data.frames.data()), sizeof(GhostFrame) * size);
    
    return data;
}}}
