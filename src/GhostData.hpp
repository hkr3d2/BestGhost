#pragma once
#include <Geode/Geode.hpp>
#include <vector>
#include <string>

using namespace geode::prelude;

struct GhostFrame {
    float x, y, rotation, timeOffset;
    bool isHolding;
    int gameMode;
};

struct GhostData {
    std::string levelID;
    float percentage;
    std::string timestamp;
    std::vector<GhostFrame> frames;
    
    bool saveToFile(const std::string& path);
    static std::optional<GhostData> loadFromFile(const std::string& path);
};
