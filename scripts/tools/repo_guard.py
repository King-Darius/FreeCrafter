#!/usr/bin/env python3
r"""
Repository hygiene check that fails when tracked files contain obvious
user-specific absolute paths.

Examples of disallowed strings:

```
C:\Users\<user>\Projects\Foo
/home/<user>/dev/foo
```

Placeholder tokens such as `C:\Users\<user>` or `/home/<user>` are allowed.
"""

from __future__ import annotations

import pathlib
import re
import subprocess
import sys
from typing import Iterable

REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]

# Pattern -> human-friendly message
BANNED_PATTERNS = {
    re.compile(r"C:\\Users\\[^\\/\s]+", re.IGNORECASE): "Windows user-profile path",
    re.compile(r"C:/Users/[^\\/\s]+", re.IGNORECASE): "Windows user-profile path",
    re.compile(r"/home/[^\\/\s]+", re.IGNORECASE): "POSIX home directory path",
}


def tracked_files() -> Iterable[pathlib.Path]:
    """Yield repo-relative paths tracked by git."""
    result = subprocess.run(
        ["git", "ls-files"],
        cwd=REPO_ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    for line in result.stdout.splitlines():
        if not line:
            continue
        yield pathlib.Path(line)


def is_placeholder(snippet: str) -> bool:
    """Return True when the string looks like a placeholder (<user>, etc.)."""
    return any(ch in snippet for ch in "<>{}|[]")


def scan() -> list[str]:
    findings: list[str] = []
    for rel_path in tracked_files():
        abs_path = REPO_ROOT / rel_path
        try:
            text = abs_path.read_text(encoding="utf-8", errors="ignore")
        except OSError:
            continue
        for pattern, reason in BANNED_PATTERNS.items():
            for match in pattern.finditer(text):
                snippet = match.group(0)
                if is_placeholder(snippet):
                    continue
                findings.append(f"{rel_path}: {reason} -> '{snippet}'")
    return findings


def main() -> int:
    try:
        findings = scan()
    except subprocess.CalledProcessError as exc:  # pragma: no cover
        print(f"Repository hygiene check could not enumerate files: {exc}", file=sys.stderr)
        return 1
    if not findings:
        return 0
    print("Repository hygiene check failed:", file=sys.stderr)
    for entry in findings:
        print(f"  - {entry}", file=sys.stderr)
    print(
        "\nReplace absolute user-specific paths with repo-relative or generic examples.",
        file=sys.stderr,
    )
    return 1


if __name__ == "__main__":
    sys.exit(main())

