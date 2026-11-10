# Cyber Rayne — Master Asset List

This document tracks **which** assets are needed and their status.
**ASSET_SPEC.md** defines **what** each one must be — dimensions, format, and the
tooling that produces and validates them. Read the spec before authoring anything.

**No hand-authored asset in `assets/` is final.** Every image file present today is
temporary — programmer art or leftover fantasy-era asset — and is slated for
replacement. A file existing on disk is not progress toward shipping.

The exception is **tiles**, which are now generated in code and are done (section 1).

### Raw sprite pass — 2026-11-10

The user supplied a 17-sprite slice brief. All 17 source images and their generation
prompts are now in [`CyberRayne/Raw/`](CyberRayne/Raw/MANIFEST.md), using the exact
requested filenames. Rayne's Solo carries a heavy pistol, Netrunner has a forearm
cyberdeck, and Infiltrator carries twin blades. Companions are `cyph.png` and
`vash.png`; the crystalline enemy is `black_ice.png`; Corporate Enforcer is the
boss. All seven map NPCs are included.

**Update, same day:** all 17 base sprites are conformed (96x128, original colours,
see ASSET_SPEC.md) and in the game under `assets/characters`, `assets/enemies` and
`assets/npcs`. Walk and battle animation sheets are still being generated in
`CyberRayne/Raw/sheets/` and are not in the game yet.

The paragraph below describes the raw sources.
These are raw design drafts, not FINAL runtime assets. The linked manifest takes
precedence over older naming/subject proposals below for this sprite batch.
Runtime entries remain TEMP/MISSING until the sources are conformed and integrated.

| Status | Meaning |
|--------|---------|
| **[TEMP]** | A file exists in `assets/`, but it is placeholder and will be replaced. |
| **[MISSING]** | No file at all. Code either falls back or renders nothing. |
| **[CUT]** | Deliberately removed from scope — do not author. |
| **[DEFERRED]** | Real scope, but not a blocker. Revisit when a map or system needs it. |
| **[FINAL]** | Shippable art, locked. |

Only **[FINAL]** and procedural sections count as done.

### Before committing any asset

```bash
python3 tools/spriteprep.py <source> --category <dir>   # conform to spec
python3 tools/validate_assets.py                        # must exit 0
```

The validator exists because the first art pass shipped 2048x2048 JPEGs renamed to
`.png` with no alpha channel, which the renderer drew as opaque rectangles.

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

## 2. Player Character Sprites (`assets/characters/`)

> **Naming is code-derived, not free-form.** `CharacterSelectionSystem.cpp:53-56` builds
> the path as `assets/characters/<classToString() lowercased>.png`. The classes are
> `NETRUNNER`, `SOLO`, `INFILTRATOR` (`Player.h:21-24`). Any filename not matching that
> lowercased string will not load. The old `warrior` / `mage` / `rogue` names are dead.

| Asset | Status | Notes |
|-------|--------|-------|
| netrunner.png | [TEMP] | Tech caster class — loaded by class selection |
| solo.png | [TEMP] | Physical frontline class |
| infiltrator.png | [TEMP] | Speed/attrition class |

### Needed for full character system
Rayne's sprite is whichever class the player picked, so there are no `rayne_*` class
variants — the three files above cover it. Directional and battle art is per-class.

| Asset | Status | Notes |
|-------|--------|-------|
| netrunner_side.png | [MISSING] | Side view for horizontal movement |
| netrunner_back.png | [MISSING] | Back view for upward movement |
| netrunner_walk.png | [MISSING] | Walk spritesheet (4-8 frames) |
| solo_side.png | [MISSING] | |
| solo_back.png | [MISSING] | |
| solo_walk.png | [MISSING] | |
| infiltrator_side.png | [MISSING] | |
| infiltrator_back.png | [MISSING] | |
| infiltrator_walk.png | [MISSING] | |
| netrunner_battle.png | [MISSING] | Battle stance |
| solo_battle.png | [MISSING] | Battle stance |
| infiltrator_battle.png | [MISSING] | Battle stance |
| ~~rayne_hurt.png~~ | [CUT] | Damage reaction = white-flash tint uniform, not an asset |
| ~~rayne_down.png~~ | [CUT] | Defeat = alpha dissolve / squash in shader, not an asset |

**Open question:** companions Cyph and Vash were previously distinct sprites
(`cyph_mage.png`, `vash_rogue.png`). Under class-derived naming they collide with the
player's class sprite. Either give companions their own `assets/characters/npc_cyph.png`
namespace, or accept that a party of three shows repeated art.

---

## 3. Enemy Sprites (`assets/enemies/`)

| Asset | Status | Notes |
|-------|--------|-------|
| street_thug.png | [MISSING] | Basic enemy — Neo-Kyoto Central, Neon Alley |
| security_drone.png | [MISSING] | Flying drone enemy — all districts |
| enemy_netrunner.png | [MISSING] | Hacker enemy — Neon Alley. **Renamed:** plain `netrunner.png` collides with the player class sprite. Keep enemies namespaced. |
| combat_cyborg.png | [MISSING] | Heavy enemy — Neon Alley, Corporate Plaza |
| corporate_enforcer.png | [MISSING] | Elite enemy — Corporate Plaza |
| goblin.png | [CUT] | Fantasy-era leftover, deleted 2026-11-10 |

### Additional enemies for full game
| Asset | Status | Notes |
|-------|--------|-------|
| street_thug_battle.png | [MISSING] | Battle stance |
| security_drone_battle.png | [MISSING] | Battle stance |
| netrunner_battle.png | [MISSING] | Battle stance |
| combat_cyborg_battle.png | [MISSING] | Battle stance |
| corporate_enforcer_battle.png | [MISSING] | Battle stance |
| corporate_soldier.png | [MISSING] | Mid-tier corporate guard |
| riot_bot.png | [MISSING] | Riot control robot |
| spider_bot.png | [MISSING] | Small crawling bot |
| turret.png | [MISSING] | Stationary turret |
| boss_data_king.png | [MISSING] | Act 1 boss — data broker kingpin |
| boss_corp_ceo.png | [MISSING] | Act 2 boss — corporate CEO |
| boss_ai_core.png | [MISSING] | Act 3 boss — rogue AI |
| ~~enemy_hurt.png~~ | [CUT] | Damage flash = tint uniform, not an asset |
| ~~enemy_defeated.png~~ | [CUT] | Defeat = shader dissolve, not an asset |

---

## 4. NPC Sprites (`assets/npcs/`)

| Asset | Status | Notes |
|-------|--------|-------|
| fixer_kaga.png | [MISSING] | Neo-Kyoto Central — quest giver |
| street_vendor.png | [MISSING] | Neo-Kyoto Central — shop |
| info_broker.png | [MISSING] | Neo-Kyoto Central — quest giver |
| smuggler_ryo.png | [MISSING] | Neon Alley — quest giver |
| hacker_zero.png | [MISSING] | Neon Alley — quest giver |
| exec_tanaka.png | [MISSING] | Corporate Plaza — quest giver |
| security_chief.png | [MISSING] | Corporate Plaza — guard NPC |

### Additional NPCs for full game
| Asset | Status | Notes |
|-------|--------|-------|
| bartender.png | [MISSING] | Bar/social hub |
| medic.png | [MISSING] | Healing station |
| black_market_dealer.png | [MISSING] | Underground shop |
| street_kid.png | [MISSING] | Ambient NPC — flavor text |
| corporate_worker.png | [MISSING] | Ambient NPC — Corporate Plaza |
| homeless_npc.png | [MISSING] | Ambient NPC — Neon Alley |
| traffic_drone.png | [MISSING] | Ambient NPC — flying traffic |
| shopkeeper_general.png | [MISSING] | General store |
| weapon_dealer.png | [MISSING] | Weapon shop |
| armor_dealer.png | [MISSING] | Armor shop |
| item_dealer.png | [MISSING] | Consumable shop |
| netrunner_vendor.png | [MISSING] | Sells hacking programs |
| taxi_driver.png | [MISSING] | Fast travel |
| subway_attendant.png | [MISSING] | Fast travel |
| gang_leader.png | [MISSING] | Faction quest giver |
| corporate_executive.png | [MISSING] | Corporate Plaza quest giver |
| police_officer.png | [MISSING] | Law enforcement NPC |
| prostitute.png | [MISSING] | Neon Alley ambient |
| drug_dealer.png | [MISSING] | Neon Alley ambient |

---

## 5. Vehicles (`assets/vehicles/`)

| Asset | Status | Notes |
|-------|--------|-------|
| hover_car.png | [MISSING] | Flying car — ambient traffic |
| hover_car_2.png | [MISSING] | Variant |
| hover_car_3.png | [MISSING] | Variant |
| police_hover.png | [MISSING] | Police flying vehicle |
| ambulance_hover.png | [MISSING] | Emergency vehicle |
| delivery_drone.png | [MISSING] | Package delivery drone |
| subway_train.png | [MISSING] | Subway train — background |
| motorcycle.png | [MISSING] | Street motorcycle |
| motorcycle_electric.png | [MISSING] | Electric bike variant |
| van.png | [MISSING] | Delivery van |
| armored_truck.png | [MISSING] | Corporate transport |
| taxi_hover.png | [MISSING] | Flying taxi — fast travel |
| garbage_truck.png | [MISSING] | Ambient |
| fire_drone.png | [MISSING] | Emergency response drone |
| news_drone.png | [MISSING] | Media drone — ambient |
| construction_mech.png | [MISSING] | Construction mech — ambient |
| billboard_car.png | [MISSING] | Mobile ad vehicle |

---

## 6. UI Assets (`assets/ui/`)

| Asset | Status | Notes |
|-------|--------|-------|
| menu_background.png | [TEMP] | Main menu background |
| start.png | [CUT] | Start button. Menu buttons are text `UIButton` widgets now; file deleted 2026-11-10 |
| loadgame.png | [CUT] | Load game button. Menu buttons are text `UIButton` widgets now; file deleted 2026-11-10 |
| settings.png | [CUT] | Settings button. Menu buttons are text `UIButton` widgets now; file deleted 2026-11-10 |
| quit.png | [CUT] | Quit button. Menu buttons are text `UIButton` widgets now; file deleted 2026-11-10 |
| cursor.png | [TEMP] | Menu cursor |
| namebanner.png | [TEMP] | Game title banner |

### Additional UI assets needed
| Asset | Status | Notes |
|-------|--------|-------|
| battle_background.png | [MISSING] | Battle scene background |
| battle_background_alley.png | [MISSING] | Neon Alley battle bg |
| battle_background_plaza.png | [MISSING] | Corporate Plaza battle bg |
| battle_background_central.png | [MISSING] | Neo-Kyoto Central battle bg |
| dialogue_box.png | [MISSING] | Dialogue text box background |
| pause_overlay.png | [MISSING] | Pause screen overlay |
| game_over.png | [MISSING] | Game over screen |
| victory.png | [MISSING] | Victory screen |
| bar_fill.png | [MISSING] | **One** 1×N gradient strip, tinted per bar type |
| ~~hp_bar.png~~ | [CUT] | Use `bar_fill.png` tinted |
| ~~mp_bar.png~~ | [CUT] | Use `bar_fill.png` tinted |
| ~~atb_bar.png~~ | [CUT] | Use `bar_fill.png` tinted |
| ~~xp_bar.png~~ | [CUT] | Use `bar_fill.png` tinted |
| status_poison.png | [MISSING] | Poison status icon |
| status_regen.png | [MISSING] | Regen status icon |
| status_stun.png | [MISSING] | Stun status icon |
| status_buff.png | [MISSING] | Attack buff icon |
| status_debuff.png | [MISSING] | Defense debuff icon |
| minimap.png | [MISSING] | Minimap frame |
| ~~minimap_player.png~~ | [CUT] | Colored quad, push-constant color |
| ~~minimap_npc.png~~ | [CUT] | Colored quad |
| ~~minimap_enemy.png~~ | [CUT] | Colored quad |
| ~~minimap_transition.png~~ | [CUT] | Colored quad |
| inventory_bg.png | [MISSING] | Inventory screen background |
| quest_log_bg.png | [MISSING] | Quest log screen background |
| shop_bg.png | [MISSING] | Shop screen background |
| tooltip_bg.png | [MISSING] | Tooltip background |
| button_small.png | [MISSING] | Small button template |
| button_large.png | [MISSING] | Large button template |
| panel_bg.png | [MISSING] | Panel background template |
| scrollbar.png | [MISSING] | Scrollbar for lists |
| character_portrait_frame.png | [MISSING] | Frame for character portraits |
| enemy_info_card.png | [MISSING] | Enemy info display in battle |
| weather_rain.png | [MISSING] | Rain particle overlay |
| ~~weather_fog.png~~ | [CUT] | Fullscreen shader, not an asset |
| ~~vignette.png~~ | [CUT] | Fullscreen shader math, not an asset |

---

## 7. Item & Spell Icons (`assets/items/`, `assets/spells/`)

### Items
| Asset | Status | Notes |
|-------|--------|-------|
| health_potion.png | [MISSING] | Healing item icon |
| mana_potion.png | [MISSING] | Mana restoration icon |
| ether.png | [MISSING] | Full mana restore |
| phoenix_down.png | [MISSING] | Revive fallen ally |
| antidote.png | [MISSING] | Cure poison |
| stim_pack.png | [MISSING] | Temporary buff |
| data_chip.png | [MISSING] | Quest item — "The Data Chip" |
| credstick.png | [MISSING] | Currency item |
| medkit.png | [MISSING] | Large heal |
| grenade.png | [MISSING] | Battle item — AoE damage |
| emp_grenade.png | [MISSING] | Battle item — stun machines |
| smoke_bomb.png | [MISSING] | Battle item — flee boost |

### Spells / Abilities
| Asset | Status | Notes |
|-------|--------|-------|
| spell_fireball.png | [MISSING] | Netrunner offensive program |
| spell_ice.png | [MISSING] | Netrunner offensive program |
| spell_lightning.png | [MISSING] | Netrunner offensive program |
| spell_heal.png | [MISSING] | Netrunner healing program |
| spell_shield.png | [MISSING] | Netrunner defensive program |
| spell_hack.png | [MISSING] | Netrunner ability — debuff enemy |
| spell_overload.png | [MISSING] | Netrunner ability — AoE damage |
| spell_cloak.png | [MISSING] | Infiltrator ability — evasion buff |
| spell_backstab.png | [MISSING] | Infiltrator ability — high damage |
| spell_berserk.png | [MISSING] | Solo ability — attack buff |
| spell_taunt.png | [MISSING] | Solo ability — draw aggro |
| spell_charge.png | [MISSING] | Solo ability — rush attack |

---

## 8. Equipment Icons (`assets/equipment/`)

### Weapons
| Asset | Status | Notes |
|-------|--------|-------|
| weapon_katana.png | [MISSING] | Solo weapon |
| weapon_monoblade.png | [MISSING] | Solo weapon upgrade |
| weapon_pistol.png | [MISSING] | Infiltrator weapon |
| weapon_smg.png | [MISSING] | Infiltrator weapon upgrade |
| weapon_shock_wand.png | [MISSING] | Netrunner weapon |
| weapon_data_staff.png | [MISSING] | Netrunner weapon upgrade |
| weapon_vibroblade.png | [MISSING] | Endgame weapon |
| weapon_plasma_rifle.png | [MISSING] | Endgame weapon |

### Armor
| Asset | Status | Notes |
|-------|--------|-------|
| armor_streetwear.png | [MISSING] | Starting armor |
| armor_kevlar.png | [MISSING] | Mid-tier armor |
| armor_tactical.png | [MISSING] | High-tier armor |
| armor_corporate_suit.png | [MISSING] | Corporate Plaza disguise |
| armor_netrunner_suit.png | [MISSING] | Netrunner armor |
| armor_stealth_cloak.png | [MISSING] | Infiltrator armor |
| armor_heavy_exo.png | [MISSING] | Endgame armor |

### Accessories
| Asset | Status | Notes |
|-------|--------|-------|
| acc_neural_implant.png | [MISSING] | Boosts magic |
| acc_reflex_chip.png | [MISSING] | Boosts speed |
| acc_armor_plating.png | [MISSING] | Boosts defense |
| acc_targeting_scope.png | [MISSING] | Boosts strength |
| acc_holo_shield.png | [MISSING] | Status resist |

---

## 9. Audio (`assets/audio/`)

> **Code/list mismatch.** `AudioManager.cpp` loads `assets/audio/bgm_main.ogg` and
> `assets/audio/sfx_hit.wav`. This list says `bgm_menu.ogg` and `sfx_hit.ogg` — different
> stem *and* different container. Pick one convention (`.ogg` throughout is the safe call)
> and reconcile before authoring, or the files will land unloadable.

### Music
| Asset | Status | Notes |
|-------|--------|-------|
| bgm_menu.ogg | [MISSING] | Main menu theme — code currently asks for `bgm_main.ogg` |
| bgm_central.ogg | [MISSING] | Neo-Kyoto Central exploration |
| bgm_alley.ogg | [MISSING] | Neon Alley exploration |
| bgm_plaza.ogg | [MISSING] | Corporate Plaza exploration |
| bgm_battle.ogg | [MISSING] | Standard battle theme |
| bgm_boss.ogg | [MISSING] | Boss battle theme |
| bgm_game_over.ogg | [MISSING] | Game over theme |
| bgm_victory.ogg | [MISSING] | Victory fanfare |
| bgm_boss_final.ogg | [MISSING] | Final boss theme |

### SFX
| Asset | Status | Notes |
|-------|--------|-------|
| sfx_hit.ogg | [MISSING] | Physical hit — code currently asks for `sfx_hit.wav` |
| sfx_shoot.ogg | [MISSING] | Gunfire |
| sfx_explosion.ogg | [MISSING] | Explosion |
| sfx_heal.ogg | [MISSING] | Healing sound |
| sfx_levelup.ogg | [MISSING] | Level up jingle |
| sfx_menu_move.ogg | [MISSING] | Menu navigation |
| sfx_menu_select.ogg | [MISSING] | Menu confirm |
| sfx_menu_cancel.ogg | [MISSING] | Menu cancel |
| sfx_pause.ogg | [MISSING] | Pause toggle |
| sfx_encounter.ogg | [MISSING] | Battle encounter trigger |
| sfx_spell_fire.ogg | [MISSING] | Fire spell |
| sfx_spell_ice.ogg | [MISSING] | Ice spell |
| sfx_spell_lightning.ogg | [MISSING] | Lightning spell |
| sfx_spell_hack.ogg | [MISSING] | Hack ability |
| sfx_footstep.ogg | [MISSING] | Walking footstep |
| sfx_door.ogg | [MISSING] | Door/transition |
| sfx_save.ogg | [MISSING] | Save game |
| sfx_load.ogg | [MISSING] | Load game |
| sfx_death.ogg | [MISSING] | Character death |
| sfx_victory.ogg | [MISSING] | Battle victory |

---

## 10. Fonts (`assets/fonts/`)

| Asset | Status | Notes |
|-------|--------|-------|
| default_font.png | [TEMP] | 8x8 bitmap font atlas — replace with generated MSDF atlas |
| default_font_data.dat | [MISSING] | Font metrics data (currently embedded) |

### Additional fonts for full game
| Asset | Status | Notes |
|-------|--------|-------|
| font_cyber_large.png | [MISSING] | Large cyberpunk display font — titles |
| font_cyber_small.png | [MISSING] | Small readable font — body text |
| font_monospace.png | [MISSING] | Monospace font — HUD numbers, code snippets |

---

## 11. Particle Effects / VFX (`assets/vfx/`)

| Asset | Status | Notes |
|-------|--------|-------|
| vfx_fireball.png | [MISSING] | Fireball projectile sprite |
| vfx_ice_shard.png | [MISSING] | Ice projectile sprite |
| vfx_lightning.png | [MISSING] | Lightning bolt sprite |
| vfx_heal_glow.png | [MISSING] | Healing glow particle |
| vfx_hit_spark.png | [MISSING] | Impact spark particle |
| vfx_explosion.png | [MISSING] | Explosion particle |
| vfx_levelup.png | [MISSING] | Level up burst effect |
| vfx_hack_stream.png | [MISSING] | Data stream visual for hacking |
| vfx_poison.png | [MISSING] | Poison cloud particle |
| vfx_dust.png | [MISSING] | Ambient dust particles |
| vfx_rain.png | [MISSING] | Rain particle spritesheet |
| vfx_neon_glow.png | [MISSING] | Neon glow overlay |

---

## 12. Cutscene / Background Art (`assets/cutscenes/`)

| Asset | Status | Notes |
|-------|--------|-------|
| intro_1.png | [MISSING] | Opening cutscene frame 1 |
| intro_2.png | [MISSING] | Opening cutscene frame 2 |
| intro_3.png | [MISSING] | Opening cutscene frame 3 |
| ending_good.png | [MISSING] | Good ending |
| ending_bad.png | [MISSING] | Bad ending |
| transition_fade.png | [MISSING] | Scene transition overlay |

---

## Summary

**[FINAL] is zero for every hand-authored category.** Tiles are the one solved
section, and they were solved by writing code instead of commissioning art.

| Category | Status | To author |
|----------|--------|-----------|
| Tile Textures | **DONE — procedural** (17 more deferred) | **0** |
| Player Sprites | 3 TEMP, 12 MISSING, 2 CUT | 15 |
| Enemy Sprites | 1 TEMP, 15 MISSING, 3 CUT | 16 |
| NPC Sprites | 19 MISSING | 19 |
| Vehicles | 17 MISSING | 17 |
| UI Assets | 7 TEMP, 26 MISSING, 7 CUT | 33 |
| Items & Spells | 24 MISSING | 24 |
| Equipment | 20 MISSING | 20 |
| Audio | 29 MISSING | 29 |
| Fonts | 1 TEMP, 3 MISSING | 4 |
| VFX | 12 MISSING | 12 |
| Cutscenes | 6 MISSING | 6 |
| **Total** | | **195** |

Down from 220. The 25 tile rows are gone entirely — not deferred, not cut, just
no longer assets. 19 rows remain [CUT] as shader work (ASSET_SPEC.md section 8).

### Priority for Vertical Slice

Roughly 25 assets stand between here and a coherent-looking slice:

1. ~~Tile textures~~ — **done, procedural**
2. **Player sprites** (3) — `netrunner.png`, `solo.png`, `infiltrator.png`.
   Files exist in `assets/characters/` but are 2048x2048 with no alpha; they need
   a `spriteprep.py` pass to reach the 96x128 RGBA spec.
3. **Enemy sprites** (5) — one per cyberpunk enemy type
4. **NPC sprites** (7) — one per defined NPC
5. **Battle backgrounds** (3) — one per district
6. **UI** — dialogue box, pause overlay, game over. (HP/MP/ATB bars are [CUT]:
   tinted quads, not textures.)
7. **Audio** — menu, exploration, battle music + basic SFX. Settle the
   `.ogg`/`.wav` convention first; see section 9.

Everything authored must pass `python3 tools/validate_assets.py`.
Format rules and the production pipeline are in **ASSET_SPEC.md**.
