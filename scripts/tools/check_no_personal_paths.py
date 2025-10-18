#!/usr/bin/env python3
r"""
Fail if the repository contains obvious local user paths.

We flag common patterns that accidentally leak usernames, e.g.

`
C:\Users\alice\Projects\Foo
/home/bob/dev/foo
`

The check ignores files under .git/ and allows normalized Qt install hints
such as `C:/Qt`. Extend BANNED_PATTERNS if other variants show up.
"""

from __future__ import annotations

import pathlib
import re
import sys

REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]

# Pattern -> human-friendly message
BANNED_PATTERNS = {
    re.compile(r"C:\\Users\\[^\\/\s]+", re.IGNORECASE): "Windows user profile path",
    re.compile(r"C:/Users/[^\\/\s]+", re.IGNORECASE): "Windows user profile path",
    re.compile(r"/home/[^\\/\s]+", re.IGNORECASE): "POSIX user home directory",
}

ALLOWLIST = {
    pathlib.Path(".gitignore"),  # mentions C:/Qt, which is fine
}


def scan() -> list[str]:
    findings: list[str] = []
    for path in REPO_ROOT.rglob("*"):
        if path.is_dir():
            continue
        rel = path.relative_to(REPO_ROOT)
        if rel.parts and rel.parts[0] == ".git":
            continue
        if rel.as_posix() in {p.as_posix() for p in ALLOWLIST}:
            continue
        try:
            text = path.read_text(encoding="utf-8", errors="ignore")
        except OSError:
            continue
        for pattern, reason in BANNED_PATTERNS.items():
            match = pattern.search(text)
            if match:
                snippet = match.group(0)
                findings.append(f"{rel}: found {reason} -> '{snippet}'")
    return findings


def main() -> int:
    findings = scan()
    if not findings:
        return 0
    print("Personal path leak check failed:", file=sys.stderr)
    for entry in findings:
        print(f"  - {entry}", file=sys.stderr)
    print(
        "\nReplace hard-coded user-specific paths with repo-relative or generic examples.",
        file=sys.stderr,
    )
    return 1


if __name__ == "__main__":
    sys.exit(main())
