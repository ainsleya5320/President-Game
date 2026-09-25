# Four Years — game concept and first playable slice

## The idea

Play a fictional president through a changing four-year term. Each month begins with a briefing and ends with a choice that costs time, political capital, money, or trust. Some outcomes are uncertain, and some decisions return months later. The Oval Office is a place you inhabit between briefings, with a small number of meaningful interactions rather than a constant 3D interface.

This is an original project inspired by the monthly decision-and-fallout structure described in [Fantasy President Career's public guide](https://fantasypresidentcareer.com/career-guide.html) and by the resource pressure and unexpected setbacks of a journey game. It does not use that game's writing, code, interface, or simulation data. All policies and outcomes in the prototype are fictional.

## What is playable today

`Prototype/Four_Years_Prototype.html` is a browser-based vertical slice. It contains:

- A walkable first-person view of the existing Oval Office model, a separate overview camera, and a desk interaction. Use W/A/S/D, drag to look, and press E near the desk to open a briefing.
- A second location aboard Air Force One: presidential office, curved windowed passage and conference room. Board or return using the toolbar, with the same briefings, resources and journal in either location. Travel is currently free exploration; it does not spend a month or change the simulation. Save/resume restores the selected location.
- Twelve original monthly briefings. Each has three choices.
- Four resources: public trust, political capital, fiscal room, and stamina.
- Visible immediate effects, uncertain setbacks, delayed effects, a journal, a one-year conclusion, and local save/resume.

This gameplay prototype runs in a browser. The native Unreal project contains the editable Oval Office scene and is the intended home for a future native version; the browser game's controls and decision system are not yet Unreal assets. The browser version is deliberately cheap to change while the rules are still fluid.

## Core loop for the larger game

1. Walk a room and inspect the day's people and documents.
2. Read a briefing that asks for a consequential decision.
3. Choose an action and see its immediate cost.
4. Advance the calendar; receive delayed consequences and occasional setbacks.
5. Adjust the administration's route through domestic policy, diplomacy, Congress, and elections.

The important design tension is that a choice can solve today's problem while reducing the resources needed for a later one. A low trust or exhausted administration can fail even if individual policy choices seemed defensible.

## Next implementation steps

1. Playtest the twelve-month slice and decide which choices feel meaningful, which stats are worth keeping, and whether terms should be monthly or weekly.
2. Add a native Unreal walking character with floor/furniture collision and desk, phone, and adviser interactions.
3. Move events and consequences into editable Unreal data assets, with a simple briefing interface and save game.
4. Expand to four years, add an election and cabinet relationships, then create only the 3D scenes that change the decisions: the Oval Office, Situation Room, Cabinet Room, and campaign events.

## Design decisions to revisit

- The prototype uses fixed options for clarity. Free-text policy input would need an explainable, testable interpretation system before it becomes a core mechanic.
- The four resources are a starting set. Public opinion, Congress, foreign relations, and the economy may become separate systems if playtesting shows that distinction creates interesting decisions.
- A single twelve-month run currently teaches the rhythm. A full term needs evolving priorities, opponents, advisers, and consequences that cross years.

The event data and rules in `Prototype/office-game.template.html` are meant to be edited freely. Run `Prototype/build_prototype.py` to rebuild the self-contained playable HTML after changes.
