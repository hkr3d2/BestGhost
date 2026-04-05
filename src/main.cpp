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

        // Status Label
        auto label = CCLabelBMFont::create("", "bigFont.fnt");
        label->setScale(0.35f);
        label->setOpacity(120);
        label->setPosition({ CCDirector::get()->getWinSize().width / 2, 25 });
        this->addChild(label, 100);
        m_fields->m_statusLabel = label;

        // Initialize Ghost as PlayerObject (Syncs with your icons)
        auto gm = GameManager::sharedState();
        auto ghost = PlayerObject::create(gm->getPlayerFrame(), gm->getPlayerShip(), this, this, true);
        if (ghost) {
            float op = static_cast<float>(Mod::get()->getSettingValue<double>("ghost-opacity"));
            ghost->setOpacity(static_cast<uint8_t>(op * 2.55f));
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
        
        if (m_fields->m_ghostVisual) {
            float op = static_cast<float>(Mod::get()->getSettingValue<double>("ghost-opacity")) * 2.55f;
            m_fields->m_ghostVisual->setOpacity(static_cast<uint8_t>(op));
        }
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

    void registerWithTouchDispatcher() override {
        CCDirector::get()->getTouchDispatcher()->addTargetedDelegate(this, -500, true);
    }

    bool init(int opacity) {
        if (!FLAlertLayer::init(opacity)) return false;

        auto winSize = CCDirector::get()->getWinSize();
        auto bg = CCScale9Sprite::create("GJ_square01.png");
        bg->setContentSize({ 250, 210 });
        bg->setPosition(winSize / 2);
        m_mainLayer->addChild(bg);

        auto menu = CCMenu::create();
        menu->setTouchPriority(-501); 
        m_mainLayer->addChild(menu);

        // Record Toggle
        createLabel("Record Ghost", {winSize.width / 2 - 50, winSize.height / 2 + 55});
        auto recToggle = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(GhostSettingsLayer::onToggleRecord), 0.7f);
        recToggle->setPosition({ 60, 55 });
        recToggle->toggle(g_isRecordingEnabled);
        menu->addChild(recToggle);

        // Status Toggle
        createLabel("Show Status", {winSize.width / 2 - 50, winSize.height / 2 + 20});
        auto statToggle = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(GhostSettingsLayer::onToggleStatus), 0.7f);
        statToggle->setPosition({ 60, 20 });
        statToggle->toggle(Mod::get()->getSettingValue<bool>("show-indicator"));
        menu->addChild(statToggle);

        // X Offset
        createLabel("X Offset", {winSize.width / 2 - 50, winSize.height / 2 - 15});
        auto offInp = createInput("ghost-offset", {winSize.width / 2 + 60, winSize.height / 2 - 15}, 1);
        m_mainLayer->addChild(offInp);

        // Opacity
        createLabel("Opacity (0-100)", {winSize.width / 2 - 50, winSize.height / 2 - 50});
        auto opInp = createInput("ghost-opacity", {winSize.width / 2 + 60, winSize.height / 2 - 50}, 2);
        m_mainLayer->addChild(opInp);

        // Close
        auto closeBtn = CCMenuItemSpriteExtra::create(CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png"), this, menu_selector(GhostSettingsLayer::onClose));
        closeBtn->setPosition({ -110, 90 });
        menu->addChild(closeBtn);

        return true;
    }

    void createLabel(const char* text, CCPoint pos) {
        auto lbl = CCLabelBMFont::create(text, "bigFont.fnt");
        lbl->setScale(0.4f);
        lbl->setPosition(pos);
        m_mainLayer->addChild(lbl);
    }

    CCTextInputNode* createInput(std::string setting, CCPoint pos, int tag) {
        auto input = CCTextInputNode::create(50.f, 20.f, "0", "bigFont.fnt");
        input->setAllowedChars("0123456789.-");
        input->setDelegate(this);
        input->setTag(tag);
        input->setString(std::to_string(Mod::get()->getSettingValue<double>(setting)).substr(0, 4).c_str());
        input->setPosition(pos);
        return input;
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
        try {
            double val = std::stod(input->getString());
            if (input->getTag() == 1) Mod::get()->setSettingValue("ghost-offset", val);
            if (input->getTag() == 2) Mod::get()->setSettingValue("ghost-opacity", std::clamp(val, 0.0, 100.0));
            refreshPlayLayer();
        } catch (...) {}
    }

    void refreshPlayLayer() {
        if (auto pl = PlayLayer::get()) static_cast<MyPlayLayer*>(static_cast<CCNode*>(pl))->updateUI();
    }

    void onClose(CCObject*) { this->removeFromParentAndCleanup(true); }
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
