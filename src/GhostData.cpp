#include "GhostData.hpp"
#include <fstream>

bool GhostData::saveToFile(const std::string& path) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    
    // Write level ID length and string
    uint32_t idLen = levelID.size();
    file.write(reinterpret_cast<const char*>(&idLen), sizeof(idLen));
    file.write(levelID.c_str(), idLen);
    
    // Write percentage
    file.write(reinterpret_cast<const char*>(&percentage), sizeof(percentage));
    
    // Write frames
    uint32_t frameCount = frames.size();
    file.write(reinterpret_cast<const char*>(&frameCount), sizeof(frameCount));
    file.write(reinterpret_cast<const char*>(frames.data()), sizeof(GhostFrame) * frameCount);
    
    return true;
}

std::optional<GhostData> GhostData::loadFromFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return std::nullopt;
    
    GhostData data;
    
    // Read level ID
    uint32_t idLen;
    file.read(reinterpret_cast<char*>(&idLen), sizeof(idLen));
    data.levelID.resize(idLen);
    file.read(&data.levelID[0], idLen);
    
    // Read percentage
    file.read(reinterpret_cast<char*>(&data.percentage), sizeof(data.percentage));
    
    // Read frames
    uint32_t frameCount;
    file.read(reinterpret_cast<char*>(&frameCount), sizeof(frameCount));
    data.frames.resize(frameCount);
    file.read(reinterpret_cast<char*>(data.frames.data()), sizeof(GhostFrame) * frameCount);
    
    return data;
}
