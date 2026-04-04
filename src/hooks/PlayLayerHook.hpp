#pragma once
#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

class $modify(PlayLayer) {
    struct Fields {
        bool m_isRecording = false;
        float m_lastRecordedTime = 0;
    };
    
public:
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects);
    void update(float dt);
    void resetLevel();
    void destroyPlayer(PlayerObject* player, GameObject* object);
    void levelComplete();
    
    void startRecordingAttempt();
    void stopRecordingAttempt(float percentage);
    void recordCurrentFrame();
    void updateGhostRacing(float dt);
    void renderGhost();
};
