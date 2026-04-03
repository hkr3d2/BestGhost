#include <Geode/Geode.hpp>
#include "GhostManager.hpp"
#include "GhostRenderer.hpp"
#include "hooks/PlayLayerHook.hpp"
#include "hooks/LevelInfoLayerHook.hpp"
#include "hooks/PauseLayerHook.hpp"

using namespace geode::prelude;

$on_mod(Loaded) {
    log::info("Personal Best Ghost Mod v1.0.0 loaded!");
    
    // Initialize settings
    Mod::get()->registerCustomSetting<int64_t>("ghost-opacity", Setting::create("Ghost Opacity", 70));
    Mod::get()->registerCustomSetting<bool>("show-click-indicators", Setting::create("Show Click Indicators", false));
    Mod::get()->registerCustomSetting<bool>("show-overlay", Setting::create("Show Comparison Overlay", false));
}

$on_mod(Unloaded) {
    log::info("Personal Best Ghost Mod unloaded");
}
