/**
 * Perfix v2.2 - ULTRA Performance Profiler & Optimizer for Geometry Dash
 *
 * COMPREHENSIVE PROFILING OF:
 * - Frame timing, visibility culling, collision detection
 * - Particle systems, shader layer, effect manager
 * - Action processing (move, rotate, follow, area effects)
 * - Object counts, batch nodes, draw calls
 *
 * OPTIMIZATION OPTIONS:
 * - Disable shaders, particles, glow, trails, pulse, shake
 * - Throttle actions, transforms, spawns, gradients
 * - Skip area/follow effects, reduce collisions
 */

#include "globals.hpp"

// ============================================================================
// GLOBAL DEFINITIONS
// ============================================================================

UltraProfiler g_prof;
SettingsCache g_settings;
ThrottleState g_throttle;

// ============================================================================
// SETTINGS REFRESH
// ============================================================================

void refreshSettings() {
    auto* mod = Mod::get();
    g_settings.showProfiler = mod->getSettingValue<bool>("show-profiler");
    g_settings.showDetailedProfiler = mod->getSettingValue<bool>("show-detailed-profiler");
    g_settings.disableShaders = mod->getSettingValue<bool>("disable-shaders");
    g_settings.disableTrails = mod->getSettingValue<bool>("disable-trails");
    g_settings.disableParticles = mod->getSettingValue<bool>("disable-particles");
    g_settings.disableGlow = mod->getSettingValue<bool>("disable-glow");
    g_settings.disablePulse = mod->getSettingValue<bool>("disable-pulse");
    g_settings.disableShake = mod->getSettingValue<bool>("disable-shake");
    g_settings.disableHighDetail = mod->getSettingValue<bool>("disable-high-detail");
    g_settings.disableMoveEffects = mod->getSettingValue<bool>("disable-move-effects");
    g_settings.reducedParticles = mod->getSettingValue<bool>("reduced-particles");
    // Experimental fixes
    g_settings.expThrottleActions = mod->getSettingValue<bool>("exp-throttle-actions");
    g_settings.expSkipAreaEffects = mod->getSettingValue<bool>("exp-skip-area-effects");
    g_settings.expThrottleTransforms = mod->getSettingValue<bool>("exp-throttle-transforms");
    g_settings.expThrottleSpawns = mod->getSettingValue<bool>("exp-throttle-spawns");
    g_settings.expReduceCollisions = mod->getSettingValue<bool>("exp-reduce-collision-checks");
    g_settings.expAggressiveCulling = mod->getSettingValue<bool>("exp-aggressive-culling");
    g_settings.expSkipFollowActions = mod->getSettingValue<bool>("exp-skip-follow-actions");
    g_settings.expReduceColorUpdates = mod->getSettingValue<bool>("exp-reduce-color-updates");
    // NEW experimental fixes
    g_settings.expThrottleGradients = mod->getSettingValue<bool>("exp-throttle-gradients");
    g_settings.expReduceWaveTrail = mod->getSettingValue<bool>("exp-reduce-wave-trail");
    g_settings.expThrottleAdvancedFollow = mod->getSettingValue<bool>("exp-throttle-advanced-follow");
    g_settings.expThrottleDynamicObjects = mod->getSettingValue<bool>("exp-throttle-dynamic-objects");
    g_settings.expThrottlePlayerFollow = mod->getSettingValue<bool>("exp-throttle-player-follow");
    g_settings.expLimitEnterEffects = mod->getSettingValue<bool>("exp-limit-enter-effects");
    g_settings.expThrottleLabels = mod->getSettingValue<bool>("exp-throttle-labels");
    g_settings.cacheValid = true;
}

// ============================================================================
// INITIALIZATION
// ============================================================================

$on_mod(Loaded) {
    refreshSettings();
}
