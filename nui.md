# NUI

Browser-rendered UI inside MTA. One HTML page per resource, FiveM-style.

## Concept

Each resource declares one `<nui src="…"/>` in its `meta.xml`. On resource start the engine spawns the page as a transparent layer above the game. Visibility, focus, and messaging are driven from Lua. Two-way communication: events for fire-and-forget, RPC for `await`-style request/response.

## `<nui>` meta tag

```xml
<nui src="<path>" [z="N"] [hidden="true"] />
```

| Attribute | Default | Meaning |
|---|---|---|
| `src` | required | HTML path inside the resource |
| `z` | `0` | CSS z-index against other resources' NUIs |
| `hidden` | `false` | Spawn hidden; first `showNuiFrame` reveals it. Page loads either way — JS is alive while hidden. |

Only one frame per resource. Need additional surfaces? Use views inside the same HTML and switch with messages, or `location.href` to navigate.

## Lua API

```lua
showNuiFrame()                   -- show this resource's NUI
hideNuiFrame()                   -- hide it (JS keeps running, no remount)
setNuiFocus(enable [, keepInput])
                                 -- route input to the focused NUI
                                 -- keepInput=true lets the game also receive input
sendNuiMessage(table)            -- Lua → JS
registerNuiCallback(type, fn)    -- handler for nui.fetch RPC
isNuiFocused()                   -- bool: any NUI focused?
toggleNuiDevTools(visible)       -- chromium devtools window
```

Cursor visibility is **not** managed by `setNuiFocus`. Call `showCursor` from Lua when you want the OS cursor visible. `setNuiFocus` only routes input.

## Lua events

| Event | Args | Fires |
|---|---|---|
| `onClientNuiReady` | `resource` | iframe finished loading, `nui` global ready |
| `onClientNuiMessage` | `resource, event, data` | JS called `nui.emit`. `data` is a JSON string |
| `onClientNuiConsoleMessage` | `resource, level, message, source, line` | iframe `console.*`. `level`: 0 verbose, 1 info, 2 warn, 3 error, 4 fatal |

## JS API — `window.nui`

Auto-injected on every NUI iframe before page scripts run.

```js
nui.resourceName                 // owning resource
nui.emit(event, data)            // → onClientNuiMessage
nui.fetch(type, body) → Promise  // → registerNuiCallback handler
nui.setFocus(enable, keepInput)  // request focus from JS side
nui.on(event, fn)                // listen for sendNuiMessage payloads
nui.off(event[, fn])             // remove a handler or clear all
nui.log(...)  nui.warn(...)  nui.error(...)
```

`nui.on` dispatches by either the `event` or `action` field of incoming messages. Handlers receive `data.data` if present, else the whole payload.

```js
nui.on('open', d => render(d.state));
nui.on('update', d => updateBars(d));
```

## Messaging — Lua → JS

```lua
sendNuiMessage({ action = "showStats", online = 42 })
```

```js
nui.on('showStats', d => stats.online.textContent = d.online);
```

`sendNuiMessage` argument must be a table. The engine serializes it to JSON; on the JS side `nui.on` matches by `event` or `action` field.

## RPC — JS → Lua + response

```lua
registerNuiCallback("login", function(body)
    local data = fromJSON(body) or {}
    return toJSON({ ok = true, user = data.user })
end)
```

```js
const r = await nui.fetch('login', { user: 'me', pass: '…' });
if (r.ok) { /* ... */ }
```

Lua returns a string or a table; JS sees a string or a parsed object.

## URL schemes

| URL | Resolves to |
|---|---|
| `http://mta/<resource>/<file>` | a file declared in that resource's `<file>` / `<nui src=…>` |
| `https://nyc-nui-<resource>/<file>` | same content, unique HTTPS origin per resource (use when you need same-origin-policy isolation) |
| `http://mta/__nuirpc/<res>/<type>` | backend for `nui.fetch` — don't call directly |

## Cross-resource assets

Reference any other resource's declared files directly:

```html
<img src="http://mta/i-assets/nui/img/logo.png" />
<link rel="stylesheet" href="http://mta/i-assets/nui/css/theme.css" />
```

The target resource just declares the files in `<file>` tags.

## Shared NUI bundle (`i-assets`)

`i-assets` ships the canonical NUI design system. Link the stylesheets instead of duplicating tokens, primitives, fonts, or icons:

```html
<link rel="stylesheet" href="http://mta/i-assets/nui/css/fonts.css">
<link rel="stylesheet" href="http://mta/i-assets/nui/css/icons.css">
<link rel="stylesheet" href="http://mta/i-assets/nui/css/theme.css">
<link rel="stylesheet" href="http://mta/i-assets/nui/css/components.css">
```

| File | Provides |
|---|---|
| `fonts.css` | `@font-face` for Inter, Geist Mono, FontAwesome — served from `i-assets/fonts/` |
| `icons.css` | FontAwesome 6 (solid + regular + brands) |
| `theme.css` | `:root` tokens, reset, `clamp()` root font-size, motion keyframes |
| `components.css` | Primitives — surfaces, buttons, forms, nav, feedback, data, overlays, atoms |

In `meta.xml` add `<include resource="i-assets" />`.

## Hidden ≠ destroyed

`hideNuiFrame()` toggles `visibility:hidden` on the iframe; the page stays mounted, JS keeps running. You can `sendNuiMessage` and `registerNuiCallback` to a hidden NUI and it works. This is what makes seamless transitions (login → character select etc.) flicker-free: navigate the iframe while it's hidden, then `showNuiFrame()` once the new content has emitted `ready`.

## DevTools

```lua
toggleNuiDevTools(true)
```

Opens Chromium DevTools with the full iframe tree. Inspect, breakpoints, live CSS edits.

## Performance notes

- The NUI host runs at 5 Hz when nothing is visible (so hidden iframes can finish loading), 30 Hz with visible frames, 60 Hz when a frame has focus.
- V8 heap is capped at 128 MB across all iframes — don't ship huge JS bundles.
- POST body to `nui.fetch` capped at 5 MiB.
- `window.open` and popups are blocked. Use `nui.fetch` → Lua handler if you need to launch an external URL.
- WebGL works but assume it doesn't on integrated GPUs.

## Troubleshooting

- **Callback not firing.** `nui.emit` → `onClientNuiMessage` event in Lua, *not* `registerNuiCallback`. RPC handlers are reached via `nui.fetch`.
- **Page blank after hide/show.** Make sure you're on a build with `visibility:hidden` (not `display:none`) for `.hidden`. Older builds destroyed the JS context on hide.
- **Cursor not showing.** `setNuiFocus(true)` does *not* show the cursor. Call `showCursor(true)` separately.
- **Two resources fighting for focus.** Whoever called `setNuiFocus(true)` last wins; the other gets implicit `setNuiFocus(false)`.
