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

    // Force touch priority so buttons click
    void registerWithTouchDispatcher() override {
        CCDirector::get()->getTouchDispatcher()->addTargetedDelegate(this, -500, true);
    }

    bool init(int opacity) {
        if (!FLAlertLayer::init(opacity)) return false;

        auto winSize = CCDirector::get()->getWinSize();
        
        auto bg = CCScale9Sprite::create("GJ_square01.png");
        bg->setContentSize({ 240, 180 });
        bg->setPosition(winSize / 2);
        m_mainLayer->addChild(bg);

        // Create manual menu with high priority
        auto menu = CCMenu::create();
        menu->setTouchPriority(-501); 
        m_mainLayer->addChild(menu);

        // 1. Ghost Record Toggler
        auto recordLabel = CCLabelBMFont::create("Record Ghost", "bigFont.fnt");
        recordLabel->setScale(0.45f);
        recordLabel->setPosition(winSize.width / 2 - 50, winSize.height / 2 + 40);
        m_mainLayer->addChild(recordLabel);

        auto recordToggle = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(GhostSettingsLayer::onToggleRecord), 0.7f);
        recordToggle->setPosition({ 60, 40 });
        recordToggle->toggle(g_isRecordingEnabled);
        menu->addChild(recordToggle);

        // 2. Status Label Toggler
        auto statusLabelTxt = CCLabelBMFont::create("Show Status", "bigFont.fnt");
        statusLabelTxt->setScale(0.45f);
        statusLabelTxt->setPosition(winSize.width / 2 - 50, winSize.height / 2 + 0);
        m_mainLayer->addChild(statusLabelTxt);

        auto statusToggle = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(GhostSettingsLayer::onToggleStatus), 0.7f);
        statusToggle->setPosition({ 60, 0 });
        statusToggle->toggle(Mod::get()->getSettingValue<bool>("show-indicator"));
        menu->addChild(statusToggle);

        // 3. Offset Textbox
        auto offsetLabel = CCLabelBMFont::create("X Offset", "bigFont.fnt");
        offsetLabel->setScale(0.45f);
        offsetLabel->setPosition(winSize.width / 2 - 50, winSize.height / 2 - 40);
        m_mainLayer->addChild(offsetLabel);

        auto input = CCTextInputNode::create(60.f, 20.f, "0.0", "bigFont.fnt");
        input->setLabelPlaceholderColor({ 150, 150, 150 });
        input->setAllowedChars("0123456789.-");
        input->setDelegate(this);
        input->setString(std::to_string(Mod::get()->getSettingValue<double>("ghost-offset")).substr(0, 4).c_str());
        input->setPosition(winSize.width / 2 + 60, winSize.height / 2 - 40);
        m_mainLayer->addChild(input);

        // Close Button
        auto closeBtn = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png"),
            this,
            menu_selector(GhostSettingsLayer::onClose)
        );
        closeBtn->setPosition({ -105, 75 });
        menu->addChild(closeBtn);

        this->setTouchEnabled(true);
        this->setKeypadEnabled(true);

        return true;
    }

    void onToggleRecord(CCObject* sender) {
        g_isRecordingEnabled = !static_cast<CCMenuItemToggler*>(sender)->isOn();
        if (auto pl = PlayLayer::get()) static_cast<MyPlayLayer*>(static_cast<CCNode*>(pl))->updateUI();
    }

    void onToggleStatus(CCObject* sender) {
        bool val = !static_cast<CCMenuItemToggler*>(sender)->isOn();
        Mod::get()->setSettingValue("show-indicator", val);
        if (auto pl = PlayLayer::get()) static_cast<MyPlayLayer*>(static_cast<CCNode*>(pl))->updateUI();
    }

    void textChanged(CCTextInputNode* input) override {
        try {
            double val = std::stod(input->getString());
            Mod::get()->setSettingValue("ghost-offset", val);
        } catch (...) {}
    }

    void onClose(CCObject*) {
        this->setTouchEnabled(false);
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
            auto settingsSprite = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
            settingsSprite->setScale(0.7f);

            auto settingsBtn = CCMenuItemSpriteExtra::create(
                settingsSprite,
                this,
                menu_selector(MyPauseLayer::onOpenCustomGhostMenu)
            );
            
            settingsBtn->setID("ghost-settings-btn"_spr);
            menu->addChild(settingsBtn);
            // Coordinates as requested
            settingsBtn->setPosition({249.0f, 116.0f}); 
            menu->updateLayout();
        }
    }
    void onOpenCustomGhostMenu(CCObject*) {
        auto layer = GhostSettingsLayer::create();
        CCDirector::get()->getRunningScene()->addChild(layer, 100);
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
        auto myPL = static_cast<MyPlayLayer*>(static_cast<CCNode*>(playLayer));

        if (g_isRecordingEnabled) {
            g_currentAttemptData.push_back({ player->getPositionX(), player->getPositionY() });
        }
        
        myPL->m_fields->m_timeCounter += 1.0;

        if (myPL->m_fields->m_ghostVisual && !g_bestAttemptData.empty()) {
            double offset = Mod::get()->getSettingValue<double>("ghost-offset");
            int targetIdx = static_cast<int>(myPL->m_fields->m_timeCounter + (offset * 4.0));

            if (targetIdx >= 0 && targetIdx < static_cast<int>(g_bestAttemptData.size())) {
                myPL->m_fields->m_ghostVisual->setVisible(true);
                myPL->m_fields->m_ghostVisual->setPosition({ g_bestAttemptData[targetIdx].x, g_bestAttemptData[targetIdx].y });
            } else {
                myPL->m_fields->m_ghostVisual->setVisible(false);
            }
        }
    }
};
