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
    float x1, y1;
    float x2, y2;
    bool isDual;
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
        PlayerObject* m_ghostPlayer1 = nullptr;
        PlayerObject* m_ghostPlayer2 = nullptr;
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

        // Initialize Ghost Players (Using actual game icons)
        auto gm = GameManager::sharedState();
        
        m_fields->m_ghostPlayer1 = PlayerObject::create(gm->getPlayerFrame(), gm->getPlayerShip(), this, this, true);
        m_fields->m_ghostPlayer2 = PlayerObject::create(gm->getPlayerFrame(), gm->getPlayerShip(), this, this, true);

        float opacity = static_cast<float>(Mod::get()->getSettingValue<double>("ghost-opacity"));
        
        for (auto player : {m_fields->m_ghostPlayer1, m_fields->m_ghostPlayer2}) {
            if (player) {
                player->setOpacity(static_cast<uint8_t>(opacity * 2.55f)); // 0-100 to 0-255
                player->setVisible(false);
                if (this->m_objectLayer) this->m_objectLayer->addChild(player, 1000);
            }
        }

        updateUI();
        return true;
    }

    void updateUI() {
        if (!m_fields->m_statusLabel) return;
        m_fields->m_statusLabel->setVisible(Mod::get()->getSettingValue<bool>("show-indicator"));
        m_fields->m_statusLabel->setString(g_isRecordingEnabled ? "BestGhost: RECORDING" : "BestGhost: OFF");
        m_fields->m_statusLabel->setColor(g_isRecordingEnabled ? ccColor3B{0, 255, 255} : ccColor3B{200, 200, 200});
        
        // Update Opacity Live
        float op = static_cast<float>(Mod::get()->getSettingValue<double>("ghost-opacity")) * 2.55f;
        if (m_fields->m_ghostPlayer1) m_fields->m_ghostPlayer1->setOpacity(static_cast<uint8_t>(op));
        if (m_fields->m_ghostPlayer2) m_fields->m_ghostPlayer2->setOpacity(static_cast<uint8_t>(op));
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        if (g_isRecordingEnabled && !g_currentAttemptData.empty()) {
            float currentMaxX = g_currentAttemptData.back().x1;
            if (currentMaxX > g_bestXAttained) {
                g_bestXAttained = currentMaxX;
                g_bestAttemptData = g_currentAttemptData;
                saveGhostFile(m_level->m_levelID.value());
            }
        }
        g_currentAttemptData.clear();
        m_fields->m_timeCounter = 0.0;
        if (m_fields->m_ghostPlayer1) m_fields->m_ghostPlayer1->setVisible(false);
        if (m_fields->m_ghostPlayer2) m_fields->m_ghostPlayer2->setVisible(false);
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
        bg->setContentSize({ 260, 220 });
        bg->setPosition(winSize / 2);
        m_mainLayer->addChild(bg);

        auto menu = CCMenu::create();
        menu->setTouchPriority(-501); 
        m_mainLayer->addChild(menu);

        // 1. Record Toggler
        createLabel("Record Ghost", {winSize.width / 2 - 55, winSize.height / 2 + 60});
        auto recToggle = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(GhostSettingsLayer::onToggleRecord), 0.7f);
        recToggle->setPosition({ 65, 60 });
        recToggle->toggle(g_isRecordingEnabled);
        menu->addChild(recToggle);

        // 2. Status Toggler
        createLabel("Show Status", {winSize.width / 2 - 55, winSize.height / 2 + 25});
        auto statToggle = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(GhostSettingsLayer::onToggleStatus), 0.7f);
        statToggle->setPosition({ 65, 25 });
        statToggle->toggle(Mod::get()->getSettingValue<bool>("show-indicator"));
        menu->addChild(statToggle);

        // 3. X Offset Input
        createLabel("X Offset", {winSize.width / 2 - 55, winSize.height / 2 - 10});
        auto offInp = createInput("ghost-offset", {winSize.width / 2 + 65, winSize.height / 2 - 10}, 1);
        m_mainLayer->addChild(offInp);

        // 4. Opacity Input
        createLabel("Opacity (0-100)", {winSize.width / 2 - 55, winSize.height / 2 - 45});
        auto opInp = createInput("ghost-opacity", {winSize.width / 2 + 65, winSize.height / 2 - 45}, 2);
        m_mainLayer->addChild(opInp);

        // Close
        auto closeBtn = CCMenuItemSpriteExtra::create(CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png"), this, menu_selector(GhostSettingsLayer::onClose));
        closeBtn->setPosition({ -115, 95 });
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

        auto p1 = playLayer->m_player1;
        auto p2 = playLayer->m_player2;
        auto myPL = static_cast<MyPlayLayer*>(static_cast<CCNode*>(playLayer));

        if (g_isRecordingEnabled) {
            g_currentAttemptData.push_back({ 
                p1->getPositionX(), p1->getPositionY(),
                p2 ? p2->getPositionX() : 0.0f, p2 ? p2->getPositionY() : 0.0f,
                playLayer->m_isDualMode
            });
        }
        
        myPL->m_fields->m_timeCounter += 1.0;

        if (!g_bestAttemptData.empty()) {
            double offset = Mod::get()->getSettingValue<double>("ghost-offset");
            int targetIdx = static_cast<int>(myPL->m_fields->m_timeCounter + (offset * 4.0));

            if (targetIdx >= 0 && targetIdx < static_cast<int>(g_bestAttemptData.size())) {
                auto& frame = g_bestAttemptData[targetIdx];
                
                if (myPL->m_fields->m_ghostPlayer1) {
                    myPL->m_fields->m_ghostPlayer1->setVisible(true);
                    myPL->m_fields->m_ghostPlayer1->setPosition({ frame.x1, frame.y1 });
                }
                
                if (myPL->m_fields->m_ghostPlayer2) {
                    myPL->m_fields->m_ghostPlayer2->setVisible(frame.isDual);
                    myPL->m_fields->m_ghostPlayer2->setPosition({ frame.x2, frame.y2 });
                }
            } else {
                if (myPL->m_fields->m_ghostPlayer1) myPL->m_fields->m_ghostPlayer1->setVisible(false);
                if (myPL->m_fields->m_ghostPlayer2) myPL->m_fields->m_ghostPlayer2->setVisible(false);
            }
        }
    }
};
