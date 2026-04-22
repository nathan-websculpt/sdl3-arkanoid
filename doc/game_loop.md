# Game Loop

This project uses a fixed-step simulation loop with SDL as the platform layer.

## Frame Flow

Each outer frame in `src/main.cpp` does:

1. poll SDL events
2. sample keyboard state
3. measure frame delta
4. clamp/sanitize delta and add to accumulator
5. run fixed simulation steps while accumulator has enough time
6. render current state

Canonical flow:

`SDL events -> input sample -> fixed-step simulation -> read-only render`

## Timing Model

Current constants in `src/main.cpp`:

- fixed step: `1 / 120` seconds
- max frame delta clamp: `0.25` seconds

Loop behavior:

- elapsed time is measured with SDL performance counters
- non-finite or negative frame delta is treated as `0`
- large frame delta is clamped before accumulation
- simulation advances in fixed increments via repeated `game.update(step)` calls

Physics/time integration is fixed-step; input is sampled once per outer frame.

## Input Flow

`main.cpp` samples held key state once per outer frame:

- left
- right
- serve

Those booleans are passed into `Game` and consumed during simulation update.
When multiple fixed steps run in one frame, each step receives the same sampled held-state values.

Serve is edge-triggered in simulation (`m_serveHeld` vs `m_previousServeHeld`), so holding serve does not retrigger launch every update.
Holding serve before `BallReady` does not auto-launch; a fresh press edge in `BallReady` is required.

## Simulation Update Boundary

`Game::update(float dt)` is the authoritative gameplay step.
Depending on phase, it advances timers/state and may:

- transition countdown phases based on `phaseTime` thresholds
- run launch-drop motion
- keep ball tethered to paddle during `LaunchDrop` and `BallReady`
- integrate live ball movement
- resolve wall/paddle/brick collisions
- increment score on brick hit
- trigger life-loss and reset transitions

Invalid `dt` (`NaN`, `inf`, zero, or negative) is ignored.

## Render Boundary

After fixed-step updates for the frame, `main.cpp` reads `const GameState&` and renders.
Rendering draws current state only and does not mutate gameplay state.

## Phase-Driven Runtime

The simulation uses explicit phases (countdown -> launch-drop -> ball-ready -> playing -> life-loss/reset).
Fixed-step updates advance both object motion and phase progression.
When a phase threshold is crossed, `phaseTime` is reset to `0` and overshoot is dropped.
Each `Game::update()` call executes one switch path for the current phase; `LifeLostTransition` and `ResetTransition` are transient phases that advance on subsequent updates.
