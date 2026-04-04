#include "GhostData.hpp"
#include <Geode/Geode.hpp>
#include <fstream>

using namespace geode::prelude;

bool GhostData::saveToFile(const std::string& path) {
    auto json = matjson::makeObject();
    json["levelID"] = levelID;
    json["percentage"] = percentage;
    json["timestamp"] = timestamp;
    json["attemptsAtSave"] = attemptsAtSave;
    
    auto framesArray = matjson::makeArray();
    for (const auto& frame : frames) {
        framesArray.push_back(matjson::makeObject({
            {"x", frame.x},
            {"y", frame.y},
            {"rotation", frame.rotation},
            {"isHolding", frame.isHolding},
            {"gameMode", frame.gameMode},
            {"timeOffset", frame.timeOffset}
        }));
    }
    json["frames"] = framesArray;
    
    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << json.dump();
    return true;
}

std::optional<GhostData> GhostData::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return std::nullopt;
    
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    auto json = matjson::parse(content);
    if (!json.isObject()) return std::nullopt;
    
    GhostData data;
    data.levelID = json["levelID"].asString().unwrapOr("");
    data.percentage = json["percentage"].asDouble().unwrapOr(0.0);
    data.timestamp = json["timestamp"].asString().unwrapOr("");
    data.attemptsAtSave = json["attemptsAtSave"].asInt().unwrapOr(0);
    
    auto framesArray = json["frames"].asArray().unwrapOr(matjson::Array());
    for (const auto& frameJson : framesArray) {
        GhostFrame frame;
        frame.x = frameJson["x"].asDouble().unwrapOr(0.0);
        frame.y = frameJson["y"].asDouble().unwrapOr(0.0);
        frame.rotation = frameJson["rotation"].asDouble().unwrapOr(0.0);
        frame.isHolding = frameJson["isHolding"].asBool().unwrapOr(false);
        frame.gameMode = frameJson["gameMode"].asInt().unwrapOr(0);
        frame.timeOffset = frameJson["timeOffset"].asDouble().unwrapOr(0.0);
        data.frames.push_back(frame);
    }
    
    return data;
}

std::string GhostData::getFilename(float percentage) {
    return fmt::format("{}_percent.ghost", static_cast<int>(percentage));
}
