# Legally Distinct Gorka Morka

## Product requirements and engineering specification — v0.4 (reconstructed)

**Status:** Working specification  
**Reconstructed:** 2026-07-14  
**Primary technologies:** Open 3D Engine (O3DE), Project Chrono  
**Authority:** This document is the product boundary for T0–T8. `RECOVERY_NOTES.md` records what was recovered and what remains undecided.

## 1. Product definition

Legally Distinct Gorka Morka is a first-person vehicular combat game built around driving an improvised armed vehicle, fighting other vehicles in bounded levels, recovering rewards, returning to a garage, and repairing or changing the vehicle before redeployment.

The implementation must prove the single-player driving and combat loop before persistence, multiplayer, contested zones, or large-scale simulation are allowed to become schedule-critical.

### 1.1 Player promise

The player should feel that they are physically driving a heavy, imperfect machine rather than steering a free-moving camera. Terrain, traction, suspension, mass, impacts, degraded handling, cockpit warnings, and mechanical audio carry that sensation. The project deliberately does not simulate hidden internal machinery.

### 1.2 Core loop

1. Select an available vehicle and loadout in a stationary garage context.
2. Deploy into a bounded gameplay level.
3. Drive, navigate terrain, engage AI or player-controlled vehicles, and pursue a scenario objective.
4. Survive or reach a defined destroyed/failure state.
5. Return results to the garage or persistence service.
6. Repair broad damaged zones, change allowed external equipment, and redeploy.

Exact economy values, mission counts, vehicle roster, and content quantities are content-design decisions outside v0.4.

### 1.3 Release envelope

- Single-player vertical slice before persistent or networked play.
- Four-player authoritative multiplayer before eight-player contested zones.
- Hard maximum of 30 concurrently active vehicles per gameplay instance.
- Level 1 provides the MVP loop; Level 2 is post-MVP expansion.

### 1.4 Permanent non-goals and hard limits

These constraints apply to every release unless this specification is deliberately superseded:

- **No more than 30 concurrently active vehicles.** The count includes player, AI, remote-player, and physically instantiated parked vehicles. A request that would create vehicle 31 must be rejected or queued. Persisted records outside the active simulation do not count.
- **No runtime panel or body deformation.** Collision may affect motion and condition but does not bend a simulated body shell.
- **No detailed live damage artwork.** Do not require damaged textures, impact decals, animated damage GIFs, image-sequence damage skins, detachable panels, or per-impact body changes.
- **No internal mechanical simulation.** Do not model engines, transmissions, oil pumps, fuel lines, wiring, internal subsystems, component faults, or diagnostic chains.
- **No third-person driving camera.** Driving is first-person. External cameras are allowed only in stationary garage/repair screens, spectating, replay/debug tooling, and editor tooling.
- **No automatic smoke for ordinary damage.** Smoke and fire indicate critical, destroyed, or heavily damaged conditions only.
- **No requirement for more than eight human players in the MVP programme.** Thirty active vehicles may include AI and physically instantiated parked vehicles.

## 2. Terms

- **Active vehicle:** A vehicle with a live physical representation participating in the current gameplay instance.
- **Persisted vehicle:** A stored vehicle record that may exist without a live physical representation.
- **Broad damage zone:** An external region such as front, rear, left, right, running gear, or mounted equipment. It is not an internal component.
- **Condition state:** `healthy`, `damaged`, `critical`, or `destroyed`.
- **Gameplay instance:** One authoritative simulation of a level or contested zone.
- **Garage context:** A stationary, non-driving context in which external vehicle views and repair schematics are permitted.

## 3. Functional requirements

### 3.1 Vehicle simulation

1. Project Chrono shall own whole-vehicle rigid-body motion, wheel/ground contact, traction, suspension response, terrain contact, and collision response.
2. O3DE shall own world composition, rendering, audio, input, UI, entity lifecycle, gameplay rules, networking integration, persistence integration, and tools.
3. The bridge between O3DE and Chrono shall define units, axes, transform ownership, fixed-step order, interpolation, and lifecycle explicitly.
4. Vehicle configuration shall be data-driven for mass, dimensions, wheel placement, suspension parameters, available drive/brake inputs, and external equipment attachment points.
5. Physics behaviour must be deterministic enough for repeatable automated scenarios on the same supported build and hardware class. Cross-platform bitwise determinism is not required.
6. The simulation shall expose telemetry needed to diagnose frame time, physics-step time, contact count, active-vehicle count, and bridge synchronization faults.

### 3.2 Driving and camera

1. The gameplay driving camera shall remain in a first-person cockpit or driver position.
2. No player-accessible chase, orbit, or third-person driving view shall exist.
3. Steering, throttle, braking, and permitted auxiliary controls shall feed the authoritative vehicle simulation.
4. Camera motion may communicate suspension and impact forces but must apply configurable comfort limits.
5. A safe reset/recovery action may reposition a stuck vehicle only under explicit gameplay rules and must not duplicate it.

### 3.3 Combat

1. Weapons and impacts may apply impulses, broad-zone damage, condition-state transitions, and external-equipment disablement.
2. Combat resolution shall not require penetration simulation, deformable armour, or internal component tracing.
3. The authoritative gameplay layer shall decide damage and destroyed state; visual and audio feedback consumes that state.
4. Friendly-fire, ammunition economy, exact weapon roster, and armour balance remain content decisions.

### 3.4 Damage feedback

Damage is intentionally coarse.

| State | Required gameplay feedback | Permitted external feedback |
|---|---|---|
| Healthy | Baseline handling and instrumentation | Normal vehicle audio |
| Damaged | Reduced handling, braking, acceleration, or speed as authored; oil/fuel-pressure warning indications; changed engine/mechanical audio | No smoke required |
| Critical | Severe performance loss; strong cockpit warnings; distressed audio | Smoke and/or fire may be shown |
| Destroyed | Shutdown, immobilization, or controlled loss of control followed by the scenario outcome | Brief fire, smoke, or destruction effect |

The oil and fuel-pressure indications are authored signals for broad condition states. They do not imply simulated oil or fuel systems.

### 3.5 Repair

1. Repair occurs in a stationary interface, not while actively driving unless a later design explicitly adds a non-visual field-repair action.
2. The interface may show a top, side, or isometric vehicle schematic.
3. The schematic shall identify broad external zones, severity, repair cost or resource requirement, and disabled external equipment.
4. Repair shall update authoritative condition data and restore only the behaviour allowed by the repair result.
5. No repair action shall expose or require internal component diagrams.

### 3.6 Active-vehicle cap

1. The authoritative instance shall maintain the active-vehicle count.
2. Spawn and activation requests shall reserve capacity atomically.
3. Requests exceeding 30 active vehicles shall be rejected or placed in an explicit queue.
4. Despawning a vehicle shall release its reservation exactly once.
5. Persistence records may exceed 30 provided only 30 are physically active in one instance.
6. Metrics and logs shall expose attempted cap violations without leaking private player data.

### 3.7 AI and scenarios

1. AI vehicles shall use the same physical vehicle interface and broad condition model as player vehicles.
2. AI may use navigation aids or simplified decision-making, but it may not bypass authoritative collision and damage rules.
3. Each playable scenario shall define deployment, objective, success, failure, and cleanup states.

### 3.8 Persistence

1. Persistence begins only after the single-player vertical slice is proven.
2. Persisted data shall include stable identifiers, vehicle configuration references, allowed external equipment, broad-zone condition, progression/economy fields, and schema version.
3. Live physics state is transient and shall not be stored as an internal mechanical simulation.
4. Load operations shall validate schema version and reject or migrate unsupported data safely.
5. A persisted record becomes active only after an authoritative capacity reservation succeeds.

### 3.9 Multiplayer

1. Multiplayer simulation shall be server-authoritative for spawning, active count, motion acceptance, combat, damage, destroyed state, rewards, and persistence writes.
2. T5 targets four connected players; T6 targets eight connected players.
3. Clients may predict local input and interpolate remote vehicles but may not authoritatively create vehicles or assign damage.
4. Join, disconnect, reconnect, timeout, and instance cleanup shall preserve the 30-vehicle cap.
5. Late join shall receive a complete current state for active vehicles and scenario state.

## 4. Architecture

### 4.1 Runtime boundaries

- **Chrono adapter:** Creates/destroys vehicle physics objects, accepts control input, advances the fixed step, and exposes transforms/telemetry.
- **O3DE vehicle entity:** Owns presentation, audio, cockpit UI, gameplay identity, external equipment, and adapter binding.
- **Vehicle gameplay state:** Owns condition, broad-zone damage, team/owner, scenario status, and activation reservation.
- **Instance authority:** Owns spawn queue, the active-vehicle registry, scenario rules, and network authority.
- **Persistence service/repository:** Stores versioned records and never treats a stored record as an active physical vehicle.

### 4.2 Update order

1. Collect validated local or server-approved control inputs.
2. Apply authored condition modifiers.
3. Advance Chrono using the configured fixed step.
4. Publish transforms, velocities, contacts, and telemetry to O3DE.
5. Resolve authoritative gameplay events and damage.
6. Replicate authoritative state.
7. Render interpolated presentation and update audio/UI.

### 4.3 Failure behaviour

- A bridge synchronization error shall fail visibly in diagnostics and must not silently duplicate a vehicle.
- A failed activation shall leave the persisted record inactive.
- Invalid network or persistence data shall be rejected with a structured reason.
- Destruction and disconnect cleanup shall be idempotent.

## 5. Data contracts

### 5.1 Vehicle definition

Minimum fields: stable definition ID, display name, mass and dimensions, wheel/suspension configuration, control limits, cockpit camera anchor, external equipment attachment points, broad repair-zone map, and presentation references.

### 5.2 Vehicle instance state

Minimum fields: instance ID, definition ID, owner/team, condition state, broad-zone severities, disabled external equipment, active/inactive state, persistence version, and current gameplay-instance ID when active.

### 5.3 Damage event

Minimum fields: event ID, authoritative timestamp/tick, source, target, magnitude/category, affected broad zone when applicable, impulse data when applicable, and resulting condition transition.

## 6. Performance and quality

1. Fixed-step rate, render target, target hardware, and network latency budget must be recorded in a versioned performance profile before T7 exit.
2. T0 shall collect baseline bridge and vehicle costs; T7 shall validate the approved profile with 30 active vehicles.
3. Profiling must separate Chrono step cost, O3DE frame cost, bridge synchronization, AI, replication, and presentation.
4. Automated scenarios must use fixed seeds and record build ID, configuration, hardware profile, and telemetry.
5. There shall be no active-vehicle leak across repeated spawn/despawn, destruction, disconnect, or level transition.

## 7. Security and integrity

- Treat client spawn, transform, damage, reward, repair, and persistence requests as untrusted.
- Validate identifiers, ranges, ownership, scenario permissions, and active capacity on the authority.
- Rate-limit or reject repeated invalid activation requests.
- Log security-relevant rejection reasons without secrets or sensitive personal data.

## 8. Delivery tranches

| Tranche | Outcome |
|---|---|
| T0 | O3DE/Chrono integration spike |
| T1 | First-person driving prototype |
| T2 | Combat and broad damage states |
| T3 | Single-player vertical slice |
| T4 | Persistent Level 1 game loop |
| T5 | Four-player authoritative multiplayer |
| T6 | Eight-player contested-zone alpha |
| T7 | Thirty-vehicle scale and MVP release candidate |
| T8 | Post-MVP Level 2 expansion |

The tranche test contracts in `test_goals/` define evidence-based entry and exit gates.

## 9. Open decisions

The following were not recoverable from the original draft and must be approved separately:

- Target desktop hardware and performance profile.
- Final input bindings and accessibility options.
- Exact mission, economy, progression, weapon, armour, and roster values.
- Vehicle art pipeline and cockpit content standards.
- Dedicated-server hosting topology and operational budget.
- Distribution platform, account system, moderation, telemetry retention, and anti-cheat scope.
- Save compatibility policy beyond the migrations needed by T8.

No open decision may silently override the permanent non-goals in section 1.4.

