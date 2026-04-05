#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <vector>
#include <fstream>
#include <filesystem>

using namespace geode::prelude;
namespace fs = std::filesystem;

struct GhostFrame {
    float x;
    float y;
};

// Global State
bool g_isRecordingEnabled = false;
std::vector<GhostFrame> g_bestAttemptData;
std::vector<GhostFrame> g_currentAttemptData;
float g_bestXAttained = 0.0f;
float g_accumulator = 0.0f; // For FPS-independent recording

/**
 * File Management
 */
fs::path getGhostPath(int levelID) {
    auto path = Mod::get()->getSaveDir() / "ghosts";
    if (!fs::exists(path)) fs::create_directories(path);
    return path / (std::to_string(levelID) + ".ghst");
}

void saveGhostFile(int levelID) {
    if (g_bestAttemptData.empty()) return;
    std::ofstream file(getGhostPath(levelID), std::ios::binary);
    if (!file.is_open()) return;
    file.write(reinterpret_cast<char*>(&g_bestXAttained), sizeof(float));
    for (const auto& frame : g_bestAttemptData) {
        file.write(reinterpret_cast<const char*>(&frame), sizeof(GhostFrame));
    }
}

void loadGhostFile(int levelID) {
    g_bestAttemptData.clear();
    auto path = getGhostPath(levelID);
    if (!fs::exists(path)) {
        g_bestXAttained = 0.0f;
        return;
    }
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return;
    file.read(reinterpret_cast<char*>(&g_bestXAttained), sizeof(float));
    GhostFrame frame;
    while (file.read(reinterpret_cast<char*>(&frame), sizeof(GhostFrame))) {
        g_bestAttemptData.push_back(frame);
    }
}

/**
 * PlayLayer Hooks
 */
class $modify(MyPlayLayer, PlayLayer) {
    struct Fields {
        CCSprite* m_ghostVisual = nullptr;
        CCLabelBMFont* m_statusLabel = nullptr;
        double m_timeCounter = 0.0;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        loadGhostFile(level->m_levelID.value());
        g_isRecordingEnabled = false;
        m_fields->m_timeCounter = 0.0;
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
        m_fields->m_statusLabel->setVisible(Mod::get()->getSettingValue<bool>("show-indicator"));
        m_fields->m_statusLabel->setString(g_isRecordingEnabled ? "BestGhost: ACTIVE" : "BestGhost: OFF");
        m_fields->m_statusLabel->setColor(g_isRecordingEnabled ? ccColor3B{0, 255, 255} : ccColor3B{200, 200, 200});
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        if (g_isRecordingEnabled && !g_currentAttemptData.empty()) {
            float currentMaxX = g_currentAttemptData.back().x;
            if (currentMaxMaxX > g_bestXAttained) {
                g_bestXAttained = currentMaxX;
                g_bestAttemptData = g_currentAttemptData;
                saveGhostFile(m_level->m_levelID.value());
            }
        }
        g_currentAttemptData.clear();
        m_fields->m_timeCounter = 0.0;
        if (m_fields->m_ghostVisual) m_fields->m_ghostVisual->setVisible(false);
        updateUI();
    }
};

/**
 * PauseLayer Hooks
 */
class $modify(MyPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        auto menu = this->getChildByID("right-button-menu");
        if (!menu) menu = typeinfo_cast<CCMenu*>(this->getChildByType<CCMenu>(0));
        if (menu) {
            auto toggler = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(MyPauseLayer::onToggleGhost), 0.6f);
            toggler->setID("ghost-toggle"_spr);
            toggler->toggle(g_isRecordingEnabled);
            menu->addChild(toggler);
            toggler->setPosition({249.0f, 116.0f}); 
            menu->updateLayout();
        }
    }

    void onToggleGhost(CCObject* sender) {
        g_isRecordingEnabled = !static_cast<CCMenuItemToggler*>(sender)->isOn();
        if (auto pl = PlayLayer::get()) {
            static_cast<MyPlayLayer*>(static_cast<CCNode*>(pl))->updateUI();
        }
    }
};

/**
 * Update Loop with FPS Correction
 */
class $modify(MyBaseGameLayer, GJBaseGameLayer) {
    void update(float dt) {
        GJBaseGameLayer::update(dt);
        if (!g_isRecordingEnabled) return;

        auto playLayer = typeinfo_cast<PlayLayer*>(this);
        if (!playLayer || playLayer->m_isPaused) return;

        auto player = playLayer->m_player1;
        if (!player) return;

        auto myPL = static_cast<MyPlayLayer*>(static_cast<CCNode*>(playLayer));

        // 1. Record Frame (Using dt ensures we know the actual time passed)
        g_currentAttemptData.push_back({ player->getPositionX(), player->getPositionY() });
        
        // Use a persistent counter to track total frames elapsed in this run
        myPL->m_fields->m_timeCounter += 1.0;

        // 2. Playback
        if (myPL->m_fields->m_ghostVisual && !g_bestAttemptData.empty()) {
            
            double offsetBlocks = Mod::get()->getSettingValue<double>("ghost-offset");
            bool fixFPS = Mod::get()->getSettingValue<bool>("fix-fps-offset");

            int frameShift;
            if (fixFPS) {
                // At 60 FPS, 1 block is roughly 4 frames. 
                // We normalize the user's high FPS to a 60 FPS equivalent delay.
                float currentFPS = 1.0f / dt;
                float fpsRatio = currentFPS / 60.0f;
                frameShift = static_cast<int>(offsetBlocks * 4.0 * fpsRatio);
            } else {
                // Raw frame delay (legacy)
                frameShift = static_cast<int>(offsetBlocks * 4.0);
            }
            
            int targetIdx = static_cast<int>(myPL->m_fields->m_timeCounter) + frameShift;

            if (targetIdx >= 0 && targetIdx < static_cast<int>(g_bestAttemptData.size())) {
                myPL->m_fields->m_ghostVisual->setVisible(true);
                myPL->m_fields->m_ghostVisual->setPosition({ 
                    g_bestAttemptData[targetIdx].x, 
                    g_bestAttemptData[targetIdx].y 
                });
            } else {
                myPL->m_fields->m_ghostVisual->setVisible(false);
            }
        }
    }
};

/**
 * Library Listener
 */
$execute {
    listenForSettingChanges<bool>("open-library", [](bool value) {
        if (value) {
            auto path = Mod::get()->getSaveDir() / "ghosts";
            if (!fs::exists(path)) fs::create_directories(path);
            utils::file::openFolder(path);
            Mod::get()->setSettingValue("open-library", false);
        }
    });
}
