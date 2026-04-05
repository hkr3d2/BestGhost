#include <Geode/Geode.hpp>
#include <Geode/loader/SettingV3.hpp>
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

/**
 * Ghost Library: File Management
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
    file.close();
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
    file.close();
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

        loadGhostFile(level->m_levelID.value());
        g_isRecordingEnabled = false;
        m_fields->m_playbackIndex = 0;
        g_currentAttemptData.clear();

        auto label = CCLabelBMFont::create("", "bigFont.fnt");
        label->setScale(0.35f);
        label->setOpacity(120);
        label->setPosition({ CCDirector::get()->getWinSize().width / 2, 25 });
        label->setVisible(Mod::get()->getSettingValue<bool>("show-indicator"));
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
        
        bool replayOnly = Mod::get()->getSettingValue<bool>("replay-only");
        bool spectate = Mod::get()->getSettingValue<bool>("spectate-mode");

        if (!g_isRecordingEnabled) {
            m_fields->m_statusLabel->setString("BestGhost: OFF");
            m_fields->m_statusLabel->setColor({ 200, 200, 200 });
        } else {
            if (spectate) {
                m_fields->m_statusLabel->setString("BestGhost: SPECTATING");
                m_fields->m_statusLabel->setColor({ 150, 0, 255 });
            } else if (replayOnly) {
                m_fields->m_statusLabel->setString("BestGhost: REPLAY ONLY");
                m_fields->m_statusLabel->setColor({ 255, 200, 0 });
            } else {
                m_fields->m_statusLabel->setString(g_bestAttemptData.empty() ? "BestGhost: RECORDING..." : "BestGhost: ACTIVE");
                m_fields->m_statusLabel->setColor({ 0, 255, 255 });
            }
        }
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        bool replayOnly = Mod::get()->getSettingValue<bool>("replay-only");
        bool spectate = Mod::get()->getSettingValue<bool>("spectate-mode");

        if (g_isRecordingEnabled && !g_currentAttemptData.empty() && !replayOnly && !spectate) {
            float currentMaxX = g_currentAttemptData.back().x;
            if (currentMaxX > g_bestXAttained) {
                g_bestXAttained = currentMaxX;
                g_bestAttemptData = g_currentAttemptData;
                saveGhostFile(m_level->m_levelID.value());
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
            toggler->setPosition({249.0f, 116.0f}); 
        }
    }

    void onToggleGhost(CCObject* sender) {
        auto toggler = static_cast<CCMenuItemToggler*>(sender);
        g_isRecordingEnabled = !toggler->isOn(); 
        this->onResume(nullptr);
        if (auto pl = PlayLayer::get()) pl->resetLevel();
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

        bool spectate = Mod::get()->getSettingValue<bool>("spectate-mode");
        bool replayOnly = Mod::get()->getSettingValue<bool>("replay-only");

        if (!replayOnly && !spectate) {
            g_currentAttemptData.push_back({ player->getPositionX(), player->getPositionY() });
        }

        auto myPL = static_cast<MyPlayLayer*>(static_cast<CCNode*>(playLayer));
        if (myPL->m_fields->m_ghostVisual && !g_bestAttemptData.empty()) {
            size_t index = myPL->m_fields->m_playbackIndex;
            if (index < g_bestAttemptData.size()) {
                myPL->m_fields->m_ghostVisual->setVisible(true);
                
                float offset = static_cast<float>(Mod::get()->getSettingValue<double>("ghost-offset"));
                float targetX = g_bestAttemptData[index].x;
                float targetY = g_bestAttemptData[index].y;

                if (spectate) {
                    player->setPosition({ targetX, targetY });
                    player->m_isDead = false;
                    player->setVisible(false);
                } else {
                    player->setVisible(true);
                }

                myPL->m_fields->m_ghostVisual->setPosition({ targetX + offset, targetY });
                myPL->m_fields->m_playbackIndex++;
            } else {
                myPL->m_fields->m_ghostVisual->setVisible(false);
                if (spectate) playLayer->resetLevel();
            }
        }
    }
};

/**
 * 4. CUSTOM SETTING BUTTON LOGIC
 */
class OpenLibrarySettingNode : public SettingNodeV3 {
protected:
    bool init(SettingValueV3* value, float width) {
        if (!SettingNodeV3::init(value, width)) return false;

        this->setContentSize({ width, 40.f });

        auto menu = CCMenu::create();
        menu->setPosition({ width / 2, 20.f });
        this->addChild(menu);

        auto sprite = ButtonSprite::create("Open Folder", "goldFont.fnt", "GJ_button_01.png", .7f);
        auto btn = CCMenuItemSpriteExtra::create(
            sprite, this, menu_selector(OpenLibrarySettingNode::onOpen)
        );
        menu->addChild(btn);

        return true;
    }

    void onOpen(CCObject*) {
        auto path = Mod::get()->getSaveDir() / "ghosts";
        if (!fs::exists(path)) fs::create_directories(path);
        utils::file::openFolder(path);
    }

public:
    static OpenLibrarySettingNode* create(SettingValueV3* value, float width) {
        auto ret = new OpenLibrarySettingNode();
        if (ret && ret->init(value, width)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    void loadValues() override {}
    void storeValues() override {}
    bool hasUnsavedChanges() override { return false; }
    bool hasNonDefaultValue() override { return false; }
    void resetToDefault() override {}
};

// Register the custom setting node
$execute {
    Mod::get()->addCustomSettingNode("open-library", &OpenLibrarySettingNode::create);
}
