#include "GhostRenderer.hpp"
#include <Geode/Geode.hpp>

GhostRenderer& GhostRenderer::get() {
    static GhostRenderer instance;
    return instance;
}

void GhostRenderer::setGhost(GhostData* ghost) {
    m_ghost = ghost;
    if (!ghost) {
        stop();
    }
}

void GhostRenderer::start(float currentGameTime) {
    if (!m_ghost || m_ghost->frames.empty()) {
        m_isActive = false;
        return;
    }
    
    m_startTime = currentGameTime;
    m_currentFrameIndex = 0;
    m_accumulatedTime = 0;
    m_isActive = true;
    m_recentClicks.clear();
    
    createGhostSprite();
    
    // Set initial position
    if (!m_ghost->frames.empty()) {
        auto& firstFrame = m_ghost->frames[0];
        if (m_ghostSprite) {
            m_ghostSprite->setPosition(ccp(firstFrame.x, firstFrame.y));
            m_ghostSprite->setRotation(firstFrame.rotation);
            updateSpriteFrame(firstFrame.gameMode, firstFrame.isHolding);
        }
    }
}

void GhostRenderer::update(float currentGameTime) {
    if (!m_isActive || !m_ghost || m_ghost->frames.empty()) return;
    
    float elapsed = currentGameTime - m_startTime;
    
    // Find the correct frame to display
    for (size_t i = m_currentFrameIndex; i < m_ghost->frames.size() - 1; i++) {
        auto& current = m_ghost->frames[i];
        auto& next = m_ghost->frames[i + 1];
        
        if (elapsed >= current.timeOffset && elapsed < next.timeOffset) {
            m_currentFrameIndex = i;
            
            // Interpolate position
            float t = (elapsed - current.timeOffset) / (next.timeOffset - current.timeOffset);
            float x = current.x + (next.x - current.x) * t;
            float y = current.y + (next.y - current.y) * t;
            float rotation = current.rotation + (next.rotation - current.rotation) * t;
            
            if (m_ghostSprite) {
                m_ghostSprite->setPosition(ccp(x, y));
                m_ghostSprite->setRotation(rotation);
            }
            
            // Update sprite based on game mode at this frame
            updateSpriteFrame(current.gameMode, current.isHolding);
            
            // Track clicks for indicators
            if (current.isHolding != next.isHolding) {
                m_recentClicks.push_back({elapsed, next.isHolding});
            }
            break;
        }
    }
    
    // Clean up old click indicators (older than 0.5 seconds)
    m_recentClicks.erase(
        std::remove_if(m_recentClicks.begin(), m_recentClicks.end(),
            [elapsed](const auto& pair) { return elapsed - pair.first > 0.5f; }),
        m_recentClicks.end()
    );
}

void GhostRenderer::render() {
    if (!m_isActive || !m_ghostSprite) return;
    
    // Set opacity from mod settings
    int opacity = Mod::get()->getSettingValue<int64_t>("ghost-opacity");
    m_ghostSprite->setOpacity(255 * opacity / 100);
    
    // Draw the ghost
    m_ghostSprite->visit();
    
    // Draw click indicators if enabled
    bool showClicks = Mod::get()->getSettingValue<bool>("show-click-indicators");
    if (showClicks) {
        renderClickIndicators(0); // Time parameter not needed here
    }
}

void GhostRenderer::stop() {
    m_isActive = false;
    if (m_ghostSprite) {
        m_ghostSprite->removeFromParent();
        m_ghostSprite = nullptr;
    }
}

std::optional<GhostFrame> GhostRenderer::getCurrentFrame() const {
    if (!m_isActive || !m_ghost || m_currentFrameIndex >= m_ghost->frames.size()) {
        return std::nullopt;
    }
    return m_ghost->frames[m_currentFrameIndex];
}

void GhostRenderer::createGhostSprite() {
    if (m_ghostSprite) {
        m_ghostSprite->removeFromParent();
    }
    
    // Get the current player icon from GameManager
    auto gm = GameManager::sharedState();
    int playerFrame = gm->getPlayerFrame();
    
    // Create a sprite sheet for the ghost (reuse existing GD textures)
    auto sprite = CCSprite::createWithSpriteFrameName(
        fmt::format("player_{:02d}_01.png", playerFrame).c_str()
    );
    
    if (sprite) {
        m_ghostSprite = sprite;
        m_ghostSprite->setColor(ccc3(100, 150, 255)); // Light blue tint
        m_ghostSprite->setOpacity(179); // ~70% opacity
        cocos2d::CCDirector::sharedDirector()->getRunningScene()->addChild(m_ghostSprite);
    }
}

void GhostRenderer::updateSpriteFrame(int gameMode, bool isHolding) {
    if (!m_ghostSprite) return;
    
    // Different animations based on game mode
    // This is simplified — you can expand based on actual sprite names
    std::string suffix = isHolding ? "_02" : "_01";
    std::string modePrefix;
    
    switch(gameMode) {
        case 0: modePrefix = "player"; break;  // cube
        case 1: modePrefix = "ship"; break;
        case 2: modePrefix = "ball"; break;
        case 3: modePrefix = "bird"; break;    // ufo
        case 4: modePrefix = "dart"; break;    // wave
        case 5: modePrefix = "robot"; break;
        case 6: modePrefix = "spider"; break;
        default: modePrefix = "player";
    }
    
    auto gm = GameManager::sharedState();
    int playerFrame = gm->getPlayerFrame();
    
    auto frameName = fmt::format("{}_{:02d}{}.png", modePrefix, playerFrame, suffix);
    auto spriteFrame = CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(frameName.c_str());
    
    if (spriteFrame) {
        m_ghostSprite->setDisplayFrame(spriteFrame);
    }
}

void GhostRenderer::renderClickIndicators(float currentTime) {
    // This would draw rings or particles at click positions
    // Simplified for now — can be expanded later
    for (const auto& [time, isPress] : m_recentClicks) {
        // Draw a small expanding ring at ghost's position
        // Requires custom drawing or particle system
    }
}
