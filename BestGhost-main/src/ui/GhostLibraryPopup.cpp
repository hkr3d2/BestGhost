#include "GhostLibraryPopup.hpp"
#include "../GhostManager.hpp"
#include "../GhostRenderer.hpp"
#include <Geode/ui/ScrollLayer.hpp>

GhostLibraryPopup* GhostLibraryPopup::create(bool selectionMode) {
    auto ret = new GhostLibraryPopup();
    if (ret && ret->setup(selectionMode)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool GhostLibraryPopup::setup(bool selectionMode) {
    m_selectionMode = selectionMode;
    
    // Set popup size
    this->setTitle(m_selectionMode ? "Select Ghost" : "Ghost Library");
    this->setContentSize(ccp(400, 350));
    
    // Create background
    auto bg = CCScale9Sprite::create("GJ_square01.png");
    bg->setContentSize(ccp(380, 330));
    bg->setPosition(ccp(200, 175));
    this->addChild(bg);
    
    // Create scroll layer
    m_scrollLayer = ScrollLayer::create(ccp(360, 260));
    m_scrollLayer->setPosition(ccp(20, 50));
    m_scrollLayer->setContentSize(ccp(360, 260));
    this->addChild(m_scrollLayer);
    
    m_listMenu = CCMenu::create();
    m_listMenu->setPosition(ccp(0, 0));
    m_scrollLayer->addChild(m_listMenu);
    
    // Close button
    auto closeBtn = CCMenuItemSpriteExtra::create(
        CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png"),
        this,
        menu_selector(GhostLibraryPopup::onClose)
    );
    closeBtn->setPosition(ccp(370, 320));
    this->addChild(closeBtn);
    
    // Delete all button (only in full library mode)
    if (!m_selectionMode) {
        auto deleteAllBtn = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("GJ_deleteBtn_001.png"),
            this,
            menu_selector(GhostLibraryPopup::onDeleteAll)
        );
        deleteAllBtn->setPosition(ccp(50, 320));
        this->addChild(deleteAllBtn);
        
        auto importBtn = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("GJ_downloadBtn_001.png"),
            this,
            menu_selector(GhostLibraryPopup::onImportGhost)
        );
        importBtn->setPosition(ccp(350, 320));
        this->addChild(importBtn);
    }
    
    refreshList();
    
    return true;
}

void GhostLibraryPopup::refreshList() {
    if (!m_listMenu) return;
    
    m_listMenu->removeAllChildrenWithCleanup(true);
    
    const auto& ghosts = GhostManager::get().getGhosts();
    
    if (ghosts.empty()) {
        auto label = CCLabelBMFont::create("No ghosts saved yet", "bigFont.fnt");
        label->setPosition(ccp(180, 130));
        label->setScale(0.7f);
        m_listMenu->addChild(label);
        m_scrollLayer->setContentHeight(260);
        return;
    }
    
    float y = 250;
    int index = 0;
    
    // Show ghosts from highest percentage to lowest
    for (auto it = ghosts.rbegin(); it != ghosts.rend(); ++it) {
        const auto& [pct, ghost] = *it;
        
        // Background for each row
        auto rowBg = CCScale9Sprite::create("GJ_square01.png");
        rowBg->setContentSize(ccp(340, 40));
        rowBg->setPosition(ccp(180, y));
        rowBg->setOpacity(100);
        m_listMenu->addChild(rowBg);
        
        // Percentage label
        auto pctLabel = CCLabelBMFont::create(
            fmt::format("{}%", static_cast<int>(pct)).c_str(),
            "bigFont.fnt"
        );
        pctLabel->setPosition(ccp(40, y));
        pctLabel->setScale(0.6f);
        m_listMenu->addChild(pctLabel);
        
        // Date label
        auto dateLabel = CCLabelBMFont::create(ghost.timestamp.c_str(), "bigFont.fnt");
        dateLabel->setPosition(ccp(140, y));
        dateLabel->setScale(0.5f);
        m_listMenu->addChild(dateLabel);
        
        if (m_selectionMode) {
            // Select button
            auto selectBtn = CCMenuItemSpriteExtra::create(
                CCSprite::createWithSpriteFrameName("GJ_playBtn2_001.png"),
                this,
                menu_selector(GhostLibraryPopup::onRaceGhost)
            );
            selectBtn->setPosition(ccp(300, y));
            selectBtn->setTag(static_cast<int>(pct * 100)); // Store percentage in tag
            m_listMenu->addChild(selectBtn);
        } else {
            // Race button
            auto raceBtn = CCMenuItemSpriteExtra::create(
                CCSprite::createWithSpriteFrameName("GJ_playBtn2_001.png"),
                this,
                menu_selector(GhostLibraryPopup::onRaceGhost)
            );
            raceBtn->setPosition(ccp(260, y));
            raceBtn->setTag(static_cast<int>(pct * 100));
            m_listMenu->addChild(raceBtn);
            
            // Watch button
            auto watchBtn = CCMenuItemSpriteExtra::create(
                CCSprite::createWithSpriteFrameName("GJ_viewBtn_001.png"),
                this,
                menu_selector(GhostLibraryPopup::onWatchGhost)
            );
            watchBtn->setPosition(ccp(310, y));
            watchBtn->setTag(static_cast<int>(pct * 100));
            m_listMenu->addChild(watchBtn);
            
            // Delete button
            auto deleteBtn = CCMenuItemSpriteExtra::create(
                CCSprite::createWithSpriteFrameName("GJ_deleteBtn_001.png"),
                this,
                menu_selector(GhostLibraryPopup::onDeleteGhost)
            );
            deleteBtn->setPosition(ccp(340, y));
            deleteBtn->setTag(static_cast<int>(pct * 100));
            m_listMenu->addChild(deleteBtn);
        }
        
        y -= 45;
        index++;
    }
    
    // Set scroll content height
    float contentHeight = std::max(260.0f, y + 100);
    m_scrollLayer->setContentHeight(contentHeight);
    m_listMenu->setContentSize(ccp(340, contentHeight));
}

float GhostLibraryPopup::getPercentageFromTag(CCObject* sender) {
    auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
    return btn->getTag() / 100.0f;
}

void GhostLibraryPopup::onRaceGhost(CCObject* sender) {
    float pct = getPercentageFromTag(sender);
    
    if (m_selectionMode) {
        // In selection mode, just set the active ghost
        GhostManager::get().setActiveGhost(pct);
        GhostManager::get().setRacing(true);
        this->onClose(nullptr);
    } else {
        // Start level with this ghost
        GhostManager::get().setActiveGhost(pct);
        GhostManager::get().setRacing(true);
        
        auto* playLayer = PlayLayer::get();
        if (playLayer) {
            playLayer->onQuit(nullptr);
        }
        
        // Start the level
        auto* levelInfo = LevelInfoLayer::get();
        if (levelInfo) {
            levelInfo->onPlay(nullptr);
        }
        
        this->onClose(nullptr);
    }
}

void GhostLibraryPopup::onWatchGhost(CCObject* sender) {
    float pct = getPercentageFromTag(sender);
    auto& ghosts = GhostManager::get().getGhosts();
    auto it = ghosts.find(pct);
    
    if (it != ghosts.end()) {
        // For watch mode, we'd create a replay viewer
        // This is a simplified version - you'd need a ReplayLayer
        Notification::create("Watch mode coming soon!", NotificationIcon::Info)->show();
    }
}

void GhostLibraryPopup::onDeleteGhost(CCObject* sender) {
    float pct = getPercentageFromTag(sender);
    GhostManager::get().deleteGhost(pct);
    refreshList();
}

void GhostLibraryPopup::onDeleteAll(CCObject* sender) {
    GhostManager::get().deleteAllGhosts();
    refreshList();
}

void GhostLibraryPopup::onExportGhost(CCObject* sender) {
    // Open file picker to save ghost
    // This requires platform-specific file dialog
    Notification::create("Export coming soon!", NotificationIcon::Info)->show();
}

void GhostLibraryPopup::onImportGhost(CCObject* sender) {
    // Open file picker to load ghost
    Notification::create("Import coming soon!", NotificationIcon::Info)->show();
}

void GhostLibraryPopup::onClose(CCObject* sender) {
    this->setKeypadEnabled(false);
    this->removeFromParent();
}
