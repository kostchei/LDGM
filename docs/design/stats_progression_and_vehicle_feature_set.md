# Stats, progression, and vehicle feature-set discussion

**Status:** Proposed design baseline for discussion  
**Recorded:** 2026-07-16  
**Source:** Original design notes supplied by the project owner, preserved verbatim in Appendix A  
**Scope:** Player stats, Fame, perception and handling options, vehicle ratings, component condition, equipment grades, slots, locations, and progression gates

## 1. Purpose

This document turns the original notes into a coherent player-facing system. It is deliberately a design proposal rather than an implementation specification. Numeric values below are starting targets for playtesting; the distinctions between systems are the important decisions.

The intended style is a compact, characterful driver card attached to a much richer vehicle. The player should understand the important numbers at a glance, while the physical vehicle remains the star. Stats should create stories and small advantages, not replace driving skill, make a bad vehicle magically good, or create hidden internal-mechanics simulation.

## 2. Recommended system boundaries

The original notes mix four different kinds of value. They should remain separate in the game and in saved data.

| Kind | Values | Meaning |
|---|---|---|
| Driver attributes | Knowhow, Grit, Luck, Perception | Persistent 3d6 character qualities |
| Progression | Permanent Fame, Display Fame | Earned reputation and currently displayed prestige |
| Player options | Handling sensitivity, Wide/Far perception mode, head-look | Player-selected controls and presentation; not character power |
| Vehicle state | Measured performance ratings, equipment grade, condition, fuel, ammunition, damage zones | Properties of the current vehicle and loadout |

This separation resolves two ambiguities in the source notes:

- **Fame is not rolled on 3d6.** It is progression, starts at 0, and can exceed 18. This makes a Level 2 threshold of 20 meaningful.
- **Handling is not a rolled stat.** It is an input sensitivity/response setting. Changing it never changes steering lock, tire grip, turning radius, or any other authoritative vehicle capability.

Unless this proposal is changed explicitly, “starting stats use 3d6” therefore means Knowhow, Grit, Luck, and Perception.

## 3. Presentation style

The stats screen should look like a battered driver licence, race card, or workshop tag rather than a modern RPG spreadsheet. Each attribute is shown as:

- its name;
- its 3–18 score;
- a plain-language band;
- one short sentence describing the current effect;
- no exposed formula or fractional modifier in the normal UI.

Recommended score bands:

| Score | Label | Design meaning |
|---:|---|---|
| 3–5 | Shaky | A noticeable weakness, but never a soft lock |
| 6–8 | Rough | A modest disadvantage |
| 9–12 | Capable | Baseline play |
| 13–15 | Seasoned | A modest advantage |
| 16–18 | Notorious | A strong but tightly bounded advantage |

Effects should be banded rather than recalculated for every point. That makes the numbers legible, keeps balance testable, and avoids implying precision the game does not need.

## 4. Character creation

Recommended default:

1. Roll 3d6 once, in fixed order, for Knowhow, Grit, Luck, and Perception.
2. Show all four results together.
3. Allow one complete reroll of the whole set before the character is accepted.
4. Store the accepted rolls and generation version permanently.
5. Start Permanent Fame and Display Fame at 0.

The one full reroll preserves the desired dice-generated identity while preventing endless per-stat reroll optimization. A later competitive mode may normalize direct combat effects, but it should not silently replace the accepted character sheet.

## 5. Exact stat roles

### 5.1 Fame

Fame is the long-term progression spine, not a conventional attribute.

**Permanent Fame** is earned from completed missions, public victories, difficult recoveries, and other authoritative progression events. It is never reduced by changing clothes or vehicle paint. It gates access to cosmetic catalogues, prestigious missions, and Level 2.

**Display Fame** comes from currently equipped paint, trophies, banners, rare bodywork, and other visible cosmetics. It is bounded to **0–5** and changes when the displayed loadout changes.

Recommended rules:

- Level 2 unlocks at **20 Permanent Fame**. Display Fame does not unlock Level 2.
- Cosmetic purchase tiers use Permanent Fame: 5, 10, 15, and 20 are the initial catalogue thresholds.
- NPC intimidation uses **Effective Fame = Permanent Fame + Display Fame**.
- An NPC archetype has a `stand_ground`, `hesitate`, and `flee` threshold. Crossing a threshold changes its decision weights; it does not mind-control an NPC already committed to a fight.
- Display Fame can help scare weak NPCs, but cannot create permanent progression by equipping and unequipping cosmetics.
- Fame does not modify weapon damage, armour, vehicle physics, shop prices, or loot rolls unless a later economy design says so explicitly.

This honours both original ideas: Fame buys cosmetics, and visible cosmetics add to the reputation an NPC perceives.

### 5.2 Knowhow

Knowhow represents practical workshop judgement. It improves repair efficiency and the quality of information shown about external parts and broad vehicle zones.

It does **not** reveal simulated pistons, fuel lines, oil pumps, or other internal machinery. More information means better estimates about authored components and condition states.

Recommended band effects:

| Band | Repair effect | Information effect |
|---|---|---|
| Shaky | Repair cost +15% | Zone severity only; estimates shown as broad ranges |
| Rough | Repair cost +5% | Zone and disabled-part identity |
| Capable | Baseline cost | Zone, effective grade, and normal repair quote |
| Seasoned | Repair cost −10% | Adds before/after performance preview |
| Notorious | Repair cost −15%; salvage yield +10% | Adds exact authored modifier and failure-risk preview |

Knowhow should improve decisions and resource efficiency. It must not make field repairs instantaneous or repair a destroyed vehicle during active driving.

### 5.3 Grit

Grit represents the driver continuing to function while hurt, shaken, tilted, or battered by the road.

It modifies the **driver response**, not the underlying Chrono vehicle motion. A high-Grit character still experiences the same collision impulse, tire failure, rollover risk, and steering geometry. The camera and control feedback become less disruptive within comfort limits.

Recommended band effects:

| Band | Driver injury duration | Camera/aim disturbance scale | Special rule |
|---|---:|---:|---|
| Shaky | 120% | 115% | None |
| Rough | 110% | 105% | None |
| Capable | 100% | 100% | None |
| Seasoned | 90% | 90% | Severe reactions recover one step sooner |
| Notorious | 80% | 80% | Once per mission, downgrade the first non-lethal driver injury by one severity |

Accessibility camera-shake settings remain separate and can reduce visual motion further. Grit must never force a player to accept uncomfortable shake.

### 5.4 Luck

Luck creates occasional stories at the edges of uncertain events. It must not visibly overturn clear physical outcomes.

Allowed uses:

- small changes to PvE enemy near-miss dispersion;
- small changes to unreliable or “squirrely” weapon outcomes when a shot was already plausibly on target;
- enemy weapon jams and recovery timing;
- fuel, ammunition, and salvage finds;
- authored mission hazards and random events.

Disallowed uses:

- changing projectile paths after a resolved hit;
- negating a direct collision;
- multiplying weapon damage;
- client-side random rolls;
- hidden direct aim assistance against another human player.

Recommended modifier by band: **−5, −2, 0, +2, or +5 percentage points** from Shaky through Notorious. Every event also has its own hard probability floor and ceiling.

For PvP, the default policy is conservative: Luck may affect salvage, jams, and non-combat world events, but not whether one player’s valid shot hits another player. PvE near misses and “squirrely hit” adjustments are server-authoritative, deterministically seeded, and limited to plausible outcomes.

### 5.5 Perception

Perception affects recognition and the strength of environmental cues. Before a mission, the player chooses one of two cockpit presentation modes:

| Mode | Initial target | Tradeoff |
|---|---|---|
| Wide | Approximately 100° horizontal field of view at 16:9 | Better peripheral awareness; normal recognition distance |
| Far | Approximately 85° horizontal field of view at 16:9 | 25% greater authored recognition/cue distance; narrower view |

The player can turn their head **30° left and 30° right** in either mode. There is no chase camera and no external driving view.

Perception band modifies recognition time and cue distance by **−10%, −5%, 0%, +5%, or +10%**. It can affect when a distant silhouette is identified, when a route marker becomes readable, or when a peripheral threat cue appears. It does not change authoritative projectile range, weapon accuracy, fog geometry, or whether the server considers a target physically visible.

The final FOV must remain adjustable for accessibility. If the player overrides it, the chosen Wide/Far mode should preserve its information tradeoff rather than forcibly changing the accessibility setting.

### 5.6 Handling sensitivity

Handling is a control preference with a default value of **1.0** and a proposed range of **0.5–1.5**. It scales input response before the command reaches the authoritative vehicle controller.

- Lower values give finer steering around centre.
- Higher values reach the requested steering input faster.
- Dead zone, inversion, response curve, and sensitivity are configured separately for mouse, gamepad, wheel, and HOTAS axes.
- The setting never increases maximum steering angle, steering rate, traction, braking, or turn performance.
- The UI should call it **Steering Sensitivity**, not a character “Handling” stat, to avoid implying vehicle power.

## 6. Vehicle stats and ratings

Character stats must not absorb vehicle properties. The garage should present a separate vehicle card derived from measured benchmark runs with the current loadout:

- acceleration;
- top speed;
- braking distance;
- turning circle;
- rough-terrain pace;
- sand pace;
- stability/roll risk;
- protection by armour face;
- fuel range;
- cargo capacity.

Use five coarse rating bars plus the underlying practical measurement where useful—for example, “Braking 3/5; 42 m from 80 km/h.” Ratings are regenerated from controlled tests or authored curves, not guessed by adding unrelated item scores.

Damage and loadout changes show clear deltas from the healthy baseline. Heavy overhangs, rams, armour, cargo, wings, arms, and trailing structures contribute mass and centre-of-mass offsets to whole-vehicle physics. They may worsen braking, turning, stability, suspension loading, and terrain performance.

## 7. Realism and damage policy

The target is **realistic vehicle consequences with intentionally abstract internal causes**.

The original notes ask for blown tires, off-kilter fans, piston timing, uneven roads, sand, gravel, and realistic component damage. The current project boundary excludes detailed internal-mechanics simulation. The recommended reconciliation is:

- Tires and wheels are real mobility components. A blown tire changes tire stiffness, grip, drag, ride height, and handling through the vehicle simulation.
- Road roughness, gravel, firm sand, and deep sand use distinct physical surface/contact behaviour.
- A “rough-running engine,” “bad fan,” or “timing problem” is an authored broad power-condition effect: reduced or oscillating power, changed sound, warning indications, and bounded shake. No individual piston, fan bearing, or timing chain is simulated.
- External weapons, armour faces, tires, rams, cages, fields, cargo mounts, and exposed equipment may have individual condition because players can see, replace, and reason about them.
- The engine bay, cabin, cargo area, frame, and body are broad zones. Damage can affect their authored output without exposing an internal component tree.

This keeps the player-facing fantasy of a damaged machine while avoiding a maintenance simulator and an unbounded damage-content problem.

Recommended damage granularity:

| Area | Tracked state | Typical outcomes |
|---|---|---|
| Each tire/wheel station | Healthy, damaged, critical, destroyed | Grip loss, drag, pull, reduced ride stability |
| Frame | Broad severity | Alignment penalty, load limit reduction, instability |
| Engine bay/power source | Broad severity | Acceleration/top-speed loss, rough-running audio and shake |
| Cabin/driver | Broad severity plus driver injury | View/control disturbance, injury state, mission failure at extreme severity |
| Each weapon | Grade, condition, ammunition, enabled state | Dispersion, jam chance, disabled weapon |
| Armour face: front/rear/left/right/top/bottom | Coverage and condition | Damage mitigation and repair burden |
| External attachment | Grade and condition | Disabled effect, altered mass still present until removed |
| Cargo area | Capacity and cargo condition | Lost/damaged mission cargo, shifted load where authored |

## 8. Equipment grade and condition

Use non-negative effective grades.

- **Base Grade:** 1–5.
- **Condition Loss:** 0–5.
- **Effective Grade:** `max(0, Base Grade − Condition Loss)`.
- **Grade 0:** scrap or non-functional.
- Starting equipment is Base Grade 1.
- A starting item may have at most one authored **+1 modifier** and no stronger positive modifier.

Keeping Effective Grade at 0–5 avoids confusing negative equipment values. “Total scrap being −4” is represented by a Grade 1 item with Condition Loss 5 and Effective Grade 0; excess loss has no further mechanical meaning.

Grade describes construction quality. Condition describes the current state of that particular item. Repair reduces Condition Loss but does not increase Base Grade. Upgrading or replacing an item changes Base Grade.

## 9. Vehicle construction and slots

The loadout model should expose the following top-level slots:

- cockpit HUD or link;
- body/shell/skin;
- outer cover;
- frame;
- engine bay;
- cabin;
- cargo area;
- weapons;
- front/rear rams and bullbars;
- side/top extensions;
- trailing overhangs.

The **frame** determines wheel configuration, base mass, weight limit, bay/cabin/cargo layout, hard points, and basic dimensions.

The **body** determines shell shape, visibility, cockpit presentation, and cosmetic skin.

The **outer cover** may contain ablative armour, RPG/drone cages, a stealth field, refractor field, conversion field, paint, and trophies. Advanced fields remain external equipment with mass, charge/ammunition, broad effect, and condition. They are not internally simulated subsystems.

The engine bay, cabin, and cargo layout each provide authored space. Each applicable zone supports armour on front, rear, left, right, top, and bottom faces. Content authors must define mass, space use, mount position, and centre-of-mass contribution for every physical attachment.

The **HUD or link** is a cockpit interface slot. It determines which otherwise-authorized cues can be presented and how damaged or degraded those cues become; it does not grant access to hidden internal diagnostics. HOTAS assignments are player input bindings, not equipment. Every driving, firing, perception-mode, head-look, and permitted auxiliary action must be bindable without changing the character or vehicle sheet.

## 10. Starter vehicle and weapons

The Level 1 starting vehicle is one light technical with:

- two fixed-forward assault rifles, one left and one right;
- 50 rounds per weapon;
- cabin-mounted iron sights zeroed for approximately 100 metres;
- a basic bullbar;
- Grade 1 starting gear with no modifier greater than +1;
- a first-person wide cockpit view;
- one rear cargo rack or comparable basic cargo space.

The two weapons fire through the same authoritative input but retain separate ammunition, jam, condition, and disabled states. Fixed-forward means the player aims by positioning the vehicle; any later convergence or elevation adjustment requires an explicit design change.

## 11. Missions, rewards, and locations

Level 1 contains four mission families:

1. transport a package;
2. raid a camp;
3. kill/intercept a bike or pedestrian target;
4. fetch or recover salvage.

The target expected gross reward for a normal completed Level 1 mission is **45–55% of the replacement price of a basic technical**, plus enough fuel and ammunition to avoid making a successful run immediately non-viable. Repair, risk, and consumable costs still matter, so gross reward is not guaranteed profit.

**Bartertown** is a safe hub with free rest, ordinary shops, and junk-price buying/selling.

**Despot Depots** are paid service hubs inside contested PvP bands. They may offer rest, auction access, and specialist science-fiction equipment such as lasers, conversion fields, stealth fields, nuclear power sources, atomic weapons, rail guns, and mole mortars. These are later-progression content, not part of the first driving/combat slice.

The zone map is a maze-like network in which PvP areas act as dangerous “walls” or bands between safer routes. Players may navigate around them or cross them for shortcuts, rare services, and better rewards.

## 12. Feature rollout

| Tranche | Stats/feature work |
|---|---|
| T1 | Steering sensitivity, Wide/Far cockpit mode, ±30° head-look, measured vehicle ratings |
| T2 | Tire/weapon/broad-zone condition, Grit feedback hooks, authored damaged power behaviour |
| T3 | Starter technical, four readable stat summaries, package mission, Bartertown repair presentation |
| T4 | Persistent 3d6 driver attributes, Permanent/Display Fame, Knowhow repair effects, Luck event policy, all four Level 1 missions |
| T5 | Server-authoritative stat effects and explicit PvP normalization/audit |
| T6 | PvP bands, Despot Depot, auction/service integration |
| T7 | Balance and performance qualification without changing the 30-vehicle cap |
| T8 | Permanent Fame 20 gate and specialist external equipment |

## 13. Proposed decisions to treat as defaults

Unless changed after discussion, the implementation-facing design should assume:

1. Knowhow, Grit, Luck, and Perception are the four 3d6 attributes.
2. Character creation uses fixed-order rolls and permits one complete reroll.
3. Fame starts at 0 and is earned; it is not rolled.
4. Display Fame is capped at 5 and affects NPC perception, not permanent unlocks.
5. Level 2 requires 20 Permanent Fame.
6. Luck never changes direct player-versus-player hit resolution.
7. Perception mode is selected before deployment; head-look is ±30°.
8. Steering Sensitivity is a player option, not a character stat.
9. Equipment Effective Grade is clamped to 0–5; there are no negative grades.
10. Internal faults are authored broad effects. Only physically meaningful, exposed, replaceable components receive individual condition.
11. The starter technical has two separately tracked fixed rifles with 50 rounds each and a 100 m sight zero.
12. Vehicle construction always accounts for attachment mass and centre-of-mass offset.

## Appendix A: Original notes, preserved verbatim

```text
**Legally Distinct Gorka Morka**

**Fame - let you buy cosmetics, scares off npcs at a higher threshold** 
**Knowhow - repair vehicle better, more info about components** 
**Grit - ignore driver damage, less impacted byl shake due to surface, off kilter tires or engine or vehicle being impacted**
**Luck - chance of shots missing you, more chance of squirlily weapons hitting your enemy, range of things like enemies jamming, finding more fuel or ammo**
**Perception you can either see further or wider (slightly). Can change it before the mission. Can then head 30 degrees each way*,*** 
**Handling you can turn the sensitivity up or down.**


**Need realistic vehicle handling physics including things like blown tires, off kilter fans or piston timing, uneven road surfaces, sand, gravel.**

**Need realistic damage model of components** 

**First person perspective but wide view**


**Start with a technical with forward mounted 2x assault rifles and cabin mounted iron sights for 100 meters. Left and right but only 50 rounds each.**

**Mission board.**
**Lv 1. Transport a package, raid a camp, kill a target (bike or pedestrian), fetch,** 
**Reward should be about 50% of a technical plus fuel and ammo.**
**Starting stats is 3d6**
**Lv 2. (Fame cap minimum 20)**



**Cosmetics can add to Fame**
**Starting gear max +1**
**Gear levels 1-5 with damaged being 1 to 5 less (total scrap being -4) ( maybe avoid negatives?, does it matter?)**

**Zone map.- basically a maze with pvp areas being the walls, ie you can navigate** 

**Despot depots- paid rest, auction house, in pvp areas, specialist scofi gear. Lasers, conversion fields, stealth fields, nuclear power plants, atomic weapons, rail guns, mole mortar,** 

**Bartertown - safe rest for free, shops, buy at junk prices.** 


**Slots.**
**Heads up display or link**
**Assigned  buttons on hotas** 
**Body (shell, skin)**
**Outer cover- ablative armour, field (stealth, refractor- deflects mass, conversion - detects speed ie dune), drone and rpg cage, snazzy paint job**
**Frame determines the wheel config, weight limit, bay, cabin and cargo layout**

**Then based on layout each zone has a certain amount of space**
**Engine bay**
**Cabin**
**Cargo**
**Each has sides where armour can be added, including top and bottom.**

**Then rams or bullbars,extensions like wings or arms, trailing overhangs.**
**Physics needs to account for such overhang weight** 
```
