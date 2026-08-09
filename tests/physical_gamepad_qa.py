"""Interactive packaged-game verification for a real physical controller.

Launches the packaged UI, records the connected Windows controller inventory,
then waits for a physical button to produce both a visible UMG transition and
a GameInput button log entry. The operator should press R1 once after launch.
"""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import time
from pathlib import Path

from packaged_ui_qa import (
    activate,
    capture,
    find_window,
    pixel_delta,
    process_image_path,
    user32,
    wait_for_render,
    window_process_id,
    window_title,
    windows_game_controllers,
    WM_CLOSE,
)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--artifacts", type=Path, required=True)
    parser.add_argument("--timeout", type=float, default=30.0)
    args = parser.parse_args()

    executable = args.exe.resolve()
    artifacts = args.artifacts.resolve()
    artifacts.mkdir(parents=True, exist_ok=True)
    isolated_user = artifacts / "user"
    isolated_user.mkdir(exist_ok=True)
    process = subprocess.Popen(
        [
            str(executable),
            "-windowed",
            "-ResX=1440",
            "-ResY=810",
            "-ForceRes",
            "-NoSplash",
            f"-UserDir={isolated_user}",
            '-LogCmds=LogGameInput Verbose',
        ],
        cwd=executable.parent,
    )

    report: dict[str, object] = {
        "instruction": "Press R1 once after the packaged window appears.",
        "windows_game_controllers": windows_game_controllers(),
        "physical_gamepad_ui_passed": False,
    }
    hwnd = 0
    game_pid = process.pid
    try:
        hwnd = find_window(process.pid, executable.parent, executable.stem)
        game_pid = window_process_id(hwnd)
        report.update(
            {
                "launcher_pid": process.pid,
                "game_pid": game_pid,
                "window_title": window_title(hwnd),
                "game_process_path": str(process_image_path(game_pid)),
            }
        )
        activate(hwnd)
        wait_for_render(hwnd)
        time.sleep(1.0)
        baseline = capture(hwnd, artifacts / "01_before_physical_gamepad.png")
        report["ready_utc"] = time.time()
        (artifacts / "report.json").write_text(json.dumps(report, indent=2), encoding="utf-8")

        deadline = time.time() + args.timeout
        largest_delta = 0.0
        changed = None
        while time.time() < deadline:
            current = capture(hwnd, artifacts / "_probe.png")
            largest_delta = max(largest_delta, pixel_delta(baseline, current))
            if largest_delta > 5.0:
                changed = current
                break
            time.sleep(0.15)

        probe = artifacts / "_probe.png"
        if probe.exists():
            probe.unlink()
        if changed is not None:
            changed.save(artifacts / "02_after_physical_gamepad.png")
        time.sleep(0.5)

        runtime_log = isolated_user / "Saved" / "Logs" / "threedcharacter.log"
        log_text = runtime_log.read_text(encoding="utf-8", errors="replace") if runtime_log.exists() else ""
        button_lines = [
            line for line in log_text.splitlines()
            if "LogGameInput" in line and ("Key Press" in line or "State: 1" in line)
        ]
        report["largest_pixel_delta"] = largest_delta
        report["gameinput_button_lines"] = button_lines[-20:]
        report["physical_gamepad_ui_passed"] = changed is not None and bool(button_lines)
        (artifacts / "report.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
        return 0 if report["physical_gamepad_ui_passed"] else 2
    finally:
        if hwnd and user32.IsWindow(hwnd):
            user32.PostMessageW(hwnd, WM_CLOSE, 0, 0)
            time.sleep(0.5)
        if process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()


if __name__ == "__main__":
    raise SystemExit(main())
