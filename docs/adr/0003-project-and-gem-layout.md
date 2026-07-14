# ADR 0003: Project and Gem layout

- Status: Accepted
- Date: 2026-07-14
- Scope: T0 project structure

## Context

The game requires a small integration surface between O3DE and Project Chrono while gameplay, networking and presentation continue to use normal O3DE components. Splitting every future system into a separate Gem during T0 would create abstractions before their requirements are proven.

## Decision

Start with two code boundaries:

- The generated `LDGM` project Gem owns shared gameplay state and presentation components.
- The Windows-only `LDMChronoVehicle` Gem owns every direct Project Chrono include, link dependency and runtime adapter.

Both Gems expose client and server variants from the same initial implementation. Server-only targets will be split only when authority or secret-bearing persistence code is introduced. Editor targets remain generated but contain no simulation logic.

The project is configured against the locked source engine through ignored `user/project.json` state. No developer-specific absolute engine path is committed.

## Consequences

- Chrono types cannot leak into public gameplay contracts; the bridge API must use LDGM/O3DE-owned data structures.
- The authoritative server and local integration host can use the same bridge during T0.
- Additional Gems require a concrete dependency or packaging reason recorded in a later ADR.
