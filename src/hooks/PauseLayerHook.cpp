#include "PauseLayerHook.hpp"
#include "../GhostManager.hpp"

bool PauseLayer::init() {
    if (!PauseLayer::init()) return false;
    
    addRaceControls();
    
    return true;
}

void PauseLayer::addRaceControls() {
    auto menu = CCMenu::create();
    
    // Create toggle button for race mode
    auto onSprite = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
    auto offSprite = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
    
    bool isRacing = GhostManager::get().isRacing();
    auto toggle = CCMenuItemToggler::create(
        offSprite, onSprite,
        this, menu_selector(PauseLayer::onToggleRace)
    );
    toggle->toggle(isRacing);
    toggle->setPosition(ccp(200, 200));
    menu->addChild(toggle);
    m_raceToggle = toggle;
    
    // Label for toggle
    auto toggleLabel = CCLabelBMFont::create("Race Ghost", "bigFont.fnt");
    toggleLabel->setPosition(ccp(200, 175));
    toggleLabel->setScale(0.6f);
    menu->addChild(toggleLabel);
    
    // Show selected ghost
    updateSelectedGhostLabel();
    if (m_selectedGhostLabel) {
        m_selectedGhostLabel->setPosition(ccp(200, 145));
        m_selectedGhostLabel->setScale(0.5f);
        menu->addChild(m_selectedGhostLabel);
    }
    
    // Button to select a different ghost
    auto selectBtn = CCMenuItemSpriteExtra::create(
        CCSprite::createWithSpriteFrameName("GJ_arrow01_001.png"),
        this,
        menu_selector(PauseLayer::onSelectGhost)
    );
    selectBtn->setPosition(ccp(200, 115));
    menu->addChild(selectBtn);
    
    this->addChild(menu);
}

void PauseLayer::updateSelectedGhostLabel() {
    auto* activeGhost = GhostManager::get().getActiveGhost();
    if (activeGhost && GhostManager::get().isRacing()) {
        std::string text = fmt::format("Ghost: {}%", activeGhost->percentage);
        if (m_selectedGhostLabel) {
            m_selectedGhostLabel->setString(text.c_str());
        } else {
            m_selectedGhostLabel = CCLabelBMFont::create(text.c_str(), "bigFont.fnt");
        }
    } else if (m_selectedGhostLabel) {
        m_selectedGhostLabel->setString("Ghost: none");
    }
}

void PauseLayer::onToggleRace(CCObject* sender) {
    bool newState = !GhostManager::get().isRacing();
    GhostManager::get().setRacing(newState);
    
    if (m_raceToggle) {
        m_raceToggle->toggle(newState);
    }
    
    updateSelectedGhostLabel();
}

void PauseLayer::onSelectGhost(CCObject* sender) {
    // Open ghost selection popup
    auto popup = GhostLibraryPopup::create(true); // true = selection mode
    popup->show();
}

void PauseLayer::onResume(CCObject* sender) {
    // Ensure ghost renderer is ready when resuming
    if (GhostManager::get().isRacing()) {
        auto* activeGhost = GhostManager::get().getActiveGhost();
        if (activeGhost) {
            auto* playLayer = PlayLayer::get();
            if (playLayer) {
                GhostRenderer::get().setGhost(activeGhost);
                GhostRenderer::get().start(playLayer->m_gameState.m_currentTime);
            }
        }
    }
    
    PauseLayer::onResume(sender);
}
