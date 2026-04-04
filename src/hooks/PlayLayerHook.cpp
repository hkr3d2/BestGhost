#include "PlayLayerHook.hpp"
#include "../GhostManager.hpp"
#include "../GhostRenderer.hpp"

bool PlayLayer::init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
    if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
    
    // Set current level in GhostManager
    GhostManager::get().setCurrentLevel(level->m_levelID.unwrap());
    
    // Start recording a new attempt
    startRecordingAttempt();
    
    return true;
}

void PlayLayer::startRecordingAttempt() {
    m_fields->m_isRecording = true;
    m_fields->m_lastRecordedTime = 0;
    GhostManager::get().startRecording();
    
    // If racing mode is enabled, start the ghost renderer
    if (GhostManager::get().isRacing()) {
        auto* activeGhost = GhostManager::get().getActiveGhost();
        if (activeGhost) {
            GhostRenderer::get().setGhost(activeGhost);
            GhostRenderer::get().start(m_gameState.m_time);
        }
    }
}

void PlayLayer::stopRecordingAttempt(float percentage) {
    if (!m_fields->m_isRecording) return;
    m_fields->m_isRecording = false;
    GhostManager::get().stopRecording(percentage);
    
    // Stop ghost renderer
    GhostRenderer::get().stop();
}

void PlayLayer::recordCurrentFrame() {
    if (!m_fields->m_isRecording) return;
    if (!m_player1) return;
    
    float currentTime = m_gameState.m_time;
    
    // Record every frame (or throttle to every 1/60th of a second)
    if (currentTime - m_fields->m_lastRecordedTime < 0.016f) return;
    m_fields->m_lastRecordedTime = currentTime;
    
    PlayerObject* player = m_player1;
    CCPoint pos = player->getPosition();
    float rotation = player->getRotation();
    bool isHolding = player->m_isHolding;
    int gameMode = static_cast<int>(m_gameState.m_gameMode);
    
    GhostManager::get().recordFrame(
        pos.x, pos.y, rotation, isHolding, gameMode, currentTime
    );
}

void PlayLayer::update(float dt) {
    PlayLayer::update(dt);
    
    // Record current frame for ghost saving
    recordCurrentFrame();
    
    // Update ghost racing
    updateGhostRacing(dt);
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
    stopRecordingAttempt(percentage);
    
    PlayLayer::destroyPlayer(player, object);
}

void PlayLayer::resetLevel() {
    // Start a fresh recording on reset
    startRecordingAttempt();
    
    PlayLayer::resetLevel();
}

void PlayLayer::levelComplete() {
    // Save the completion ghost (100%)
    stopRecordingAttempt(100.0f);
    
    PlayLayer::levelComplete();
}void PlayLayer::update(float dt) {
    PlayLayer::update(dt);
    
    // Record current frame for ghost saving
    recordCurrentFrame();
    
    // Update ghost racing
    updateGhostRacing(dt);
}

void PlayLayer::updateGhostRacing(float dt) {
    if (!GhostManager::get().isRacing()) return;
    if (!GhostRenderer::get().isActive()) return;
    
    float currentTime = m_gameState.m_currentTime;
    GhostRenderer::get().update(currentTime);
    
    // Optional: Show comparison overlay if enabled
    bool showOverlay = Mod::get()->getSettingValue<bool>("show-overlay");
    if (showOverlay && GhostManager::get().getActiveGhost()) {
        auto* ghost = GhostManager::get().getActiveGhost();
        float ghostBest = ghost->percentage;
        float currentPercent = m_gameState.m_percentage;
        
        // Draw text overlay (simplified - you'd use CCLabelBMFont)
        // This would show "You: X% / Ghost: Y%" on screen
    }
}

void PlayLayer::renderGhost() {
    if (!GhostManager::get().isRacing()) return;
    if (!GhostRenderer::get().isActive()) return;
    
    GhostRenderer::get().render();
}

void PlayLayer::destroyPlayer(PlayerObject* player, GameObject* object) {
    // Get current percentage before death
    float percentage = m_gameState.m_percentage;
    
    // Stop recording and save if this was a new best
    stopRecordingAttempt(percentage);
    
    PlayLayer::destroyPlayer(player, object);
}

void PlayLayer::resetLevel() {
    // Start a fresh recording on reset
    startRecordingAttempt();
    
    PlayLayer::resetLevel();
}

void PlayLayer::levelComplete() {
    // Save the completion ghost (100%)
    stopRecordingAttempt(100.0f);
    
    PlayLayer::levelComplete();
}
