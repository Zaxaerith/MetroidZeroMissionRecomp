"""Run a native VBlank input route with isolated SRAM and optional pixel gates.

Captures mono PCM from the runtime mixer; does not certify host speakers or
controller devices. Acceptance always starts from reset.
"""
import argparse
import array
import bisect
import hashlib
import json
import math
import os
from pathlib import Path
import subprocess
import wave

from smoke_support import Client, ROOT, digest, png


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--bios", type=Path, required=True)
    p.add_argument("--save", type=Path, default=ROOT / "saves/metroid_zero_mission_usa.sav")
    p.add_argument("--route", type=Path, default=ROOT / "tests/routes/rooms.csv")
    p.add_argument("--checkpoints")
    p.add_argument("--out", type=Path, default=ROOT / "logs/gameplay-smoke")
    p.add_argument("--expected", type=Path)
    p.add_argument("--audio-start", type=int, default=6200)
    p.add_argument("--port", type=int, default=19946)
    p.add_argument("--no-wav", action="store_true", help="Verify complete PCM without retaining a duplicate WAV")
    a = p.parse_args()
    a.bios, a.save, a.out = a.bios.resolve(), a.save.resolve(), a.out.resolve()
    identity = json.loads((ROOT / "docs/ROM_IDENTITY.json").read_text())
    rom = ROOT / "Metroid - Zero Mission (USA).gba"
    for path, want in ((rom, identity["rom"]["sha1"]), (a.bios, identity["bios"]["sha1"])):
        if hashlib.sha1(path.read_bytes()).hexdigest() != want:
            raise RuntimeError(f"identity mismatch: {path}")
    fixture = a.save.read_bytes()
    if len(fixture) != 32768:
        raise RuntimeError("expected a 32 KiB SRAM fixture")
    events = []
    for line in a.route.read_text().splitlines():
        if line.strip() and not line.lstrip().startswith("#"):
            frame, key = (int(value.strip(), 0) for value in line.split(","))
            if frame < 0 or not 0 <= key <= 1023 or (events and frame <= events[-1][0]):
                raise RuntimeError("invalid/nonascending route")
            events.append((frame, key))
    if not events or events[0][0] != 0:
        raise RuntimeError("route must initialize keys at step zero")
    expected = json.loads(a.expected.read_text()) if a.expected else None
    point_spec = a.checkpoints or (",".join(expected["rgb_sha256"]) if expected else "1000,1300,6200,6520")
    points = sorted(set(map(int, point_spec.split(","))))
    if not points or points[0] < 0:
        raise RuntimeError("invalid checkpoints")
    if expected and set(points) != set(map(int, expected["rgb_sha256"])):
        raise RuntimeError("all manifest checkpoints must be checked")
    if expected and (expected["route_sha256"] != digest(a.route.read_bytes()) or
                     expected["fixture_sha256"] != digest(fixture)):
        raise RuntimeError("smoke route/fixture does not match manifest")
    a.out.mkdir(parents=True, exist_ok=True)
    test_save = a.out / "native-test.sav"
    if test_save == a.save:
        raise RuntimeError("output save collides with source fixture")
    test_save.write_bytes(fixture)
    env = {k: v for k, v in os.environ.items() if not k.startswith("GBARECOMP_")}
    env.update(GBARECOMP_BIOS_HLE="0", GBARECOMP_BIOS_SKIP_INTRO="0",
               GBARECOMP_FORCE_INTERP="0", GBARECOMP_STRICT_STATIC="0",
               GBARECOMP_SELFHEAL_RECOMPILE="0")
    env["PATH"] = str(ROOT / "build/host") + os.pathsep + env["PATH"]
    exe = ROOT / "build/host/MetroidZeroMissionRecomp.exe"
    result = dict(rom_sha1=identity["rom"]["sha1"], bios_sha1=identity["bios"]["sha1"],
                  exe_sha256=digest(exe.read_bytes()), fixture_sha256=digest(fixture),
                  route_sha256=digest(a.route.read_bytes()), mode="static+bridge; healing disabled",
                  phase="completed VBlank calls; input before next step", debug_state=False,
                  checkpoints=[], status="RUNNING")
    pcm = bytearray()
    rate = None
    client = None
    with (a.out / "native.log").open("wb") as log:
        process = subprocess.Popen([str(exe), "--rom", str(rom), "--bios", str(a.bios),
                                    "--no-window", "--tcp", str(a.port), "--save-path", str(test_save)],
                                   cwd=ROOT, env=env, stdout=log, stderr=log,
                                   creationflags=subprocess.CREATE_NO_WINDOW)
        try:
            client = Client(a.port, process)
            current_key = None
            event_frames = [event[0] for event in events]
            for step in range(points[-1] + 1):
                if step in points:
                    shot = client.call("screenshot")
                    if (shot["w"], shot["h"]) != (240, 160):
                        raise RuntimeError("unexpected framebuffer geometry")
                    rgb = bytes.fromhex(shot["data"])
                    png(a.out / f"frame-{step}.png", rgb)
                    row = dict(step=step, rgb_sha256=digest(rgb), ppu=client.call("ppu_state"))
                    if expected:
                        row["matches"] = row["rgb_sha256"] == expected["rgb_sha256"].get(str(step))
                    result["checkpoints"].append(row)
                    print(json.dumps(dict(step=step, rgb_sha256=row["rgb_sha256"],
                                          vcount=row["ppu"]["vcount"], matches=row.get("matches"))), flush=True)
                    (a.out / "result.json").write_text(json.dumps(result, indent=2) + "\n")
                # Drain often enough to preserve the FIFO, discarding boot PCM.
                if step % 8 == 0 or step == points[-1]:
                    audio = client.call("audio_samples", count=16384)
                    if step > a.audio_start:
                        if rate is not None and rate != audio["rate"]:
                            raise RuntimeError("sample rate changed during captured segment")
                        rate = audio["rate"]
                        pcm.extend(bytes.fromhex(audio["data"]))
                if step == points[-1]:
                    break
                key = events[bisect.bisect_right(event_frames, step) - 1][1]
                if key != current_key:
                    client.call("set_keyinput", value=key)
                    current_key = key
                client.call("step")
            result["coverage"] = client.call("misses")
            client.call("quit")
            client.close()
            client = None
            if process.wait(timeout=10) != 0:
                raise RuntimeError(f"native process failed: {process.returncode}")
        finally:
            if client:
                try:
                    client.call("quit")
                except (OSError, ValueError, RuntimeError):
                    pass
                client.close()
            if process.poll() is None:
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()
            if digest(a.save.read_bytes()) != digest(fixture):
                raise RuntimeError("source save fixture changed")
    samples = array.array("h", pcm)
    result["audio"] = dict(rate=rate, channels=1, samples=len(samples), pcm_sha256=digest(pcm),
                           nonzero=sum(value != 0 for value in samples),
                           peak=max((abs(value) for value in samples), default=0),
                           rms=math.sqrt(sum(value * value for value in samples) / len(samples)) if samples else 0,
                           clipped=sum(value in (-32768, 32767) for value in samples))
    if rate is None:
        raise RuntimeError("no audio segment captured; end checkpoint must follow audio start")
    if not a.no_wav:
        with wave.open(str(a.out / "gameplay.wav"), "wb") as wav:
            wav.setnchannels(1)
            wav.setsampwidth(2)
            wav.setframerate(rate)
            wav.writeframes(pcm)
    result["final_save_sha256"] = digest(test_save.read_bytes())
    save_ok = True
    if expected and "final_save_sha256" in expected:
        save_ok = result["final_save_sha256"] == expected["final_save_sha256"]
        result["save_matches"] = save_ok
    # A signal-presence gate, not a listening/fidelity test. Isolated peaks
    # can occur in legitimate effects; reject sustained clipping above 0.1%.
    result["audio"]["clipped_fraction"] = result["audio"]["clipped"] / len(samples) if samples else 0
    audio_ok = (len(samples) >= rate and result["audio"]["nonzero"] > len(samples) * 0.01
                and result["audio"]["clipped_fraction"] <= 0.001)
    result["audio"]["signal_gate"] = audio_ok
    if expected and "pcm_sha256" in expected:
        result["audio"]["matches"] = result["audio"]["pcm_sha256"] == expected["pcm_sha256"]
        audio_ok = audio_ok and result["audio"]["matches"]
    result["status"] = ("PASS" if audio_ok and save_ok and all(row["matches"] for row in result["checkpoints"])
                        else "FAIL") if expected else "CAPTURED"
    (a.out / "result.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(dict(status=result["status"], save_matches=result.get("save_matches"), audio=result["audio"],
                          distinct_misses=result["coverage"]["distinct_misses"],
                          interpreted=result["coverage"]["interpreted_insns"])), flush=True)
    return 2 if result["status"] == "FAIL" else 0


if __name__ == "__main__":
    raise SystemExit(main())
