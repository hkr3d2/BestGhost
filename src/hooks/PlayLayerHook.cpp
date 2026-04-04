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
    
    void update(float dt) {
        // Call parent update FIRST
        PlayLayer::update(dt);
        
        if (!m_player1) return;
        
        // Get player's X position directly every frame
        float playerX = m_player1->getPositionX();
        
        // Ignore positions before the player actually starts moving (X < 100)
        if (playerX < 100) return;
        
        // Mark that we've started
        if (!m_fields->m_hasStarted) {
            m_fields->m_hasStarted = true;
            log::info("Best Ghost: Player started moving!");
        }
        
        // Track farthest X reached
        if (playerX > m_fields->m_farthestX) {
            m_fields->m_farthestX = playerX;
            
            // Calculate percentage based on farthest X
            // Most levels end around 80000-150000
            float estimatedPercent = (m_fields->m_farthestX / 100000.0f) * 100.0f;
            if (estimatedPercent > 100) estimatedPercent = 100;
            
            // Log when we reach new milestones
            int intPercent = static_cast<int>(estimatedPercent);
            if (intPercent > m_fields->m_lastNotifiedPercent) {
                m_fields->m_lastNotifiedPercent = intPercent;
                log::info("Best Ghost: Progress - X: {:.0f}, Estimated: {}%", 
                          m_fields->m_farthestX, intPercent);
            }
            
            // Update best percentage
            if (estimatedPercent > m_fields->m_bestPercentage) {
                m_fields->m_bestPercentage = estimatedPercent;
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
        // Get final position
        float finalX = m_player1 ? m_player1->getPositionX() : 0;
        
        log::info("Best Ghost: LEVEL COMPLETED!");
        log::info("Best Ghost: Final X position: {:.0f}", finalX);
        log::info("Best Ghost: Best percentage during run: {:.1f}%", m_fields->m_bestPercentage);
        log::info("Best Ghost: Farthest X reached: {:.0f}", m_fields->m_farthestX);
        
        // Call the original levelComplete
        PlayLayer::levelComplete();
    }
};
