# Cyber Rayne — Master Asset List

Tracks generated sources, runtime assets, and remaining art. Updated 2026-11-10
against the repository, asset validator, item catalog, and ability registry.
[ASSET_SPEC.md](ASSET_SPEC.md) defines runtime dimensions and preparation rules.

**Completed:** 17 approved base sprites and 22 animation sheets are generated,
conformed, and integrated. Their originals and exact prompts are in
[CyberRayne/Raw/](CyberRayne/Raw/MANIFEST.md) and
[Raw/sheets/](CyberRayne/Raw/sheets/README.md). All **40 runtime image files** pass
`python3 CyberRayne/tools/validate_assets.py` (39 character/sheet assets + font).
Animation timing, facing and visual polish still need manual playtesting.

**New raw art:** 9 item/story and equipment icons are saved with exact prompts.
See the [icon manifest](CyberRayne/Raw/ICON_MANIFEST.md). These originals still
need conversion to 32x32 and runtime integration.

| Status | Meaning |
| --- | --- |
| **[IN GAME]** | Runtime file exists, is used by the game, and passed the asset validator. |
| **[RAW]** | Generated original exists in `CyberRayne/Raw/`; conversion/integration remains. |
| **[MISSING]** | Planned art has no generated source or runtime file yet. |
| **[COVERED]** | Existing code or another asset supplies the visual; no separate image required. |
| **[DEFERRED]** | Optional enhancement or full-game wishlist; not a slice blocker. |
| **[CUT]** | Deliberately removed or superseded; do not generate. |
| **[FINAL]** | Explicitly locked for shipping after visual review. |

IN GAME is not a claim that animation polish or a final manual playthrough is done.
Raw generation is tracked separately so finished art is not requested twice.

### Asset workflow

Keep original generated files and exact prompts in `CyberRayne/Raw/`. Write
conformed copies into the appropriate `CyberRayne/assets/` category. Preserve the
approved originals and earlier versions.

Run from `CyberRayne/`:

```bash
python3 tools/spriteprep.py Raw/<name>.png --category characters --no-palette --outdir assets/characters
python3 tools/sheetprep.py Raw/sheets/<name>_walk.png
python3 tools/validate_assets.py
```

Characters/NPCs are now **96x128**, with original colours retained. Enemies range
from 96x96 to 192x192. Icons remain 32x32; see the spec for other categories.
Do not run the single-sprite converter on an animation sheet.

---

## 1. Tile Textures — **[DONE, procedural]**

Tiles are **generated in code**, not authored as files. `Map::loadTileTextures()`
calls `TileTextureGen::generate()` for all 15 types and uploads them via
`loadTextureFromMemory()`. Nothing is read from disk.

All 15 types render: CONCRETE, ROAD, NEON, METAL, HOLOGRAM, VENDING, plus the
9 legacy fantasy types. Verified seamless (3x3 tiling check) and in-game.

**Nothing in this section needs an artist.** To change a tile's look, edit the
constants in the matching `make*()` function in `TileTextureGen.cpp`. To add a
type: enum case in `Tile.h`, a `make*()` function, and an entry in `kTileTypes`
in `Map.cpp`. See ASSET_SPEC.md section 4.

### Stale files — removed
The 9 fantasy tile images under `assets/textures/tiles/` were deleted on 2026-11-10
along with the other unreferenced fantasy-era files. They remain in git history.

### Tile types deferred, not cut
The 17 speculative types previously listed here (sidewalk, crosswalk, subway_platform,
graffiti, puddle, …) are **[DEFERRED]**. They are map-design scope, not an art
blocker — add them procedurally when a map actually needs them. Doing so costs one
function each, not one commission each.

---

## 2. Party Sprites (`assets/characters/`)

Rayne uses the class names `netrunner`, `solo`, and `infiltrator`. Companions use
their own names, `cyph` and `vash`; this naming decision is resolved.

| Asset | Status | Notes |
| --- | --- | --- |
| netrunner.png | [IN GAME] | Rayne: hooded coat and forearm cyberdeck |
| solo.png | [IN GAME] | Rayne: armored jacket and heavy pistol |
| infiltrator.png | [IN GAME] | Rayne: sleek suit and twin blades |
| cyph.png | [IN GAME] | Distinct lanky hacker companion: visor and cyan cables |
| vash.png | [IN GAME] | Distinct wiry rogue companion: face scarf and knives |

### Party animation sheets

| Asset | Status | Notes |
| --- | --- | --- |
| netrunner_walk.png | [IN GAME] | 4x4: down, left, right, up; four frames per direction |
| netrunner_battle.png | [IN GAME] | 4x4: idle, attack, ability, guard |
| solo_walk.png | [IN GAME] | 4x4: down, left, right, up; four frames per direction |
| solo_battle.png | [IN GAME] | 4x4: idle, attack, ability, guard |
| infiltrator_walk.png | [IN GAME] | 4x4: down, left, right, up; four frames per direction |
| infiltrator_battle.png | [IN GAME] | 4x4: idle, attack, ability, guard |
| cyph_walk.png | [IN GAME] | 4x4: down, left, right, up; four frames per direction |
| cyph_battle.png | [IN GAME] | 4x4: idle, attack, ability, guard |
| vash_walk.png | [IN GAME] | 4x4: down, left, right, up; four frames per direction |
| vash_battle.png | [IN GAME] | 4x4: idle, attack, ability, guard |

Side/back views are included in the walking sheets. Separate `<class>_side.png`
and `<class>_back.png` files are **[COVERED]** and do not need generating.
Hurt flashes and defeat effects remain **[CUT]** as standalone art; `BattleFx`
supplies runtime effects.

---

## 3. Enemy Sprites (`assets/enemies/`)

| Asset | Status | Notes |
| --- | --- | --- |
| street_thug.png | [IN GAME] | Mohawk, vest and pipe |
| street_thug_battle.png | [IN GAME] | 4x4: idle, attack, ability, guard |
| security_drone.png | [IN GAME] | Round hover drone, red eye and taser prongs |
| security_drone_battle.png | [IN GAME] | 4x4: idle, attack, ability, guard |
| black_ice.png | [IN GAME] | Purple/magenta crystalline digital entity |
| black_ice_battle.png | [IN GAME] | 4x4: idle, attack, ability, guard |
| combat_cyborg.png | [IN GAME] | Bulky metal limbs and orange optics |
| combat_cyborg_battle.png | [IN GAME] | 4x4: idle, attack, ability, guard |
| corporate_enforcer.png | [IN GAME] | Slice boss: riot armor, shield and shock baton |
| corporate_enforcer_battle.png | [IN GAME] | 4x4: idle, attack, ability, guard |

`black_ice.png` and `black_ice_battle.png` supersede the old enemy Netrunner
filenames. Corporate Enforcer is the slice boss; no additional boss is needed for
this story chain. Enemy map sprites currently use their standing images.

### Additional enemies for full game

| Asset | Status | Notes |
| --- | --- | --- |
| corporate_soldier.png | [DEFERRED] | Mid-tier corporate guard |
| riot_bot.png | [DEFERRED] | Riot control robot |
| spider_bot.png | [DEFERRED] | Small crawling bot |
| turret.png | [DEFERRED] | Stationary turret |
| boss_data_king.png | [DEFERRED] | Future data broker kingpin |
| boss_corp_ceo.png | [DEFERRED] | Future corporate CEO boss |
| boss_ai_core.png | [DEFERRED] | Future rogue AI boss |
| goblin.png | [CUT] | Removed fantasy-era art |
| enemy_hurt.png | [CUT] | Runtime hit flash |
| enemy_defeated.png | [CUT] | Runtime defeat fade |

---

## 4. NPC Sprites (`assets/npcs/`)

All seven map NPCs have distinct approved designs.

| Asset | Status | Notes |
| --- | --- | --- |
| fixer_kaga.png | [IN GAME] | Quest giver; long coat and cigarette |
| fixer_kaga_idle_talk.png | [IN GAME] | 4x2: idle, talk |
| street_vendor.png | [IN GAME] | Recovery service; apron, noodle bowl and ladle |
| street_vendor_idle_talk.png | [IN GAME] | 4x2: idle, talk |
| info_broker.png | [IN GAME] | Story NPC; mirrored glasses and data tablet |
| info_broker_idle_talk.png | [IN GAME] | 4x2: idle, talk |
| smuggler_ryo.png | [IN GAME] | Neon Alley; large backpack |
| smuggler_ryo_idle_talk.png | [IN GAME] | 4x2: idle, talk |
| hacker_zero.png | [IN GAME] | Neon Alley; teenage hacker, headset and hoodie |
| hacker_zero_idle_talk.png | [IN GAME] | 4x2: idle, talk |
| exec_tanaka.png | [IN GAME] | Corporate Plaza; sharp suit and cold expression |
| exec_tanaka_idle_talk.png | [IN GAME] | 4x2: idle, talk |
| security_chief.png | [IN GAME] | Corporate Plaza; uniform, cap and holstered sidearm |
| security_chief_idle_talk.png | [IN GAME] | 4x2: idle, talk |

### Additional NPCs for full game
| Asset | Status | Notes |
|-------|--------|-------|
| bartender.png | [DEFERRED] | Bar/social hub |
| medic.png | [DEFERRED] | Healing station |
| black_market_dealer.png | [DEFERRED] | Underground shop |
| street_kid.png | [DEFERRED] | Ambient NPC — flavor text |
| corporate_worker.png | [DEFERRED] | Ambient NPC — Corporate Plaza |
| homeless_npc.png | [DEFERRED] | Ambient NPC — Neon Alley |
| traffic_drone.png | [DEFERRED] | Ambient NPC — flying traffic |
| shopkeeper_general.png | [DEFERRED] | General store |
| weapon_dealer.png | [DEFERRED] | Weapon shop |
| armor_dealer.png | [DEFERRED] | Armor shop |
| item_dealer.png | [DEFERRED] | Consumable shop |
| netrunner_vendor.png | [DEFERRED] | Sells hacking programs |
| taxi_driver.png | [DEFERRED] | Fast travel |
| subway_attendant.png | [DEFERRED] | Fast travel |
| gang_leader.png | [DEFERRED] | Faction quest giver |
| corporate_executive.png | [DEFERRED] | Corporate Plaza quest giver |
| police_officer.png | [DEFERRED] | Law enforcement NPC |
| prostitute.png | [DEFERRED] | Neon Alley ambient |
| drug_dealer.png | [DEFERRED] | Neon Alley ambient |

---

## 5. Vehicles (`assets/vehicles/`)

| Asset | Status | Notes |
|-------|--------|-------|
| hover_car.png | [DEFERRED] | Flying car — ambient traffic |
| hover_car_2.png | [DEFERRED] | Variant |
| hover_car_3.png | [DEFERRED] | Variant |
| police_hover.png | [DEFERRED] | Police flying vehicle |
| ambulance_hover.png | [DEFERRED] | Emergency vehicle |
| delivery_drone.png | [DEFERRED] | Package delivery drone |
| subway_train.png | [DEFERRED] | Subway train — background |
| motorcycle.png | [DEFERRED] | Street motorcycle |
| motorcycle_electric.png | [DEFERRED] | Electric bike variant |
| van.png | [DEFERRED] | Delivery van |
| armored_truck.png | [DEFERRED] | Corporate transport |
| taxi_hover.png | [DEFERRED] | Flying taxi — fast travel |
| garbage_truck.png | [DEFERRED] | Ambient |
| fire_drone.png | [DEFERRED] | Emergency response drone |
| news_drone.png | [DEFERRED] | Media drone — ambient |
| construction_mech.png | [DEFERRED] | Construction mech — ambient |
| billboard_car.png | [DEFERRED] | Mobile ad vehicle |

---

## 6. UI Assets (`assets/ui/`)

| Asset | Status | Notes |
|-------|--------|-------|
| menu_background.png | [CUT] | Main menu background. Title and character select are drawn from code now; file deleted 2026-11-10 |
| start.png | [CUT] | Start button. Menu buttons are text `UIButton` widgets now; file deleted 2026-11-10 |
| loadgame.png | [CUT] | Load game button. Menu buttons are text `UIButton` widgets now; file deleted 2026-11-10 |
| settings.png | [CUT] | Settings button. Menu buttons are text `UIButton` widgets now; file deleted 2026-11-10 |
| quit.png | [CUT] | Quit button. Menu buttons are text `UIButton` widgets now; file deleted 2026-11-10 |
| cursor.png | [CUT] | Menu cursor. Title and character select are drawn from code now; file deleted 2026-11-10 |
| namebanner.png | [CUT] | Game title banner. Title and character select are drawn from code now; file deleted 2026-11-10 |

### Optional UI art and background replacements

The current menus, overlays and per-district battle backdrops are drawn from
code. Background artwork, if commissioned, belongs in `assets/backgrounds/`.
| Asset | Status | Notes |
|-------|--------|-------|
| battle_background.png | [COVERED] | Procedural battle backdrop in BattleScene; no generic image required |
| battle_background_alley.png | [DEFERRED] | Neon Alley battle bg |
| battle_background_plaza.png | [DEFERRED] | Corporate Plaza battle bg |
| battle_background_central.png | [DEFERRED] | Neo-Kyoto Central battle bg |
| dialogue_box.png | [COVERED] | StoryHud and Theme draw dialogue panels |
| pause_overlay.png | [COVERED] | PauseMenu and Theme draw the pause overlay |
| game_over.png | [COVERED] | StoryHud draws the game-over card |
| victory.png | [COVERED] | StoryHud draws battle results |
| bar_fill.png | [COVERED] | HP/MP/ATB/XP fills are colored geometry |
| ~~hp_bar.png~~ | [CUT] | Runtime colored geometry |
| ~~mp_bar.png~~ | [CUT] | Runtime colored geometry |
| ~~atb_bar.png~~ | [CUT] | Runtime colored geometry |
| ~~xp_bar.png~~ | [CUT] | Runtime colored geometry |
| status_poison.png | [DEFERRED] | Poison status icon |
| status_regen.png | [DEFERRED] | Regen status icon |
| status_stun.png | [DEFERRED] | Stun status icon |
| status_buff.png | [DEFERRED] | Attack buff icon |
| status_debuff.png | [DEFERRED] | Defense debuff icon |
| minimap.png | [DEFERRED] | Minimap frame |
| ~~minimap_player.png~~ | [CUT] | Colored quad, push-constant color |
| ~~minimap_npc.png~~ | [CUT] | Colored quad |
| ~~minimap_enemy.png~~ | [CUT] | Colored quad |
| ~~minimap_transition.png~~ | [CUT] | Colored quad |
| inventory_bg.png | [COVERED] | PauseMenu draws inventory panels |
| quest_log_bg.png | [COVERED] | PauseMenu draws quest panels |
| shop_bg.png | [DEFERRED] | Shop screen background |
| tooltip_bg.png | [DEFERRED] | Tooltip background |
| button_small.png | [COVERED] | Buttons are drawn from code |
| button_large.png | [COVERED] | Buttons are drawn from code |
| panel_bg.png | [COVERED] | Theme draws panel backgrounds |
| scrollbar.png | [DEFERRED] | Scrollbar for lists |
| character_portrait_frame.png | [COVERED] | Character selection draws its frames |
| enemy_info_card.png | [COVERED] | BattleScene draws enemy information |
| weather_rain.png | [DEFERRED] | Rain particle overlay |
| ~~weather_fog.png~~ | [CUT] | Fullscreen shader, not an asset |
| ~~vignette.png~~ | [CUT] | Fullscreen shader math, not an asset |

---


## 7. Item and Ability Icons (`assets/items/`, `assets/spells/`)

Icons are additional presentation art; the current menus use text. Raw generation
and runtime integration are separate steps. Catalog icons use stable item IDs from
`Inventory.cpp`; the quest chip and currency are visual/story icons, not bag entries.

### Current slice items and story icons

| Asset | Status | Notes |
| --- | --- | --- |
| [stim_pack.png](CyberRayne/Raw/items/stim_pack.png) | [RAW] | Stim Pack: restores 40 HP; medical injector, not a temporary buff |
| [trauma_kit.png](CyberRayne/Raw/items/trauma_kit.png) | [RAW] | Trauma Kit: restores 120 HP; large field medical kit |
| [defib_patch.png](CyberRayne/Raw/items/defib_patch.png) | [RAW] | Defib Patch: revives at quarter health; paired electrode patch pack |
| [data_chip.png](CyberRayne/Raw/items/data_chip.png) | [RAW] | Story data chip from the three-quest chain |
| [credstick.png](CyberRayne/Raw/items/credstick.png) | [RAW] | Currency icon; gold/credits are not inventory stacks |

### Current party abilities

Proposed image filenames match the IDs in `AbilityRegistry.cpp`. No icon loader is
wired yet. These names replace the old generic spell list for the current classes.

| Asset | Status | Notes |
| --- | --- | --- |
| solo_suppressing_fire.png | [MISSING] | Suppressing Fire: damage to all enemies |
| solo_guard_protocol.png | [MISSING] | Guard Protocol: ally defense buff |
| solo_adrenal_surge.png | [MISSING] | Adrenal Surge: self attack buff |
| solo_executioner.png | [MISSING] | Executioner: heavy single-target strike |
| net_ice_breaker.png | [MISSING] | Ice Breaker: tech damage to one target |
| net_patch.png | [MISSING] | Patch: heal an ally |
| net_system_shock.png | [MISSING] | System Shock: area tech damage and stun |
| net_corrupt.png | [MISSING] | Corrupt: target defense debuff |
| inf_backstab.png | [MISSING] | Backstab: single-target strike |
| inf_neurotoxin.png | [MISSING] | Neurotoxin: poison |
| inf_smoke_screen.png | [MISSING] | Smoke Screen: slows enemies |
| inf_killswitch.png | [MISSING] | Killswitch: heavy single-target strike |

### Deferred item concepts (not current catalog IDs)
| Asset | Status | Notes |
|-------|--------|-------|
| health_potion.png | [DEFERRED] | Healing item icon |
| mana_potion.png | [DEFERRED] | Mana restoration icon |
| ether.png | [DEFERRED] | Full mana restore |
| phoenix_down.png | [DEFERRED] | Revive fallen ally |
| antidote.png | [DEFERRED] | Cure poison |
| medkit.png | [DEFERRED] | Large heal |
| grenade.png | [DEFERRED] | Battle item — AoE damage |
| emp_grenade.png | [DEFERRED] | Battle item — stun machines |
| smoke_bomb.png | [DEFERRED] | Battle item — flee boost |


### Deferred legacy ability concepts
| Asset | Status | Notes |
|-------|--------|-------|
| spell_fireball.png | [DEFERRED] | Netrunner offensive program |
| spell_ice.png | [DEFERRED] | Netrunner offensive program |
| spell_lightning.png | [DEFERRED] | Netrunner offensive program |
| spell_heal.png | [DEFERRED] | Netrunner healing program |
| spell_shield.png | [DEFERRED] | Netrunner defensive program |
| spell_hack.png | [DEFERRED] | Netrunner ability — debuff enemy |
| spell_overload.png | [DEFERRED] | Netrunner ability — AoE damage |
| spell_cloak.png | [DEFERRED] | Infiltrator ability — evasion buff |
| spell_backstab.png | [DEFERRED] | Infiltrator ability — high damage |
| spell_berserk.png | [DEFERRED] | Solo ability — attack buff |
| spell_taunt.png | [DEFERRED] | Solo ability — draw aggro |
| spell_charge.png | [DEFERRED] | Solo ability — rush attack |

---

## 8. Equipment Icons (`assets/equipment/`)

### Current slice equipment

Names match the stable IDs in `ItemCatalog`; weapons remain equippable through the
shared bag rather than being restricted by the appearance of a class sprite.

| Asset | Status | Notes |
| --- | --- | --- |
| [shock_baton.png](CyberRayne/Raw/equipment/shock_baton.png) | [RAW] | Shock Baton: ATK +4 |
| [mono_katana.png](CyberRayne/Raw/equipment/mono_katana.png) | [RAW] | Mono-Katana: ATK +8 |
| [street_jacket.png](CyberRayne/Raw/equipment/street_jacket.png) | [RAW] | Street Jacket: DEF +3 |
| [kevlar_weave.png](CyberRayne/Raw/equipment/kevlar_weave.png) | [RAW] | Kevlar Weave: DEF +6 |

### Deferred equipment concepts

These are visual wishlist ideas, not additional items in the current catalog.

#### Weapons
| Asset | Status | Notes |
|-------|--------|-------|
| weapon_katana.png | [DEFERRED] | Katana concept; current catalog uses mono_katana |
| weapon_monoblade.png | [DEFERRED] | Solo weapon upgrade |
| weapon_pistol.png | [DEFERRED] | Pistol concept; approved Rayne Solo carries a heavy pistol |
| weapon_smg.png | [DEFERRED] | Infiltrator weapon upgrade |
| weapon_shock_wand.png | [DEFERRED] | Netrunner weapon |
| weapon_data_staff.png | [DEFERRED] | Netrunner weapon upgrade |
| weapon_vibroblade.png | [DEFERRED] | Endgame weapon |
| weapon_plasma_rifle.png | [DEFERRED] | Endgame weapon |

#### Armor
| Asset | Status | Notes |
|-------|--------|-------|
| armor_streetwear.png | [DEFERRED] | Starting armor |
| armor_kevlar.png | [DEFERRED] | Mid-tier armor |
| armor_tactical.png | [DEFERRED] | High-tier armor |
| armor_corporate_suit.png | [DEFERRED] | Corporate Plaza disguise |
| armor_netrunner_suit.png | [DEFERRED] | Netrunner armor |
| armor_stealth_cloak.png | [DEFERRED] | Infiltrator armor |
| armor_heavy_exo.png | [DEFERRED] | Endgame armor |

#### Accessories
| Asset | Status | Notes |
|-------|--------|-------|
| acc_neural_implant.png | [DEFERRED] | Boosts magic |
| acc_reflex_chip.png | [DEFERRED] | Boosts speed |
| acc_armor_plating.png | [DEFERRED] | Boosts defense |
| acc_targeting_scope.png | [DEFERRED] | Boosts strength |
| acc_holo_shield.png | [DEFERRED] | Status resist |

---

## 9. Audio (`assets/audio/`)

Rewritten 2026-11-10 to match the vertical slice as it actually plays. The earlier
list dated from the fantasy prototype (fire and ice spells, a final boss) and is
replaced. Nothing here exists yet; `AudioManager` is still a stub until phase 12.

**Formats (proposed, to confirm in phase 12):**
- Music: `.ogg`, stereo, 44.1 kHz. Tracks marked *loop* must loop cleanly from end to
  start with no gap or click.
- Sound effects: `.wav`, mono, 44.1 kHz, 16-bit. Trim silence from the start so the
  sound lands on the action; most should be under one second. Leave a little headroom
  (peaks around -3 dB) so several can play at once.

Music lives in `assets/audio/music/`, sound effects in `assets/audio/sfx/`.

### Music (9 tracks; composer)

| File | Status | Where it plays |
|------|--------|----------------|
| title.ogg | [MISSING] | Title screen and character select. *Loop.* The main theme. |
| central.ogg | [MISSING] | Exploring Neo-Kyoto Central. *Loop.* Home district; calm. |
| alley.ogg | [MISSING] | Exploring Neon Alley. *Loop.* Seedier, tenser. |
| plaza.ogg | [MISSING] | Exploring Corporate Plaza. *Loop.* Cold, corporate. |
| battle.ogg | [MISSING] | Ordinary fights. *Loop.* |
| boss.ogg | [MISSING] | The two story fights (thugs, Enforcer). *Loop.* |
| victory.ogg | [MISSING] | Victory card. Short fanfare, then may loop quietly. |
| game_over.ogg | [MISSING] | Game-over card. Short, plays once. |
| ending.ogg | [MISSING] | Ending card after "The Truth". Plays once or loops. |

If the composer's time is short, the order of importance is: title, battle, central,
victory, then the rest. `alley` and `plaza` can share `central` at first, and `boss`
can share `battle`.

### Sound effects (32; generated)

**Menus and interface**

| File | Status | When |
|------|--------|------|
| ui_move.wav | [MISSING] | Cursor moves in any menu. Very short tick; heard constantly. |
| ui_confirm.wav | [MISSING] | A choice is confirmed. |
| ui_cancel.wav | [MISSING] | Backing out of a menu. |
| ui_error.wav | [MISSING] | Something refused: cannot afford an ability, item cannot be used. |
| ui_pause.wav | [MISSING] | Pause menu opens. |
| ui_save.wav | [MISSING] | Game saved. |
| ui_equip.wav | [MISSING] | Gear changed in the equipment screen. |

**Exploring**

| File | Status | When |
|------|--------|------|
| step.wav | [MISSING] | Footstep, one per tile walked. Soft; heard constantly. |
| travel.wav | [MISSING] | Moving between districts. |
| blocked.wav | [MISSING] | Turned back from a locked district. |
| talk_blip.wav | [MISSING] | Dialogue advances to the next line. |
| quest_new.wav | [MISSING] | A quest begins. |
| quest_done.wav | [MISSING] | A quest is completed. |
| rest.wav | [MISSING] | The Street Vendor's meal restores the party. |
| encounter.wav | [MISSING] | A fight begins (the swoosh into battle). |

**Battle: general**

| File | Status | When |
|------|--------|------|
| turn_ready.wav | [MISSING] | A party member's turn gauge fills. |
| guard.wav | [MISSING] | Someone defends. |
| heal.wav | [MISSING] | HP restored: Stim Pack, Trauma Kit, Patch, regen. |
| revive.wav | [MISSING] | Defib Patch brings someone back. |
| buff.wav | [MISSING] | A helpful effect lands: Guard Protocol, Adrenal Surge, Smoke Screen, Servo Overdrive. |
| debuff.wav | [MISSING] | A harmful effect lands: Corrupt, Lockout. |
| poison.wav | [MISSING] | Poison ticks (Neurotoxin). Short and quiet; it repeats. |
| stun.wav | [MISSING] | Someone is stunned (System Shock, Taser Burst). |
| enemy_down.wav | [MISSING] | An enemy is defeated and fades out. |
| ally_down.wav | [MISSING] | A party member falls. |
| flee.wav | [MISSING] | The party escapes. |
| level_up.wav | [MISSING] | Level up on the victory card. Short jingle. |

**Battle: attacks**

| File | Status | When |
|------|--------|------|
| hit_blunt.wav | [MISSING] | Heavy physical hit: Street Thug's pipe, Riot Slam, Combat Cyborg. |
| hit_blade.wav | [MISSING] | Blades: the Infiltrator and Vash (Backstab, Shiv, Killswitch). |
| gunshot.wav | [MISSING] | Guns: the Solo (attack, Suppressing Fire, Executioner). |
| hack.wav | [MISSING] | Digital attacks: the Netrunner and Cyph (Ice Breaker), Black ICE (ICE Spike). |
| zap.wav | [MISSING] | Electric and energy: Security Drone, System Shock, Taser Burst. |

One sound per kind of attack rather than one per ability keeps the list short. If
there is time for more, the first to split out are each class's strongest ability
(Executioner, Corrupt, Killswitch) and the Corporate Enforcer's attacks.

## 10. Fonts (`assets/fonts/`)

| Asset | Status | Notes |
|-------|--------|-------|
| default_font.png | [IN GAME] | Existing bitmap font atlas; generated font data is embedded |
| default_font_data.dat | [COVERED] | Metrics are embedded in BitmapFontData.h |

### Additional fonts for full game
| Asset | Status | Notes |
|-------|--------|-------|
| font_cyber_large.png | [DEFERRED] | Large cyberpunk display font — titles |
| font_cyber_small.png | [DEFERRED] | Small readable font — body text |
| font_monospace.png | [DEFERRED] | Monospace font — HUD numbers, code snippets |

---

## 11. Particle Effects / VFX (`assets/vfx/`)

| Asset | Status | Notes |
|-------|--------|-------|
| vfx_fireball.png | [DEFERRED] | Fireball projectile sprite |
| vfx_ice_shard.png | [DEFERRED] | Ice projectile sprite |
| vfx_lightning.png | [DEFERRED] | Lightning bolt sprite |
| vfx_heal_glow.png | [DEFERRED] | Healing glow particle |
| vfx_hit_spark.png | [DEFERRED] | Impact spark particle |
| vfx_explosion.png | [DEFERRED] | Explosion particle |
| vfx_levelup.png | [DEFERRED] | Level up burst effect |
| vfx_hack_stream.png | [DEFERRED] | Data stream visual for hacking |
| vfx_poison.png | [DEFERRED] | Poison cloud particle |
| vfx_dust.png | [DEFERRED] | Ambient dust particles |
| vfx_rain.png | [DEFERRED] | Rain particle spritesheet |
| vfx_neon_glow.png | [DEFERRED] | Neon glow overlay |

---

## 12. Cutscene / Background Art (`assets/cutscenes/`)

| Asset | Status | Notes |
|-------|--------|-------|
| intro_1.png | [DEFERRED] | Opening cutscene frame 1 |
| intro_2.png | [DEFERRED] | Opening cutscene frame 2 |
| intro_3.png | [DEFERRED] | Opening cutscene frame 3 |
| ending_good.png | [DEFERRED] | Good ending |
| ending_bad.png | [DEFERRED] | Bad ending |
| transition_fade.png | [COVERED] | Can be drawn as a full-screen colored quad |

---

## Summary and next art batches

| Group | Current state |
| --- | --- |
| Party | 5 base sprites + 10 animation sheets integrated |
| Enemies | 5 base sprites + 5 battle sheets integrated |
| NPCs | 7 base sprites + 7 idle/talk sheets integrated |
| Font | 1 existing runtime atlas |
| Runtime image validation | 40 files pass; zero failures |
| Tiles, menus, overlays, battle skyline | Existing procedural/code-drawn visuals |
| Item/story and equipment icons | 9 raw originals generated; conformance and wiring remain |
| Party ability icons | 12 planned sources using current ability IDs |
| Optional art | District backdrops, status icons, VFX, extra cast and vehicles retained above |
| Audio | Separate phase; backend/format work remains |

The old "195 assets remaining" total mixed slice requirements with speculative
full-game ideas and counted images already replaced by code. It is retired.
Generate a coherent batch at a time, preserve sources and prompts under `Raw/`,
and update each row to RAW when saved, then IN GAME after conformance and wiring.
No additional art here changes the completed Phase 11 status or advances the story
beyond the approved three-quest slice.
