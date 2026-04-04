#include "GhostData.hpp"
#include <fstream>
#include <Geode/loader/JsonValidation.hpp>

using json = nlohmann::json;

bool GhostData::saveToFile(const std::string& path) {
    json j;
    j["levelID"] = levelID;
    j["percentage"] = percentage;
    j["timestamp"] = timestamp;
    j["attemptsAtSave"] = attemptsAtSave;
    
    json framesArray = json::array();
    for (const auto& frame : frames) {
        framesArray.push_back({
            {"x", frame.x},
            {"y", frame.y},
            {"rotation", frame.rotation},
            {"isHolding", frame.isHolding},
            {"gameMode", frame.gameMode},
            {"timeOffset", frame.timeOffset}
        });
    }
    j["frames"] = framesArray;
    
    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << j.dump(4);
    return true;
}

std::optional<GhostData> GhostData::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return std::nullopt;
    
    json j;
    file >> j;
    
    GhostData data;
    data.levelID = j["levelID"];
    data.percentage = j["percentage"];
    data.timestamp = j["timestamp"];
    data.attemptsAtSave = j["attemptsAtSave"];
    
    for (const auto& frameJson : j["frames"]) {
        GhostFrame frame;
        frame.x = frameJson["x"];
        frame.y = frameJson["y"];
        frame.rotation = frameJson["rotation"];
        frame.isHolding = frameJson["isHolding"];
        frame.gameMode = frameJson["gameMode"];
        frame.timeOffset = frameJson["timeOffset"];
        data.frames.push_back(frame);
    }
    
    return data;
}

std::string GhostData::getFilename(float percentage) {
    // Replace decimal with underscore for filename safety
    std::string pctStr = std::to_string(static_cast<int>(percentage));
    return pctStr + "_percent.ghost";
}
