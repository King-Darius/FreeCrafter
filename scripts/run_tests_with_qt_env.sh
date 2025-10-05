#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'USAGE'
Usage: scripts/run_tests_with_qt_env.sh [--build-dir DIR] [--ctest [ARGS...]]

Ensures Qt runtime variables are exported before launching renderer tests.

Options:
  --build-dir DIR   Build directory containing CMake artifacts (default: build)
  --ctest [ARGS...] Invoke ctest with the provided arguments (defaults to --output-on-failure).
                    All arguments following --ctest are passed directly to ctest.
  -h, --help        Show this message and exit.
USAGE
}

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repo_root=$(cd "$script_dir/.." && pwd)
qt_base="$repo_root/qt/6.5.3"

build_dir="build"
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

case "$(uname -s)" in
  Darwin)
    qt_prefix="$qt_base/macos"
    qt_platform="cocoa"
    library_var=DYLD_LIBRARY_PATH
    ;;
  Linux)
    qt_prefix="$qt_base/gcc_64"
    qt_platform="offscreen"
    library_var=LD_LIBRARY_PATH
    ;;
  *)
    echo "error: unsupported platform '$(uname -s)'" >&2
    exit 1
    ;;
endcase

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
export QT_PLUGIN_PATH="$qt_plugins"
export QT_QPA_PLATFORM_PLUGIN_PATH="$qt_platform_plugins"
export QT_QPA_PLATFORM="$qt_platform"

pushd "$build_dir" >/dev/null

if (( use_ctest )); then
  if ! command -v ctest >/dev/null 2>&1; then
    echo "error: ctest was not found on PATH. Install CMake or ensure it is on PATH." >&2
    popd >/dev/null
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
      popd >/dev/null
      exit 1
    fi
  fi

  echo "$binary"
  "$binary"
fi

popd >/dev/null
