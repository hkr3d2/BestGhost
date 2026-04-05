#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <vector>
#include <fstream>
#include <string>
#include <filesystem>

using namespace geode::prelude;
namespace fs = std::filesystem;

struct GhostFrame {
    float x;
    float y;
    float rotation;
};

// Global Session State
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
 * PlayLayer Hooks
 */
class $modify(MyPlayLayer, PlayLayer) {
    struct Fields {
        PlayerObject* m_ghostVisual = nullptr;
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

        auto gm = GameManager::sharedState();
        // Create a PlayerObject that uses your icons
        auto ghost = PlayerObject::create(gm->getPlayerFrame(), gm->getPlayerShip(), this, this, true);
        if (ghost) {
            ghost->setOpacity(76); // 30% Opacity
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
        m_fields->m_statusLabel->setString(g_isRecordingEnabled ? "BestGhost: RECORDING" : "BestGhost: OFF");
        m_fields->m_statusLabel->setColor(g_isRecordingEnabled ? ccColor3B{0, 255, 255} : ccColor3B{200, 200, 200});
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        if (g_isRecordingEnabled && !g_currentAttemptData.empty()) {
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
 * Custom Settings Menu
 */
class GhostSettingsLayer : public FLAlertLayer, public TextInputDelegate {
protected:
    CCTextInputNode* m_offsetInput = nullptr;

public:
    static GhostSettingsLayer* create() {
        auto ret = new GhostSettingsLayer();
        if (ret && ret->init(150)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    void registerWithTouchDispatcher() override {
        CCDirector::get()->getTouchDispatcher()->addTargetedDelegate(this, -500, true);
    }

    bool init(int opacity) {
        if (!FLAlertLayer::init(opacity)) return false;

        auto winSize = CCDirector::get()->getWinSize();
        auto bg = CCScale9Sprite::create("GJ_square01.png");
        bg->setContentSize({ 250, 160 });
        bg->setPosition(winSize / 2);
        m_mainLayer->addChild(bg);

        auto menu = CCMenu::create();
        menu->setTouchPriority(-501); 
        m_mainLayer->addChild(menu);

        createLabel("Record Ghost", {winSize.width / 2 - 50, winSize.height / 2 + 40});
        auto recToggle = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(GhostSettingsLayer::onToggleRecord), 0.7f);
        recToggle->setPosition({ 60, 40 });
        recToggle->toggle(g_isRecordingEnabled);
        menu->addChild(recToggle);

        createLabel("Show Status", {winSize.width / 2 - 50, winSize.height / 2 + 5});
        auto statToggle = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(GhostSettingsLayer::onToggleStatus), 0.7f);
        statToggle->setPosition({ 60, 5 });
        statToggle->toggle(Mod::get()->getSettingValue<bool>("show-indicator"));
        menu->addChild(statToggle);

        createLabel("X Offset", {winSize.width / 2 - 50, winSize.height / 2 - 30});
        m_offsetInput = CCTextInputNode::create(60.f, 20.f, "0", "bigFont.fnt");
        m_offsetInput->setAllowedChars("0123456789.-");
        m_offsetInput->setDelegate(this);
        
        double val = Mod::get()->getSettingValue<double>("ghost-offset");
        m_offsetInput->setString(std::to_string(val).substr(0, 5).c_str());
        m_offsetInput->setPosition({winSize.width / 2 + 60, winSize.height / 2 - 30});
        m_mainLayer->addChild(m_offsetInput);

        auto closeBtn = CCMenuItemSpriteExtra::create(CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png"), this, menu_selector(GhostSettingsLayer::onClose));
        closeBtn->setPosition({ -110, 65 });
        menu->addChild(closeBtn);

        return true;
    }

    void createLabel(const char* text, CCPoint pos) {
        auto lbl = CCLabelBMFont::create(text, "bigFont.fnt");
        lbl->setScale(0.4f);
        lbl->setPosition(pos);
        m_mainLayer->addChild(lbl);
    }

    void onToggleRecord(CCObject* sender) {
        g_isRecordingEnabled = !static_cast<CCMenuItemToggler*>(sender)->isOn();
        refreshPlayLayer();
    }

    void onToggleStatus(CCObject* sender) {
        Mod::get()->setSettingValue("show-indicator", !static_cast<CCMenuItemToggler*>(sender)->isOn());
        refreshPlayLayer();
    }

    void textChanged(CCTextInputNode* input) override {
        std::string str = input->getString();
        if (str.empty() || str == "-" || str == ".") return;
        try {
            double val = std::stod(str);
            Mod::get()->setSettingValue("ghost-offset", val);
            refreshPlayLayer();
        } catch (...) {}
    }

    void refreshPlayLayer() {
        if (auto pl = PlayLayer::get()) static_cast<MyPlayLayer*>(static_cast<CCNode*>(pl))->updateUI();
    }

    void onClose(CCObject*) {
        if (m_offsetInput) {
            m_offsetInput->setDelegate(nullptr);
            m_offsetInput->onClickTrackNode(false);
        }
        this->removeFromParentAndCleanup(true);
    }

    void keyBackClicked() override { onClose(nullptr); }
    bool ccTouchBegan(CCTouch* touch, CCEvent* event) override { return true; }
};

/**
 * PauseLayer Hook
 */
class $modify(MyPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        auto menu = this->getChildByID("right-button-menu");
        if (!menu) menu = typeinfo_cast<CCMenu*>(this->getChildByType<CCMenu>(0));
        if (menu) {
            auto settingsBtn = CCMenuItemSpriteExtra::create(CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png"), this, menu_selector(MyPauseLayer::onOpenCustomGhostMenu));
            settingsBtn->setScale(0.7f);
            menu->addChild(settingsBtn);
            settingsBtn->setPosition({249.0f, 116.0f}); 
            menu->updateLayout();
        }
    }
    void onOpenCustomGhostMenu(CCObject*) { GhostSettingsLayer::create()->show(); }
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
        auto myPL = static_cast<MyPlayLayer*>(static_cast<CCNode*>(playLayer));

        // 1. Record Position AND Rotation
        if (g_isRecordingEnabled) {
            g_currentAttemptData.push_back({ 
                player->getPositionX(), 
                player->getPositionY(), 
                player->getRotation() 
            });
        }
        
        myPL->m_fields->m_timeCounter += 1.0;

        // 2. Playback recorded data
        if (myPL->m_fields->m_ghostVisual && !g_bestAttemptData.empty()) {
            double offset = Mod::get()->getSettingValue<double>("ghost-offset");
            int targetIdx = static_cast<int>(myPL->m_fields->m_timeCounter + (offset * 4.0));

            if (targetIdx >= 0 && targetIdx < static_cast<int>(g_bestAttemptData.size())) {
                auto& frame = g_bestAttemptData[targetIdx];
                
                myPL->m_fields->m_ghostVisual->setVisible(true);
                myPL->m_fields->m_ghostVisual->setPosition({ frame.x, frame.y });
                myPL->m_fields->m_ghostVisual->setRotation(frame.rotation);
                
                // Keep the ghost's animation state updated (mimics your current gamemode visuals)
                myPL->m_fields->m_ghostVisual->update(dt);
                
            } else {
                myPL->m_fields->m_ghostVisual->setVisible(false);
            }
        }
    }
};
