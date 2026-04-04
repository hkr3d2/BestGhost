#pragma once
#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>

using namespace geode::prelude;

class $modify(LevelInfoLayer) {
    struct Fields {
        CCMenuItemSpriteExtra* m_ghostLibraryBtn = nullptr;
    };
    
public:
    bool init(GJGameLevel* level, bool challenge);
    void onPlay(CCObject* sender);
    void onPracticeMode(CCObject* sender);
    
    void onGhostLibrary(CCObject* sender);
    
    void addGhostLibraryButton();
    void refreshGhostButton();
};
