#include <Geode/Geode.hpp>
#include "GhostManager.hpp"
#include "GhostRenderer.hpp"
#include "hooks/PlayLayerHook.hpp"
#include "hooks/LevelInfoLayerHook.hpp"
#include "hooks/PauseLayerHook.hpp"

using namespace geode::prelude;

$on_mod(Loaded) {
    log::info("Best Ghost Mod v1.0.0 loaded!");
    
    // Register settings
    auto mod = Mod::get();
    mod->addSetting<bool>("show-click-indicators", "Show Click Indicators", false);
    mod->addSetting<bool>("show-overlay", "Show Comparison Overlay", false);
}

$on_mod(Unloaded) {
    log::info("Best Ghost Mod unloaded");
}
