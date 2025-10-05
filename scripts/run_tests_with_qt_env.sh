#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'USAGE'
Usage: scripts/run_tests_with_qt_env.sh [--build-dir DIR] [--qt-prefix PATH] [--ctest [ARGS...]]

Ensures Qt runtime variables are exported before launching renderer tests.

Options:
  --build-dir DIR   Build directory containing CMake artifacts (default: build)
  --qt-prefix PATH  Override the Qt installation prefix. Defaults to the prefix
                    detected by scripts/bootstrap.py (or qt/<version>/<arch>).
  --ctest [ARGS...] Invoke ctest with the provided arguments (defaults to --output-on-failure).
                    All arguments following --ctest are passed directly to ctest.
  -h, --help        Show this message and exit.
USAGE
}

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repo_root=$(cd "$script_dir/.." && pwd)

build_dir="build"
qt_prefix_override=""
use_ctest=0
ctest_args=()

while (($#)); do
  case "$1" in
    --build-dir)
      if (($# < 2)); then
        echo "error: --build-dir requires a value" >&2
        exit 2
      fi
      build_dir="$2"
      shift 2
      ;;
    --qt-prefix)
      if (($# < 2)); then
        echo "error: --qt-prefix requires a value" >&2
        exit 2
      fi
      qt_prefix_override="$2"
      shift 2
      ;;
    --ctest)
      use_ctest=1
      shift
      ctest_args=("$@")
      break
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "error: unknown option '$1'" >&2
      usage >&2
      exit 2
      ;;
  esac
done

find_python() {
  if command -v python3 >/dev/null 2>&1; then
    echo python3
  elif command -v python >/dev/null 2>&1; then
    echo python
  else
    echo "error: python3 (or python) is required to detect the Qt prefix" >&2
    exit 1
  fi
}

if [[ -n "$qt_prefix_override" ]]; then
  if [[ "${qt_prefix_override:0:1}" != "/" ]]; then
    qt_prefix_override="$repo_root/$qt_prefix_override"
  fi
  if [[ ! -d "$qt_prefix_override" ]]; then
    echo "error: --qt-prefix path '$qt_prefix_override' does not exist" >&2
    exit 1
  fi
  qt_prefix=$(cd "$qt_prefix_override" && pwd)
  qt_version="custom"
else
  python_bin=$(find_python)
  export FREECRAFTER_BOOTSTRAP_DIR="$script_dir"
  qt_output=$("$python_bin" - <<'PY'
import os
from pathlib import Path
import sys

script_dir = Path(os.environ["FREECRAFTER_BOOTSTRAP_DIR"])
sys.path.insert(0, str(script_dir))

try:
    import bootstrap
except Exception as exc:  # pragma: no cover - propagate to shell
    print(f"failed to import bootstrap: {exc}", file=sys.stderr)
    sys.exit(1)

prefix = bootstrap.detect_qt()
if prefix is None:
    prefix = bootstrap.install_dir

print(prefix)
print(bootstrap.QT_VERSION)
PY
  )
  status=$?
  unset FREECRAFTER_BOOTSTRAP_DIR
  if ((status != 0)); then
    echo "error: unable to determine Qt prefix via scripts/bootstrap.py" >&2
    exit 1
  fi
  qt_output=${qt_output//$'\r'/}
  IFS=$'\n' read -r qt_prefix qt_version <<<"$qt_output"
  if [[ -z "$qt_prefix" || -z "$qt_version" ]]; then
    echo "error: scripts/bootstrap.py did not provide Qt metadata" >&2
    exit 1
  fi
  if [[ "${qt_prefix:0:1}" != "/" ]]; then
    qt_prefix="$repo_root/$qt_prefix"
  fi
  if [[ ! -d "$qt_prefix" ]]; then
    echo "error: Qt runtime not found at $qt_prefix (expected from scripts/bootstrap.py)" >&2
    echo "Run scripts/bootstrap.py or pass --qt-prefix." >&2
    exit 1
  fi
  qt_prefix=$(cd "$qt_prefix" && pwd)
fi

case "$(uname -s)" in
  Darwin)
    qt_platform="cocoa"
    library_var=DYLD_LIBRARY_PATH
    framework_var=DYLD_FRAMEWORK_PATH
    ;;
  Linux)
    qt_platform="offscreen"
    library_var=LD_LIBRARY_PATH
    framework_var=""
    ;;
  *)
    echo "error: unsupported platform '$(uname -s)'" >&2
    exit 1
    ;;
esac

qt_bin="$qt_prefix/bin"
qt_plugins="$qt_prefix/plugins"
qt_platform_plugins="$qt_plugins/platforms"
qt_lib="$qt_prefix/lib"

if [[ ! -d "$qt_bin" ]]; then
  echo "error: Qt runtime not found at $qt_bin. Run scripts/bootstrap.py first." >&2
  exit 1
fi
if [[ ! -d "$qt_plugins" ]]; then
  echo "error: Qt plugins not found at $qt_plugins. Run scripts/bootstrap.py first." >&2
  exit 1
fi
if [[ ! -d "$qt_platform_plugins" ]]; then
  echo "error: Qt platform plugins not found at $qt_platform_plugins. Run scripts/bootstrap.py first." >&2
  exit 1
fi

if [[ ! -d "$qt_lib" ]]; then
  echo "error: Qt libraries not found at $qt_lib. Run scripts/bootstrap.py first." >&2
  exit 1
fi

if [[ "${qt_version:-}" == "custom" ]]; then
  echo "Using Qt prefix: $qt_prefix"
else
  echo "Using Qt prefix: $qt_prefix (Qt $qt_version)"
fi

if [[ "${build_dir:0:1}" != "/" ]]; then
  build_dir="$repo_root/$build_dir"
fi

if [[ ! -d "$build_dir" ]]; then
  echo "error: build directory '$build_dir' does not exist." >&2
  exit 1
fi

export PATH="$qt_bin:$PATH"
if [[ -n "${!library_var-}" ]]; then
  export "$library_var=$qt_lib:${!library_var}"
else
  export "$library_var=$qt_lib"
fi
if [[ -n "${framework_var-}" ]]; then
  if [[ -n "${!framework_var-}" ]]; then
    export "$framework_var=$qt_lib:${!framework_var}"
  else
    export "$framework_var=$qt_lib"
  fi
fi
if [[ -n "${QT_PLUGIN_PATH-}" ]]; then
  export QT_PLUGIN_PATH="$qt_plugins:$QT_PLUGIN_PATH"
else
  export QT_PLUGIN_PATH="$qt_plugins"
fi
if [[ -n "${QT_QPA_PLATFORM_PLUGIN_PATH-}" ]]; then
  export QT_QPA_PLATFORM_PLUGIN_PATH="$qt_platform_plugins:$QT_QPA_PLATFORM_PLUGIN_PATH"
else
  export QT_QPA_PLATFORM_PLUGIN_PATH="$qt_platform_plugins"
fi
export QT_QPA_PLATFORM="$qt_platform"

pushd "$build_dir" >/dev/null
cleanup() {
  popd >/dev/null
}
trap cleanup EXIT

if (( use_ctest )); then
  if ! command -v ctest >/dev/null 2>&1; then
    echo "error: ctest was not found on PATH. Install CMake or ensure it is on PATH." >&2
    exit 1
  fi

  has_output=0
  has_config=0
  for arg in "${ctest_args[@]}"; do
    case "$arg" in
      --output-on-failure)
        has_output=1
        ;;
      -C|--build-config|--build-config=*)
        has_config=1
        ;;
      -C=*)
        has_config=1
        ;;
    esac
  done

  if (( ! has_output )); then
    ctest_args=("--output-on-failure" "${ctest_args[@]}")
  fi

  if (( ! has_config )); then
    config="${CTEST_CONFIGURATION_TYPE-}"
    if [[ -z "$config" && -f "CMakeCache.txt" ]]; then
      if grep -q '^CMAKE_CONFIGURATION_TYPES:' CMakeCache.txt; then
        config="Release"
      fi
    fi
    if [[ -n "$config" ]]; then
      ctest_args+=('-C' "$config")
    fi
  fi

  export QT_DEBUG_PLUGINS=1
  export FREECRAFTER_RENDER_SKIP_COVERAGE=1

  echo "ctest ${ctest_args[*]}"
  ctest "${ctest_args[@]}"
  else
    binary="./test_render"
    if [[ ! -x "$binary" ]]; then
      mac_binary="./test_render.app/Contents/MacOS/test_render"
      if [[ -x "$mac_binary" ]]; then
        binary="$mac_binary"
      else
        echo "error: test binary not found at $binary" >&2
        exit 1
      fi
    fi

    echo "$binary"
    "$binary"
  fi
