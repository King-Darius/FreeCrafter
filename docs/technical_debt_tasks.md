# Follow-up Maintenance Tasks

The following technical-debt cleanups remain outstanding. They focus on deduplicating logic and sharing camera utilities to keep behavior consistent across tools.

## HotkeyManager JSON Serialization
- **Problem:** `HotkeyManager::importFromFile`, `exportToFile`, `load`, and `save` each reconstruct the JSON document independently, repeating the same loops and key names.
- **Suggested fix:** Introduce shared helpers (e.g., `readShortcuts(const QJsonObject&)` and `writeShortcuts(QJsonObject&)`) that encapsulate reading/writing the schema. Refactor the four entry points to rely on the helpers so the schema is defined in one place.
- **Benefit:** Prevents drift when adding new shortcut metadata and centralizes version handling.

## Share Camera Ray Construction
- **Problem:** `MoveTool::pointerToWorld` relies on a local `pointerToGround` helper duplicating the math that already exists in `CameraNavigation::computeRay`.
- **Suggested fix:** Expose a reusable ray-construction helper in `CameraNavigation.h/.cpp` (either by making `computeRay` public or adding a new function). Update `MoveTool` to call the shared helper before intersecting with the ground plane.
- **Benefit:** Guarantees pointer picking uses the same projection math as navigation features and avoids maintenance of two separate implementations.

## Unify Pointer Drag State in Navigation Tools
- **Problem:** `PanTool` and `OrbitTool` reimplement the same pointer bookkeeping (`dragging`, last pointer position, scaling) in each class.
- **Suggested fix:** Add a shared base or mixin that stores the pointer state and exposes callbacks for tool-specific behavior; refactor both tools to delegate to the shared implementation.
- **Benefit:** Makes future adjustments to pointer handling (e.g., pixel scaling or cancel behavior) consistent across tools.
