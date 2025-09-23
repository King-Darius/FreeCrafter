# Design Foundation — Layout & Visual Tokens

## Layout Translation from Figma
- **Viewport targets:** 1280×800 minimum, 1440×900 reference, 1920×1080 stretched.
- **Structure:** Native menu bar [A], 48px primary toolbar [B], 56px tool ribbon [C], tabbed viewport host [D], right dock tabs [E], 28px status bar [F].
- **Spacing system:** 8px base grid with 4px micro-adjustments (4, 8, 12, 16, 24, 32 multiples).
- **Responsive rules:** Left ribbon locks at 56px; right dock snaps between 280–420px; central viewport flexes and exposes scrollable tab bar when tabs exceed width.

## Visual Tokens
| Token | Dark | Light | Usage |
| --- | --- | --- | --- |
| `--bg` | `#0B0D10` | `#FFFFFF` | Main window background |
| `--panel` | `#0F1216` | `#F7F7F8` | Toolbars, docks |
| `--border` | `rgba(255,255,255,.08)` | `rgba(0,0,0,.08)` | Toolbar/dock separators |
| `--fg` | `#E7EAF0` | `#111827` | Primary text/icon |
| `--fg-muted` | 60% alpha of `--fg` | 60% alpha of `--fg` | Secondary labels |
| `--accent` | `#6AA9FF` | `#2563EB` | Focus rings, active states |
| `--success` | `#2ECC71` | `#15803D` | Task complete pill |
| `--warning` | `#F4BF50` | `#CA8A04` | Long-running task |
| `--error` | `#EF5350` | `#DC2626` | Failure pill |

## Component Guidelines
- **Toolbar buttons:** SVG icons sized 20×20 within a 32×32 hit target. Tooltips show name + shortcut.
- **Tab bar:** 36px height, 6px radius corners, bullet (•) dirty indicator appended to text when unsaved.
- **Status bar:** Left = cursor coordinates, center = selection summary + measurement entry, right = task pill with subtle shadow.
- **Focus states:** 2px accent outline offset by 2px for keyboard focus. Hover states lighten backgrounds by 10% alpha.

## Assets & Fonts
- **Icons:** Flowbite outline placeholders shipped as `resources/icons/*.svg`; swap-in ready for final art.
- **Typography:** Inter > system fallback; 14pt base, 12pt for tabs/status, uppercase label style for panel headings.
- **Shadows:** Panels cast `0 6px 24px rgba(0,0,0,0.24)` in dark theme, `0 12px 24px rgba(17,24,39,0.12)` in light.

## Deliverables
1. Application stylesheet (`:/styles/app.qss`) encoding tokens above.
2. Toolbar/ribbon construction instructions (implemented in Phase 1 code scaffold).
3. Persistent dock/tab state stored in `QSettings` for consistent reopening.
4. Theme toggle action to swap dark/light palettes in under 50ms.
