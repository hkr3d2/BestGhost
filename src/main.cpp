#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <vector>
#include <fstream>
#include <filesystem>
#include <algorithm>

using namespace geode::prelude;
namespace fs = std::filesystem;

struct GhostFrame {
    float x;
    float y;
};

struct GhostRun {
    float distance = 0.0f;
    std::vector<GhostFrame> frames;
};

// Global State
bool g_isRecordingEnabled = false;
std::vector<GhostFrame> g_currentAttemptData;
GhostRun g_activeGhost; 

/**
 * File Management
 */
fs::path getGhostPath(int levelID, int slot) {
    auto path = Mod::get()->getSaveDir() / "ghosts";
    if (!fs::exists(path)) fs::create_directories(path);
    return path / (std::to_string(levelID) + "_" + std::to_string(slot) + ".ghst");
}

void saveGhostToSlot(int levelID, int slot, const GhostRun& run) {
    std::ofstream file(getGhostPath(levelID, slot), std::ios::binary);
    if (!file.is_open()) return;
    file.write(reinterpret_cast<const char*>(&run.distance), sizeof(float));
    for (const auto& frame : run.frames) {
        file.write(reinterpret_cast<const char*>(&frame), sizeof(GhostFrame));
    }
}

GhostRun loadGhostFromSlot(int levelID, int slot) {
    GhostRun run;
    auto path = getGhostPath(levelID, slot);
    if (!fs::exists(path)) return run;

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return run;

    file.read(reinterpret_cast<char*>(&run.distance), sizeof(float));
    GhostFrame frame;
    while (file.read(reinterpret_cast<char*>(&frame), sizeof(GhostFrame))) {
        run.frames.push_back(frame);
    }
    return run;
}

/**
 * Leaderboard Logic: Shift and Save
 */
void updateLeaderboard(int levelID, GhostRun currentRun) {
    std::vector<GhostRun> leaderboard;
    
    // 1. Load all existing ghosts
    for (int i = 1; i <= 5; i++) {
        auto g = loadGhostFromSlot(levelID, i);
        if (g.distance > 0.1f) leaderboard.push_back(g);
    }

    // 2. Add current run to the mix
    leaderboard.push_back(currentRun);

    // 3. Sort by distance (highest/best first)
    std::sort(leaderboard.begin(), leaderboard.end(), [](const GhostRun& a, const GhostRun& b) {
        return a.distance > b.distance;
    });

    // 4. Remove duplicates (keep unique distances)
    leaderboard.erase(std::unique(leaderboard.begin(), leaderboard.end(), [](const GhostRun& a, const GhostRun& b) {
        return std::abs(a.distance - b.distance) < 0.1f;
    }), leaderboard.end());

    // 5. Save only the top 5
    for (int i = 0; i < 5 && i < leaderboard.size(); i++) {
        saveGhostToSlot(levelID, i + 1, leaderboard[i]);
    }
}

/**
 * Hooks
 */
class $modify(MyPlayLayer, PlayLayer) {
    struct Fields {
        CCSprite* m_ghostVisual = nullptr;
        CCLabelBMFont* m_statusLabel = nullptr;
        size_t m_playbackIndex = 0;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        // Load the ghost selected in settings
        int64_t slot = Mod::get()->getSettingValue<int64_t>("ghost-slot");
        g_activeGhost = loadGhostFromSlot(level->m_levelID.value(), static_cast<int>(slot));
        
        g_isRecordingEnabled = false;
        m_fields->m_playbackIndex = 0;
        g_currentAttemptData.clear();

        auto label = CCLabelBMFont::create("", "bigFont.fnt");
        label->setScale(0.35f);
        label->setOpacity(150);
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
        int64_t slot = Mod::get()->getSettingValue<int64_t>("ghost-slot");

        if (!g_isRecordingEnabled) {
            m_fields->m_statusLabel->setString(fmt::format("Ghost (Slot {}): OFF", slot).c_str());
            m_fields->m_statusLabel->setColor({ 200, 200, 200 });
        } else {
            m_fields->m_statusLabel->setString(fmt::format("Ghost (Slot {}): RECORDING", slot).c_str());
            m_fields->m_statusLabel->setColor({ 0, 255, 255 });
        }
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        
        // Save current run if it exists
        if (g_isRecordingEnabled && !g_currentAttemptData.empty()) {
            GhostRun newRun;
            newRun.distance = g_currentAttemptData.back().x;
            newRun.frames = g_currentAttemptData;
            updateLeaderboard(m_level->m_levelID.value(), newRun);
        }
        
        // Reload active ghost for next attempt
        int64_t slot = Mod::get()->getSettingValue<int64_t>("ghost-slot");
        g_activeGhost = loadGhostFromSlot(m_level->m_levelID.value(), static_cast<int>(slot));

        g_currentAttemptData.clear();
        m_fields->m_playbackIndex = 0;
        if (m_fields->m_ghostVisual) m_fields->m_ghostVisual->setVisible(false);
        updateUI();
    }
};

class $modify(MyPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        auto menu = this->getChildByID("right-button-menu");
        if (menu) {
            auto toggler = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(MyPauseLayer::onToggleGhost), 0.6f);
            toggler->toggle(g_isRecordingEnabled);
            menu->addChild(toggler);
        }
    }

    void onToggleGhost(CCObject* sender) {
        g_isRecordingEnabled = !static_cast<CCMenuItemToggler*>(sender)->isOn();
        if (auto pl = PlayLayer::get()) pl->resetLevel();
    }
};

class $modify(MyBaseGameLayer, GJBaseGameLayer) {
    void update(float dt) {
        GJBaseGameLayer::update(dt);
        if (!g_isRecordingEnabled) return;

        auto playLayer = typeinfo_cast<PlayLayer*>(this);
        if (!playLayer || playLayer->m_isPaused) return;

        auto player = playLayer->m_player1;
        if (!player) return;

        // Record current frame
        g_currentAttemptData.push_back({ player->getPositionX(), player->getPositionY() });

        // Display ghost playback
        auto myPL = static_cast<MyPlayLayer*>(static_cast<CCNode*>(playLayer));
        if (myPL->m_fields->m_ghostVisual && !g_activeGhost.frames.empty()) {
            size_t idx = myPL->m_fields->m_playbackIndex;
            
            if (idx < g_activeGhost.frames.size()) {
                float offset = static_cast<float>(Mod::get()->getSettingValue<double>("ghost-offset"));
                myPL->m_fields->m_ghostVisual->setVisible(true);
                myPL->m_fields->m_ghostVisual->setPosition({ 
                    g_activeGhost.frames[idx].x + offset, 
                    g_activeGhost.frames[idx].y 
                });
                myPL->m_fields->m_playbackIndex++;
            } else {
                myPL->m_fields->m_ghostVisual->setVisible(false);
            }
        }
    }
};

$execute {
    listenForSettingChanges<bool>("open-library", [](bool value) {
        if (value) {
            auto path = Mod::get()->getSaveDir() / "ghosts";
            if (!fs::exists(path)) fs::create_directories(path);
            utils::file::openFolder(path);
            Mod::get()->setSettingValue("open-library", false);
        }
    });
}
