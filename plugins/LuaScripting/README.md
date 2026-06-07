# LuaScripting plugin

Scripting plugin for LogToGraph that lets you turn each row of a log file into signal events through a Lua script.

Built on:

- **[LuaJIT 5.1](https://luajit.org/)** — fast JIT-compiled Lua. The JIT is automatically disabled when a debug session is armed (line hook would defeat it) and re-enabled afterwards.
- **[sol2](https://github.com/ThePhD/sol2)** — modern C++ <-> Lua binding. Configured with `SOL_ALL_SAFETIES_ON=1` and `SOL_EXCEPTIONS_SAFE_PROPAGATION=0` so C++ exceptions thrown by bindings surface their real `e.what()` in Lua (not the literal "C++ exception" string).
- **[boost::regex](https://www.boost.org/doc/libs/release/libs/regex/)** — exposed to Lua as `ltg:regex(pattern)`. PCRE-like syntax, much more expressive than Lua's built-in patterns. The same engine is also linked host-side (it ships with imguipack), keeping host and plugin in sync.

## Callbacks the host calls into

Your script defines three callbacks. All three are required (the host logs a warning if missing).

```lua
function startFile(filepath)
    -- called once before the first row of a log file.
    -- `filepath` is the full path. Use for per-file initialization.
end

function parse(buffer)
    -- called once per row. `buffer` is the row text (no trailing newline).
    -- Emit signals here via the ltg:* API.
end

function endFile(filepath)
    -- called once after the last row of a log file. Use for per-file finalization.
end
```

If you import several log files in the same project, the sequence is repeated for each file in the order they were added.

## The `ltg` object

A single global `ltg` userdata is injected by the host before the script runs. All host services hang off it.

### Logging (Console pane)

```lua
ltg:logInfo("...")
ltg:logWarning("...")
ltg:logError("...")
ltg:logDebug("...")
```

Each call goes through the host's Messaging singleton and shows up in the Console pane categorized by type. The console keeps its own per-category counter.

### Date / time conversion

ISO-like strings (`"YYYY-MM-DD HH:MM:SS,MS"` or `"YYYY-MM-DD HH:MM:SS.MS"`), with an hour offset as the second argument:

```lua
local epoch = ltg:stringToEpoch("2023-01-16 15:24:26,464", 0)   -- -> double (epoch seconds, ms preserved)
local s     = ltg:epochToString(18798798465465.546546, 0)      -- -> string
```

Both throw a `std::runtime_error` on a bad format. With the auto-bp-on-error toggle on, that throw pauses the worker at the exact line of the call, with the actual boost message visible.

### Row position

```lua
ltg:getRowIndex()   -- 0-based index of the current row within the current file
ltg:getRowCount()   -- total row count of the current file (set once before parse() loops)
```

Useful for progress display, heuristics ("skip the first 10 lines"), or correlating multiple signals emitted from the same row.

### Signal emission

The fundamental primitive: a "signal tick" at an epoch time on a `(category, name)` channel.

```lua
ltg:addSignalValue(category, name, epoch, value, desc?)
```

- `category`, `name`: strings — `(category, name)` is the unique channel identifier shown in the ToolPane tree.
- `epoch`: number (epoch seconds, sub-second precision preserved).
- `value`: number (the y value plotted on the graph).
- `desc`: optional string — shown as the value's hover tooltip on the graph.

```lua
ltg:addSignalStatus(category, name, epoch, status_string)
```

A string-valued tick instead of a numeric one. Renders as colored bands on the graph.

```lua
ltg:addSignalStartZone(category, name, epoch, label)
ltg:addSignalEndZone(category, name, epoch, label)
```

Pair these to mark a duration zone on a signal. The graph renders the zone as a translucent rectangle with `label` as the tooltip.

```lua
ltg:addSignalTag(epoch, r, g, b, a, name, help)
```

A point event marker (not tied to a signal). Color components are linear in `[0:1]`. `help` is shown as a tooltip on hover. Useful for errors / state transitions / etc.

### Regex (boost engine)

```lua
local re = ltg:regex(pattern)   -- compile once at script init, reuse on every row
```

Throws `boost::regex_error` on a bad pattern; with auto-bp-on-error on, you see the boost diagnostic at the `ltg:regex(...)` line.

The returned `LtgRegex` userdata has five methods:

```lua
re:test(input)              -- bool: does the pattern match anywhere in input?
re:match(input)             -- captures as multiple return values, or nil
                            --   - if the pattern has NO capture groups, returns the whole match
                            --   - otherwise returns each capture in order
re:find(input)              -- (start, end) 1-based inclusive, or nil — mirrors string.find
re:gsub(input, repl)        -- (result_string, count) — mirrors string.gsub
re:gmatch(input)            -- Lua iterator over matches, each step returns the captures
```

Example — XML-like profiler row with four fields:

```lua
local profilerPattern = ltg:regex([[<profiler section="([^"]*)" epoch_time="([^"]*)" name="([^"]*)" render_time_ms="([^"]*)">]])

function parse(buffer)
    local section, time, name, value = profilerPattern:match(buffer)
    if section then
        ltg:addSignalValue(section, name, tonumber(time), tonumber(value))
    end
end
```

Performance tip: `[^"]*` (negated-class star) is faster than `.*` for "match up to the next `"`" — boost doesn't have to backtrack to find the closing quote.

`gmatch` example — multiple key=value pairs on one row:

```lua
local kvPattern = ltg:regex([[(\w+)=(\S+)]])

function parse(buffer)
    for k, v in kvPattern:gmatch(buffer) do
        local n = tonumber(v)
        if n then
            ltg:addSignalValue("kv", k, ltg:getRowIndex(), n)
        end
    end
end
```

## Lua stdlib available

Curated for a log-parsing context:

- **base** — `tostring`, `tonumber`, `type`, `pairs`, `ipairs`, `next`, `unpack`, `select`, `pcall`, `xpcall`, `assert`, `error`, `setmetatable`, `getmetatable`, `rawget`, `rawset`, `rawequal`, `print`.
- **string** — `match`, `gmatch`, `gsub`, `find`, `format`, `len`, `lower`, `upper`, `sub`, `rep`, `reverse`, `byte`, `char`.
- **math** — `abs`, `ceil`, `floor`, `fmod`, `modf`, `max`, `min`, `exp`, `log`, `log10`, `pow`, `sqrt`, all the trig functions, `random`, `randomseed`, `deg`, `rad`, `frexp`, `ldexp`.
- **table** — `insert`, `remove`, `concat`, `sort`, `maxn`.
- **os** — `date`, `time`, `difftime`, `clock`, `getenv`.
- **jit** — driven by the host (`jit.off()` / `jit.on()` around debug sessions).

Not loaded:

- `io` — file I/O is a sandbox escape and is not the right tool for emitting signals (the right tool is `ltg:addSignal*`).
- `ffi` — LuaJIT's foreign function interface; loaded would let user code call arbitrary C, full sandbox escape.
- `debug` — user code that called `debug.sethook` would silently override the host's line hook and break the debugger. The host uses `lua_sethook` from C directly.
- `package` — `require` / `loadfile` would let user code load arbitrary external modules. Will be re-enabled when multi-script projects + a local lib folder land.
- `coroutine` / `bit32` — unused in parsing in practice; `bit32` is redundant with LuaJIT's native `bit` table.

## Errors and the debugger

The plugin hooks the sol2 exception handler (for C++ exceptions thrown by bindings — bad date string in `ltg:stringToEpoch`, bad pattern in `ltg:regex`, etc.) AND a Lua-level message handler attached to every protected_function call (for pure-Lua errors — `string.match(nil)`, `nil:method()`, type errors, explicit `error("...")`, etc.).

Both paths converge on the same logic: when the host's `Settings → Debug → Auto-bp + pause on error` toggle is on, the worker is paused **synchronously at the throw site**, with the Lua call stack still intact. The watcher / scope / calltrace panes show locals and upvalues at the moment of the throw, the error line is highlighted with an amber arrow + red background + the boost / Lua error message as a tooltip, and an automatic breakpoint is placed at that line for the next run.

Continue / Step / Stop work as expected. Stop is honoured even past further errors in the cleanup path (e.g. an `endFile` that itself throws on the way out).

## Project script storage

The script lives **inside the project's `.ltg` SQLite database** (table `script_code`), not as an external `.lua` file. New projects start from a documented boilerplate template (the same API reference in this file, in comment form, plus the three callbacks scaffolded).

Breakpoints are persisted in the project XML (`<breakpoints><line>N</line>...</breakpoints>`); when you reopen a project, your debug spots come back.
