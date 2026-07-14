# ADR 0001: Source engine and immutable dependency pinning

- Status: Accepted
- Date: 2026-07-14
- Scope: Foundation and T0

## Context

T0 requires clean client and headless authoritative-host builds, packaged Project Chrono linkage, debug evidence and a reproducible bootstrap. T7 later requires monolithic release packaging, diagnostics and qualification builds. The repository originally contained no engine or dependency setup.

## Decision

Use O3DE as a project-centric source engine checked out outside this repository. Pin source dependencies by release tag and full Git commit in `dependencies.lock.json`:

- O3DE 26.05.0 / tag `2605.0` / commit `3db6943249d8bd7960b9ed7e9aee310b7668586e`.
- Project Chrono 10.0.0 / annotated tag object `94fc98ae6f7f2bdcaf4ea8d34ee0892409ac9810` / source commit `9faf13dd8f1128dd75ed233a9627027b0422c3f7`.

Use Visual Studio 2022 x64 and CMake 4.2.3 on Windows. CMake is installed from Kitware's SHA-256-verified portable release archive so it does not require elevation or depend on a mutable system `PATH`. The exact installed compiler and Windows SDK versions must be captured in build evidence.

Dependencies are stored in a configurable external directory rather than vendored into the game repository. Bootstrap refuses to use a checkout whose `HEAD` differs from the lock.

Chrono uses a sparse working tree containing its root build files, CMake modules, source tree and Windows build helpers. Large unrelated documentation and runtime-data trees are excluded; only Core and Vehicle will be enabled during configuration.

## Consequences

- Initial setup and storage costs are higher than using only the prebuilt O3DE installer.
- Engine symbols, server targets, build flags and release packaging remain under project control.
- Dependency upgrades require an explicit lock-file and ADR update.
- A clean machine still requires an administrator-controlled Visual Studio installation step.
