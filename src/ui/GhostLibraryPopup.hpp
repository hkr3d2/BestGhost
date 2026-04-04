#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include "GhostData.hpp"

using namespace geode::prelude;

class GhostLibraryPopup : public Popup<>, public geode::Popup<> {
protected:
    bool setup(bool selectionMode) override;
    
public:
    static GhostLibraryPopup* create(bool selectionMode = false);
    
private:
    bool m_selectionMode = false;
    CCMenu* m_listMenu = nullptr;
    ScrollLayer* m_scrollLayer = nullptr;
    
    void refreshList();
    void onRaceGhost(CCObject* sender);
    void onWatchGhost(CCObject* sender);
    void onDeleteGhost(CCObject* sender);
    void onDeleteAll(CCObject* sender);
    void onExportGhost(CCObject* sender);
    void onImportGhost(CCObject* sender);
    void onClose(CCObject* sender);
    
    float getPercentageFromTag(CCObject* sender);
};
