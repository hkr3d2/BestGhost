#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <vector>
#include <sstream>

using namespace geode::prelude;

struct GhostFrame {
    float x;
    float y;
};

// Global Settings
bool g_isRecordingEnabled = false;
float g_ghostXOffset = -30.0f; // 1 Block Behind

// Best Run Persistence
std::vector<GhostFrame> g_bestAttemptData;
std::vector<GhostFrame> g_currentAttemptData;
float g_bestXAttained = 0.0f;

/**
 * Data Persistence Helpers
 */
void saveGhostData(int levelID) {
    if (g_bestAttemptData.empty()) return;

    std::string encoded;
    for (const auto& frame : g_bestAttemptData) {
        encoded += std::to_string(frame.x) + "," + std::to_string(frame.y) + "|";
    }

    std::string key = "best_ghost_" + std::to_string(levelID);
    Mod::get()->setSavedValue<std::string>(key, encoded);
    Mod::get()->setSavedValue<float>(key + "_dist", g_bestXAttained);
}

void loadGhostData(int levelID) {
    g_bestAttemptData.clear();
    std::string key = "best_ghost_" + std::to_string(levelID);
    
    g_bestXAttained = Mod::get()->getSavedValue<float>(key + "_dist", 0.0f);
    std::string encoded = Mod::get()->getSavedValue<std::string>(key, "");

    if (encoded.empty()) return;

    std::stringstream ss(encoded);
    std::string segment;
    while (std::getline(ss, segment, '|')) {
        size_t comma = segment.find(',');
        if (comma != std::string::npos) {
            try {
                float x = std::stof(segment.substr(0, comma));
                float y = std::stof(segment.substr(comma + 1));
                g_bestAttemptData.push_back({x, y});
            } catch (...) {
                continue; 
            }
        }
    }
}

/**
 * 1. PlayLayer Hooks
 */
class $modify(MyPlayLayer, PlayLayer) {
    struct Fields {
        CCSprite* m_ghostVisual = nullptr;
        CCLabelBMFont* m_statusLabel = nullptr;
        size_t m_playbackIndex = 0;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        // Load saved ghost for this level
        loadGhostData(level->m_levelID.value());

        g_isRecordingEnabled = false;
        m_fields->m_playbackIndex = 0;
        g_currentAttemptData.clear();

        auto label = CCLabelBMFont::create("", "bigFont.fnt");
        label->setScale(0.35f);
        label->setOpacity(120);
        label->setPosition({ CCDirector::get()->getWinSize().width / 2, 25 });
        this->addChild(label, 100);
        m_fields->m_statusLabel = label;

        auto ghost = CCSprite::createWithSpriteFrameName("GJ_square01_001.png");
        if (ghost) {
            ghost->setScale(0.6f);
            ghost->setOpacity(130);
            ghost->setColor({ 0, 255, 255 });
            ghost->setVisible(false);
            m_fields->m_ghostVisual = ghost;
            if (this->m_objectLayer) this->m_objectLayer->addChild(ghost, 1000);
        }

        updateUI();
        return true;
    }

    void updateUI() {
        if (!m_fields->m_statusLabel) return;
        if (!g_isRecordingEnabled) {
            m_fields->m_statusLabel->setString("BestGhost: OFF");
            m_fields->m_statusLabel->setColor({ 200, 200, 200 });
        } else {
            m_fields->m_statusLabel->setString(g_bestAttemptData.empty() ? "BestGhost: RECORDING..." : "BestGhost: ACTIVE");
            m_fields->m_statusLabel->setColor({ 0, 255, 255 });
        }
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        
        if (g_isRecordingEnabled && !g_currentAttemptData.empty()) {
            float currentMaxX = g_currentAttemptData.back().x;

            if (currentMaxX > g_bestXAttained) {
                g_bestXAttained = currentMaxX;
                g_bestAttemptData = g_currentAttemptData;
                saveGhostData(m_level->m_levelID.value());
            }
        }

        g_currentAttemptData.clear();
        m_fields->m_playbackIndex = 0;
        if (m_fields->m_ghostVisual) m_fields->m_ghostVisual->setVisible(false);
        updateUI();
    }
};

/**
 * 2. PauseLayer Hooks
 */
class $modify(MyPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        auto menu = this->getChildByID("settings-button-menu");
        if (!menu) menu = this->getChildByID("right-button-menu");
        if (!menu) menu = typeinfo_cast<CCMenu*>(this->getChildByType<CCMenu>(0));

        if (menu) {
            auto toggler = CCMenuItemToggler::createWithStandardSprites(
                this, menu_selector(MyPauseLayer::onToggleGhost), 0.6f
            );
            toggler->setID("ghost-toggle"_spr);
            toggler->toggle(g_isRecordingEnabled);
            menu->addChild(toggler);

            // Manual coordinates from your working version
            toggler->setPosition({249.0f, 116.0f}); 
        }
    }

    void onToggleGhost(CCObject* sender) {
        auto toggler = static_cast<CCMenuItemToggler*>(sender);
        g_isRecordingEnabled = !toggler->isOn(); 

        this->onResume(nullptr);
        if (auto pl = PlayLayer::get()) {
            pl->resetLevel();
        }
    }
};

/**
 * 3. Update Loop
 */
class $modify(MyBaseGameLayer, GJBaseGameLayer) {
    void update(float dt) {
        GJBaseGameLayer::update(dt);
        if (!g_isRecordingEnabled) return;

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
                myPL->m_fields->m_ghostVisual->setPosition({ 
                    g_bestAttemptData[index].x + g_ghostXOffset, 
                    g_bestAttemptData[index].y 
                });
                myPL->m_fields->m_playbackIndex++;
            } else {
                myPL->m_fields->m_ghostVisual->setVisible(false);
            }
        }
    }
};
