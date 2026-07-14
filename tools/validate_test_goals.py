#!/usr/bin/env python3
"""Validate the LDGM tranche test contracts and manifest."""

from __future__ import annotations

import json
import sys
from pathlib import Path

try:
    import jsonschema
except ImportError as exc:
    print("ERROR: install the 'jsonschema' Python package to run validation.")
    raise SystemExit(2) from exc


ROOT = Path(__file__).resolve().parents[1]
TEST_DIR = ROOT / "test_goals"


def load(path: Path):
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def main() -> int:
    errors: list[str] = []
    schema = load(TEST_DIR / "test_goal.schema.json")
    manifest = load(TEST_DIR / "manifest.json")
    validator = jsonschema.Draft202012Validator(schema)
    seen_ids: set[str] = set()
    seen_tranches: set[str] = set()
    total = 0

    for index, entry in enumerate(manifest["tranches"]):
        path = TEST_DIR / entry["file"]
        if not path.is_file():
            errors.append(f"missing tranche file: {entry['file']}")
            continue
        document = load(path)
        schema_errors = sorted(validator.iter_errors(document), key=lambda err: list(err.path))
        for error in schema_errors:
            location = ".".join(str(part) for part in error.path) or "<root>"
            errors.append(f"{entry['file']}:{location}: {error.message}")

        if document.get("tranche_id") != entry["tranche_id"]:
            errors.append(f"{entry['file']}: tranche_id does not match manifest")
        if document.get("dependencies") != entry["dependencies"]:
            errors.append(f"{entry['file']}: dependencies do not match manifest")
        if any(dep not in seen_tranches for dep in entry["dependencies"]):
            errors.append(f"{entry['file']}: dependency appears after dependent tranche")

        for goal in document.get("goals", []):
            goal_id = goal.get("id", "")
            if not goal_id.startswith(f"{entry['tranche_id']}-"):
                errors.append(f"{entry['file']}: goal {goal_id!r} has wrong tranche prefix")
            if goal_id in seen_ids:
                errors.append(f"duplicate goal id: {goal_id}")
            seen_ids.add(goal_id)
            total += 1
        seen_tranches.add(entry["tranche_id"])

    if total != 37:
        errors.append(f"total goals {total} does not match required total 37")
    if [entry["tranche_id"] for entry in manifest["tranches"]] != [f"T{i}" for i in range(9)]:
        errors.append("manifest tranche order must be T0 through T8")
    if manifest.get("mvp_final_tranche") != "T7" or manifest.get("post_mvp_start") != "T8":
        errors.append("manifest MVP/post-MVP boundary must be T7/T8")

    if errors:
        print(f"FAILED: {len(errors)} validation error(s)")
        for error in errors:
            print(f"- {error}")
        return 1

    print("PASS")
    print(f"Schema: JSON Schema draft 2020-12")
    print(f"Tranche documents: {len(manifest['tranches'])}")
    print(f"Unique test goals: {total}")
    print("Order: T0 -> T1 -> T2 -> T3 -> T4 -> T5 -> T6 -> T7 -> T8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
