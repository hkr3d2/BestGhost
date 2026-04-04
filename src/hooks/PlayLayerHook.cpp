#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

class $modify(PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        
        log::info("Best Ghost: Level loaded - {}", level->m_levelName);
        
        return true;
    }
    
    void destroyPlayer(PlayerObject* player, GameObject* object) {
        log::info("Best Ghost: Player died!");
        PlayLayer::destroyPlayer(player, object);
    }
    
    void levelComplete() {
        log::info("Best Ghost: Level completed!");
        PlayLayer::levelComplete();
    }
};
