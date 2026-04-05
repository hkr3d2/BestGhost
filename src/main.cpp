#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
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
 * PlayerObject Hooks for Invincibility
 */
class $modify(PlayerObject) {
    void playerWillDie(bool p0) {
        if (Mod::get()->getSettingValue<bool>("spectate-mode")) return;
        PlayerObject::playerWillDie(p0);
    }
};

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
        bool isReplay = Mod::get()->getSettingValue<bool>("replay-mode");
        bool isSpectate = Mod::get()->getSettingValue<bool>("spectate-mode");
        
        m_fields->m_statusLabel->setVisible(Mod::get()->getSettingValue<bool>("show-indicator"));
        
        if (isSpectate) {
            m_fields->m_statusLabel->setString("MODE: SPECTATING (GODMODE)");
            m_fields->m_statusLabel->setColor({ 255, 200, 0 });
        } else if (isReplay) {
            m_fields->m_statusLabel->setString("MODE: REPLAY (NO SAVE)");
            m_fields->m_statusLabel->setColor({ 100, 255, 100 });
        } else {
            m_fields->m_statusLabel->setString(g_isRecordingEnabled ? "BestGhost: RECORDING" : "BestGhost: OFF");
            m_fields->m_statusLabel->setColor(g_isRecordingEnabled ? ccColor3B{0, 255, 255} : ccColor3B{200, 200, 200});
        }
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        bool isReplay = Mod::get()->getSettingValue<bool>("replay-mode");
        bool isSpectate = Mod::get()->getSettingValue<bool>("spectate-mode");

        if (g_isRecordingEnabled && !isReplay && !isSpectate && !g_currentAttemptData.empty()) {
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
        bool isSpectate = Mod::get()->getSettingValue<bool>("spectate-mode");

        if (g_isRecordingEnabled && !isReplay && !isSpectate) {
            g_currentAttemptData.push_back({ player->getPositionX(), player->getPositionY() });
        }
        
        myPL->m_fields->m_timeCounter += 1.0;

        if (myPL->m_fields->m_ghostVisual && !g_bestAttemptData.empty()) {
            double offsetBlocks = Mod::get()->getSettingValue<double>("ghost-offset");
            bool fixFPS = Mod::get()->getSettingValue<bool>("fix-fps-offset");

            int frameShift = 0;
            if (!isSpectate) { 
                if (fixFPS) {
                    float currentFPS = (dt > 0) ? (1.0f / dt) : 60.0f;
                    float fpsRatio = currentFPS / 60.0f;
                    frameShift = static_cast<int>(offsetBlocks * 4.0 * fpsRatio);
                } else {
                    frameShift = static_cast<int>(offsetBlocks * 4.0);
                }
            }
            
            int targetIdx = static_cast<int>(myPL->m_fields->m_timeCounter) + frameShift;

            if (targetIdx >= 0 && targetIdx < static_cast<int>(g_bestAttemptData.size())) {
                myPL->m_fields->m_ghostVisual->setVisible(true);
                auto ghostPos = g_bestAttemptData[targetIdx];
                myPL->m_fields->m_ghostVisual->setPosition({ ghostPos.x, ghostPos.y });

                if (isSpectate) {
                    player->setPosition({ ghostPos.x, ghostPos.y });
                }
            } else {
                myPL->m_fields->m_ghostVisual->setVisible(false);
            }
        }
    }

    void pushButton(int p0, bool p1) {
        if (Mod::get()->getSettingValue<bool>("spectate-mode")) return;
        GJBaseGameLayer::pushButton(p0, p1);
    }

    void releaseButton(int p0, bool p1) {
        if (Mod::get()->getSettingValue<bool>("spectate-mode")) return;
        GJBaseGameLayer::releaseButton(p0, p1);
    }
};
