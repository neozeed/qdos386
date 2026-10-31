# RC6 assembly coverage — R1

Coverage means that a native definition exists. Passing tests are recorded separately.
Data symbols and internal helpers are counted too; this is not a feature-completion percentage.

| Original source | Exports present | Remaining exported symbols |
|---|---:|---|
| bulbs.s | 0/1 | `_dashboard_bulbs` |
| cargo.s | 5/5 | — |
| combat.s | 1/9 | `_laser_fire`, `_laser_hit`, `_laser_tick`, `_laser_xor`, `_player_damage`, `_ship_angry`, `_ship_damage`, `_sight_xor` |
| commander.s | 4/4 | — |
| compass.s | 0/3 | `_compass_frame`, `_compass_project`, `_compass_xor` |
| controls.s | 1/1 | — |
| dashboard.s | 0/1 | `_dashboard_pixels` |
| development.s | 0/1 | `_release_mode` |
| dials.s | 0/5 | `_dials_altitude`, `_dials_frame`, `_dials_fuel`, `_dials_last_status`, `_energy_step` |
| docking.s | 1/2 | `_dock_check` |
| dust.s | 0/2 | `_dust_run`, `dust_plot` |
| dust_generated.s | 0/4 | `dust_original_flip`, `dust_original_init`, `dust_original_redraw`, `dust_original_step` |
| encounters.s | 0/2 | `_encounter_pool`, `_encounter_step` |
| equipment.s | 4/4 | — |
| explosion.s | 0/5 | `_cloud_advance`, `_cloud_points`, `_cloud_xor`, `_explosion_frame`, `_explosion_start` |
| flight.s | 0/3 | `_motion_position`, `_move_ship`, `_rotate_body` |
| keyboard.s | 0/1 | `_keyboard_drain` |
| line.s | 0/4 | `_hash_screen`, `_xor_line`, `clip_segment`, `xor_segment` |
| market.s | 0/4 | `_launch_legal`, `_market_buy`, `_market_init`, `_market_trade` |
| missile.s | 0/3 | `_ecm_step`, `_hostile_missile`, `_spawn_child` |
| models.s | 0/2 | `_models`, `_orientations` |
| models_game.s | 0/1 | `_models` |
| motion.s | 3/3 | — |
| navigation.s | 0/2 | `_missile_icons`, `_planet_vector` |
| planet.s | 0/1 | `_move_planet` |
| planet_view.s | 0/5 | `_circle_ring`, `_circle_xy`, `_planet_altitude`, `_planet_frame`, `_planet_geometry` |
| presentation.s | 0/2 | `_local_position`, `_system_details` |
| release.s | 0/1 | `_release_mode` |
| render.s | 0/9 | `_effect_vertices`, `_effect_visible`, `_line_total`, `_live_matrix`, `_projected_total`, `_render_cache`, `_render_exploding`, `_render_inwk`, `_render_ship` |
| render_game.s | 0/9 | `_effect_vertices`, `_effect_visible`, `_line_total`, `_live_matrix`, `_projected_total`, `_render_cache`, `_render_exploding`, `_render_inwk`, `_render_ship` |
| save.s | 0/2 | `_commander_filename`, `_commander_io` |
| scanner.s | 0/5 | `_scanner_flags`, `_scanner_frame`, `_scanner_project`, `_scanner_reset`, `_scanner_xor` |
| screen_store.s | 0/2 | `_screen_pack`, `_screen_unpack` |
| shell.s | 0/4 | `__Cstart`, `__prog_name`, `__stack`, `_exit` |
| shop.s | 4/4 | — |
| sound.s | 0/5 | `_sound_advance`, `_sound_event`, `_sound_poll`, `_sound_stop`, `_sound_tick` |
| station.s | 0/4 | `_station_candidate`, `_station_launch`, `_warp_planet`, `_warp_world` |
| station_ai.s | 1/1 | — |
| tactics.s | 0/7 | `_missile_steer`, `_pod_step`, `_tactics_cnt`, `_tactics_dot`, `_tactics_due`, `_tactics_step`, `_tactics_vector` |
| target.s | 0/3 | `_friendly_missile`, `_player_missile_init`, `_relative_vector` |
| tidy.s | 0/3 | `_tidy_ship`, `normalize_vector`, `tidy_mult` |
| travel.s | 10/10 | — |
| universe.s | 0/4 | `_chart_cursor_xor`, `_galaxy_chart`, `_galaxy_chart_cursor`, `_solar_planet` |
| universe_list.s | 5/5 | — |
| views.s | 1/1 | — |

Files with no exports may contain required local code or tables; 0/0 does not mean ported.
The two render/model variants and release/development variants remain separate upstream choices.
The shell and hardware-facing routines have not been stubbed.

## Native definitions

- `_bomb_key`
- `_bomb_ship`
- `_bomb_tick`
- `_cargo_room`
- `_cargo_tumble`
- `_chart_move`
- `_chart_nearest`
- `_commander_rank`
- `_commander_settle`
- `_commander_tally`
- `_commander_tick`
- `_computer_dock`
- `_control_step`
- `_debris_count`
- `_elite_random`
- `_equipment_damage`
- `_escape_commander`
- `_galaxy_distance`
- `_galaxy_system`
- `_laser_fitting`
- `_motion_matrix`
- `_own_rotate`
- `_scoop_step`
- `_ship_fit`
- `_shop_buy`
- `_shop_buy_view`
- `_station_police`
- `_travel_arrive`
- `_travel_init`
- `_travel_refuel`
- `_travel_select`
- `_travel_start`
- `_travel_tick`
- `_universe_add`
- `_universe_cost`
- `_universe_missile_target`
- `_universe_remove`
- `_universe_reset`
- `_view_transform`
- `sign_add`
