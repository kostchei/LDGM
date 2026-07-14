# Recovery notes

## Status

This repository was recovered on 2026-07-14 after an earlier ChatGPT conversation claimed to create downloadable artifacts that were not present in ChatGPT Library, local Downloads, or the Codex workspace.

The user subsequently supplied the public share `https://chatgpt.com/share/6a561090-fba4-83ec-9b71-648d758c9557`. Its embedded conversation state contained the exact Python generator originally used for the development package. That 56,501-byte program was recovered as `tools/recovered_original_package_generator.py`; only its two `/mnt/data` output paths were changed to local repository paths. Running it reproduced `development_plan.md`, the nine tranche contracts, manifest, JSON Schema, validation report, and package ZIP.

## Recovered source material

The surviving conversation record established these requirements:

- O3DE is the game engine and Project Chrono is used only for whole-vehicle movement, traction, suspension, terrain contact, and collision.
- No more than 30 vehicles may be concurrently active in a gameplay instance. Persisted vehicles that are not physically instantiated do not count.
- No runtime panel deformation or detailed live damage artwork.
- No simulated internal vehicle mechanics, subsystem diagnostics, oil pumps, fuel systems, or engine internals.
- Driving is first-person only; external views are limited to stationary garage/repair, spectating, and editor contexts.
- Initial damage feedback is handling/braking/acceleration/speed loss, oil/fuel-pressure warnings, and altered mechanical audio.
- Smoke or fire is reserved for critical, destroyed, or heavily damaged states; ordinary damage does not automatically produce smoke.
- Repair uses a simple top, side, or isometric schematic with broad external zones, severity, cost, and disabled external equipment.
- Work is divided into tranches T0 through T8.
- The test package contains 37 uniquely identified goals, a manifest, a JSON Schema, and a validation report.

## Recovery boundary

The public share references the original 64,569-byte upload, `legally_distinct_gorka_morka_prd_eng_spec.md`, with ChatGPT file ID `file_00000000d658720bb13a3af49d44f0f2` and Library ID `libfile_e243a43ea8988191986747a6509cb245`, but it does not include the attachment bytes. The intermediate and v0.4 specification downloads were also ephemeral `/mnt/data` links. The full original specification therefore could not be recovered byte-for-byte.

`legally_distinct_gorka_morka_prd_eng_spec_v0_4.md` is a transparent reconstruction from the surviving requirements and recovered plan/test generator. The development plan, schema, manifest, validation report, and tranche contracts are generated from the original recovered program rather than rewritten from memory.
