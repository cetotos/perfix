# Perfix - Performance Fixes for Geometry Dash

A comprehensive performance optimization mod with a built-in debug profiler to find and fix lag spikes.

## Features

### Debug Profiler
- **Real-time FPS display** - Both simulation FPS and wall-clock FPS
- **Frame time breakdown** - See exactly where time is spent (Update, Shader, Particles, Effects)
- **Object count tracking** - Total objects, visible objects
- **Optimization statistics** - Shows how many particles/glows/objects are being skipped

### Performance Optimizations

#### Shader Effects (BIGGEST IMPACT)
| Setting | Effect | FPS Impact |
|---------|--------|------------|
| Disable ALL Shaders | Removes chromatic, sepia, glitch, blur, pixelate, bulge, shockwave, lens, motion blur | +++ |

#### Visual Effects
| Setting | Effect | FPS Impact |
|---------|--------|------------|
| Disable Particles | Removes all particle systems (fire, dust, explosions) | +++ |
| Reduced Particles | Updates particles every other frame | ++ |
| Disable Glow | Removes glow sprites from objects and player | ++ |
| Disable Trails | Removes player ghost trail snapshots | + |

#### Trigger Effects
| Setting | Effect | FPS Impact |
|---------|--------|------------|
| Disable Pulse | Disables color pulse triggers | ++ |
| Disable Shake | Disables camera shake triggers | + |
| Disable Opacity/Move | Disables alpha trigger animations | + |

#### Object Optimizations
| Setting | Effect | FPS Impact |
|---------|--------|------------|
| Disable High-Detail | Skips objects marked as high-detail | ++ |

## Why Levels Lag

### Common Performance Issues

1. **Shader Effects (Critical)** - Post-processing shaders like chromatic aberration, glitch, and blur require multiple GPU render passes. A single shader trigger can drop FPS by 50%.

2. **Particle Systems** - Each particle system can have hundreds of individual particles, each requiring position/color updates every frame.

3. **Glow Effects** - Additive blending requires reading back from the framebuffer, which is expensive on GPUs.

4. **Pulse Triggers** - Color interpolation calculations for every affected object each frame.

5. **Move/Alpha Triggers** - Position and opacity recalculation for grouped objects.

### The "Dash" Spider Section
The spider section is laggy because of:
- Multiple move triggers activating simultaneously
- Pulse triggers on many groups
- High object density with glow effects
- Continuous transform updates

## Recommended Settings

### Maximum Performance (Potato PC / Mobile)
- Enable ALL shader, particle, and glow disables
- Disable high-detail objects
- Disable pulse and shake

### Balanced (Mid-range)
- Disable shaders
- Use reduced particles (not full disable)
- Disable glow
- Keep pulse and shake enabled

### Profiler Only
- Only enable "Show Profiler Overlay"
- All other settings off
- Use to identify problem areas in levels

## Technical Details

This mod hooks into:
- `ShaderLayer` - Skips shader render passes
- `CCParticleSystem` - Skips particle updates
- `GJEffectManager` - Skips pulse/opacity calculations
- `GhostTrailEffect` - Skips trail snapshot creation
- `GameObject` - Skips glow color updates and high-detail activation
- `PlayLayer` - Skips camera shake and glitter effects
- `PlayerObject` - Skips player glow and fire particles
- `EffectGameObject` - Skips shake/pulse trigger activation

Settings are cached and refreshed every 0.25 seconds for minimal overhead.
