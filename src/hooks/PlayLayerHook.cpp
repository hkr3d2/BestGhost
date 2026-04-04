#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

class $modify(PlayLayer) {
    struct Fields {
        float m_bestPercentage = 0;
        float m_levelLength = 0;
    };
    
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        
        // Get level length (the X position where the level ends)
        // This is typically the last object's X position
        if (m_level) {
            // Try to get level length from the level object
            // If this doesn't work, we'll calculate it dynamically
            m_fields->m_levelLength = 100000; // Fallback value
            log::info("Best Ghost: Level loaded - {}", level->m_levelName);
        }
        
        return true;
    }
    
    float getCurrentPercentage() {
        if (!m_player1) return 0;
        
        // Get player's X position
        float playerX = m_player1->getPositionX();
        
        // Calculate percentage based on level length
        // Most Geometry Dash levels end around X position 100000-200000
        // We'll use a dynamic approach: track the farthest X reached
        static float farthestX = 0;
        
        if (playerX > farthestX) {
            farthestX = playerX;
        }
        
        // Estimate percentage based on farthest X
        // A typical extreme demon might end at ~150000
        // This is an approximation - we can improve it later
        float estimatedPercent = (farthestX / 150000.0f) * 100.0f;
        if (estimatedPercent > 100) estimatedPercent = 100;
        
        return estimatedPercent;
    }
    
    void update(float dt) {
        PlayLayer::update(dt);
        
        if (!m_player1) return;
        
        float currentPercent = getCurrentPercentage();
        
        // Check if this is a new best
        if (currentPercent > m_fields->m_bestPercentage) {
            m_fields->m_bestPercentage = currentPercent;
            log::info("Best Ghost: New best! {:.1f}%", currentPercent);
            
            // Show notification every 10% for testing
            int intPercent = static_cast<int>(currentPercent);
            if (intPercent % 10 == 0) {
                Notification::create(fmt::format("Progress: {}%", intPercent), NotificationIcon::Info)->show();
            }
        }
    }
    
    void destroyPlayer(PlayerObject* player, GameObject* object) {
        log::info("Best Ghost: Died at {:.1f}%", m_fields->m_bestPercentage);
        PlayLayer::destroyPlayer(player, object);
    }
    
    void levelComplete() {
        log::info("Best Ghost: Level completed at 100%!");
        Notification::create("Level Completed!", NotificationIcon::Success)->show();
        PlayLayer::levelComplete();
    }
};
