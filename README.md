# AETHERWING

A lightweight, native 3D arcade aerial-combat & floating-archipelago starfighter built in **C++17 + SDL3 + OpenGL 3.3 Core** specifically tuned for Arch Linux systems with tight CPU & RAM budgets (4 CPU cores, 1.6 GB RAM — runs great on both hardware GPU and Mesa `llvmpipe`).

## Features
- **Zero external asset bloat**: 100% procedurally crafted 3D starfighters, enemy classes, colossal multi-core Sky-Dreadnought bosses, terraced islands, floating crystal sky-isles, stone arch ring gates, drifting clouds, and animated sunlit ocean waves.
- **3 Atmospheric Sectors**: *Azure Archipelago* (golden hour), *Ember Caldera* (sunset volcanic twilight), and *Astral Citadel* (bioluminescent midnight aurora + starfield).
- **Juicy Combat & Flight Mechanics**:
  - Twin pulse cannons with subtle lead-assisted targeting & predictive 3D HUD lead pips.
  - Hold Right-Click (`K`/`X`) to paint up to 4+ enemies/boss cores and fire curving Macross-style **Swarm Missiles** with ribbon trails.
  - **Snap Barrel Roll** (`Space` / `Q` / `E`) that deflects incoming enemy plasma fire!
  - **Afterburner Boost** (`Shift`) with dynamic FOV surge, sonic shockwaves, wingtip vapor trails, and radial speed streaks.
  - **Ancient Ring Gates** for instant shield recharge, hyper-boost, and combo multipliers.
  - **Wave-Skimming** low over ocean waves kicks up water spray and recharges boost!
  - **Roguelite Upgrade Draft** between waves + **Free-Flight Tour** mode.
- **Procedural SDL3 Audio Synthesizer**: Real-time jet turbine hum, polyphonic laser/explosion/lock-on/ring-gate SFX, and dynamic synthwave soundtrack that shifts intensity during Dreadnought Boss fights.
- **Hardware-Adaptive Graphics (`F1`)**: Auto-detects `llvmpipe` vs hardware GPU and renders the 3D world to a scaled HDR FBO with ACES filmic tone-mapping + optional bloom while keeping the 2D HUD, reticles, and radar razor-sharp at native window resolution.

## Build & Run

```bash
make
./aetherwing
```

*(Note: Build with single-threaded `make` rather than `make -j4` on 1.6 GB RAM systems to keep compiler RAM usage low.)*

Optional CLI flags:
```bash
./aetherwing --quality fast        # Best FPS on software rendering (llvmpipe)
./aetherwing --quality balanced    # Crisp 3D + bloom + native HUD (default on llvmpipe)
./aetherwing --quality high        # Full native resolution + HDR bloom (default on GPU)
./aetherwing --res 960x540         # Custom window resolution
./aetherwing --fullscreen          # Launch fullscreen
```

## Controls
| Input | Action |
|---|---|
| **Mouse** or **Arrow Keys** | Steer pitch & yaw (no pointer-grab needed) |
| **W / S** or **Scroll Wheel** | Throttle Up / Airbrake |
| **A / D** | Bank-Turn Left / Right |
| **Left Click** or **Z / J** | Twin Plasma Cannons |
| **Hold Right Click** or **X / K** | Paint Targets → Release for Swarm Missiles |
| **Shift** (or Middle Click) | Afterburner Boost |
| **Space** / **Q** / **E** | Snap Barrel Roll (Deflects Enemy Fire!) |
| **C** | Cycle Camera (Chase / Close Action / Cockpit) |
| **T** | Switch Atmospheric Sector (Title / Free-Flight) |
| **F1** | Cycle Quality (`FAST` / `BALANCED` / `HIGH`) |
| **F11** | Toggle Fullscreen |
| **M** / **N** | Toggle Music / Mute All Audio |
| **P** / **Esc** | Pause / Menu |
