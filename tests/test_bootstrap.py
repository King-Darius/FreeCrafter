import hashlib
import sys
import builtins
import logging
import subprocess
from pathlib import Path

import pytest

# Import the bootstrap module from the scripts directory
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
import bootstrap  # noqa: E402


@pytest.fixture
def no_aqt(monkeypatch):
    real_import = builtins.__import__

    def fake_import(name, globals=None, locals=None, fromlist=(), level=0):
        if name == "aqtinstall":
            raise ImportError()
        return real_import(name, globals, locals, fromlist, level)

    monkeypatch.setattr(builtins, "__import__", fake_import)


def test_verify_checksums_valid(tmp_path):
    wheel = tmp_path / "pkg.whl"
    wheel.write_bytes(b"data")
    digest = hashlib.sha256(b"data").hexdigest()
    (tmp_path / "checksums.txt").write_text(f"{digest} {wheel.name}\n")
    bootstrap.verify_checksums(tmp_path)


def test_verify_checksums_invalid(tmp_path):
    wheel = tmp_path / "pkg.whl"
    wheel.write_bytes(b"data")
    wrong_digest = "0" * 64
    (tmp_path / "checksums.txt").write_text(f"{wrong_digest} {wheel.name}\n")
    with pytest.raises(RuntimeError):
        bootstrap.verify_checksums(tmp_path)


def test_verify_checksums_missing_checksum_file(tmp_path):
    bootstrap.verify_checksums(tmp_path)


def test_verify_checksums_missing_directory(tmp_path):
    missing = tmp_path / "missing"
    bootstrap.verify_checksums(missing)


def test_verify_checksums_malformed_line(tmp_path):
    wheel = tmp_path / "pkg.whl"
    wheel.write_bytes(b"data")
    # Missing filename entry -> line cannot be split into digest/filename
    (tmp_path / "checksums.txt").write_text("incomplete\n")
    with pytest.raises(ValueError):
        bootstrap.verify_checksums(tmp_path)


def test_verify_checksums_comment_lines(tmp_path):
    wheel = tmp_path / "pkg.whl"
    wheel.write_bytes(b"data")
    digest = hashlib.sha256(b"data").hexdigest()
    (tmp_path / "checksums.txt").write_text(
        "# comment\n" f"{digest} {wheel.name}\n"
    )
    bootstrap.verify_checksums(tmp_path)


def test_run_retries(monkeypatch):
    calls = {"count": 0}

    def fake_run(cmd, **kwargs):
        calls["count"] += 1
        class Result:
            returncode = 1
        return Result()

    monkeypatch.setattr(bootstrap.subprocess, "run", fake_run)
    monkeypatch.setattr(bootstrap.time, "sleep", lambda _s: None)

    rc = bootstrap.run(["echo", "hi"], retries=3)
    assert rc == 1
    assert calls["count"] == 3


def test_detect_qt_from_env(monkeypatch, tmp_path):
    bin_dir = tmp_path / "bin"
    bin_dir.mkdir()
    qtpaths = "qtpaths.exe" if sys.platform == "win32" else "qtpaths"
    (bin_dir / qtpaths).touch()
    monkeypatch.setenv("CMAKE_PREFIX_PATH", str(tmp_path))
    assert bootstrap.detect_qt() == tmp_path


def test_ensure_qt_offline_missing(monkeypatch, tmp_path):
    monkeypatch.setattr(bootstrap, "install_dir", tmp_path / "missing")
    for var in ("CMAKE_PREFIX_PATH", "QT_PREFIX_PATH", "QTDIR"):
        monkeypatch.delenv(var, raising=False)
    with pytest.raises(RuntimeError):
        bootstrap.ensure_qt(True)


def test_ensure_aqt_offline_missing_cache(monkeypatch, no_aqt):
    called = {"run": False}

    def fake_run(cmd, **kwargs):
        called["run"] = True
        return 0

    monkeypatch.setattr(bootstrap, "run", fake_run)
    with pytest.raises(RuntimeError):
        bootstrap.ensure_aqt(True, None)
    assert not called["run"]


def test_ensure_aqt_offline_uses_cache(monkeypatch, tmp_path, no_aqt):
    wheel_cache = tmp_path / "cache"
    wheel_cache.mkdir()

    checks = {"called": False}

    def fake_verify(cache):
        checks["called"] = True
        assert cache == wheel_cache

    monkeypatch.setattr(bootstrap, "verify_checksums", fake_verify)

    captured = {}

    def fake_run(cmd, **kwargs):
        captured["cmd"] = cmd
        return 0

    monkeypatch.setattr(bootstrap, "run", fake_run)

    bootstrap.ensure_aqt(True, wheel_cache)

    assert checks["called"]
    cmd = captured["cmd"]
    assert "--no-index" in cmd
    assert "--find-links" in cmd
    assert str(wheel_cache) in cmd


def test_ensure_aqt_online_user_flag(monkeypatch, no_aqt):
    checks = {"called": False}

    def fake_verify(cache):
        checks["called"] = True

    monkeypatch.setattr(bootstrap, "verify_checksums", fake_verify)

    captured = {}

    def fake_run(cmd, **kwargs):
        captured["cmd"] = cmd
        return 0

    monkeypatch.setattr(bootstrap, "run", fake_run)

    bootstrap.ensure_aqt(False, None)

    assert not checks["called"]
    cmd = captured["cmd"]
    assert "--user" in cmd
    assert "--no-index" not in cmd


def test_main_success(monkeypatch):
    calls = []

    def fake_ensure_aqt(offline, wheel_cache):
        calls.append(("aqt", offline, wheel_cache))

    def fake_ensure_qt(offline):
        calls.append(("qt", offline))
        return Path("/qt")

    def fake_run_cmake(prefix):
        calls.append(("cmake", prefix))

    monkeypatch.setattr(bootstrap, "ensure_aqt", fake_ensure_aqt)
    monkeypatch.setattr(bootstrap, "ensure_qt", fake_ensure_qt)
    monkeypatch.setattr(bootstrap, "run_cmake", fake_run_cmake)

    rc = bootstrap.main(["--offline"])
    assert rc == 0
    assert calls == [("aqt", True, None), ("qt", True), ("cmake", Path("/qt"))]


def test_main_subprocess_error(monkeypatch, caplog):
    def fake_ensure_aqt(offline, wheel_cache):
        pass

    def fake_ensure_qt(offline):
        return Path("/qt")

    def fake_run_cmake(prefix):
        raise subprocess.CalledProcessError(2, ["cmake"])

    monkeypatch.setattr(bootstrap, "ensure_aqt", fake_ensure_aqt)
    monkeypatch.setattr(bootstrap, "ensure_qt", fake_ensure_qt)
    monkeypatch.setattr(bootstrap, "run_cmake", fake_run_cmake)

    caplog.set_level(logging.ERROR)
    rc = bootstrap.main([])
    assert rc == 2
    assert "Command 'cmake' failed with code 2" in caplog.text


def test_main_exception_logging(monkeypatch, caplog):
    def fake_ensure_aqt(offline, wheel_cache):
        pass

    def fake_ensure_qt(offline):
        raise RuntimeError("boom")

    monkeypatch.setattr(bootstrap, "ensure_aqt", fake_ensure_aqt)
    monkeypatch.setattr(bootstrap, "ensure_qt", fake_ensure_qt)

    caplog.set_level(logging.ERROR)
    rc = bootstrap.main([])
    assert rc == 1
    assert "boom" in caplog.text
