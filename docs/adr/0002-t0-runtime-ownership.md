# ADR 0002: T0 simulation and state ownership

- Status: Accepted provisionally; T0 evidence must confirm it
- Date: 2026-07-14
- Scope: T0–T7 architecture boundary

## Context

The specification assigns whole-vehicle motion, wheel contact, traction, suspension and terrain collision to Project Chrono. O3DE owns presentation, input, gameplay, networking and tools. Simultaneously driving the vehicle through O3DE PhysX and Chrono would create conflicting authority and unstable synchronization.

## Decision

- The authoritative host owns the fixed-step Chrono simulation.
- The `LDMChronoVehicle` Gem is the only runtime code allowed to directly depend on Chrono.
- O3DE vehicle entities are presentation and gameplay proxies; they do not run a second authoritative vehicle rigid body.
- Validated inputs and broad condition modifiers enter the adapter before each fixed step.
- Chrono transforms, velocities, contacts and timings leave the adapter after each fixed step.
- Clients receive authoritative snapshots and interpolate presentation. Client vehicle prediction is excluded until T5 measurements demonstrate a need.
- Terrain used by Chrono and rendered by O3DE must be derived from the same versioned source data and checksum.
- Active-vehicle capacity is reserved by the authority before a Chrono vehicle is created. Capacity is released idempotently after destruction or deactivation.

The T0 adapter uses a provisional 5 ms fixed step and executes no more than eight catch-up steps per O3DE tick. Whole overdue steps beyond that ceiling are dropped and reported in telemetry rather than allowing an unbounded spiral. The exact axis conversion, production substep policy and snapshot rate remain provisional until measured during T0.

## Consequences

- T0 must prove coordinate conversion, transform synchronization and terrain alignment before gameplay systems are added.
- Vehicle entities require an explicit render-interpolation path.
- O3DE collision can still be used for non-vehicle queries and presentation, but it cannot authoritatively move the vehicle.
- The integration boundary is narrow enough to mock in unit tests and to replace without rewriting gameplay state.
