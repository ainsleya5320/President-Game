# International diplomacy — native Unreal preview

Press **M** while walking or from any domestic screen, or select **World map [M]** in the briefing. Click a partner's marker to inspect relations, stability, U.S. influence and rival influence. Trade agreements draw routes from the United States. Scroll the partner panel to see all available actions.

The Crisis Desk shows deadlines and decisions. Strategy & Record explains the rules, offers three victory goals, and preserves recent diplomatic news. Values and incidents describe an original fictional strategic scenario, not current real-world assessments. The map uses original schematic coastlines, not territorial boundaries.

## A playable strategic campaign

- Twelve partners; three diplomatic actions per quarter; one bilateral action per partner per quarter.
- Six bilateral actions: envoy, development aid, trade, security partnership, sanctions, and lifting sanctions.
- Actions share domestic political capital. Financial commitments add debt, with a hard debt-limit check.
- Trade pacts earn a capped quarterly debt dividend and support domestic growth. Trade pauses below 35 stability. Sanctions terminate the pact.
- Security partners resist rival pressure. Relations below 50 end a security partnership.
- A deterministic rival offensive targets one partner each quarter, alongside slower worldwide rival gains.
- Six rotating authored crises, with a new incident every two quarters and a three-quarter deadline.
- Mediation requires two rounds in separate quarters. A funded settlement costs more and resolves immediately. Coalition diplomacy requires local influence 60 or at least three security partners.
- Missed deadlines reduce prestige, raise tension, weaken the affected partner and damage a relevant domestic metric. Tension 70+ harms growth and price stability.
- Domestic decisions close the diplomatic window. Advance the quarter through the existing briefing; international outcomes are included in its report.

## What winning means

Choose a strategy before the first diplomatic action. That first action commits it for the presidency.

| Strategy | Conditions at term end |
|---|---|
| Commercial power | 4 trade pacts, 55 weighted influence, tension <= 45 |
| Coalition builder | 4 security partners, 60 weighted influence, tension <= 55 |
| Peacemaker | 4 resolved crises, 70 prestige, tension <= 30 |

The score displays progress toward all three conditions. Meeting them early means holding that position until the term ends. A diplomatic victory does not override the domestic election or guarantee re-election. Late-term existing saves retain their quarter and inherit the full goals; start a new presidency for the complete campaign.

## Save compatibility and scope

Existing native saves gain a new `diplomacy` object in memory without changing domestic progress or writing a save simply by opening the map. Successful actions and normal quarter advancement save it. A new presidency resets this layer along with the domestic simulation.

This is currently **native Unreal gameplay**. The separate pure C++ diplomacy module is called by the Unreal subsystem before domestic quarter resolution, so international effects feed that quarter's domestic calculations and election. The browser rules and their C++ parity core are unchanged; the browser currently preserves but does not simulate the new diplomacy data. No tactical combat, invasions, or territory occupation are included.

## Validation

`Tests/FourYearsCore/diplomacy_main.cpp` is an isolated standard-C++ rules test. Compile it with `FourYearsJson.cpp` and `FourYearsDiplomacy.cpp`, including `Source/OvalOffice/Public`. It covers action limits, cost and prerequisite gates, deterministic turns, save round-tripping, deadlines, domestic consequences, victory conditions, and three bounded 16-quarter simulations.

`Scripts/test_diplomacy.py` runs in Play in Editor and validates the real subsystem, persisted actions, inauguration integration, quarterly reports, and crisis resolution. It backs up the player's save, uses a temporary test presidency, stops that play session, and restores the original bytes even on failure. Its report is written to `Saved/DiplomacyValidation.json`.
