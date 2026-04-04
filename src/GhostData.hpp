#pragma once
#include <Geode/Geode.hpp>
#include <vector>

using namespace geode::prelude;

struct GhostFrame {
    float x = 0;
    float y = 0;
    float rotation = 0;
    float timeOffset = 0;
    bool isHolding = false;
    int gameMode = 0;
};

struct GhostData {
    int levelID = 0;
    float percentage = 0;
    std::vector<GhostFrame> frames;
    
    bool saveToFile(const std::string& path);
    static std::optional<GhostData> loadFromFile(const std::string& path);
};
