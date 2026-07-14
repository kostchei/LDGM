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

## Product constraints at a glance

- First-person driving only.
- No internal mechanical simulation.
- No runtime body or panel deformation.
- Broad condition states communicate damage through vehicle behaviour, cockpit warnings, audio, and smoke/fire only when critical.
- Repair screens may show broad external damage zones on a schematic.
- A hard cap of 30 concurrently active physical vehicles per gameplay instance.
