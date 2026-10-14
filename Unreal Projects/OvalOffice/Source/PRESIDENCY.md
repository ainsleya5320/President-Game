# Four Years: The Presidency

A native Unreal campaign with a beginning, middle and ending. The existing political simulation, county electorate, diplomacy and authored room assets are retained.

## Play a term

1. Begin with the skippable inauguration and choose a Democratic or Republican administration.
2. Press J. Pick a mandate: three public targets and a signature bill.
3. Each quarter, visit the location for your personal chapter, then choose a response. Prior preparation can help in later chapters. You may delegate a chapter for trust -1; unhandled chapters are automatically delegated when advancing.
4. Before filing the desk decision, use your limited appointments and capital: advisers, policies, Congress, press appearances, cabinet management, organizing or diplomacy.
5. File a response in Briefing, advance and read the consequences. Policies have implementation delays; laws need both votes and delivery time.
6. Face midterms after quarter eight. After quarter sixteen, read the election result and the Record page's epilogue. A victory completes this campaign; a second playable term is not simulated. A 269–269 result receives an unresolved-mandate epilogue; the contingent election is not simulated.

Four acts: The promise, The reckoning, The work, The legacy. Sixteen original chapters and 48 responses cover delivery plans, storm resilience, oversight, congressional coalitions, trade, the promise ledger and election eve. These are fictional scenarios, not historical events.

## Controls and travel

WASD walks, mouse looks, E interacts with nearby staff or points of interest. J opens the journal; P policies; C Congress; M world map; V voters. Escape closes a screen. Time advances only through the quarterly briefing.

Places connects the existing Oval Office, Cabinet Room and Air Force One with a new residence sitting room and limo cabin. Travel consumes neither time nor resources. The limo is a stationary secure cabin. The new rooms are runtime geometry; NPCs use eight textured human models with fitted suits, distinct faces and hair, seated poses, restrained idle motion and head tracking. They remain non-voiced, stationary characters. The old browser West Wing remains intact but is not a new native map in this expansion.

## Costs and consequences

* Personal chapter effects, capital/debt costs and two-quarter delayed results appear on the response buttons. Chapter fatigue is +5; every quarter adds +7. At fatigue 70 or higher, advancing costs capital -1 and trust -1.
* Once per quarter, use personal time for residence rest (fatigue -20) or a limo briefing (capital +2, fatigue +4).
* One caucus relationship meeting per quarter costs 2 capital and adds 9 relationship points. Meet in the Cabinet Room or call from the limo. Each point above/below 50 changes that caucus's bill support by 0.35 points. Relationships drift 10% toward 50 per quarter. Bill-specific lobbying remains separate.
* Government exposes existing cabinet and media rules. Department support costs 2 capital and an appointment; an acting replacement costs 3. One media appearance per quarter uses an appointment; debates occur before midterms and the general election. Domestic crises retain their staged consequences.
* The legacy score weights approval (40%), three achieved platform goals (30%), delivery of up to three laws (21%) and trust (9%). Electoral victory is evaluated separately by the existing 538-elector model. A successful record can outlive an electoral defeat.

## Saves and scope

`Saved/FourYears/Term.json` remains the autosave. Existing saves gain a `presidency` section and retain their current quarter, party, decisions and platform. They are not penalized retroactively for earlier chapters. Starting over requires two clicks on the Record page and copies the current save to `PreviousTerm.json`. This is a single rotating archive, not unlimited save slots.

Campaign data: `Prototype/data/presidency.json`. Rules: `FourYearsPresidency.cpp`. Journal: `SFourYearsPresidency.cpp`. Runtime rooms and NPCs: `FourYearsPresence.cpp`. The simulation is deterministic for a given seed and choices. The browser keeps the previous gameplay interface; only the optional shared congressional relationship modifier is mirrored there.

## Validation

`Tests/FourYearsCore/presidency_main.cpp` exercises all chapter responses, mandates, availability guards, exhaustion/time limits, relationship effects, six complete terms across both parties, endings and save migration. `Scripts/test_presidency.py` exercises native travel, walking clearance, NPCs and journal access in Play in Editor. Tests preserve the user's original save.

Validated with 555 campaign checks, 333 native travel/interaction checks, a complete 16-quarter native ending playtest, and shared-core parity across 34 terms, 2,757 actions and 528 quarter-end states. Run the C++ campaign suite from a Visual Studio developer shell with `Tests/FourYearsCore/run_presidency.ps1`.

Build with `Scripts/BuildFourYearsUnreal.ps1`. Run from the repository's `Play Four Years in Unreal.cmd`. This is a playable development build, not a packaged shipping executable. It retains the project's DX11 rendering settings for this computer.
