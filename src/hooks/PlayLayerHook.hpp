#pragma once
#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

class $modify(PlayLayer) {
private:
    bool m_isRecording = false;
    float m_lastRecordedTime = 0;
    
public:
    // Hooked methods
    bool init(GJGameLevel* level);
    void update(float dt);
    void resetLevel();
    void destroyPlayer(PlayerObject* player, GameObject* object);
    void levelComplete();
    
    // Custom methods
    void startRecordingAttempt();
    void stopRecordingAttempt(float percentage);
    void recordCurrentFrame();
    void updateGhostRacing(float dt);
    void renderGhost();
};
