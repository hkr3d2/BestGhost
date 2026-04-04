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
            
            // Show notification every 10%
            int intPercent = static_cast<int>(currentPercent);
            if (intPercent >= m_fields->m_lastNotifiedPercent + 10) {
                m_fields->m_lastNotifiedPercent = intPercent - (intPercent % 10);
                
                // Use a simple log instead of popup for now (popups can cause issues)
                log::info("Best Ghost: Milestone reached - {}%", m_fields->m_lastNotifiedPercent);
            }
        }
    }
    
    void destroyPlayer(PlayerObject* player, GameObject* object) {
        // Always log deaths with useful info
        if (m_fields->m_hasStarted) {
            log::info("Best Ghost: DIED at {:.1f}% (farthest X: {:.0f})", 
                      m_fields->m_bestPercentage, m_fields->m_farthestX);
        } else {
            log::info("Best Ghost: DIED at start (no progress made)");
        }
        
        // Call the original destroyPlayer
        PlayLayer::destroyPlayer(player, object);
    }
    
    void levelComplete() {
        log::info("Best Ghost: LEVEL COMPLETED at 100%!");
        log::info("Best Ghost: Final stats - Best: {:.1f}%, Farthest X: {:.0f}", 
                  m_fields->m_bestPercentage, m_fields->m_farthestX);
        
        // Call the original levelComplete
        PlayLayer::levelComplete();
    }
};
