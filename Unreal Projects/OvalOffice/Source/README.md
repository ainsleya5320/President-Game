# Four Years in Unreal: native simulation

The presidency simulation now runs natively in Unreal Engine 5.8, sharing its content and save format with the browser game.

## What is playable

Open the Oval Office map, walk to the Resolute Desk and press **E** to open the quarterly briefing. From there you can:

- Read the quarter's event.
- Choose a response.
- Advance the quarter and read the report, including midterm and general election results.
- Play all 16 quarters of a term, then start a new one.

**Controls:** W/A/S/D walks, the mouse looks around, and Esc or E closes the briefing.

The term saves automatically to `Saved/FourYears/Term.json`.

The other strategy screens are still browser-only for now: policies, Congress, meetings, campaign trips, the dashboard and the rest. The C++ simulation already implements all of their rules; they only need screens.

## Build

Unreal needs to compile the new C++ module once.

The Windows helper `Scripts/BuildFourYearsUnreal.ps1` builds `OvalOfficeEditor` with the installed bundled runtime, including an x64 fallback when an ARM Windows installation lacks the ARM .NET bundle. This preview was compiled successfully with Unreal 5.8.3, Visual Studio 2026/MSVC 14.51 and Windows SDK 10.0.26100.0.

After building, **Edit Four Years in Unreal.cmd** opens the editor and runs `Scripts/configure_four_years.py`. The imported Oval map has an explicit `GameModeBase` override, which must be changed to `FourYearsGameMode` for Play in Editor. The setup script changes and saves only that map's game-mode setting. **Play Four Years in Unreal.cmd** also explicitly selects the native game mode when launching.

1. Install **Visual Studio 2022** with the **Game development with C++** workload. On a Snapdragon (ARM) PC, also install the MSVC ARM64/x64 build tools the Unreal installer asks for.
2. Right-click `OvalOffice.uproject` and choose **Generate Visual Studio project files**.
3. Open the project in Unreal Engine 5.8 and accept the prompt to rebuild the `OvalOffice` module. Alternatively, build the `OvalOfficeEditor` target in Visual Studio.
4. Run **Play Four Years in Unreal.cmd** at the repository root, or press Play in the editor on `/Game/OvalOffice/Maps/OvalOffice`.

The Oval Office map now uses `FourYearsGameMode`, set in `Config/DefaultEngine.ini`. The Air Force One map keeps its own Blueprint game mode.

## Architecture

| Path | What it is |
| --- | --- |
| `OvalOffice/Public/FourYears/Core`, `Private/FourYears/Core` | The simulation core: standard C++17 with no Unreal headers, exceptions or RTTI. `FourYearsSim` ports `simulation-core.js` and `executive-systems.js` line for line. `FourYearsJson` is a small JSON value type with JavaScript semantics. |
| `FourYearsSubsystem` | A game-instance subsystem. It loads `Prototype/data/*.json`, owns the current term, and saves it. It also exposes Blueprint-callable functions (`GetBriefing`, `ChooseResponse`, `AdvanceQuarter`, `StartNewTerm`) for future UMG screens. |
| `FourYearsGameMode`, `FourYearsWalker`, `FourYearsPlayerController` | Walking the Oval Office. The walker uses the browser game's walkable area and furniture footprints, because the imported room meshes have no collision. |
| `SFourYearsBriefing` | The Slate briefing screen at the desk. |

**One copy of the rules and content.** Both games read `Prototype/data/*.json`. That includes `executive.json`, which holds Congress, regions, goals, the government team, and the crisis and press dialogue. The game state is a JSON object with the browser save's exact shape, so a save can be copied between the two. To move a browser save into Unreal, export it from the browser's local storage (`four-years-oval-prototype-v1`) into `Saved/FourYears/Term.json`; `Migrate` upgrades it on load.

**Packaging.** In the editor and `-game` launches, data is read from `Prototype/data`. A packaged build looks in `Content/FourYears/Data`, so copy the JSON files there and add that folder to *Additional Non-Asset Directories to Package*.

## Parity test

The C++ core must match the JavaScript exactly. To check it:

```sh
node "Unreal Projects/OvalOffice/Tests/FourYearsCore/run_parity.cjs"
```

It needs Node and either `g++` or `clang++` (on Windows, for example through WSL or MSYS2). The script does three things:

1. It generates a reference trace from the browser game (`Prototype/parity/make_trace.cjs`). A seeded bot plays 34 full terms through every action: policies, meetings, Congress, the cabinet, crises, the press, campaign trips and travel. Some terms start from older save versions, and some start from extreme agendas that trigger unrest and assassination attempts.
2. It builds the core with warnings as errors and without exceptions or RTTI.
3. It replays every action, comparing each result and the full state after every quarter with no tolerance.

Run it after changing either implementation. A change to game rules must be made in both.
