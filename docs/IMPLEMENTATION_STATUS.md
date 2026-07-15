# LDGM implementation status

Last updated: 2026-07-15

## Current stage

**T0 gate evaluated — blocked on required capture and clean-build evidence**

The T0 implementation now runs two authoritative Chrono vehicles (player and enemy) on one shared terrain fixture and presents both through real client-side Atom mesh entities. A runtime O3DE `CameraComponent` follows the player cockpit, and the O3DE terrain entity is read back and measured against the Chrono terrain definition. Every assertion exercised by the current incremental run passes, including the permanent 30-vehicle registry ceiling, but the formal gate remains blocked until a clean build and the required rendered driving/collision captures are executed and retained as evidence.

## Stage ledger

| Stage | Status | Evidence |
|---|---|---|
| Repository recovery and contract validation | Complete | `validation_report.txt`; 37 unique goals validated |
| Windows toolchain definition and installation | Complete | VS 2022 17.14.37411.7; MSVC 19.44.35228 x64; Windows SDK 10.0.26100.0; CMake 4.2.3 |
| O3DE 26.05.0 source checkout | Complete | Clean checkout at `3db6943249d8bd7960b9ed7e9aee310b7668586e` |
| Project Chrono 10.0.0 source checkout | Complete | Clean sparse checkout at `9faf13dd8f1128dd75ed233a9627027b0422c3f7` |
| O3DE project configuration | Complete | Visual Studio 2022 generator at `build/windows`; generated files ignored |
| Empty O3DE client/server build | Complete | Profile `LDGM.GameLauncher.exe` and `LDGM.ServerLauncher.exe` linked successfully |
| Empty-project asset processing | Complete | 1,496 assets processed in the initial pass, 0 failed; incremental pass completed with 0 warnings/errors |
| Project Chrono build | Complete | Eigen 5.0.0 plus Chrono Core/Vehicle and required model export closure installed as static RelWithDebInfo libraries |
| Standalone Chrono consumer smoke | Complete | Installed-package `find_package(Chrono COMPONENTS Vehicle)`; 120 dynamics steps; 1/1 CTest passed |
| O3DE project and Gem scaffolding | Complete | Minimal project Gem plus Windows-only `LDMChronoVehicle` Gem |
| O3DE–Chrono Gem linkage | Complete | Private imported targets; Profile client/server link; both 15-second launcher probes log the lifecycle smoke result |
| T0 fixed-step and active registry primitives | Complete | 6 GoogleTests pass; bounded catch-up and dropped-time reporting; 30 accepted, 31st rejected, release idempotent |
| T0 authoritative runtime adapter | Complete | Server-only 5 ms fixed stepping; eight-step catch-up bound; public plain-data telemetry and capacity API; both launcher probes logged the correct role (client-linkage / authoritative) |
| T0 coordinate conversion boundary | Complete | Shared right-handed Z-up frame (Chrono default ISO); component-wise pose/vector/quaternion conversion in `Simulation/CoordinateConversion`; GoogleTests include cross-engine rotation agreement |
| T0 vehicle and shared-terrain fixture | Complete | Two programmatic HMMWVs (player and enemy) advance on one authoritative `RigidTerrain`; the runtime registry reports 2 active vehicles |
| T0 client presentation | Complete | Two game-context entities own real Atom `MeshComponent` presentation and consume per-vehicle 20 Hz snapshots; 451 snapshots received with 0 rejects in the recorded probe |
| T0 terrain alignment | Automated assertions pass | Runtime readback of the O3DE ground transform measures 0.000 m origin/surface/extent error against the Chrono terrain definition |
| T0 cockpit camera | Automated assertions pass | A real runtime `CameraComponent` remains active at the player cockpit; 0 invalid frames and 0.000025 m maximum attachment error |
| T0 integration implementation | Complete for two-vehicle spike | Capacity and snapshot structures retain the permanent maximum of 30; 31st registry reservation is rejected by test |
| T0 acceptance gate | Blocked | Machine-readable result: `artifacts/t0/t0_gate_result.json`; clean-build, non-null-RHI video, and collision-input camera evidence remain missing |
| T1–T7 | Not started | Must follow preceding tranche gates |

## Completed work

- Inspected the recovered product specification, development plan, test schema, manifest and all T0–T7 acceptance procedures.
- Re-ran `tools/validate_test_goals.py`; all 37 recovered test goals pass schema and manifest validation.
- Confirmed the repository initially contained documentation and test contracts only, with no O3DE project or runtime code.
- Selected a project-centric O3DE source-engine workflow for reproducible client, server and release builds.
- Resolved immutable release commits for O3DE 26.05.0 and Project Chrono 10.0.0.
- Defined the required Visual Studio 2022 workloads and the pinned CMake version.
- Installed Visual Studio Community 2022 with the checked-in C++ workloads.
- Replaced the elevation-dependent CMake MSI path with Kitware's SHA-256-verified portable CMake 4.2.3 archive.
- Passed the Windows preflight with Visual Studio 17.14.37411.7, MSVC 19.44.35228 x64, Windows SDK 10.0.26100.0 and CMake 4.2.3.
- Materialized and verified the locked O3DE 26.05.0 source checkout at commit `3db6943249d8bd7960b9ed7e9aee310b7668586e`.
- Narrowed the Chrono checkout to build files, source and Windows helpers after the initial partial-clone checkout attempted to fetch thousands of unrelated data blobs without progress.
- Correctly distinguished Chrono's annotated tag object from its dereferenced source commit; the dependency verifier rejected the tag-object mismatch before the lock was corrected.
- Re-ran the dependency bootstrap successfully and verified both external worktrees are clean and exactly match their locked commits.
- Bootstrapped O3DE's pinned Python 3.10.13 environment after one transient CDN DNS failure.
- Generated the O3DE MinimalProject scaffold and the dedicated `LDMChronoVehicle` code Gem using O3DE's own templates.
- Restricted the project and custom Gems to O3DE 2.6.0 compatibility and replaced all generated placeholder metadata.
- Configured the project successfully with CMake 4.2.3, Visual Studio 2022, MSVC 19.44.35228 and Windows SDK 10.0.26100.0.
- Built the Profile GameLauncher and ServerLauncher targets successfully.
- Diagnosed MSVC `CL.exe` access violations (`0xC0000005`) during large O3DE unity builds at unrestricted, eight-job and four-job MSBuild parallelism, then found a second compiler-worker layer enabled by `/MP`.
- Set the checked-in Windows build defaults to one MSBuild project and one MSVC compiler worker; the previously failing launcher, tools and EMotionFX targets pass with both limits enforced.
- Probed both launchers: the GameLauncher remained running for the full 15-second bounded smoke interval, while the ServerLauncher exited cleanly after reporting unprocessed shader/font assets and the absent template level.
- Built `AssetProcessorBatch.exe` and added a reproducible PC asset-processing script with explicit project and source-engine paths, avoiding any reliance on global O3DE registration.
- Completed the initial asset pass with 1,496 successful assets and 0 failed assets, then verified the scripted incremental pass with 0 warnings and 0 errors.
- Repeated the headless startup probes after asset processing; both GameLauncher and ServerLauncher remained running for the full 15-second interval.
- Locked Eigen 5.0.0 at commit `549bf8c75b6aae071cde2f28aa48f16ee3ae60b0`, bootstrapped it beside the engine sources and installed its headers without optional tests, BLAS, LAPACK, demos or documentation probes.
- Built Project Chrono 10.0.0 as static RelWithDebInfo libraries using the dynamic MSVC runtime required by O3DE.
- Added a checked-in MSBuild override that bounds compiler memory by giving each translation unit a distinct command line when single-compiler mode is selected; this avoids MSVC 19.44's unstable long-lived compiler batching without editing either upstream checkout.
- Scoped an MSVC 19.44 optimizer workaround to Chrono's `ChElementHexaANCF_3813.cpp`; all other Chrono sources retain the requested RelWithDebInfo optimization.
- Built and installed `Chrono_core`, `Chrono_vehicle`, and Chrono 10's required `ChronoModels_robot` and `ChronoModels_vehicle` export closure. Chrono's installed `Vehicle` component requires both model targets even when a consumer only uses base Vehicle symbols.
- Normalized the configured Eigen path to CMake-style forward slashes after the standalone consumer caught invalid Windows escape sequences in the generated `ChronoConfig.cmake`.
- Added and passed a standalone installed-package test that links the supported Vehicle target set, selects the Vehicle Y-up frame, advances a Core NSC system for 120 fixed steps, and validates elapsed simulation time and falling-body height.
- Disabled optional Thrust discovery and cleared stale cached include paths so the installed Chrono package exports no CUDA toolkit or machine-local NVIDIA dependency.
- Configured O3DE with the installed Chrono package and promoted its directory-scoped imported targets for O3DE's delayed project-root link pass.
- Kept every Chrono include and type inside `LDMChronoVehicle` implementation files; the system component header exposes only an opaque private state.
- Added a lifecycle smoke that selects the Vehicle Y-up frame, creates an NSC system, advances one 5 ms step and queues its validated result until the engine log is available.
- Added a processed runtime bootstrap override so preprocessed client/server launches do not require a live Asset Processor connection.
- Rebuilt the Profile GameLauncher and ServerLauncher successfully, then passed a reusable bounded runtime test in which both roles remained alive for 15 seconds and logged `Chrono Core/Vehicle lifecycle smoke passed`.
- Added a fixed-capacity active-vehicle registry with stable insertion order, invalid/duplicate detection, a hard maximum of 30 and idempotent release.
- Added a deterministic fixed-step accumulator with bounded catch-up, explicit dropped-time telemetry, fractional remainder preservation and invalid elapsed-time rejection.
- Enabled the generated Gem's Windows client tests and added 6 passing GoogleTests for the registry and clock contracts.
- Corrected the compiler-isolation definition so hyphenated sources such as `gtest-all.cc` receive a unique command line without creating an invalid preprocessor macro name.
- Added a Chrono-free public Gem API for authoritative capacity reservation and simulation telemetry.
- Restricted continuous Chrono stepping and the active-vehicle registry to the server launcher; the game client performs only the lifecycle linkage probe.
- Integrated the fixed-step clock at a provisional 5 ms interval with an eight-step catch-up ceiling and wall-time, step-count, accumulator and dropped-time telemetry.
- Replaced the silent non-authoritative default in launcher role detection with a fatal error when neither `sv_isDedicated` nor the build-target settings key is available.
- Verified the authoritative adapter: 6/6 Gem GoogleTests pass, both launchers rebuilt with embedded `/Z7` debug information, and both bounded runtime probes logged the lifecycle smoke with the correct role.
- Observed two isolated `cl.exe` crashes (`0xC000001D`, `0xC0000005`) during the `/Z7` framework recompile; retries completed without recurrence and no crash repeated at the same translation unit.
- Replaced the provisional Chrono Y-up world frame with Chrono's default ISO Z-up frame so both engines share a right-handed Z-up convention; the adapter now asserts the ISO frame at startup (ADR 0002 amendment).
- Added the private `Simulation/CoordinateConversion` boundary for vectors, quaternions and rigid poses with finite/unit-length/unit-scale enforcement, plus 6 GoogleTests covering round trips, cross-engine rotation agreement and Chrono-height-to-Z mapping.
- Added the versioned shared-terrain source (`TerrainFixtureConfig`) with a CRC32 fingerprint logged by every consumer, and a fully programmatic HMMWV fixture (TMeasy tires, simple-map powertrain, no data files) driven by a deterministic settle/ramp/drive schedule.
- Reserved the fixture vehicle through the active registry before Chrono vehicle creation, integrated fixture stepping into the server fixed-step loop, and published vehicle pose, speed and terrain checksum through the Chrono-free telemetry.
- Verified 14/14 Gem GoogleTests (2.5 s HMMWV simulation runs in ~65 ms) and captured per-second `T0 transform trace` lines in the server probe log: the vehicle settles from 1.0 m to its 0.59 m ride height and accelerates to 17.8 m/s while the server sustains real-time 5 ms stepping.

- Added the public `GetActiveVehiclePose` API and the `VehicleProxyComponent` presentation proxy (ticks after the physics-slot simulation, never a second rigid body), spawned programmatically as the `T0VehicleProxy` entity on the authoritative host.
- Extended the launcher probe to require `role=authoritative`, `T0 transform trace` and `T0 proxy trace` in the server log; captured proxy traces timestamped in simulation time with 0.000 m position error and 0.040° maximum rotation error.

- Added the `CockpitCameraComponent` on the proxy entity: rigid driver-eye offset, per-frame NaN and attachment-error telemetry, and a one-time input-action inventory proving no external driving camera action exists; the launcher probe now also requires the camera trace and input inventory in the server log.

- Added the T0 spike snapshot channel (connected UDP over loopback, fixed ports, versioned trivially-copyable packet with finite-value validation): the authoritative host publishes 20 Hz chassis poses and the client applies them to its own proxy entity.
- Rewrote the launcher probe to run both roles concurrently: the server must log its simulation, proxy, camera and inventory evidence while the client logs received snapshot traces (218 received, 0 rejected in the passing probe).
- Replaced the single authoritative fixture with player and enemy HMMWVs sharing one Chrono terrain instance; both are reserved through the same 30-slot active-vehicle registry and published independently.
- Changed snapshot draining to retain the newest packet per vehicle, using a fixed-capacity batch bounded by `MaxActiveVehicles`.
- Replaced transform-only client proxies with game-context entities that own Atom mesh components, added a scaled runtime ground presentation, and activated a real O3DE camera entity rigidly attached to the player cockpit.
- Added runtime O3DE terrain-transform readback and alignment telemetry. The passing profile probe measured 0.000 m terrain origin/surface/extent error, 0.000 m proxy position error, and 0.039565° maximum proxy orientation error.
- Added `tools/evaluate_t0_gate.py` and generated the formal machine-readable T0 result. Automated physics, camera, capacity, timing, unit-test, and smoke-test assertions pass; the overall gate remains blocked on explicitly listed evidence gaps.

## Next checkpoint

1. Run a fresh clean dependency/bootstrap and Profile client/server build while retaining the build logs.
2. Run the client with a non-null RHI and capture first-person driving video.
3. Add or drive a collision-input fixture, capture camera stability telemetry/video, then rerun `python tools/evaluate_t0_gate.py --configuration profile`.

## Working rules

- Each meaningful, verified milestone is committed separately.
- This document is updated whenever the active stage changes or a gate is evaluated.
- Generated engine sources, build trees, caches, telemetry and test evidence are not committed.
- A later tranche does not begin until the preceding blocker and critical goals pass.
