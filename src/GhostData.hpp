#pragma once
#include <Geode/Geode.hpp>
#include <vector>
#include <string>
#include <ctime>

using namespace geode::prelude;

struct GhostFrame {
    float x;                // position x
    float y;                // position y
    float rotation;         // player rotation
    bool isHolding;         // is clicking/holding
    int gameMode;           // 0=cube, 1=ship, 2=ball, 3=ufo, 4=wave, 5=robot, 6=spider
    float timeOffset;       // time in seconds since start
};

struct GhostData {
    std::string levelID;
    float percentage;
    std::string timestamp;
    int attemptsAtSave;
    std::vector<GhostFrame> frames;
    
    // Save to file
    bool saveToFile(const std::string& path);
    
    // Load from file
    static std::optional<GhostData> loadFromFile(const std::string& path);
    
    // Get filename based on percentage
    static std::string getFilename(float percentage);
};
