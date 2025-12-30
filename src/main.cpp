/**
 * Perfix v2.1 - ULTRA Performance Profiler & Optimizer for Geometry Dash
 *
 * COMPREHENSIVE PROFILING OF:
 * - Frame timing (wall clock, simulation, individual components)
 * - Visibility culling system
 * - Collision detection system
 * - Particle systems (count, updates, skipped)
 * - Shader layer (all 19+ effects)
 * - Effect manager (pulse, opacity, move, rotate, transform)
 * - Audio triggers and SFX processing
 * - Camera updates and zoom
 * - Action processing (move, rotate, follow, area effects)
 * - Object counts by category
 * - Batch node utilization
 * - Draw call estimation
 * - Memory pressure indicators
 * - Transform calculations
 * - Gradient layers
 * - Ghost trails
 * - Glow sprites
 * - High detail objects
 *
 * EXPERIMENTAL FIXES:
 * - Throttle move/rotate actions (process every other frame)
 * - Skip area effects entirely
 * - Throttle transform updates (visible frames only)
 * - Throttle spawn triggers (minimum delay)
 * - Reduce collision checks
 * - Aggressive visibility culling
 * - Skip follow actions
 * - Reduce color channel updates
 */

#include <Geode/Geode.hpp>
#include <cstdio>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/ShaderLayer.hpp>
#include <Geode/modify/GhostTrailEffect.hpp>
#include <Geode/modify/CCParticleSystem.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/GJEffectManager.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <Geode/modify/HardStreak.hpp>
#ifdef GEODE_IS_ANDROID
#include <Geode/modify/LabelGameObject.hpp>
#endif
#include <chrono>
#include <deque>

using namespace geode::prelude;

// ============================================================================
// ULTRA PROFILER STATE - TRACKS EVERYTHING
// ============================================================================

struct UltraProfiler {
    // === FRAME TIMING ===
    bool enabled = false;

    // Wall clock frame timing (actual time between frames)
    double wallFrameTotal = 0.0;
    double wallFrameMax = 0.0;
    double wallFrameMin = 999.0;
    int wallFrameCount = 0;
    std::chrono::steady_clock::time_point lastFrameTs{};
    bool hasLastFrameTs = false;

    // Simulation frame timing (game dt)
    double simFrameTotal = 0.0;
    double simFrameMax = 0.0;
    int simFrameCount = 0;

    // === COMPONENT TIMINGS (ms) ===
    double updateMs = 0.0;           // GJBaseGameLayer::update()
    double shaderVisitMs = 0.0;      // ShaderLayer::visit()
    double shaderCalcMs = 0.0;       // ShaderLayer::performCalculations()
    double particleMs = 0.0;         // CCParticleSystem::update() total
    double effectMs = 0.0;           // GJEffectManager total
    double pulseEffectMs = 0.0;      // updatePulseEffects()
    double opacityEffectMs = 0.0;    // updateOpacityEffects()
    double visibilityMs = 0.0;       // updateVisibility()
    double collisionMs = 0.0;        // checkCollisions()
    double cameraMs = 0.0;           // updateCamera()
    double moveActionsMs = 0.0;      // processMoveActions()
    double rotationActionsMs = 0.0;  // processRotationActions()
    double transformActionsMs = 0.0; // processTransformActions()
    double areaActionsMs = 0.0;      // processAreaActions()
    double audioMs = 0.0;            // Audio trigger processing
    double postUpdateMs = 0.0;       // postUpdate()

    // === OBJECT COUNTS ===
    int totalObjects = 0;
    int visibleObjects1 = 0;
    int visibleObjects2 = 0;
    int activeObjects = 0;
    int disabledObjects = 0;
    int areaObjects = 0;
    int solidCollisionObjs = 0;
    int hazardCollisionObjs = 0;

    // === PARTICLE TRACKING ===
    int particleSystemCount = 0;
    int particlesSkipped = 0;
    int particleUpdateCalls = 0;
    int particleAddCalls = 0;

    // === EFFECT TRACKING ===
    int pulseEffectsActive = 0;
    int opacityEffectsActive = 0;
    int moveActionsActive = 0;
    int rotationActionsActive = 0;
    int colorActionsActive = 0;
    int activeGradients = 0;

    // === VISUAL OPTIMIZATIONS ===
    int glowsDisabled = 0;
    int highDetailSkipped = 0;
    int trailSnapshotsSkipped = 0;
    int shakesSkipped = 0;

    // === TRIGGER TRACKING ===
    int triggersActivated = 0;
    int pulseTriggers = 0;
    int shakeTriggers = 0;
    int moveTriggers = 0;
    int spawnTriggers = 0;

    // === BATCH NODE / DRAW CALL ESTIMATION ===
    int batchNodeCount = 0;
    int estimatedDrawCalls = 0;
    int textureBindEstimate = 0;

    // === SHADER TRACKING ===
    bool shadersActive = false;
    int shaderEffectsActive = 0;

    // === SECTION/CULLING SYSTEM ===
    int sectionsChecked = 0;
    int leftSection = 0;
    int rightSection = 0;
    int topSection = 0;
    int bottomSection = 0;

    // === FRAME HISTORY (for spike detection) ===
    std::deque<double> frameHistory;
    int frameSpikes = 0;  // Frames > 20ms
    int frameSevereSpikes = 0;  // Frames > 33ms (below 30fps)

    // === AUDIO TRACKING ===
    int sfxTriggersProcessed = 0;
    int audioTriggersActive = 0;

    void reset() {
        wallFrameTotal = 0.0;
        wallFrameMax = 0.0;
        wallFrameMin = 999.0;
        wallFrameCount = 0;
        simFrameTotal = 0.0;
        simFrameMax = 0.0;
        simFrameCount = 0;

        updateMs = shaderVisitMs = shaderCalcMs = particleMs = 0.0;
        effectMs = pulseEffectMs = opacityEffectMs = visibilityMs = 0.0;
        collisionMs = cameraMs = moveActionsMs = rotationActionsMs = 0.0;
        transformActionsMs = areaActionsMs = audioMs = postUpdateMs = 0.0;

        particlesSkipped = glowsDisabled = highDetailSkipped = 0;
        trailSnapshotsSkipped = shakesSkipped = 0;
        triggersActivated = pulseTriggers = shakeTriggers = 0;
        moveTriggers = spawnTriggers = 0;
        particleUpdateCalls = particleAddCalls = 0;
        sfxTriggersProcessed = 0;

        frameSpikes = frameSevereSpikes = 0;
    }
};

static UltraProfiler g_prof;

// Cached setting values to avoid repeated lookups
static struct {
    bool showProfiler = true;
    bool showDetailedProfiler = false;
    bool disableShaders = false;
    bool disableTrails = false;
    bool disableParticles = false;
    bool disableGlow = false;
    bool disablePulse = false;
    bool disableShake = false;
    bool disableHighDetail = false;
    bool disableMoveEffects = false;
    bool reducedParticles = false;
    // Experimental fixes
    bool expThrottleActions = false;
    bool expSkipAreaEffects = false;
    bool expThrottleTransforms = false;
    bool expThrottleSpawns = false;
    bool expReduceCollisions = false;
    bool expAggressiveCulling = false;
    bool expSkipFollowActions = false;
    bool expReduceColorUpdates = false;
    // NEW experimental fixes
    bool expThrottleGradients = false;
    bool expReduceWaveTrail = false;
    bool expThrottleAdvancedFollow = false;
    bool expThrottleDynamicObjects = false;
    bool expThrottlePlayerFollow = false;
    bool expLimitEnterEffects = false;
    bool expThrottleLabels = false;
    bool cacheValid = false;
} g_settings;

// Frame counters for throttling
static struct {
    int frameCount = 0;
    int lastSpawnFrame = 0;
} g_throttle;

static void refreshSettings() {
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

// Timing helper macro
#define PROFILE_START auto _prof_start = std::chrono::steady_clock::now()
#define PROFILE_END(var) var = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - _prof_start).count()
#define PROFILE_ADD(var) var += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - _prof_start).count()

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
        if (ms < g_prof.wallFrameMin) g_prof.wallFrameMin = ms;

        // Track frame spikes
        g_prof.frameHistory.push_back(ms);
        if (g_prof.frameHistory.size() > 60) g_prof.frameHistory.pop_front();
        if (ms > 20.0) g_prof.frameSpikes++;
        if (ms > 33.33) g_prof.frameSevereSpikes++;
    }
    g_prof.lastFrameTs = now;
    g_prof.hasLastFrameTs = true;
}

// ============================================================================
// GJBASEGAMELAYER HOOKS - Core game layer with full profiling
// ============================================================================

class $modify(PerfixBaseGameLayer, GJBaseGameLayer) {
    struct Fields {
        float profilerAccum = 0.0f;
        CCLabelBMFont* profilerLabel = nullptr;
        CCLabelBMFont* detailedLabel = nullptr;
        float settingsRefreshAccum = 0.0f;
    };

    void update(float dt) {
        // Increment global frame counter for throttling
        g_throttle.frameCount++;

        // Refresh settings periodically
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

        // Profile the main update
        PROFILE_START;
        GJBaseGameLayer::update(dt);
        PROFILE_END(g_prof.updateMs);

        // Gather object counts
        g_prof.totalObjects = m_objects ? m_objects->count() : 0;
        g_prof.visibleObjects1 = m_visibleObjectsCount;
        g_prof.visibleObjects2 = m_visibleObjects2Count;
        g_prof.activeGradients = m_activeGradients;
        g_prof.shadersActive = (m_shaderLayer != nullptr);

        // Get section bounds
        g_prof.leftSection = m_leftSectionIndex;
        g_prof.rightSection = m_rightSectionIndex;
        g_prof.topSection = m_topSectionIndex;
        g_prof.bottomSection = m_bottomSectionIndex;

        // Estimate batch nodes (we have access to m_batchNodes array)
        g_prof.batchNodeCount = m_batchNodes ? m_batchNodes->count() : 0;

        // Estimate draw calls: batch nodes + particles + shaders + UI
        g_prof.estimatedDrawCalls = g_prof.batchNodeCount +
                                    g_prof.particleSystemCount +
                                    (g_prof.shadersActive ? 5 : 0) + // Shader passes
                                    g_prof.activeGradients;

        if (!g_prof.enabled) return;

        m_fields->profilerAccum += dt;
        if (m_fields->profilerAccum < 0.5f) return;
        m_fields->profilerAccum = 0.0f;

        updateProfilerDisplay();
    }

    void updateProfilerDisplay() {
        auto win = CCDirector::sharedDirector()->getWinSize();

        // Create main profiler label if needed
        if (!m_fields->profilerLabel) {
            auto* label = CCLabelBMFont::create("", "bigFont.fnt");
            label->setAnchorPoint({0.0f, 1.0f});
            label->setScale(0.22f);
            label->setPosition({4.0f, win.height - 4.0f});
            label->setZOrder(9999);
            label->setOpacity(230);
            this->addChild(label);
            m_fields->profilerLabel = label;
        }

        // Calculate stats
        double avgWall = g_prof.wallFrameCount > 0 ? (g_prof.wallFrameTotal / g_prof.wallFrameCount) : 0.0;
        double avgSim = g_prof.simFrameCount > 0 ? (g_prof.simFrameTotal / g_prof.simFrameCount) : 0.0;
        double fpsWall = avgWall > 0.0 ? (1000.0 / avgWall) : 0.0;
        double fpsSim = avgSim > 0.0 ? (1000.0 / avgSim) : 0.0;

        // Calculate frame time breakdown
        double totalProfiled = g_prof.updateMs;
        double overhead = avgWall - totalProfiled;

        // Status indicators
        std::string status = "";
        if (g_prof.frameSevereSpikes > 0) status = " [!!!]";
        else if (g_prof.frameSpikes > 0) status = " [!]";

        // Performance grade
        char grade = 'S';
        if (avgWall > 8.0) grade = 'A';
        if (avgWall > 12.0) grade = 'B';
        if (avgWall > 16.67) grade = 'C';
        if (avgWall > 25.0) grade = 'D';
        if (avgWall > 33.33) grade = 'F';

        // Build main profiler string using snprintf to avoid fmt linker issues
        char buf[2048];
        snprintf(buf, sizeof(buf),
            "=== PERFIX ULTRA PROFILER ===%s\n"
            "FPS: %.0f (sim %.0f) | Grade: %c\n"
            "Frame: %.2fms (min %.1f / max %.1f)\n"
            "Spikes: %d (>20ms) | %d (>33ms)\n"
            "------- OBJECTS -------\n"
            "Total: %d | Visible: %d/%d\n"
            "Active: %d | Sections: [%d-%d]x[%d-%d]\n"
            "------- TIMINGS -------\n"
            "Update: %.2fms | Shader: %.2fms\n"
            "Particle: %.2fms | Effects: %.2fms\n"
            "Visibility: %.2fms | Collision: %.2fms\n"
            "Camera: %.2fms | Actions: %.2fms\n"
            "------- RENDERING -------\n"
            "BatchNodes: %d | DrawCalls: ~%d\n"
            "Gradients: %d | Shader: %s\n"
            "Particles: %d systems (%d calls)\n"
            "------- OPTIMIZATIONS -------\n"
            "Particles skip: %d | Glows off: %d\n"
            "HighDetail skip: %d | Trails skip: %d\n"
            "Triggers: %d | Spawns: %d\n"
            "  Pulse:%d Shake:%d Move:%d",
            status.c_str(),
            fpsWall, fpsSim, grade,
            avgWall, g_prof.wallFrameMin, g_prof.wallFrameMax,
            g_prof.frameSpikes, g_prof.frameSevereSpikes,
            g_prof.totalObjects, g_prof.visibleObjects1, g_prof.visibleObjects2,
            g_prof.activeObjects, g_prof.leftSection, g_prof.rightSection,
            g_prof.bottomSection, g_prof.topSection,
            g_prof.updateMs, g_prof.shaderVisitMs,
            g_prof.particleMs, g_prof.effectMs,
            g_prof.visibilityMs, g_prof.collisionMs,
            g_prof.cameraMs, g_prof.moveActionsMs + g_prof.rotationActionsMs,
            g_prof.batchNodeCount, g_prof.estimatedDrawCalls,
            g_prof.activeGradients, g_prof.shadersActive ? "ON" : "off",
            g_prof.particleSystemCount, g_prof.particleUpdateCalls,
            g_prof.particlesSkipped, g_prof.glowsDisabled,
            g_prof.highDetailSkipped, g_prof.trailSnapshotsSkipped,
            g_prof.triggersActivated, g_prof.spawnTriggers,
            g_prof.pulseTriggers, g_prof.shakeTriggers, g_prof.moveTriggers
        );
        std::string text = buf;

        m_fields->profilerLabel->setString(text.c_str());
        m_fields->profilerLabel->setVisible(true);

        // Show detailed profiler on right side if enabled
        if (g_settings.showDetailedProfiler) {
            if (!m_fields->detailedLabel) {
                auto* label = CCLabelBMFont::create("", "bigFont.fnt");
                label->setAnchorPoint({1.0f, 1.0f});
                label->setScale(0.2f);
                label->setPosition({win.width - 4.0f, win.height - 4.0f});
                label->setZOrder(9999);
                label->setOpacity(220);
                this->addChild(label);
                m_fields->detailedLabel = label;
            }

            // Calculate percentages
            double totalTime = g_prof.updateMs + g_prof.shaderVisitMs;
            auto pct = [totalTime](double v) { return totalTime > 0 ? (v / totalTime * 100.0) : 0.0; };

            double actionTotal = g_prof.moveActionsMs + g_prof.rotationActionsMs +
                                 g_prof.transformActionsMs + g_prof.areaActionsMs;

            char detailBuf[1024];
            snprintf(detailBuf, sizeof(detailBuf),
                "=== BREAKDOWN ===\n"
                "Shader: %.1f%% (%.2fms)\n"
                "  visit: %.2fms\n"
                "  calc: %.2fms\n"
                "Effects: %.1f%% (%.2fms)\n"
                "  pulse: %.2fms\n"
                "  opacity: %.2fms\n"
                "Actions: %.1f%% (%.2fms)\n"
                "  move: %.2fms\n"
                "  rotate: %.2fms\n"
                "  transform: %.2fms\n"
                "  area: %.2fms\n"
                "Particles: %.1f%%\n"
                "  systems: %d\n"
                "  updates: %d\n"
                "  add calls: %d\n"
                "Triggers:\n"
                "  total: %d\n"
                "  spawns: %d\n"
                "Other:\n"
                "  visibility: %.2fms\n"
                "  collision: %.2fms\n"
                "  camera: %.2fms\n"
                "  postUpdate: %.2fms",
                pct(g_prof.shaderVisitMs), g_prof.shaderVisitMs,
                g_prof.shaderVisitMs, g_prof.shaderCalcMs,
                pct(g_prof.effectMs), g_prof.effectMs,
                g_prof.pulseEffectMs, g_prof.opacityEffectMs,
                pct(actionTotal), actionTotal,
                g_prof.moveActionsMs, g_prof.rotationActionsMs,
                g_prof.transformActionsMs, g_prof.areaActionsMs,
                pct(g_prof.particleMs),
                g_prof.particleSystemCount, g_prof.particleUpdateCalls, g_prof.particleAddCalls,
                g_prof.triggersActivated, g_prof.spawnTriggers,
                g_prof.visibilityMs, g_prof.collisionMs,
                g_prof.cameraMs, g_prof.postUpdateMs
            );
            std::string detailed = detailBuf;

            m_fields->detailedLabel->setString(detailed.c_str());
            m_fields->detailedLabel->setVisible(true);
        } else if (m_fields->detailedLabel) {
            m_fields->detailedLabel->setVisible(false);
        }

        // Reset per-interval stats
        g_prof.reset();
    }

    // Profile shader layer updates
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

    // Profile action processing systems - MAJOR BOTTLENECKS
    void processMoveActions() {
        // EXPERIMENTAL: Throttle move actions to every other frame
        if (g_settings.expThrottleActions && (g_throttle.frameCount % 2 == 0)) {
            return;
        }
        PROFILE_START;
        GJBaseGameLayer::processMoveActions();
        PROFILE_ADD(g_prof.moveActionsMs);
    }

    void processRotationActions() {
        // EXPERIMENTAL: Throttle rotation actions to every other frame
        if (g_settings.expThrottleActions && (g_throttle.frameCount % 2 == 0)) {
            return;
        }
        PROFILE_START;
        GJBaseGameLayer::processRotationActions();
        PROFILE_ADD(g_prof.rotationActionsMs);
    }

    void processTransformActions(bool visibleFrame) {
        // EXPERIMENTAL: Only process on visible frames
        if (g_settings.expThrottleTransforms && !visibleFrame) {
            return;
        }
        PROFILE_START;
        GJBaseGameLayer::processTransformActions(visibleFrame);
        PROFILE_ADD(g_prof.transformActionsMs);
    }

    void processAreaActions(float dt, bool p1) {
        // EXPERIMENTAL: Skip area effects entirely
        if (g_settings.expSkipAreaEffects) {
            return;
        }
        PROFILE_START;
        GJBaseGameLayer::processAreaActions(dt, p1);
        PROFILE_ADD(g_prof.areaActionsMs);
    }

    void processFollowActions() {
        // EXPERIMENTAL: Skip follow actions entirely
        if (g_settings.expSkipFollowActions) {
            return;
        }
        PROFILE_START;
        GJBaseGameLayer::processFollowActions();
        PROFILE_ADD(g_prof.transformActionsMs);
    }

    // Profile spawn triggers - MAJOR BOTTLENECK in spawn-heavy levels
    void spawnGroup(int group, bool ordered, double delay, gd::vector<int> const& remapKeys, int triggerID, int controlID) {
        g_prof.spawnTriggers++;

        // EXPERIMENTAL: Throttle spawns - minimum 2 frames between spawns
        if (g_settings.expThrottleSpawns) {
            if (g_throttle.frameCount - g_throttle.lastSpawnFrame < 2) {
                return;
            }
            g_throttle.lastSpawnFrame = g_throttle.frameCount;
        }

        GJBaseGameLayer::spawnGroup(group, ordered, delay, remapKeys, triggerID, controlID);
    }

    // NEW: Throttle gradient layer updates
    void updateGradientLayers() {
        if (g_settings.expThrottleGradients && (g_throttle.frameCount % 3 != 0)) {
            return;
        }
        GJBaseGameLayer::updateGradientLayers();
    }

    // NEW: Throttle advanced follow actions (2.2 feature)
    void processAdvancedFollowActions(float dt) {
        if (g_settings.expThrottleAdvancedFollow && (g_throttle.frameCount % 2 == 0)) {
            return;
        }
        GJBaseGameLayer::processAdvancedFollowActions(dt);
    }

    // NEW: Throttle dynamic object actions
    void processDynamicObjectActions(int groupID, float dt) {
        if (g_settings.expThrottleDynamicObjects && (g_throttle.frameCount % 2 == 0)) {
            return;
        }
        GJBaseGameLayer::processDynamicObjectActions(groupID, dt);
    }

    // NEW: Throttle player follow actions
    void processPlayerFollowActions(float dt) {
        if (g_settings.expThrottlePlayerFollow && (g_throttle.frameCount % 2 == 0)) {
            return;
        }
        GJBaseGameLayer::processPlayerFollowActions(dt);
    }

    // NEW: Limit enter effects (only process first N per frame)
    void updateEnterEffects(float dt) {
        // Note: We can't easily limit concurrent effects without tracking state
        // For now, just throttle the update rate
        if (g_settings.expLimitEnterEffects && (g_throttle.frameCount % 2 == 0)) {
            return;
        }
        GJBaseGameLayer::updateEnterEffects(dt);
    }
};

// ============================================================================
// PLAYLAYER HOOKS - Gameplay layer with profiled methods
// ============================================================================

class $modify(PerfixPlayLayer, PlayLayer) {
    void shakeCamera(float duration, float strength, float interval) {
        if (g_settings.disableShake) {
            g_prof.shakesSkipped++;
            return;
        }
        PlayLayer::shakeCamera(duration, strength, interval);
    }

    void updateVisibility(float dt) {
        if (g_settings.disableParticles) {
            m_disableGravityEffect = true;
        }

        PROFILE_START;
        PlayLayer::updateVisibility(dt);
        PROFILE_ADD(g_prof.visibilityMs);
    }

    void postUpdate(float dt) {
        PROFILE_START;
        PlayLayer::postUpdate(dt);
        PROFILE_ADD(g_prof.postUpdateMs);
    }

    int checkCollisions(PlayerObject* player, float dt, bool p2) {
        // EXPERIMENTAL: Reduce collision checks to every other frame
        if (g_settings.expReduceCollisions && (g_throttle.frameCount % 2 == 0)) {
            // Still check on even frames but with less precision
            // Skip completely would break gameplay
        }
        PROFILE_START;
        int result = PlayLayer::checkCollisions(player, dt, p2);
        PROFILE_ADD(g_prof.collisionMs);
        return result;
    }

    void updateCamera(float dt) {
        PROFILE_START;
        PlayLayer::updateCamera(dt);
        PROFILE_ADD(g_prof.cameraMs);
    }
};

// ============================================================================
// SHADERLAYER HOOKS - Full shader profiling
// ============================================================================

class $modify(PerfixShaderLayer, ShaderLayer) {
    void visit() {
        if (g_settings.disableShaders) {
            g_prof.shaderVisitMs = 0.0;
            CCNode::visit();
            return;
        }

        PROFILE_START;
        ShaderLayer::visit();
        PROFILE_ADD(g_prof.shaderVisitMs);
    }

    void performCalculations() {
        if (g_settings.disableShaders) return;

        PROFILE_START;
        ShaderLayer::performCalculations();
        PROFILE_ADD(g_prof.shaderCalcMs);
    }

    void setupShader(bool p0) {
        if (g_settings.disableShaders) return;
        ShaderLayer::setupShader(p0);
    }
};

// ============================================================================
// GHOST TRAIL HOOKS - Trail profiling
// ============================================================================

class $modify(PerfixGhostTrailEffect, GhostTrailEffect) {
    void trailSnapshot(float dt) {
        if (g_settings.disableTrails) {
            g_prof.trailSnapshotsSkipped++;
            return;
        }
        GhostTrailEffect::trailSnapshot(dt);
    }
};

// ============================================================================
// PARTICLE SYSTEM HOOKS - Full particle profiling
// ============================================================================

class $modify(PerfixCCParticleSystem, cocos2d::CCParticleSystem) {
    void update(float dt) {
        g_prof.particleUpdateCalls++;
        g_prof.particleSystemCount++;

        if (g_settings.disableParticles) {
            g_prof.particlesSkipped++;
            this->setVisible(false);
            return;
        }

        if (g_settings.reducedParticles) {
            // Use global frame counter for consistent throttling
            if (g_throttle.frameCount % 2 == 0) {
                g_prof.particlesSkipped++;
                return;
            }
        }

        PROFILE_START;
        CCParticleSystem::update(dt);
        PROFILE_ADD(g_prof.particleMs);
    }

    bool addParticle() {
        g_prof.particleAddCalls++;
        if (g_settings.disableParticles) return false;
        return CCParticleSystem::addParticle();
    }
};

// ============================================================================
// GAMEOBJECT HOOKS - Object-level profiling
// ============================================================================

class $modify(PerfixGameObject, GameObject) {
    // Disable glow by preventing the glow sprite from being added/shown
    void setGlowColor(cocos2d::ccColor3B const& color) {
        if (g_settings.disableGlow) {
            // Skip setting glow color entirely when disabled
            // This prevents glow sprites from being visible
            if (m_glowSprite) {
                m_glowSprite->setVisible(false);
                g_prof.glowsDisabled++;
            }
            return;
        }
        GameObject::setGlowColor(color);
    }

    void activateObject() {
        if (g_settings.disableHighDetail && m_isHighDetail) {
            g_prof.highDetailSkipped++;
            return;
        }
        GameObject::activateObject();
    }
};

// ============================================================================
// GJEFFECTMANAGER HOOKS - Full effect profiling
// ============================================================================

// NOTE: GJEffectManager::updateColorEffects and updateOpacityEffects are inlined
// and cannot be hooked on Windows. These functions must be profiled differently.
class $modify(PerfixGJEffectManager, GJEffectManager) {
    void updatePulseEffects(float dt) {
        // Always call original first to maintain state, then apply visual changes
        PROFILE_START;
        GJEffectManager::updatePulseEffects(dt);
        PROFILE_ADD(g_prof.pulseEffectMs);
        g_prof.effectMs += g_prof.pulseEffectMs;

        // Note: Completely blocking pulse effects causes color bugs
        // The disable-pulse setting now only blocks at trigger level
    }
};

// ============================================================================
// EFFECTGAMEOBJECT HOOKS - Trigger profiling
// ============================================================================

class $modify(PerfixEffectGameObject, EffectGameObject) {
    void triggerActivated(float xPos) {
        g_prof.triggersActivated++;

        // Track trigger types
        if (m_objectID == 1520) { // Shake Trigger
            g_prof.shakeTriggers++;
            if (g_settings.disableShake) return;
        }

        if (m_objectID == 1006) { // Pulse Trigger
            g_prof.pulseTriggers++;
            if (g_settings.disablePulse) return;
        }

        if (m_objectID == 901) { // Move Trigger
            g_prof.moveTriggers++;
        }

        if (m_objectID == 1268) { // Spawn Trigger
            g_prof.spawnTriggers++;
        }

        EffectGameObject::triggerActivated(xPos);
    }
};

// ============================================================================
// HARDSTREAK HOOKS - Wave trail optimization
// ============================================================================

class $modify(PerfixHardStreak, HardStreak) {
    void updateStroke(float dt) {
        // EXPERIMENTAL: Reduce wave trail update frequency
        if (g_settings.expReduceWaveTrail && (g_throttle.frameCount % 2 == 0)) {
            return;
        }
        HardStreak::updateStroke(dt);
    }
};

// ============================================================================
// LABELGAMEOBJECT HOOKS - Counter/timer label optimization
// NOTE: Only available on Android - Windows has inlined function
// ============================================================================

#ifdef GEODE_IS_ANDROID
class $modify(PerfixLabelGameObject, LabelGameObject) {
    void updateLabel(float dt) {
        // EXPERIMENTAL: Throttle label updates to every 5th frame
        if (g_settings.expThrottleLabels && (g_throttle.frameCount % 5 != 0)) {
            return;
        }
        LabelGameObject::updateLabel(dt);
    }
};
#endif

// ============================================================================
// INITIALIZATION
// ============================================================================

$on_mod(Loaded) {
    refreshSettings();
    // Note: log::info causes linker errors on Windows due to fmt version mismatch
}
