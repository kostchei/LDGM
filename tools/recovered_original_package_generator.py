from pathlib import Path
import json
import textwrap
import zipfile
from datetime import date

root = Path(__file__).resolve().parents[2] / "ldgm_development_plan"
tests_dir = root / "test_goals"
root.mkdir(parents=True, exist_ok=True)
tests_dir.mkdir(parents=True, exist_ok=True)

source_spec = "legally_distinct_gorka_morka_prd_eng_spec_v0_4.md"

tranches = [
    {
        "id": "T0",
        "slug": "integration_spike",
        "title": "Engine and Vehicle Integration Spike",
        "phase": "foundation",
        "mvp": True,
        "objective": "Prove that the pinned O3DE and Project Chrono versions can build, run and exchange authoritative whole-vehicle state before gameplay systems are added.",
        "deliverables": [
            "Pinned O3DE 26.05.0 project and reproducible build instructions.",
            "Project Chrono 10.0.0 packaged and linked through a minimal LDMChronoVehicle Gem.",
            "One generic vehicle driving on terrain shared by O3DE and Chrono.",
            "Stable first-person cockpit camera with no player-accessible third-person driving view.",
            "Headless authoritative host sending snapshots to one client.",
            "Active-vehicle registry with a hard maximum of 30 and deterministic overflow rejection.",
            "Basic timing, simulation and network telemetry."
        ],
        "exclusions": [
            "Custom starter technical art.",
            "Weapons, missions, persistence and economy.",
            "Detailed vehicle assembly.",
            "Client prediction."
        ],
        "dependencies": [],
        "exit": "Proceed only when build packaging, coordinate conversion, terrain alignment, transform synchronization, first-person camera stability and server simulation have no unresolved architecture blocker."
    },
    {
        "id": "T1",
        "slug": "driving_prototype",
        "title": "First-Person Driving Prototype",
        "phase": "prototype",
        "mvp": True,
        "objective": "Deliver the basic driving experience with the starter technical, the required surfaces and usable cockpit controls.",
        "deliverables": [
            "Starter technical chassis and cockpit.",
            "Keyboard and gamepad driving controls; input abstraction ready for HOTAS mapping.",
            "Hard road, broken road, gravel, firm sand and deep-sand surface profiles.",
            "First-person head-look and Perception wide/far modes.",
            "Fuel consumption and basic vehicle condition values.",
            "Physics regression range and golden telemetry capture."
        ],
        "exclusions": [
            "Weapons and combat.",
            "Damage feedback beyond developer controls.",
            "Mission flow and persistence.",
            "External player driving camera."
        ],
        "dependencies": ["T0"],
        "exit": "The starter technical is controllable, stable and measurably distinct across the required terrain surfaces, with repeatable physics telemetry inside agreed tolerances."
    },
    {
        "id": "T2",
        "slug": "combat_damage",
        "title": "Combat and Broad Damage States",
        "phase": "prototype",
        "mvp": True,
        "objective": "Add fixed-forward combat and inexpensive damage feedback without internal-mechanics simulation or detailed damage rendering.",
        "deliverables": [
            "Two independently tracked fixed rifles with 50 rounds each and cabin sights.",
            "Server-authoritative projectile, hit and ammunition handling.",
            "Intact, damaged, critical and destroyed vehicle states.",
            "Authored handling, braking, acceleration or power loss for damaged states.",
            "Cockpit gauge/warning changes such as oil or fuel pressure indicators.",
            "Altered engine or mechanical audio for damage feedback.",
            "Smoke or fire only for critical, heavily damaged or destroyed vehicles.",
            "Repair-zone records and a simple repair schematic."
        ],
        "exclusions": [
            "Panel deformation, damaged meshes, impact decals and detailed damage textures.",
            "Individually simulated engines, transmissions, oil systems, fuel lines or other internals.",
            "Complex armour penetration simulation.",
            "Third-person combat camera."
        ],
        "dependencies": ["T1"],
        "exit": "A player can shoot, be hit and correctly infer ordinary damage from vehicle behaviour, instruments or audio; critical vehicles may smoke or burn; repair data identifies broad damaged zones."
    },
    {
        "id": "T3",
        "slug": "vertical_slice",
        "title": "Single-Player Vertical Slice",
        "phase": "vertical_slice",
        "mvp": True,
        "objective": "Prove the complete game loop with one package-delivery mission before investing in broad persistence or multiplayer.",
        "deliverables": [
            "Bartertown mission board, garage and departure flow.",
            "One package-delivery mission across road, gravel and sand.",
            "Mission success, failure and abandonment outcomes.",
            "Return, repair, refuel and rearm flow.",
            "One salvaged external part that can be installed and changes appearance or broad handling.",
            "Basic route risk, fuel estimate and cargo mass/fragility presentation.",
            "Playable build suitable for structured user testing."
        ],
        "exclusions": [
            "All four mission templates.",
            "PostgreSQL persistence.",
            "PvP, auctions and Despot Depot services.",
            "More than one player."
        ],
        "dependencies": ["T2"],
        "exit": "A new tester can launch, accept the mission, drive, fight, take understandable damage, complete or fail, return and modify the vehicle without developer intervention."
    },
    {
        "id": "T4",
        "slug": "persistent_level_one",
        "title": "Persistent Level 1 Game Loop",
        "phase": "mvp_build",
        "mvp": True,
        "objective": "Turn the vertical slice into a durable solo game loop with all Level 1 mission types and authoritative progression.",
        "deliverables": [
            "PostgreSQL persistence with SQLite test fixtures.",
            "Package delivery, camp raid, moving-target interception and salvage recovery missions.",
            "Character stats, permanent Fame and bounded Display Fame.",
            "Inventory, salvage, shops, fuel, ammunition and external-part installation.",
            "Transactional rewards, recovery and repair.",
            "Disconnect/reconnect without duplication or reward replay.",
            "Data-authored mission definitions and schema validation."
        ],
        "exclusions": [
            "Player-versus-player zones.",
            "Auction settlement between players.",
            "More than one connected player as a release requirement.",
            "Level 2 progression."
        ],
        "dependencies": ["T3"],
        "exit": "A player can complete repeated Level 1 sessions across restarts without losing or duplicating characters, vehicles, inventory, mission state or rewards."
    },
    {
        "id": "T5",
        "slug": "authoritative_multiplayer",
        "title": "Authoritative Cooperative Multiplayer",
        "phase": "mvp_build",
        "mvp": True,
        "objective": "Add reliable server-authoritative play for up to four players while preserving the first-person vehicle feel and persistent state.",
        "deliverables": [
            "Dedicated-server join, leave and reconnect flow.",
            "Up to four players driving and firing in one active zone.",
            "Interest-managed vehicle snapshots and interpolation.",
            "Authoritative ammunition, hits, damage, mission state and rewards.",
            "Latency, loss and snapshot-reordering test harness.",
            "Collision correction that does not destabilize the first-person camera.",
            "Security checks for impossible transforms, fire rates and inventory changes."
        ],
        "exclusions": [
            "Eight-player target.",
            "PvP band and contested-zone rules.",
            "Local shadow prediction unless interpolation proves insufficient.",
            "Thirty-vehicle performance qualification."
        ],
        "dependencies": ["T4"],
        "exit": "Four players can complete Level 1 missions under representative network impairment without state duplication, authority violations or unusable first-person correction."
    },
    {
        "id": "T6",
        "slug": "contested_zone_alpha",
        "title": "Contested Zone Alpha",
        "phase": "alpha",
        "mvp": True,
        "objective": "Add the territorial risk layer, eight-player target and dangerous service hub.",
        "deliverables": [
            "Clearly bounded contested PvP band.",
            "Long safe route and shorter contested route.",
            "Despot Depot service point.",
            "Up to eight connected players.",
            "PvP damage, defeat and recovery policy implementation.",
            "Basic auction listing and authoritative settlement.",
            "Expanded armour coverage and repair-zone data.",
            "Prediction/correction absorber only if required by test results."
        ],
        "exclusions": [
            "Large faction wars.",
            "Full-loot assumptions unless separately approved.",
            "Level 2 specialist equipment.",
            "More than 30 active vehicles."
        ],
        "dependencies": ["T5"],
        "exit": "Players can knowingly enter and leave the contested band, fight under server authority, use the depot and settle auction transactions without duplication or ambiguous PvP rules."
    },
    {
        "id": "T7",
        "slug": "scale_release_candidate",
        "title": "Thirty-Vehicle Scale and MVP Release Candidate",
        "phase": "release_candidate",
        "mvp": True,
        "objective": "Qualify the permanent 30-active-vehicle ceiling, performance budgets and operational reliability for the MVP.",
        "deliverables": [
            "Thirty simultaneous active vehicles across player, AI and active parked categories.",
            "Hard rejection or queueing of the thirty-first active vehicle.",
            "Simulation LOD transitions that preserve state.",
            "Client and server profiling against defined budgets.",
            "Eight-player combat-and-driving load scenario.",
            "Long-running reliability and reconnect tests.",
            "Release CI, migrations, telemetry and support diagnostics.",
            "Final MVP content and regression pass."
        ],
        "exclusions": [
            "Raising the vehicle cap to solve performance problems.",
            "Deformable terrain or vehicle panels.",
            "Level 2 content.",
            "Seamless MMO architecture."
        ],
        "dependencies": ["T6"],
        "exit": "The release candidate meets the hard population rule, functional regression gates and agreed performance/reliability thresholds on the defined test machines."
    },
    {
        "id": "T8",
        "slug": "level_two_expansion",
        "title": "Level 2 Expansion",
        "phase": "post_mvp",
        "mvp": False,
        "objective": "Add Fame-gated progression and specialist external equipment without expanding into internal vehicle simulation.",
        "deliverables": [
            "Fame 20 Level 2 gate.",
            "Multi-stage mission templates.",
            "Specialist external parts and limited advanced fields.",
            "Economy, reward and progression rebalance from MVP telemetry.",
            "Additional vehicles and environments only within the 30-active-vehicle cap.",
            "Updated test corpus and compatibility migrations."
        ],
        "exclusions": [
            "Internal component simulation.",
            "Aircraft, boats or walkers.",
            "Globally deformable terrain.",
            "Unbounded exotic equipment systems."
        ],
        "dependencies": ["T7"],
        "exit": "Level 2 adds meaningful choices and mission complexity while preserving all permanent scope limits and MVP reliability gates."
    }
]

plan = f"""---
title: "Legally Distinct Gorka Morka — Development Plan"
version: "0.1"
date: "{date.today().isoformat()}"
source_specification: "{source_spec}"
status: "Proposed execution plan"
---

# Purpose

This plan converts the v0.4 product and engineering specification into sequential tranches. Each tranche ends in a demonstrable product state or a decisive technical result. A later tranche should not begin until the preceding exit gate passes, unless the work is explicitly isolated and cannot conceal a failed dependency.

The plan deliberately delays broad persistence, multiplayer and scale work until the single-player driving and mission loop is proven. The MVP ends at **T7**. **T8** is a post-MVP expansion.

# Permanent constraints carried into every tranche

- First-person driving and vehicle combat only; no player-accessible third-person driving camera.
- Maximum of 30 simultaneously active vehicles per gameplay instance.
- Project Chrono is used for whole-vehicle motion, traction, suspension response, collision and terrain contact only.
- No internal vehicle-mechanics simulation.
- No runtime panel deformation or detailed live damage rendering.
- Ordinary damage is communicated through handling/performance loss, cockpit indicators and audio.
- Smoke or fire is reserved for heavily damaged, critical or destroyed vehicles.
- Detailed damage location exists only on the repair schematic.
- Server authority owns combat, mission, inventory, reward and persistent outcomes.

# Delivery rules

1. **Playable evidence beats framework completion.** Each tranche must produce a build or headless test that demonstrates its claim.
2. **Do not build the next abstraction early.** For example, do not build auctions before the persistent inventory transaction model passes.
3. **Every exit gate is binary.** Known failures are either fixed, explicitly removed from scope or accepted in a written decision record.
4. **Automate stable checks.** Build, schema, persistence, authority and scale checks belong in CI once they stop changing daily.
5. **Use golden telemetry, not bit-identical physics.** Vehicle regression tests compare measured ranges and tolerances.
6. **The 30-vehicle cap is a rule, not a performance aspiration.** Overflow is rejected or queued; the cap is never silently exceeded.
7. **Test documents are source-controlled.** The JSON files in `test_goals/` define the current acceptance contract for each tranche.

# Tranche map

| Tranche | Outcome | MVP? | Depends on |
|---|---|---:|---|
"""

for t in tranches:
    plan += f"| {t['id']} | {t['title']} | {'Yes' if t['mvp'] else 'Post-MVP'} | {', '.join(t['dependencies']) if t['dependencies'] else 'None'} |\n"

plan += "\n# Detailed tranches\n\n"

for t in tranches:
    plan += f"""## {t['id']} — {t['title']}

**Phase:** {t['phase'].replace('_', ' ').title()}  
**Objective:** {t['objective']}

### Deliverables

"""
    for item in t["deliverables"]:
        plan += f"- {item}\n"
    plan += "\n### Explicitly excluded\n\n"
    for item in t["exclusions"]:
        plan += f"- {item}\n"
    plan += f"\n### Exit gate\n\n{t['exit']}\n\n"
    plan += f"**Test contract:** `test_goals/{t['id'].lower()}_{t['slug']}.json`\n\n"

plan += """# Cross-tranche quality gates

## Build and content

- Pinned engine and third-party versions.
- Reproducible developer setup.
- JSON schema validation for data-authored content.
- No missing simulation-relevant assets in release builds.
- Third-party notices generated in CI.

## Physics and first-person experience

- Physics regressions use tolerances for acceleration, braking, turning, terrain speed, rollover and collision recovery.
- Camera correction and shake tests are performed from the cockpit.
- No player command or input binding can enter an external driving camera.
- Damage feedback remains broad and authored; tests must not depend on internal engine-component state.

## Authority and persistence

- Clients send inputs and requests, not authoritative transforms, ammunition counts, rewards or prices.
- Reward and auction operations use idempotency keys.
- Vehicle, component and inventory writes use version checks or equivalent locking.
- Disconnect, retry and duplicate-message tests must not create additional items, currency, vehicles or mission rewards.

## Population and performance

- The active registry includes every LOD 0–2 player, AI and physically active parked vehicle.
- An attempted thirty-first active vehicle is rejected or placed in an explicit queue.
- The cap is tested in unit, integration and load suites.
- Performance tuning may reduce distant simulation fidelity but may not increase the cap.

# Change control

A tranche test may change only when one of the following is recorded:

- The product requirement changed.
- The test was invalid or could not measure the intended behaviour.
- The implementation approach changed without weakening the user-facing requirement.
- A threshold was provisional and profiling produced a better justified value.

A failing test is not removed solely to allow the tranche to pass.

# Produced documents

- `development_plan.md` — this plan.
- `test_goals/test_goal.schema.json` — validation schema.
- `test_goals/manifest.json` — tranche and file index.
- One JSON acceptance contract for each tranche, T0 through T8.
"""

(root / "development_plan.md").write_text(plan, encoding="utf-8")

schema = {
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "$id": "https://example.invalid/ldgm/test_goal.schema.json",
    "title": "LDGM Tranche Test Goal",
    "type": "object",
    "additionalProperties": False,
    "required": [
        "schema_version", "source_specification", "tranche_id", "title", "phase",
        "mvp", "purpose", "dependencies", "entry_criteria", "non_goals",
        "test_environment", "goals", "exit_gate"
    ],
    "properties": {
        "schema_version": {"const": "1.0"},
        "source_specification": {"type": "string", "minLength": 1},
        "tranche_id": {"type": "string", "pattern": "^T[0-9]+$"},
        "title": {"type": "string", "minLength": 1},
        "phase": {"type": "string", "minLength": 1},
        "mvp": {"type": "boolean"},
        "purpose": {"type": "string", "minLength": 1},
        "dependencies": {"type": "array", "items": {"type": "string", "pattern": "^T[0-9]+$"}, "uniqueItems": True},
        "entry_criteria": {"type": "array", "items": {"type": "string", "minLength": 1}, "minItems": 1},
        "non_goals": {"type": "array", "items": {"type": "string", "minLength": 1}},
        "test_environment": {
            "type": "object",
            "additionalProperties": False,
            "required": ["build_configuration", "topology", "required_assets"],
            "properties": {
                "build_configuration": {"type": "array", "items": {"type": "string"}, "minItems": 1},
                "topology": {"type": "array", "items": {"type": "string"}, "minItems": 1},
                "required_assets": {"type": "array", "items": {"type": "string"}},
                "network_profiles": {"type": "array", "items": {"type": "string"}},
                "notes": {"type": "array", "items": {"type": "string"}}
            }
        },
        "goals": {
            "type": "array",
            "minItems": 1,
            "items": {
                "type": "object",
                "additionalProperties": False,
                "required": [
                    "id", "category", "priority", "statement", "method",
                    "procedure", "assertions", "evidence"
                ],
                "properties": {
                    "id": {"type": "string", "pattern": "^T[0-9]+-[A-Z0-9]+-[0-9]{3}$"},
                    "category": {"enum": ["build", "physics", "camera", "input", "combat", "damage", "mission", "ui", "persistence", "network", "security", "performance", "content", "economy", "reliability"]},
                    "priority": {"enum": ["blocker", "critical", "major", "minor"]},
                    "statement": {"type": "string", "minLength": 1},
                    "method": {"enum": ["automated", "instrumented", "manual", "mixed"]},
                    "procedure": {"type": "array", "items": {"type": "string", "minLength": 1}, "minItems": 1},
                    "assertions": {
                        "type": "array",
                        "minItems": 1,
                        "items": {
                            "type": "object",
                            "additionalProperties": False,
                            "required": ["metric", "operator", "target"],
                            "properties": {
                                "metric": {"type": "string", "minLength": 1},
                                "operator": {"enum": ["==", "!=", "<", "<=", ">", ">=", "contains", "not_contains", "in_range", "all_pass"]},
                                "target": {},
                                "unit": {"type": "string"},
                                "tolerance": {"type": ["number", "null"]},
                                "notes": {"type": "string"}
                            }
                        }
                    },
                    "evidence": {"type": "array", "items": {"type": "string", "minLength": 1}, "minItems": 1}
                }
            }
        },
        "exit_gate": {
            "type": "object",
            "additionalProperties": False,
            "required": ["rule", "required_priorities", "required_evidence", "decision_owner"],
            "properties": {
                "rule": {"enum": ["all_required_pass", "all_blocker_and_critical_pass"]},
                "required_priorities": {"type": "array", "items": {"enum": ["blocker", "critical", "major", "minor"]}, "minItems": 1},
                "required_evidence": {"type": "array", "items": {"type": "string", "minLength": 1}, "minItems": 1},
                "decision_owner": {"type": "string", "minLength": 1},
                "notes": {"type": "array", "items": {"type": "string"}}
            }
        }
    }
}
(tests_dir / "test_goal.schema.json").write_text(json.dumps(schema, indent=2) + "\n", encoding="utf-8")

def goal(gid, category, priority, statement, method, procedure, assertions, evidence):
    return {
        "id": gid,
        "category": category,
        "priority": priority,
        "statement": statement,
        "method": method,
        "procedure": procedure,
        "assertions": assertions,
        "evidence": evidence
    }

test_docs = {}

test_docs["T0"] = [
    goal("T0-BUILD-001", "build", "blocker",
         "The pinned O3DE client and authoritative host build reproducibly with the packaged Chrono dependency.",
         "automated",
         ["Run a clean dependency/bootstrap step.", "Build the Windows profile client.", "Build at least one headless authoritative host target.", "Run the executable smoke tests."],
         [
             {"metric": "clean_client_build_exit_code", "operator": "==", "target": 0},
             {"metric": "clean_server_build_exit_code", "operator": "==", "target": 0},
             {"metric": "chrono_link_smoke_test", "operator": "==", "target": "pass"}
         ],
         ["CI build logs", "dependency lock or package manifest", "smoke-test report"]),
    goal("T0-PHYS-002", "physics", "blocker",
         "O3DE and Chrono agree on vehicle position, orientation and wheel transforms on shared terrain.",
         "instrumented",
         ["Spawn one generic vehicle at a known transform.", "Drive a fixed input sequence.", "Record Chrono and rendered proxy transforms.", "Compare coordinate and terrain alignment."],
         [
             {"metric": "terrain_origin_error_m", "operator": "<=", "target": 0.02, "unit": "m"},
             {"metric": "render_proxy_position_error_m", "operator": "<=", "target": 0.05, "unit": "m"},
             {"metric": "orientation_error_deg", "operator": "<=", "target": 1.0, "unit": "degrees"}
         ],
         ["transform trace", "terrain checksum report", "captured test video"]),
    goal("T0-CAM-003", "camera", "blocker",
         "The cockpit camera remains stable and no player-accessible third-person driving view exists.",
         "mixed",
         ["Drive over the integration terrain.", "Apply normal steering, braking and collision inputs.", "Enumerate runtime camera actions and bindings.", "Attempt to enter any external view while driving."],
         [
             {"metric": "camera_nan_or_invalid_frames", "operator": "==", "target": 0},
             {"metric": "player_external_driving_camera_actions", "operator": "==", "target": 0},
             {"metric": "camera_remains_attached_to_cockpit", "operator": "==", "target": True}
         ],
         ["input-action inventory", "camera telemetry", "test video"]),
    goal("T0-PERF-004", "performance", "critical",
         "The authoritative simulation publishes timing telemetry and enforces the permanent active-vehicle ceiling.",
         "automated",
         ["Create the active-vehicle registry.", "Spawn 30 active test vehicles.", "Attempt one additional active spawn.", "Capture simulation timing and registry events."],
         [
             {"metric": "peak_active_vehicle_count", "operator": "==", "target": 30},
             {"metric": "thirty_first_spawn_result", "operator": "in_range", "target": ["rejected", "queued"]},
             {"metric": "simulation_timing_metrics_present", "operator": "==", "target": True}
         ],
         ["registry event log", "telemetry output", "automated test result"])
]

test_docs["T1"] = [
    goal("T1-DRIVE-001", "physics", "blocker",
         "The starter technical drives predictably on all required MVP surface classes.",
         "instrumented",
         ["Run the same acceleration, braking and constant-steer sequence on hard road, broken road, gravel, firm sand and deep sand.", "Capture speed, slip, stopping distance and stability telemetry.", "Store golden ranges."],
         [
             {"metric": "required_surface_profiles_tested", "operator": "==", "target": 5},
             {"metric": "physics_run_has_nan", "operator": "==", "target": False},
             {"metric": "surface_results_are_distinguishable", "operator": "==", "target": True}
         ],
         ["physics telemetry", "golden-range file", "test-range video"]),
    goal("T1-INPUT-002", "input", "critical",
         "Keyboard and gamepad controls support the complete basic driving action set and use a shared input abstraction.",
         "mixed",
         ["Bind throttle, brake, steering, handbrake, gear actions, engine start and head-look.", "Drive one complete test lap with keyboard.", "Repeat with gamepad.", "Inspect normalized input trace."],
         [
             {"metric": "required_driving_actions_bound", "operator": "all_pass", "target": ["throttle", "brake", "steering", "handbrake", "gear_up", "gear_down", "engine_start", "head_look"]},
             {"metric": "keyboard_lap_complete", "operator": "==", "target": True},
             {"metric": "gamepad_lap_complete", "operator": "==", "target": True}
         ],
         ["binding export", "input traces", "lap videos"]),
    goal("T1-CAM-003", "camera", "critical",
         "The first-person camera supports head-look and Perception modes without changing projectile or vehicle physics.",
         "mixed",
         ["Switch between wide and far modes while stationary and driving.", "Exercise head-look to its configured limits.", "Compare vehicle and aim simulation values before and after FOV changes."],
         [
             {"metric": "head_look_left_limit_deg", "operator": "in_range", "target": [-35, -25], "unit": "degrees"},
             {"metric": "head_look_right_limit_deg", "operator": "in_range", "target": [25, 35], "unit": "degrees"},
             {"metric": "fov_changes_physics_state", "operator": "==", "target": False},
             {"metric": "external_driving_camera_available", "operator": "==", "target": False}
         ],
         ["camera settings capture", "simulation comparison trace", "test video"]),
    goal("T1-REG-004", "physics", "critical",
         "Core driving metrics are repeatable enough for regression testing.",
         "instrumented",
         ["Run acceleration, braking, turn-radius, rollover and collision-recovery tests at least five times.", "Calculate spread for each metric.", "Record provisional tolerances for the test machine."],
         [
             {"metric": "runs_per_test", "operator": ">=", "target": 5},
             {"metric": "all_core_metrics_have_tolerances", "operator": "==", "target": True},
             {"metric": "unexplained_outlier_runs", "operator": "==", "target": 0}
         ],
         ["regression baseline JSON", "summary report", "raw telemetry"])
]

test_docs["T2"] = [
    goal("T2-COMB-001", "combat", "blocker",
         "The two fixed rifles fire from their physical mounts, track ammunition separately and are resolved by authority.",
         "mixed",
         ["Load 50 rounds into each rifle.", "Fire left, right and grouped actions.", "Verify muzzle origin and obstruction.", "Attempt client-side ammunition or fire-rate tampering."],
         [
             {"metric": "left_starting_ammunition", "operator": "==", "target": 50, "unit": "rounds"},
             {"metric": "right_starting_ammunition", "operator": "==", "target": 50, "unit": "rounds"},
             {"metric": "ammunition_counts_are_independent", "operator": "==", "target": True},
             {"metric": "client_authoritative_ammo_change_accepted", "operator": "==", "target": False}
         ],
         ["combat log", "muzzle trace capture", "tamper-test result"]),
    goal("T2-DMG-002", "damage", "blocker",
         "Ordinary vehicle damage is communicated through broad performance, instrument or audio changes without internal-mechanics simulation.",
         "mixed",
         ["Apply a damaged-state event.", "Drive the same test sequence before and after damage.", "Observe cockpit pressure/warning state and audio selection.", "Inspect runtime data for internal component objects."],
         [
             {"metric": "damaged_state_changes_at_least_one_driving_metric", "operator": "==", "target": True},
             {"metric": "damaged_state_changes_instrument_or_warning", "operator": "==", "target": True},
             {"metric": "damaged_state_changes_audio", "operator": "==", "target": True},
             {"metric": "simulated_internal_mechanical_fault_count", "operator": "==", "target": 0}
         ],
         ["before/after telemetry", "cockpit capture", "audio event log", "runtime state inspection"]),
    goal("T2-DMG-003", "damage", "critical",
         "Smoke or fire is reserved for critical, heavily damaged or destroyed states.",
         "automated",
         ["Cycle a test vehicle through intact, damaged, critical and destroyed states.", "Record the selected external marker state."],
         [
             {"metric": "intact_external_smoke_or_fire", "operator": "==", "target": False},
             {"metric": "damaged_external_smoke_or_fire", "operator": "==", "target": False},
             {"metric": "critical_external_marker", "operator": "in_range", "target": ["smoke", "fire", "smoke_and_fire"]},
             {"metric": "destroyed_external_marker_present", "operator": "==", "target": True}
         ],
         ["state-to-marker test report", "captured state sequence"]),
    goal("T2-REPAIR-004", "ui", "critical",
         "The repair screen highlights broad damaged zones and does not expose simulated internal machinery.",
         "mixed",
         ["Apply damage to multiple repair zones.", "Open the repair view.", "Compare highlighted zones with authoritative records.", "Search repair UI data for prohibited internal subsystem records."],
         [
             {"metric": "repair_zone_mapping_accuracy_pct", "operator": "==", "target": 100, "unit": "%"},
             {"metric": "internal_mechanical_components_shown", "operator": "==", "target": 0},
             {"metric": "repair_cost_and_severity_present", "operator": "==", "target": True}
         ],
         ["repair-record fixture", "UI screenshots", "mapping test"])
]

test_docs["T3"] = [
    goal("T3-LOOP-001", "mission", "blocker",
         "A player can complete the full package-delivery loop from Bartertown departure through return and refit.",
         "manual",
         ["Start from a clean profile.", "Accept the package mission.", "Load cargo and depart.", "Reach the destination or trigger a supported failure.", "Return to Bartertown.", "Repair, refuel, rearm and install one salvaged part."],
         [
             {"metric": "mission_flow_steps_completed", "operator": "all_pass", "target": ["accept", "load", "depart", "objective", "return", "repair", "refuel", "rearm", "install_part"]},
             {"metric": "developer_console_required", "operator": "==", "target": False},
             {"metric": "blocking_progression_bug_count", "operator": "==", "target": 0}
         ],
         ["structured playtest record", "save/profile snapshot", "video"]),
    goal("T3-TIME-002", "mission", "major",
         "The package mission is compatible with the intended 10–25 minute session length.",
         "manual",
         ["Have at least five testers complete or fail the mission without developer shortcuts.", "Record mission-active time and outcome.", "Review outliers."],
         [
             {"metric": "completed_playtests", "operator": ">=", "target": 5},
             {"metric": "median_mission_duration_min", "operator": "in_range", "target": [10, 25], "unit": "minutes"}
         ],
         ["playtest timing sheet", "outlier notes"]),
    goal("T3-UX-003", "ui", "critical",
         "A new tester can understand route risk, fuel expectation, cargo state and vehicle damage without debug screens.",
         "manual",
         ["Give the build to testers with only normal control instructions.", "Ask them to select a route, identify fuel risk, notice damage and locate repair.", "Record task success."],
         [
             {"metric": "route_selection_success_pct", "operator": ">=", "target": 80, "unit": "%"},
             {"metric": "own_damage_noticed_without_repair_screen_pct", "operator": ">=", "target": 80, "unit": "%"},
             {"metric": "critical_vehicle_recognition_pct", "operator": ">=", "target": 80, "unit": "%"}
         ],
         ["playtest questionnaire", "observation notes"]),
    goal("T3-CONTENT-004", "content", "critical",
         "The vertical slice contains no prohibited camera or damage presentation features.",
         "automated",
         ["Scan input actions, runtime camera modes and vehicle damage asset references.", "Run the release content linter."],
         [
             {"metric": "player_third_person_driving_modes", "operator": "==", "target": 0},
             {"metric": "runtime_deformed_panel_assets", "operator": "==", "target": 0},
             {"metric": "internal_mechanics_damage_definitions", "operator": "==", "target": 0}
         ],
         ["content-lint report", "camera/action inventory"])
]

test_docs["T4"] = [
    goal("T4-PERS-001", "persistence", "blocker",
         "Character, vehicle, inventory and active mission state survive clean restart and reconnect.",
         "automated",
         ["Create a character and vehicle.", "Accept a mission and change fuel, ammunition, inventory and vehicle condition.", "Restart services.", "Reconnect and compare authoritative state."],
         [
             {"metric": "character_state_match", "operator": "==", "target": True},
             {"metric": "vehicle_state_match", "operator": "==", "target": True},
             {"metric": "inventory_state_match", "operator": "==", "target": True},
             {"metric": "mission_state_match", "operator": "==", "target": True}
         ],
         ["database fixture diff", "reconnect test log"]),
    goal("T4-PERS-002", "persistence", "blocker",
         "Duplicate or retried reward requests cannot duplicate currency, items, Fame or vehicles.",
         "automated",
         ["Complete a mission using a fixed idempotency key.", "Replay and concurrently retry the reward request.", "Inspect resulting balances and audit records."],
         [
             {"metric": "reward_commits_for_one_idempotency_key", "operator": "==", "target": 1},
             {"metric": "duplicated_currency", "operator": "==", "target": 0},
             {"metric": "duplicated_items", "operator": "==", "target": 0},
             {"metric": "duplicated_vehicles", "operator": "==", "target": 0}
         ],
         ["transaction log", "database assertions", "audit record"]),
    goal("T4-MISS-003", "mission", "critical",
         "All four Level 1 mission types can be loaded from validated data and reach complete or failed terminal states.",
         "automated",
         ["Validate mission JSON.", "Instantiate each Level 1 mission template.", "Exercise success and failure paths.", "Verify transactional consequence or reward commit."],
         [
             {"metric": "level_one_mission_types_passing", "operator": "==", "target": 4},
             {"metric": "mission_schema_errors", "operator": "==", "target": 0},
             {"metric": "terminal_states_without_commit", "operator": "==", "target": 0}
         ],
         ["schema report", "mission state-machine tests"]),
    goal("T4-ECON-004", "economy", "critical",
         "Install, repair, refuel, rearm, shop and salvage operations preserve ownership and balances transactionally.",
         "automated",
         ["Run successful operations.", "Inject version conflicts and insufficient-balance cases.", "Retry requests.", "Inspect inventory and audit state."],
         [
             {"metric": "successful_economy_operations", "operator": "all_pass", "target": ["install", "repair", "refuel", "rearm", "purchase", "salvage"]},
             {"metric": "partial_transaction_count", "operator": "==", "target": 0},
             {"metric": "ownership_violation_count", "operator": "==", "target": 0}
         ],
         ["repository test output", "transaction traces"])
]

test_docs["T5"] = [
    goal("T5-NET-001", "network", "blocker",
         "Four players can join, drive, fire and complete a mission under server authority.",
         "mixed",
         ["Start a dedicated server.", "Join four clients.", "Run a Level 1 mission with simultaneous driving and firing.", "Disconnect and reconnect one client.", "Complete the mission."],
         [
             {"metric": "simultaneous_connected_players", "operator": "==", "target": 4},
             {"metric": "mission_completed_after_reconnect", "operator": "==", "target": True},
             {"metric": "duplicate_vehicle_or_inventory_count", "operator": "==", "target": 0}
         ],
         ["server log", "client captures", "database audit"]),
    goal("T5-NET-002", "network", "critical",
         "The game remains usable under representative latency, packet loss and snapshot reordering.",
         "instrumented",
         ["Run the four-player scenario at 50, 100 and 200 ms latency.", "Repeat at 1%, 5% and 10% packet loss.", "Inject snapshot reordering.", "Capture correction and disconnect metrics."],
         [
             {"metric": "network_profiles_executed", "operator": "==", "target": 7},
             {"metric": "authority_desync_causing_state_duplication", "operator": "==", "target": 0},
             {"metric": "unrecoverable_client_disconnects", "operator": "==", "target": 0}
         ],
         ["network test report", "correction telemetry", "server/client logs"]),
    goal("T5-SEC-003", "security", "blocker",
         "The server rejects impossible transforms, ammunition changes, fire rates and inventory requests.",
         "automated",
         ["Submit forged transforms.", "Submit ammunition increases.", "Exceed weapon fire rate.", "Attempt to install or sell an unowned part."],
         [
             {"metric": "forged_transform_accept_count", "operator": "==", "target": 0},
             {"metric": "forged_ammunition_accept_count", "operator": "==", "target": 0},
             {"metric": "invalid_fire_rate_accept_count", "operator": "==", "target": 0},
             {"metric": "unowned_item_operation_accept_count", "operator": "==", "target": 0}
         ],
         ["security test output", "server rejection audit"]),
    goal("T5-CAM-004", "camera", "critical",
         "Network corrections do not create unusable first-person camera motion.",
         "mixed",
         ["Induce controlled vehicle correction during acceleration, turning and collision.", "Record root correction and cockpit camera motion.", "Review severe cases."],
         [
             {"metric": "camera_invalid_frames", "operator": "==", "target": 0},
             {"metric": "camera_detaches_from_cockpit", "operator": "==", "target": False},
             {"metric": "correction_absorber_required_decision_recorded", "operator": "==", "target": True}
         ],
         ["correction trace", "camera video", "decision record"])
]

test_docs["T6"] = [
    goal("T6-PVP-001", "mission", "blocker",
         "Contested PvP begins and ends only at clear zone boundaries and follows the approved defeat/recovery policy.",
         "mixed",
         ["Approach the contested boundary from a safe route.", "Verify warning and rule transition.", "Conduct PvP.", "Exit or recover after defeat.", "Inspect authoritative rule-space events."],
         [
             {"metric": "pvp_enabled_in_safe_zone", "operator": "==", "target": False},
             {"metric": "pvp_enabled_in_contested_zone", "operator": "==", "target": True},
             {"metric": "boundary_warning_present", "operator": "==", "target": True},
             {"metric": "defeat_policy_applied_consistently", "operator": "==", "target": True}
         ],
         ["zone event log", "PvP test record", "UI capture"]),
    goal("T6-NET-002", "network", "blocker",
         "Eight players can drive and fight in the contested-zone scenario without authority or persistence failure.",
         "mixed",
         ["Join eight clients to a dedicated server.", "Split players between safe and contested routes.", "Run simultaneous combat, depot and return actions.", "Reconnect at least one client."],
         [
             {"metric": "simultaneous_connected_players", "operator": "==", "target": 8},
             {"metric": "authority_violation_count", "operator": "==", "target": 0},
             {"metric": "persistence_duplication_count", "operator": "==", "target": 0}
         ],
         ["load-session log", "database audit", "network telemetry"]),
    goal("T6-ECON-003", "economy", "blocker",
         "Auction listing and settlement transfer ownership and payment exactly once.",
         "automated",
         ["Create a listing.", "Place or accept the supported transaction action.", "Race two settlement attempts.", "Retry the winning request.", "Inspect ownership, balances and listing state."],
         [
             {"metric": "successful_settlements_per_listing", "operator": "==", "target": 1},
             {"metric": "owners_after_settlement", "operator": "==", "target": 1},
             {"metric": "seller_payments_per_listing", "operator": "==", "target": 1}
         ],
         ["auction transaction log", "database assertions"]),
    goal("T6-REPAIR-004", "ui", "critical",
         "Expanded armour and repair-zone data remain consistent across vehicle variants.",
         "automated",
         ["Validate each vehicle's armour coverage and schematic map.", "Apply hits to every supported zone.", "Compare hit zone, recorded repair zone and highlighted UI region."],
         [
             {"metric": "vehicle_variants_with_complete_zone_map_pct", "operator": "==", "target": 100, "unit": "%"},
             {"metric": "hit_to_repair_zone_mapping_accuracy_pct", "operator": "==", "target": 100, "unit": "%"}
         ],
         ["zone-map validation report", "mapping tests"])
]

test_docs["T7"] = [
    goal("T7-SCALE-001", "performance", "blocker",
         "The active simulation supports exactly 30 vehicles and never activates a thirty-first.",
         "instrumented",
         ["Run a representative mix of player, AI and active parked vehicles.", "Increase the active count to 30.", "Attempt additional spawns and LOD promotions.", "Exercise despawn and queued activation."],
         [
             {"metric": "peak_active_vehicle_count", "operator": "==", "target": 30},
             {"metric": "active_count_overflow_events", "operator": "==", "target": 0},
             {"metric": "overflow_requests_rejected_or_queued_pct", "operator": "==", "target": 100, "unit": "%"},
             {"metric": "queued_vehicle_activates_after_slot_release", "operator": "==", "target": True}
         ],
         ["load-test telemetry", "registry audit", "spawn/LOD event trace"]),
    goal("T7-PERF-002", "performance", "blocker",
         "The 30-vehicle representative scenario meets the agreed client and server budgets on the defined qualification machines.",
         "instrumented",
         ["Run the release scenario at 1080p target settings.", "Capture client frame, server tick, Chrono, combat, mission and networking timings.", "Repeat for at least ten minutes after warm-up."],
         [
             {"metric": "client_frame_time_p95_ms", "operator": "<=", "target": 16.67, "unit": "ms", "notes": "Provisional v0.4 target on the defined minimum client."},
             {"metric": "server_tick_time_p95_ms", "operator": "<=", "target": 16.67, "unit": "ms"},
             {"metric": "chrono_whole_vehicle_time_p95_ms", "operator": "<=", "target": 9.0, "unit": "ms"},
             {"metric": "sustained_tick_overrun_seconds", "operator": "==", "target": 0, "unit": "seconds"}
         ],
         ["profiling capture", "hardware definition", "performance summary"]),
    goal("T7-LOD-003", "physics", "critical",
         "Simulation LOD transitions conserve authoritative gameplay state.",
         "automated",
         ["Move vehicles through LOD 0, 1, 2 and parked/off-zone states.", "Record state before and after transitions.", "Include a damaged, armed and mission-bound vehicle."],
         [
             {"metric": "position_velocity_discontinuity_above_tolerance_count", "operator": "==", "target": 0},
             {"metric": "fuel_ammunition_condition_mission_state_match", "operator": "==", "target": True},
             {"metric": "active_registry_count_matches_lod_state", "operator": "==", "target": True}
         ],
         ["LOD transition report", "state diffs"]),
    goal("T7-REL-004", "reliability", "blocker",
         "The release candidate survives repeated missions, reconnects and service restart without progressive corruption.",
         "mixed",
         ["Run an extended automated or supervised session containing mission completion, PvP, repair, auction, disconnect and reconnect.", "Restart the authoritative service during a safe checkpoint.", "Run database integrity checks."],
         [
             {"metric": "soak_duration_hours", "operator": ">=", "target": 8, "unit": "hours"},
             {"metric": "uncaught_server_crash_count", "operator": "==", "target": 0},
             {"metric": "database_integrity_error_count", "operator": "==", "target": 0},
             {"metric": "duplicate_reward_inventory_vehicle_count", "operator": "==", "target": 0}
         ],
         ["soak log", "database integrity report", "crash report summary"]),
    goal("T7-REG-005", "content", "blocker",
         "All MVP blocker and critical tests pass in the release build with permanent scope limits intact.",
         "automated",
         ["Run all blocker and critical tests from T0 through T7 against the release candidate.", "Run content lint for prohibited features.", "Archive evidence."],
         [
             {"metric": "blocker_and_critical_test_result", "operator": "==", "target": "pass"},
             {"metric": "player_third_person_driving_modes", "operator": "==", "target": 0},
             {"metric": "internal_mechanics_simulation_definitions", "operator": "==", "target": 0},
             {"metric": "runtime_panel_deformation_features", "operator": "==", "target": 0}
         ],
         ["release test index", "CI report", "content-lint report"])
]

test_docs["T8"] = [
    goal("T8-PROG-001", "mission", "blocker",
         "The Level 2 gate unlocks at permanent Fame 20 according to the approved progression decision.",
         "automated",
         ["Test permanent Fame values below, at and above 20.", "Test Display Fame contribution according to the approved rule.", "Attempt direct client unlock requests."],
         [
             {"metric": "unlock_below_required_fame", "operator": "==", "target": False},
             {"metric": "unlock_at_required_fame", "operator": "==", "target": True},
             {"metric": "client_forced_unlock_accepted", "operator": "==", "target": False}
         ],
         ["progression test report", "authority audit"]),
    goal("T8-MISS-002", "mission", "critical",
         "Multi-stage Level 2 missions resume correctly and commit rewards once.",
         "automated",
         ["Instantiate each Level 2 mission template.", "Advance to an intermediate stage.", "Disconnect or restart.", "Resume and complete.", "Replay completion requests."],
         [
             {"metric": "multi_stage_resume_success_pct", "operator": "==", "target": 100, "unit": "%"},
             {"metric": "reward_commits_per_run", "operator": "==", "target": 1}
         ],
         ["mission persistence tests", "transaction audit"]),
    goal("T8-SCOPE-003", "content", "blocker",
         "Specialist equipment uses the existing external-part and broad-condition architecture without internal mechanics or cap expansion.",
         "automated",
         ["Validate specialist part schemas.", "Inspect runtime entities and damage definitions.", "Run the active-vehicle cap tests with Level 2 content enabled."],
         [
             {"metric": "specialist_parts_with_internal_simulated_subsystems", "operator": "==", "target": 0},
             {"metric": "specialist_parts_missing_external_mount_mass_or_condition_data", "operator": "==", "target": 0},
             {"metric": "active_vehicle_cap_with_level_two_content", "operator": "==", "target": 30}
         ],
         ["schema validation", "runtime content inspection", "scale-test result"]),
    goal("T8-ECON-004", "economy", "critical",
         "Level 2 rewards and sinks do not invalidate the measured Level 1 economy.",
         "instrumented",
         ["Run simulated and live progression cohorts using MVP telemetry.", "Measure time to replacement, repair burden, currency sources/sinks and specialist-part acquisition.", "Review exploit paths."],
         [
             {"metric": "economy_model_review_completed", "operator": "==", "target": True},
             {"metric": "known_infinite_currency_or_item_loops", "operator": "==", "target": 0},
             {"metric": "level_one_progression_remains_functional", "operator": "==", "target": True}
         ],
         ["economy simulation report", "telemetry comparison", "exploit review"])
]

manifest_entries = []
for t in tranches:
    filename = f"{t['id'].lower()}_{t['slug']}.json"
    doc = {
        "schema_version": "1.0",
        "source_specification": source_spec,
        "tranche_id": t["id"],
        "title": t["title"],
        "phase": t["phase"],
        "mvp": t["mvp"],
        "purpose": t["objective"],
        "dependencies": t["dependencies"],
        "entry_criteria": (
            ["Approved v0.4 specification is available.", "Pinned engine versions and repository access are available."]
            if not t["dependencies"] else
            [f"{dep} exit gate has passed." for dep in t["dependencies"]]
        ),
        "non_goals": t["exclusions"],
        "test_environment": {
            "build_configuration": ["profile or release-like build", "debug symbols retained for test captures"],
            "topology": (
                ["single client plus authoritative host"]
                if t["id"] in ["T0", "T1", "T2", "T3", "T4"]
                else ["dedicated authoritative server", "multiple client processes"]
            ),
            "required_assets": ["tranche-specific test map and fixtures", "versioned data definitions"],
            "network_profiles": (
                []
                if t["id"] in ["T0", "T1", "T2", "T3", "T4"]
                else ["baseline LAN", "50 ms latency", "100 ms latency", "200 ms latency", "1% loss", "5% loss", "10% loss"]
            ),
            "notes": [
                "No player-accessible third-person driving camera.",
                "No internal vehicle-mechanics simulation.",
                "No active gameplay instance may exceed 30 vehicles."
            ]
        },
        "goals": test_docs[t["id"]],
        "exit_gate": {
            "rule": "all_blocker_and_critical_pass",
            "required_priorities": ["blocker", "critical"],
            "required_evidence": ["machine-readable test result", "logs or telemetry", "artifact or build identifier"],
            "decision_owner": "project technical owner",
            "notes": [t["exit"]]
        }
    }
    (tests_dir / filename).write_text(json.dumps(doc, indent=2) + "\n", encoding="utf-8")
    manifest_entries.append({
        "tranche_id": t["id"],
        "title": t["title"],
        "phase": t["phase"],
        "mvp": t["mvp"],
        "file": filename,
        "dependencies": t["dependencies"]
    })

manifest = {
    "schema_version": "1.0",
    "plan_version": "0.1",
    "source_specification": source_spec,
    "schema_file": "test_goal.schema.json",
    "mvp_final_tranche": "T7",
    "post_mvp_start": "T8",
    "tranches": manifest_entries
}
(tests_dir / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")

readme = """# Test goals

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
"""
(tests_dir / "README.md").write_text(readme, encoding="utf-8")

# Validate JSON syntax, unique test IDs and cross-file dependencies.
all_ids = set()
validation_notes = []
for path in sorted(tests_dir.glob("*.json")):
    obj = json.loads(path.read_text(encoding="utf-8"))
    if path.name not in ("test_goal.schema.json", "manifest.json"):
        assert obj["tranche_id"] in {t["id"] for t in tranches}
        for g in obj["goals"]:
            assert g["id"] not in all_ids, f"Duplicate test ID {g['id']}"
            all_ids.add(g["id"])
    validation_notes.append(f"OK: {path.name}")

(root / "validation_report.txt").write_text(
    "\n".join(validation_notes) +
    f"\n\nUnique test goals: {len(all_ids)}\n"
    f"Tranche documents: {len(tranches)}\n",
    encoding="utf-8"
)

zip_path = root.parent / "ldgm_development_plan_and_test_goals.zip"
with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as zf:
    for path in sorted(root.rglob("*")):
        if path.is_file():
            zf.write(path, arcname=str(path.relative_to(root.parent)))

print(f"Created {root / 'development_plan.md'}")
print(f"Created {len(tranches)} tranche JSON documents plus schema and manifest")
print(f"Created {zip_path}")
print(f"Unique test goals: {len(all_ids)}")
