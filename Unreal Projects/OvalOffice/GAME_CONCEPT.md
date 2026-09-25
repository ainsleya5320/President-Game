# Four Years — playable political simulation

Four Years is an original fictional presidency game with two walkable 3D locations: the Oval Office and Air Force One's presidential suite. The browser edition runs a full four-year term in sixteen quarterly turns. It draws on the broad idea of interconnected political simulation, but uses original policies, events, voter profiles, rules, writing, and interface.

## Playable systems

- **Policy atlas:** 26 adjustable policies across treasury, economy, health, education, housing, justice, environment, foreign affairs, and government. Each has a five-position setting, political-capital cost, budget effect, implementation lag, and links to national conditions.
- **National conditions:** growth, employment, price stability, health, education, housing, safety, climate resilience, energy reliability, mobility, civil liberties, and institutional trust. Conditions feed into one another and may trigger situations such as recession or a housing emergency.
- **Budget:** revenue, spending, quarterly balance, interest, and accumulating debt. Tax policy raises revenue but can constrain growth or prices; spending policies create tradeoffs rather than free gains.
- **Electorate:** ten voter blocs with different priorities, policy stances, population shares, and turnout. Their approval feeds the election forecast.
- **Quarterly agenda:** a distinct dilemma with three responses each quarter. Effects appear in the next report. The term includes a midterm and a final general election.
- **3D continuity:** walking, camera views, the desk briefing interaction, the aircraft hallway and conference room, and travel between rooms remain in place. Travel does not advance the term. Save/resume preserves the location and simulation state.

Open **Play Four Years.cmd** at the repository root or serve `Prototype/Four_Years_Prototype.html`. Use **Policies**, **Voters**, and **Budget** in the toolbar to inspect the systems, then choose the quarterly response from the Situation screen and advance. Existing monthly saves are migrated into the four-year system and their previous journal entries are archived in the new Record screen; a backup of the original save is also kept locally.

The native Unreal project still contains the editable Oval Office scene and walkable Air Force One exploration level. The political simulation and location switching currently run in the browser game, not in native Unreal. The models are artistic approximations rather than exact architectural surveys.

## Development

The rules and data are in `Prototype/simulation-core.js`, and the strategy interface is in `Prototype/strategy-ui.js` and `Prototype/strategy-ui.css`. `Prototype/office-game.template.html` retains the 3D scene wiring; `Prototype/build_prototype.py` embeds the model assets and these source files into the self-contained playable HTML. Run `node Prototype/test_simulation.cjs` and `node Prototype/test_aircraft_navigation.cjs` after gameplay changes, then rebuild the HTML.

The current simulation is intentionally legible and deterministic. Later work could add advisers, legislative bargaining, donors, regional maps, external shocks, and a more detailed campaign phase after playtesting reveals where extra complexity would improve decisions.
