# Architecture

The runtime is intentionally small:

- a tiny entrypoint in `src/main.cpp`
- an SDL application shell in `src/app/application.cpp`
- a read-only renderer in `src/render/render_frame.cpp`
- one authoritative simulation core in `src/core/game.cpp`

## High-Level Shape

Main runtime pieces:

1. Entrypoint: `src/main.cpp`
2. Platform shell: `src/app/application.cpp`
3. Renderer: `src/render/render_frame.cpp`
4. Simulation core: `src/core/game.cpp`

Public state and phase types:

- `include/arkanoid/core/game.hpp`
- `include/arkanoid/core/game_phase.hpp`
- `include/arkanoid/sim/game_state.hpp`

## Ownership Boundaries

### `src/main.cpp`

Parses run mode and enters the application runtime.

### `src/app/application.cpp`

Owns platform concerns:

- SDL init/shutdown
- window and renderer lifetime
- event polling
- keyboard sampling
- fixed-step accumulation and frame timing

It does not own gameplay rules.

### `src/render/render_frame.cpp`

Owns draw calls.
It reads `const GameState&` and does not mutate gameplay state.

### `arkanoid::Game`

Owns gameplay mutation in runtime code; tests intentionally bypass this for direct state setup.

`Game` contains:

- full `GameState`
- current held input latches
- previous serve input state (for edge-triggered serve)

Public API:

- `setInput(...)`
- `update(float dt)`
- `getState() const`

### `GameState`

`GameState` is authoritative runtime state:

- paddle
- ball
- a fixed-size brick array whose size is derived from the configured row/column layout
- score
- current phase
- phase timer

Rendering reads this state; gameplay update mutates it.

## Phase Model

Current phases:

- `CountdownYellow1`
- `CountdownPause1`
- `CountdownYellow2`
- `CountdownPause2`
- `CountdownGreen`
- `LaunchDrop`
- `BallReady`
- `Playing`
- `BoardClearedTransition`
- `LifeLostTransition`
- `ResetTransition`

This enum-backed state machine is the full gameplay progression model.

## Simulation-Owned Behavior

`src/core/game.cpp` owns gameplay behavior, including:

- canonical pre-serve pose
- brick layout generation
- countdown and launch-drop progression
- serve transition
- ball integration
- wall/paddle/brick collision responses
- score increments
- board clear auto-restart/new game
- miss and reset transitions

In runtime code, gameplay mutation flows through `Game::update()`.

## Restart And Reset Semantics

Board clear starts a new game:

- final brick hit during `Playing`
- transition to `BoardClearedTransition`
- next valid update recreates bricks and clears score
- restore canonical pre-serve paddle/ball pose
- restart countdown at `CountdownYellow1`

Life-loss reset is explicit:

- bottom miss during `Playing`
- transition to `LifeLostTransition`
- transition to `ResetTransition`
- restore canonical pre-serve paddle/ball pose
- restart countdown at `CountdownYellow1`

Life-loss reset does not recreate the full startup state:

- score persists
- destroyed bricks stay destroyed

## Rendering Boundary

Rendering is immediate and read-only.

`render_frame.cpp` reads `const GameState&` and draws bricks, paddle, ball, and countdown indicator.
Rendering does not mutate gameplay state.

## Collision Model

Collision behavior is intentionally simple and purpose-built:

- boundary checks against playfield limits using the ball center
- paddle hit test at paddle top crossing
- brick hit tests using vertical crossing and horizontal overlap
- collision checks are center-point style (simulation does not use rendered ball size)
- paddle position is center-clamped to playfield min/max, so paddle geometry can extend off-screen near edges
- at most one brick removed per simulation update step

## Test Boundary

Tests target simulation behavior (`Game`) through CTest/GTest and do not test SDL draw calls as gameplay.
