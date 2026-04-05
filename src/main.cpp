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
bool g_isRecordingEnabled = false; // Always starts false for a new session
std::vector<GhostFrame> g_bestAttemptData;
std::vector<GhostFrame> g_currentAttemptData;
float g_bestXAttained = 0.0f;

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
        
        // CRITICAL: Reset recording to false every time a level starts
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
        bool isReplay = Mod::get()->getSettingValue<bool>("replay-mode");
        
        m_fields->m_statusLabel->setVisible(Mod::get()->getSettingValue<bool>("show-indicator"));
        
        if (isReplay) {
            m_fields->m_statusLabel->setString("MODE: REPLAY (NO SAVE)");
            m_fields->m_statusLabel->setColor({ 100, 255, 100 });
        } else {
            // Uses the global flag that the user must manually toggle
            m_fields->m_statusLabel->setString(g_isRecordingEnabled ? "BestGhost: RECORDING" : "BestGhost: OFF");
            m_fields->m_statusLabel->setColor(g_isRecordingEnabled ? ccColor3B{0, 255, 255} : ccColor3B{200, 200, 200});
        }
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        bool isReplay = Mod::get()->getSettingValue<bool>("replay-mode");

        if (g_isRecordingEnabled && !isReplay && !g_currentAttemptData.empty()) {
            float currentMaxX = g_currentAttemptData.back().x;
            if (currentMaxX > g_bestXAttained) {
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
            auto settingsSprite = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
            settingsSprite->setScale(0.7f);

            auto settingsBtn = CCMenuItemSpriteExtra::create(
                settingsSprite,
                this,
                menu_selector(MyPauseLayer::onOpenGhostSettings)
            );
            
            settingsBtn->setID("ghost-settings-btn"_spr);
            menu->addChild(settingsBtn);
            settingsBtn->setPosition({249.0f, 116.0f}); 
            menu->updateLayout();
        }
    }

    void onOpenGhostSettings(CCObject* sender) {
        geode::openSettings(Mod::get());
    }
};

/**
 * Update Loop
 */
class $modify(MyBaseGameLayer, GJBaseGameLayer) {
    void update(float dt) {
        GJBaseGameLayer::update(dt);
        
        auto playLayer = typeinfo_cast<PlayLayer*>(this);
        if (!playLayer || playLayer->m_isPaused) return;

        auto player = playLayer->m_player1;
        if (!player) return;

        auto myPL = static_cast<MyPlayLayer*>(static_cast<CCNode*>(playLayer));

        bool isReplay = Mod::get()->getSettingValue<bool>("replay-mode");

        // Uses the volatile global flag
        if (g_isRecordingEnabled && !isReplay) {
            g_currentAttemptData.push_back({ player->getPositionX(), player->getPositionY() });
        }
        
        myPL->m_fields->m_timeCounter += 1.0;

        if (myPL->m_fields->m_ghostVisual && !g_bestAttemptData.empty()) {
            double offsetBlocks = Mod::get()->getSettingValue<double>("ghost-offset");
            bool fixFPS = Mod::get()->getSettingValue<bool>("fix-fps-offset");

            int frameShift;
            if (fixFPS) {
                float currentFPS = (dt > 0) ? (1.0f / dt) : 60.0f;
                float fpsRatio = currentFPS / 60.0f;
                frameShift = static_cast<int>(offsetBlocks * 4.0 * fpsRatio);
            } else {
                frameShift = static_cast<int>(offsetBlocks * 4.0);
            }
            
            int targetIdx = static_cast<int>(myPL->m_fields->m_timeCounter) + frameShift;

            if (targetIdx >= 0 && targetIdx < static_cast<int>(g_bestAttemptData.size())) {
                myPL->m_fields->m_ghostVisual->setVisible(true);
                auto ghostPos = g_bestAttemptData[targetIdx];
                myPL->m_fields->m_ghostVisual->setPosition({ ghostPos.x, ghostPos.y });
            } else {
                myPL->m_fields->m_ghostVisual->setVisible(false);
            }
        }
    }
};

/**
 * Settings Change Listener
 */
$execute {
    // When the setting "ghost-recording" is toggled in the menu, update the GLOBAL flag
    listenForSettingChanges<bool>("ghost-recording", [](bool value) {
        g_isRecordingEnabled = value;
        if (auto pl = PlayLayer::get()) {
             static_cast<MyPlayLayer*>(static_cast<CCNode*>(pl))->updateUI();
        }
    });
    
    listenForSettingChanges<bool>("replay-mode", [](bool value) {
        if (auto pl = PlayLayer::get()) {
             static_cast<MyPlayLayer*>(static_cast<CCNode*>(pl))->updateUI();
        }
    });

    listenForSettingChanges<bool>("open-library", [](bool value) {
        if (value) {
            auto path = Mod::get()->getSaveDir() / "ghosts";
            if (!fs::exists(path)) fs::create_directories(path);
            utils::file::openFolder(path);
            Mod::get()->setSettingValue("open-library", false);
        }
    });
}
