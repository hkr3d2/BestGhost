#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <vector>
#include <fstream>

using namespace geode::prelude;

// Structure for ghost data
struct GhostFrame {
    float x;
    float y;
};

// Global storage
std::vector<GhostFrame> g_bestAttemptData;
std::vector<GhostFrame> g_currentAttemptData;
float g_bestXAttained = 0.0f;

/**
 * Helper: Get the file path for a specific level's ghost
 */
std::filesystem::path getGhostPath(GJGameLevel* level) {
    auto ghostDir = Mod::get()->getSaveDir() / "ghosts";
    if (!std::filesystem::exists(ghostDir)) {
        std::filesystem::create_directories(ghostDir);
    }
    // Files are named based on Level ID: ghost_123.dat
    return ghostDir / (std::to_string(level->m_levelID.value()) + ".dat");
}

/**
 * 1. PlayLayer Hooks
 */
class $modify(MyPlayLayer, PlayLayer) {
    struct Fields {
        CCSprite* m_ghostVisual = nullptr;
        size_t m_playbackIndex = 0;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        
        g_currentAttemptData.clear(); 
        g_bestAttemptData.clear();
        g_bestXAttained = 0.0f;
        m_fields->m_playbackIndex = 0;

        // --- LOAD GHOST FROM DISK ---
        auto path = getGhostPath(level);
        if (std::filesystem::exists(path)) {
            std::ifstream file(path, std::ios::binary);
            GhostFrame frame;
            while (file.read(reinterpret_cast<char*>(&frame), sizeof(GhostFrame))) {
                g_bestAttemptData.push_back(frame);
            }
            if (!g_bestAttemptData.empty()) {
                g_bestXAttained = g_bestAttemptData.back().x;
            }
            log::info("Ghost: Loaded {} frames from disk.", g_bestAttemptData.size());
        }

        // Create Visual
        auto ghost = CCSprite::createWithSpriteFrameName("GJ_square01_001.png"); 
        if (ghost) {
            ghost->setScale(0.6f);
            ghost->setOpacity(130); 
            ghost->setColor({ 0, 255, 255 }); 
            ghost->setVisible(false); 
            m_fields->m_ghostVisual = ghost;
            if (this->m_objectLayer) this->m_objectLayer->addChild(ghost, 1000);
        }

        return true;
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        
        if (!g_currentAttemptData.empty()) {
            float currentMaxX = g_currentAttemptData.back().x;
            if (currentMaxX > g_bestXAttained) {
                g_bestXAttained = currentMaxX;
                g_bestAttemptData = g_currentAttemptData;

                // --- SAVE GHOST TO DISK ---
                auto path = getGhostPath(this->m_level);
                std::ofstream file(path, std::ios::binary);
                for (auto& frame : g_bestAttemptData) {
                    file.write(reinterpret_cast<const char*>(&frame), sizeof(GhostFrame));
                }
                log::info("Ghost: New best saved to disk!");
            }
        }

        g_currentAttemptData.clear(); 
        m_fields->m_playbackIndex = 0;
        if (m_fields->m_ghostVisual) m_fields->m_ghostVisual->setVisible(false);
    }

    void levelComplete() {
        PlayLayer::levelComplete();
        // Force save on 100%
        g_bestAttemptData = g_currentAttemptData;
        auto path = getGhostPath(this->m_level);
        std::ofstream file(path, std::ios::binary);
        for (auto& frame : g_bestAttemptData) {
            file.write(reinterpret_cast<const char*>(&frame), sizeof(GhostFrame));
        }
        log::info("Ghost: Victory run saved to disk.");
    }
};

/**
 * 2. GJBaseGameLayer Hooks
 */
class $modify(MyBaseGameLayer, GJBaseGameLayer) {
    void update(float dt) {
        GJBaseGameLayer::update(dt);
        auto playLayer = typeinfo_cast<PlayLayer*>(this);
        if (!playLayer || playLayer->m_isPaused) return;

        auto player = playLayer->m_player1;
        if (!player) return;

        g_currentAttemptData.push_back({ player->getPositionX(), player->getPositionY() });

        auto myPL = static_cast<MyPlayLayer*>(static_cast<CCNode*>(playLayer));
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
};
