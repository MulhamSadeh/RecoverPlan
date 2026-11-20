"""End-to-end CLI smoke tests for RecoverPlan's MVP.

Run via: ctest --test-dir build --output-on-failure
No real student data is used; state files are created in isolated temp dirs.
"""
import pathlib
import subprocess
import sys
import tempfile


def run(binary, cwd, commands):
    completed = subprocess.run(
        [str(binary)], input="\n".join(commands) + "\n", text=True,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=cwd, timeout=15,
    )
    assert completed.returncode == 0, completed.stderr[-3000:]
    return completed.stdout


def parse_sessions(save_path):
    lines = save_path.read_text().splitlines()
    header = next(i for i, line in enumerate(lines) if line.startswith("SESSIONS "))
    count = int(lines[header].split()[1])
    return [line.split("|") for line in lines[header + 1:header + 1 + count]]


def main():
    binary = pathlib.Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory(prefix="recoverplan_smoke_") as tmp:
        wd = pathlib.Path(tmp)
        csv_file = wd / "tasks.csv"
        csv_file.write_text(
            "subject,task,type,difficulty,importance,estimatedTime,pages,days\n"
            "OOP,Implement model,Project,7,9,3,0,2\n",
            encoding="utf8",
        )
        out = run(binary, wd, ["1", "Demo Student", "4", "2", "7", "4", str(csv_file), "8", "1", "10", "0"])
        assert "Imported: 1" in out, out[-3000:]
        assert "Using student estimates" in out, out[-3000:]
        assert "Study Schedule" in out.upper().title() or "SCHEDULE" in out, out[-3000:]

        saved = wd / "recoverplan_save.dat"
        assert saved.exists()
        parts = saved.read_text().splitlines()[1].split("|")
        assert len(parts) == 6 and int(parts[5]) == 2, parts
        sessions = parse_sessions(saved)
        assert len(sessions) == 2, sessions
        assert [int(s[4]) for s in sessions] == [1, 1], sessions
        assert [float(s[5]) for s in sessions] == [2.0, 1.0], sessions

        # Reopen saved state, record the first planned session as missed,
        # then recover its hours on the following day.
        out = run(binary, wd, ["11", "3", "4", "12", "10", "0"])
        assert "Previous session loaded: Demo Student" in out
        assert "Recovery" in out or "recovery" in out
        sessions = parse_sessions(saved)
        recovery_sessions = [s for s in sessions if int(s[4]) == 2 and float(s[5]) > 0]
        assert recovery_sessions, sessions
        assert all(float(s[5]) <= 2.00001 for s in sessions), sessions
        for day in range(1, 8):
            active = [s for s in sessions if int(s[4]) == day and float(s[5]) > 0.00001]
            assert len(active) <= 2, (day, active)

        # Old five-field student state must continue to load using 1 session/day.
        data = saved.read_text().splitlines()
        data[1] = "|".join(data[1].split("|")[:5])
        saved.write_text("\n".join(data) + "\n")
        out = run(binary, wd, ["0"])
        assert "Previous session loaded: Demo Student" in out
        assert "1 sessions/day" in out
    print("PASS: import, scheduling limits, saved state, recovery, legacy save")


if __name__ == "__main__":
    main()
