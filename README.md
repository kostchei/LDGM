# Legally Distinct Gorka Morka

Recovered planning repository for a first-person vehicular combat game built with O3DE and Project Chrono.

## Repository contents

- `legally_distinct_gorka_morka_prd_eng_spec_v0_4.md` — reconstructed product and engineering specification.
- `development_plan.md` — recovered tranche-based implementation plan from integration spike through post-MVP expansion.
- `test_goals/` — recovered JSON test contracts for every tranche, plus the original manifest and JSON Schema.
- `validation_report.txt` — validation result for the current test package.
- `artifacts/ldgm_development_plan_and_test_goals.zip` — clean ZIP recreated by the recovered original generator.
- `tools/validate_test_goals.py` — repeatable package validator.
- `tools/recovered_original_package_generator.py` — generator recovered from the public ChatGPT share and adapted only to use this repository path.
- `RECOVERY_NOTES.md` — provenance and limitations of the reconstruction.

## Validate

From the repository root:

```powershell
python tools/validate_test_goals.py
```

The validator checks every tranche document against the recovered JSON Schema, verifies manifest ordering and dependencies, rejects duplicate goal IDs, and confirms the expected total of 37 goals.

## Windows development bootstrap

The checked-in scripts assume no global engine or CMake registration. From PowerShell at the repository root:

```powershell
.\scripts\bootstrap-machine.ps1
.\scripts\bootstrap-dependencies.ps1
.\scripts\preflight.ps1
.\scripts\build-chrono.ps1
.\scripts\test-chrono.ps1
.\scripts\configure-project.ps1
.\scripts\build-project.ps1
.\scripts\test-vehicle-gem.ps1
.\scripts\process-assets.ps1
.\scripts\test-launchers.ps1
python tools/evaluate_t0_gate.py --configuration profile
```

External source, build and install trees are placed beside the repository under `LDGM-deps` by default and are not committed. The default Windows build uses one MSBuild project and one isolated MSVC translation unit at a time because MSVC 19.44 is unstable on the large O3DE and Chrono source sets when compiler batching is enabled. The launcher smoke script uses the processed PC asset cache, verifies two authoritative vehicles plus two real client presentation entities and an active cockpit camera, and bounds each process to a 15-second probe. The T0 evaluator writes `artifacts/t0/t0_gate_result.json` and returns exit code 2 while required manual evidence keeps the gate blocked.

See [`docs/IMPLEMENTATION_STATUS.md`](docs/IMPLEMENTATION_STATUS.md) for the current stage, verified evidence and next checkpoint.

## Product constraints at a glance

- First-person driving only.
- No internal mechanical simulation.
- No runtime body or panel deformation.
- Broad condition states communicate damage through vehicle behaviour, cockpit warnings, audio, and smoke/fire only when critical.
- Repair screens may show broad external damage zones on a schematic.
- A hard cap of 30 concurrently active physical vehicles per gameplay instance.
