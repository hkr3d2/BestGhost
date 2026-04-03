#include "GhostManager.hpp"
#include <Geode/Geode.hpp>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

GhostManager& GhostManager::get() {
    static GhostManager instance;
    return instance;
}

std::string GhostManager::getLevelFolderPath() {
    auto saveDir = Mod::get()->getSaveDir();
    auto levelPath = saveDir / m_currentLevelID;
    
    if (!fs::exists(levelPath)) {
        fs::create_directories(levelPath);
    }
    
    return levelPath.string();
}

void GhostManager::loadGhostsForCurrentLevel() {
    m_ghosts.clear();
    auto folderPath = getLevelFolderPath();
    
    for (const auto& entry : fs::directory_iterator(folderPath)) {
        if (entry.path().extension() == ".ghost") {
            auto ghost = GhostData::loadFromFile(entry.path().string());
            if (ghost.has_value()) {
                m_ghosts[ghost->percentage] = ghost.value();
            }
        }
    }
    
    if (m_onGhostsUpdated) {
        m_onGhostsUpdated();
    }
}

void GhostManager::setCurrentLevel(const std::string& levelID) {
    m_currentLevelID = levelID;
    m_activeGhost = nullptr;
    m_isRacing = false;
    loadGhostsForCurrentLevel();
}

void GhostManager::startRecording() {
    m_isRecording = true;
    m_currentRecording.clear();
    m_recordingStartTime = 0; // Will be set on first frame
}

void GhostManager::recordFrame(float x, float y, float rotation, bool isHolding, int gameMode, float time) {
    if (!m_isRecording) return;
    
    if (m_currentRecording.empty()) {
        m_recordingStartTime = time;
    }
    
    GhostFrame frame;
    frame.x = x;
    frame.y = y;
    frame.rotation = rotation;
    frame.isHolding = isHolding;
    frame.gameMode = gameMode;
    frame.timeOffset = time - m_recordingStartTime;
    
    m_currentRecording.push_back(frame);
}

void GhostManager::stopRecording(float finalPercentage) {
    if (!m_isRecording) return;
    m_isRecording = false;
    
    // Check if this is a new best
    float bestPercentage = 0;
    for (const auto& [pct, _] : m_ghosts) {
        if (pct > bestPercentage) bestPercentage = pct;
    }
    
    if (finalPercentage > bestPercentage) {
        // New best! Save it
        GhostData newGhost;
        newGhost.levelID = m_currentLevelID;
        newGhost.percentage = finalPercentage;
        
        // Get current time as string
        auto now = std::time(nullptr);
        char timeStr[100];
        std::strftime(timeStr, sizeof(timeStr), "%b %d, %I:%M%p", std::localtime(&now));
        newGhost.timestamp = timeStr;
        
        newGhost.attemptsAtSave = 0; // Could track this if desired
        newGhost.frames = m_currentRecording;
        
        auto folderPath = getLevelFolderPath();
        auto filePath = fs::path(folderPath) / GhostData::getFilename(finalPercentage);
        
        if (newGhost.saveToFile(filePath.string())) {
            m_ghosts[finalPercentage] = newGhost;
            
            if (m_onGhostsUpdated) {
                m_onGhostsUpdated();
            }
            
            // Show notification
            Notification::create(fmt::format("🏆 New best saved at {}%", finalPercentage), 
                                 NotificationIcon::Success)->show();
        }
    }
}

void GhostManager::setActiveGhost(float percentage) {
    auto it = m_ghosts.find(percentage);
    if (it != m_ghosts.end()) {
        m_activeGhost = &it->second;
    } else {
        m_activeGhost = nullptr;
    }
}

void GhostManager::setRacing(bool enabled) {
    m_isRacing = enabled;
}

void GhostManager::deleteGhost(float percentage) {
    auto it = m_ghosts.find(percentage);
    if (it != m_ghosts.end()) {
        auto filePath = fs::path(getLevelFolderPath()) / GhostData::getFilename(percentage);
        fs::remove(filePath);
        m_ghosts.erase(it);
        
        if (m_activeGhost == &it->second) {
            m_activeGhost = nullptr;
        }
        
        if (m_onGhostsUpdated) {
            m_onGhostsUpdated();
        }
    }
}

void GhostManager::deleteAllGhosts() {
    auto folderPath = getLevelFolderPath();
    for (const auto& entry : fs::directory_iterator(folderPath)) {
        if (entry.path().extension() == ".ghost") {
            fs::remove(entry.path());
        }
    }
    m_ghosts.clear();
    m_activeGhost = nullptr;
    
    if (m_onGhostsUpdated) {
        m_onGhostsUpdated();
    }
}

void GhostManager::setOnGhostsUpdatedCallback(std::function<void()> callback) {
    m_onGhostsUpdated = callback;
}

bool GhostManager::exportGhost(float percentage, const std::string& path) {
    auto it = m_ghosts.find(percentage);
    if (it == m_ghosts.end()) return false;
    
    return it->second.saveToFile(path);
}

bool GhostManager::importGhost(const std::string& path) {
    auto ghost = GhostData::loadFromFile(path);
    if (!ghost.has_value()) return false;
    
    if (ghost->levelID != m_currentLevelID) return false;
    
    auto savePath = fs::path(getLevelFolderPath()) / GhostData::getFilename(ghost->percentage);
    if (ghost->saveToFile(savePath.string())) {
        m_ghosts[ghost->percentage] = ghost.value();
        
        if (m_onGhostsUpdated) {
            m_onGhostsUpdated();
        }
        
        return true;
    }
    
    return false;
}
