#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <vector>

using namespace geode::prelude;

struct GhostFrame {
    float x;
    float y;
};

// Global Settings
bool g_isRecordingEnabled = false;
float g_ghostYOffset = 30.0f; // 30 units = 1 Block offset

// Global Data
std::vector<GhostFrame> g_bestAttemptData;
std::vector<GhostFrame> g_currentAttemptData;

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

        m_fields->m_playbackIndex = 0;
        g_currentAttemptData.clear();

        // Status Label (Bottom Center)
        auto label = CCLabelBMFont::create("", "bigFont.fnt");
        label->setScale(0.35f);
        label->setOpacity(120);
        label->setPosition({ CCDirector::get()->getWinSize().width / 2, 25 });
        this->addChild(label, 100);
        m_fields->m_statusLabel = label;

        // Ghost Sprite (Cyan Square)
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
            m_fields->m_statusLabel->setString("Ghost: OFF");
            m_fields->m_statusLabel->setColor({ 200, 200, 200 });
        } else {
            m_fields->m_statusLabel->setString(g_bestAttemptData.empty() ? "Ghost: RECORDING..." : "Ghost: ACTIVE");
            m_fields->m_statusLabel->setColor({ 0, 255, 255 });
        }
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        
        // If recording is on, move the last attempt to the 'Best' slot
        if (g_isRecordingEnabled && !g_currentAttemptData.empty()) {
            g_bestAttemptData = g_currentAttemptData;
        }

        g_currentAttemptData.clear();
        m_fields->m_playbackIndex = 0;
        if (m_fields->m_ghostVisual) m_fields->m_ghostVisual->setVisible(false);
        updateUI();
    }
};

/**
 * 2. PauseLayer Hooks - Adding the Toggle
 */
class $modify(MyPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        auto menu = this->getChildByID("right-button-menu");
        if (!menu) return;

        // Standard Geode Toggler
        auto toggler = CCMenuItemToggler::createWithStandardSprites(
            this, menu_selector(MyPauseLayer::onToggleGhost), 0.7f
        );
        toggler->setID("ghost-toggle"_spr);
        toggler->toggle(g_isRecordingEnabled);
        
        menu->addChild(toggler);
        menu->updateLayout();
    }

    void onToggleGhost(CCObject* sender) {
        g_isRecordingEnabled = !static_cast<CCMenuItemToggler*>(sender)->isOn(); // Geode toggler state logic
        
        if (!g_isRecordingEnabled) {
            g_bestAttemptData.clear();
            g_currentAttemptData.clear();
        }

        // Auto-Restart logic
        if (auto pl = PlayLayer::get()) {
            // Close pause menu and reset
            this->onResume(nullptr);
            pl->resetLevel();
        }
    }
};

/**
 * 3. Physics/Update Loop
 */
class $modify(MyBaseGameLayer, GJBaseGameLayer) {
    void update(float dt) {
        GJBaseGameLayer::update(dt);
        
        if (!g_isRecordingEnabled) return;

        auto playLayer = typeinfo_cast<PlayLayer*>(this);
        if (!playLayer || playLayer->m_isPaused) return;

        auto player = playLayer->m_player1;
        if (!player) return;

        // 1. Record Frame
        g_currentAttemptData.push_back({ player->getPositionX(), player->getPositionY() });

        // 2. Playback Ghost
        auto myPL = static_cast<MyPlayLayer*>(static_cast<CCNode*>(playLayer));
        if (myPL->m_fields->m_ghostVisual && !g_bestAttemptData.empty()) {
            size_t index = myPL->m_fields->m_playbackIndex;
            
            if (index < g_bestAttemptData.size()) {
                myPL->m_fields->m_ghostVisual->setVisible(true);
                
                // Position + Y Offset
                myPL->m_fields->m_ghostVisual->setPosition({ 
                    g_bestAttemptData[index].x, 
                    g_bestAttemptData[index].y + g_ghostYOffset 
                });
                
                myPL->m_fields->m_playbackIndex++;
            } else {
                myPL->m_fields->m_ghostVisual->setVisible(false);
            }
        }
    }
};
