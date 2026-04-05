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
float g_ghostXOffset = -30.0f; // 1 Block Behind

// Global Data (Persists until the game is closed or a new recording starts)
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

        // Turn OFF recording/playback by default when entering a level
        // But we DO NOT clear g_bestAttemptData here so the ghost is preserved
        g_isRecordingEnabled = false;
        
        m_fields->m_playbackIndex = 0;
        g_currentAttemptData.clear();

        // Status Label
        auto label = CCLabelBMFont::create("", "bigFont.fnt");
        label->setScale(0.35f);
        label->setOpacity(120);
        label->setPosition({ CCDirector::get()->getWinSize().width / 2, 25 });
        this->addChild(label, 100);
        m_fields->m_statusLabel = label;

        // Ghost Sprite
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
        m_fields->m_statusLabel->setString(g_isRecordingEnabled ? (g_bestAttemptData.empty() ? "Ghost: RECORDING..." : "Ghost: ACTIVE") : "Ghost: OFF");
        m_fields->m_statusLabel->setColor(g_isRecordingEnabled ? ccColor3B{0, 255, 255} : ccColor3B{200, 200, 200});
    }

    void resetLevel() {
        PlayLayer::resetLevel();
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
 * 2. PauseLayer Hooks - Precision placement near settings
 */
class $modify(MyPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        auto winSize = CCDirector::get()->getWinSize();
        
        // Find the settings menu
        auto menu = this->getChildByID("settings-button-menu");
        
        if (menu) {
            auto toggler = CCMenuItemToggler::createWithStandardSprites(
                this, menu_selector(MyPauseLayer::onToggleGhost), 0.65f
            );
            toggler->setID("ghost-toggle"_spr);
            toggler->toggle(g_isRecordingEnabled);
            
            // Add to the menu
            menu->addChild(toggler);
            
            // Manual Offset: Push it to the left of the existing buttons
            toggler->setPosition({-40.0f, 0.0f}); 
        } else {
            // Fallback: If no settings menu, put it in the top right
            auto fallbackMenu = CCMenu::create();
            fallbackMenu->setPosition({winSize.width - 30.0f, winSize.height - 30.0f});
            this->addChild(fallbackMenu);
            
            auto toggler = CCMenuItemToggler::createWithStandardSprites(
                this, menu_selector(MyPauseLayer::onToggleGhost), 0.65f
            );
            toggler->toggle(g_isRecordingEnabled);
            fallbackMenu->addChild(toggler);
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
                // 1 block behind on X axis
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
