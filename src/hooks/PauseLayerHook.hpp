#pragma once
#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>

using namespace geode::prelude;

class $modify(PauseLayer) {
public:
    void onToggleRace(CCObject* sender);
    void onSelectGhost(CCObject* sender);
    
    // Hooked methods
    bool init();
    void onResume(CCObject* sender);
    
private:
    CCMenuItemToggler* m_raceToggle = nullptr;
    CCLabelBMFont* m_selectedGhostLabel = nullptr;
    
    void addRaceControls();
    void updateSelectedGhostLabel();
};
