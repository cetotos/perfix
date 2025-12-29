/**
 * Perfix - Performance Fixes and Profiler for Geometry Dash
 *
 * Critical performance optimizations targeting:
 * - ShaderLayer (chromatic, sepia, glitch, blur, pixelate, etc.)
 * - Particle Systems (fire, dust, explosion effects)
 * - Glow Effects (additive blending sprites)
 * - Pulse Effects (color pulsing triggers)
 * - Ghost Trails (player trail snapshots)
 * - High Detail Objects
 * - Camera Shake
 * - Move/Rotate effect calculations
 */

#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/ShaderLayer.hpp>
#include <Geode/modify/GhostTrailEffect.hpp>
#include <Geode/modify/CCParticleSystem.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/GJEffectManager.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <chrono>

using namespace geode::prelude;

// ============================================================================
// PROFILER STATE
// ============================================================================

struct Prof {
    bool enabled = false;
    double wallFrameTotal = 0.0;
    double wallFrameMax = 0.0;
    int wallFrameCount = 0;
    double simFrameTotal = 0.0;
    double simFrameMax = 0.0;
    int simFrameCount = 0;
    std::chrono::steady_clock::time_point lastFrameTs{};
    bool hasLastFrameTs = false;

    double shaderVisitMs = 0.0;
    double updateMs = 0.0;
    double particleMs = 0.0;
    double effectMs = 0.0;

    int particlesSkipped = 0;
    int glowsDisabled = 0;
    int highDetailSkipped = 0;
};

static Prof g_prof;

// Cached setting values to avoid repeated lookups
static struct {
    bool showProfiler = true;
    bool disableShaders = false;
    bool disableTrails = false;
    bool disableParticles = false;
    bool disableGlow = false;
    bool disablePulse = false;
    bool disableShake = false;
    bool disableHighDetail = false;
    bool disableMoveEffects = false;
    bool reducedParticles = false;
    bool cacheValid = false;
} g_settings;

static void refreshSettings() {
    auto* mod = Mod::get();
    g_settings.showProfiler = mod->getSettingValue<bool>("show-profiler");
    g_settings.disableShaders = mod->getSettingValue<bool>("disable-shaders");
    g_settings.disableTrails = mod->getSettingValue<bool>("disable-trails");
    g_settings.disableParticles = mod->getSettingValue<bool>("disable-particles");
    g_settings.disableGlow = mod->getSettingValue<bool>("disable-glow");
    g_settings.disablePulse = mod->getSettingValue<bool>("disable-pulse");
    g_settings.disableShake = mod->getSettingValue<bool>("disable-shake");
    g_settings.disableHighDetail = mod->getSettingValue<bool>("disable-high-detail");
    g_settings.disableMoveEffects = mod->getSettingValue<bool>("disable-move-effects");
    g_settings.reducedParticles = mod->getSettingValue<bool>("reduced-particles");
    g_settings.cacheValid = true;
}

static void profilerSimFrame(float dt) {
    double ms = dt * 1000.0;
    g_prof.simFrameTotal += ms;
    g_prof.simFrameCount++;
    if (ms > g_prof.simFrameMax) g_prof.simFrameMax = ms;
}

static void profilerWallFrame() {
    auto now = std::chrono::steady_clock::now();
    if (g_prof.hasLastFrameTs) {
        double ms = std::chrono::duration<double, std::milli>(now - g_prof.lastFrameTs).count();
        g_prof.wallFrameTotal += ms;
        g_prof.wallFrameCount++;
        if (ms > g_prof.wallFrameMax) g_prof.wallFrameMax = ms;
    }
    g_prof.lastFrameTs = now;
    g_prof.hasLastFrameTs = true;
}

// ============================================================================
// GJBASEGAMELAYER HOOKS - Core game layer optimizations
// ============================================================================

class $modify(PerfixBaseGameLayer, GJBaseGameLayer) {
    struct Fields {
        float profilerAccum = 0.0f;
        CCLabelBMFont* profilerLabel = nullptr;
        float settingsRefreshAccum = 0.0f;
    };

    void update(float dt) {
        // Refresh settings every 0.25 seconds instead of every frame
        m_fields->settingsRefreshAccum += dt;
        if (m_fields->settingsRefreshAccum >= 0.25f || !g_settings.cacheValid) {
            refreshSettings();
            m_fields->settingsRefreshAccum = 0.0f;
        }

        g_prof.enabled = g_settings.showProfiler;

        if (g_prof.enabled) {
            auto* director = CCDirector::sharedDirector();
            profilerSimFrame(director ? director->getDeltaTime() : dt);
            profilerWallFrame();
        }

        auto start = std::chrono::steady_clock::now();
        GJBaseGameLayer::update(dt);
        auto end = std::chrono::steady_clock::now();
        g_prof.updateMs = std::chrono::duration<double, std::milli>(end - start).count();

        if (!g_prof.enabled) return;

        m_fields->profilerAccum += dt;
        if (m_fields->profilerAccum < 0.5f) return;
        m_fields->profilerAccum = 0.0f;

        // Create label if needed
        if (!m_fields->profilerLabel) {
            auto* label = CCLabelBMFont::create("", "bigFont.fnt");
            label->setAnchorPoint({0.0f, 1.0f});
            label->setScale(0.25f);
            auto win = CCDirector::sharedDirector()->getWinSize();
            label->setPosition({4.0f, win.height - 4.0f});
            label->setZOrder(9999);
            label->setOpacity(220);
            this->addChild(label);
            m_fields->profilerLabel = label;
        }

        // Calculate stats
        double avgWall = g_prof.wallFrameCount > 0 ? (g_prof.wallFrameTotal / g_prof.wallFrameCount) : 0.0;
        double avgSim = g_prof.simFrameCount > 0 ? (g_prof.simFrameTotal / g_prof.simFrameCount) : 0.0;
        double fpsWall = avgWall > 0.0 ? (1000.0 / avgWall) : 0.0;
        double fpsSim = avgSim > 0.0 ? (1000.0 / avgSim) : 0.0;

        int totalObjs = m_objects ? m_objects->count() : 0;

        // Build string with optimization stats
        std::string text = fmt::format(
            "FPS: {:.0f} (sim {:.0f})\n"
            "Frame: {:.1f}ms (max {:.1f})\n"
            "Objects: {} | vis: {}/{}\n"
            "Update: {:.1f}ms | Shader: {:.1f}ms\n"
            "Particles: {:.1f}ms | Effects: {:.1f}ms\n"
            "Gradients: {} | Shader: {}\n"
            "-- Optimizations --\n"
            "Particles skipped: {}\n"
            "Glows disabled: {}\n"
            "HighDetail skipped: {}",
            fpsWall, fpsSim,
            avgWall, g_prof.wallFrameMax,
            totalObjs, m_visibleObjectsCount, m_visibleObjects2Count,
            g_prof.updateMs, g_prof.shaderVisitMs,
            g_prof.particleMs, g_prof.effectMs,
            m_activeGradients, m_shaderLayer ? "yes" : "no",
            g_prof.particlesSkipped,
            g_prof.glowsDisabled,
            g_prof.highDetailSkipped
        );

        m_fields->profilerLabel->setString(text.c_str());
        m_fields->profilerLabel->setVisible(true);

        // Reset stats
        g_prof.wallFrameTotal = 0.0;
        g_prof.wallFrameMax = 0.0;
        g_prof.wallFrameCount = 0;
        g_prof.simFrameTotal = 0.0;
        g_prof.simFrameMax = 0.0;
        g_prof.simFrameCount = 0;
        g_prof.particlesSkipped = 0;
        g_prof.glowsDisabled = 0;
        g_prof.highDetailSkipped = 0;
    }

    // Optimize shader layer updates - skip when disabled
    void updateShaderLayer(float dt) {
        if (g_settings.disableShaders) {
            if (m_shaderLayer) {
                m_shaderLayer->setVisible(false);
            }
            return;
        }
        if (m_shaderLayer) {
            m_shaderLayer->setVisible(true);
        }
        GJBaseGameLayer::updateShaderLayer(dt);
    }
};

// ============================================================================
// PLAYLAYER HOOKS - Gameplay layer optimizations
// ============================================================================

class $modify(PerfixPlayLayer, PlayLayer) {
    // Override camera shake to disable when setting is on
    void shakeCamera(float duration, float strength, float interval) {
        if (g_settings.disableShake) {
            return; // Skip camera shake entirely
        }
        PlayLayer::shakeCamera(duration, strength, interval);
    }

    // Optimize gravity effect visibility updates
    void updateVisibility(float dt) {
        if (g_settings.disableParticles) {
            m_disableGravityEffect = true;
        }
        PlayLayer::updateVisibility(dt);
    }
};

// ============================================================================
// SHADERLAYER HOOKS - Visual effect shader optimizations
// ============================================================================

class $modify(PerfixShaderLayer, ShaderLayer) {
    // Skip shader rendering entirely when disabled
    void visit() {
        if (g_settings.disableShaders) {
            g_prof.shaderVisitMs = 0.0;
            // Just render children without shader effects
            CCNode::visit();
            return;
        }

        auto start = std::chrono::steady_clock::now();
        ShaderLayer::visit();
        auto end = std::chrono::steady_clock::now();
        g_prof.shaderVisitMs = std::chrono::duration<double, std::milli>(end - start).count();
    }

    // Skip expensive shader calculations when disabled
    void performCalculations() {
        if (g_settings.disableShaders) return;
        ShaderLayer::performCalculations();
    }

    // Skip shader setup operations
    void setupShader(bool p0) {
        if (g_settings.disableShaders) return;
        ShaderLayer::setupShader(p0);
    }
};

// ============================================================================
// GHOST TRAIL HOOKS - Player trail effect optimizations
// ============================================================================

class $modify(PerfixGhostTrailEffect, GhostTrailEffect) {
    void trailSnapshot(float dt) {
        if (g_settings.disableTrails) return;
        GhostTrailEffect::trailSnapshot(dt);
    }
};

// ============================================================================
// PARTICLE SYSTEM HOOKS - Particle effect optimizations
// ============================================================================

class $modify(PerfixCCParticleSystem, cocos2d::CCParticleSystem) {
    void update(float dt) {
        // Skip all particle updates when disabled
        if (g_settings.disableParticles) {
            g_prof.particlesSkipped++;
            this->setVisible(false);
            return;
        }

        // Reduced particles mode - only update every other frame
        if (g_settings.reducedParticles) {
            static int frameCounter = 0;
            frameCounter++;
            if (frameCounter % 2 == 0) {
                g_prof.particlesSkipped++;
                return;
            }
        }

        auto start = std::chrono::steady_clock::now();
        CCParticleSystem::update(dt);
        auto end = std::chrono::steady_clock::now();
        g_prof.particleMs += std::chrono::duration<double, std::milli>(end - start).count();
    }

    // Skip particle emission when disabled
    bool addParticle() {
        if (g_settings.disableParticles) return false;
        return CCParticleSystem::addParticle();
    }
};

// ============================================================================
// GAMEOBJECT HOOKS - Per-object optimizations
// ============================================================================

class $modify(PerfixGameObject, GameObject) {
    // Optimize glow sprite visibility
    void setGlowColor(cocos2d::ccColor3B const& color) {
        if (g_settings.disableGlow) {
            if (m_glowSprite) {
                m_glowSprite->setVisible(false);
                g_prof.glowsDisabled++;
            }
            return;
        }
        GameObject::setGlowColor(color);
    }

    // Skip high detail objects when setting enabled
    void activateObject() {
        if (g_settings.disableHighDetail && m_isHighDetail) {
            g_prof.highDetailSkipped++;
            return;
        }
        GameObject::activateObject();
    }
};

// ============================================================================
// GJEFFECTMANAGER HOOKS - Effect trigger optimizations
// ============================================================================

class $modify(PerfixGJEffectManager, GJEffectManager) {
    // Skip pulse effect processing when disabled
    void updatePulseEffects(float dt) {
        if (g_settings.disablePulse) {
            return;
        }

        auto start = std::chrono::steady_clock::now();
        GJEffectManager::updatePulseEffects(dt);
        auto end = std::chrono::steady_clock::now();
        g_prof.effectMs += std::chrono::duration<double, std::milli>(end - start).count();
    }

    // Skip opacity effect updates when move effects disabled
    void updateOpacityEffects(float dt) {
        if (g_settings.disableMoveEffects) {
            return;
        }
        GJEffectManager::updateOpacityEffects(dt);
    }
};

// ============================================================================
// EFFECTGAMEOBJECT HOOKS - Effect trigger optimizations
// ============================================================================

class $modify(PerfixEffectGameObject, EffectGameObject) {
    // Optimize trigger activation for shake triggers
    void triggerActivated(float xPos) {
        // Skip shake triggers when shake is disabled (object ID 1520 = Shake Trigger)
        if (g_settings.disableShake && m_objectID == 1520) {
            return;
        }

        // Skip pulse triggers when pulse is disabled (object ID 1006 = Pulse Trigger)
        if (g_settings.disablePulse && m_objectID == 1006) {
            return;
        }

        EffectGameObject::triggerActivated(xPos);
    }
};

// ============================================================================
// PLAYEROBJECT HOOKS - Player-specific optimizations
// ============================================================================

class $modify(PerfixPlayerObject, PlayerObject) {
    // Optimize player glow effects
    void updateGlowColor() {
        if (g_settings.disableGlow) {
            return;
        }
        PlayerObject::updateGlowColor();
    }
};

// ============================================================================
// INITIALIZATION
// ============================================================================

$on_mod(Loaded) {
    // Pre-cache settings on load
    refreshSettings();

    log::info("Perfix loaded - Performance fixes active");
}
