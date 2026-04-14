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

/**
 * Ghost Gamemode
 */
enum GhostGameMode { Cube, Ship, Ball, Bird, Dart, Robot, Spider, Swing };

struct GhostFrame {
    float x; float y;
    float rotation;
    GhostGameMode mode;
    bool isMiniSize; 
};

bool isGhostRecording = false; 
std::vector<GhostFrame> bestTrySave;
std::vector<GhostFrame> bestTryCurrentAtt;
float furthestGhost = 0.0f;

/**
 * Ghost save files
 */
fs::path getGhostFolder() {
    auto path = Mod::get()->getSaveDir() / "ghosts";
    if (!fs::exists(path)) fs::create_directories(path);
    return path;
}

fs::path getGhostPath(int levelID) {
    return getGhostFolder() / (std::to_string(levelID) + ".ghst");
}

void saveGhostFile(int levelID) {
    if (bestTrySave.empty()) return;
    std::ofstream file(getGhostPath(levelID), std::ios::binary);
    if (!file.is_open()) return;
    file.write(reinterpret_cast<char*>(&furthestGhost), sizeof(float));
    for (const auto& frame : bestTrySave) {
        file.write(reinterpret_cast<const char*>(&frame), sizeof(GhostFrame));
    }
}

void loadGhostFile(int levelID) {
    bestTrySave.clear();
    auto path = getGhostPath(levelID);
    if (!fs::exists(path)) {
        furthestGhost = 0.0f;
        return;
    }
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return;
    file.read(reinterpret_cast<char*>(&furthestGhost), sizeof(float));
    GhostFrame frame;
    while (file.read(reinterpret_cast<char*>(&frame), sizeof(GhostFrame))) {
        bestTrySave.push_back(frame);
    }
}

$execute {  // Library toggle
    listenForSettingChanges<bool>("open-library", [](bool value) {
        if (value) {
            utils::file::openFolder(getGhostFolder());
            Mod::get()->setSettingValue<bool>("open-library", false);
        }
    });
}

/**
 * Ghost status label
 */
class $modify(MyPlayLayer, PlayLayer) {
    struct Fields {
        PlayerObject* m_ghostVisual = nullptr;
        CCLabelBMFont* ghostStatus = nullptr;
        double ghostTimer = 0.0;
        GhostGameMode lastGhostGMode = Cube;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        loadGhostFile(level->m_levelID.value());
        
        isGhostRecording = false; 
        m_fields->ghostTimer = 0.0;
        bestTryCurrentAtt.clear();

        auto label = CCLabelBMFont::create("", "bigFont.fnt");
        label->setScale(0.35f);
        label->setOpacity(120);
        label->setPosition({ CCDirector::get()->getWinSize().width / 2, 25 });
        this->addChild(label, 100);
        m_fields->ghostStatus = label;

        auto gm = GameManager::sharedState();
        auto ghost = PlayerObject::create(gm->getPlayerFrame(), gm->getPlayerShip(), this, this, true);
        if (ghost) {
            ghost->setOpacity(76); 
            ghost->setVisible(false);
            m_fields->m_ghostVisual = ghost;
            if (this->m_objectLayer) this->m_objectLayer->addChild(ghost, 1000);
        }

        updateUI();
        return true;
    }

    void updateUI() {
        if (!m_fields->ghostStatus) return;
        m_fields->ghostStatus->setVisible(Mod::get()->getSettingValue<bool>("show-indicator"));
        m_fields->ghostStatus->setString(isGhostRecording ? "BestGhost: RECORDING" : "BestGhost: OFF");
        m_fields->ghostStatus->setColor(isGhostRecording ? ccColor3B{0, 255, 255} : ccColor3B{200, 200, 200});
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        if (isGhostRecording && !bestTryCurrentAtt.empty()) {
            float currentMaxX = bestTryCurrentAtt.back().x;
            if (currentMaxX > furthestGhost) {
                furthestGhost = currentMaxX;
                bestTrySave = bestTryCurrentAtt;
                saveGhostFile(m_level->m_levelID.value());
            }
        }
        bestTryCurrentAtt.clear();
        m_fields->ghostTimer = 0.0;
        if (m_fields->m_ghostVisual) {
            m_fields->m_ghostVisual->setVisible(false);
            m_fields->lastGhostGMode = Cube;
        }
        updateUI();
    }
};

/**
 * ghost menu ui
 */
class GhostMenuLayer : public FLAlertLayer, public TextInputDelegate {
protected:
    CCTextInputNode* offsetTextInput = nullptr;
public:
    static GhostMenuLayer* create() {
        auto ret = new GhostMenuLayer();
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
        bg->setContentSize({ 250, 220 });
        bg->setPosition(winSize / 2);
        m_mainLayer->addChild(bg);

        auto title = CCLabelBMFont::create("Ghost Menu", "bigFont.fnt");
        title->setScale(0.7f);
        title->setPosition({ winSize.width / 2, winSize.height / 2 + 88 });
        m_mainLayer->addChild(title);

        auto menu = CCMenu::create();
        menu->setTouchPriority(-501); 
        m_mainLayer->addChild(menu);

        createLabel("Record Ghost", {winSize.width / 2 - 50, winSize.height / 2 + 50});
        auto recToggle = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(GhostMenuLayer::onToggleRecord), 0.7f);
        recToggle->setPosition({ 60, 50 });
        recToggle->toggle(isGhostRecording);
        menu->addChild(recToggle);

        createLabel("Show Status", {winSize.width / 2 - 50, winSize.height / 2 + 15});
        auto statToggle = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(GhostMenuLayer::onToggleStatus), 0.7f);
        statToggle->setPosition({ 60, 15 });
        statToggle->toggle(Mod::get()->getSettingValue<bool>("show-indicator"));
        menu->addChild(statToggle);

        createLabel("X Offset", {winSize.width / 2 - 50, winSize.height / 2 - 20});
        offsetTextInput = CCTextInputNode::create(60.f, 20.f, "0", "bigFont.fnt");
        offsetTextInput->setAllowedChars("0123456789.-");
        offsetTextInput->setDelegate(this);
        offsetTextInput->setString(std::to_string(Mod::get()->getSettingValue<double>("ghost-offset")).substr(0, 5).c_str());
        offsetTextInput->setPosition({winSize.width / 2 + 60, winSize.height / 2 - 20});
        m_mainLayer->addChild(offsetTextInput);

        createLabel("Clear Ghost", {winSize.width / 2 - 30, winSize.height / 2 - 55});
        auto trashBtn = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("GJ_deleteBtn_001.png"), 
            this, 
            menu_selector(GhostMenuLayer::onConfirmDelete)
        );
        trashBtn->setPosition({ 75, -55 });
        trashBtn->setScale(0.8f);
        menu->addChild(trashBtn);

        auto closeBtn = CCMenuItemSpriteExtra::create(CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png"), this, menu_selector(GhostMenuLayer::onClose));
        closeBtn->setPosition({ -110, 95 });
        menu->addChild(closeBtn);

        return true;
    }

    void createLabel(const char* text, CCPoint pos) {
        auto lbl = CCLabelBMFont::create(text, "bigFont.fnt");
        lbl->setScale(0.4f);
        lbl->setPosition(pos);
        m_mainLayer->addChild(lbl);
    }

    void onConfirmDelete(CCObject*) {
        geode::createQuickPopup("Delete Ghost", "Clear best attempt for this level?", "Cancel", "Delete", [this](auto, bool btn2) {
            if (btn2) {
                if (auto pl = PlayLayer::get()) {
                    auto path = getGhostPath(pl->m_level->m_levelID.value());
                    if (fs::exists(path)) fs::remove(path);
                    bestTrySave.clear();
                    furthestGhost = 0.0f;
                    FLAlertLayer::create("Success", "Ghost data deleted.", "OK")->show();
                }
            }
        });
    }

    void onToggleRecord(CCObject* sender) { isGhostRecording = !static_cast<CCMenuItemToggler*>(sender)->isOn(); refreshPlayLayer(); }
    void onToggleStatus(CCObject* sender) { Mod::get()->setSettingValue("show-indicator", !static_cast<CCMenuItemToggler*>(sender)->isOn()); refreshPlayLayer(); }
    void textChanged(CCTextInputNode* input) override { try { Mod::get()->setSettingValue("ghost-offset", std::stod(input->getString())); refreshPlayLayer(); } catch (...) {} }
    void refreshPlayLayer() { if (auto pl = PlayLayer::get()) static_cast<MyPlayLayer*>(static_cast<CCNode*>(pl))->updateUI(); }
    void onClose(CCObject*) { if (offsetTextInput) offsetTextInput->setDelegate(nullptr); this->removeFromParentAndCleanup(true); }
    void keyBackClicked() override { onClose(nullptr); }
    bool ccTouchBegan(CCTouch* touch, CCEvent* event) override { return true; }
};

/**
 * menu button on the pause layer
 */
class $modify(MyPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        auto menu = this->getChildByID("right-button-menu");
        if (!menu) menu = typeinfo_cast<CCMenu*>(this->getChildByType<CCMenu>(0));
        
        if (menu) {
            auto settingsSprite = CCSprite::create("hkr3d2.bestghost/ghost_btn.png");
            if (!settingsSprite) settingsSprite = CCSprite::create("ghost_btn.png");

            if (settingsSprite) {
                settingsSprite->setScale(0.08f); 
            } else {
                settingsSprite = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
            }

            auto settingsBtn = CCMenuItemSpriteExtra::create(settingsSprite, this, menu_selector(MyPauseLayer::onOpenCustomGhostMenu));
            menu->addChild(settingsBtn);
            settingsBtn->setPosition({249.0f, 116.0f}); 
            settingsBtn->setScale(1.0f); 

            menu->updateLayout();
        }
    }
    void onOpenCustomGhostMenu(CCObject*) { GhostMenuLayer::create()->show(); }
};

/**
 * Ghost replay and record logic
 */
class $modify(MyBaseGameLayer, GJBaseGameLayer) {
    void update(float dt) {
        GJBaseGameLayer::update(dt);
        auto playLayer = typeinfo_cast<PlayLayer*>(this);
        if (!playLayer || playLayer->m_isPaused) return;

        auto p = playLayer->m_player1;
        auto myPL = static_cast<MyPlayLayer*>(static_cast<CCNode*>(playLayer));

        if (isGhostRecording) {
            GhostGameMode currentMode = Cube;
            if (p->m_isShip) currentMode = Ship;
            else if (p->m_isBall) currentMode = Ball;
            else if (p->m_isBird) currentMode = Bird;
            else if (p->m_isDart) currentMode = Dart;
            else if (p->m_isRobot) currentMode = Robot;
            else if (p->m_isSpider) currentMode = Spider;
            else if (p->m_isSwing) currentMode = Swing;
            bestTryCurrentAtt.push_back({ p->getPositionX(), p->getPositionY(), p->getRotation(), currentMode, (p->m_vehicleSize < 1.0f) });
        }
        
        myPL->m_fields->ghostTimer += 1.0;

        if (myPL->m_fields->m_ghostVisual && !bestTrySave.empty()) {
            double offset = Mod::get()->getSettingValue<double>("ghost-offset");
            int targetIdx = static_cast<int>(myPL->m_fields->ghostTimer + (offset * 4.0));

            if (targetIdx >= 0 && targetIdx < static_cast<int>(bestTrySave.size())) {
                auto& frame = bestTrySave[targetIdx];
                auto g = myPL->m_fields->m_ghostVisual;

                g->setVisible(true);
                g->setPosition({ frame.x, frame.y });
                g->setRotation(frame.rotation);
                g->setScale(frame.isMiniSize ? 0.6f : 1.0f);

                if (frame.mode != myPL->m_fields->lastGhostGMode) {
                    g->toggleFlyMode(false, false); g->toggleRollMode(false, false);
                    g->toggleBirdMode(false, false); g->toggleDartMode(false, false);
                    g->toggleRobotMode(false, false); g->toggleSpiderMode(false, false);
                    g->toggleSwingMode(false, false);

                    if (frame.mode == Ship) g->toggleFlyMode(true, true);
                    else if (frame.mode == Ball) g->toggleRollMode(true, true);
                    else if (frame.mode == Bird) g->toggleBirdMode(true, true);
                    else if (frame.mode == Dart) g->toggleDartMode(true, true);
                    else if (frame.mode == Robot) g->toggleRobotMode(true, true);
                    else if (frame.mode == Spider) g->toggleSpiderMode(true, true);
                    else if (frame.mode == Swing) g->toggleSwingMode(true, true);

                    myPL->m_fields->lastGhostGMode = frame.mode;
                }
                g->update(dt);
            } else {
                myPL->m_fields->m_ghostVisual->setVisible(false);
            }
        }
    }
};
