#ifndef ELITE386_H
#define ELITE386_H
/* Native i386 C89 declarations. Buffer layouts are unchanged RC6 state arrays. */
void control_step(unsigned char *, const unsigned char *);
void commander_tally(unsigned char *);
void commander_tick(unsigned char *);
unsigned commander_settle(unsigned char *,unsigned,unsigned);
unsigned commander_rank(const unsigned char *);
void universe_reset(void *);
void universe_remove(void *,unsigned);
int universe_add(void *,unsigned,unsigned);
unsigned universe_cost(unsigned);
unsigned universe_missile_target(unsigned,unsigned);
void view_transform(unsigned char *,const unsigned char *,unsigned);
unsigned station_police(unsigned char *,const unsigned char *,unsigned char *,unsigned);
unsigned elite_random(unsigned char *,unsigned);
unsigned equipment_damage(unsigned char *,unsigned char *,unsigned,unsigned);
void bomb_key(unsigned char *,unsigned);
void bomb_tick(unsigned char *);
unsigned bomb_ship(unsigned char *,unsigned,unsigned);
unsigned cargo_room(const unsigned char *,unsigned,unsigned,unsigned);
unsigned scoop_step(unsigned char *,unsigned,unsigned,unsigned char *,unsigned char *,unsigned);
unsigned debris_count(unsigned,unsigned,unsigned char *);
void cargo_tumble(unsigned char *,unsigned char *);
extern unsigned char ship_fit[8];
void own_rotate(unsigned char *);
void motion_matrix(unsigned char *,short *);
unsigned shop_buy(unsigned char *,unsigned char *,unsigned char *,unsigned char *,unsigned);
unsigned shop_buy_view(unsigned char *,unsigned char *,unsigned char *,unsigned char *,unsigned,unsigned);
unsigned laser_fitting(const unsigned char *,unsigned);
unsigned escape_commander(unsigned char *,unsigned char *,unsigned char *);
unsigned computer_dock(unsigned,unsigned,unsigned,unsigned);
void galaxy_system(unsigned char *,unsigned,unsigned char *);
unsigned galaxy_distance(unsigned char *,unsigned char *);
void travel_select(unsigned char *,unsigned);
void travel_init(unsigned char *);
unsigned travel_start(unsigned char *,unsigned,unsigned);
unsigned travel_tick(unsigned char *);
void travel_arrive(unsigned char *,unsigned char *,unsigned);
unsigned travel_refuel(unsigned char *,unsigned char *);
unsigned chart_move(unsigned,unsigned);
unsigned chart_nearest(const unsigned char *,unsigned,unsigned);
#endif
