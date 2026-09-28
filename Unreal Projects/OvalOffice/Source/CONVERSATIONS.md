# Portrait conversations

At the sofas, press **E** and choose a face (or use **1–4**). Each person has a large portrait, a relationship meter, a dialogue box and numbered responses. **Space** reveals the complete line, **1–3** selects a response, **Backspace** returns to the roster, and **E** returns to the room. Reading a conversation is free; committing to a meeting response uses an appointment. **P** opens all policies and **C** opens Congress.

The policy shortcut inside a conversation focuses on that adviser's requested policy. Return to the conversation from there or choose **All policies**. Closed quarters, unavailable advisers and exhausted appointments are explained before another response can be selected. The simulation remains the authority for all costs and consequences.

## Adding characters and scenes

Character voices live in `Prototype/data/dialogue.json`. The keys match the existing people IDs (`chen`, `ruiz`, `brooks`, `bell`). Add a character override for any cue; omitted cues inherit the shared fallback. Acting replacements deliberately use fallback dialogue and initials so they do not inherit the former adviser's identity. Portraits still use the existing runtime character asset folder.

Cues include greeting, trusted, strained, pending, met, busy, closed, ended, resigned, and a reaction for each supported meeting response. These are authored lines; there is no network dependency, generated speech or live language-model call. Relationships, promises and appointment eligibility select the context.

`SFourYearsScreen` owns presentation and `UFourYearsSubsystem` owns gameplay. Future room interactions can open the same screen through `AFourYearsPlayerController::OpenAdvisers`. New room maps still require their own traversal, collision, spawn points and a correct simulation location; this change does not enable native travel to the West Wing or Air Force One.

The next useful expansion is a Cabinet Room with an arrival interaction and a small number of named NPC positions, followed by shared travel/save handling for the existing Air Force One map. Keep the portrait conversation presentation shared across those locations.

## Validation

Build with `Scripts/BuildFourYearsUnreal.ps1`. In Play in Editor, execute `Scripts/test_conversations.py` through Unreal's Python command. It runs a temporary term, checks portraits, dialogue fallback, meeting limits, policy costs, the complete promise-to-report flow and save output, then restores the player's original save and stops Play. Results are written to `Saved/ConversationValidation.json`. Start Play again to resume the original term.

Conversation reactions remain on screen while the current overlay is open; persistent commitments, relationships and meeting history stay in the existing term save. No save migration is required.
