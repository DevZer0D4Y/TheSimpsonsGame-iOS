#!/usr/bin/env python3
"""
The Simpsons Game — Recompiled : Launcher  (v3)

- Native desktop app (PySide6/Qt WebEngine) with browser fallback.
- Install from the user's own legally-owned ISO (extract-xiso).
- Launcher art generated from the user's own game files.
- Deep settings: display, FPS, anti-aliasing, input (keyboard/mouse!),
  language, audio — written to a launcher-owned block in simpsons.toml.
- Patches tab (skip intro videos; more to come).
- Save-data backup/restore, Add to Steam, GitHub update checks.
"""

import http.server
import json
import os
import platform
import re
import secrets
import shutil
import socket
import struct
import subprocess
import sys
import threading
import time
import urllib.parse
import urllib.request
import zipfile
import zlib
from pathlib import Path

VERSION = "0.3.0"

LAUNCHER_DIR = Path(__file__).resolve().parent
ROOT = LAUNCHER_DIR.parent
UI_DIR = LAUNCHER_DIR / "ui"
ART_DIR = LAUNCHER_DIR / "art"
MODS_DIR = LAUNCHER_DIR / "mods"
BACKUPS_DIR = LAUNCHER_DIR / "backups"
CONFIG_JSON = LAUNCHER_DIR / "launcher.json"

DEFAULT_CONFIG = {
    "github_repo": "",   # e.g. "username/simpsons-recomp" once published
    "engine": {
        "Linux": "simpsons/out/build/linux-amd64-relwithdebinfo/simpsons",
        "Windows": "simpsons/out/build/win-amd64-relwithdebinfo/simpsons.exe",
    },
    "lib_dirs": {
        "Linux": ["tools/rexglue-bin/linux-amd64/lib",
                  "/home/.steamos/offload/nix/store/ah4525ca553drv47jhvgpl9sl87i7a1d-libxml2-2.13.8/lib"],
        "Windows": [],
    },
}


def load_config():
    cfg = json.loads(json.dumps(DEFAULT_CONFIG))
    if CONFIG_JSON.exists():
        try:
            user = json.loads(CONFIG_JSON.read_text(encoding="utf-8"))
            for k, v in user.items():
                if isinstance(v, dict) and k in cfg:
                    cfg[k].update(v)
                else:
                    cfg[k] = v
        except Exception:
            pass
    else:
        CONFIG_JSON.write_text(json.dumps(cfg, indent=2) + "\n", encoding="utf-8")
    return cfg


CONFIG = load_config()
PLAT = platform.system()


def _resolve(p):
    p = Path(p)
    return p if p.is_absolute() else ROOT / p


GAME_BIN = _resolve(CONFIG["engine"].get(PLAT, CONFIG["engine"]["Linux"]))
BUILD_DIR = GAME_BIN.parent
GAME_TOML = BUILD_DIR / "simpsons.toml"
GAMEDATA = ROOT / "gamedata"
EXTRACT_XISO = ROOT / "tools/extract-xiso/build/extract-xiso"
if PLAT == "Windows":
    EXTRACT_XISO = EXTRACT_XISO.with_suffix(".exe")
USER_DATA = Path.home() / ".local/share/simpsons"

TOKEN = secrets.token_hex(16)
PORT = 8712

SETTINGS_BEGIN = "# >>> LAUNCHER SETTINGS (managed block - do not edit by hand) >>>"
SETTINGS_END = "# <<< LAUNCHER SETTINGS <<<"

# key -> (type, default, needs_restart)
SETTINGS_SCHEMA = {
    # display
    "fullscreen": ("bool", False, True),
    "resolution": ("str", "", True),               # "", 720p, 1080p, 1440p, 4k
    "window_width": ("int", 0, True),
    "window_height": ("int", 0, True),
    "vsync": ("bool", True, False),
    "present_letterbox": ("bool", True, False),
    # quality
    "resolution_scale": ("int", 1, True),
    "anisotropic_override": ("int", 3, False),
    "swap_post_effect": ("str", "none", True),     # none, fxaa, fxaa_extreme
    # fps
    "video_mode_refresh_rate": ("float", 60.0, True),
    # input
    "mnk_mode": ("bool", False, True),
    "mnk_sensitivity": ("float", 1.0, False),
    # game
    "user_language": ("int", 1, True),
    # audio
    "audio_mute": ("bool", False, False),
    "audio_maxqframes": ("int", 32, True),
}

LOGO_MOVIES = ("ealogo", "ealogo_sd", "foxlogo", "foxlogo_sd",
               "gracielogo", "gracielogo_sd")

game_proc_lock = threading.Lock()
game_proc = None
install_state = {"running": False, "log": [], "ok": None}
update_state = {"checked": False, "msg": "", "update_available": False}


# ----------------------------------------------------------------- settings

def _fmt(v, typ):
    if typ == "bool":
        return "true" if v else "false"
    if typ == "str":
        return f'"{v}"'
    return str(v)


def _parse(raw, typ):
    raw = raw.strip()
    if typ == "bool":
        return raw.lower() == "true"
    if typ == "str":
        return raw.strip('"')
    if typ == "float":
        return float(raw)
    return int(raw)


def read_settings():
    values = {k: v[1] for k, v in SETTINGS_SCHEMA.items()}
    if not GAME_TOML.exists():
        return values
    for line in GAME_TOML.read_text(encoding="utf-8").splitlines():
        s = line.strip()
        if "=" in s and not s.startswith("#"):
            key, _, raw = s.partition("=")
            key = key.strip()
            if key in SETTINGS_SCHEMA:
                try:
                    values[key] = _parse(raw, SETTINGS_SCHEMA[key][0])
                except ValueError:
                    pass
    return values


def write_settings(new_values):
    values = read_settings()
    for k, v in new_values.items():
        if k not in SETTINGS_SCHEMA:
            continue
        typ = SETTINGS_SCHEMA[k][0]
        values[k] = (bool(v) if typ == "bool" else str(v) if typ == "str"
                     else float(v) if typ == "float" else int(v))
    lines = []
    if GAME_TOML.exists():
        in_block = False
        for line in GAME_TOML.read_text(encoding="utf-8").splitlines():
            s = line.strip()
            if s == SETTINGS_BEGIN:
                in_block = True
                continue
            if s == SETTINGS_END:
                in_block = False
                continue
            if in_block:
                continue
            if "=" in s and not s.startswith("#") and s.partition("=")[0].strip() in SETTINGS_SCHEMA:
                continue
            lines.append(line)
        while lines and not lines[-1].strip():
            lines.pop()
    block = [SETTINGS_BEGIN]
    for k, (typ, _d, _r) in SETTINGS_SCHEMA.items():
        block.append(f"{k} = {_fmt(values[k], typ)}")
    block.append(SETTINGS_END)
    GAME_TOML.write_text("\n".join(lines + ["", *block]) + "\n", encoding="utf-8")
    return values


# ------------------------------------------------------------------ patches

def patch_skip_intro_state():
    en = GAMEDATA / "movies" / "en"
    if not en.is_dir():
        return "unavailable"
    disabled = any((en / f"{m}.vp6.disabled").exists() for m in LOGO_MOVIES)
    present = any((en / f"{m}.vp6").exists() for m in LOGO_MOVIES)
    return "on" if disabled and not present else "off"


def patch_skip_intro(enable):
    en = GAMEDATA / "movies" / "en"
    if not en.is_dir():
        return False, "game data not installed"
    n = 0
    for m in LOGO_MOVIES:
        src = en / (f"{m}.vp6" if enable else f"{m}.vp6.disabled")
        dst = en / (f"{m}.vp6.disabled" if enable else f"{m}.vp6")
        if src.exists():
            src.rename(dst)
            n += 1
    return True, f"{'skipped' if enable else 'restored'} {n} intro videos"


def _toml_flag(key, default="false"):
    if not GAME_TOML.exists():
        return default
    m = re.search(rf"^{key}\s*=\s*(\S+)", GAME_TOML.read_text(encoding="utf-8"), re.M)
    return m.group(1) if m else default


def patch_instant_popin_state():
    return "on" if _toml_flag("gpu_allow_invalid_fetch_constants") == "true" else "off"


def patch_instant_popin(enable):
    """Community fix for characters/props loading in late (they exist on the
    disc with 'invalid' vertex descriptors; real hardware drew them anyway --
    xenia-project/game-compatibility#542). Trade-off on Steam Deck: rarely, a
    level load can hand the GPU garbage and hang it, so a shader runaway cap
    is enabled alongside as a guardrail. If a level load freezes, turn this
    patch off."""
    if not GAME_TOML.exists():
        return False, "game config not found"
    text = GAME_TOML.read_text(encoding="utf-8")
    subs = [
        (r"^gpu_allow_invalid_fetch_constants\s*=.*$",
         f"gpu_allow_invalid_fetch_constants = {'true' if enable else 'false'}"),
        (r"^gpu_shader_max_cf_iterations\s*=.*$",
         f"gpu_shader_max_cf_iterations = {'4096' if enable else '0'}"),
    ]
    for pat, rep in subs:
        if not re.search(pat, text, re.M):
            return False, "config keys missing - reinstall/repair first"
        text = re.sub(pat, rep, text, flags=re.M)
    GAME_TOML.write_text(text, encoding="utf-8")
    # the runaway cap is baked into translated shaders - force a rebuild
    shutil.rmtree(USER_DATA / "cache", ignore_errors=True)
    return True, ("instant pop-in ON (shader cache rebuilds on next launch)"
                  if enable else "instant pop-in OFF (maximum stability)")


def patches_list():
    return [
        {"id": "instant_popin", "name": "Instant character pop-in (community fix)",
         "desc": "EXPERIMENTAL - currently CRASHES on Steam Deck during level "
                 "loads (a GPU driver interaction; the same fix works in Xenia "
                 "on desktop GPUs). Leave OFF on Deck. Kept for future driver "
                 "updates and the Windows build.",
         "state": patch_instant_popin_state(), "available": GAME_TOML.exists()},
        {"id": "skip_intro", "name": "Skip intro logo videos",
         "desc": "Boots straight past the EA / Fox / Gracie logo movies.",
         "state": patch_skip_intro_state(), "available": patch_skip_intro_state() != "unavailable"},
        {"id": "fps_unlock", "name": "60 FPS mode",
         "desc": "Runs the game at 60 Hz instead of the original 30. Set it in "
                 "Settings → FRAMERATE. Experimental: cutscenes/physics may misbehave.",
         "state": "see settings", "available": False},
    ]


# ------------------------------------------------------------------ status

def game_running_pids():
    try:
        out = subprocess.run(["pgrep", "-f", str(GAME_BIN)], capture_output=True, text=True)
        return [int(p) for p in out.stdout.split()]
    except Exception:
        return []


def gamedata_ok():
    return (GAMEDATA / "movies").is_dir()


def art_files():
    return sorted(p.name for p in ART_DIR.glob("hero*.jpg")) if ART_DIR.is_dir() else []


def list_mods():
    MODS_DIR.mkdir(exist_ok=True)
    return sorted(p.name for p in MODS_DIR.iterdir() if not p.name.startswith("."))


def list_backups():
    BACKUPS_DIR.mkdir(exist_ok=True)
    return [{"name": p.name, "size_kb": p.stat().st_size // 1024,
             "date": time.strftime("%Y-%m-%d %H:%M", time.localtime(p.stat().st_mtime))}
            for p in sorted(BACKUPS_DIR.glob("saves-*.zip"), reverse=True)]


def status():
    return {
        "version": VERSION,
        "platform": PLAT,
        "engine_ready": GAME_BIN.exists(),
        "engine_date": time.strftime("%Y-%m-%d %H:%M", time.localtime(GAME_BIN.stat().st_mtime)) if GAME_BIN.exists() else None,
        "gamedata_ready": gamedata_ok(),
        "art": art_files(),
        "running": bool(game_running_pids()),
        "mods": list_mods(),
        "backups": list_backups(),
        "patches": patches_list(),
        "github_repo": CONFIG.get("github_repo", ""),
        "update": update_state,
        "steam_available": bool(shutil.which("steamos-add-to-steam") or shutil.which("steam")),
        "install": {"running": install_state["running"], "ok": install_state["ok"],
                    "log": install_state["log"][-40:]},
        "platforms": [
            {"name": "Linux / Steam Deck", "state": "ready"},
            {"name": "Windows", "state": "in development"},
            {"name": "Android", "state": "planned"},
        ],
    }


# ------------------------------------------------------------ file browser

BROWSE_ROOTS = [Path.home(), Path("/run/media"), Path("/media"), Path("/mnt")]


def browse(path_str):
    entries = []
    if not path_str:
        for r in BROWSE_ROOTS:
            if r.is_dir():
                entries.append({"name": str(r), "path": str(r), "dir": True})
        return {"path": "", "up": None, "entries": entries}
    p = Path(path_str).resolve()
    if not any(str(p).startswith(str(r)) for r in BROWSE_ROOTS):
        p = Path.home()
    try:
        for child in sorted(p.iterdir(), key=lambda c: (not c.is_dir(), c.name.lower())):
            if child.name.startswith("."):
                continue
            if child.is_dir():
                entries.append({"name": child.name, "path": str(child), "dir": True})
            elif child.suffix.lower() in (".iso", ".xiso", ".360", ".img"):
                entries.append({"name": child.name, "path": str(child), "dir": False,
                                "size_mb": child.stat().st_size // (1 << 20)})
    except PermissionError:
        pass
    up = str(p.parent) if p != p.parent else None
    return {"path": str(p), "up": up, "entries": entries[:400]}


# -------------------------------------------------------------------- art

def generate_art(force=False):
    if not gamedata_ok() or not shutil.which("ffmpeg"):
        return
    ART_DIR.mkdir(parents=True, exist_ok=True)
    if art_files() and not force:
        return
    movie_dir = GAMEDATA / "movies" / "en"
    # prefer HD in-game cutscenes, then any other movies (skip logo bumpers)
    igc = sorted(p for p in movie_dir.glob("*_igc*.vp6") if "_sd" not in p.name)
    rest = sorted(p for p in movie_dir.glob("*.vp6")
                  if "_sd" not in p.name and "logo" not in p.name and p not in igc)
    candidates = igc + rest
    for old_art in ART_DIR.glob("hero*.jpg"):
        old_art.unlink(missing_ok=True)
    made = 0
    for mv in candidates:
        if made >= 24:
            break
        for ts in ("2.0", "6.0", "12.0"):
            if made >= 24:
                break
            out = ART_DIR / f"hero{made}.jpg"
            r = subprocess.run(
                ["ffmpeg", "-loglevel", "error", "-y", "-ss", ts, "-i", str(mv),
                 "-frames:v", "1", "-q:v", "3", str(out)],
                capture_output=True, timeout=60)
            # size threshold filters black/flat frames
            if r.returncode == 0 and out.exists() and out.stat().st_size > 45000:
                made += 1
            else:
                out.unlink(missing_ok=True)


# ------------------------------------------------------------------- icon

def write_donut_icon(path, size=128):
    import math
    w = h = size
    cx = cy = size / 2
    R, r_hole = size * 0.42, size * 0.16
    spr = []
    for i in range(26):
        a = i * 2.399963
        rad = r_hole + (R - r_hole) * (0.35 + 0.45 * ((i * 37) % 10) / 10)
        spr.append((cx + math.cos(a) * rad, cy + math.sin(a) * rad, a,
                    [(255, 255, 255), (87, 185, 232), (255, 217, 15),
                     (124, 179, 66), (142, 36, 170)][i % 5]))
    rows = []
    for y in range(h):
        row = bytearray([0])
        for x in range(w):
            d = math.hypot(x - cx, y - cy)
            px = (0, 0, 0, 0)
            if r_hole <= d <= R:
                px = (232, 163, 61, 255)
                wave = math.sin(x * 0.35) * 2.5
                if d <= R * 0.94 and d >= r_hole * 1.08 and (y < cy + R * 0.28 + wave):
                    px = (240, 98, 146, 255)
                    for sx, sy, sa, col in spr:
                        dx, dy = x - sx, y - sy
                        u = dx * math.cos(sa) + dy * math.sin(sa)
                        v = -dx * math.sin(sa) + dy * math.cos(sa)
                        if abs(u) < 4 and abs(v) < 1.6:
                            px = (*col, 255)
                            break
                if d > R - 2 or d < r_hole + 2:
                    px = (35, 38, 41, 255)
            row += bytes(px)
        rows.append(bytes(row))
    raw = b"".join(rows)

    def chunk(tag, data):
        return (struct.pack(">I", len(data)) + tag + data
                + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))

    Path(path).write_bytes(b"\x89PNG\r\n\x1a\n"
                           + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
                           + chunk(b"IDAT", zlib.compress(raw, 9))
                           + chunk(b"IEND", b""))


def install_desktop_entry():
    icon = LAUNCHER_DIR / "icon.png"
    if not icon.exists():
        try:
            write_donut_icon(icon)
        except Exception:
            pass
    apps = Path.home() / ".local/share/applications"
    apps.mkdir(parents=True, exist_ok=True)
    (apps / "simpsons-recompiled.desktop").write_text(f"""[Desktop Entry]
Type=Application
Name=The Simpsons Game (Recompiled)
Comment=Launcher for the Simpsons Game native recompilation
Exec={LAUNCHER_DIR}/simpsons-launcher.sh
Icon={icon if icon.exists() else 'applications-games'}
Terminal=false
Categories=Game;
""")


# ---------------------------------------------------------------- install

def run_install(iso_path):
    install_state["running"] = True
    install_state["ok"] = None
    install_state["log"] = []
    log = install_state["log"]

    def fail(msg):
        log.append("ERROR: " + msg)
        install_state["ok"] = False
        install_state["running"] = False

    try:
        iso = Path(iso_path).expanduser()
        if not iso.is_file():
            return fail(f"ISO not found: {iso}")
        if not EXTRACT_XISO.exists():
            return fail("extract-xiso tool missing")
        target = ROOT / "gamedata_extracting"
        if target.exists():
            shutil.rmtree(target)
        target.mkdir()
        log.append(f"Extracting {iso.name} ... (this can take a few minutes)")
        # Options must precede the positional ISO: the Windows getopt does not
        # permute argv, so "-x <iso> -d <target>" would leave -d unparsed.
        p = subprocess.Popen([str(EXTRACT_XISO), "-x", "-d", str(target), str(iso)],
                             stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        for line in p.stdout:
            if line.strip():
                log.append(line.rstrip())
                del log[:-400]
        p.wait()
        if p.returncode != 0:
            return fail(f"extract-xiso exited with {p.returncode}")
        if not (target / "movies").is_dir():
            nested = [d for d in target.iterdir() if (d / "movies").is_dir()]
            if not nested:
                return fail("Extraction finished but game files not recognized "
                            "(is this The Simpsons Game Xbox 360 ISO?)")
            target = nested[0]
        log.append("Extraction complete. Installing game data ...")
        if GAMEDATA.exists():
            backup = ROOT / "gamedata_previous"
            if backup.exists():
                shutil.rmtree(backup)
            GAMEDATA.rename(backup)
            log.append("(previous game data kept as gamedata_previous)")
        target.rename(GAMEDATA)
        log.append("Generating launcher artwork from your game files ...")
        generate_art(force=True)
        log.append("DONE! The game is installed and ready to play.")
        install_state["ok"] = True
    except Exception as e:  # noqa: BLE001
        fail(str(e))
    finally:
        install_state["running"] = False


# ------------------------------------------------------------- save backup

def backup_saves():
    if not USER_DATA.exists():
        return False, "No save data found yet — play the game first!"
    BACKUPS_DIR.mkdir(exist_ok=True)
    name = time.strftime("saves-%Y%m%d-%H%M%S.zip")
    with zipfile.ZipFile(BACKUPS_DIR / name, "w", zipfile.ZIP_DEFLATED) as z:
        for f in USER_DATA.rglob("*"):
            rel = f.relative_to(USER_DATA)
            if f.is_file() and rel.parts and rel.parts[0] != "cache":
                z.write(f, rel)
    return True, name


def restore_saves(name):
    src = (BACKUPS_DIR / os.path.basename(name)).resolve()
    if not src.is_file() or src.parent != BACKUPS_DIR.resolve():
        return False, "backup not found"
    if game_running_pids():
        return False, "Stop the game before restoring saves"
    with zipfile.ZipFile(src) as z:
        z.extractall(USER_DATA)
    return True, "restored"


# ---------------------------------------------------------------- updates

def check_updates():
    repo = CONFIG.get("github_repo", "")
    if not repo:
        update_state.update(checked=True, update_available=False,
                            msg="No GitHub repository configured yet — coming with the open-source release.")
        return
    try:
        req = urllib.request.Request(
            f"https://api.github.com/repos/{repo}/releases/latest",
            headers={"User-Agent": f"simpsons-launcher/{VERSION}"})
        with urllib.request.urlopen(req, timeout=8) as r:
            data = json.loads(r.read())
        tag = data.get("tag_name", "")
        if tag and tag.lstrip("v") != VERSION:
            update_state.update(checked=True, update_available=True,
                                msg=f"Update available: {tag} — {data.get('html_url','')}")
        else:
            update_state.update(checked=True, update_available=False,
                                msg=f"You're up to date (latest release: {tag or 'none'}).")
    except Exception as e:  # noqa: BLE001
        update_state.update(checked=True, update_available=False,
                            msg=f"Update check failed: {e}")


# ----------------------------------------------------------------- launch

def repair_saves():
    """Self-heal 'damaged' save slots: the game truncates a slot to 0 bytes when
    it is killed mid-write (or mid-failed-load). Restore any zero-byte slot from
    the newest backup that holds a good copy; if none exists, remove the husk so
    the game sees a clean empty slot instead of a damaged one."""
    repaired = []
    try:
        profiles = list(USER_DATA.glob("*/45410809"))
        backups = sorted(BACKUPS_DIR.glob("*.zip"), key=lambda p: p.stat().st_mtime,
                         reverse=True) if BACKUPS_DIR.is_dir() else []
        # also treat the flatpak-originals folder backups as a source
        folder_backups = sorted(BACKUPS_DIR.glob("flatpak-originals-*"),
                                key=lambda p: p.stat().st_mtime, reverse=True)
        for prof in profiles:
            for slot_file in prof.glob("00000001/*/*"):
                if not slot_file.is_file() or slot_file.stat().st_size >= 4096:
                    continue
                slot_name = slot_file.name
                rel_data = f"{prof.parent.name}/45410809/00000001/{slot_name}/{slot_name}"
                rel_head = f"{prof.parent.name}/45410809/Headers/00000001/{slot_name}.header"
                fixed = False
                for zf in backups:
                    try:
                        with zipfile.ZipFile(zf) as z:
                            names = set(z.namelist())
                            if rel_data in names and z.getinfo(rel_data).file_size >= 4096:
                                slot_file.write_bytes(z.read(rel_data))
                                if rel_head in names:
                                    head = prof / "Headers/00000001" / f"{slot_name}.header"
                                    head.parent.mkdir(parents=True, exist_ok=True)
                                    head.write_bytes(z.read(rel_head))
                                repaired.append(f"{slot_name} (from {zf.name})")
                                fixed = True
                                break
                    except Exception:
                        continue
                if not fixed:
                    for fb in folder_backups:
                        src = fb / rel_data
                        srch = fb / rel_head
                        if src.is_file() and src.stat().st_size >= 4096:
                            slot_file.write_bytes(src.read_bytes())
                            if srch.is_file():
                                head = prof / "Headers/00000001" / f"{slot_name}.header"
                                head.write_bytes(srch.read_bytes())
                            repaired.append(f"{slot_name} (from {fb.name})")
                            fixed = True
                            break
                if not fixed:
                    # no good copy anywhere: remove the husk + header entirely
                    shutil.rmtree(slot_file.parent, ignore_errors=True)
                    (prof / "Headers/00000001" / f"{slot_name}.header").unlink(missing_ok=True)
                    repaired.append(f"{slot_name} (husk removed)")
    except Exception:
        pass
    return repaired


def auto_backup_saves():
    try:
        if not USER_DATA.exists():
            return
        BACKUPS_DIR.mkdir(exist_ok=True)
        name = time.strftime("auto-%Y%m%d-%H%M%S.zip")
        with zipfile.ZipFile(BACKUPS_DIR / name, "w", zipfile.ZIP_DEFLATED) as z:
            for f in USER_DATA.rglob("*"):
                rel = f.relative_to(USER_DATA)
                if f.is_file() and rel.parts and rel.parts[0] != "cache":
                    z.write(f, rel)
        autos = sorted(BACKUPS_DIR.glob("auto-*.zip"))
        for old_zip in autos[:-10]:
            old_zip.unlink()
    except Exception:
        pass


def _watch_game_exit(proc):
    """Record how the game ended; a GPU hang / kill mid-load can leave a corrupt
    shader cache behind, which then hangs the GPU on every later load. Purge the
    cache automatically after any abnormal exit."""
    code = proc.wait()
    try:
        (LAUNCHER_DIR / "last_exit.txt").write_text(str(code))
        if code != 0:
            good = BACKUPS_DIR / "goodcache"
            if good.is_dir():
                shutil.rmtree(USER_DATA / "cache", ignore_errors=True)
                shutil.copytree(good, USER_DATA / "cache")
                install_state["log"].append(
                    f"Game exited abnormally (code {code}) - known-good shader cache restored.")
    except Exception:
        pass


def launch_game():
    global game_proc
    with game_proc_lock:
        if game_running_pids():
            return False, "Game is already running"
        repaired = repair_saves()
        if repaired:
            install_state["log"].append("Save self-heal: " + ", ".join(repaired))
        auto_backup_saves()
        env = os.environ.copy()
        # Lossless Scaling frame-gen (lsfg-vk) hooks Vulkan via a RenderDoc-style
        # capture layer that deadlocks this game's FSI render path (GPU hang,
        # "device lost" on level load -- the 2026-07-05 crash saga). Hard-disable
        # it for the game regardless of the user's global LS settings.
        env["DISABLE_LSFG"] = "1"
        if patch_instant_popin_state() == "on":
            # capture a driver dump if the GPU ever hangs (black-box recorder;
            # remove once the priming-draw fix has a few clean sessions)
            env["RADV_DEBUG"] = "hang"
        libs = [str(_resolve(d)) for d in CONFIG["lib_dirs"].get(PLAT, [])]
        if libs:
            env["LD_LIBRARY_PATH"] = ":".join(libs) + ":" + env.get("LD_LIBRARY_PATH", "")
        cmd = [str(GAME_BIN), "--game_data_root", str(GAMEDATA)]
        debug_gdb = LAUNCHER_DIR / "debug_run.gdb"
        crash_log = LAUNCHER_DIR / "last_run_debug.txt"
        if debug_gdb.exists() and shutil.which("gdb"):
            # temporary diagnostics mode: capture a backtrace if the game crashes
            cmd = ["gdb", "-batch", "-x", str(debug_gdb), "--args"] + cmd
        if shutil.which("systemd-run"):
            # dedicated scope marked exempt from systemd-oomd pressure kills --
            # level-load memory spikes were getting the game SIGKILLed.
            cmd = ["systemd-run", "--user", "--scope", "--collect",
                   "--unit", "simpsons-game",
                   "-p", "ManagedOOMPreference=omit"] + cmd
        out = open(crash_log, "w") if debug_gdb.exists() else subprocess.DEVNULL
        game_proc = subprocess.Popen(cmd, cwd=str(BUILD_DIR), env=env,
                                     stdout=out, stderr=subprocess.STDOUT
                                     if debug_gdb.exists() else subprocess.DEVNULL)
        threading.Thread(target=_watch_game_exit, args=(game_proc,), daemon=True).start()
        return True, "launched"


def stop_game():
    global game_proc
    with game_proc_lock:
        if shutil.which("systemctl"):
            subprocess.run(["systemctl", "--user", "stop", "simpsons-game.scope"],
                           capture_output=True)
        if game_proc and game_proc.poll() is None:
            game_proc.kill()
            game_proc = None
            return True, "stopped"
        if not game_running_pids():
            return True, "stopped"
    return False, "not started by launcher"


def add_to_steam():
    script = LAUNCHER_DIR / "simpsons-launcher.sh"
    tool = shutil.which("steamos-add-to-steam")
    if tool:
        subprocess.Popen([tool, str(script)],
                         stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        return True, "Sent to Steam — check your library (may need a Steam restart)."
    if shutil.which("steam"):
        subprocess.Popen(["steam", f"steam://addnonsteamgame/{urllib.parse.quote(str(script))}"],
                         stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        return True, "Asked Steam to add the launcher — confirm the dialog in Steam."
    return False, ("Steam tools not found. Add manually: Steam → Games → "
                   f"Add a Non-Steam Game → browse to {script}")


def tail_game_log(n=60):
    logs = sorted((BUILD_DIR / "logs").glob("simpsons_*.log"),
                  key=lambda p: p.stat().st_mtime, reverse=True)
    if not logs:
        return []
    try:
        return logs[0].read_text(errors="replace").splitlines()[-n:]
    except Exception:
        return []


# ------------------------------------------------------------------ http

class Handler(http.server.BaseHTTPRequestHandler):
    def log_message(self, *a):
        pass

    def _send(self, code, body, ctype="application/json"):
        data = body if isinstance(body, bytes) else json.dumps(body).encode()
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self):
        parsed = urllib.parse.urlparse(self.path)
        path = parsed.path
        if path in ("/", "/index.html"):
            html = (UI_DIR / "index.html").read_text(encoding="utf-8").replace("__TOKEN__", TOKEN)
            return self._send(200, html.encode(), "text/html; charset=utf-8")
        if path == "/api/status":
            return self._send(200, status())
        if path == "/api/settings":
            return self._send(200, {"values": read_settings(),
                                    "restart_needed": [k for k, v in SETTINGS_SCHEMA.items() if v[2]]})
        if path == "/api/log":
            return self._send(200, {"lines": tail_game_log()})
        if path == "/api/browse":
            q = urllib.parse.parse_qs(parsed.query)
            return self._send(200, browse((q.get("path") or [""])[0]))
        if path.startswith("/art/"):
            f = (ART_DIR / os.path.basename(path)).resolve()
            if f.is_file() and f.parent == ART_DIR.resolve():
                return self._send(200, f.read_bytes(), "image/jpeg")
        if path == "/icon.png":
            f = LAUNCHER_DIR / "icon.png"
            if f.is_file():
                return self._send(200, f.read_bytes(), "image/png")
        return self._send(404, {"error": "not found"})

    def do_POST(self):
        if self.headers.get("X-Token") != TOKEN:
            return self._send(403, {"error": "bad token"})
        path = urllib.parse.urlparse(self.path).path
        length = int(self.headers.get("Content-Length") or 0)
        body = json.loads(self.rfile.read(length) or b"{}")
        if path == "/api/launch":
            ok, msg = launch_game()
            return self._send(200 if ok else 409, {"ok": ok, "msg": msg})
        if path == "/api/stop":
            ok, msg = stop_game()
            return self._send(200, {"ok": ok, "msg": msg})
        if path == "/api/settings":
            return self._send(200, {"values": write_settings(body.get("values", {}))})
        if path == "/api/install":
            if install_state["running"]:
                return self._send(409, {"ok": False, "msg": "install already running"})
            threading.Thread(target=run_install, args=(body.get("iso_path", ""),),
                             daemon=True).start()
            return self._send(200, {"ok": True})
        if path == "/api/regen-art":
            threading.Thread(target=generate_art, kwargs={"force": True}, daemon=True).start()
            return self._send(200, {"ok": True})
        if path == "/api/backup-saves":
            ok, msg = backup_saves()
            return self._send(200, {"ok": ok, "msg": msg})
        if path == "/api/restore-saves":
            ok, msg = restore_saves(body.get("name", ""))
            return self._send(200, {"ok": ok, "msg": msg})
        if path == "/api/add-to-steam":
            ok, msg = add_to_steam()
            return self._send(200, {"ok": ok, "msg": msg})
        if path == "/api/patch":
            if body.get("id") == "skip_intro":
                ok, msg = patch_skip_intro(bool(body.get("enable")))
                return self._send(200, {"ok": ok, "msg": msg})
            if body.get("id") == "instant_popin":
                if game_running_pids():
                    return self._send(200, {"ok": False, "msg": "close the game first"})
                ok, msg = patch_instant_popin(bool(body.get("enable")))
                return self._send(200, {"ok": ok, "msg": msg})
            return self._send(404, {"ok": False, "msg": "unknown patch"})
        if path == "/api/check-updates":
            threading.Thread(target=check_updates, daemon=True).start()
            return self._send(200, {"ok": True})
        return self._send(404, {"error": "not found"})


# --------------------------------------------------------------- frontend

def run_native(url):
    """Native desktop window via PySide6 QWebEngineView."""
    from PySide6.QtCore import QUrl
    from PySide6.QtGui import QIcon
    from PySide6.QtWebEngineWidgets import QWebEngineView
    from PySide6.QtWidgets import QApplication, QMainWindow

    app = QApplication(sys.argv)
    win = QMainWindow()
    win.setWindowTitle("The Simpsons Game — Recompiled")
    icon = LAUNCHER_DIR / "icon.png"
    if icon.exists():
        win.setWindowIcon(QIcon(str(icon)))
    view = QWebEngineView()
    view.setUrl(QUrl(url))
    win.setCentralWidget(view)
    win.resize(1180, 800)
    win.show()
    app.exec()


def open_browser(url):
    for browser in ("chromium", "chromium-browser", "google-chrome-stable", "brave"):
        exe = shutil.which(browser)
        if exe:
            subprocess.Popen([exe, f"--app={url}", "--window-size=1180,800"],
                             stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            return
    import webbrowser
    webbrowser.open(url)


def main():
    generate_art()
    install_desktop_entry()
    server = None
    for cand in range(PORT, PORT + 20):
        try:
            server = http.server.ThreadingHTTPServer(("127.0.0.1", cand), Handler)
            break
        except OSError:
            continue
    if server is None:
        server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    port = server.server_address[1]
    url = f"http://127.0.0.1:{port}/"
    print(f"Simpsons Launcher v{VERSION}: {url}")
    threading.Thread(target=server.serve_forever, daemon=True).start()
    threading.Thread(target=check_updates, daemon=True).start()

    if "--no-browser" in sys.argv:
        try:
            while True:
                time.sleep(3600)
        except KeyboardInterrupt:
            return
    try:
        run_native(url)          # real app window
    except Exception as e:       # noqa: BLE001
        print(f"(native window unavailable: {e}; falling back to browser)")
        open_browser(url)
        try:
            while True:
                time.sleep(3600)
        except KeyboardInterrupt:
            pass


if __name__ == "__main__":
    main()
