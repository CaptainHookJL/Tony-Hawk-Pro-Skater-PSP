# Sidewalk Sessions

Working title for an original arcade skateboarding game built for PSP.

## Direction

Short sessions, approachable controls, and satisfying combos in compact original
parks. The first park, **Foundry Yard**, is a converted industrial courtyard:
two launch banks, a central rail, a loading platform, and five gold spot markers.
The surrounding warehouses and containers are scenery outside the skating area.
The skater, park, markers, and game logo are created in code.

## Play loop

Choose a two-minute Score Run or untimed Free Skate from the title screen.
Push, pop, perform tricks, and land cleanly to bank the combo. Bails lose the
pending combo. Explore the park for spot markers and complete the five challenges.
Start opens the challenge card and pauses both skating and the session clock.
Triangle on that card ends the session.

At the score-run deadline, a current airborne or grinding combo gets up to three
seconds to land. Fresh ollies, flips, and grabs cannot start in that window.
An unfinished combo is discarded when the window ends. Results show the score,
best combo, challenge count, collected spots, landed flips, grinding time, and bails.
Cross retries; Circle or Start returns to the title screen.

Free Skate uses the same park and challenges without a clock. End it through
the pause card. It can improve the all-time combo record; the timed score and
challenge records belong to Score Runs.

## Challenges and medals

| Challenge | Completion rule |
| --- | --- |
| Score 2,500 points | Banked session points reach 2,500 |
| Bank a 1,000-point combo | One landed combo reaches 1,000 |
| Land 3 kickflips | Three completed flips survive their landings |
| Land 1 second of grinding | Accumulated grinding time from banked combos reaches one second |
| Find all 5 spot markers | Touch each marker in the current session |

| Score-run medal | Requirement |
| --- | --- |
| Bronze | At least 1,000 points |
| Silver | At least 2,500 points and three challenges |
| Gold | At least 5,000 points and all five challenges |

Challenges and markers reset for every new session. Respawning preserves banked
points, completed challenges, and collected markers, while discarding the current
unbanked combo. Pausing, respawning, and retrying cannot carry an unfinished combo
into a new run.

Personal records are saved when a session ends. The save contains the best timed
score, best combo across modes, highest timed challenge count, and completed
score-run count. A backup retains the previous save. Invalid or missing main
saves recover from that backup, or start fresh if neither file is usable.
A failed write is reported on the results screen.

## Current playable version

Title screen, two modes, timed runs and landing grace, pause/challenge card,
results/retry, persistent personal records, original industrial park, collectible
markers, ollies, kickflips, grabs, spins, rail grinding, combo scoring, and bails.

The character uses simple block geometry and the park uses flat colors. Audio,
manuals, reverts, character customization, extra parks, and objective campaigns
are future additions. Real PSP controls, performance, and save behavior still
need device testing.

## Next development priorities

1. Play-test steering, jumps, rail entry, and camera behavior with a controller.
2. Add manuals so players can carry a combo between obstacles.
3. Add trick, landing, bail, and goal sounds, then improve skater animation.
4. Add varied rails and a second original park with its own challenges.
5. Profile frame time and memory on PSP hardware before expanding scene detail.

The technical baseline targets standard PSP user memory and 480 x 272 rendering.
Keep geometry and texture budgets explicit as art becomes more detailed. Each
new mechanic should have a gameplay scenario test and a PSP integration check.
