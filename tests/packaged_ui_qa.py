"""Packaged Unreal UI smoke test using Win32 input and screenshot evidence.

Unreal renders UMG into a self-drawn viewport, so UI Automation cannot address
individual controls. This harness drives the fixed 1440x810 design coordinates,
captures each state, and verifies that interactions materially change pixels.
"""

from __future__ import annotations

import argparse
import ctypes
import json
import os
import subprocess
import time
from pathlib import Path

from PIL import ImageChops, ImageGrab, ImageStat


user32 = ctypes.windll.user32
kernel32 = ctypes.windll.kernel32

try:
    user32.SetProcessDpiAwarenessContext(ctypes.c_void_p(-4))  # Per-monitor v2; keep Win32 and screenshot pixels aligned.
except (AttributeError, OSError):
    try:
        ctypes.windll.shcore.SetProcessDpiAwareness(2)
    except (AttributeError, OSError):
        user32.SetProcessDPIAware()

SW_RESTORE = 9
HWND_TOPMOST = ctypes.c_void_p(-1)
WM_CLOSE = 0x0010
WM_KEYDOWN = 0x0100
WM_KEYUP = 0x0101
WM_CHAR = 0x0102
WM_MOUSEMOVE = 0x0200
WM_LBUTTONDOWN = 0x0201
WM_LBUTTONUP = 0x0202
MK_LBUTTON = 0x0001
MOUSEEVENTF_LEFTDOWN = 0x0002
MOUSEEVENTF_LEFTUP = 0x0004
KEYEVENTF_KEYUP = 0x0002
KEYEVENTF_UNICODE = 0x0004
VK_ESCAPE = 0x1B
PROCESS_QUERY_LIMITED_INFORMATION = 0x1000
SWP_NOSIZE = 0x0001
SWP_NOMOVE = 0x0002
SWP_SHOWWINDOW = 0x0040

user32.SetWindowPos.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_uint]
user32.SetWindowPos.restype = ctypes.c_bool
user32.SetForegroundWindow.argtypes = [ctypes.c_void_p]
user32.SetForegroundWindow.restype = ctypes.c_bool
user32.BringWindowToTop.argtypes = [ctypes.c_void_p]
user32.BringWindowToTop.restype = ctypes.c_bool
user32.AttachThreadInput.argtypes = [ctypes.c_ulong, ctypes.c_ulong, ctypes.c_bool]
user32.AttachThreadInput.restype = ctypes.c_bool
user32.GetForegroundWindow.restype = ctypes.c_void_p
user32.GetWindowThreadProcessId.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
user32.GetWindowThreadProcessId.restype = ctypes.c_ulong
user32.PostMessageW.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_size_t, ctypes.c_ssize_t]
user32.PostMessageW.restype = ctypes.c_bool
user32.SendInput.argtypes = [ctypes.c_uint, ctypes.c_void_p, ctypes.c_int]
user32.SendInput.restype = ctypes.c_uint


class RECT(ctypes.Structure):
    _fields_ = [("left", ctypes.c_long), ("top", ctypes.c_long), ("right", ctypes.c_long), ("bottom", ctypes.c_long)]


class POINT(ctypes.Structure):
    _fields_ = [("x", ctypes.c_long), ("y", ctypes.c_long)]


class KEYBDINPUT(ctypes.Structure):
    _fields_ = [
        ("wVk", ctypes.c_ushort),
        ("wScan", ctypes.c_ushort),
        ("dwFlags", ctypes.c_ulong),
        ("time", ctypes.c_ulong),
        ("dwExtraInfo", ctypes.c_void_p),
    ]


class MOUSEINPUT(ctypes.Structure):
    _fields_ = [
        ("dx", ctypes.c_long),
        ("dy", ctypes.c_long),
        ("mouseData", ctypes.c_ulong),
        ("dwFlags", ctypes.c_ulong),
        ("time", ctypes.c_ulong),
        ("dwExtraInfo", ctypes.c_void_p),
    ]


class HARDWAREINPUT(ctypes.Structure):
    _fields_ = [("uMsg", ctypes.c_ulong), ("wParamL", ctypes.c_ushort), ("wParamH", ctypes.c_ushort)]


class INPUT_UNION(ctypes.Union):
    _fields_ = [("mi", MOUSEINPUT), ("ki", KEYBDINPUT), ("hi", HARDWAREINPUT)]


class INPUT(ctypes.Structure):
    _fields_ = [("type", ctypes.c_ulong), ("union", INPUT_UNION)]


def process_image_path(pid: int) -> Path | None:
    handle = kernel32.OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, False, pid)
    if not handle:
        return None
    try:
        size = ctypes.c_ulong(32768)
        buffer = ctypes.create_unicode_buffer(size.value)
        if kernel32.QueryFullProcessImageNameW(handle, 0, buffer, ctypes.byref(size)):
            return Path(buffer.value)
        return None
    finally:
        kernel32.CloseHandle(handle)


def window_process_id(hwnd: int) -> int:
    process_id = ctypes.c_ulong()
    user32.GetWindowThreadProcessId(hwnd, ctypes.byref(process_id))
    return process_id.value


def window_title(hwnd: int) -> str:
    length = user32.GetWindowTextLengthW(hwnd)
    buffer = ctypes.create_unicode_buffer(length + 1)
    user32.GetWindowTextW(hwnd, buffer, len(buffer))
    return buffer.value


def find_window(pid: int, package_root: Path, expected_title: str, timeout: float = 45.0) -> int:
    found: list[int] = []
    callback_type = ctypes.WINFUNCTYPE(ctypes.c_bool, ctypes.c_void_p, ctypes.c_void_p)

    def enum_callback(hwnd: int, _: int) -> bool:
        process_id = window_process_id(hwnd)
        if user32.IsWindowVisible(hwnd):
            image_path = process_image_path(process_id)
            title_matches = expected_title.lower() in window_title(hwnd).lower()
            process_matches = process_id == pid or (image_path and str(image_path).lower().startswith(str(package_root).lower()))
            if title_matches and process_matches:
                found.append(hwnd)
        return True

    callback = callback_type(enum_callback)
    deadline = time.time() + timeout
    previous = 0
    stable_polls = 0
    while time.time() < deadline:
        found.clear()
        user32.EnumWindows(callback, 0)
        if found:
            candidate = found[0]
            stable_polls = stable_polls + 1 if candidate == previous else 1
            previous = candidate
            if stable_polls >= 3:
                return candidate
        else:
            previous = 0
            stable_polls = 0
        time.sleep(0.2)
    raise TimeoutError(f"No visible packaged-game window appeared for PID {pid} under {package_root}")


def client_rect_screen(hwnd: int) -> tuple[int, int, int, int]:
    rect = RECT()
    if not user32.GetClientRect(hwnd, ctypes.byref(rect)):
        raise ctypes.WinError()
    origin = POINT(0, 0)
    if not user32.ClientToScreen(hwnd, ctypes.byref(origin)):
        raise ctypes.WinError()
    return origin.x, origin.y, origin.x + rect.right, origin.y + rect.bottom


def activate(hwnd: int) -> None:
    foreground = user32.GetForegroundWindow()
    foreground_thread = user32.GetWindowThreadProcessId(foreground, None) if foreground else 0
    target_thread = user32.GetWindowThreadProcessId(hwnd, None)
    current_thread = kernel32.GetCurrentThreadId()
    attached_foreground = bool(foreground_thread and foreground_thread != current_thread and user32.AttachThreadInput(current_thread, foreground_thread, True))
    attached_target = bool(target_thread and target_thread != current_thread and target_thread != foreground_thread and user32.AttachThreadInput(current_thread, target_thread, True))
    try:
        user32.ShowWindow(hwnd, SW_RESTORE)
        user32.SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW)
        user32.BringWindowToTop(hwnd)
        user32.SetActiveWindow(hwnd)
        user32.SetForegroundWindow(hwnd)
        user32.SetFocus(hwnd)
        if user32.GetForegroundWindow() != hwnd:
            user32.keybd_event(0x12, 0, 0, 0)  # ALT temporarily permits foreground activation.
            user32.SetForegroundWindow(hwnd)
            user32.keybd_event(0x12, 0, KEYEVENTF_KEYUP, 0)
    finally:
        if attached_target:
            user32.AttachThreadInput(current_thread, target_thread, False)
        if attached_foreground:
            user32.AttachThreadInput(current_thread, foreground_thread, False)
    time.sleep(0.08)


def click_design(hwnd: int, x: float, y: float) -> None:
    left, top, right, bottom = client_rect_screen(hwnd)
    client_width = right - left
    client_height = bottom - top
    fit_scale = min(client_width / 1440.0, client_height / 810.0)
    content_width = 1440.0 * fit_scale
    content_height = 810.0 * fit_scale
    offset_x = (client_width - content_width) * 0.5
    offset_y = (client_height - content_height) * 0.5
    screen_x = round(left + offset_x + x * fit_scale)
    screen_y = round(top + offset_y + y * fit_scale)
    activate(hwnd)
    user32.SetCursorPos(screen_x, screen_y)
    time.sleep(0.08)
    user32.mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0)
    time.sleep(0.06)
    user32.mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0)
    time.sleep(0.08)


def press_key(hwnd: int, vk: int) -> None:
    activate(hwnd)
    user32.keybd_event(vk, 0, 0, 0)
    time.sleep(0.04)
    user32.keybd_event(vk, 0, KEYEVENTF_KEYUP, 0)


def type_unicode(hwnd: int, text: str) -> None:
    activate(hwnd)
    for character in text:
        scan_code = ord(character)
        events = (INPUT * 2)()
        events[0].type = 1
        events[0].union.ki = KEYBDINPUT(0, scan_code, KEYEVENTF_UNICODE, 0, None)
        events[1].type = 1
        events[1].union.ki = KEYBDINPUT(0, scan_code, KEYEVENTF_UNICODE | KEYEVENTF_KEYUP, 0, None)
        if user32.SendInput(2, ctypes.byref(events), ctypes.sizeof(INPUT)) != 2:
            raise ctypes.WinError()
        time.sleep(0.01)


def windows_game_controllers() -> list[dict[str, str]]:
    """Return present Windows game-controller devices, including non-XInput HID pads."""
    script = r"""
$devices = Get-PnpDevice -PresentOnly | Where-Object {
    $_.FriendlyName -match 'DualSense|Wireless Controller|PlayStation|Game Controller|HID-compliant game controller'
} | ForEach-Object {
    [PSCustomObject]@{
        status = [string]$_.Status
        class = [string]$_.Class
        name = [string]$_.FriendlyName
        instance_id = [string]$_.InstanceId
    }
}
@($devices) | ConvertTo-Json -Compress
"""
    completed = subprocess.run(
        ["powershell", "-NoProfile", "-Command", script],
        capture_output=True,
        text=True,
        timeout=15,
        check=True,
    )
    payload = completed.stdout.strip()
    if not payload:
        return []
    parsed = json.loads(payload)
    return parsed if isinstance(parsed, list) else [parsed]


def capture(hwnd: int, path: Path):
    path.parent.mkdir(parents=True, exist_ok=True)
    image = ImageGrab.grab(bbox=client_rect_screen(hwnd), all_screens=True)
    image.save(path)
    return image


def wait_for_render(hwnd: int, timeout: float = 30.0) -> float:
    deadline = time.time() + timeout
    while time.time() < deadline:
        image = ImageGrab.grab(bbox=client_rect_screen(hwnd), all_screens=True).convert("RGB")
        width, height = image.size
        interior = image.crop((20, 20, max(21, width - 20), max(21, height - 20)))
        variation = max(ImageStat.Stat(interior).stddev)
        if variation > 2.0:
            return variation
        time.sleep(0.5)
    raise TimeoutError("The packaged window stayed visually blank after launch")


def pixel_delta(first, second) -> float:
    difference = ImageChops.difference(first.convert("RGB"), second.convert("RGB"))
    return sum(ImageStat.Stat(difference).mean) / 3.0


def xinput_devices() -> list[int]:
    class GAMEPAD(ctypes.Structure):
        _fields_ = [
            ("buttons", ctypes.c_ushort), ("left_trigger", ctypes.c_ubyte), ("right_trigger", ctypes.c_ubyte),
            ("left_x", ctypes.c_short), ("left_y", ctypes.c_short), ("right_x", ctypes.c_short), ("right_y", ctypes.c_short),
        ]

    class STATE(ctypes.Structure):
        _fields_ = [("packet", ctypes.c_ulong), ("gamepad", GAMEPAD)]

    for name in ("xinput1_4", "xinput9_1_0", "xinput1_3"):
        try:
            library = ctypes.WinDLL(name)
            return [index for index in range(4) if library.XInputGetState(index, ctypes.byref(STATE())) == 0]
        except OSError:
            continue
    return []


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--artifacts", type=Path, required=True)
    args = parser.parse_args()
    executable = args.exe.resolve()
    artifacts = args.artifacts.resolve()
    artifacts.mkdir(parents=True, exist_ok=True)
    isolated_user = artifacts / "user"
    isolated_user.mkdir(exist_ok=True)

    process = subprocess.Popen([
        str(executable), "-windowed", "-ResX=1440", "-ResY=810", "-ForceRes",
        "-NoSplash", f"-UserDir={isolated_user}",
    ], cwd=executable.parent)
    report: dict[str, object] = {
        "pid": process.pid,
        "xinput_devices": xinput_devices(),
        "windows_game_controllers": windows_game_controllers(),
        "steps": {},
    }
    hwnd = 0
    game_pid = process.pid
    try:
        hwnd = find_window(process.pid, executable.parent, executable.stem)
        game_pid = window_process_id(hwnd)
        report["game_pid"] = game_pid
        report["window_title"] = window_title(hwnd)
        image_path = process_image_path(game_pid)
        report["game_process_path"] = str(image_path) if image_path else None
        (artifacts / "report.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
        activate(hwnd)
        foreground_hwnd = user32.GetForegroundWindow()
        report["foreground_pid"] = window_process_id(foreground_hwnd) if foreground_hwnd else None
        time.sleep(3.0)
        report["launch_pixel_stddev"] = wait_for_render(hwnd)
        time.sleep(0.5)
        left, top, right, bottom = client_rect_screen(hwnd)
        report["client_size"] = [right - left, bottom - top]

        dashboard = capture(hwnd, artifacts / "01_dashboard.png")
        click_design(hwnd, 1061, 266)  # NEW CHARACTER
        time.sleep(0.35)
        type_unicode(hwnd, " Packaged QA")
        prompt = capture(hwnd, artifacts / "02_named_project_prompt_keyboard.png")
        report["steps"]["mouse_open_named_project_prompt"] = pixel_delta(dashboard, prompt)
        press_key(hwnd, VK_ESCAPE)
        time.sleep(0.25)
        dashboard_after_escape = capture(hwnd, artifacts / "03_keyboard_escape_dashboard.png")
        report["steps"]["keyboard_escape_closes_modal"] = pixel_delta(prompt, dashboard_after_escape)

        click_design(hwnd, 984, 28)  # GLOBAL BROWSER
        time.sleep(0.35)
        projects = capture(hwnd, artifacts / "04_project_browser_mouse.png")
        report["steps"]["mouse_project_browser"] = pixel_delta(dashboard_after_escape, projects)

        click_design(hwnd, 1330, 28)  # GLOBAL GAMEPAD OVERLAY
        time.sleep(0.25)
        gamepad_overlay = capture(hwnd, artifacts / "05_gamepad_overlay.png")
        report["steps"]["gamepad_overlay"] = pixel_delta(projects, gamepad_overlay)
        press_key(hwnd, VK_ESCAPE)
        time.sleep(0.2)
        press_key(hwnd, VK_ESCAPE)
        time.sleep(0.3)
        returned = capture(hwnd, artifacts / "06_keyboard_return_dashboard.png")
        report["steps"]["keyboard_return_dashboard"] = pixel_delta(projects, returned)

        click_design(hwnd, 445, 790)  # OPEN ASSET BROWSER tray
        time.sleep(0.4)
        assets = capture(hwnd, artifacts / "07_asset_browser_mouse.png")
        report["steps"]["mouse_asset_browser"] = pixel_delta(returned, assets)

        click_design(hwnd, 870, 305)  # FAVORITE first mounted asset; forces live data-row rebuild.
        time.sleep(0.5)
        favorited = capture(hwnd, artifacts / "08_asset_favorite_rebuild.png")
        report["steps"]["asset_favorite_live_rebuild"] = pixel_delta(assets, favorited)

        thresholds = {"asset_favorite_live_rebuild": 0.25}
        report["passed"] = all(
            float(value) > thresholds.get(name, 1.0)
            for name, value in report["steps"].items()
        )
        report["physical_gamepad_present"] = bool(report["windows_game_controllers"])
        report["physical_gamepad_ui_passed"] = False  # Set only by a real button-driven UI transition.
        (artifacts / "report.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
        return 0 if report["passed"] else 2
    finally:
        try:
            if hwnd and user32.IsWindow(hwnd):
                user32.PostMessageW(hwnd, WM_CLOSE, 0, 0)
                deadline = time.time() + 5.0
                while user32.IsWindow(hwnd) and time.time() < deadline:
                    time.sleep(0.1)
            if hwnd and user32.IsWindow(hwnd):
                try:
                    os.kill(game_pid, 15)
                except OSError:
                    pass
        finally:
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=3)
                except subprocess.TimeoutExpired:
                    process.kill()


if __name__ == "__main__":
    raise SystemExit(main())
