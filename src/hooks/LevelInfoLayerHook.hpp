#pragma once
#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>

using namespace geode::prelude;

class $modify(LevelInfoLayer) {
public:
    void onGhostLibrary(CCObject* sender);
    
    // Hooked methods
    bool init(GJGameLevel* level);
    void onPlay(CCObject* sender);
    void onPracticeMode(CCObject* sender);
    
private:
    CCMenuItemSpriteExtra* m_ghostLibraryBtn = nullptr;
    void addGhostLibraryButton();
    void refreshGhostButton();
};
