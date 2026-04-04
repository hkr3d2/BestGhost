#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

class $modify(PlayLayer) {
    struct Fields {
        float m_bestPercentage = 0;
        float m_farthestX = 0;
        bool m_hasStarted = false;
        int m_lastNotifiedPercent = 0;
    };
    
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        
        // Reset tracking
        m_fields->m_bestPercentage = 0;
        m_fields->m_farthestX = 0;
        m_fields->m_hasStarted = false;
        m_fields->m_lastNotifiedPercent = 0;
        
        log::info("Best Ghost: Level loaded - {}", level->m_levelName);
        
        return true;
    }
    
    float getCurrentPercentage() {
        if (!m_player1) return 0;
        
        // Get player's X position
        float playerX = m_player1->getPositionX();
        
        // Ignore positions before the player actually starts moving (X < 100)
        if (playerX < 100) return 0;
        
        // Track farthest X reached
        if (playerX > m_fields->m_farthestX) {
            m_fields->m_farthestX = playerX;
            m_fields->m_hasStarted = true;
        }
        
        // Calculate percentage based on farthest X
        // Most levels end around 80000-150000
        // Using 100000 as a rough estimate
        float estimatedPercent = (m_fields->m_farthestX / 100000.0f) * 100.0f;
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
            
            // Log every new best
            log::info("Best Ghost: New best! {:.1f}%", currentPercent);
            
            // Show notification every 10% using FLAlert (more reliable)
            int intPercent = static_cast<int>(currentPercent);
            if (intPercent >= m_fields->m_lastNotifiedPercent + 10) {
                m_fields->m_lastNotifiedPercent = intPercent - (intPercent % 10);
                
                // Use FLAlert instead of Notification
                FLAlertLayer::create(
                    "Best Ghost",
                    fmt::format("New best: {}%", m_fields->m_lastNotifiedPercent),
                    "OK"
                )->show();
            }
        }
    }
    
    void destroyPlayer(PlayerObject* player, GameObject* object) {
        // Only log if we actually made progress (X > 100)
        if (m_fields->m_hasStarted && m_fields->m_bestPercentage > 0) {
            log::info("Best Ghost: Died at {:.1f}%", m_fields->m_bestPercentage);
        }
        
        PlayLayer::destroyPlayer(player, object);
    }
    
    void levelComplete() {
        log::info("Best Ghost: Level completed at 100%!");
        
        FLAlertLayer::create(
            "Best Ghost",
            "Level completed!",
            "OK"
        )->show();
        
        PlayLayer::levelComplete();
    }
};
