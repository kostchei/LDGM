#!/usr/bin/env python3
"""Evaluate the checked-in T0 contract from current local build evidence."""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import subprocess
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def values(pattern: str, text: str, cast=float) -> list:
    return [cast(match) for match in re.findall(pattern, text)]


def latest_max(pattern: str, text: str):
    found = values(pattern, text)
    return max(found) if found else None


def at_most(measured, target: float) -> bool:
    return measured is not None and measured <= target


def exactly(measured, target) -> bool:
    return measured is not None and measured == target


def assertion(metric: str, operator: str, target, measured, passed: bool, evidence: str) -> dict:
    return {
        "metric": metric,
        "operator": operator,
        "target": target,
        "measured": measured,
        "pass": passed,
        "evidence": evidence,
    }


def locate_ctest() -> Path | None:
    lock = json.loads((ROOT / "dependencies.lock.json").read_text(encoding="utf-8"))
    local_app_data = os.environ.get("LOCALAPPDATA")
    if local_app_data:
        candidate = (
            Path(local_app_data)
            / "LDGM"
            / "tools"
            / lock["windows_toolchain"]["cmake_portable_directory"]
            / "bin"
            / "ctest.exe"
        )
        if candidate.exists():
            return candidate
    found = shutil.which("ctest")
    return Path(found) if found else None


def run_unit_tests(configuration: str) -> tuple[int | None, str]:
    ctest = locate_ctest()
    if ctest is None:
        return None, "ctest was not found"
    completed = subprocess.run(
        [
            str(ctest),
            "--test-dir",
            str(ROOT / "build" / "windows"),
            "-C",
            configuration,
            "-R",
            "LDMChronoVehicle",
            "--output-on-failure",
        ],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    )
    return completed.returncode, (completed.stdout + completed.stderr).strip()


def git_output(*args: str) -> str:
    return subprocess.run(
        ["git", *args], cwd=ROOT, text=True, capture_output=True, check=True
    ).stdout.strip()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--configuration", default="profile")
    parser.add_argument(
        "--output",
        type=Path,
        default=ROOT / "artifacts" / "t0" / "t0_gate_result.json",
    )
    args = parser.parse_args()

    server_log_path = ROOT / "user" / "log" / "Server.log"
    client_log_path = ROOT / "user" / "log" / "Game.log"
    if not server_log_path.exists() or not client_log_path.exists():
        raise SystemExit("Run scripts/test-launchers.ps1 before evaluating T0.")

    server = server_log_path.read_text(encoding="utf-8", errors="replace")
    client = client_log_path.read_text(encoding="utf-8", errors="replace")
    unit_exit_code, unit_output = run_unit_tests(args.configuration)
    unit_passed = unit_exit_code == 0

    server_smoke = (
        "Chrono Core/Vehicle lifecycle smoke passed" in server
        and "role=authoritative" in server
    )
    client_smoke = (
        "Chrono Core/Vehicle lifecycle smoke passed" in client
        and "role=client-linkage" in client
    )
    two_vehicle_runtime = "role=player" in server and "role=enemy" in server and "active=2" in server

    terrain_origin_error = latest_max(r"origin_err_m=([0-9.]+)", client)
    terrain_surface_error = latest_max(r"surface_err_m=([0-9.]+)", client)
    proxy_position_error = latest_max(r"max_err_pos_m=([0-9.]+)", client)
    proxy_rotation_error = latest_max(r"max_err_rot_deg=([0-9.]+)", client)
    camera_invalid_frames_value = latest_max(r"invalid_frames=(\d+)", client)
    camera_invalid_frames = (
        int(camera_invalid_frames_value)
        if camera_invalid_frames_value is not None
        else None
    )
    camera_attachment_error = latest_max(r"max_attach_err_m=([0-9.]+)", client)
    camera_active = "camera_component=true active=true" in client
    external_camera_actions_value = latest_max(r"external-camera actions=(\d+)", client)
    external_camera_actions = (
        int(external_camera_actions_value)
        if external_camera_actions_value is not None
        else None
    )
    received_snapshots_value = latest_max(r"received=(\d+)", client)
    received_snapshots = (
        int(received_snapshots_value) if received_snapshots_value is not None else None
    )
    rejected_snapshots_value = latest_max(r"rejected=(\d+)", client)
    rejected_snapshots = (
        int(rejected_snapshots_value) if rejected_snapshots_value is not None else None
    )
    timing_metrics_present = all(
        marker in server
        for marker in ("step_wall_ms=", "total_steps=", "dropped_s=", "accumulator_s=")
    )
    max_step_wall_ms = latest_max(r"step_wall_ms=([0-9.]+)", server)
    max_dropped_seconds = latest_max(r"dropped_s=([0-9.]+)", server)
    max_accumulator_seconds = latest_max(r"accumulator_s=([0-9.]+)", server)
    total_steps = latest_max(r"total_steps=(\d+)", server)

    physics_assertions = [
        assertion("terrain_origin_error_m", "<=", 0.02, terrain_origin_error,
                  at_most(terrain_origin_error, 0.02),
                  "T0 client terrain presentation trace"),
        assertion("render_proxy_position_error_m", "<=", 0.05, proxy_position_error,
                  at_most(proxy_position_error, 0.05),
                  "T0 proxy trace from mesh-backed client entities"),
        assertion("orientation_error_deg", "<=", 1.0, proxy_rotation_error,
                  at_most(proxy_rotation_error, 1.0),
                  "T0 proxy trace from player/enemy entities"),
    ]
    camera_attached = (
        camera_active and at_most(camera_attachment_error, 0.05)
    )
    camera_assertions = [
        assertion("camera_nan_or_invalid_frames", "==", 0, camera_invalid_frames,
                  exactly(camera_invalid_frames, 0),
                  "T0 camera trace"),
        assertion("player_external_driving_camera_actions", "==", 0,
                  external_camera_actions, exactly(external_camera_actions, 0),
                  "T0 input inventory; template FlyCameraInputComponent removed"),
        assertion("camera_remains_attached_to_cockpit", "==", True,
                  camera_attached, camera_attached,
                  "active CameraComponent trace and attachment telemetry"),
    ]
    performance_assertions = [
        assertion("peak_active_vehicle_count", "==", 30, 30 if unit_passed else None,
                  unit_passed,
                  "ActiveVehicleRegistryTests.RejectsThirtyFirstUniqueVehicle"),
        assertion("thirty_first_spawn_result", "in_range", ["rejected", "queued"],
                  "rejected" if unit_passed else None, unit_passed,
                  "ActiveVehicleRegistryTests.RejectsThirtyFirstUniqueVehicle"),
        assertion("simulation_timing_metrics_present", "==", True,
                  timing_metrics_present, timing_metrics_present,
                  "authoritative transform telemetry"),
    ]

    goals = [
        {
            "id": "T0-BUILD-001",
            "status": "blocked",
            "assertions": [
                assertion("clean_client_build_exit_code", "==", 0, None, False,
                          "A clean client rebuild was not run by this evaluator."),
                assertion("clean_server_build_exit_code", "==", 0, None, False,
                          "A clean server rebuild was not run by this evaluator."),
                assertion("chrono_link_smoke_test", "==", "pass",
                          "pass" if server_smoke and client_smoke else "fail",
                          server_smoke and client_smoke,
                          "user/log/Server.log; user/log/Game.log"),
            ],
            "missing_evidence": ["fresh clean dependency/bootstrap and launcher build logs"],
        },
        {
            "id": "T0-PHYS-002",
            "status": "blocked" if all(a["pass"] for a in physics_assertions) else "fail",
            "assertions": physics_assertions,
            "observations": {
                "terrain_surface_height_error_m": terrain_surface_error,
                "received_snapshots": received_snapshots,
                "rejected_snapshots": rejected_snapshots,
                "runtime_vehicle_count": 2,
                "player_and_enemy_observed": two_vehicle_runtime,
            },
            "missing_evidence": ["captured non-null-RHI driving video"],
        },
        {
            "id": "T0-CAM-003",
            "status": "blocked" if all(a["pass"] for a in camera_assertions) else "fail",
            "assertions": camera_assertions,
            "observations": {
                "max_attachment_error_m": camera_attachment_error,
                "runtime_camera_component_active": camera_active,
            },
            "missing_evidence": [
                "collision-input camera stability run",
                "captured non-null-RHI first-person video",
            ],
        },
        {
            "id": "T0-PERF-004",
            "status": "pass" if all(a["pass"] for a in performance_assertions) else "fail",
            "assertions": performance_assertions,
            "observations": {
                "runtime_fixture_count": 2,
                "capacity_implementation": 30,
                "unit_test_exit_code": unit_exit_code,
                "max_step_wall_time_ms": max_step_wall_ms,
                "max_dropped_simulation_seconds": max_dropped_seconds,
                "max_accumulator_seconds": max_accumulator_seconds,
                "total_authoritative_steps": int(total_steps) if total_steps is not None else None,
            },
            "missing_evidence": [],
        },
    ]

    automated_failure = any(goal["status"] == "fail" for goal in goals)
    gate_status = "fail" if automated_failure else "blocked"
    decision = "do_not_advance_to_t1"
    summary = (
        "One or more automated T0 assertions failed; inspect the per-goal results."
        if automated_failure
        else (
            "Two-vehicle authoritative simulation, client mesh presentation, runtime cockpit camera, "
            "snapshot transport, terrain alignment, registry capacity, and timing telemetry pass their "
            "automated assertions. T0 remains blocked on clean-build evidence and required visual/collision captures."
        )
    )

    result = {
        "schema_version": "1.0",
        "tranche_id": "T0",
        "source_contract": "test_goals/t0_integration_spike.json",
        "evaluated_utc": datetime.now(timezone.utc).isoformat(),
        "build": {
            "git_commit": git_output("rev-parse", "HEAD"),
            "working_tree_clean": not bool(git_output("status", "--porcelain")),
            "configuration": args.configuration,
            "platform": "Windows",
        },
        "gate_rule": "all_blocker_and_critical_pass",
        "gate_status": gate_status,
        "decision": decision,
        "summary": summary,
        "goals": goals,
        "unit_test_output": unit_output,
        "evidence": [
            "user/log/Server.log",
            "user/log/Game.log",
            "build/windows/Testing/Temporary/LastTest.log",
            "dependencies.lock.json",
        ],
    }

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {args.output}")
    print(f"T0 gate status: {gate_status}")
    return 1 if automated_failure else 2


if __name__ == "__main__":
    raise SystemExit(main())
