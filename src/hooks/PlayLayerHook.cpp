#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

class $modify(PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        log::info("Best Ghost: PlayLayer initialized for level: {}", level->m_levelName);
        return PlayLayer::init(level, useReplay, dontCreateObjects);
    }
};    
    // Record current frame for ghost saving
    this->recordCurrentFrame();
    
    // Update ghost racing
    this->updateGhostRacing(dt);
}

void PlayLayer::updateGhostRacing(float dt) {
    if (!GhostManager::get().isRacing()) return;
    if (!GhostRenderer::get().isActive()) return;
    
    float currentTime = m_gameState.m_time;
    GhostRenderer::get().update(currentTime);
}

void PlayLayer::destroyPlayer(PlayerObject* player, GameObject* object) {
    // Get current percentage before death
    float percentage = m_gameState.m_percentage;
    
    // Stop recording and save if this was a new best
    this->stopRecordingAttempt(percentage);
    
    PlayLayer::destroyPlayer(player, object);
}

void PlayLayer::resetLevel() {
    // Start a fresh recording on reset
    this->startRecordingAttempt();
    
    PlayLayer::resetLevel();
}

void PlayLayer::levelComplete() {
    // Save the completion ghost (100%)
    this->stopRecordingAttempt(100.0f);
    
    PlayLayer::levelComplete();
}
