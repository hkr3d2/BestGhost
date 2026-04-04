#include <Geode/Geode.hpp>
#include "GhostManager.hpp"
#include "GhostRenderer.hpp"
#include "hooks/PlayLayerHook.hpp"
#include "hooks/LevelInfoLayerHook.hpp"
#include "hooks/PauseLayerHook.hpp"

using namespace geode::prelude;

$on_mod(Loaded) {
    log::info("Best Ghost Mod v1.0.0 loaded!");
}

$on_mod(Unloaded) {
    log::info("Best Ghost Mod unloaded");
}
