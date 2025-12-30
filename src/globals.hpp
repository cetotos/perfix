#pragma once

#include <Geode/Geode.hpp>
#include <chrono>
#include <deque>

using namespace geode::prelude;

// profiler state
struct UltraProfiler {
    bool enabled = false;

    // frame timing
    double wallFrameTotal = 0.0;
    double wallFrameMax = 0.0;
    double wallFrameMin = 999.0;
    int wallFrameCount = 0;
    std::chrono::steady_clock::time_point lastFrameTs{};
    bool hasLastFrameTs = false;

    double simFrameTotal = 0.0;
    double simFrameMax = 0.0;
    int simFrameCount = 0;

    // component timings in ms
    double updateMs = 0.0;
    double shaderVisitMs = 0.0;
    double shaderCalcMs = 0.0;
    double particleMs = 0.0;
    double effectMs = 0.0;
    double pulseEffectMs = 0.0;
    double opacityEffectMs = 0.0;
    double visibilityMs = 0.0;
    double collisionMs = 0.0;
    double cameraMs = 0.0;
    double moveActionsMs = 0.0;
    double rotationActionsMs = 0.0;
    double transformActionsMs = 0.0;
    double areaActionsMs = 0.0;
    double audioMs = 0.0;
    double postUpdateMs = 0.0;

    // object counts
    int totalObjects = 0;
    int visibleObjects1 = 0;
    int visibleObjects2 = 0;
    int activeObjects = 0;
    int disabledObjects = 0;
    int areaObjects = 0;
    int solidCollisionObjs = 0;
    int hazardCollisionObjs = 0;

    // particles
    int particleSystemCount = 0;
    int particlesSkipped = 0;
    int particleUpdateCalls = 0;
    int particleAddCalls = 0;

    // effects
    int pulseEffectsActive = 0;
    int opacityEffectsActive = 0;
    int moveActionsActive = 0;
    int rotationActionsActive = 0;
    int colorActionsActive = 0;
    int activeGradients = 0;

    // optimization counters
    int glowsDisabled = 0;
    int highDetailSkipped = 0;
    int trailSnapshotsSkipped = 0;
    int shakesSkipped = 0;

    // triggers
    int triggersActivated = 0;
    int pulseTriggers = 0;
    int shakeTriggers = 0;
    int moveTriggers = 0;
    int spawnTriggers = 0;

    // rendering
    int batchNodeCount = 0;
    int estimatedDrawCalls = 0;
    int textureBindEstimate = 0;
    bool shadersActive = false;
    int shaderEffectsActive = 0;

    // sections
    int sectionsChecked = 0;
    int leftSection = 0;
    int rightSection = 0;
    int topSection = 0;
    int bottomSection = 0;

    // frame history for spike detection
    std::deque<double> frameHistory;
    int frameSpikes = 0;
    int frameSevereSpikes = 0;

    // audio
    int sfxTriggersProcessed = 0;
    int audioTriggersActive = 0;

    void reset() {
        wallFrameTotal = wallFrameMax = 0.0;
        wallFrameMin = 999.0;
        wallFrameCount = simFrameCount = 0;
        simFrameTotal = simFrameMax = 0.0;

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

extern UltraProfiler g_prof;

// cached settings
struct SettingsCache {
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
    bool expThrottleActions = false;
    bool expSkipAreaEffects = false;
    bool expThrottleTransforms = false;
    bool expThrottleSpawns = false;
    bool expReduceCollisions = false;
    bool expAggressiveCulling = false;
    bool expSkipFollowActions = false;
    bool expReduceColorUpdates = false;
    bool expThrottleGradients = false;
    bool expReduceWaveTrail = false;
    bool expThrottleAdvancedFollow = false;
    bool expThrottleDynamicObjects = false;
    bool expThrottlePlayerFollow = false;
    bool expLimitEnterEffects = false;
    bool expThrottleLabels = false;
    bool cacheValid = false;
};

extern SettingsCache g_settings;

// throttle state
struct ThrottleState {
    int frameCount = 0;
    int lastSpawnFrame = 0;
};

extern ThrottleState g_throttle;

void refreshSettings();

// timing macros
#define PROFILE_START auto _prof_start = std::chrono::steady_clock::now()
#define PROFILE_END(var) var = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - _prof_start).count()
#define PROFILE_ADD(var) var += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - _prof_start).count()

inline void profilerSimFrame(float dt) {
    double ms = dt * 1000.0;
    g_prof.simFrameTotal += ms;
    g_prof.simFrameCount++;
    if (ms > g_prof.simFrameMax) g_prof.simFrameMax = ms;
}

inline void profilerWallFrame() {
    auto now = std::chrono::steady_clock::now();
    if (g_prof.hasLastFrameTs) {
        double ms = std::chrono::duration<double, std::milli>(now - g_prof.lastFrameTs).count();
        g_prof.wallFrameTotal += ms;
        g_prof.wallFrameCount++;
        if (ms > g_prof.wallFrameMax) g_prof.wallFrameMax = ms;
        if (ms < g_prof.wallFrameMin) g_prof.wallFrameMin = ms;

        g_prof.frameHistory.push_back(ms);
        if (g_prof.frameHistory.size() > 60) g_prof.frameHistory.pop_front();
        if (ms > 20.0) g_prof.frameSpikes++;
        if (ms > 33.33) g_prof.frameSevereSpikes++;
    }
    g_prof.lastFrameTs = now;
    g_prof.hasLastFrameTs = true;
}
