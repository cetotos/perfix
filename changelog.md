# Changelog

## v1.1.0
- **Major Update: Comprehensive Performance Patches**
- Added shader optimization hooks (chromatic, sepia, glitch, blur, pixelate, etc.)
- Added particle system hooks with full disable and reduced mode
- Added glow effect optimization (objects and player)
- Added pulse trigger optimization (GJEffectManager)
- Added camera shake optimization (PlayLayer + EffectGameObject)
- Added opacity/move effect optimization
- Added high-detail object skipping
- Added player-specific optimizations (fire, glow, trails)
- Improved profiler with timing breakdown (Update, Shader, Particles, Effects)
- Added optimization statistics display (skipped particles/glows/objects)
- Settings are now cached for better performance
- Reorganized settings into logical sections

## v1.0.7
- Stability improvements
- Fixed shader layer visibility toggle

## v1.0.0
- Initial release
- Debug profiler overlay with FPS, frame time, object count
- Basic shader disable
- Trail optimization
