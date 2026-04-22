# Architecture

The runtime is intentionally small:

- a thin SDL shell in `src/main.cpp`
- one authoritative simulation core in `src/core/game.cpp`

## High-Level Shape

Main runtime pieces:

1. Platform shell: `src/main.cpp`
2. Simulation core: `src/core/game.cpp`

Public state and phase types:

- `include/arkanoid/core/game.hpp`
- `include/arkanoid/core/game_phase.hpp`
- `include/arkanoid/sim/game_state.hpp`

## Ownership Boundaries

### `main.cpp`

Owns platform concerns:

- SDL init/shutdown
- window and renderer lifetime
- event polling
- keyboard sampling
- fixed-step accumulation and frame timing
- draw calls

`main.cpp` does not own gameplay rules.

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
- fixed-size brick array (`24`)
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
- `LifeLostTransition`
- `ResetTransition`

This enum-backed phase machine is the full gameplay progression model.

## Simulation-Owned Behavior

`src/core/game.cpp` owns gameplay behavior, including:

- canonical pre-serve pose
- brick layout generation
- countdown and launch-drop progression
- serve transition
- ball integration
- wall/paddle/brick collision responses
- score increments
- miss and reset transitions

In runtime code, gameplay mutation flows through `Game::update()`.

## Reset Semantics

Life-loss reset is explicit:

- miss during `Playing`
- transition to `LifeLostTransition`
- transition to `ResetTransition`
- restore canonical pre-serve paddle/ball pose
- restart countdown at `CountdownYellow1`

Life-loss reset does not recreate the full startup state:

- score persists
- destroyed bricks stay destroyed

## Rendering Boundary

Rendering is immediate and read-only.

`main.cpp` reads `const GameState&` and draws bricks, paddle, ball, and countdown indicator.
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
