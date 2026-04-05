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

std::filesystem::path getGhostPath(GJGameLevel* level) {
    auto ghostDir = Mod::get()->getSaveDir() / "ghosts";
    if (!std::filesystem::exists(ghostDir)) std::filesystem::create_directories(ghostDir);
    return ghostDir / (std::to_string(level->m_levelID.value()) + ".dat");
}

class $modify(MyPlayLayer, PlayLayer) {
    struct Fields {
        CCSprite* m_ghostVisual = nullptr;
        CCLabelBMFont* m_statusLabel = nullptr;
        size_t m_playbackIndex = 0;
        int m_sessionAttempt = 0; // Tracks attempts in this specific session
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        
        g_currentAttemptData.clear(); 
        g_bestAttemptData.clear();
        g_bestXAttained = 0.0f;
        m_fields->m_playbackIndex = 0;
        m_fields->m_sessionAttempt = 1; // Start at attempt 1

        // 1. LOAD GHOST FROM DISK
        auto path = getGhostPath(level);
        if (std::filesystem::exists(path)) {
            std::ifstream file(path, std::ios::binary);
            GhostFrame frame;
            while (file.read(reinterpret_cast<char*>(&frame), sizeof(GhostFrame))) {
                g_bestAttemptData.push_back(frame);
            }
            if (!g_bestAttemptData.empty()) g_bestXAttained = g_bestAttemptData.back().x;
        }

        // 2. CREATE STATUS LABEL
        auto label = CCLabelBMFont::create("", "bigFont.fnt");
        label->setScale(0.4f);
        label->setOpacity(150);
        label->setPosition({ CCDirector::get()->getWinSize().width / 2, 40 });
        this->addChild(label, 100);
        m_fields->m_statusLabel = label;

        // 3. CREATE GHOST SPRITE
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
        if (!m_fields->m_statusLabel) return;
        
        // If it's the first attempt of the session, we are just warming up/recording
        if (m_fields->m_sessionAttempt == 1) {
            m_fields->m_statusLabel->setString("Attempt 1: Calibrating...");
            m_fields->m_statusLabel->setColor({ 255, 255, 100 }); // Yellow for calibration
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
        
        // Only save data IF it wasn't the first "calibration" attempt
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

        // Increment session attempt
        m_fields->m_sessionAttempt++;
        
        g_currentAttemptData.clear(); 
        m_fields->m_playbackIndex = 0;
        if (m_fields->m_ghostVisual) m_fields->m_ghostVisual->setVisible(false);
        
        updateStatusUI();
    }

    void levelComplete() {
        PlayLayer::levelComplete();
        // Only save victory if it wasn't the skipped first attempt
        if (m_fields->m_sessionAttempt > 1) {
            g_bestAttemptData = g_currentAttemptData;
            auto path = getGhostPath(this->m_level);
            std::ofstream file(path, std::ios::binary);
            for (auto& frame : g_bestAttemptData) {
                file.write(reinterpret_cast<const char*>(&frame), sizeof(GhostFrame));
            }
        }
    }
};

class $modify(MyBaseGameLayer, GJBaseGameLayer) {
    void update(float dt) {
        GJBaseGameLayer::update(dt);
        auto playLayer = typeinfo_cast<PlayLayer*>(this);
        if (!playLayer || playLayer->m_isPaused) return;

        auto player = playLayer->m_player1;
        if (!player) return;

        auto myPL = static_cast<MyPlayLayer*>(static_cast<CCNode*>(playLayer));

        // RECORDING: Skip storing data if it's attempt 1
        if (myPL->m_fields->m_sessionAttempt > 1) {
            g_currentAttemptData.push_back({ player->getPositionX(), player->getPositionY() });
        }

        // PLAYBACK: Skip showing ghost if it's attempt 1
        if (myPL->m_fields->m_sessionAttempt > 1 && myPL->m_fields->m_ghostVisual && !g_bestAttemptData.empty()) {
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
};
