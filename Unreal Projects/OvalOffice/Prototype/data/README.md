# Four Years game data

All of the simulation's content lives in these JSON files. `simulation-core.js` reads them as a single `FourYearsData` object, keyed by file name (`events.json` becomes `FourYearsData.events`). The rules code contains no content, so another engine can load the same files later.

- `build_prototype.py` bundles the files into the playable HTML through `game_data.py`.
- The Node tests load them through `load_simulation.cjs`.

The data is validated whenever the simulation loads. A typo such as an unknown metric, policy, voter group or situation name stops the game with a list of every problem it found, rather than quietly producing broken numbers. Run `node test_event_deck.cjs` after editing.

Metric keys used throughout: `growth`, `jobs`, `prices`, `health`, `schools`, `housing`, `safety`, `climate`, `energy`, `equality`, `liberty`, `trust`. Where noted, `debt` can also be used.

## Files

| File | Contents |
| --- | --- |
| `metrics.json` | `labels` (display name for each metric) and `base` (the level each metric drifts towards before policies) |
| `policies.json` | The 26 policy levers: `id`, `name`, `department`, `cost` (budget units per level; negative means revenue), `effects` (metric change per level per quarter), optional `lag` (quarters to implement; derived from cost if omitted) |
| `electorate.json` | The simulated population: number of `voters`, trait distributions (each must add up to 1), each voter's random `lean` range and the approval `bar` |
| `groups.json` | Voter groups, which overlap (below) |
| `unrest.json` | Radicalization: anger threshold, stages and the attempt risk (below) |
| `people.json` | The four recurring advisers: their voter group, the policies they ask for, and their campaign expertise |
| `campaign.json` | Campaign `regions` (voter groups and issues) and speech `issues` |
| `events.json` | The quarterly event deck (below) |
| `situations.json` | National situations (below) |

## Electorate and groups

- **Voters:** each term generates a population of individual voters from the save's seed. Each voter draws traits from `electorate.json` (age, place, income) and a personal `lean`.
- **Membership:** a voter joins each group with a probability set by the group's `share` (the average percentage of voters who belong), multiplied by its `traits` table (for example `"age": {"senior": 3.8}`). The multipliers are normalized so the average still matches `share`. Voters often belong to several groups, so shares add up to well over 100.
- **`drivers`:** make membership move with conditions. Each coefficient applies per 50 points of distance from 50. For example, `"health": -0.9` means failing public health creates more health-care voters.
- **Voting:** each group has a mood (its approval), calculated from its `priorities`, favored (`fav`) and opposed (`opp`) policies, situations, campaigning and the regions. A voter averages the moods of their groups, adds their lean, and approves if the result reaches `bar`. The poll is the turnout-weighted share of approving voters. `turnout` adds a group-specific turnout bonus.
- **`disruption`:** what the group does once its unrest reaches a disruptive stage. It has a `name`, `effects` on metrics and optionally a `revenue` change.

## Unrest

- **Building anger:** each quarter a group's anger (0 to 100) decays by `decay` and grows by `rate` for every point of approval below `threshold`.
- **`stages`:** anger escalates through the stages (Protests, Disruption, Extremists). Their `effects` apply every quarter. Stages marked `disruption` also apply the group's own disruption. All unrest effects scale with group size.
- **`attempt`:** while any group is at or above `at`, each quarter carries a seeded `chance` of an attempt on the president. The chance is lowered by the `security` policies (per level above 2) and raised when several groups are extreme. It ends in the `foiled` or `attack` outcome.
- **Events:** can require unrest with `when.unrest` (any group at or above that anger). A choice's `calm` lowers the angriest group's anger; a negative value inflames it.

## Events

```json
{
  "id": "rolling-blackouts",
  "title": "Rolling blackouts",
  "body": "Shown in the Situation Room.",
  "weight": 4,
  "when": {"from": 2, "until": 12, "below": {"energy": 45}, "above": {"debt": 90}, "situations": ["Power instability"], "absent": ["Recession"]},
  "choices": [
    {"label": "Lease emergency generation", "effects": {"energy": 4, "climate": -2}, "cost": 7}
  ]
}
```

- **Drawing an event:** each quarter the deck draws one unused event whose `when` conditions all hold. The draw is weighted by `weight` (default 1) and seeded by the save's `seed`, so the same save always draws the same events.
- **Fallbacks:** if no event qualifies, any unused event is drawn instead. If the deck ever runs out, used events can come back.
- **`fixed`:** schedules an event for a specific zero-based quarter (0 to 15), regardless of conditions. The midterm aftermath uses 8 and the final address uses 15.
- **`when` conditions:**
  - `from` and `until` are inclusive zero-based quarters.
  - `below` and `above` compare metrics, or `debt`.
  - `situations` must all be active.
  - `absent` must all be inactive.
- **Choice `cost`:** budget units added to debt at 45%. A negative cost saves money.
- **Choice `effects`:** arrive in the next quarter's report.
- **`legacyQuarter`:** marks the original 16 events, so saves made before the deck existed keep the event that was already on the desk. New events don't need it.

## Situations

```json
{
  "name": "Recession",
  "metric": "growth",
  "below": 38,
  "clearsAt": 42,
  "description": "Shown as a tooltip.",
  "effects": {"jobs": -2, "trust": -1},
  "groups": {"workers": -3},
  "revenue": -4,
  "capital": 0
}
```

- **Starting and clearing:** set either `below` or `above`. A situation starts when the metric (or `debt`) crosses that trigger, and stays active until it recovers past `clearsAt`. `clearsAt` must be on the recovery side of the trigger. The gap stops situations flickering on and off.
- **While active:**
  - `effects` push on metrics every quarter.
  - `groups` shifts voter-group approval.
  - `revenue` changes quarterly revenue.
  - `capital` changes the political capital gained each quarter.
- **Names:** events refer to situations by `name`, and saves store the names too, so rename a situation carefully.
