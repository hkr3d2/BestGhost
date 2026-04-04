#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "../GhostData.hpp"

using namespace geode::prelude;

class $modify(PlayLayer) {
    struct Fields {
        std::vector<GhostFrame> m_currentRecording;
        float m_bestPercentage = 0;
        bool m_isRecording = false;
        float m_startTime = 0;
    };
    
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        
        // Start recording
        m_fields->m_isRecording = true;
        m_fields->m_currentRecording.clear();
        m_fields->m_startTime = m_gameState.m_time;
        
        // Load best percentage for this level
        auto saveDir = Mod::get()->getSaveDir();
        auto bestPath = saveDir / (level->m_levelID + "_best.bin");
        
        if (std::filesystem::exists(bestPath)) {
            auto data = GhostData::loadFromFile(bestPath.string());
            if (data) {
                m_fields->m_bestPercentage = data->percentage;
                log::info("Best Ghost: Best for this level is {}%", m_fields->m_bestPercentage);
            }
        }
        
        log::info("Best Ghost: Recording started for level: {}", level->m_levelName);
        
        return true;
    }
    
    void update(float dt) {
        PlayLayer::update(dt);
        
        if (!m_fields->m_isRecording) return;
        if (!m_player1) return;
        
        float currentTime = m_gameState.m_time;
        float currentPercent = m_gameState.m_percentage;
        
        // Record frame every ~1/60 second
        static float lastRecordTime = 0;
        if (currentTime - lastRecordTime >= 0.016f) {
            lastRecordTime = currentTime;
            
            GhostFrame frame;
            frame.x = m_player1->getPositionX();
            frame.y = m_player1->getPositionY();
            frame.rotation = m_player1->getRotation();
            frame.timeOffset = currentTime - m_fields->m_startTime;
            frame.isHolding = m_player1->m_isHolding;
            frame.gameMode = static_cast<int>(m_gameState.m_gameMode);
            
            m_fields->m_currentRecording.push_back(frame);
        }
        
        // Check if this is a new best
        if (currentPercent > m_fields->m_bestPercentage) {
            m_fields->m_bestPercentage = currentPercent;
            log::info("Best Ghost: New best! {}%", currentPercent);
        }
    }
    
    void destroyPlayer(PlayerObject* player, GameObject* object) {
        float finalPercent = m_gameState.m_percentage;
        
        // Save if this was a new best
        if (finalPercent > m_fields->m_bestPercentage && !m_fields->m_currentRecording.empty()) {
            GhostData data;
            data.levelID = m_level->m_levelID;
            data.percentage = finalPercent;
            data.frames = m_fields->m_currentRecording;
            
            auto saveDir = Mod::get()->getSaveDir();
            auto bestPath = saveDir / (data.levelID + "_best.bin");
            
            if (data.saveToFile(bestPath.string())) {
                log::info("Best Ghost: Saved new best at {}%", finalPercent);
                Notification::create(fmt::format("New best: {}%", finalPercent), NotificationIcon::Success)->show();
            }
        }
        
        m_fields->m_isRecording = false;
        PlayLayer::destroyPlayer(player, object);
    }
    
    void levelComplete() {
        // Save 100% completion
        if (!m_fields->m_currentRecording.empty()) {
            GhostData data;
            data.levelID = m_level->m_levelID;
            data.percentage = 100.0f;
            data.frames = m_fields->m_currentRecording;
            
            auto saveDir = Mod::get()->getSaveDir();
            auto bestPath = saveDir / (data.levelID + "_best.bin");
            data.saveToFile(bestPath.string());
            
            log::info("Best Ghost: Level completed! Saved 100% ghost");
            Notification::create("Level completed! Ghost saved.", NotificationIcon::Success)->show();
        }
        
        PlayLayer::levelComplete();
    }
};
