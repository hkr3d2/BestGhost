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

// ==========================================
// DATA STRUCTURES
// ==========================================

enum PlayerState { Cube, Ship, Ball, Bird, Dart, Robot, Spider, Swing };

struct RecordingFrame {
    float xPos;
    float yPos;
    float rot;
    PlayerState gameMode;
    bool smallSize; 
};

// Global variables for the current session
bool isRecordingNow = false; 
float bestDistanceEver = 0.0f;
std::vector<RecordingFrame> savedBestRun;
std::vector<RecordingFrame> liveRecording;

// -- YOUR COMMENTS HERE --


// ==========================================
// FILE MANAGEMENT
// ==========================================

fs::path getStorageDir() {
    auto folder = Mod::get()->getSaveDir() / "ghost_files";
    if (!fs::exists(folder)) fs::create_directories(folder);
    return folder;
}

fs::path getFilePath(int id) {
    return getStorageDir() / (std::to_string(id) + ".ghost");
}

void saveToDisk(int id) {
    if (savedBestRun.empty()) return;
    std::ofstream file(getFilePath(id), std::ios::binary);
    if (!file.is_open()) return;

    file.write(reinterpret_cast<char*>(&bestDistanceEver), sizeof(float));
    for (const auto& frame : savedBestRun) {
        file.write(reinterpret_cast<const char*>(&frame), sizeof(RecordingFrame));
    }
}

void loadFromDisk(int id) {
    savedBestRun.clear();
    auto path = getFilePath(id);
    if (!fs::exists(path)) {
        bestDistanceEver = 0.0f;
        return;
    }

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return;

    file.read(reinterpret_cast<char*>(&bestDistanceEver), sizeof(float));
    RecordingFrame frame;
    while (file.read(reinterpret_cast<char*>(&frame), sizeof(RecordingFrame))) {
        savedBestRun.push_back(frame);
    }
}

// -- YOUR COMMENTS HERE --


// ==========================================
// PLAYLAYER (LEVEL LOGIC)
// ==========================================

class $modify(GhostPlayLayer, PlayLayer) {
    struct Fields {
        PlayerObject* ghostPlayer = nullptr;
        CCLabelBMFont* recordingLabel = nullptr;
        double frameClock = 0.0;
        PlayerState lastMode = Cube;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        
        loadFromDisk(level->m_levelID.value());
        
        isRecordingNow = false; 
        m_fields->frameClock = 0.0;
        liveRecording.clear();

        // Setup the text at the bottom
        auto label = CCLabelBMFont::create("", "bigFont.fnt");
        label->setScale(0.35f);
        label->setOpacity(130);
        label->setPosition({ CCDirector::get()->getWinSize().width / 2, 25 });
        this->addChild(label, 100);
        m_fields->recordingLabel = label;

        // Setup the actual ghost player
        auto gm = GameManager::sharedState();
        auto ghost = PlayerObject::create(gm->getPlayerFrame(), gm->getPlayerShip(), this, this, true);
        if (ghost) {
            ghost->setOpacity(80); 
            ghost->setVisible(false);
            m_fields->ghostPlayer = ghost;
            if (this->m_objectLayer) this->m_objectLayer->addChild(ghost, 1000);
        }

        refreshUI();
        return true;
    }

    void refreshUI() {
        if (!m_fields->recordingLabel) return;
        bool showLabel = Mod::get()->getSettingValue<bool>("show-indicator");
        m_fields->recordingLabel->setVisible(showLabel);
        
        if (isRecordingNow) {
            m_fields->recordingLabel->setString("GHOST: RECORDING");
            m_fields->recordingLabel->setColor({ 0, 255, 255 });
        } else {
            m_fields->recordingLabel->setString("GHOST: READY");
            m_fields->recordingLabel->setColor({ 200, 200, 200 });
        }
    }

    void resetLevel() {
        PlayLayer::resetLevel();

        if (isRecordingNow && !liveRecording.empty()) {
            float lastPos = liveRecording.back().xPos;
            if (lastPos > bestDistanceEver) {
                bestDistanceEver = lastPos;
                savedBestRun = liveRecording;
                saveToDisk(m_level->m_levelID.value());
            }
        }

        liveRecording.clear();
        m_fields->frameClock = 0.0;
        if (m_fields->ghostPlayer) {
            m_fields->ghostPlayer->setVisible(false);
            m_fields->lastMode = Cube;
        }
        refreshUI();
    }
};

// -- YOUR COMMENTS HERE --


// ==========================================
// GHOST MENU (UI)
// ==========================================

class GhostMenu : public FLAlertLayer, public TextInputDelegate {
protected:
    CCTextInputNode* offsetBox = nullptr;
public:
    static GhostMenu* create() {
        auto ret = new GhostMenu();
        if (ret && ret->init(160)) {
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
        
        auto box = CCScale9Sprite::create("GJ_square01.png");
        box->setContentSize({ 250, 210 });
        box->setPosition(winSize / 2);
        m_mainLayer->addChild(box);

        auto title = CCLabelBMFont::create("Ghost Settings", "bigFont.fnt");
        title->setScale(0.65f);
        title->setPosition({ winSize.width / 2, winSize.height / 2 + 85 });
        m_mainLayer->addChild(title);

        auto buttons = CCMenu::create();
        buttons->setTouchPriority(-501); 
        m_mainLayer->addChild(buttons);

        // Record Toggle
        addLabel("Record Mode", {winSize.width / 2 - 50, winSize.height / 2 + 50});
        auto recBtn = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(GhostMenu::onToggleRec), 0.7f);
        recBtn->setPosition({ 60, 50 });
        recBtn->toggle(isRecordingNow);
        buttons->addChild(recBtn);

        // Offset Input
        addLabel("Offset", {winSize.width / 2 - 50, winSize.height / 2 + 5});
        offsetBox = CCTextInputNode::create(60.f, 20.f, "0", "bigFont.fnt");
        offsetBox->setAllowedChars("0123456789.-");
        offsetBox->setDelegate(this);
        offsetBox->setString(std::to_string(Mod::get()->getSettingValue<double>("ghost-offset")).substr(0, 4).c_str());
        offsetBox->setPosition({winSize.width / 2 + 60, winSize.height / 2 + 5});
        m_mainLayer->addChild(offsetBox);

        // THE RED X DELETE BUTTON
        addLabel("Delete Run", {winSize.width / 2 - 30, winSize.height / 2 - 50});
        auto deleteBtn = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("GJ_deleteBtn_001.png"), 
            this, 
            menu_selector(GhostMenu::onDelete)
        );
        deleteBtn->setPosition({ 75, -50 });
        deleteBtn->setScale(0.75f);
        buttons->addChild(deleteBtn);

        auto close = CCMenuItemSpriteExtra::create(CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png"), this, menu_selector(GhostMenu::onExit));
        close->setPosition({ -110, 90 });
        buttons->addChild(close);

        return true;
    }

    void addLabel(const char* txt, CCPoint pos) {
        auto lbl = CCLabelBMFont::create(txt, "bigFont.fnt");
        lbl->setScale(0.4f);
        lbl->setPosition(pos);
        m_mainLayer->addChild(lbl);
    }

    void onDelete(CCObject*) {
        geode::createQuickPopup("Delete", "Wipe best ghost?", "Cancel", "Delete", [this](auto, bool confirmed) {
            if (confirmed) {
                if (auto pl = PlayLayer::get()) {
                    auto path = getFilePath(pl->m_level->m_levelID.value());
                    if (fs::exists(path)) fs::remove(path);
                    savedBestRun.clear();
                    bestDistanceEver = 0.0f;
                    FLAlertLayer::create("Success", "Ghost data deleted.", "OK")->show();
                }
            }
        });
    }

    void onToggleRec(CCObject* s) { isRecordingNow = !static_cast<CCMenuItemToggler*>(s)->isOn(); sync(); }
    void textChanged(CCTextInputNode* i) override { try { Mod::get()->setSettingValue("ghost-offset", std::stod(i->getString())); sync(); } catch (...) {} }
    void sync() { if (auto pl = PlayLayer::get()) static_cast<GhostPlayLayer*>(static_cast<CCNode*>(pl))->refreshUI(); }
    void onExit(CCObject*) { if (offsetBox) offsetBox->setDelegate(nullptr); this->removeFromParentAndCleanup(true); }
    void keyBackClicked() override { onExit(nullptr); }
};

// -- YOUR COMMENTS HERE --


// ==========================================
// PAUSE MENU MODS
// ==========================================

class $modify(GhostPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        auto rightMenu = this->getChildByID("right-button-menu");
        if (!rightMenu) rightMenu = typeinfo_cast<CCMenu*>(this->getChildByType<CCMenu>(0));
        
        if (rightMenu) {
            auto icon = CCSprite::create("hkr3d2.bestghost/ghost_btn.png");
            if (!icon) icon = CCSprite::create("ghost_btn.png");

            if (icon) {
                // FIXED SCALE: Hand-tuned for high-res image
                icon->setScale(0.08f); 
            } else {
                icon = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
            }

            auto btn = CCMenuItemSpriteExtra::create(icon, this, menu_selector(GhostPauseLayer::openGhostMenu));
            rightMenu->addChild(btn);
            btn->setPosition({249.0f, 116.0f}); 
            btn->setScale(1.0f); 

            rightMenu->updateLayout();
        }
    }
    void openGhostMenu(CCObject*) { GhostMenu::create()->show(); }
};

// -- YOUR COMMENTS HERE --


// ==========================================
// CORE GAME LOOP (PLAYBACK/RECORDING)
// ==========================================

class $modify(GhostGameLayer, GJBaseGameLayer) {
    void update(float dt) {
        GJBaseGameLayer::update(dt);
        
        auto playLayer = typeinfo_cast<PlayLayer*>(this);
        if (!playLayer || playLayer->m_isPaused) return;

        auto p = playLayer->m_player1;
        auto customPL = static_cast<GhostPlayLayer*>(static_cast<CCNode*>(playLayer));

        // 1. Handle Recording
        if (isRecordingNow) {
            PlayerState currentMode = Cube;
            if (p->m_isShip) currentMode = Ship;
            else if (p->m_isBall) currentMode = Ball;
            else if (p->m_isBird) currentMode = Bird;
            else if (p->m_isDart) currentMode = Dart;
            else if (p->m_isRobot) currentMode = Robot;
            else if (p->m_isSpider) currentMode = Spider;
            else if (p->m_isSwing) currentMode = Swing;

            liveRecording.push_back({ p->getPositionX(), p->getPositionY(), p->getRotation(), currentMode, (p->m_vehicleSize < 1.0f) });
        }
        
        customPL->m_fields->frameClock += 1.0;

        // 2. Handle Playback
        if (customPL->m_fields->ghostPlayer && !savedBestRun.empty()) {
            double userOffset = Mod::get()->getSettingValue<double>("ghost-offset");
            int frameIdx = static_cast<int>(customPL->m_fields->frameClock + (userOffset * 4.0));

            if (frameIdx >= 0 && frameIdx < static_cast<int>(savedBestRun.size())) {
                auto& frame = savedBestRun[frameIdx];
                auto g = customPL->m_fields->ghostPlayer;

                g->setVisible(true);
                g->setPosition({ frame.xPos, frame.yPos });
                g->setRotation(frame.rot);
                g->setScale(frame.smallSize ? 0.6f : 1.0f);

                if (frame.gameMode != customPL->m_fields->lastMode) {
                    g->toggleFlyMode(false, false); g->toggleRollMode(false, false);
                    g->toggleBirdMode(false, false); g->toggleDartMode(false, false);
                    g->toggleRobotMode(false, false); g->toggleSpiderMode(false, false);
                    g->toggleSwingMode(false, false);

                    if (frame.gameMode == Ship) g->toggleFlyMode(true, true);
                    else if (frame.gameMode == Ball) g->toggleRollMode(true, true);
                    else if (frame.gameMode == Bird) g->toggleBirdMode(true, true);
                    else if (frame.gameMode == Dart) g->toggleDartMode(true, true);
                    else if (frame.gameMode == Robot) g->toggleRobotMode(true, true);
                    else if (frame.gameMode == Spider) g->toggleSpiderMode(true, true);
                    else if (frame.gameMode == Swing) g->toggleSwingMode(true, true);

                    customPL->m_fields->lastMode = frame.gameMode;
                }
                g->update(dt);
            } else {
                customPL->m_fields->ghostPlayer->setVisible(false);
            }
        }
    }
};