#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

class $modify(PlayLayer) {
    struct Fields {
        float m_bestPercentage = 0;
        float m_startX = 0;
        float m_startY = 0;
    };
    
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        
        // Store starting position
        if (m_player1) {
            m_fields->m_startX = m_player1->getPositionX();
            m_fields->m_startY = m_player1->getPositionY();
        }
        
        log::info("Best Ghost: Recording started for level: {}", level->m_levelName);
        
        return true;
    }
    
    void update(float dt) {
        PlayLayer::update(dt);
        
        if (!m_player1) return;
        
        // Get current percentage from the game
        float currentPercent = this->m_gameState.m_percentage;
        
        // Check if this is a new best
        if (currentPercent > m_fields->m_bestPercentage) {
            m_fields->m_bestPercentage = currentPercent;
            log::info("Best Ghost: New best! {}%", currentPercent);
            Notification::create(fmt::format("New best: {}%", currentPercent), NotificationIcon::Success)->show();
        }
    }
    
    void destroyPlayer(PlayerObject* player, GameObject* object) {
        float finalPercent = this->m_gameState.m_percentage;
        
        if (finalPercent > m_fields->m_bestPercentage) {
            log::info("Best Ghost: Achieved {}% - would save here!", finalPercent);
        }
        
        PlayLayer::destroyPlayer(player, object);
    }
    
    void levelComplete() {
        log::info("Best Ghost: Level completed! Would save 100% ghost here!");
        Notification::create("Level completed!", NotificationIcon::Success)->show();
        PlayLayer::levelComplete();
    }
};
