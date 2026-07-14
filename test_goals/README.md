# Test goals

Each tranche has one JSON acceptance contract.

## Files

- `test_goal.schema.json` validates the structure.
- `manifest.json` lists the tranche files and dependencies.
- `t0_...json` through `t8_...json` contain entry criteria, non-goals, test environment, test goals, structured assertions, evidence requirements and the exit gate.

## Result handling

The goal files define desired tests; they do not prescribe a specific test runner. A runner should emit a result record containing:

- test goal ID;
- build or commit identifier;
- start and finish timestamps;
- measured assertion values;
- pass, fail, blocked or skipped status;
- evidence paths;
- failure notes.

A tranche passes only when its exit-gate rule is satisfied. A skipped blocker or critical test does not count as a pass.
