#include "LevelInfoLayerHook.hpp"
#include "../GhostManager.hpp"
#include "../ui/GhostLibraryPopup.hpp"

bool LevelInfoLayer::init(GJGameLevel* level) {
    if (!LevelInfoLayer::init(level)) return false;
    
    addGhostLibraryButton();
    
    // Set callback to refresh button when ghosts change
    GhostManager::get().setOnGhostsUpdatedCallback([this]() {
        this->refreshGhostButton();
    });
    
    return true;
}

void LevelInfoLayer::addGhostLibraryButton() {
    // Create button
    auto buttonSprite = CCSprite::createWithSpriteFrameName("GJ_undoBtn_001.png");
    if (!buttonSprite) return;
    
    // Tint it blue to look different
    buttonSprite->setColor(ccc3(100, 150, 255));
    
    auto button = CCMenuItemSpriteExtra::create(
        buttonSprite,
        this,
        menu_selector(LevelInfoLayer::onGhostLibrary)
    );
    
    auto menu = CCMenu::create();
    menu->addChild(button);
    
    // Position it near other buttons (adjust coordinates as needed)
    button->setPosition(ccp(350, 200));
    
    this->addChild(menu);
    m_ghostLibraryBtn = button;
    
    refreshGhostButton();
}

void LevelInfoLayer::refreshGhostButton() {
    if (!m_ghostLibraryBtn) return;
    
    const auto& ghosts = GhostManager::get().getGhosts();
    bool hasGhosts = !ghosts.empty();
    
    // Visual indicator if ghosts exist
    if (hasGhosts) {
        // Add a badge or make button glow
        // Simplified: just set a scale effect
        m_ghostLibraryBtn->setScale(1.0f);
    } else {
        m_ghostLibraryBtn->setScale(0.9f);
    }
}

void LevelInfoLayer::onGhostLibrary(CCObject* sender) {
    auto popup = GhostLibraryPopup::create();
    popup->show();
}

void LevelInfoLayer::onPlay(CCObject* sender) {
    // Reset racing mode when playing normally
    GhostManager::get().setRacing(false);
    LevelInfoLayer::onPlay(sender);
}

void LevelInfoLayer::onPracticeMode(CCObject* sender) {
    // Reset racing mode for practice too
    GhostManager::get().setRacing(false);
    LevelInfoLayer::onPracticeMode(sender);
}
