#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

class $modify(PlayLayer) {
    struct Fields {
        float m_bestX = 0;
        float m_deathX = 0;
        bool m_didProgress = false;
    };
    
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        
        // Reset tracking
        m_fields->m_bestX = 0;
        m_fields->m_deathX = 0;
        m_fields->m_didProgress = false;
        
        log::info("Best Ghost: Level loaded - {}", level->m_levelName);
        
        return true;
    }
    
    void update(float dt) {
        PlayLayer::update(dt);
        
        if (!m_player1) return;
        
        // Get player's X position
        float playerX = m_player1->getPositionX();
        
        // Track farthest X reached
        if (playerX > m_fields->m_bestX) {
            m_fields->m_bestX = playerX;
            
            // Calculate percentage based on final X position from your completion
            // Final X was 30302, so we'll use that as 100%
            float percentage = (playerX / 30302.0f) * 100.0f;
            if (percentage > 100) percentage = 100;
            
            // Log every 10%
            int intPercent = static_cast<int>(percentage);
            if (intPercent % 10 == 0 && intPercent > 0) {
                log::info("Best Ghost: Progress - X: {:.0f} / {}%", playerX, intPercent);
            }
        }
        
        // Mark that we made progress if X > 100
        if (playerX > 100 && !m_fields->m_didProgress) {
            m_fields->m_didProgress = true;
            log::info("Best Ghost: Player started moving!");
        }
    }
    
    void destroyPlayer(PlayerObject* player, GameObject* object) {
        // Store death position BEFORE calling parent
        float deathX = m_player1 ? m_player1->getPositionX() : 0;
        m_fields->m_deathX = deathX;
        
        // Calculate percentage at death
        float percentage = (m_fields->m_bestX / 30302.0f) * 100.0f;
        if (percentage > 100) percentage = 100;
        
        // Log death
        if (m_fields->m_didProgress && m_fields->m_bestX > 0) {
            log::info("Best Ghost: DIED at {:.1f}% (X: {:.0f})", percentage, m_fields->m_bestX);
        } else {
            log::info("Best Ghost: DIED at start (no progress)");
        }
        
        PlayLayer::destroyPlayer(player, object);
    }
    
    void levelComplete() {
        // Calculate final percentage
        float finalX = m_fields->m_bestX;
        float percentage = (finalX / 30302.0f) * 100.0f;
        
        log::info("Best Ghost: LEVEL COMPLETED!");
        log::info("Best Ghost: Final X: {:.0f} / 100%", finalX);
        
        PlayLayer::levelComplete();
    }
};
