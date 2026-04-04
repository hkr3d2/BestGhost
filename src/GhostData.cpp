#include "GhostData.hpp"
#include <Geode/Geode.hpp>
#include <fstream>

using namespace geode::prelude;

bool GhostData::saveToFile(const std::string& path) {
    auto json = matjson::Object();
    json["levelID"] = levelID;
    json["percentage"] = percentage;
    json["timestamp"] = timestamp;
    json["attemptsAtSave"] = attemptsAtSave;
    
    auto framesArray = matjson::Array();
    for (const auto& frame : frames) {
        framesArray.push_back(matjson::Object{
            {"x", frame.x},
            {"y", frame.y},
            {"rotation", frame.rotation},
            {"isHolding", frame.isHolding},
            {"gameMode", frame.gameMode},
            {"timeOffset", frame.timeOffset}
        });
    }
    json["frames"] = framesArray;
    
    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << matjson::serialize(json);
    return true;
}

std::optional<GhostData> GhostData::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return std::nullopt;
    
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    auto json = matjson::parse(content);
    if (!json.isObject()) return std::nullopt;
    
    GhostData data;
    data.levelID = json["levelID"].asString();
    data.percentage = json["percentage"].asDouble();
    data.timestamp = json["timestamp"].asString();
    data.attemptsAtSave = json["attemptsAtSave"].asInt();
    
    for (const auto& frameJson : json["frames"].asArray()) {
        GhostFrame frame;
        frame.x = frameJson["x"].asDouble();
        frame.y = frameJson["y"].asDouble();
        frame.rotation = frameJson["rotation"].asDouble();
        frame.isHolding = frameJson["isHolding"].asBool();
        frame.gameMode = frameJson["gameMode"].asInt();
        frame.timeOffset = frameJson["timeOffset"].asDouble();
        data.frames.push_back(frame);
    }
    
    return data;
}

std::string GhostData::getFilename(float percentage) {
    return fmt::format("{}_percent.ghost", static_cast<int>(percentage));
}
