#pragma once
#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>

using namespace geode::prelude;

class $modify(PauseLayer) {
    struct Fields {
        CCMenuItemToggler* m_raceToggle = nullptr;
        CCLabelBMFont* m_selectedGhostLabel = nullptr;
    };
    
public:
    bool init(bool unfocused);
    void onResume(CCObject* sender);
    
    void onToggleRace(CCObject* sender);
    void onSelectGhost(CCObject* sender);
    
    void addRaceControls();
    void updateSelectedGhostLabel();
};
