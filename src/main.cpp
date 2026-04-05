#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <vector>
#include <fstream>
#include <filesystem>

using namespace geode::prelude;

struct GhostFrame {
    float x;
    float y;
};

// Global session storage
std::vector<GhostFrame> g_bestAttemptData;
std::vector<GhostFrame> g_currentAttemptData;
float g_bestXAttained = 0.0f;

/**
 * Helper: Safely get the ID. User levels sometimes have their ID 
 * in m_levelID, but we must ensure it's not a local editor level (ID 0).
 */
int getValidLevelID(GJGameLevel* level) {
    if (!level) return 0;
    int id = level->m_levelID.value();
    return id;
}

std::filesystem::path getGhostPath(GJGameLevel* level) {
    auto ghostDir = Mod::get()->getSaveDir() / "ghosts";
    if (!std::filesystem::exists(ghostDir)) std::filesystem::create_directories(ghostDir);
    return ghostDir / (std::to_string(getValidLevelID(level)) + ".dat");
}

class $modify(MyPlayLayer, PlayLayer) {
    struct Fields {
        CCSprite* m_ghostVisual = nullptr;
        CCLabelBMFont* m_statusLabel = nullptr;
        size_t m_playbackIndex = 0;
        int m_sessionAttempt = 0; 
        bool m_isValidLevel = false;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        
        int levelID = getValidLevelID(level);
        
        // Disable for Editor/ID 0
        if (levelID <= 0) {
            m_fields->m_isValidLevel = false;
            return true; 
        }

        m_fields->m_isValidLevel = true;
        m_fields->m_sessionAttempt = 1; 
        m_fields->m_playbackIndex = 0;
        g_currentAttemptData.clear(); 
        g_bestAttemptData.clear();
        g_bestXAttained = 0.0f;

        // LOAD GHOST
        auto path = getGhostPath(level);
        if (std::filesystem::exists(path)) {
            std::ifstream file(path, std::ios::binary);
            GhostFrame frame;
            while (file.read(reinterpret_cast<char*>(&frame), sizeof(GhostFrame))) {
                g_bestAttemptData.push_back(frame);
            }
            if (!g_bestAttemptData.empty()) g_bestXAttained = g_bestAttemptData.back().x;
        }

        // UI
        auto label = CCLabelBMFont::create("", "bigFont.fnt");
        label->setScale(0.4f);
        label->setOpacity(150);
        label->setPosition({ CCDirector::get()->getWinSize().width / 2, 40 });
        this->addChild(label, 100);
        m_fields->m_statusLabel = label;

        // GHOST SPRITE
        auto ghost = CCSprite::createWithSpriteFrameName("GJ_square01_001.png"); 
        if (ghost) {
            ghost->setScale(0.6f);
            ghost->setOpacity(130); 
            ghost->setColor({ 0, 255, 255 }); 
            ghost->setVisible(false); 
            m_fields->m_ghostVisual = ghost;
            if (this->m_objectLayer) this->m_objectLayer->addChild(ghost, 1000);
        }

        updateStatusUI();
        return true;
    }

    void updateStatusUI() {
        if (!m_fields->m_statusLabel || !m_fields->m_isValidLevel) return;
        
        if (m_fields->m_sessionAttempt == 1) {
            m_fields->m_statusLabel->setString("Attempt 1: Calibrating...");
            m_fields->m_statusLabel->setColor({ 255, 255, 100 });
        } else {
            if (g_bestAttemptData.empty()) {
                m_fields->m_statusLabel->setString("Ghost: Recording...");
                m_fields->m_statusLabel->setColor({ 255, 100, 100 });
            } else {
                m_fields->m_statusLabel->setString("Ghost: Replaying Best");
                m_fields->m_statusLabel->setColor({ 100, 255, 255 });
            }
        }
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        if (!m_fields->m_isValidLevel) return;

        // Save Logic (Skip Att 1)
        if (m_fields->m_sessionAttempt > 1 && !g_currentAttemptData.empty()) {
            float currentMaxX = g_currentAttemptData.back().x;
            if (currentMaxX > g_bestXAttained) {
                g_bestXAttained = currentMaxX;
                g_bestAttemptData = g_currentAttemptData;

                auto path = getGhostPath(this->m_level);
                std::ofstream file(path, std::ios::binary);
                for (auto& frame : g_bestAttemptData) {
                    file.write(reinterpret_cast<const char*>(&frame), sizeof(GhostFrame));
                }
            }
        }

        m_fields->m_sessionAttempt++;
        g_currentAttemptData.clear(); 
        m_fields->m_playbackIndex = 0;
        if (m_fields->m_ghostVisual) m_fields->m_ghostVisual->setVisible(false);
        updateStatusUI();
    }
};

class $modify(MyBaseGameLayer, GJBaseGameLayer) {
    void update(float dt) {
        GJBaseGameLayer::update(dt);
        auto playLayer = typeinfo_cast<PlayLayer*>(this);
        if (!playLayer || playLayer->m_isPaused) return;

        auto myPL = static_cast<MyPlayLayer*>(static_cast<CCNode*>(playLayer));
        if (!myPL->m_fields->m_isValidLevel) return;

        auto player = playLayer->m_player1;
        if (!player) return;

        // Main Logic (Skip Att 1)
        if (myPL->m_fields->m_sessionAttempt > 1) {
            g_currentAttemptData.push_back({ player->getPositionX(), player->getPositionY() });

            if (myPL->m_fields->m_ghostVisual && !g_bestAttemptData.empty()) {
                size_t index = myPL->m_fields->m_playbackIndex;
                if (index < g_bestAttemptData.size()) {
                    myPL->m_fields->m_ghostVisual->setVisible(true);
                    myPL->m_fields->m_ghostVisual->setPosition({ g_bestAttemptData[index].x, g_bestAttemptData[index].y });
                    myPL->m_fields->m_playbackIndex++;
                } else {
                    myPL->m_fields->m_ghostVisual->setVisible(false);
                }
            }
        }
    }
};
