#pragma once
#include <Geode/Geode.hpp>
#include <vector>
#include <string>
#include <optional>

using namespace geode::prelude;

struct GhostFrame {
    float x;
    float y;
    float rotation;
    bool isHolding;
    int gameMode;
    float timeOffset;
};

struct GhostData {
    std::string levelID;
    float percentage;
    std::string timestamp;
    int attemptsAtSave;
    std::vector<GhostFrame> frames;
    
    bool saveToFile(const std::string& path);
    static std::optional<GhostData> loadFromFile(const std::string& path);
    static std::string getFilename(float percentage);
};
