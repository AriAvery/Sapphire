# Event object and instance script generator

`event_object_parser` generates C++ instance and quest-battle skeletons from an
FFXIV installation. It reads EXD metadata and the zone LGB/SGB layout files,
then fills `instance.tmpl` or `questbattle.tmpl` with `addEObj` calls and
quest-battle definitions.

The generated files are starting points. They do not contain complete encounter
or quest-battle behavior and are not regenerated as part of a normal Sapphire
build.

## Build and run

Build the tool normally:

```text
cmake --build build --target event_object_parser
```

CMake copies `instance.tmpl` and `questbattle.tmpl` next to the executable. Run
it with the current directory set to that directory because output paths are
relative to the current directory:

```text
cd bin/tools
event_object_parser "E:/Sapphire/game/sqpack"
```

Output is written below `instances/`, grouped by instance-content type. In
particular, quest battles are written to `instances/questbattles/`.

The optional second argument is currently ineffective: the generator iterates
all known instance-content and quest-battle rows.

## Data flow

1. `ExdData` loads `InstanceContent`, `QuestBattle`, `Quest`, `TerritoryType`,
   and `EObj` rows from the supplied game data.
2. Territory names are resolved to layout paths.
3. `bg.lgb`, `planmap.lgb`, and `planevent.lgb` are parsed for event objects and
   shared groups.
4. Referenced SGB files are inspected for timelines and collision geometry.
5. EXD object names, layout IDs, transforms, states, and collision data are
   emitted into the C++ template.

Related but separate tools:

- `quest_parser` extracts quest Lua bytecode, decompiles it with the bundled
  `unluac_2015_06_13.jar`, and generates normal quest-script templates.
- `questbattle_bruteforce` locates and decompiles quest-battle Lua files, but
  does not generate the C++ instance skeletons.

## Generated string literals

Names are byte strings supplied by the game data. Generated C++ source escapes
non-ASCII and control bytes as `\xHH`. Consecutive escapes can remain in one
literal because the next backslash terminates the preceding escape:

```cpp
"\xE4\xBB\xAE"
```

A C++ `\x` escape consumes all following hexadecimal digits, so an adjacent
literal boundary is inserted only when an ordinary hexadecimal character
follows an escape:

```cpp
"\x97""ClsExc250"
```

Adjacent literals are concatenated by the compiler, so the runtime string still
contains exactly the original bytes. Quotes, backslashes, and common whitespace
controls are escaped separately.

## Known data-version constraints

The parser uses the Win32 sqpack reader. It was reproduced successfully against
a 3.x installation (`ffxivgame.ver` 2016.07.05 and `ex1.ver` 2016.05.21).

## Looking up an EObj timeline

In Toolkit, open **Tools → Zones**, select a territory in the Zone Editor and expand
**EObj timelines** in the zone details. Filter by object name, EObj base ID
or layout instance ID, then select the placement. This view reads the selected
territory directly from the configured game data and works in towns, fields
and instances. It also shows the source layer, bound placement and SGB path.

Each general slot shows its mask, SGB member ID and authored timeline name.
For example, Tam-Tara layout `1958523` uses slot 0 (mask `0x0001`) for member
6, `vf_beam2_on`, and slot 1 (mask `0x0002`) for member 5, `vf_beam2_off`.
The same slot can select a different timeline on another object. A combined
mask selects each of its set slots. The four visibility bindings have separate
rows. **Other** lists timelines with no reference from those twenty bindings.
**Unused** means the binding is zero. A missing member or layout asset has its
own diagnostic. The generated instance scripts include the same slot and
member information next to their EObj definitions.
