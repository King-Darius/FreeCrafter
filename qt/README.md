# Vendored Qt Runtime

This directory holds the Qt runtime that ships alongside FreeCrafter builds. The
runtime is not committed to the repository, but the `manifest.json` file
records the version and modules that should be fetched. Run
`scripts/fetch_qt_runtime.py` to download or update the runtime using the
configuration in the manifest.

```
python3 scripts/fetch_qt_runtime.py
```

The script will download Qt into `qt/<version>/<arch>` and refresh the manifest
with metadata about the retrieved packages. Generated binaries or libraries
should not be checked into git.
