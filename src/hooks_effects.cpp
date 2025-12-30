// visual effect hooks (particles, trails, objects, triggers)

#include "globals.hpp"
#include <Geode/modify/GhostTrailEffect.hpp>
#include <Geode/modify/CCParticleSystem.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/GJEffectManager.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <Geode/modify/HardStreak.hpp>
#ifdef GEODE_IS_ANDROID
#include <Geode/modify/LabelGameObject.hpp>
#endif

// ghost trail

class $modify(PerfixGhostTrailEffect, GhostTrailEffect) {
    void trailSnapshot(float dt) {
        if (g_settings.disableTrails) {
            g_prof.trailSnapshotsSkipped++;
            return;
        }
        GhostTrailEffect::trailSnapshot(dt);
    }
};

// particle system

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

// game object

class $modify(PerfixGameObject, GameObject) {
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

    void activateObject() {
        if (g_settings.disableHighDetail && m_isHighDetail) {
            g_prof.highDetailSkipped++;
            return;
        }
        GameObject::activateObject();
    }
};

// effect manager

class $modify(PerfixGJEffectManager, GJEffectManager) {
    void updatePulseEffects(float dt) {
        PROFILE_START;
        GJEffectManager::updatePulseEffects(dt);
        PROFILE_ADD(g_prof.pulseEffectMs);
        g_prof.effectMs += g_prof.pulseEffectMs;
    }
};

// trigger tracking

class $modify(PerfixEffectGameObject, EffectGameObject) {
    void triggerActivated(float xPos) {
        g_prof.triggersActivated++;

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

// wave trail

class $modify(PerfixHardStreak, HardStreak) {
    void updateStroke(float dt) {
        if (g_settings.expReduceWaveTrail && (g_throttle.frameCount % 2 == 0)) return;
        HardStreak::updateStroke(dt);
    }
};

// label updates (android only, inlined on windows)

#ifdef GEODE_IS_ANDROID
class $modify(PerfixLabelGameObject, LabelGameObject) {
    void updateLabel(float dt) {
        if (g_settings.expThrottleLabels && (g_throttle.frameCount % 5 != 0)) return;
        LabelGameObject::updateLabel(dt);
    }
};
#endif
