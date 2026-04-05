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

// Best Run Persistence
std::vector<GhostFrame> g_bestAttemptData;
std::vector<GhostFrame> g_currentAttemptData;
float g_bestXAttained = 0.0f;

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

        // Reset session-specific flags but KEEP g_bestAttemptData
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
        
        if (!g_isRecordingEnabled) {
            m_fields->m_statusLabel->setString("BestGhost: OFF");
            m_fields->m_statusLabel->setColor({ 200, 200, 200 });
        } else {
            if (g_bestAttemptData.empty()) {
                m_fields->m_statusLabel->setString("BestGhost: Recording New Best...");
                m_fields->m_statusLabel->setColor({ 255, 100, 100 }); // Red for first record
            } else {
                m_fields->m_statusLabel->setString("BestGhost: Chasing Best");
                m_fields->m_statusLabel->setColor({ 0, 255, 255 }); // Cyan for active chase
            }
        }
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        
        if (g_isRecordingEnabled && !g_currentAttemptData.empty()) {
            float currentMaxX = g_currentAttemptData.back().x;

            // --- THE "BEST" LOGIC ---
            // Only update the ghost if we went further than the previous record
            if (currentMaxX > g_bestXAttained) {
                g_bestXAttained = currentMaxX;
                g_bestAttemptData = g_currentAttemptData;
                log::info("BestGhost: New Record! {} units", g_bestXAttained);
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

        // Try to find the settings gear menu
        auto menu = this->getChildByID("settings-button-menu");
        
        if (menu) {
            auto toggler = CCMenuItemToggler::createWithStandardSprites(
                this, menu_selector(MyPauseLayer::onToggleGhost), 0.65f
            );
            toggler->setID("ghost-toggle"_spr);
            toggler->toggle(g_isRecordingEnabled);
            
            menu->addChild(toggler);
            
            // Placed to the left of the gear icon
            toggler->setPosition({-40.0f, 0.0f}); 
            menu->updateLayout();
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

        // Always record the current attempt
        g_currentAttemptData.push_back({ player->getPositionX(), player->getPositionY() });

        // Only playback if we have a "Best" saved
        auto myPL = static_cast<MyPlayLayer*>(static_cast<CCNode*>(playLayer));
        if (myPL->m_fields->m_ghostVisual && !g_bestAttemptData.empty()) {
            size_t index = myPL->m_fields->m_playbackIndex;
            
            if (index < g_bestAttemptData.size()) {
                myPL->m_fields->m_ghostVisual->setVisible(true);
                // 1 block trail behind
                myPL->m_fields->m_ghostVisual->setPosition({ 
                    g_bestAttemptData[index].x + g_ghostXOffset, 
                    g_bestAttemptData[index].y 
                });
                myPL->m_fields->m_playbackIndex++;
            } else {
                // If the player survives longer than the ghost, hide the ghost
                myPL->m_fields->m_ghostVisual->setVisible(false);
                if (myPL->m_fields->m_statusLabel) {
                    myPL->m_fields->m_statusLabel->setString("BestGhost: NEW RECORD!");
                    myPL->m_fields->m_statusLabel->setColor({ 100, 255, 100 }); // Green for improvement
                }
            }
        }
    }
};
