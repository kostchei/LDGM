---
title: "Legally Distinct Gorka Morka — Development Plan"
version: "0.1"
date: "2026-07-14"
source_specification: "legally_distinct_gorka_morka_prd_eng_spec_v0_4.md"
status: "Proposed execution plan"
---

# Purpose

This plan converts the v0.4 product and engineering specification into sequential tranches. Each tranche ends in a demonstrable product state or a decisive technical result. A later tranche should not begin until the preceding exit gate passes, unless the work is explicitly isolated and cannot conceal a failed dependency.

The plan deliberately delays broad persistence, multiplayer and scale work until the single-player driving and mission loop is proven. The MVP ends at **T7**. **T8** is a post-MVP expansion.

# Permanent constraints carried into every tranche

- First-person driving and vehicle combat only; no player-accessible third-person driving camera.
- Maximum of 30 simultaneously active vehicles per gameplay instance.
- Project Chrono is used for whole-vehicle motion, traction, suspension response, collision and terrain contact only.
- No internal vehicle-mechanics simulation.
- No runtime panel deformation or detailed live damage rendering.
- Ordinary damage is communicated through handling/performance loss, cockpit indicators and audio.
- Smoke or fire is reserved for heavily damaged, critical or destroyed vehicles.
- Detailed damage location exists only on the repair schematic.
- Server authority owns combat, mission, inventory, reward and persistent outcomes.

# Delivery rules

1. **Playable evidence beats framework completion.** Each tranche must produce a build or headless test that demonstrates its claim.
2. **Do not build the next abstraction early.** For example, do not build auctions before the persistent inventory transaction model passes.
3. **Every exit gate is binary.** Known failures are either fixed, explicitly removed from scope or accepted in a written decision record.
4. **Automate stable checks.** Build, schema, persistence, authority and scale checks belong in CI once they stop changing daily.
5. **Use golden telemetry, not bit-identical physics.** Vehicle regression tests compare measured ranges and tolerances.
6. **The 30-vehicle cap is a rule, not a performance aspiration.** Overflow is rejected or queued; the cap is never silently exceeded.
7. **Test documents are source-controlled.** The JSON files in `test_goals/` define the current acceptance contract for each tranche.

# Tranche map

| Tranche | Outcome | MVP? | Depends on |
|---|---|---:|---|
| T0 | Engine and Vehicle Integration Spike | Yes | None |
| T1 | First-Person Driving Prototype | Yes | T0 |
| T2 | Combat and Broad Damage States | Yes | T1 |
| T3 | Single-Player Vertical Slice | Yes | T2 |
| T4 | Persistent Level 1 Game Loop | Yes | T3 |
| T5 | Authoritative Cooperative Multiplayer | Yes | T4 |
| T6 | Contested Zone Alpha | Yes | T5 |
| T7 | Thirty-Vehicle Scale and MVP Release Candidate | Yes | T6 |
| T8 | Level 2 Expansion | Post-MVP | T7 |

# Detailed tranches

## T0 — Engine and Vehicle Integration Spike

**Phase:** Foundation  
**Objective:** Prove that the pinned O3DE and Project Chrono versions can build, run and exchange authoritative whole-vehicle state before gameplay systems are added.

### Deliverables

- Pinned O3DE 26.05.0 project and reproducible build instructions.
- Project Chrono 10.0.0 packaged and linked through a minimal LDMChronoVehicle Gem.
- One generic vehicle driving on terrain shared by O3DE and Chrono.
- Stable first-person cockpit camera with no player-accessible third-person driving view.
- Headless authoritative host sending snapshots to one client.
- Active-vehicle registry with a hard maximum of 30 and deterministic overflow rejection.
- Basic timing, simulation and network telemetry.

### Explicitly excluded

- Custom starter technical art.
- Weapons, missions, persistence and economy.
- Detailed vehicle assembly.
- Client prediction.

### Exit gate

Proceed only when build packaging, coordinate conversion, terrain alignment, transform synchronization, first-person camera stability and server simulation have no unresolved architecture blocker.

**Test contract:** `test_goals/t0_integration_spike.json`

## T1 — First-Person Driving Prototype

**Phase:** Prototype  
**Objective:** Deliver the basic driving experience with the starter technical, the required surfaces and usable cockpit controls.

### Deliverables

- Starter technical chassis and cockpit.
- Keyboard and gamepad driving controls; input abstraction ready for HOTAS mapping.
- Hard road, broken road, gravel, firm sand and deep-sand surface profiles.
- First-person head-look and Perception wide/far modes.
- Fuel consumption and basic vehicle condition values.
- Physics regression range and golden telemetry capture.

### Explicitly excluded

- Weapons and combat.
- Damage feedback beyond developer controls.
- Mission flow and persistence.
- External player driving camera.

### Exit gate

The starter technical is controllable, stable and measurably distinct across the required terrain surfaces, with repeatable physics telemetry inside agreed tolerances.

**Test contract:** `test_goals/t1_driving_prototype.json`

## T2 — Combat and Broad Damage States

**Phase:** Prototype  
**Objective:** Add fixed-forward combat and inexpensive damage feedback without internal-mechanics simulation or detailed damage rendering.

### Deliverables

- Two independently tracked fixed rifles with 50 rounds each and cabin sights.
- Server-authoritative projectile, hit and ammunition handling.
- Intact, damaged, critical and destroyed vehicle states.
- Authored handling, braking, acceleration or power loss for damaged states.
- Cockpit gauge/warning changes such as oil or fuel pressure indicators.
- Altered engine or mechanical audio for damage feedback.
- Smoke or fire only for critical, heavily damaged or destroyed vehicles.
- Repair-zone records and a simple repair schematic.

### Explicitly excluded

- Panel deformation, damaged meshes, impact decals and detailed damage textures.
- Individually simulated engines, transmissions, oil systems, fuel lines or other internals.
- Complex armour penetration simulation.
- Third-person combat camera.

### Exit gate

A player can shoot, be hit and correctly infer ordinary damage from vehicle behaviour, instruments or audio; critical vehicles may smoke or burn; repair data identifies broad damaged zones.

**Test contract:** `test_goals/t2_combat_damage.json`

## T3 — Single-Player Vertical Slice

**Phase:** Vertical Slice  
**Objective:** Prove the complete game loop with one package-delivery mission before investing in broad persistence or multiplayer.

### Deliverables

- Bartertown mission board, garage and departure flow.
- One package-delivery mission across road, gravel and sand.
- Mission success, failure and abandonment outcomes.
- Return, repair, refuel and rearm flow.
- One salvaged external part that can be installed and changes appearance or broad handling.
- Basic route risk, fuel estimate and cargo mass/fragility presentation.
- Playable build suitable for structured user testing.

### Explicitly excluded

- All four mission templates.
- PostgreSQL persistence.
- PvP, auctions and Despot Depot services.
- More than one player.

### Exit gate

A new tester can launch, accept the mission, drive, fight, take understandable damage, complete or fail, return and modify the vehicle without developer intervention.

**Test contract:** `test_goals/t3_vertical_slice.json`

## T4 — Persistent Level 1 Game Loop

**Phase:** Mvp Build  
**Objective:** Turn the vertical slice into a durable solo game loop with all Level 1 mission types and authoritative progression.

### Deliverables

- PostgreSQL persistence with SQLite test fixtures.
- Package delivery, camp raid, moving-target interception and salvage recovery missions.
- Character stats, permanent Fame and bounded Display Fame.
- Inventory, salvage, shops, fuel, ammunition and external-part installation.
- Transactional rewards, recovery and repair.
- Disconnect/reconnect without duplication or reward replay.
- Data-authored mission definitions and schema validation.

### Explicitly excluded

- Player-versus-player zones.
- Auction settlement between players.
- More than one connected player as a release requirement.
- Level 2 progression.

### Exit gate

A player can complete repeated Level 1 sessions across restarts without losing or duplicating characters, vehicles, inventory, mission state or rewards.

**Test contract:** `test_goals/t4_persistent_level_one.json`

## T5 — Authoritative Cooperative Multiplayer

**Phase:** Mvp Build  
**Objective:** Add reliable server-authoritative play for up to four players while preserving the first-person vehicle feel and persistent state.

### Deliverables

- Dedicated-server join, leave and reconnect flow.
- Up to four players driving and firing in one active zone.
- Interest-managed vehicle snapshots and interpolation.
- Authoritative ammunition, hits, damage, mission state and rewards.
- Latency, loss and snapshot-reordering test harness.
- Collision correction that does not destabilize the first-person camera.
- Security checks for impossible transforms, fire rates and inventory changes.

### Explicitly excluded

- Eight-player target.
- PvP band and contested-zone rules.
- Local shadow prediction unless interpolation proves insufficient.
- Thirty-vehicle performance qualification.

### Exit gate

Four players can complete Level 1 missions under representative network impairment without state duplication, authority violations or unusable first-person correction.

**Test contract:** `test_goals/t5_authoritative_multiplayer.json`

## T6 — Contested Zone Alpha

**Phase:** Alpha  
**Objective:** Add the territorial risk layer, eight-player target and dangerous service hub.

### Deliverables

- Clearly bounded contested PvP band.
- Long safe route and shorter contested route.
- Despot Depot service point.
- Up to eight connected players.
- PvP damage, defeat and recovery policy implementation.
- Basic auction listing and authoritative settlement.
- Expanded armour coverage and repair-zone data.
- Prediction/correction absorber only if required by test results.

### Explicitly excluded

- Large faction wars.
- Full-loot assumptions unless separately approved.
- Level 2 specialist equipment.
- More than 30 active vehicles.

### Exit gate

Players can knowingly enter and leave the contested band, fight under server authority, use the depot and settle auction transactions without duplication or ambiguous PvP rules.

**Test contract:** `test_goals/t6_contested_zone_alpha.json`

## T7 — Thirty-Vehicle Scale and MVP Release Candidate

**Phase:** Release Candidate  
**Objective:** Qualify the permanent 30-active-vehicle ceiling, performance budgets and operational reliability for the MVP.

### Deliverables

- Thirty simultaneous active vehicles across player, AI and active parked categories.
- Hard rejection or queueing of the thirty-first active vehicle.
- Simulation LOD transitions that preserve state.
- Client and server profiling against defined budgets.
- Eight-player combat-and-driving load scenario.
- Long-running reliability and reconnect tests.
- Release CI, migrations, telemetry and support diagnostics.
- Final MVP content and regression pass.

### Explicitly excluded

- Raising the vehicle cap to solve performance problems.
- Deformable terrain or vehicle panels.
- Level 2 content.
- Seamless MMO architecture.

### Exit gate

The release candidate meets the hard population rule, functional regression gates and agreed performance/reliability thresholds on the defined test machines.

**Test contract:** `test_goals/t7_scale_release_candidate.json`

## T8 — Level 2 Expansion

**Phase:** Post Mvp  
**Objective:** Add Fame-gated progression and specialist external equipment without expanding into internal vehicle simulation.

### Deliverables

- Fame 20 Level 2 gate.
- Multi-stage mission templates.
- Specialist external parts and limited advanced fields.
- Economy, reward and progression rebalance from MVP telemetry.
- Additional vehicles and environments only within the 30-active-vehicle cap.
- Updated test corpus and compatibility migrations.

### Explicitly excluded

- Internal component simulation.
- Aircraft, boats or walkers.
- Globally deformable terrain.
- Unbounded exotic equipment systems.

### Exit gate

Level 2 adds meaningful choices and mission complexity while preserving all permanent scope limits and MVP reliability gates.

**Test contract:** `test_goals/t8_level_two_expansion.json`

# Cross-tranche quality gates

## Build and content

- Pinned engine and third-party versions.
- Reproducible developer setup.
- JSON schema validation for data-authored content.
- No missing simulation-relevant assets in release builds.
- Third-party notices generated in CI.

## Physics and first-person experience

- Physics regressions use tolerances for acceleration, braking, turning, terrain speed, rollover and collision recovery.
- Camera correction and shake tests are performed from the cockpit.
- No player command or input binding can enter an external driving camera.
- Damage feedback remains broad and authored; tests must not depend on internal engine-component state.

## Authority and persistence

- Clients send inputs and requests, not authoritative transforms, ammunition counts, rewards or prices.
- Reward and auction operations use idempotency keys.
- Vehicle, component and inventory writes use version checks or equivalent locking.
- Disconnect, retry and duplicate-message tests must not create additional items, currency, vehicles or mission rewards.

## Population and performance

- The active registry includes every LOD 0–2 player, AI and physically active parked vehicle.
- An attempted thirty-first active vehicle is rejected or placed in an explicit queue.
- The cap is tested in unit, integration and load suites.
- Performance tuning may reduce distant simulation fidelity but may not increase the cap.

# Change control

A tranche test may change only when one of the following is recorded:

- The product requirement changed.
- The test was invalid or could not measure the intended behaviour.
- The implementation approach changed without weakening the user-facing requirement.
- A threshold was provisional and profiling produced a better justified value.

A failing test is not removed solely to allow the tranche to pass.

# Produced documents

- `development_plan.md` — this plan.
- `test_goals/test_goal.schema.json` — validation schema.
- `test_goals/manifest.json` — tranche and file index.
- One JSON acceptance contract for each tranche, T0 through T8.
