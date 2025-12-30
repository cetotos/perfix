// core gameplay hooks

#include "globals.hpp"
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/ShaderLayer.hpp>

class $modify(PerfixBaseGameLayer, GJBaseGameLayer) {
    struct Fields {
        float profilerAccum = 0.0f;
        CCLabelBMFont* profilerLabel = nullptr;
        CCLabelBMFont* detailedLabel = nullptr;
        float settingsRefreshAccum = 0.0f;
    };

    void update(float dt) {
        g_throttle.frameCount++;

        // refresh settings periodically
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

        PROFILE_START;
        GJBaseGameLayer::update(dt);
        PROFILE_END(g_prof.updateMs);

        // gather stats
        g_prof.totalObjects = m_objects ? m_objects->count() : 0;
        g_prof.visibleObjects1 = m_visibleObjectsCount;
        g_prof.visibleObjects2 = m_visibleObjects2Count;
        g_prof.activeGradients = m_activeGradients;
        g_prof.shadersActive = (m_shaderLayer != nullptr);
        g_prof.leftSection = m_leftSectionIndex;
        g_prof.rightSection = m_rightSectionIndex;
        g_prof.topSection = m_topSectionIndex;
        g_prof.bottomSection = m_bottomSectionIndex;
        g_prof.batchNodeCount = m_batchNodes ? m_batchNodes->count() : 0;
        g_prof.estimatedDrawCalls = g_prof.batchNodeCount + g_prof.particleSystemCount +
                                    (g_prof.shadersActive ? 5 : 0) + g_prof.activeGradients;

        if (!g_prof.enabled) return;

        m_fields->profilerAccum += dt;
        if (m_fields->profilerAccum < 0.5f) return;
        m_fields->profilerAccum = 0.0f;

        updateProfilerDisplay();
    }

    void updateProfilerDisplay() {
        auto win = CCDirector::sharedDirector()->getWinSize();

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

        double avgWall = g_prof.wallFrameCount > 0 ? (g_prof.wallFrameTotal / g_prof.wallFrameCount) : 0.0;
        double avgSim = g_prof.simFrameCount > 0 ? (g_prof.simFrameTotal / g_prof.simFrameCount) : 0.0;
        double fpsWall = avgWall > 0.0 ? (1000.0 / avgWall) : 0.0;
        double fpsSim = avgSim > 0.0 ? (1000.0 / avgSim) : 0.0;

        std::string status = "";
        if (g_prof.frameSevereSpikes > 0) status = " [!!!]";
        else if (g_prof.frameSpikes > 0) status = " [!]";

        char grade = 'S';
        if (avgWall > 8.0) grade = 'A';
        if (avgWall > 12.0) grade = 'B';
        if (avgWall > 16.67) grade = 'C';
        if (avgWall > 25.0) grade = 'D';
        if (avgWall > 33.33) grade = 'F';

        char buf[2048];
        snprintf(buf, sizeof(buf),
            "Perfix%s\n"
            "FPS: %.0f (sim %.0f) | Grade: %c\n"
            "Frame: %.2fms (min %.1f / max %.1f)\n"
            "Spikes: %d (>20ms) | %d (>33ms)\n"
            "\n"
            "Objects\n"
            "Total: %d | Visible: %d/%d\n"
            "Sections: [%d-%d]x[%d-%d]\n"
            "\n"
            "Timings\n"
            "Update: %.2fms | Shader: %.2fms\n"
            "Particle: %.2fms | Effects: %.2fms\n"
            "Visibility: %.2fms | Collision: %.2fms\n"
            "Camera: %.2fms | Actions: %.2fms\n"
            "\n"
            "Rendering\n"
            "BatchNodes: %d | DrawCalls: ~%d\n"
            "Gradients: %d | Particles: %d\n"
            "\n"
            "Optimizations\n"
            "Skip: P%d G%d H%d T%d\n"
            "Triggers: %d (S%d P%d M%d)",
            status.c_str(),
            fpsWall, fpsSim, grade,
            avgWall, g_prof.wallFrameMin, g_prof.wallFrameMax,
            g_prof.frameSpikes, g_prof.frameSevereSpikes,
            g_prof.totalObjects, g_prof.visibleObjects1, g_prof.visibleObjects2,
            g_prof.leftSection, g_prof.rightSection, g_prof.bottomSection, g_prof.topSection,
            g_prof.updateMs, g_prof.shaderVisitMs,
            g_prof.particleMs, g_prof.effectMs,
            g_prof.visibilityMs, g_prof.collisionMs,
            g_prof.cameraMs, g_prof.moveActionsMs + g_prof.rotationActionsMs,
            g_prof.batchNodeCount, g_prof.estimatedDrawCalls,
            g_prof.activeGradients, g_prof.particleSystemCount,
            g_prof.particlesSkipped, g_prof.glowsDisabled, g_prof.highDetailSkipped, g_prof.trailSnapshotsSkipped,
            g_prof.triggersActivated, g_prof.spawnTriggers, g_prof.pulseTriggers, g_prof.moveTriggers
        );

        m_fields->profilerLabel->setString(buf);
        m_fields->profilerLabel->setVisible(true);

        // detailed panel on right side
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

            double totalTime = g_prof.updateMs + g_prof.shaderVisitMs;
            auto pct = [totalTime](double v) { return totalTime > 0 ? (v / totalTime * 100.0) : 0.0; };
            double actionTotal = g_prof.moveActionsMs + g_prof.rotationActionsMs +
                                 g_prof.transformActionsMs + g_prof.areaActionsMs;

            char detailBuf[1024];
            snprintf(detailBuf, sizeof(detailBuf),
                "Breakdown\n"
                "Shader: %.1f%%\n"
                "Effects: %.1f%%\n"
                "  pulse: %.2fms\n"
                "  opacity: %.2fms\n"
                "Actions: %.1f%%\n"
                "  move: %.2fms\n"
                "  rotate: %.2fms\n"
                "  transform: %.2fms\n"
                "  area: %.2fms\n"
                "Particles: %.1f%%\n"
                "\n"
                "Other\n"
                "  visibility: %.2fms\n"
                "  collision: %.2fms\n"
                "  camera: %.2fms",
                pct(g_prof.shaderVisitMs),
                pct(g_prof.effectMs),
                g_prof.pulseEffectMs, g_prof.opacityEffectMs,
                pct(actionTotal),
                g_prof.moveActionsMs, g_prof.rotationActionsMs,
                g_prof.transformActionsMs, g_prof.areaActionsMs,
                pct(g_prof.particleMs),
                g_prof.visibilityMs, g_prof.collisionMs, g_prof.cameraMs
            );

            m_fields->detailedLabel->setString(detailBuf);
            m_fields->detailedLabel->setVisible(true);
        } else if (m_fields->detailedLabel) {
            m_fields->detailedLabel->setVisible(false);
        }

        g_prof.reset();
    }

    void updateShaderLayer(float dt) {
        if (g_settings.disableShaders) {
            if (m_shaderLayer) m_shaderLayer->setVisible(false);
            return;
        }
        if (m_shaderLayer) m_shaderLayer->setVisible(true);
        GJBaseGameLayer::updateShaderLayer(dt);
    }

    void processMoveActions() {
        if (g_settings.expThrottleActions && (g_throttle.frameCount % 2 == 0)) return;
        PROFILE_START;
        GJBaseGameLayer::processMoveActions();
        PROFILE_ADD(g_prof.moveActionsMs);
    }

    void processRotationActions() {
        if (g_settings.expThrottleActions && (g_throttle.frameCount % 2 == 0)) return;
        PROFILE_START;
        GJBaseGameLayer::processRotationActions();
        PROFILE_ADD(g_prof.rotationActionsMs);
    }

    void processTransformActions(bool visibleFrame) {
        if (g_settings.expThrottleTransforms && !visibleFrame) return;
        PROFILE_START;
        GJBaseGameLayer::processTransformActions(visibleFrame);
        PROFILE_ADD(g_prof.transformActionsMs);
    }

    void processAreaActions(float dt, bool p1) {
        if (g_settings.expSkipAreaEffects) return;
        PROFILE_START;
        GJBaseGameLayer::processAreaActions(dt, p1);
        PROFILE_ADD(g_prof.areaActionsMs);
    }

    void processFollowActions() {
        if (g_settings.expSkipFollowActions) return;
        PROFILE_START;
        GJBaseGameLayer::processFollowActions();
        PROFILE_ADD(g_prof.transformActionsMs);
    }

    void spawnGroup(int group, bool ordered, double delay, gd::vector<int> const& remapKeys, int triggerID, int controlID) {
        g_prof.spawnTriggers++;
        if (g_settings.expThrottleSpawns) {
            if (g_throttle.frameCount - g_throttle.lastSpawnFrame < 2) return;
            g_throttle.lastSpawnFrame = g_throttle.frameCount;
        }
        GJBaseGameLayer::spawnGroup(group, ordered, delay, remapKeys, triggerID, controlID);
    }

    void updateGradientLayers() {
        if (g_settings.expThrottleGradients && (g_throttle.frameCount % 3 != 0)) return;
        GJBaseGameLayer::updateGradientLayers();
    }

    void processAdvancedFollowActions(float dt) {
        if (g_settings.expThrottleAdvancedFollow && (g_throttle.frameCount % 2 == 0)) return;
        GJBaseGameLayer::processAdvancedFollowActions(dt);
    }

    void processDynamicObjectActions(int groupID, float dt) {
        if (g_settings.expThrottleDynamicObjects && (g_throttle.frameCount % 2 == 0)) return;
        GJBaseGameLayer::processDynamicObjectActions(groupID, dt);
    }

    void processPlayerFollowActions(float dt) {
        if (g_settings.expThrottlePlayerFollow && (g_throttle.frameCount % 2 == 0)) return;
        GJBaseGameLayer::processPlayerFollowActions(dt);
    }

    void updateEnterEffects(float dt) {
        if (g_settings.expLimitEnterEffects && (g_throttle.frameCount % 2 == 0)) return;
        GJBaseGameLayer::updateEnterEffects(dt);
    }
};

class $modify(PerfixPlayLayer, PlayLayer) {
    void shakeCamera(float duration, float strength, float interval) {
        if (g_settings.disableShake) {
            g_prof.shakesSkipped++;
            return;
        }
        PlayLayer::shakeCamera(duration, strength, interval);
    }

    void updateVisibility(float dt) {
        if (g_settings.disableParticles) m_disableGravityEffect = true;
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
