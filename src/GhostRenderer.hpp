#pragma once
#include "GhostData.hpp"
#include <Geode/Geode.hpp>

using namespace geode::prelude;

class GhostRenderer {
private:
    GhostData* m_ghost = nullptr;
    CCSprite* m_ghostSprite = nullptr;
    int m_currentFrameIndex = 0;
    float m_accumulatedTime = 0;
    float m_startTime = 0;
    bool m_isActive = false;
    
    // For click indicators
    std::vector<std::pair<float, bool>> m_recentClicks; // time, wasClick
    
public:
    static GhostRenderer& get();
    
    // Set which ghost to render (nullptr to disable)
    void setGhost(GhostData* ghost);
    
    // Start rendering from the beginning
    void start(float currentGameTime);
    
    // Update ghost position based on current game time
    void update(float currentGameTime);
    
    // Render the ghost at its current position
    void render();
    
    // Stop rendering
    void stop();
    
    // Is currently rendering a ghost?
    bool isActive() const { return m_isActive && m_ghost != nullptr; }
    
    // Get ghost's current frame for collision detection (if needed)
    std::optional<GhostFrame> getCurrentFrame() const;
    
private:
    void createGhostSprite();
    void updateSpriteFrame(int gameMode, bool isHolding);
    void renderClickIndicators(float currentTime);
};
