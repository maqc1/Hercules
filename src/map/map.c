/**
 * This file is part of Hercules.
 * http://herc.ws - http://github.com/HerculesWS/Hercules
 *
 * Copyright (C) 2012-2025 Hercules Dev Team
 * Copyright (C) Athena Dev Teams
 *
 * Hercules is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#define HERCULES_CORE

#include "config/core.h" // CELL_NOSTACK, CIRCULAR_AREA, CONSOLE_INPUT, DBPATH, RENEWAL
#include "map.h"

#include "map/HPMmap.h"
#include "map/atcommand.h"
#include "map/battle.h"
#include "map/battleground.h"
#include "map/channel.h"
#include "map/chat.h"
#include "map/chrif.h"
#include "map/clan.h"
#include "map/clif.h"
#include "map/duel.h"
#include "map/elemental.h"
#include "map/enchantui.h"
#include "map/goldpc.h"
#include "map/grader.h"
#include "map/guild.h"
#include "map/homunculus.h"
#include "map/instance.h"
#include "map/intif.h"
#include "map/irc-bot.h"
#include "map/itemdb.h"
#include "map/log.h"
#include "map/macro.h"
#include "map/mapiif.h"
#include "map/mail.h"
#include "map/mapreg.h"
#include "map/mercenary.h"
#include "map/mob.h"
#include "map/npc.h" // npc_setcells(), npc_unsetcells()
#include "map/party.h"
#include "map/path.h"
#include "map/pc.h"
#include "map/pet.h"
#include "map/quest.h"
#include "map/script.h"
#include "map/skill.h"
#include "map/status.h"
#include "map/storage.h"
#include "map/stylist.h"
#include "map/rodex.h"
#include "map/refine.h"
#include "map/trade.h"
#include "map/unit.h"
#include "map/achievement.h"
#include "common/HPM.h"
#include "common/cbasetypes.h"
#include "common/conf.h"
#include "common/console.h"
#include "common/core.h"
#include "common/ers.h"
#include "common/extraconf.h"
#include "common/grfio.h"
#include "common/md5calc.h"
#include "common/memmgr.h"
#include "common/nullpo.h"
#include "common/random.h"
#include "common/showmsg.h"
#include "common/socket.h" // WFIFO*()
#include "common/sql.h"
#include "common/strlib.h"
#include "common/timer.h"
#include "common/utils.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifndef _WIN32
#include <unistd.h>
#endif

static struct map_interface map_s;
static struct mapit_interface mapit_s;

struct map_interface *map;
struct mapit_interface *mapit;

/*==========================================
 * server player count (of all mapservers)
 *------------------------------------------*/
static void map_setusers(int users)
{
	map->users = users;
}

static int map_getusers(void)
{
	return map->users;
}

/**
 * Expands map->bl_list on demand
 **/
static inline void map_bl_list_expand(void)
{
	map->bl_list_size += 250;
	RECREATE(map->bl_list, struct block_list *, map->bl_list_size);
}

/**
 * Expands map->block_free on demand
 **/
static inline void map_block_free_expand(void)
{
	map->block_free_list_size += 100;
	RECREATE(map->block_free, struct block_list *, map->block_free_list_size);
#ifdef SANITIZE
	RECREATE(map->block_free_sanitize, int *, map->block_free_list_size);
#endif
}

/*==========================================
 * server player count (this mapserver only)
 *------------------------------------------*/
static int map_usercount(void)
{
	return db_size(map->pc_db);
}

/*==========================================
 * Attempt to free a map blocklist
 *------------------------------------------*/
static int map_freeblock(struct block_list *bl)
{
	nullpo_retr(map->block_free_lock, bl);

	if (map->block_free_lock == 0) {
		if( bl->type == BL_ITEM )
			ers_free(map->flooritem_ers, bl);
		else
			aFree(bl);
		bl = NULL;
	} else {
		if (bl->deleted == true)
			return map->block_free_lock;
		if (map->block_free_count >= map->block_free_list_size)
			map_block_free_expand();

		map->block_free[map->block_free_count] = bl;
#ifdef SANITIZE
		map->block_free_sanitize[map->block_free_count] = aMalloc(4);
#endif
		bl->deleted = true;
		map->block_free_count++;
	}

	return map->block_free_lock;
}
/*==========================================
 * Lock blocklist, (prevent map->freeblock usage)
 *------------------------------------------*/
static int map_freeblock_lock(void)
{
	return ++map->block_free_lock;
}

/*==========================================
 * Remove the lock on map_bl
 *------------------------------------------*/
static int map_freeblock_unlock(void)
{
	if ((--map->block_free_lock) == 0) {
		int i;
		for (i = 0; i < map->block_free_count; i++) {
#ifdef SANITIZE
			aFree(map->block_free_sanitize[i]);
			map->block_free_sanitize[i] = NULL;
#endif
			if( map->block_free[i]->type == BL_ITEM )
				ers_free(map->flooritem_ers, map->block_free[i]);
			else
				aFree(map->block_free[i]);
			map->block_free[i] = NULL;
		}
		map->block_free_count = 0;
	} else if (map->block_free_lock < 0) {
		ShowError("map_freeblock_unlock: lock count < 0 !\n");
		map->block_free_lock = 0;
	}

	return map->block_free_lock;
}

// Timer function to check if there some remaining lock and remove them if so.
// Called each 1s
static int map_freeblock_timer(int tid, int64 tick, int id, intptr_t data)
{
	if (map->block_free_lock > 0) {
		ShowError("map_freeblock_timer: block_free_lock(%d) is invalid.\n", map->block_free_lock);
		map->block_free_lock = 1;
		map->freeblock_unlock();
	}

	return 0;
}

/**
 * Updates the counter (cell.cell_bl) of how many objects are on a tile.
 * @param add Whether the counter should be increased or decreased
 **/
static void map_update_cell_bl(struct block_list *bl, bool increase)
{
#ifdef CELL_NOSTACK
	int pos;

	nullpo_retv(bl);
	if( bl->m < 0 || bl->x < 0 || bl->x >= map->list[bl->m].xs
	              || bl->y < 0 || bl->y >= map->list[bl->m].ys
	              || !(bl->type&BL_CHAR) )
		return;

	// When reading from mapcache the cell isn't initialized
	// TODO: Maybe start initializing cells when they're loaded instead of
	// having to get them here? [Panikon]
	if( map->list[bl->m].cell == (struct mapcell *)0xdeadbeaf )
		map->cellfromcache(&map->list[bl->m]);

	pos = bl->x + bl->y*map->list[bl->m].xs;
	if( increase )
		map->list[bl->m].cell[pos].cell_bl++;
	else
		map->list[bl->m].cell[pos].cell_bl--;
#endif
	return;
}

/*==========================================
 * Adds a block to the map.
 * Returns 0 on success, 1 on failure (illegal coordinates).
 *------------------------------------------*/
static int map_addblock(struct block_list *bl)
{
	int16 m, x, y;
	int pos;

	nullpo_ret(bl);

	if (bl->prev != NULL) {
		ShowError("map_addblock: bl->prev != NULL\n");
		return 1;
	}

	m = bl->m;
	x = bl->x;
	y = bl->y;
	if( m < 0 || m >= map->count ) {
		ShowError("map_addblock: invalid map id (%d), only %d are loaded.\n", m, map->count);
		return 1;
	}
	if( x < 0 || x >= map->list[m].xs || y < 0 || y >= map->list[m].ys ) {
		ShowError("map_addblock: out-of-bounds coordinates (\"%s\",%d,%d), map is %dx%d\n", map->list[m].name, x, y, map->list[m].xs, map->list[m].ys);
		return 1;
	}

	pos = x/BLOCK_SIZE+(y/BLOCK_SIZE)*map->list[m].bxs;

	if (bl->type == BL_MOB) {
		Assert_ret(map->list[m].block_mob != NULL);
		bl->next = map->list[m].block_mob[pos];
		bl->prev = &map->bl_head;
		if (bl->next) bl->next->prev = bl;
		map->list[m].block_mob[pos] = bl;
	} else {
		Assert_ret(map->list[m].block != NULL);
		bl->next = map->list[m].block[pos];
		bl->prev = &map->bl_head;
		if (bl->next) bl->next->prev = bl;
		map->list[m].block[pos] = bl;
	}

#ifdef CELL_NOSTACK
	map->update_cell_bl(bl, true);
#endif

	return 0;
}

/*==========================================
 * Removes a block from the map.
 *------------------------------------------*/
static int map_delblock(struct block_list *bl)
{
	int pos;
	nullpo_ret(bl);

	// blocklist (2ways chainlist)
	if (bl->prev == NULL) {
		if (bl->next != NULL) {
			// can't delete block (already at the beginning of the chain)
			ShowError("map_delblock error : bl->next!=NULL\n");
		}
		return 0;
	}

#ifdef CELL_NOSTACK
	map->update_cell_bl(bl, false);
#endif

	pos = bl->x/BLOCK_SIZE+(bl->y/BLOCK_SIZE)*map->list[bl->m].bxs;

	if (bl->next)
		bl->next->prev = bl->prev;
	if (bl->prev == &map->bl_head) {
		//Since the head of the list, update the block_list map of []
		if (bl->type == BL_MOB) {
			Assert_ret(map->list[bl->m].block_mob != NULL);
			map->list[bl->m].block_mob[pos] = bl->next;
		} else {
			Assert_ret(map->list[bl->m].block != NULL);
			map->list[bl->m].block[pos] = bl->next;
		}
	} else {
		bl->prev->next = bl->next;
	}
	bl->next = NULL;
	bl->prev = NULL;

	return 0;
}

/*==========================================
 * Moves a block a x/y target position. [Skotlex]
 * Pass flag as 1 to prevent doing skill->unit_move checks
 * (which are executed by default on BL_CHAR types)
 *------------------------------------------*/
static int map_moveblock(struct block_list *bl, int x1, int y1, int64 tick)
{
	struct status_change *sc = NULL;
	int x0, y0;
	int moveblock;

	nullpo_ret(bl);
	x0 = bl->x;
	y0 = bl->y;
	moveblock = ( x0/BLOCK_SIZE != x1/BLOCK_SIZE || y0/BLOCK_SIZE != y1/BLOCK_SIZE);

	if (!bl->prev) {
		//Block not in map, just update coordinates, but do naught else.
		bl->x = x1;
		bl->y = y1;
		return 0;
	}

	//TODO: Perhaps some outs of bounds checking should be placed here?
	if (bl->type&BL_CHAR) {
		sc = status->get_sc(bl);

		skill->unit_move(bl,tick,2);
		status_change_end(bl, SC_RG_CCONFINE_M, INVALID_TIMER);
		status_change_end(bl, SC_RG_CCONFINE_S, INVALID_TIMER);
		//status_change_end(bl, SC_BLADESTOP, INVALID_TIMER); //Won't stop when you are knocked away, go figure...
		status_change_end(bl, SC_NJ_TATAMIGAESHI, INVALID_TIMER);
		status_change_end(bl, SC_MAGICROD, INVALID_TIMER);
		status_change_end(bl, SC_SU_STOOP, INVALID_TIMER);
		if (sc && sc->data[SC_PROPERTYWALK] &&
			sc->data[SC_PROPERTYWALK]->val3 >= skill->get_maxcount(sc->data[SC_PROPERTYWALK]->val1,sc->data[SC_PROPERTYWALK]->val2) )
			status_change_end(bl,SC_PROPERTYWALK,INVALID_TIMER);
	} else if (bl->type == BL_NPC) {
		npc->unsetcells(BL_UCAST(BL_NPC, bl));
	}

	if (moveblock) map->delblock(bl);
#ifdef CELL_NOSTACK
	else map->update_cell_bl(bl, false);
#endif
	bl->x = x1;
	bl->y = y1;
	if (moveblock) map->addblock(bl);
#ifdef CELL_NOSTACK
	else map->update_cell_bl(bl, true);
#endif

	if (bl->type&BL_CHAR) {
		struct map_session_data *sd = BL_CAST(BL_PC, bl);

		skill->unit_move(bl,tick,3);

		if (sd != NULL && sd->shadowform_id != 0) {
			//Shadow Form Target Moving
			struct block_list *d_bl;
			if ((d_bl = map->id2bl(sd->shadowform_id)) == NULL || !check_distance_bl(bl,d_bl,10)) {
				if( d_bl )
					status_change_end(d_bl,SC__SHADOWFORM,INVALID_TIMER);
				sd->shadowform_id = 0;
			}
		}

		if (sc && sc->count) {
			if (sc->data[SC_DANCING])
				skill->unit_move_unit_group(skill->id2group(sc->data[SC_DANCING]->val2), bl->m, x1-x0, y1-y0);
			else {
				if (sc->data[SC_CLOAKING])
					skill->check_cloaking(bl, sc->data[SC_CLOAKING]);
				if (sc->data[SC_WARM])
					skill->unit_move_unit_group(skill->id2group(sc->data[SC_WARM]->val4), bl->m, x1-x0, y1-y0);
				if (sc->data[SC_BANDING])
					skill->unit_move_unit_group(skill->id2group(sc->data[SC_BANDING]->val4), bl->m, x1-x0, y1-y0);

				if (sc->data[SC_NEUTRALBARRIER_MASTER])
					skill->unit_move_unit_group(skill->id2group(sc->data[SC_NEUTRALBARRIER_MASTER]->val2), bl->m, x1-x0, y1-y0);
				else if (sc->data[SC_STEALTHFIELD_MASTER])
					skill->unit_move_unit_group(skill->id2group(sc->data[SC_STEALTHFIELD_MASTER]->val2), bl->m, x1-x0, y1-y0);

				if( sc->data[SC__SHADOWFORM] ) {//Shadow Form Caster Moving
					struct block_list *d_bl;
					if( (d_bl = map->id2bl(sc->data[SC__SHADOWFORM]->val2)) == NULL || !check_distance_bl(bl,d_bl,10) )
						status_change_end(bl,SC__SHADOWFORM,INVALID_TIMER);
				}

				if (sc->data[SC_PROPERTYWALK]
				 && sc->data[SC_PROPERTYWALK]->val3 < skill->get_maxcount(sc->data[SC_PROPERTYWALK]->val1,sc->data[SC_PROPERTYWALK]->val2)
				 && map->find_skill_unit_oncell(bl,bl->x,bl->y,SO_ELECTRICWALK,NULL,0) == NULL
				 && map->find_skill_unit_oncell(bl,bl->x,bl->y,SO_FIREWALK,NULL,0) == NULL
				 && skill->unitsetting(bl,sc->data[SC_PROPERTYWALK]->val1,sc->data[SC_PROPERTYWALK]->val2,x0, y0,0)
				) {
					sc->data[SC_PROPERTYWALK]->val3++;
				}
			}
			/* Guild Aura Moving */
			if (sd != NULL && sd->state.gmaster_flag) {
				if (sc->data[SC_LEADERSHIP])
					skill->unit_move_unit_group(skill->id2group(sc->data[SC_LEADERSHIP]->val4), bl->m, x1-x0, y1-y0);
				if (sc->data[SC_GLORYWOUNDS])
					skill->unit_move_unit_group(skill->id2group(sc->data[SC_GLORYWOUNDS]->val4), bl->m, x1-x0, y1-y0);
				if (sc->data[SC_SOULCOLD])
					skill->unit_move_unit_group(skill->id2group(sc->data[SC_SOULCOLD]->val4), bl->m, x1-x0, y1-y0);
				if (sc->data[SC_HAWKEYES])
					skill->unit_move_unit_group(skill->id2group(sc->data[SC_HAWKEYES]->val4), bl->m, x1-x0, y1-y0);
			}
		}
	} else if (bl->type == BL_NPC) {
		npc->setcells(BL_UCAST(BL_NPC, bl));
	}

	return 0;
}

/*==========================================
 * Counts specified number of objects on given cell.
 * flag:
 *   0x1 - only count standing units
 *   0x2 - don't count invinsible units
 * TODO: merge with bl_getall_area
 *------------------------------------------*/
static int map_count_oncell(int16 m, int16 x, int16 y, int type, int flag)
{
	int bx,by;
	struct block_list *bl;
	int count = 0;

	Assert_ret(m >= -1);
	if (m < 0)
		return 0;
	Assert_ret(m < map->count);
	Assert_ret(map->list[m].block != NULL);

	if (x < 0 || y < 0 || (x >= map->list[m].xs) || (y >= map->list[m].ys))
		return 0;

	bx = x/BLOCK_SIZE;
	by = y/BLOCK_SIZE;

	if (type&~BL_MOB) {
		for (bl = map->list[m].block[bx+by*map->list[m].bxs]; bl != NULL; bl = bl->next) {
			if (bl->x == x && bl->y == y && bl->type&type) {
				if (flag&0x2) {
					struct status_change *sc = status->get_sc(bl);
					if (sc && (sc->option&OPTION_INVISIBLE))
						continue;
					if (bl->type == BL_NPC) {
						const struct npc_data *nd = BL_UCCAST(BL_NPC, bl);
						if (nd->class_ == FAKE_NPC || nd->class_ == HIDDEN_WARP_CLASS || nd->dyn.isdynamic)
							continue;
					}
				}
				if (flag&0x1) {
					struct unit_data *ud = unit->bl2ud(bl);
					if (ud && ud->walktimer != INVALID_TIMER)
						continue;
				}
				count++;
			}
		}
	}

	if (type&BL_MOB) {
		for (bl = map->list[m].block_mob[bx+by*map->list[m].bxs]; bl != NULL; bl = bl->next) {
			if (bl->x == x && bl->y == y) {
				if (flag&0x2) {
					struct status_change *sc = status->get_sc(bl);
					if (sc && (sc->option&OPTION_INVISIBLE))
						continue;
				}
				if (flag&0x1) {
					struct unit_data *ud = unit->bl2ud(bl);
					if (ud && ud->walktimer != INVALID_TIMER)
						continue;
				}
				count++;
			}
		}
	}

	return count;
}
/*
 * Looks for a skill unit on a given cell
 * flag&1: runs battle_check_target check based on unit->group->target_flag
 */
static struct skill_unit *map_find_skill_unit_oncell(struct block_list *target, int16 x, int16 y, uint16 skill_id, struct skill_unit *out_unit, int flag)
{
	int16 m,bx,by;
	struct block_list *bl;
	struct skill_unit *su;

	nullpo_retr(NULL, target);
	m = target->m;

	Assert_ret(m >= -1);
	if (m < 0)
		return 0;
	Assert_ret(m < map->count);
	Assert_ret(map->list[m].block != NULL);

	if (x < 0 || y < 0 || (x >= map->list[m].xs) || (y >= map->list[m].ys))
		return NULL;

	bx = x/BLOCK_SIZE;
	by = y/BLOCK_SIZE;

	for( bl = map->list[m].block[bx+by*map->list[m].bxs] ; bl != NULL ; bl = bl->next ) {
		if (bl->x != x || bl->y != y || bl->type != BL_SKILL)
			continue;

		su = BL_UCAST(BL_SKILL, bl);
		if( su == out_unit || !su->alive || !su->group || su->group->skill_id != skill_id )
			continue;
		if( !(flag&1) || battle->check_target(&su->bl,target,su->group->target_flag) > 0 )
			return su;
	}
	return NULL;
}

/**
 * @name Functions for block_list search and manipulation
 *
 * @{
 */

/**
 * Applies func to every block_list in bl_list starting with bl_list[blockcount].
 * Sets bl_list_count back to blockcount.
 * Returns the sum of values returned by func.
 * @param func Function to be applied
 * @param blockcount Index of first relevant entry in bl_list
 * @param max Maximum sum of values returned by func (usually max number of func calls)
 * @param args Extra arguments for func
 * @return Sum of the values returned by func
 */
static int bl_vforeach(int (*func)(struct block_list*, va_list), int blockcount, int max, va_list args)
{
	GUARD_MAP_LOCK

	int i;
	int returnCount = 0;

	map->freeblock_lock();
	for (i = blockcount; i < map->bl_list_count && returnCount < max; i++) {
		if (map->bl_list[i]->prev) { //func() may delete this bl_list[] slot, checking for prev ensures it wasn't queued for deletion.
			va_list argscopy;
			va_copy(argscopy, args);
			returnCount += func(map->bl_list[i], argscopy);
			va_end(argscopy);
		}
	}
	map->freeblock_unlock();

	map->bl_list_count = blockcount;

	return returnCount;
}

/**
 * Applies func to every block_list object of bl_type type on map m.
 * Returns the sum of values returned by func.
 * @param func Function to be applied
 * @param m Map id
 * @param type enum bl_type
 * @param args Extra arguments for func
 * @return Sum of the values returned by func
 */
static int map_vforeachinmap(int (*func)(struct block_list*, va_list), int16 m, int type, va_list args)
{
	int i;
	int returnCount = 0;
	int bsize;
	va_list argscopy;
	struct block_list *bl;
	int blockcount = map->bl_list_count;

	Assert_ret(m >= -1);
	if (m < 0)
		return 0;
	Assert_ret(m < map->count);
	Assert_ret(map->list[m].block != NULL);

	bsize = map->list[m].bxs * map->list[m].bys;
	for (i = 0; i < bsize; i++) {
		if (type&~BL_MOB) {
			for (bl = map->list[m].block[i]; bl != NULL; bl = bl->next) {
				if (bl->type&type) {
					if( map->bl_list_count >= map->bl_list_size )
						map_bl_list_expand();
					map->bl_list[map->bl_list_count++] = bl;
				}
			}
		}
		if (type&BL_MOB) {
			for (bl = map->list[m].block_mob[i]; bl != NULL; bl = bl->next) {
				if( map->bl_list_count >= map->bl_list_size )
					map_bl_list_expand();
				map->bl_list[map->bl_list_count++] = bl;
			}
		}
	}

	va_copy(argscopy, args);
	returnCount = bl_vforeach(func, blockcount, INT_MAX, argscopy);
	va_end(argscopy);

	return returnCount;
}

/**
 * Applies func to every block_list object of bl_type type on map m.
 * Returns the sum of values returned by func.
 * @see map_vforeachinmap
 * @param func Function to be applied
 * @param m Map id
 * @param type enum bl_type
 * @param ... Extra arguments for func
 * @return Sum of the values returned by func
 */
static int map_foreachinmap(int (*func)(struct block_list*, va_list), int16 m, int type, ...)
{
	int returnCount;
	va_list ap;

	va_start(ap, type);
	returnCount = map->vforeachinmap(func, m, type, ap);
	va_end(ap);

	return returnCount;
}

static int map_forcountinmap(int (*func)(struct block_list*, va_list), int16 m, int count, int type, ...)
{
	int returnCount = 0;
	va_list ap;

	if (m < 0)
		return returnCount;

	va_start(ap, type);
	returnCount = map->vforcountinarea(func, m, 0, 0, map->list[m].xs, map->list[m].ys, count, type, ap);
	va_end(ap);

	return returnCount;
}

/**
 * Applies func to every block_list object of bl_type type on all maps
 * of instance instance_id.
 * Returns the sum of values returned by func.
 * @see map_vforeachinmap.
 * @param func Function to be applied
 * @param m Map id
 * @param type enum bl_type
 * @param ap Extra arguments for func
 * @return Sum of the values returned by func
 */
static int map_vforeachininstance(int (*func)(struct block_list*, va_list), int16 instance_id, int type, va_list ap)
{
	int i;
	int returnCount = 0;

	for (i = 0; i < instance->list[instance_id].num_map; i++) {
		int m = instance->list[instance_id].map[i];
		va_list apcopy;
		va_copy(apcopy, ap);
		returnCount += map->vforeachinmap(func, m, type, apcopy);
		va_end(apcopy);
	}

	return returnCount;
}

/**
 * Applies func to every block_list object of bl_type type on all maps
 * of instance instance_id.
 * Returns the sum of values returned by func.
 * @see map_vforeachininstance.
 * @param func Function to be applied
 * @param m Map id
 * @param type enum bl_type
 * @param ... Extra arguments for func
 * @return Sum of the values returned by func
 */
static int map_foreachininstance(int (*func)(struct block_list*, va_list), int16 instance_id, int type, ...)
{
	int returnCount;
	va_list ap;

	va_start(ap, type);
	returnCount = map->vforeachininstance(func, instance_id, type, ap);
	va_end(ap);

	return returnCount;
}

/**
 * Retrieves all map objects in area that are matched by the type
 * and func. Appends them at the end of global bl_list array.
 * @param type Matching enum bl_type
 * @param m Map
 * @param func Matching function
 * @param ... Extra arguments for func
 * @return Number of found objects
 */
static int bl_getall_area(int type, int m, int x0, int y0, int x1, int y1, int (*func)(struct block_list*, va_list), ...)
{
	va_list args;
	int bx, by;
	struct block_list *bl;
	int found = 0;

	Assert_ret(m >= -1);
	if (m < 0)
		return 0;
	Assert_ret(m < map->count);
	const struct map_data *const listm = &map->list[m];
	Assert_ret(listm->xs > 0 && listm->ys > 0);
	Assert_ret(listm->block != NULL);

	// Limit search area to map size
	x0 = min(max(x0, 0), map->list[m].xs - 1);
	y0 = min(max(y0, 0), map->list[m].ys - 1);
	x1 = min(max(x1, 0), map->list[m].xs - 1);
	y1 = min(max(y1, 0), map->list[m].ys - 1);

	if (x1 < x0) swap(x0, x1);
	if (y1 < y0) swap(y0, y1);

	{
		const int x0b = x0 / BLOCK_SIZE;
		const int x1b = x1 / BLOCK_SIZE;
		const int y0b = y0 / BLOCK_SIZE;
		const int y1b = y1 / BLOCK_SIZE;
		const int bxs0 = listm->bxs;

		// duplication for better performance
		if (func != NULL) {
			if (type & ~BL_MOB) {
				for (by = y0b; by <= y1b; by++) {
					const int bxs = by * bxs0;
					for (bx = x0b; bx <= x1b; bx++) {
						for (bl = listm->block[bx + bxs]; bl != NULL; bl = bl->next) {
							const int x = bl->x;
							const int y = bl->y;
							if (bl->type & type && x >= x0 && x <= x1 && y >= y0 && y <= y1) {
								va_start(args, func);
								if (func(bl, args)) {
									if (map->bl_list_count >= map->bl_list_size)
										map_bl_list_expand();
									map->bl_list[map->bl_list_count++] = bl;
									found++;
								}
								va_end(args);
							}
						}
					}
				}
			}
			if (type & BL_MOB) {
				for (by = y0b; by <= y1b; by++) {
					const int bxs = by * bxs0;
					for (bx = x0b; bx <= x1b; bx++) {
						for (bl = listm->block_mob[bx + bxs]; bl != NULL; bl = bl->next) {
							const int x = bl->x;
							const int y = bl->y;
							if (x >= x0 && x <= x1 && y >= y0 && y <= y1) {
								va_start(args, func);
								if (func(bl, args)) {
									if (map->bl_list_count >= map->bl_list_size)
										map_bl_list_expand();
									map->bl_list[map->bl_list_count++] = bl;
									found++;
								}
								va_end(args);
							}
						}
					}
				}
			}
		} else {  // func != NULL
			if (type & ~BL_MOB) {
				for (by = y0b; by <= y1b; by++) {
					const int bxs = by * bxs0;
					for (bx = x0b; bx <= x1b; bx++) {
						for (bl = listm->block[bx + bxs]; bl != NULL; bl = bl->next) {
							const int x = bl->x;
							const int y = bl->y;
							if (bl->type & type && x >= x0 && x <= x1 && y >= y0 && y <= y1) {
								if (map->bl_list_count >= map->bl_list_size)
									map_bl_list_expand();
								map->bl_list[map->bl_list_count++] = bl;
								found++;
							}
						}
					}
				}
			}
			if (type & BL_MOB) {
				for (by = y0b; by <= y1b; by++) {
					const int bxs = by * bxs0;
					for (bx = x0b; bx <= x1b; bx++) {
						for (bl = listm->block_mob[bx + bxs]; bl != NULL; bl = bl->next) {
							const int x = bl->x;
							const int y = bl->y;
							if (x >= x0 && x <= x1 && y >= y0 && y <= y1) {
								if (map->bl_list_count >= map->bl_list_size)
									map_bl_list_expand();
								map->bl_list[map->bl_list_count++] = bl;
								found++;
							}
						}
					}
				}
			}
		}
	}
	return found;
}

/**
 * Checks if bl is within range cells from center.
 * If CIRCULAR AREA is not used always returns 1, since
 * preliminary range selection is already done in bl_getall_area.
 * @return 1 if matches, 0 otherwise
 */
static int bl_vgetall_inrange(struct block_list *bl, va_list args)
{
#ifdef CIRCULAR_AREA
	struct block_list *center = va_arg(args, struct block_list*);
	int range = va_arg(args, int);
	if (!check_distance_bl(center, bl, range))
		return 0;
#endif
	return 1;
}

/**
 * Applies func to every block_list object of bl_type type within range cells from center.
 * Area is rectangular, unless CIRCULAR_AREA is defined.
 * Returns the sum of values returned by func.
 * @param func Function to be applied
 * @param center Center of the selection area
 * @param range Range in cells from center
 * @param type enum bl_type
 * @param ap Extra arguments for func
 * @return Sum of the values returned by func
 */
static int map_vforeachinrange(int (*func)(struct block_list*, va_list), struct block_list *center, int16 range, int type, va_list ap)
{
	int returnCount = 0;
	int blockcount = map->bl_list_count;
	va_list apcopy;

	if (range < 0) range *= -1;

	bl_getall_area(type, center->m, center->x - range, center->y - range, center->x + range, center->y + range, bl_vgetall_inrange, center, range);

	va_copy(apcopy, ap);
	returnCount = bl_vforeach(func, blockcount, INT_MAX, apcopy);
	va_end(apcopy);

	return returnCount;
}

/**
 * Applies func to every block_list object of bl_type type within range cells from center.
 * Area is rectangular, unless CIRCULAR_AREA is defined.
 * Returns the sum of values returned by func.
 * @see map_vforeachinrange
 * @param func Function to be applied
 * @param center Center of the selection area
 * @param range Range in cells from center
 * @param type enum bl_type
 * @param ... Extra arguments for func
 * @return Sum of the values returned by func
 */
static int map_foreachinrange(int (*func)(struct block_list*, va_list), struct block_list *center, int16 range, int type, ...)
{
	int returnCount;
	va_list ap;

	va_start(ap, type);
	returnCount = map->vforeachinrange(func, center, range, type, ap);
	va_end(ap);

	return returnCount;
}

/**
 * Applies func to some block_list objects of bl_type type within range cells from center.
 * Limit is set by count parameter.
 * Area is rectangular, unless CIRCULAR_AREA is defined.
 * Returns the sum of values returned by func.
 * @param func Function to be applied
 * @param center Center of the selection area
 * @param range Range in cells from center
 * @param count Maximum sum of values returned by func (usually max number of func calls)
 * @param type enum bl_type
 * @param ap Extra arguments for func
 * @return Sum of the values returned by func
 */
static int map_vforcountinrange(int (*func)(struct block_list*, va_list), struct block_list *center, int16 range, int count, int type, va_list ap)
{
	int returnCount = 0;
	int blockcount = map->bl_list_count;
	va_list apcopy;

	if (range < 0) range *= -1;

	bl_getall_area(type, center->m, center->x - range, center->y - range, center->x + range, center->y + range, bl_vgetall_inrange, center, range);

	va_copy(apcopy, ap);
	returnCount = bl_vforeach(func, blockcount, count, apcopy);
	va_end(apcopy);

	return returnCount;
}

/**
 * Applies func to some block_list objects of bl_type type within range cells from center.
 * Limit is set by count parameter.
 * Area is rectangular, unless CIRCULAR_AREA is defined.
 * Returns the sum of values returned by func.
 * @see map_vforcountinrange
 * @param func Function to be applied
 * @param center Center of the selection area
 * @param range Range in cells from center
 * @param count Maximum sum of values returned by func (usually max number of func calls)
 * @param type enum bl_type
 * @param ... Extra arguments for func
 * @return Sum of the values returned by func
 */
static int map_forcountinrange(int (*func)(struct block_list*, va_list), struct block_list *center, int16 range, int count, int type, ...)
{
	int returnCount;
	va_list ap;

	va_start(ap, type);
	returnCount = map->vforcountinrange(func, center, range, count, type, ap);
	va_end(ap);

	return returnCount;
}

/**
 * Checks if bl is within shooting range from center.
 * There must be a shootable path between bl and center.
 * Does not check for range if CIRCULAR AREA is not defined, since
 * preliminary range selection is already done in bl_getall_area.
 * @return 1 if matches, 0 otherwise
 */
static int bl_vgetall_inshootrange(struct block_list *bl, va_list args)
{
	struct block_list *center = va_arg(args, struct block_list*);
#ifdef CIRCULAR_AREA
	int range = va_arg(args, int);
	nullpo_ret(center);
	nullpo_ret(bl);

	if (!check_distance_bl(center, bl, range))
		return 0;
#endif
	if (!path->search_long(NULL, center, center->m, center->x, center->y, bl->x, bl->y, CELL_CHKWALL))
		return 0;
	return 1;
}

/**
 * Applies func to every block_list object of bl_type type within shootable range from center.
 * There must be a shootable path between bl and center.
 * Area is rectangular, unless CIRCULAR_AREA is defined.
 * Returns the sum of values returned by func.
 * @param func Function to be applied
 * @param center Center of the selection area
 * @param range Range in cells from center
 * @param type enum bl_type
 * @param ap Extra arguments for func
 * @return Sum of the values returned by func
 */
static int map_vforeachinshootrange(int (*func)(struct block_list*, va_list), struct block_list *center, int16 range, int type, va_list ap)
{
	int returnCount = 0;
	int blockcount = map->bl_list_count;
	va_list apcopy;

	if (range < 0) range *= -1;

	bl_getall_area(type, center->m, center->x - range, center->y - range, center->x + range, center->y + range, bl_vgetall_inshootrange, center, range);

	va_copy(apcopy, ap);
	returnCount = bl_vforeach(func, blockcount, INT_MAX, ap);
	va_end(apcopy);

	return returnCount;
}

/**
 * Applies func to every block_list object of bl_type type within shootable range from center.
 * There must be a shootable path between bl and center.
 * Area is rectangular, unless CIRCULAR_AREA is defined.
 * Returns the sum of values returned by func.
 * @param func Function to be applied
 * @param center Center of the selection area
 * @param range Range in cells from center
 * @param type enum bl_type
 * @param ... Extra arguments for func
 * @return Sum of the values returned by func
 */
static int map_foreachinshootrange(int (*func)(struct block_list*, va_list), struct block_list *center, int16 range, int type, ...)
{
	int returnCount;
	va_list ap;

	va_start(ap, type);
	returnCount = map->vforeachinshootrange(func, center, range, type, ap);
	va_end(ap);

	return returnCount;
}

/**
 * Applies func to every block_list object of bl_type type in
 * rectangular area (x0,y0)~(x1,y1) on map m.
 * Returns the sum of values returned by func.
 * @param func Function to be applied
 * @param m Map id
 * @param x0 Starting X-coordinate
 * @param y0 Starting Y-coordinate
 * @param x1 Ending X-coordinate
 * @param y1 Ending Y-coordinate
 * @param type enum bl_type
 * @param ap Extra arguments for func
 * @return Sum of the values returned by func
 */
static int map_vforeachinarea(int (*func)(struct block_list*, va_list), int16 m, int16 x0, int16 y0, int16 x1, int16 y1, int type, va_list ap)
{
	int returnCount = 0;
	int blockcount = map->bl_list_count;
	va_list apcopy;

	bl_getall_area(type, m, x0, y0, x1, y1, NULL);

	va_copy(apcopy, ap);
	returnCount = bl_vforeach(func, blockcount, INT_MAX, apcopy);
	va_end(apcopy);

	return returnCount;
}

/**
 * Applies func to every block_list object of bl_type type in
 * rectangular area (x0,y0)~(x1,y1) on map m.
 * Returns the sum of values returned by func.
 * @see map_vforeachinarea
 * @param func Function to be applied
 * @param m Map id
 * @param x0 Starting X-coordinate
 * @param y0 Starting Y-coordinate
 * @param x1 Ending X-coordinate
 * @param y1 Ending Y-coordinate
 * @param type enum bl_type
 * @param ... Extra arguments for func
 * @return Sum of the values returned by func
 */
static int map_foreachinarea(int (*func)(struct block_list*, va_list), int16 m, int16 x0, int16 y0, int16 x1, int16 y1, int type, ...)
{
	int returnCount;
	va_list ap;

	va_start(ap, type);
	returnCount = map->vforeachinarea(func, m, x0, y0, x1, y1, type, ap);
	va_end(ap);

	return returnCount;
}

/**
 * Applies func to some block_list objects of bl_type type in
 * rectangular area (x0,y0)~(x1,y1) on map m.
 * Limit is set by @count parameter.
 * Returns the sum of values returned by func.
 * @param func Function to be applied
 * @param m Map id
 * @param x0 Starting X-coordinate
 * @param y0 Starting Y-coordinate
 * @param x1 Ending X-coordinate
 * @param y1 Ending Y-coordinate
 * @param count Maximum sum of values returned by func (usually max number of func calls)
 * @param type enum bl_type
 * @param ap Extra arguments for func
 * @return Sum of the values returned by func
 */
static int map_vforcountinarea(int (*func)(struct block_list*, va_list), int16 m, int16 x0, int16 y0, int16 x1, int16 y1, int count, int type, va_list ap)
{
	int returnCount = 0;
	int blockcount = map->bl_list_count;
	va_list apcopy;

	bl_getall_area(type, m, x0, y0, x1, y1, NULL);

	va_copy(apcopy, ap);
	returnCount = bl_vforeach(func, blockcount, count, apcopy);
	va_end(apcopy);

	return returnCount;
}

/**
 * Applies func to some block_list objects of bl_type type in
 * rectangular area (x0,y0)~(x1,y1) on map m.
 * Limit is set by @count parameter.
 * Returns the sum of values returned by func.
 * @see map_vforcountinarea
 * @param func Function to be applied
 * @param m Map id
 * @param x0 Starting X-coordinate
 * @param y0 Starting Y-coordinate
 * @param x1 Ending X-coordinate
 * @param y1 Ending Y-coordinate
 * @param count Maximum sum of values returned by func (usually max number of func calls)
 * @param type enum bl_type
 * @param ... Extra arguments for func
 * @return Sum of the values returned by func
 */
static int map_forcountinarea(int (*func)(struct block_list*, va_list), int16 m, int16 x0, int16 y0, int16 x1, int16 y1, int count, int type, ...)
{
	int returnCount;
	va_list ap;

	va_start(ap, type);
	returnCount = map->vforcountinarea(func, m, x0, y0, x1, y1, count, type, ap);
	va_end(ap);

	return returnCount;
}

/**
 * Checks if bl is inside area that was in range cells from the center
 * before it was moved by (dx,dy) cells, but it is not in range cells
 * from center after movement is completed.
 * In other words, checks if bl is inside area that is no longer covered
 * by center's range.
 * Preliminary range selection is already done in bl_getall_area.
 * @return 1 if matches, 0 otherwise
 */
static int bl_vgetall_inmovearea(struct block_list *bl, va_list args)
{
	int dx = va_arg(args, int);
	int dy = va_arg(args, int);
	struct block_list *center = va_arg(args, struct block_list*);
	int range = va_arg(args, int);

	nullpo_ret(bl);
	nullpo_ret(center);

	if ((dx > 0 && bl->x < center->x - range + dx) ||
		(dx < 0 && bl->x > center->x + range + dx) ||
		(dy > 0 && bl->y < center->y - range + dy) ||
		(dy < 0 && bl->y > center->y + range + dy))
		return 1;
	return 0;
}

/**
 * Applies func to every block_list object of bl_type type in
 * area that was covered by range cells from center, but is no
 * longer after center is moved by (dx,dy) cells (i.e. area that
 * center has lost sight of).
 * If used after center has reached its destination and with
 * opposed movement vector (-dx,-dy), selection corresponds
 * to new area in center's view).
 * Uses rectangular area.
 * Returns the sum of values returned by func.
 * @param func Function to be applied
 * @param center Center of the selection area
 * @param range Range in cells from center
 * @param dx Center's movement on X-axis
 * @param dy Center's movement on Y-axis
 * @param type enum bl_type
 * @param ap Extra arguments for func
 * @return Sum of the values returned by func
 */
static int map_vforeachinmovearea(int (*func)(struct block_list*, va_list), struct block_list *center, int16 range, int16 dx, int16 dy, int type, va_list ap)
{
	int returnCount = 0;
	int blockcount = map->bl_list_count;
	int m, x0, x1, y0, y1;
	va_list apcopy;

	if (!range) return 0;
	if (!dx && !dy) return 0; // No movement.

	if (range < 0) range *= -1;

	m = center->m;
	x0 = center->x - range;
	x1 = center->x + range;
	y0 = center->y - range;
	y1 = center->y + range;

	if (dx == 0 || dy == 0) { // Movement along one axis only.
		if (dx == 0) {
			if (dy < 0) { y0 = y1 + dy + 1; } // Moving south
			else        { y1 = y0 + dy - 1; } // North
		} else { //dy == 0
			if (dx < 0) { x0 = x1 + dx + 1; } // West
			else        { x1 = x0 + dx - 1; } // East
		}
		bl_getall_area(type, m, x0, y0, x1, y1, NULL);
	}
	else { // Diagonal movement
		bl_getall_area(type, m, x0, y0, x1, y1, bl_vgetall_inmovearea, dx, dy, center, range);
	}

	va_copy(apcopy, ap);
	returnCount = bl_vforeach(func, blockcount, INT_MAX, apcopy);
	va_end(apcopy);

	return returnCount;
}

/**
 * Applies func to every block_list object of bl_type type in
 * area that was covered by range cells from center, but is no
 * longer after center is moved by (dx,dy) cells (i.e. area that
 * center has lost sight of).
 * If used after center has reached its destination and with
 * opposed movement vector (-dx,-dy), selection corresponds
 * to new area in center's view).
 * Uses rectangular area.
 * Returns the sum of values returned by func.
 * @see map_vforeachinmovearea
 * @param func Function to be applied
 * @param center Center of the selection area
 * @param range Range in cells from center
 * @param dx Center's movement on X-axis
 * @param dy Center's movement on Y-axis
 * @param type enum bl_type
 * @param ... Extra arguments for func
 * @return Sum of the values returned by func
 */
static int map_foreachinmovearea(int (*func)(struct block_list*, va_list), struct block_list *center, int16 range, int16 dx, int16 dy, int type, ...)
{
	int returnCount;
	va_list ap;

	va_start(ap, type);
	returnCount = map->vforeachinmovearea(func, center, range, dx, dy, type, ap);
	va_end(ap);

	return returnCount;
}

/**
 * Applies func to every block_list object of bl_type type in
 * cell (x,y) on map m.
 * Returns the sum of values returned by func.
 * @param func Function to be applied
 * @param m Map id
 * @param x Target cell X-coordinate
 * @param y Target cell Y-coordinate
 * @param type enum bl_type
 * @param ap Extra arguments for func
 * @return Sum of the values returned by func
 */
static int map_vforeachincell(int (*func)(struct block_list*, va_list), int16 m, int16 x, int16 y, int type, va_list ap)
{
	int returnCount = 0;
	int blockcount = map->bl_list_count;
	va_list apcopy;

	bl_getall_area(type, m, x, y, x, y, NULL);

	va_copy(apcopy, ap);
	returnCount = bl_vforeach(func, blockcount, INT_MAX, apcopy);
	va_end(apcopy);

	return returnCount;
}

/**
 * Applies func to every block_list object of bl_type type in
 * cell (x,y) on map m.
 * Returns the sum of values returned by func.
 * @param func Function to be applied
 * @param m Map id
 * @param x Target cell X-coordinate
 * @param y Target cell Y-coordinate
 * @param type enum bl_type
 * @param ... Extra arguments for func
 * @return Sum of the values returned by func
 */
static int map_foreachincell(int (*func)(struct block_list*, va_list), int16 m, int16 x, int16 y, int type, ...)
{
	int returnCount;
	va_list ap;

	va_start(ap, type);
	returnCount = map->vforeachincell(func, m, x, y, type, ap);
	va_end(ap);

	return returnCount;
}

/**
 * Helper function for map_foreachinpath()
 * Checks if shortest distance from bl to path
 * between (x0,y0) and (x1,y1) is shorter than range.
 * @see map_foreachinpath
 */
static int bl_vgetall_inpath(struct block_list *bl, va_list args)
{
	int m  = va_arg(args, int);
	int x0 = va_arg(args, int);
	int y0 = va_arg(args, int);
	int x1 = va_arg(args, int);
	int y1 = va_arg(args, int);
	int range = va_arg(args, int);
	int len_limit = va_arg(args, int);
	int magnitude2 = va_arg(args, int);

	int xi;
	int yi;
	int k;

	nullpo_ret(bl);
	xi = bl->x;
	yi = bl->y;
	k = ( xi - x0 ) * ( x1 - x0 ) + ( yi - y0 ) * ( y1 - y0 );

	if ( k < 0 || k > len_limit ) //Since more skills use this, check for ending point as well.
		return 0;

	if ( k > magnitude2 && !path->search_long(NULL, NULL, m, x0, y0, xi, yi, CELL_CHKWALL) )
		return 0; //Targets beyond the initial ending point need the wall check.

	/**
	 * We're shifting the coords 8 bits higher
	 * to have higher precision on the cell comparisons.
	 * Especially the multiplication of k * (x1 - x0)
	 * between the intersecting point on the line and the bl we might affect
	 * requires higher precision due to int math.
	 * Since the coords are 8 bits higher,
	 * the range is 8 bits higher too when comparing
	 */
	k = (k << 8) / magnitude2;
	int xu = (x0 << 8) + k * (x1 - x0);
	int yu = (y0 << 8) + k * (y1 - y0);
	xi <<= 8;
	yi <<= 8;

	/**
	 * We're calculating the distance like path->distance,
	 * but without CIRCULAR_AREA since NPCs use map->foreachinpath too.
	 */
	int dx = abs(xi - xu);
	int dy = abs(yi - yu);
	int distance = (dx < dy ? dy : dx);
	if (distance > (range << 8))
		return 0;

	return 1;
}

/**
 * Applies func to every block_list object of bl_type type in
 * path on a line between (x0,y0) and (x1,y1) on map m.
 * Path starts at (x0,y0) and is \a length cells long and \a range cells wide.
 * Objects beyond the initial (x1,y1) ending point are checked
 * for walls in the path.
 * Returns the sum of values returned by func.
 * @param func Function to be applied
 * @param m Map id
 * @param x Target cell X-coordinate
 * @param y Target cell Y-coordinate
 * @param type enum bl_type
 * @param ap Extra arguments for func
 * @return Sum of the values returned by func
 */
static int map_vforeachinpath(int (*func)(struct block_list*, va_list), int16 m, int16 x0, int16 y0, int16 x1, int16 y1, int16 range, int length, int type, va_list ap)
{
	// [Skotlex]
	// check for all targets in the square that
	// contains the initial and final positions (area range increased to match the
	// radius given), then for each object to test, calculate the distance to the
	// path and include it if the range fits and the target is in the line (0<k<1,
	// as they call it).
	// The implementation I took as reference is found at
	// http://web.archive.org/web/20050720125314/http://astronomy.swin.edu.au/~pbourke/geometry/pointline/
	// http://paulbourke.net/geometry/pointlineplane/
	// I won't use doubles/floats, but pure int math for
	// speed purposes. The range considered is always the same no matter how
	// close/far the target is because that's how SharpShooting works currently in
	// kRO

	int returnCount = 0;
	int blockcount = map->bl_list_count;
	va_list apcopy;

	//method specific variables
	int magnitude2, len_limit; //The square of the magnitude
	int mx0 = x0, mx1 = x1, my0 = y0, my1 = y1;

//Avoid needless calculations by not getting the sqrt right away.
#define MAGNITUDE2(x0, y0, x1, y1) ( ( ( x1 ) - ( x0 ) ) * ( ( x1 ) - ( x0 ) ) + ( ( y1 ) - ( y0 ) ) * ( ( y1 ) - ( y0 ) ) )
	len_limit = magnitude2 = MAGNITUDE2(x0, y0, x1, y1);
	if (magnitude2 < 1) //Same begin and ending point, can't trace path.
		return 0;

	if (length) { //Adjust final position to fit in the given area.
		//TODO: Find an alternate method which does not requires a square root calculation.
		int k = (int)sqrt((float)magnitude2);
		mx1 = x0 + (x1 - x0) * length / k;
		my1 = y0 + (y1 - y0) * length / k;
		len_limit = MAGNITUDE2(x0, y0, mx1, my1);
	}
	//Expand target area to cover range.
	if (mx0 > mx1) {
		mx0 += range;
		mx1 -= range;
	} else {
		mx0 -= range;
		mx1 += range;
	}
	if (my0 > my1) {
		my0 += range;
		my1 -= range;
	} else {
		my0 -= range;
		my1 += range;
	}

	bl_getall_area(type, m, mx0, my0, mx1, my1, bl_vgetall_inpath, m, x0, y0, x1, y1, range, len_limit, magnitude2);

	va_copy(apcopy, ap);
	returnCount = bl_vforeach(func, blockcount, INT_MAX, apcopy);
	va_end(apcopy);

	return returnCount;
}
#undef MAGNITUDE2

/**
 * Applies func to every block_list object of bl_type type in
 * path on a line between (x0,y0) and (x1,y1) on map m.
 * Path starts at (x0,y0) and is \a length cells long and \a range cells wide.
 * Objects beyond the initial (x1,y1) ending point are checked
 * for walls in the path.
 * Returns the sum of values returned by func.
 * @see map_vforeachinpath
 * @param func Function to be applied
 * @param m Map id
 * @param x Target cell X-coordinate
 * @param y Target cell Y-coordinate
 * @param type enum bl_type
 * @param ... Extra arguments for func
 * @return Sum of the values returned by func
 */
static int map_foreachinpath(int (*func)(struct block_list*, va_list), int16 m, int16 x0, int16 y0, int16 x1, int16 y1, int16 range, int length, int type, ...)
{
	int returnCount;
	va_list ap;

	va_start(ap, type);
	returnCount = map->vforeachinpath(func, m, x0, y0, x1, y1, range, length, type, ap);
	va_end(ap);

	return returnCount;
}

/** @} */

/// Generates a new flooritem object id from the interval [MIN_FLOORITEM, MAX_FLOORITEM).
/// Used for floor items, skill units and chatroom objects.
/// @return The new object id
static int map_get_new_object_id(void)
{
	static int last_object_id = MIN_FLOORITEM - 1;
	int i;

	// find a free id
	i = last_object_id + 1;
	while( i != last_object_id ) {
		if( i == MAX_FLOORITEM )
			i = MIN_FLOORITEM;

		if( !idb_exists(map->id_db, i) )
			break;

		++i;
	}

	if( i == last_object_id ) {
		ShowError("map_addobject: no free object id!\n");
		return 0;
	}

	// update cursor
	last_object_id = i;

	return i;
}

/*==========================================
 * Timered function to clear the floor (remove remaining item)
 * Called each flooritem_lifetime ms
 *------------------------------------------*/
static int map_clearflooritem_timer(int tid, int64 tick, int id, intptr_t data)
{
	struct block_list *bl = idb_get(map->id_db, id);
	struct flooritem_data *fitem = BL_CAST(BL_ITEM, bl);

	if (fitem == NULL || fitem->cleartimer != tid) {
		ShowError("map_clearflooritem_timer : error\n");
		return 1;
	}

	if (pet->search_petDB_index(fitem->item_data.nameid, PET_EGG) != INDEX_NOT_FOUND)
		intif->delete_petdata(MakeDWord(fitem->item_data.card[1], fitem->item_data.card[2]));

	clif->clearflooritem(fitem, 0);
	map->deliddb(&fitem->bl);
	map->delblock(&fitem->bl);
	map->freeblock(&fitem->bl);
	return 0;
}

/*
 * clears a single bl item out of the bazooonga.
 */
static void map_clearflooritem(struct block_list *bl)
{
	struct flooritem_data *fitem = BL_CAST(BL_ITEM, bl);

	nullpo_retv(fitem);

	if( fitem->cleartimer != INVALID_TIMER )
		timer->delete(fitem->cleartimer,map->clearflooritem_timer);

	clif->clearflooritem(fitem, 0);
	map->deliddb(&fitem->bl);
	map->delblock(&fitem->bl);
	map->freeblock(&fitem->bl);
}

/*==========================================
 * (m,x,y) locates a random available free cell around the given coordinates
 * to place an BL_ITEM object. Scan area is 9x9, returns 1 on success.
 * x and y are modified with the target cell when successful.
 *------------------------------------------*/
static int map_searchrandfreecell(int16 m, const struct block_list *bl, int16 *x, int16 *y, int stack)
{
	int free_cell,i,j;
	int free_cells[9][2];

	nullpo_ret(x);
	nullpo_ret(y);

	for(free_cell=0,i=-1;i<=1;i++){
		if(i+*y<0 || i+*y>=map->list[m].ys)
			continue;
		for(j=-1;j<=1;j++){
			if(j+*x<0 || j+*x>=map->list[m].xs)
				continue;
			if (map->getcell(m, bl, j + *x, i + *y, CELL_CHKNOPASS) && !map->getcell(m, bl, j + *x, i + *y, CELL_CHKICEWALL))
				continue;
			//Avoid item stacking to prevent against exploits. [Skotlex]
			if(stack && map->count_oncell(m,j+*x,i+*y, BL_ITEM, 0) > stack)
				continue;
			free_cells[free_cell][0] = j+*x;
			free_cells[free_cell++][1] = i+*y;
		}
	}
	if(free_cell==0)
		return 0;
	free_cell = rnd()%free_cell;
	*x = free_cells[free_cell][0];
	*y = free_cells[free_cell][1];
	return 1;
}

static int map_count_sub(struct block_list *bl, va_list ap)
{
	return 1;
}

/**
 * Locates a random free cell (x, y) on a rectangle or the entire map.
 *
 * The rectangles center is either on map `m` at (x, y) or at the location of object `src`.
 * Searches on entire map if range_x < 0 and range_y < 0.
 * @remark Usage in e.g. warping or mob spawning.
 * @param src object used to base reach checks on
 * @param m map to search cells on, if flag has SFC_XY_CENTER set
 * @param[in,out] x pointer to the x-axis
 * @param[in,out] y pointer to the y-axis
 * @param range_x range to east border of rectangle, if range_x < 0 use horizontal map range
 * @param range_y range to north border of rectangle, if range_y < 0 use vertical map range
 * @param flag *flag* parameter based on @enum search_freecell with following options @n
 *  - `& SFC_XY_CENTER` -> 0: `src` as center, 1: center is on `m` at (x, y)
 *  - `& SFC_REACHABLE` -> 1: `src` needs to be able to reach found cell.
 *  - `& SFC_AVOIDPLAYER` -> 1: avoid players around found cell (@see no_spawn_on_player setting)
 * @retval 0 success, free cell found
 * @retval 1 failure, ran out of tries or wrong usage
 * @retval 2 failure, nullpointer
 */
static int map_search_free_cell(struct block_list *src, int16 m, int16 *x, int16 *y,
                               int16 range_x, int16 range_y, int flag)
{
	nullpo_retr(2, x);
	nullpo_retr(2, y);

	if (src == NULL && ((flag & SFC_XY_CENTER) == 0 || (flag & SFC_REACHABLE) != 0)) {
		ShowDebug("map_search_free_cell: Incorrect usage! When src is NULL, flag has to have SFC_XY_CENTER set"
		          " and can't have SFC_REACHABLE set\n");
		return 1;
	}

	int center_x = *x;
	int center_y = *y;
	if ((flag & SFC_XY_CENTER) == 0) {
		nullpo_retr(2, src);
		center_x = src->x;
		center_y = src->y;
		m = src->m;
	}
	if (range_x == 0 && range_y == 0) {
		// No range? Return the target cell then....
		*x = center_x;
		*y = center_y;
		if (map->getcell(m, src, *x, *y, CELL_CHKREACH) == 0)
			return 1;
		else
			return 0;
	}

	int width = 2 * range_x + 1;
	int height = 2 * range_y + 1;
	int tries;
	const int margin = battle_config.search_freecell_map_margin;
	if (range_x < 0 || range_y < 0) {
		if (Assert_chk(map->list[m].xs > 2 * margin && map->list[m].ys > 2 * margin))
			ShowDebug("search_freecell_map_margin is too big for at least one map.");
		tries = min(map->list[m].xs * map->list[m].ys, 500); // For likely every map this will be 500...
	} else {
		tries = min(width * height, 100);
	}

	int avoidplayer_retries = 0;
	while (tries-- > 0) {
		if (range_x < 0)
			*x = rnd() % max(1, map->list[m].xs - 2 * margin) + margin;
		else
			*x = rnd() % width - range_x + center_x;

		if (range_y < 0)
			*y = rnd() % max(1, map->list[m].ys - 2 * margin) + margin;
		else
			*y = rnd() % height - range_y + center_y;

		// Ensure we don't get out of map bounds.
		*x = cap_value(*x, 1, map->list[m].xs - 1);
		*y = cap_value(*y, 1, map->list[m].ys - 1);

		if (*x == center_x && *y == center_y)
			continue; // Avoid picking the same target tile.

		if (map->getcell(m, src, *x, *y, CELL_CHKREACH) == 0)
			continue;

		if ((flag & SFC_REACHABLE) != 0 && !unit->can_reach_pos(src, *x, *y, 1))
			continue;

		if ((flag & SFC_AVOIDPLAYER) == 0)
			return 0;

		if (avoidplayer_retries >= 100)
			return 1; // Limit of retries reached.

		if (avoidplayer_retries++ < battle_config.no_spawn_on_player
		    && map->foreachinarea(map->count_sub, m, *x - AREA_SIZE, *y - AREA_SIZE,
					  *x + AREA_SIZE, *y + AREA_SIZE, BL_PC) != 0)
			continue;
		return 0;
	}
	*x = center_x;
	*y = center_y;
	return 1;
}

/*==========================================
 * Locates the closest, walkable cell with no blocks of a certain type on it
 * Returns true on success and sets x and y to cell found.
 * Otherwise returns false and x and y are not changed.
 * type: Types of block to count
 * flag:
 *   0x1 - only count standing units
 *------------------------------------------*/
static bool map_closest_freecell(int16 m, const struct block_list *bl, int16 *x, int16 *y, int type, int flag)
{
	enum unit_dir dir = battle_config.keep_dir_free_cell ? unit->getdir(bl) : UNIT_DIR_EAST;
	int16 tx;
	int16 ty;
	int costrange = 10;

	nullpo_ret(x);
	nullpo_ret(y);
	tx = *x;
	ty = *y;

	if(!map->count_oncell(m, tx, ty, type, flag))
		return true; //Current cell is free

	//Algorithm only works up to costrange of 34
	while(costrange <= 34) {
		short dx = dirx[dir];
		short dy = diry[dir];

		//Linear search
		if (!unit_is_diagonal_dir(dir) && (costrange % MOVE_COST) == 0) {
			tx = *x+dx*(costrange/MOVE_COST);
			ty = *y+dy*(costrange/MOVE_COST);
			if (!map->count_oncell(m, tx, ty, type, flag) && map->getcell(m, bl, tx, ty, CELL_CHKPASS)) {
				*x = tx;
				*y = ty;
				return true;
			}
		}
		//Full diagonal search
		else if (unit_is_diagonal_dir(dir) && (costrange % MOVE_DIAGONAL_COST) == 0) {
			tx = *x+dx*(costrange/MOVE_DIAGONAL_COST);
			ty = *y+dy*(costrange/MOVE_DIAGONAL_COST);
			if (!map->count_oncell(m, tx, ty, type, flag) && map->getcell(m, bl, tx, ty, CELL_CHKPASS)) {
				*x = tx;
				*y = ty;
				return true;
			}
		}
		//One cell diagonal, rest linear (TODO: Find a better algorithm for this)
		else if (unit_is_diagonal_dir(dir) && (costrange % MOVE_COST) == 4) {
			tx = *x + dx;
			ty = *y + dy;
			if (unit_is_dir_or_opposite(dir, UNIT_DIR_SOUTHWEST))
				tx = tx * costrange / MOVE_COST;
			if (unit_is_dir_or_opposite(dir, UNIT_DIR_NORTHWEST))
				ty = ty * costrange / MOVE_COST;
			if (!map->count_oncell(m, tx, ty, type, flag) && map->getcell(m, bl, tx, ty, CELL_CHKPASS)) {
				*x = tx;
				*y = ty;
				return true;
			}
			tx = *x + dx;
			ty = *y + dy;
			if (unit_is_dir_or_opposite(dir, UNIT_DIR_NORTHWEST))
				tx = tx * costrange / MOVE_COST;
			if (unit_is_dir_or_opposite(dir, UNIT_DIR_SOUTHWEST))
				ty = ty * costrange / MOVE_COST;
			if (!map->count_oncell(m, tx, ty, type, flag) && map->getcell(m, bl, tx, ty, CELL_CHKPASS)) {
				*x = tx;
				*y = ty;
				return true;
			}
		}

		//Get next direction
		if (dir == UNIT_DIR_SOUTHEAST) {
			//Diagonal search complete, repeat with higher cost range
			if(costrange == 14) costrange += 6;
			else if(costrange == 28 || costrange >= 38) costrange += 2;
			else costrange += 4;
			dir = UNIT_DIR_EAST;
		} else if (dir == UNIT_DIR_SOUTH) {
			//Linear search complete, switch to diagonal directions
			dir = UNIT_DIR_NORTHEAST;
		} else {
			dir = unit_get_ccw90_dir(dir);
		}
	}

	return false;
}

/*==========================================
 * Add an item to location (m,x,y)
 * Parameters
 * @item_data item attributes
 * @amount quantity
 * @m, @x, @y mapid,x,y
 * @first_charid, @second_charid, @third_charid, looting priority
 * @flag: &1 MVP item. &2 do stacking check.
 * @showdropeffect: show effect when the item is dropped.
 *------------------------------------------*/
static int map_addflooritem(const struct block_list *bl, struct item *item_data, int amount, int16 m, int16 x, int16 y, int first_charid, int second_charid, int third_charid, int flags, bool showdropeffect)
{
	int r;
	struct flooritem_data *fitem=NULL;

	nullpo_ret(item_data);

	if (!map->searchrandfreecell(m, bl, &x, &y, (flags&2)?1:0))
		return 0;
	r=rnd();

	fitem = ers_alloc(map->flooritem_ers, struct flooritem_data);

	fitem->bl.type = BL_ITEM;
	fitem->bl.prev = fitem->bl.next = NULL;
	fitem->bl.m = m;
	fitem->bl.x = x;
	fitem->bl.y = y;
	fitem->bl.id = map->get_new_object_id();
	fitem->showdropeffect = showdropeffect;
	if(fitem->bl.id==0){
		ers_free(map->flooritem_ers, fitem);
		return 0;
	}

	fitem->first_get_charid = first_charid;
	fitem->first_get_tick = timer->gettick() + ((flags&1) ? battle_config.mvp_item_first_get_time : battle_config.item_first_get_time);
	fitem->second_get_charid = second_charid;
	fitem->second_get_tick = fitem->first_get_tick + ((flags&1) ? battle_config.mvp_item_second_get_time : battle_config.item_second_get_time);
	fitem->third_get_charid = third_charid;
	fitem->third_get_tick = fitem->second_get_tick + ((flags&1) ? battle_config.mvp_item_third_get_time : battle_config.item_third_get_time);

	memcpy(&fitem->item_data,item_data,sizeof(*item_data));
	fitem->item_data.amount=amount;
	fitem->subx=(r&3)*3+3;
	fitem->suby=((r>>2)&3)*3+3;
	fitem->cleartimer=timer->add(timer->gettick()+battle_config.flooritem_lifetime,map->clearflooritem_timer,fitem->bl.id,0);

	map->addiddb(&fitem->bl);
	map->addblock(&fitem->bl);
	clif->dropflooritem(fitem);

	return fitem->bl.id;
}

/**
 * @see DBCreateData
 */
static struct DBData create_charid2nick(union DBKey key, va_list args)
{
	struct charid2nick *p;
	CREATE(p, struct charid2nick, 1);
	return DB->ptr2data(p);
}

/// Adds(or replaces) the nick of charid to nick_db and fullfils pending requests.
/// Does nothing if the character is online.
static void map_addnickdb(int charid, const char *nick)
{
	struct charid2nick* p;
	struct charid_request* req;

	if( map->charid2sd(charid) )
		return;// already online

	p = idb_ensure(map->nick_db, charid, map->create_charid2nick);
	safestrncpy(p->nick, nick, sizeof(p->nick));

	while (p->requests) {
		struct map_session_data* sd;
		req = p->requests;
		p->requests = req->next;
		sd = map->charid2sd(req->charid);
		if (sd)
			clif->solved_charname(sd->fd, charid, p->nick);
		aFree(req);
	}
}

/// Removes the nick of charid from nick_db.
/// Sends name to all pending requests on charid.
static void map_delnickdb(int charid, const char *name)
{
	struct charid2nick* p;
	struct charid_request* req;
	struct DBData data;

	if (!map->nick_db->remove(map->nick_db, DB->i2key(charid), &data) || (p = DB->data2ptr(&data)) == NULL)
		return;

	while (p->requests) {
		struct map_session_data* sd;
		req = p->requests;
		p->requests = req->next;
		sd = map->charid2sd(req->charid);
		if (sd)
			clif->solved_charname(sd->fd, charid, name);
		aFree(req);
	}
	aFree(p);
}

/// Notifies sd of the nick of charid.
/// Uses the name in the character if online.
/// Uses the name in nick_db if offline.
static void map_reqnickdb(struct map_session_data  *sd, int charid)
{
	struct charid2nick* p;
	struct charid_request* req;
	struct map_session_data* tsd;

	nullpo_retv(sd);

	tsd = map->charid2sd(charid);
	if( tsd ) {
		clif->solved_charname(sd->fd, charid, tsd->status.name);
		return;
	}

	p = idb_ensure(map->nick_db, charid, map->create_charid2nick);
	if( *p->nick ) {
		clif->solved_charname(sd->fd, charid, p->nick);
		return;
	}
	// not in cache, request it
	CREATE(req, struct charid_request, 1);
	req->charid = sd->status.char_id;
	req->next = p->requests;
	p->requests = req;
	chrif->searchcharid(charid);
}

/*==========================================
 * add bl to id_db
 *------------------------------------------*/
static void map_addiddb(struct block_list *bl)
{
	nullpo_retv(bl);

	if (bl->type == BL_PC) {
		struct map_session_data *sd = BL_UCAST(BL_PC, bl);
		idb_put(map->pc_db,sd->bl.id,sd);
		idb_put(map->charid_db,sd->status.char_id,sd);
	} else if (bl->type == BL_MOB) {
		struct mob_data *md = BL_UCAST(BL_MOB, bl);
		idb_put(map->mobid_db,bl->id,bl);

		if (md->state.boss == BTYPE_MVP)
			idb_put(map->bossid_db, bl->id, bl);
	}

	if( bl->type & BL_REGEN )
		idb_put(map->regen_db, bl->id, bl);

	idb_put(map->id_db,bl->id,bl);
}

/*==========================================
 * remove bl from id_db
 *------------------------------------------*/
static void map_deliddb(struct block_list *bl)
{
	nullpo_retv(bl);

	if (bl->type == BL_PC) {
		struct map_session_data *sd = BL_UCAST(BL_PC, bl);
		idb_remove(map->pc_db,sd->bl.id);
		idb_remove(map->charid_db,sd->status.char_id);
	} else if (bl->type == BL_MOB) {
		idb_remove(map->mobid_db,bl->id);
		idb_remove(map->bossid_db,bl->id);
	}

	if( bl->type & BL_REGEN )
		idb_remove(map->regen_db,bl->id);

	idb_remove(map->id_db,bl->id);
}

/*==========================================
 * Standard call when a player connection is closed.
 *------------------------------------------*/
static int map_quit(struct map_session_data *sd)
{
	int i;

	nullpo_ret(sd);

	if(!sd->state.active) { //Removing a player that is not active.
		struct auth_node *node = chrif->search(sd->status.account_id);
		if (node && node->char_id == sd->status.char_id &&
			node->state != ST_LOGOUT)
			//Except when logging out, clear the auth-connect data immediately.
			chrif->auth_delete(node->account_id, node->char_id, node->state);
		//Non-active players should not have loaded any data yet (or it was cleared already) so no additional cleanups are needed.
		return 0;
	}

	if( sd->expiration_tid != INVALID_TIMER )
		timer->delete(sd->expiration_tid,pc->expiration_timer);

	if (sd->npc_timer_id != INVALID_TIMER) //Cancel the event timer.
		npc->timerevent_quit(sd);

	if (sd->npc_id)
		npc->event_dequeue(sd);

	if( sd->bg_id && !sd->bg_queue.arena ) /* TODO: dump this chunk after bg_queue is fully enabled */
		bg->team_leave(sd,BGTL_QUIT);

	if (sd->status.clan_id)
	  clan->member_offline(sd);

	if (sd->state.autotrade && core->runflag != MAPSERVER_ST_SHUTDOWN && !channel->config->closing)
		pc->autotrade_update(sd,PAUC_REMOVE);

	skill->cooldown_save(sd);
	pc->itemcd_do(sd,false);

	for (i = 0; i < VECTOR_LENGTH(sd->script_queues); i++) {
		struct script_queue *queue = script->queue(VECTOR_INDEX(sd->script_queues, i));
		if (queue && queue->event_logout[0] != '\0') {
			npc->event(sd, queue->event_logout, 0);
		}
	}
	/* two times, the npc event above may assign a new one or delete others */
	while (VECTOR_LENGTH(sd->script_queues)) {
		int qid = VECTOR_LAST(sd->script_queues);
		script->queue_remove(qid, sd->status.account_id);
	}

	npc->script_event(sd, NPCE_LOGOUT);
	rodex->clean(sd, 0);
	goldpc->stop(sd);

	//Unit_free handles clearing the player related data,
	//map->quit handles extra specific data which is related to quitting normally
	//(changing map-servers invokes unit_free but bypasses map->quit)
	if( sd->sc.count ) {
		//Status that are not saved...
		for(i=0; i < SC_MAX; i++){
			if ( status->get_sc_type(i)&SC_NO_SAVE ) {
				if ( !sd->sc.data[i] )
					continue;
				switch( i ){
					case SC_ENDURE:
					case SC_GDSKILL_REGENERATION:
						if( !sd->sc.data[i]->val4 )
							break;
						FALLTHROUGH
					default:
						status_change_end(&sd->bl, (sc_type)i, INVALID_TIMER);
				}
			}
		}
	}

	for( i = 0; i < EQI_MAX; i++ ) {
		if( sd->equip_index[ i ] >= 0 )
			if( !pc->isequip( sd , sd->equip_index[ i ] ) )
				pc->unequipitem(sd, sd->equip_index[i], PCUNEQUIPITEM_FORCE);
	}

	// Return loot to owner
	if( sd->pd ) pet->lootitem_drop(sd->pd, sd);

	if( sd->state.storage_flag == STORAGE_FLAG_NORMAL ) sd->state.storage_flag = STORAGE_FLAG_CLOSED; // No need to Double Save Storage on Quit.

	if( sd->ed ) {
		elemental->clean_effect(sd->ed);
		unit->remove_map(&sd->ed->bl,CLR_TELEPORT,ALC_MARK);
	}

	channel->quit(sd);

	unit->remove_map_pc(sd,CLR_RESPAWN);

	if( map->list[sd->bl.m].instance_id >= 0 ) { // Avoid map conflicts and warnings on next login
		int16 m;
		struct point *pt;
		if( map->list[sd->bl.m].save.map )
			pt = &map->list[sd->bl.m].save;
		else
			pt = &sd->status.save_point;

		if( (m=map->mapindex2mapid(pt->map)) >= 0 ) {
			sd->bl.m = m;
			sd->bl.x = pt->x;
			sd->bl.y = pt->y;
			sd->mapindex = pt->map;
		}
	}

	if( sd->state.vending ) {
		idb_remove(vending->db, sd->status.char_id);
	}

	party->booking_delete(sd); // Party Booking [Spiria]
	pc->makesavestatus(sd);
	pc->clean_skilltree(sd);
	pc->crimson_marker_clear(sd);
	macro->detector_disconnect(sd);
	chrif->save(sd,1);
	unit->free_pc(sd);
	return 0;
}

/**
 * Looks up a session data by ID.
 *
 * The search is performed using the pc_db.
 *
 * @param id The bl ID to search.
 * @return The searched map_session_data, if it exists.
 * @retval NULL if the ID is invalid or doesn't belong to a player unit.
 */
static struct map_session_data *map_id2sd(int id)
{
	struct block_list *bl = NULL;
	if (id <= 0)
		return NULL;

	bl = idb_get(map->pc_db,id);

	if (bl)
		Assert_retr(NULL, bl->type == BL_PC);
	return BL_UCAST(BL_PC, bl);
}

/**
 * Looks up a NPC data by ID.
 *
 * @param id The bl ID to search.
 * @return The searched npc_data, if it exists.
 * @retval NULL if the ID is invalid or doesn't belong to a NPC.
 */
static struct npc_data *map_id2nd(int id)
{
	// just a id2bl lookup because there's no npc_db
	struct block_list* bl = map->id2bl(id);

	return BL_CAST(BL_NPC, bl);
}

/**
 * Looks up a mob data by ID.
 *
 * The search is performed using the mobid_db.
 *
 * @param id The bl ID to search.
 * @return The searched mob_data, if it exists.
 * @retval NULL if the ID is invalid or doesn't belong to a mob unit.
 */
static struct mob_data *map_id2md(int id)
{
	struct block_list *bl = NULL;
	if (id <= 0)
		return NULL;

	bl = idb_get(map->mobid_db,id);

	if (bl)
		Assert_retr(NULL, bl->type == BL_MOB);
	return BL_UCAST(BL_MOB, bl);
}

/**
 * Looks up a floor item data by ID.
 *
 * @param id The bl ID to search.
 * @return The searched flooritem_data, if it exists.
 * @retval NULL if the ID is invalid or doesn't belong to a floor item.
 */
static struct flooritem_data *map_id2fi(int id)
{
	struct block_list* bl = map->id2bl(id);

	return BL_CAST(BL_ITEM, bl);
}

/**
 * Looks up a chat data by ID.
 *
 * @param id The bl ID to search.
 * @return The searched chat_data, if it exists.
 * @retval NULL if the ID is invalid or doesn't belong to a chat.
 */
static struct chat_data *map_id2cd(int id)
{
	struct block_list* bl = map->id2bl(id);

	return BL_CAST(BL_CHAT, bl);
}

/**
 * Looks up a skill unit data by ID.
 *
 * @param id The bl ID to search.
 * @return The searched skill_unit data, if it exists.
 * @retval NULL if the ID is invalid or doesn't belong to a skill unit.
 */
static struct skill_unit *map_id2su(int id)
{
	struct block_list* bl = map->id2bl(id);

	return BL_CAST(BL_SKILL, bl);
}

/**
 * Looks up a pet data by ID.
 *
 * @param id The bl ID to search.
 * @return The searched pet_data, if it exists.
 * @retval NULL if the ID is invalid or doesn't belong to a pet.
 */
static struct pet_data *map_id2pd(int id)
{
	struct block_list* bl = map->id2bl(id);

	return BL_CAST(BL_PET, bl);
}

/**
 * Looks up a homunculus data by ID.
 *
 * @param id The bl ID to search.
 * @return The searched homun_data, if it exists.
 * @retval NULL if the ID is invalid or doesn't belong to a homunculus.
 */
static struct homun_data *map_id2hd(int id)
{
	struct block_list* bl = map->id2bl(id);

	return BL_CAST(BL_HOM, bl);
}

/**
 * Looks up a mercenary data by ID.
 *
 * @param id The bl ID to search.
 * @return The searched mercenary_data, if it exists.
 * @retval NULL if the ID is invalid or doesn't belong to a mercenary.
 */
static struct mercenary_data *map_id2mc(int id)
{
	struct block_list* bl = map->id2bl(id);

	return BL_CAST(BL_MER, bl);
}

/**
 * Looks up an elemental data by ID.
 *
 * @param id The bl ID to search.
 * @return The searched elemental_data, if it exists.
 * @retval NULL if the ID is invalid or doesn't belong to an elemental.
 */
static struct elemental_data *map_id2ed(int id)
{
	struct block_list* bl = map->id2bl(id);

	return BL_CAST(BL_ELEM, bl);
}

/**
 * Looks up a block_list by ID.
 *
 * The search is performed using the id_db.
 *
 * @param id The bl ID to search.
 * @return The searched block_list, if it exists.
 * @retval NULL if the ID is invalid.
 */
static struct block_list *map_id2bl(int id)
{
	return idb_get(map->id_db, id);
}

/**
 * Verifies whether a block list ID is valid.
 *
 * @param id The bl ID to search.
 * @retval true if the ID exists and is valid.
 * @retval false otherwise.
 */
static bool map_blid_exists(int id)
{
	return (idb_exists(map->id_db,id));
}

/// Returns the nick of the target charid or NULL if unknown (requests the nick to the char server).
static const char *map_charid2nick(int charid)
{
	struct charid2nick *p;
	struct map_session_data* sd;

	sd = map->charid2sd(charid);
	if( sd )
		return sd->status.name;// character is online, return it's name

	p = idb_ensure(map->nick_db, charid, map->create_charid2nick);
	if( *p->nick )
		return p->nick;// name in nick_db

	chrif->searchcharid(charid);// request the name
	return NULL;
}

/// Returns the struct map_session_data of the charid or NULL if the char is not online.
static struct map_session_data *map_charid2sd(int charid)
{
	struct block_list *bl = idb_get(map->charid_db, charid);
	if (bl)
		Assert_retr(NULL, bl->type == BL_PC);
	return BL_UCAST(BL_PC, bl);
}

/*==========================================
 * Search session data from a nick name
 * (without sensitive case if necessary)
 * return map_session_data pointer or NULL
 *------------------------------------------*/
static struct map_session_data *map_nick2sd(const char *nick, bool allow_partial)
{
	if (nick == NULL)
		return NULL;

	struct s_mapiterator *iter = mapit_getallusers();
	struct map_session_data *found_sd = NULL;

	if (battle_config.partial_name_scan && allow_partial) {
		int nicklen = (int)strlen(nick);
		int qty = 0;

		// partial name search
		for (struct map_session_data *sd = BL_UCAST(BL_PC, mapit->first(iter)); mapit->exists(iter); sd = BL_UCAST(BL_PC, mapit->next(iter))) {
			if (strnicmp(sd->status.name, nick, nicklen) == 0) {
				found_sd = sd;

				if (strcmp(sd->status.name, nick) == 0) {
					// Perfect Match
					qty = 1;
					break;
				}

				qty++;
			}
		}

		if (qty != 1)
			found_sd = NULL;
	} else {
		// exact search only
		for (struct map_session_data *sd = BL_UCAST(BL_PC, mapit->first(iter)); mapit->exists(iter); sd = BL_UCAST(BL_PC, mapit->next(iter))) {
			if (strcasecmp(sd->status.name, nick) == 0) {
				found_sd = sd;
				break;
			}
		}
	}
	mapit->free(iter);

	return found_sd;
}

/*==========================================
 * Convext Mirror
 *------------------------------------------*/
static struct mob_data *map_getmob_boss(int16 m)
{
	struct DBIterator *iter;
	struct mob_data *md = NULL;
	bool found = false;

	iter = db_iterator(map->bossid_db);
	for (md = dbi_first(iter); dbi_exists(iter); md = dbi_next(iter)) {
		if (md->bl.m == m) {
			found = true;
			break;
		}
	}
	dbi_destroy(iter);

	return (found)? md : NULL;
}

static struct mob_data *map_id2boss(int id)
{
	struct block_list *bl = NULL;
	if (id <= 0)
		return NULL;
	bl = idb_get(map->bossid_db,id);
	if (bl)
		Assert_retr(NULL, bl->type == BL_MOB);
	return BL_UCAST(BL_MOB, bl);
}

/**
 * Returns the equivalent bitmask to the given race ID.
 *
 * @param race A race identifier (@see enum Race)
 *
 * @return The equivalent race bitmask.
 */
static uint32 map_race_id2mask(int race)
{
	if (race >= RC_FORMLESS && race < RC_MAX)
		return 1 << race;

	if (race == RC_ALL)
		return RCMASK_ALL;

	if (race == RC_NONPLAYER)
		return RCMASK_NONPLAYER;

	if (race == RC_NONDEMIHUMAN)
		return RCMASK_NONDEMIHUMAN;

	if (race == RC_DEMIPLAYER)
		return RCMASK_DEMIPLAYER;

	if (race == RC_NONDEMIPLAYER)
		return RCMASK_NONDEMIPLAYER;

	ShowWarning("map_race_id2mask: Invalid race: %d\n", race);
	Assert_report((race >= RC_FORMLESS && race < RC_NONDEMIPLAYER) || race == RC_ALL);

	return RCMASK_NONE;
}

/// Applies func to all the players in the db.
/// Stops iterating if func returns -1.
static void map_vforeachpc(int (*func)(struct map_session_data *sd, va_list args), va_list args)
{
	struct DBIterator *iter = db_iterator(map->pc_db);
	struct map_session_data *sd = NULL;

	for( sd = dbi_first(iter); dbi_exists(iter); sd = dbi_next(iter) )
	{
		va_list argscopy;
		int ret;

		va_copy(argscopy, args);
		ret = func(sd, argscopy);
		va_end(argscopy);
		if( ret == -1 )
			break;// stop iterating
	}
	dbi_destroy(iter);
}

/// Applies func to all the players in the db.
/// Stops iterating if func returns -1.
/// @see map_vforeachpc
static void map_foreachpc(int (*func)(struct map_session_data *sd, va_list args), ...)
{
	va_list args;

	va_start(args, func);
	map->vforeachpc(func, args);
	va_end(args);
}

/// Applies func to all the mobs in the db.
/// Stops iterating if func returns -1.
static void map_vforeachmob(int (*func)(struct mob_data *md, va_list args), va_list args)
{
	struct DBIterator *iter = db_iterator(map->mobid_db);
	struct mob_data *md = NULL;

	for (md = dbi_first(iter); dbi_exists(iter); md = dbi_next(iter)) {
		va_list argscopy;
		int ret;

		va_copy(argscopy, args);
		ret = func(md, argscopy);
		va_end(argscopy);
		if( ret == -1 )
			break;// stop iterating
	}
	dbi_destroy(iter);
}

/// Applies func to all the mobs in the db.
/// Stops iterating if func returns -1.
/// @see map_vforeachmob
static void map_foreachmob(int (*func)(struct mob_data *md, va_list args), ...)
{
	va_list args;

	va_start(args, func);
	map->vforeachmob(func, args);
	va_end(args);
}

/// Applies func to all the npcs in the db.
/// Stops iterating if func returns -1.
static void map_vforeachnpc(int (*func)(struct npc_data *nd, va_list args), va_list args)
{
	struct DBIterator *iter = db_iterator(map->id_db);
	struct block_list *bl = NULL;

	for (bl = dbi_first(iter); dbi_exists(iter); bl = dbi_next(iter)) {
		if (bl->type == BL_NPC) {
			struct npc_data *nd = BL_UCAST(BL_NPC, bl);
			va_list argscopy;
			int ret;

			va_copy(argscopy, args);
			ret = func(nd, argscopy);
			va_end(argscopy);
			if( ret == -1 )
				break;// stop iterating
		}
	}
	dbi_destroy(iter);
}

/// Applies func to all the npcs in the db.
/// Stops iterating if func returns -1.
/// @see map_vforeachnpc
static void map_foreachnpc(int (*func)(struct npc_data *nd, va_list args), ...)
{
	va_list args;

	va_start(args, func);
	map->vforeachnpc(func, args);
	va_end(args);
}

/// Applies func to everything in the db.
/// Stops iterating gif func returns -1.
static void map_vforeachregen(int (*func)(struct block_list *bl, va_list args), va_list args)
{
	struct DBIterator *iter = db_iterator(map->regen_db);
	struct block_list *bl = NULL;

	for (bl = dbi_first(iter); dbi_exists(iter); bl = dbi_next(iter)) {
		va_list argscopy;
		int ret;

		va_copy(argscopy, args);
		ret = func(bl, argscopy);
		va_end(argscopy);
		if( ret == -1 )
			break;// stop iterating
	}
	dbi_destroy(iter);
}

/// Applies func to everything in the db.
/// Stops iterating gif func returns -1.
/// @see map_vforeachregen
static void map_foreachregen(int (*func)(struct block_list *bl, va_list args), ...)
{
	va_list args;

	va_start(args, func);
	map->vforeachregen(func, args);
	va_end(args);
}

/// Applies func to everything in the db.
/// Stops iterating if func returns -1.
static void map_vforeachiddb(int (*func)(struct block_list *bl, va_list args), va_list args)
{
	struct DBIterator *iter = db_iterator(map->id_db);
	struct block_list *bl = NULL;

	for (bl = dbi_first(iter); dbi_exists(iter); bl = dbi_next(iter)) {
		va_list argscopy;
		int ret;

		va_copy(argscopy, args);
		ret = func(bl, argscopy);
		va_end(argscopy);
		if( ret == -1 )
			break;// stop iterating
	}
	dbi_destroy(iter);
}

/// Applies func to everything in the db.
/// Stops iterating if func returns -1.
/// @see map_vforeachiddb
static void map_foreachiddb(int (*func)(struct block_list *bl, va_list args), ...)
{
	va_list args;

	va_start(args, func);
	map->vforeachiddb(func, args);
	va_end(args);
}

/// Iterator.
/// Can filter by bl type.
struct s_mapiterator {
	enum e_mapitflags flags; ///< flags for special behaviour
	enum bl_type types;      ///< what bl types to return
	struct DBIterator *dbi;  ///< database iterator
};

/// Returns true if the block_list matches the description in the iterator.
///
/// @param _mapit_ Iterator
/// @param _bl_ block_list
/// @return true if it matches
#define MAPIT_MATCHES(_mapit_,_bl_) \
	( (_bl_)->type & (_mapit_)->types /* type matches */ )

/// Allocates a new iterator.
/// Returns the new iterator.
/// types can represent several BL's as a bit field.
/// TODO should this be expanded to allow filtering of map/guild/party/chat/cell/area/...?
///
/// @param flags Flags of the iterator
/// @param type Target types
/// @return Iterator
static struct s_mapiterator *mapit_alloc(enum e_mapitflags flags, enum bl_type types)
{
	struct s_mapiterator* iter;

	iter = ers_alloc(map->iterator_ers, struct s_mapiterator);
	iter->flags = flags;
	iter->types = types;
	if( types == BL_PC )       iter->dbi = db_iterator(map->pc_db);
	else if( types == BL_MOB ) iter->dbi = db_iterator(map->mobid_db);
	else                       iter->dbi = db_iterator(map->id_db);
	return iter;
}

/// Frees the iterator.
///
/// @param iter Iterator
static void mapit_free(struct s_mapiterator *iter)
{
	nullpo_retv(iter);

	dbi_destroy(iter->dbi);
	ers_free(map->iterator_ers, iter);
}

/// Returns the first block_list that matches the description.
/// Returns NULL if not found.
///
/// @param iter Iterator
/// @return first block_list or NULL
static struct block_list *mapit_first(struct s_mapiterator *iter)
{
	struct block_list* bl;

	nullpo_retr(NULL,iter);

	for (bl = dbi_first(iter->dbi); bl != NULL; bl = dbi_next(iter->dbi) ) {
		if( MAPIT_MATCHES(iter,bl) )
			break;// found match
	}
	return bl;
}

/// Returns the last block_list that matches the description.
/// Returns NULL if not found.
///
/// @param iter Iterator
/// @return last block_list or NULL
static struct block_list *mapit_last(struct s_mapiterator *iter)
{
	struct block_list* bl;

	nullpo_retr(NULL,iter);

	for (bl = dbi_last(iter->dbi); bl != NULL; bl = dbi_prev(iter->dbi)) {
		if( MAPIT_MATCHES(iter,bl) )
			break;// found match
	}
	return bl;
}

/// Returns the next block_list that matches the description.
/// Returns NULL if not found.
///
/// @param iter Iterator
/// @return next block_list or NULL
static struct block_list *mapit_next(struct s_mapiterator *iter)
{
	struct block_list* bl;

	nullpo_retr(NULL,iter);

	for( ; ; ) {
		bl = dbi_next(iter->dbi);
		if( bl == NULL )
			break;// end
		if( MAPIT_MATCHES(iter,bl) )
			break;// found a match
		// try next
	}
	return bl;
}

/// Returns the previous block_list that matches the description.
/// Returns NULL if not found.
///
/// @param iter Iterator
/// @return previous block_list or NULL
static struct block_list *mapit_prev(struct s_mapiterator *iter)
{
	struct block_list* bl;

	nullpo_retr(NULL,iter);

	for( ; ; ) {
		bl = dbi_prev(iter->dbi);
		if( bl == NULL )
			break;// end
		if( MAPIT_MATCHES(iter,bl) )
			break;// found a match
		// try prev
	}
	return bl;
}

/// Returns true if the current block_list exists in the database.
///
/// @param iter Iterator
/// @return true if it exists
static bool mapit_exists(struct s_mapiterator *iter)
{
	nullpo_retr(false,iter);

	return dbi_exists(iter->dbi);
}

/*==========================================
 * Add npc-bl to id_db, basically register npc to map
 *------------------------------------------*/
static bool map_addnpc(int16 m, struct npc_data *nd)
{
	nullpo_ret(nd);

	if( m < 0 || m >= map->count )
		return false;

	if( map->list[m].npc_num == MAX_NPC_PER_MAP ) {
		ShowWarning("too many NPCs in one map %s\n",map->list[m].name);
		return false;
	}

	map->list[m].npc[map->list[m].npc_num]=nd;
	map->list[m].npc_num++;
	idb_put(map->id_db,nd->bl.id,nd);
	return true;
}

/*=========================================
 * Dynamic Mobs [Wizputer]
 *-----------------------------------------*/
// Stores the spawn data entry in the mob list.
// Returns the index of successful, or -1 if the list was full.
static int map_addmobtolist(unsigned short m, struct spawn_data *spawn)
{
	int i;
	nullpo_retr(-1, spawn);
	ARR_FIND( 0, MAX_MOB_LIST_PER_MAP, i, map->list[m].moblist[i] == NULL );
	if( i < MAX_MOB_LIST_PER_MAP ) {
		map->list[m].moblist[i] = spawn;
		return i;
	}
	return -1;
}

static void map_spawnmobs(int16 m)
{
	int i, k=0;
	if (map->list[m].mob_delete_timer != INVALID_TIMER) {
		//Mobs have not been removed yet [Skotlex]
		timer->delete(map->list[m].mob_delete_timer, map->removemobs_timer);
		map->list[m].mob_delete_timer = INVALID_TIMER;
		return;
	}
	for(i=0; i<MAX_MOB_LIST_PER_MAP; i++)
		if(map->list[m].moblist[i]!=NULL) {
			k+=map->list[m].moblist[i]->num;
			npc->parse_mob2(map->list[m].moblist[i]);
		}

	if (battle_config.etc_log && k > 0) {
		ShowStatus("Map %s: Spawned '"CL_WHITE"%d"CL_RESET"' mobs.\n",map->list[m].name, k);
	}
}

static int map_removemobs_sub(struct block_list *bl, va_list ap)
{
	struct mob_data *md = NULL;
	nullpo_ret(bl);
	Assert_ret(bl->type == BL_MOB);
	md = BL_UCAST(BL_MOB, bl);

	//When not to remove mob:
	// doesn't respawn and is not a slave
	if( !md->spawn && !md->master_id )
		return 0;
	// respawn data is not in cache
	if( md->spawn && !md->spawn->state.dynamic )
		return 0;
	// hasn't spawned yet
	if( md->spawn_timer != INVALID_TIMER )
		return 0;
	// is damaged and mob_remove_damaged is off
	if( !battle_config.mob_remove_damaged && md->status.hp < md->status.max_hp )
		return 0;
	// is a mvp
	if( md->db->mexp > 0 )
		return 0;

	unit->free(&md->bl,CLR_OUTSIGHT);

	return 1;
}

static int map_removemobs_timer(int tid, int64 tick, int id, intptr_t data)
{
	int count;
	const int16 m = id;

	if (m < 0 || m >= map->count) { //Incorrect map id!
		ShowError("map_removemobs_timer error: timer %d points to invalid map %d\n",tid, m);
		return 0;
	}
	if (map->list[m].mob_delete_timer != tid) { //Incorrect timer call!
		ShowError("map_removemobs_timer mismatch: %d != %d (map %s)\n",map->list[m].mob_delete_timer, tid, map->list[m].name);
		return 0;
	}
	map->list[m].mob_delete_timer = INVALID_TIMER;
	if (map->list[m].users > 0) //Map not empty!
		return 1;

	count = map->foreachinmap(map->removemobs_sub, m, BL_MOB);

	if (battle_config.etc_log && count > 0)
		ShowStatus("Map %s: Removed '"CL_WHITE"%d"CL_RESET"' mobs.\n",map->list[m].name, count);

	return 1;
}

static void map_removemobs(int16 m)
{
	Assert_retv(m >= 0 && m < map->count);
	if (map->list[m].mob_delete_timer != INVALID_TIMER) // should never happen
		return; //Mobs are already scheduled for removal

	map->list[m].mob_delete_timer = timer->add(timer->gettick()+battle_config.mob_remove_delay, map->removemobs_timer, m, 0);
}

/*==========================================
 * Hookup, get map_id from map_name
 *------------------------------------------*/
static int16 map_mapname2mapid(const char *name)
{
	unsigned short map_index;
	map_index = mapindex->name2id(name);
	if (!map_index)
		return -1;
	return map->mapindex2mapid(map_index);
}

/*==========================================
 * Returns the map of the given mapindex. [Skotlex]
 *------------------------------------------*/
static int16 map_mapindex2mapid(unsigned short map_index)
{

	if (!map_index || map_index >= MAX_MAPINDEX)
		return -1;

	return map->index2mapid[map_index];
}

/**
 * Checks if both dirs point in the same direction.
 * @param s_dir: direction source is facing
 * @param t_dir: direction target is facing
 * @return 0: success(both face the same direction), 1: failure
 **/
static int map_check_dir(enum unit_dir s_dir, enum unit_dir t_dir)
{
	if (s_dir == t_dir || ((t_dir + UNIT_DIR_MAX - 1) % UNIT_DIR_MAX) == s_dir
	    || ((t_dir + UNIT_DIR_MAX + 1) % UNIT_DIR_MAX) == s_dir)
		return 0;
	return 1;
}

/**
 * Returns the direction of the given cell, relative to 'src'
 * @param src: object to put in relation between coordinates
 * @param x: x-coordinate of cell
 * @param y: y-coordinate of cell
 * @return the direction of the given cell, relative to 'src'
 **/
static enum unit_dir map_calc_dir(const struct block_list *src, int16 x, int16 y)
{
	nullpo_retr(UNIT_DIR_NORTH, src);
	enum unit_dir dir = UNIT_DIR_NORTH;

	int dx = x - src->x;
	int dy = y - src->y;
	if (dx == 0 && dy == 0) {
		// both are standing on the same spot.
		// aegis-style, makes knockback default to the left.
		// athena-style, makes knockback default to behind 'src'.
		if (battle_config.knockback_left != 0)
			dir = UNIT_DIR_EAST;
		else
			dir = unit->getdir(src);
	} else if (dx >= 0 && dy >= 0) {
		if (dx * 2 < dy || dx == 0)
			dir = UNIT_DIR_NORTH;
		else if (dx > dy * 2 + 1 || dy == 0)
			dir = UNIT_DIR_EAST;
		else
			dir = UNIT_DIR_NORTHEAST;
	} else if (dx >= 0 && dy <= 0) {
		if (dx * 2 < -dy || dx == 0)
			dir = UNIT_DIR_SOUTH;
		else if (dx > -dy * 2 + 1 || dy == 0)
			dir = UNIT_DIR_EAST;
		else
			dir = UNIT_DIR_SOUTHEAST;
	} else if (dx <= 0 && dy <= 0) {
		if (dx * 2 > dy || dx == 0 )
			dir = UNIT_DIR_SOUTH;
		else if (dx < dy * 2 + 1 || dy == 0)
			dir = UNIT_DIR_WEST;
		else
			dir = UNIT_DIR_SOUTHWEST;
	} else {
		if (-dx * 2 < dy || dx == 0 )
			dir = UNIT_DIR_NORTH;
		else if (-dx > dy * 2 + 1 || dy == 0)
			dir = UNIT_DIR_WEST;
		else
			dir = UNIT_DIR_NORTHWEST;
	}
	return dir;
}

/**
 * Randomizes (x, y) to a walkable cell that is at least min_dist away on a square and less than max_dist.
 *
 * __Only__ tries each quarter of the square once, doesn't try all possibilities. (Aegis behavior)
 * The square has a width and height of 2 * max_dist - 1.
 * The smaller square with (x,y) center has a width and height of 2 * min_dist - 1,
 * and is not included in the returning random cells.
 *
 * @remark e.g. (x + x_range, y + y_range) is not part of the square and won't be randomized.
 * @param[in] bl optional object to base walkable checks on.
 * @param[in] m map ID for the walkable cell checks
 * @param[in,out] x pointer which contains the x-axis center value of the square
 * @param[in,out] y pointer which contains the y-axis center value of the square
 * @param[in] min_dist minimum distance of cells from (x, y)
 * @param[in] max_dist maximum distance of cells from (x, y)
 * @retval 0 success, random walkable cell found
 * @retval 1 failure, no cell found, x and y remain unchanged
 * @retval 2 failure, x or y nullpointer.
 */
static int map_get_random_cell(struct block_list *bl, int16 m, int16 *x, int16 *y, int16 min_dist, int16 max_dist)
{
	nullpo_retr(2, x);
	nullpo_retr(2, y);
	enum unit_dir dir = unit_get_rnd_diagonal_dir();

	for (int i = 0; i < 4; i++, dir = unit_get_ccw90_dir(dir)) {
		int16 x_rnd_dist = (min_dist + rnd()) % max(1, max_dist);
		int16 y_rnd_dist = (min_dist + rnd()) % max(1, max_dist);
		int16 x_rnd = *x + dirx[dir] * x_rnd_dist;
		int16 y_rnd = *y + diry[dir] * y_rnd_dist;

		// cell walkable?
		if (map->getcell(m, bl, x_rnd, y_rnd, CELL_CHKNOPASS) != 0)
			continue;
		if (!path->search(NULL, bl, m, *x, *y, x_rnd, y_rnd, 1, CELL_CHKNOREACH))
			continue;

		*x = x_rnd;
		*y = y_rnd;
		return 0;
	}

	return 1;
}

/**
 * Randomizes (x, y) to a walkable cell on a rectangle with (x, y) being the center cell.
 *
 * __Only__ tries each quarter of the rectangle once, doesn't try all possibilities. (Aegis behavior)
 * The rectangle has a width of 2 * x_range - 1 and a height of 2 * y_range - 1 .
 *
 * @remark e.g. (x + x_range, y + y_range) is not part of the rectangle and won't be randomized.
 * @param[in] bl optional object to base walkable checks on.
 * @param[in] m map ID for the walkable cell checks
 * @param[in,out] x pointer which contains the x-axis center value of the rectangle
 * @param[in,out] y pointer which contains the y-axis center value of the rectangle
 * @param[in] x_range horizontal distance from center to border
 * @param[in] y_range vertical distance from center to border
 * @retval 0 success, random walkable cell found
 * @retval 1 failure, no cell found, x and y remain unchanged
 * @retval 2 failure, x or y nullpointer.
 */
static int map_get_random_cell_in_range(struct block_list *bl, int16 m, int16 *x, int16 *y, int16 x_range, int16 y_range)
{
	nullpo_retr(2, x);
	nullpo_retr(2, y);
	enum unit_dir dir = unit_get_rnd_diagonal_dir();

	for (int i = 0; i < 4; i++, dir = unit_get_ccw90_dir(dir)) {
		int16 x_rnd_range = rnd() % max(1, x_range);
		int16 y_rnd_range = rnd() % max(1, y_range);
		int16 x_rnd = *x + dirx[dir] * x_rnd_range;
		int16 y_rnd = *y + diry[dir] * y_rnd_range;

		// cell walkable?
		if (map->getcell(m, bl, x_rnd, y_rnd, CELL_CHKNOPASS) != 0)
			continue;
		if (!path->search(NULL, bl, m, *x, *y, x_rnd, y_rnd, 1, CELL_CHKNOREACH))
			continue;

		*x = x_rnd;
		*y = y_rnd;
		return 0;
	}

	return 1;
}

/**
 * Randomizes target cell x, y to a random walkable cell that
 * has the same distance from bl on a circle as given coordinates do.
 *
 * @warning this function has gaps, especially on the west and east side in relation to @p bl
 * @param bl object to which we keep the same distance after randomizing the giving cells
 * @param[in,out] x x-axis pointer of cell for distance to @p bl
 * @param[in,out] y y-axis pointer of cell for distance to @p bl
 * @retval 0 failure to randomize coordinates, x and y won't be changed
 * @retval 1 success
 */
static int map_random_dir(struct block_list *bl, int16 *x, int16 *y)
{
	nullpo_ret(bl);
	nullpo_ret(x);
	nullpo_ret(y);
	int16 xi = *x - bl->x;
	int16 yi = *y - bl->y;
	if (xi == 0 && yi == 0) {
		// No distance between points, go with distance 1 instead to prevent NaN in second sqrt
		xi = 1;
		yi = 1;
	}
	int dist2 = xi * xi + yi * yi;
	int16 dist = (int16)sqrt(dist2);

	int16 i = 0;
	do {
		enum unit_dir dir = unit_get_rnd_diagonal_dir();
		int16 segment = 1 + (rnd() % dist); // Pick a random interval from the whole vector in that direction
		xi = bl->x + segment * dirx[dir];
		segment = (int16)sqrt(dist2 - segment * segment); // The complement of the previously picked segment
		yi = bl->y + segment * diry[dir];
	} while ((map->getcell(bl->m, bl, xi, yi, CELL_CHKNOPASS) || !path->search(NULL, bl, bl->m, bl->x, bl->y, xi, yi, 1, CELL_CHKNOREACH))
	         && (++i) < 100);

	if (i < 100) {
		*x = xi;
		*y = yi;
		return 1;
	}
	return 0;
}

// gat system
static struct mapcell map_gat2cell(int gat)
{
	struct mapcell cell;

	memset(&cell,0,sizeof(struct mapcell));

	switch( gat ) {
		case 0: cell.walkable = 1; cell.shootable = 1; cell.water = 0; break; // walkable ground
		case 1: cell.walkable = 0; cell.shootable = 0; cell.water = 0; break; // non-walkable ground
		case 2: cell.walkable = 1; cell.shootable = 1; cell.water = 0; break; // ???
		case 3: cell.walkable = 1; cell.shootable = 1; cell.water = 1; break; // walkable water
		case 4: cell.walkable = 1; cell.shootable = 1; cell.water = 0; break; // ???
		case 5: cell.walkable = 0; cell.shootable = 1; cell.water = 0; break; // gap (snipable)
		case 6: cell.walkable = 1; cell.shootable = 1; cell.water = 0; break; // ???
	default:
		ShowWarning("map_gat2cell: unrecognized gat type '%d'\n", gat);
		break;
	}

	return cell;
}

static int map_cell2gat(struct mapcell cell)
{
	if( cell.walkable == 1 && cell.shootable == 1 && cell.water == 0 ) return 0;
	if( cell.walkable == 0 && cell.shootable == 0 && cell.water == 0 ) return 1;
	if( cell.walkable == 1 && cell.shootable == 1 && cell.water == 1 ) return 3;
	if( cell.walkable == 0 && cell.shootable == 1 && cell.water == 0 ) return 5;

	ShowWarning("map_cell2gat: cell has no matching gat type\n");
	return 1; // default to 'wall'
}

/**
 * Extracts a map's cell data from its compressed mapcache.
 *
 * @param[in, out] m The target map.
 */
static void map_cellfromcache(struct map_data *m)
{
	nullpo_retv(m);

	if (m->cell_buf.data != NULL) {
		char decode_buffer[MAX_MAP_SIZE];
		unsigned long size, xy;
		int i;

		size = (unsigned long)m->xs * (unsigned long)m->ys;

		// TO-DO: Maybe handle the scenario, if the decoded buffer isn't the same size as expected? [Shinryo]
		grfio->decode_zip(decode_buffer, &size, m->cell_buf.data, m->cell_buf.len);

		CREATE(m->cell, struct mapcell, size);

		// Set cell properties
		for( xy = 0; xy < size; ++xy ) {
			m->cell[xy] = map->gat2cell(decode_buffer[xy]);
		}

		m->getcellp = map->getcellp;
		m->setcell  = map->setcell;

		for(i = 0; i < m->npc_num; i++) {
			npc->setcells(m->npc[i]);
		}
	}
}

/*==========================================
 * Confirm if celltype in (m,x,y) match the one given in cellchk
 *------------------------------------------*/
static int map_getcell(int16 m, const struct block_list *bl, int16 x, int16 y, cell_chk cellchk)
{
	return (m < 0 || m >= map->count) ? 0 : map->list[m].getcellp(&map->list[m], bl, x, y, cellchk);
}

static int map_getcellp(struct map_data *m, const struct block_list *bl, int16 x, int16 y, cell_chk cellchk)
{
	struct mapcell cell;

	nullpo_ret(m);

	//NOTE: this intentionally overrides the last row and column
	if(x<0 || x>=m->xs-1 || y<0 || y>=m->ys-1)
		return( cellchk == CELL_CHKNOPASS );

	cell = m->cell[x + y*m->xs];

	switch(cellchk) {
		// gat type retrieval
	case CELL_GETTYPE:
		return map->cell2gat(cell);

		// base gat type checks
	case CELL_CHKWALL:
		return (!cell.walkable && !cell.shootable);
	case CELL_CHKWATER:
		return (cell.water);
	case CELL_CHKCLIFF:
		return (!cell.walkable && cell.shootable);

		// base cell type checks
	case CELL_CHKNPC:
		return (cell.npc);
	case CELL_CHKBASILICA:
		return (cell.basilica);
	case CELL_CHKLANDPROTECTOR:
		return (cell.landprotector);
	case CELL_CHKNOVENDING:
		return (cell.novending);
	case CELL_CHKNOCHAT:
		return (cell.nochat);
	case CELL_CHKICEWALL:
		return (cell.icewall);
	case CELL_CHKNOICEWALL:
		return (cell.noicewall);
	case CELL_CHKNOSKILL:
		return (cell.noskill);

		// special checks
	case CELL_CHKPASS:
#ifdef CELL_NOSTACK
		if (cell.cell_bl >= battle_config.custom_cell_stack_limit)
			return 0;
		FALLTHROUGH
#endif
	case CELL_CHKREACH:
		return (cell.walkable);

	case CELL_CHKNOPASS:
#ifdef CELL_NOSTACK
		if (cell.cell_bl >= battle_config.custom_cell_stack_limit)
			return 1;
		FALLTHROUGH
#endif
	case CELL_CHKNOREACH:
		return (!cell.walkable);

	case CELL_CHKSTACK:
#ifdef CELL_NOSTACK
		return (cell.cell_bl >= battle_config.custom_cell_stack_limit);
#else
		return 0;
#endif

	default:
		return 0;
	}
}

/* [Ind/Hercules] */
static int map_sub_getcellp(struct map_data *m, const struct block_list *bl, int16 x, int16 y, cell_chk cellchk)
{
	nullpo_ret(m);
	map->cellfromcache(m);
	m->getcellp = map->getcellp;
	m->setcell  = map->setcell;
	return m->getcellp(m, bl, x, y, cellchk);
}

/*==========================================
 * Change the type/flags of a map cell
 * 'cell' - which flag to modify
 * 'flag' - true = on, false = off
 *------------------------------------------*/
static void map_setcell(int16 m, int16 x, int16 y, cell_t cell, bool flag)
{
	int j;

	if( m < 0 || m >= map->count || x < 0 || x >= map->list[m].xs || y < 0 || y >= map->list[m].ys )
		return;

	j = x + y*map->list[m].xs;

	switch( cell ) {
	case CELL_WALKABLE:      map->list[m].cell[j].walkable = flag;      break;
	case CELL_SHOOTABLE:     map->list[m].cell[j].shootable = flag;     break;
	case CELL_WATER:         map->list[m].cell[j].water = flag;         break;

	case CELL_NPC:           map->list[m].cell[j].npc = flag;           break;
	case CELL_BASILICA:      map->list[m].cell[j].basilica = flag;      break;
	case CELL_LANDPROTECTOR: map->list[m].cell[j].landprotector = flag; break;
	case CELL_NOVENDING:     map->list[m].cell[j].novending = flag;     break;
	case CELL_NOCHAT:        map->list[m].cell[j].nochat = flag;        break;
	case CELL_ICEWALL:       map->list[m].cell[j].icewall = flag;       break;
	case CELL_NOICEWALL:     map->list[m].cell[j].noicewall = flag;     break;
	case CELL_NOSKILL:       map->list[m].cell[j].noskill = flag;       break;

	default:
		ShowWarning("map_setcell: invalid cell type '%d'\n", (int)cell);
		break;
	}
}
static void map_sub_setcell(int16 m, int16 x, int16 y, cell_t cell, bool flag)
{
	if( m < 0 || m >= map->count || x < 0 || x >= map->list[m].xs || y < 0 || y >= map->list[m].ys )
		return;

	map->cellfromcache(&map->list[m]);
	map->list[m].setcell = map->setcell;
	map->list[m].getcellp = map->getcellp;
	map->list[m].setcell(m,x,y,cell,flag);
}
static void map_setgatcell(int16 m, int16 x, int16 y, int gat)
{
	int j;
	struct mapcell cell;

	if( m < 0 || m >= map->count || x < 0 || x >= map->list[m].xs || y < 0 || y >= map->list[m].ys )
		return;

	j = x + y*map->list[m].xs;

	cell = map->gat2cell(gat);
	map->list[m].cell[j].walkable = cell.walkable;
	map->list[m].cell[j].shootable = cell.shootable;
	map->list[m].cell[j].water = cell.water;
}

/*==========================================
 * Invisible Walls
 *------------------------------------------*/
static void map_iwall_nextxy(int16 x, int16 y, int8 dir, int pos, int16 *x1, int16 *y1)
{
	nullpo_retv(x1);
	nullpo_retv(y1);

	if( dir == 0 || dir == 4 )
		*x1 = x; // Keep X
	else if( dir > 0 && dir < 4 )
		*x1 = x - pos; // Going left
	else
		*x1 = x + pos; // Going right

	if( dir == 2 || dir == 6 )
		*y1 = y;
	else if( dir > 2 && dir < 6 )
		*y1 = y - pos;
	else
		*y1 = y + pos;
}

static bool map_iwall_set(int16 m, int16 x, int16 y, int size, int8 dir, bool shootable, const char *wall_name)
{
	struct iwall_data *iwall;
	int i;
	int16 x1 = 0, y1 = 0;

	if( size < 1 || !wall_name )
		return false;

	if( (iwall = (struct iwall_data *)strdb_get(map->iwall_db, wall_name)) != NULL )
		return false; // Already Exists

	if (map->getcell(m, NULL, x, y, CELL_CHKNOREACH))
		return false; // Starting cell problem

	CREATE(iwall, struct iwall_data, 1);
	iwall->m = m;
	iwall->x = x;
	iwall->y = y;
	iwall->size = size;
	iwall->dir = dir;
	iwall->shootable = shootable;
	safestrncpy(iwall->wall_name, wall_name, sizeof(iwall->wall_name));

	for( i = 0; i < size; i++ ) {
		map->iwall_nextxy(x, y, dir, i, &x1, &y1);

		if (map->getcell(m, NULL, x1, y1, CELL_CHKNOREACH))
			break; // Collision

		map->list[m].setcell(m, x1, y1, CELL_WALKABLE, false);
		map->list[m].setcell(m, x1, y1, CELL_SHOOTABLE, shootable);

		clif->changemapcell(0, m, x1, y1, map->getcell(m, NULL, x1, y1, CELL_GETTYPE), ALL_SAMEMAP);
	}

	iwall->size = i;

	strdb_put(map->iwall_db, iwall->wall_name, iwall);
	map->list[m].iwall_num++;

	return true;
}

static void map_iwall_get(struct map_session_data *sd)
{
	struct iwall_data *iwall;
	struct DBIterator *iter;
	int16 x1, y1;
	int i;

	nullpo_retv(sd);

	if( map->list[sd->bl.m].iwall_num < 1 )
		return;

	iter = db_iterator(map->iwall_db);
	for( iwall = dbi_first(iter); dbi_exists(iter); iwall = dbi_next(iter) ) {
		if( iwall->m != sd->bl.m )
			continue;

		for( i = 0; i < iwall->size; i++ ) {
			map->iwall_nextxy(iwall->x, iwall->y, iwall->dir, i, &x1, &y1);
			clif->changemapcell(sd->fd, iwall->m, x1, y1, map->getcell(iwall->m, &sd->bl, x1, y1, CELL_GETTYPE), SELF);
		}
	}
	dbi_destroy(iter);
}

static bool map_iwall_remove(const char *wall_name)
{
	struct iwall_data *iwall;
	int16 i, x1, y1;

	if( (iwall = (struct iwall_data *)strdb_get(map->iwall_db, wall_name)) == NULL )
		return false;

	for( i = 0; i < iwall->size; i++ ) {
		map->iwall_nextxy(iwall->x, iwall->y, iwall->dir, i, &x1, &y1);

		map->list[iwall->m].setcell(iwall->m, x1, y1, CELL_SHOOTABLE, true);
		map->list[iwall->m].setcell(iwall->m, x1, y1, CELL_WALKABLE, true);

		clif->changemapcell(0, iwall->m, x1, y1, map->getcell(iwall->m, NULL, x1, y1, CELL_GETTYPE), ALL_SAMEMAP);
	}

	map->list[iwall->m].iwall_num--;
	strdb_remove(map->iwall_db, iwall->wall_name);
	return true;
}

/**
 * Reads a map's compressed cell data from its mapcache file.
 *
 * @param[in,out] m The target map.
 * @return The loading success state.
 * @retval false in case of errors.
 */
static bool map_readfromcache(struct map_data *m)
{
	unsigned int file_size;
	char file_path[256];
	FILE *fp = NULL;
	bool retval = false;
	int16 version;

	nullpo_retr(false, m);

	snprintf(file_path, sizeof(file_path), "%s%s%s.%s", "maps/", DBPATH, m->name, "mcache");
	fp = fopen(file_path, "rb");

	if (fp == NULL) {
		ShowWarning("map_readfromcache: Could not open the mapcache file for map '%s' at path '%s'.\n", m->name, file_path);
		return false;
	}

	if (fread(&version, sizeof(version), 1, fp) < 1) {
		ShowError("map_readfromcache: Could not read file version for map '%s'.\n", m->name);
		fclose(fp);
		return false;
	}

	fseek(fp, 0, SEEK_END);
	file_size = (unsigned int)ftell(fp);
	fseek(fp, 0, SEEK_SET); // Rewind file pointer before passing it to the read function.

	switch(version) {
	case 1:
		retval = map->readfromcache_v1(fp, m, file_size);
		break;
	default:
		ShowError("map_readfromcache: Mapcache file has unknown version '%d' for map '%s'.\n", version, m->name);
		break;
	}

	fclose(fp);
	return retval;
}

/**
 * Reads a map's compressed cell data from its mapcache file (file format
 * version 1).
 *
 * @param[in]     fp        The file pointer to read from (opened and closed by
 *                          the caller).
 * @param[in,out] m         The target map.
 * @param[in]     file_size The size of the file to load from.
 * @return The loading success state.
 * @retval false in case of errors.
 */
static bool map_readfromcache_v1(FILE *fp, struct map_data *m, unsigned int file_size)
{
	struct map_cache_header mheader = { 0 };
	uint8 md5buf[16] = { 0 };
	int map_size;
	nullpo_retr(false, fp);
	nullpo_retr(false, m);

	if (file_size <= sizeof(mheader) || fread(&mheader, sizeof(mheader), 1, fp) < 1) {
		ShowError("map_readfromcache: Failed to read cache header for map '%s'.\n", m->name);
		return false;
	}

	if (mheader.len <= 0) {
		ShowError("map_readfromcache: A file with negative or zero compressed length passed '%d'.\n", mheader.len);
		return false;
	}

	if (file_size < sizeof(mheader) + mheader.len) {
		ShowError("map_readfromcache: An incomplete file passed for map '%s'.\n", m->name);
		return false;
	}

	if (mheader.ys <= 0 || mheader.xs <= 0) {
		ShowError("map_readfromcache: A map with invalid size passed '%s' xs: '%d' ys: '%d'.\n", m->name, mheader.xs, mheader.ys);
		return false;
	}

	m->xs = mheader.xs;
	m->ys = mheader.ys;
	map_size = (int)mheader.xs * (int)mheader.ys;

	if (map_size > MAX_MAP_SIZE) {
		ShowWarning("map_readfromcache: %s exceeded MAX_MAP_SIZE of %d.\n", m->name, MAX_MAP_SIZE);
		return false;
	}

	CREATE(m->cell_buf.data, uint8, mheader.len);
	m->cell_buf.len = mheader.len;
	if (fread(m->cell_buf.data, mheader.len, 1, fp) < 1) {
		ShowError("mapreadfromcache: Could not load the compressed cell data for map '%s'.\n", m->name);
		aFree(m->cell_buf.data);
		m->cell_buf.data = NULL;
		m->cell_buf.len = 0;
		return false;
	}

	md5->binary(m->cell_buf.data, m->cell_buf.len, md5buf);

	if (memcmp(md5buf, mheader.md5_checksum, sizeof(md5buf)) != 0) {
		ShowError("mapreadfromcache: md5 checksum check failed for map '%s'\n", m->name);
		aFree(m->cell_buf.data);
		m->cell_buf.data = NULL;
		m->cell_buf.len = 0;
		return false;
	}

	m->cell = (struct mapcell *)0xdeadbeaf;

	return true;
}

/**
 * Adds a new empty map to the map list.
 *
 * Assumes that there's enough space in the map list.
 *
 * @param mapname The new map's name.
 * @return success state.
 */
static int map_addmap(const char *mapname)
{
	map->list[map->count].instance_id = -1;
	mapindex->getmapname(mapname, map->list[map->count++].name);
	return 0;
}

/**
 * Removes a map from the map list.
 *
 * @param id The map ID.
 */
static void map_delmapid(int id)
{
	Assert_retv(id >= 0 && id < map->count);
	ShowNotice("Removing map [ %s ] from maplist"CL_CLL"\n",map->list[id].name);
	memmove(map->list+id, map->list+id+1, sizeof(map->list[0])*(map->count-id-1));
	map->count--;
}

/**
 * Removes a map fromt he map list.
 *
 * @param mapname The name of the map to remove.
 * @return the number of removed maps.
 */
static int map_delmap(const char *mapname)
{
	int i;
	char map_name[MAP_NAME_LENGTH];

	nullpo_ret(mapname);
	if (strcmpi(mapname, "all") == 0) {
		map->count = 0;
		return 0;
	}

	mapindex->getmapname(mapname, map_name);
	for(i = 0; i < map->count; i++) {
		if (strcmp(map->list[i].name, map_name) == 0) {
			map->delmapid(i);
			return 1;
		}
	}
	return 0;
}

/**
 *
 **/
static void map_zone_clear_single(struct map_zone_data *zone)
{
	int i;

	nullpo_retv(zone);

	for(i = 0; i < zone->disabled_skills_count; i++) {
		aFree(zone->disabled_skills[i]);
	}

	if( zone->disabled_skills )
		aFree(zone->disabled_skills);

	if( zone->disabled_items )
		aFree(zone->disabled_items);

	if( zone->cant_disable_items )
		aFree(zone->cant_disable_items);

	for(i = 0; i < zone->mapflags_count; i++) {
		aFree(zone->mapflags[i]);
	}

	if( zone->mapflags )
		aFree(zone->mapflags);

	for(i = 0; i < zone->disabled_commands_count; i++) {
		aFree(zone->disabled_commands[i]);
	}

	if( zone->disabled_commands )
		aFree(zone->disabled_commands);

	for(i = 0; i < zone->capped_skills_count; i++) {
		aFree(zone->capped_skills[i]);
	}

	if( zone->capped_skills )
		aFree(zone->capped_skills);
}
/**
 *
 **/
static void map_zone_db_clear(void)
{
	struct DBIterator *iter = db_iterator(map->zone_db);
	struct map_zone_data *zone = NULL;

	for(zone = dbi_first(iter); dbi_exists(iter); zone = dbi_next(iter)) {
		map->zone_clear_single(zone);
	}

	dbi_destroy(iter);

	db_destroy(map->zone_db);/* will aFree(zone) */

	/* clear the pk zone stuff */
	map->zone_clear_single(&map->zone_pk);
	/* clear the main zone stuff */
	map->zone_clear_single(&map->zone_all);
}
static void map_clean(int i)
{
	Assert_retv(i >= 0 && i < map->count);

	if (map->list[i].cell && map->list[i].cell != (struct mapcell *)0xdeadbeaf)
		aFree(map->list[i].cell);
	if (map->list[i].block)
		aFree(map->list[i].block);
	if (map->list[i].block_mob)
		aFree(map->list[i].block_mob);

	if (battle_config.dynamic_mobs != 0) { //Dynamic mobs flag by [random]
		if (map->list[i].mob_delete_timer != INVALID_TIMER)
			timer->delete(map->list[i].mob_delete_timer, map->removemobs_timer);
		for (int j = 0; j < MAX_MOB_LIST_PER_MAP; j++) {
			if (map->list[i].moblist[j] != NULL)
				aFree(map->list[i].moblist[j]);
		}
	}

	if (map->list[i].unit_count != 0) {
		if (map->list[i].units != NULL) {
			for (int v = 0; v < map->list[i].unit_count; v++) {
				aFree(map->list[i].units[v]);
			}
			aFree(map->list[i].units);
			map->list[i].units = NULL;
		}
		map->list[i].unit_count = 0;
	}

	if (map->list[i].skill_count != 0) {
		if (map->list[i].skills != NULL) {
			for (int v = 0; v < map->list[i].skill_count; v++) {
				aFree(map->list[i].skills[v]);
			}
			aFree(map->list[i].skills);
			map->list[i].skills = NULL;
		}
		map->list[i].skill_count = 0;
	}

	if (map->list[i].zone_mf_count != 0) {
		if (map->list[i].zone_mf != NULL) {
			for (int v = 0; v < map->list[i].zone_mf_count; v++) {
				aFree(map->list[i].zone_mf[v]);
			}
			aFree(map->list[i].zone_mf);
			map->list[i].zone_mf = NULL;
		}
		map->list[i].zone_mf_count = 0;
	}

	if (map->list[i].drop_list_count != 0)
		map->list[i].drop_list_count = 0;
	if (map->list[i].drop_list != NULL)
		aFree(map->list[i].drop_list);

	if (map->list[i].channel != NULL)
		channel->delete(map->list[i].channel);

	VECTOR_CLEAR(map->list[i].qi_list);
	HPM->data_store_destroy(&map->list[i].hdata);
}
static void do_final_maps(void)
{
	for (int i = 0; i < map->count; i++)
		map->clean(i);
	map->zone_db_clear();
}

static void map_zonedb_reload(void)
{
	// first, reset maps to their initial zones:
	for (int i = 0; i < map->count; i++) {
		map->zone_remove_all(i);

		if (battle_config.pk_mode) {
			map->list[i].flag.pvp = 1;
			map->list[i].zone = &map->zone_pk;
		} else {
			map->list[i].flag.pvp = 0;
			map->list[i].zone = &map->zone_all;
		}

		map->list[i].prev_zone = map->list[i].zone;
	}

	// now it's safe to remove the zones:
	map->zone_db_clear();

	// then reload everything from scratch:
	map->zone_db = strdb_alloc(DB_OPT_DUP_KEY | DB_OPT_RELEASE_DATA, MAP_ZONE_NAME_LENGTH);
	map->read_zone_db();
}


/// Initializes map flags and adjusts them depending on configuration.
static void map_flags_init(void)
{
	int i, v = 0;

	for( i = 0; i < map->count; i++ ) {
		// mapflags
		memset(&map->list[i].flag, 0, sizeof(map->list[i].flag));

		// additional mapflag data
		map->list[i].nocommand = 0;   // nocommand mapflag level
		map->list[i].bexp      = 100; // per map base exp multiplicator
		map->list[i].jexp      = 100; // per map job exp multiplicator
		if( map->list[i].drop_list != NULL )
			aFree(map->list[i].drop_list);
		map->list[i].drop_list = NULL;
		map->list[i].drop_list_count = 0;

		if( map->list[i].unit_count ) {
			for(v = 0; v < map->list[i].unit_count; v++) {
				aFree(map->list[i].units[v]);
			}
			aFree(map->list[i].units);
		}
		map->list[i].units = NULL;
		map->list[i].unit_count = 0;

		if( map->list[i].skill_count ) {
			for(v = 0; v < map->list[i].skill_count; v++) {
				aFree(map->list[i].skills[v]);
			}
			aFree(map->list[i].skills);
		}
		map->list[i].skills = NULL;
		map->list[i].skill_count = 0;

		// adjustments
		if( battle_config.pk_mode ) {
			map->list[i].flag.pvp = 1; // make all maps pvp for pk_mode [Valaris]
			map->list[i].zone = &map->zone_pk;
		} else /* align with 'All' zone */
			map->list[i].zone = &map->zone_all;

		if( map->list[i].zone_mf_count ) {
			for(v = 0; v < map->list[i].zone_mf_count; v++) {
				aFree(map->list[i].zone_mf[v]);
			}
			aFree(map->list[i].zone_mf);
		}

		map->list[i].zone_mf = NULL;
		map->list[i].zone_mf_count = 0;
		map->list[i].prev_zone = map->list[i].zone;

		map->list[i].invincible_time_inc = 0;

		map->list[i].weapon_damage_rate = 100;
		map->list[i].magic_damage_rate  = 100;
		map->list[i].misc_damage_rate   = 100;
		map->list[i].short_damage_rate  = 100;
		map->list[i].long_damage_rate   = 100;

		VECTOR_CLEAR(map->list[i].qi_list);
		VECTOR_INIT(map->list[i].qi_list);
	}
}

#define NO_WATER 1000000

/*
 * Reads from the .rsw for each map
 * Returns water height (or NO_WATER if file doesn't exist) or other error is encountered.
 * Assumed path for file is data/mapname.rsw
 * Credits to LittleWolf
 */
static int map_waterheight(char *mapname)
{
	char fn[256];
	char *rsw = NULL;
	const char *found;

	nullpo_retr(NO_WATER, mapname);
	//Look up for the rsw
	snprintf(fn, sizeof(fn), "data\\%s.rsw", mapname);

	if ((found = grfio->find_file(fn)))
		safestrncpy(fn, found, sizeof(fn)); // replace with real name

	// read & convert fn
	rsw = grfio_read(fn);
	if (rsw) {
		if (memcmp(rsw, "GRSW", 4) != 0) {
			ShowWarning("Failed to find water level for %s (%s)\n", mapname, fn);
			aFree(rsw);
			return NO_WATER;
		}
		int major_version = rsw[4];
		int minor_version = rsw[5];
		if (major_version > 2 || (major_version == 2 && minor_version > 5)) {
			ShowWarning("Failed to find water level for %s (%s)\n", mapname, fn);
			aFree(rsw);
			return NO_WATER;
		}
		if (major_version < 1 || (major_version == 1 && minor_version <= 4)) {
			ShowWarning("Failed to find water level for %s (%s)\n", mapname, fn);
			aFree(rsw);
			return NO_WATER;
		}
		int offset = 166;
		if (major_version == 2 && minor_version >= 5) {
			offset += 4;
		}
		if (major_version == 2 && minor_version >= 2) {
			offset += 1;
		}

		//Load water height from file
		int wh = (int)*(float*)(rsw + offset);
		aFree(rsw);
		return wh;
	}
	ShowWarning("Failed to find water level for %s (%s)\n", mapname, fn);
	return NO_WATER;
}

/*==================================
 * .GAT format
 *----------------------------------*/
static int map_readgat(struct map_data *m)
{
	char filename[256];
	uint8* gat;
	int water_height;
	size_t xy, off, num_cells;

	nullpo_ret(m);
	sprintf(filename, "data\\%s.gat", m->name);

	gat = grfio_read(filename);
	if (gat == NULL)
		return 0;

	m->xs = *(int32*)(gat+6);
	m->ys = *(int32*)(gat+10);
	num_cells = m->xs * m->ys;
	CREATE(m->cell, struct mapcell, num_cells);

	water_height = map->waterheight(m->name);

	// Set cell properties
	off = 14;
	for( xy = 0; xy < num_cells; ++xy )
	{
		// read cell data
		float height = *(float*)( gat + off      );
		uint32 type = *(uint32*)( gat + off + 16 );
		off += 20;

		if( type == 0 && water_height != NO_WATER && height > water_height )
			type = 3; // Cell is 0 (walkable) but under water level, set to 3 (walkable water)

		m->cell[xy] = map->gat2cell(type);
	}

	aFree(gat);

	return 1;
}

/*======================================
 * Add/Remove map to the map_db
 *--------------------------------------*/
static void map_addmap2db(struct map_data *m)
{
	nullpo_retv(m);
	map->index2mapid[m->index] = m->m;
}

static void map_removemapdb(struct map_data *m)
{
	nullpo_retv(m);
	map->index2mapid[m->index] = -1;
}

/*======================================
 * Initiate maps loading stage
 *--------------------------------------*/
static int map_readallmaps(void)
{
	int i;
	int maps_removed = 0;

	if (map->enable_grf) {
		ShowStatus("Loading maps (using GRF files)...\n");
	} else {
		ShowStatus("Loading maps using map cache files...\n");
	}

	for(i = 0; i < map->count; i++) {
		size_t size;

		// show progress
		if(map->enable_grf)
			ShowStatus("Loading maps [%i/%i]: %s"CL_CLL"\r", i, map->count, map->list[i].name);

		// try to load the map
		if( !
			(map->enable_grf?
			map->readgat(&map->list[i])
			:map->readfromcache(&map->list[i]))
			) {
				map->delmapid(i);
				maps_removed++;
				i--;
				continue;
		}

		map->list[i].index = mapindex->name2id(map->list[i].name);

		if ( map->index2mapid[map_id2index(i)] != -1 ) {
			ShowWarning("Map %s already loaded!"CL_CLL"\n", map->list[i].name);
			if (map->list[i].cell && map->list[i].cell != (struct mapcell *)0xdeadbeaf) {
				aFree(map->list[i].cell);
				map->list[i].cell = NULL;
			}
			map->delmapid(i);
			maps_removed++;
			i--;
			continue;
		}

		map->list[i].m = i;
		map->addmap2db(&map->list[i]);

		memset(map->list[i].moblist, 0, sizeof(map->list[i].moblist)); //Initialize moblist [Skotlex]
		map->list[i].mob_delete_timer = INVALID_TIMER; //Initialize timer [Skotlex]

		map->list[i].bxs = (map->list[i].xs + BLOCK_SIZE - 1) / BLOCK_SIZE;
		map->list[i].bys = (map->list[i].ys + BLOCK_SIZE - 1) / BLOCK_SIZE;

		size = map->list[i].bxs * map->list[i].bys * sizeof(struct block_list*);
		map->list[i].block = (struct block_list**)aCalloc(1, size);
		map->list[i].block_mob = (struct block_list**)aCalloc(1, size);

		map->list[i].getcellp = map->sub_getcellp;
		map->list[i].setcell  = map->sub_setcell;
	}

	// intialization and configuration-dependent adjustments of mapflags
	map->flags_init();

	// finished map loading
	ShowInfo("Successfully loaded '"CL_WHITE"%d"CL_RESET"' maps."CL_CLL"\n",map->count);
	instance->start_id = map->count; // Next Map Index will be instances

	if (maps_removed)
		ShowNotice("Maps removed: '"CL_WHITE"%d"CL_RESET"'\n",maps_removed);

	return 0;
}

/**
 * Reads 'map_configuration/console' and initializes required variables.
 *
 * @param filename Path to configuration file (used in error and warning messages).
 * @param config   The current config being parsed.
 * @param imported Whether the current config is imported from another file.
 *
 * @retval false in case of error.
 */
static bool map_config_read_console(const char *filename, struct config_t *config, bool imported)
{
	struct config_setting_t *setting = NULL;

	nullpo_retr(false, filename);
	nullpo_retr(false, config);

	if ((setting = libconfig->lookup(config, "map_configuration/console")) == NULL) {
		if (imported)
			return true;
		ShowError("map_config_read: map_configuration/console was not found in %s!\n", filename);
		return false;
	}

	libconfig->setting_lookup_bool_real(setting, "stdout_with_ansisequence", &showmsg->stdout_with_ansisequence);
	if (libconfig->setting_lookup_int(setting, "console_silent", &showmsg->silent) == CONFIG_TRUE) {
		if (showmsg->silent) // only bother if its actually enabled
			ShowInfo("Console Silent Setting: %d\n", showmsg->silent);
	}
	libconfig->setting_lookup_mutable_string(setting, "timestamp_format", showmsg->timestamp_format, sizeof(showmsg->timestamp_format));
	libconfig->setting_lookup_int(setting, "console_msg_log", &showmsg->console_log);

	return true;
}

/**
 * Reads 'map_configuration/sql_connection' and initializes required variables.
 *
 * @param filename Path to configuration file (used in error and warning messages).
 * @param config   The current config being parsed.
 * @param imported Whether the current config is imported from another file.
 *
 * @retval false in case of error.
 */
static bool map_config_read_connection(const char *filename, struct config_t *config, bool imported)
{
	struct config_setting_t *setting = NULL;

	nullpo_retr(false, filename);
	nullpo_retr(false, config);

	if ((setting = libconfig->lookup(config, "map_configuration/sql_connection")) == NULL) {
		if (imported)
			return true;
		ShowError("map_config_read: map_configuration/sql_connection was not found in %s!\n", filename);
		ShowWarning("map_config_read_connection: Defaulting sql_connection...\n");
		return false;
	}

	libconfig->setting_lookup_int(setting, "db_port", &map->server_port);
	libconfig->setting_lookup_mutable_string(setting, "db_hostname", map->server_ip, sizeof(map->server_ip));
	libconfig->setting_lookup_mutable_string(setting, "db_username", map->server_id, sizeof(map->server_id));
	libconfig->setting_lookup_mutable_string(setting, "db_password", map->server_pw, sizeof(map->server_pw));
	libconfig->setting_lookup_mutable_string(setting, "db_database", map->server_db, sizeof(map->server_db));
	libconfig->setting_lookup_mutable_string(setting, "default_codepage", map->default_codepage, sizeof(map->default_codepage));
	return true;
}

/**
 * Reads 'map_configuration/inter' and initializes required variables.
 *
 * @param filename Path to configuration file (used in error and warning messages).
 * @param config   The current config being parsed.
 * @param imported Whether the current config is imported from another file.
 *
 * @retval false in case of error.
 */
static bool map_config_read_inter(const char *filename, struct config_t *config, bool imported)
{
	struct config_setting_t *setting = NULL;
	const char *str = NULL;
	char temp[24];
	uint16 port;

	nullpo_retr(false, filename);
	nullpo_retr(false, config);

	if ((setting = libconfig->lookup(config, "map_configuration/inter")) == NULL) {
		if (imported)
			return true;
		ShowError("map_config_read: map_configuration/inter was not found in %s!\n", filename);
		return false;
	}

	// Login information
	if (libconfig->setting_lookup_mutable_string(setting, "userid", temp, sizeof(temp)) == CONFIG_TRUE)
		chrif->setuserid(temp);
	if (libconfig->setting_lookup_mutable_string(setting, "passwd", temp, sizeof(temp)) == CONFIG_TRUE)
		chrif->setpasswd(temp);

	// Char and map-server information
	if (libconfig->setting_lookup_string(setting, "char_ip", &str) == CONFIG_TRUE)
		map->char_ip_set = chrif->setip(str);
	if (libconfig->setting_lookup_uint16(setting, "char_port", &port) == CONFIG_TRUE)
		chrif->setport(port);

	if (libconfig->setting_lookup_string(setting, "map_ip", &str) == CONFIG_TRUE)
		map->ip_set = clif->setip(str);
	if (libconfig->setting_lookup_uint16(setting, "map_port", &port) == CONFIG_TRUE) {
		clif->setport(port);
		map->port = port;
	}
	if (libconfig->setting_lookup_string(setting, "bind_ip", &str) == CONFIG_TRUE)
		clif->setbindip(str);

	return true;
}

/**
 * Reads 'map_configuration/database' and initializes required variables
 *
 * @param filename Path to configuration file (used in error and warning messages).
 * @param config   The current config being parsed.
 * @param imported Whether the current config is imported from another file.
 *
 * @retval false in case of error.
 */
static bool map_config_read_database(const char *filename, struct config_t *config, bool imported)
{
	struct config_setting_t *setting = NULL;

	nullpo_retr(false, filename);
	nullpo_retr(false, config);

	if ((setting = libconfig->lookup(config, "map_configuration/database")) == NULL) {
		if (imported)
			return true;
		ShowError("map_config_read: map_configuration/database was not found in %s!\n", filename);
		return false;
	}
	libconfig->setting_lookup_mutable_string(setting, "db_path", map->db_path, sizeof(map->db_path));
	libconfig->set_db_path(map->db_path);
	libconfig->setting_lookup_int(setting, "save_settings", &map->save_settings);

	if (libconfig->setting_lookup_int(setting, "autosave_time", &map->autosave_interval) == CONFIG_TRUE) {
		if (map->autosave_interval < 1) // Revert to default saving
			map->autosave_interval = DEFAULT_MAP_AUTOSAVE_INTERVAL;
		else
			map->autosave_interval *= 1000; // Pass from s to ms
	}
	if (libconfig->setting_lookup_int(setting, "minsave_time", &map->minsave_interval) == CONFIG_TRUE) {
		if (map->minsave_interval < 1)
			map->minsave_interval = 1;
	}

	return true;
}

/**
 * Reads 'map_configuration/map_list'/'map_configuration/map_removed' and adds
 * or removes maps from map-server.
 *
 * @param filename Path to configuration file (used in error and warning messages).
 * @param config   The current config being parsed.
 * @param imported Whether the current config is imported from another file.
 *
 * @retval false in case of error.
 */
static bool map_config_read_map_list(const char *filename, struct config_t *config, bool imported)
{
	struct config_setting_t *setting = NULL;
	int i, count = 0;
	struct DBMap *deleted_maps;

	nullpo_retr(false, filename);
	nullpo_retr(false, config);

	deleted_maps = strdb_alloc(DB_OPT_DUP_KEY|DB_OPT_ALLOW_NULL_DATA, MAP_NAME_LENGTH);

	// Remove maps
	if ((setting = libconfig->lookup(config, "map_configuration/map_removed")) != NULL) {
		count = libconfig->setting_length(setting);
		for (i = 0; i < count; i++) {
			const char *mapname;

			if ((mapname = libconfig->setting_get_string_elem(setting, i)) == NULL || mapname[0] == '\0')
				continue;

			strdb_put(deleted_maps, mapname, NULL);

			if (imported) // Map list is empty on the first run, only do this for imported files.
				map->delmap(mapname);
		}
	}

	if ((setting = libconfig->lookup(config, "map_configuration/map_list")) == NULL) {
		db_destroy(deleted_maps);
		if (imported)
			return true;
		ShowError("map_config_read_map_list: map_configuration/map_list was not found in %s!\n", filename);
		return false;
	}

	// Add maps to map->list
	count = libconfig->setting_length(setting);

	if (count <= 0) {
		db_destroy(deleted_maps);
		if (imported)
			return true;
		ShowWarning("map_config_read_map_list: no maps found in %s!\n", filename);
		return false;
	}

	RECREATE(map->list, struct map_data, map->count + count); // TODO: VECTOR candidate

	for (i = 0; i < count; i++) {
		const char *mapname;

		if ((mapname = libconfig->setting_get_string_elem(setting, i)) == NULL || mapname[0] == '\0')
			continue;

		if (strdb_exists(deleted_maps, mapname))
			continue;

		map->addmap(mapname);
	}

	RECREATE(map->list, struct map_data, map->count);

	db_destroy(deleted_maps);
	return true;
}

/**
 * Reads map-server configuration files (map-server.conf) and initialises
 * required variables.
 *
 * @param filename Path to configuration file.
 * @param imported Whether the current config is imported from another file.
 *
 * @retval false in case of error.
 */
static bool map_config_read(const char *filename, bool imported)
{
	struct config_t config;
	struct config_setting_t *setting = NULL;
	const char *import = NULL;
	bool retval = true;

	nullpo_retr(false, filename);

	if (!libconfig->load_file(&config, filename))
		return false;

	if ((setting = libconfig->lookup(&config, "map_configuration")) == NULL) {
		libconfig->destroy(&config);
		if (imported)
			return true;
		ShowError("map_config_read: map_configuration was not found in %s!\n", filename);
		return false;
	}

	libconfig->setting_lookup_mutable_string(setting, "help_txt", map->help_txt, sizeof(map->help_txt));
	libconfig->setting_lookup_mutable_string(setting, "charhelp_txt", map->charhelp_txt, sizeof(map->charhelp_txt));
	libconfig->setting_lookup_bool(setting, "enable_spy", &map->enable_spy);
	libconfig->setting_lookup_bool(setting, "use_grf", &map->enable_grf);
	libconfig->setting_lookup_mutable_string(setting, "default_language", map->default_lang_str, sizeof(map->default_lang_str));

	if (!map->config_read_console(filename, &config, imported))
		retval = false;
	if (!map->config_read_connection(filename, &config, imported))
		retval = false;
	if (!map->config_read_inter(filename, &config, imported))
		retval = false;
	if (!map->config_read_database(filename, &config, imported))
		retval = false;
	if (!map->config_read_map_list(filename, &config, imported))
		retval = false;

	// import should overwrite any previous configuration, so it should be called last
	if (libconfig->lookup_string(&config, "import", &import) == CONFIG_TRUE) {
		if (strcmp(import, filename) == 0 || strcmp(import, map->MAP_CONF_NAME) == 0) {
			ShowWarning("map_config_read: Loop detected! Skipping 'import'...\n");
		} else {
			if (!map->config_read(import, true))
				retval = false;
		}
	}

	libconfig->destroy(&config);
	return retval;
}

/**
 * Reads 'npc_global_list'/'npc_removed_list' and adds or removes NPC sources
 * from map-server.
 *
 * @param filename Path to configuration file (used in error and warning messages).
 * @param imported Whether the current config is imported from another file.
 *
 * @retval false in case of error.
 */
static bool map_read_npclist(const char *filename, bool imported)
{
	struct config_t config;
	struct config_setting_t *setting = NULL;
	const char *import = NULL;
	bool retval = true;
	bool remove_all = false;

	struct DBMap *deleted_npcs;

	nullpo_retr(false, filename);

	if (!libconfig->load_file(&config, filename))
		return false;

	deleted_npcs = strdb_alloc(DB_OPT_DUP_KEY|DB_OPT_ALLOW_NULL_DATA, 0);

	// Remove NPCs
	if ((setting = libconfig->lookup(&config, "npc_removed_list")) != NULL) {
		int i, del_count = libconfig->setting_length(setting);
		for (i = 0; i < del_count; i++) {
			const char *scriptname;

			if ((scriptname = libconfig->setting_get_string_elem(setting, i)) == NULL || scriptname[0] == '\0')
				continue;

			if (strcmp(scriptname, "all") == 0) {
				remove_all = true;
				npc->clearsrcfile();
			} else {
				strdb_put(deleted_npcs, scriptname, NULL);
				npc->delsrcfile(scriptname);
			}
		}
	}

	if ((setting = libconfig->lookup(&config, "npc_global_list")) != NULL) {
		int i, count = libconfig->setting_length(setting);
		if (count <= 0) {
			if (!imported) {
				ShowWarning("map_read_npclist: no NPCs found in %s!\n", filename);
				retval = false;
			}
		}
		for (i = 0; i < count; i++) {
			const char *scriptname;

			if ((scriptname = libconfig->setting_get_string_elem(setting, i)) == NULL || scriptname[0] == '\0')
				continue;

			if (remove_all || strdb_exists(deleted_npcs, scriptname))
				continue;

			npc->addsrcfile(scriptname);
		}
	} else {
		ShowError("map_read_npclist: npc_global_list was not found in %s!\n", filename);
		retval = false;
	}

	db_destroy(deleted_npcs);

	// import should overwrite any previous configuration, so it should be called last
	if (libconfig->lookup_string(&config, "import", &import) == CONFIG_TRUE) {
		const char *base_npclist = NULL;
#ifdef RENEWAL
		base_npclist = "npc/re/scripts_main.conf";
#else
		base_npclist = "npc/pre-re/scripts_main.conf";
#endif
		if (strcmp(import, filename) == 0 || strcmp(import, base_npclist) == 0) {
			ShowWarning("map_read_npclist: Loop detected! Skipping 'import'...\n");
		} else {
			if (!map->read_npclist(import, true))
				retval = false;
		}
	}

	libconfig->destroy(&config);
	return retval;
}

/**
 * Reloads all the scripts.
 *
 * @param clear whether to clear the script list before reloading.
 */
static void map_reloadnpc(bool clear)
{
	int i;
	if (clear)
		npc->clearsrcfile();

#ifdef RENEWAL
	map->read_npclist("npc/re/scripts_main.conf", false);
#else
	map->read_npclist("npc/pre-re/scripts_main.conf", false);
#endif

	// Append extra scripts
	for( i = 0; i < map->extra_scripts_count; i++ ) {
		npc->addsrcfile(map->extra_scripts[i]);
	}
}

/**
 * Reads inter-server.conf and initializes required variables.
 *
 * @param filename Path to configuration file
 * @param imported Whether the current config is imported from another file.
 *
 * @retval false in case of error.
 */
static bool inter_config_read(const char *filename, bool imported)
{
	struct config_t config;
	const struct config_setting_t *setting = NULL;
	const char *import = NULL;
	bool retval = true;

	nullpo_retr(false, filename);

	if (!libconfig->load_file(&config, filename))
		return false;

	if ((setting = libconfig->lookup(&config, "inter_configuration")) == NULL) {
		libconfig->destroy(&config);
		if (imported)
			return true;
		ShowError("inter_config_read: inter_configuration was not found in %s!\n", filename);
		return false;
	}

	if (!map->inter_config_read_database_names(filename, &config, imported))
		retval = false;
	if (!map->inter_config_read_connection(filename, &config, imported))
		retval = false;

	if (!HPM->parse_conf(&config, filename, HPCT_MAP_INTER, imported))
		retval = false;

	// import should overwrite any previous configuration, so it should be called last
	if (libconfig->lookup_string(&config, "import", &import) == CONFIG_TRUE) {
		if (strcmp(import, filename) == 0 || strcmp(import, map->INTER_CONF_NAME) == 0) {
			ShowWarning("inter_config_read: Loop detected in %s! Skipping 'import'...\n", filename);
		} else {
			if (!map->inter_config_read(import, true))
				retval = false;
		}
	}

	libconfig->destroy(&config);
	return retval;
}

/**
 * Reads the 'inter_configuration/log/sql_connection' config entry and initializes required variables.
 *
 * @param filename Path to configuration file (used in error and warning messages).
 * @param config   The current config being parsed.
 * @param imported Whether the current config is imported from another file.
 *
 * @retval false in case of error.
 */
static bool inter_config_read_connection(const char *filename, const struct config_t *config, bool imported)
{
	const struct config_setting_t *setting = NULL;

	nullpo_retr(false, filename);
	nullpo_retr(false, config);

	if ((setting = libconfig->lookup(config, "inter_configuration/log/sql_connection")) == NULL) {
		if (imported)
			return true;
		ShowError("inter_config_read: inter_configuration/log/sql_connection was not found in %s!\n", filename);
		return false;
	}

	libconfig->setting_lookup_int(setting, "db_port", &logs->db_port);
	libconfig->setting_lookup_mutable_string(setting, "db_hostname", logs->db_ip, sizeof(logs->db_ip));
	libconfig->setting_lookup_mutable_string(setting, "db_username", logs->db_id, sizeof(logs->db_id));
	libconfig->setting_lookup_mutable_string(setting, "db_password", logs->db_pw, sizeof(logs->db_pw));
	libconfig->setting_lookup_mutable_string(setting, "db_database", logs->db_name, sizeof(logs->db_name));

	return true;
}

/**
 * Reads the 'inter_configuration/database_names' config entry and initializes required variables.
 *
 * @param filename Path to configuration file (used in error and warning messages).
 * @param config   The current config being parsed.
 * @param imported Whether the current config is imported from another file.
 *
 * @retval false in case of error.
 */
static bool inter_config_read_database_names(const char *filename, const struct config_t *config, bool imported)
{
	const struct config_setting_t *setting = NULL;

	nullpo_retr(false, filename);
	nullpo_retr(false, config);

	if ((setting = libconfig->lookup(config, "inter_configuration/database_names")) == NULL) {
		if (imported)
			return true;
		ShowError("inter_config_read: inter_configuration/database_names was not found in %s!\n", filename);
		return false;
	}

	libconfig->setting_lookup_mutable_string(setting, "autotrade_merchants_db", map->autotrade_merchants_db, sizeof(map->autotrade_merchants_db));
	libconfig->setting_lookup_mutable_string(setting, "autotrade_data_db", map->autotrade_data_db, sizeof(map->autotrade_data_db));
	libconfig->setting_lookup_mutable_string(setting, "npc_market_data_db", map->npc_market_data_db, sizeof(map->npc_market_data_db));
	libconfig->setting_lookup_mutable_string(setting, "npc_barter_data_db", map->npc_barter_data_db, sizeof(map->npc_barter_data_db));
	libconfig->setting_lookup_mutable_string(setting, "npc_expanded_barter_data_db", map->npc_expanded_barter_data_db, sizeof(map->npc_expanded_barter_data_db));

	if ((setting = libconfig->lookup(config, "inter_configuration/database_names/registry")) == NULL) {
		if (imported)
			return true;
		ShowError("inter_config_read: inter_configuration/database_names/registry was not found in %s!\n", filename);
		return false;
	}

	return mapreg->config_read_registry(filename, setting, imported);
}

/*=======================================
 *  Config reading utilities
 *---------------------------------------*/

/**
 * Looks up configuration "name" which is expect to have a final value of int, but may be specified by a string constant.
 *
 * If the config is a string, it will be looked up using script->get_constant function to find the actual integer value.
 *
 * @param[in]  setting        The setting to read.
 * @param[in]  name           The setting name to lookup.
 * @param[out] value          Where to output the read value (after constant resolving, if necessary)
 *
 * @retval true if it was read successfully or false if it could not read or resolve it.
 */
static bool map_setting_lookup_const(const struct config_setting_t *setting, const char *name, int *value)
{
	const char *str = NULL;

	nullpo_retr(false, name);
	nullpo_retr(false, value);

	if (libconfig->setting_lookup_int(setting, name, value)) {
		return true;
	}

	if (libconfig->setting_lookup_string(setting, name, &str)) {
		if (*str && script->get_constant(str, value))
			return true;
	}

	return false;
}

/**
 * Looks up configuration "name" which is expect to have a final value of int,
 * but may be specified by a string constant or an array of bitflag constants.
 *
 * If the config is a string, it will be looked up using script->get_constant function to find the actual integer value.
 * If the config is an array, each value will be read and added to the final bitmask.
 *
 * @param[in]  setting        The setting to read.
 * @param[in]  name           The setting name to lookup.
 * @param[out] value          Where to output the read value (after constant resolving, if necessary)
 *
 * @retval true if it was read successfully or false if it could not read or resolve it.
 */
static bool map_setting_lookup_const_mask(const struct config_setting_t *setting, const char *name, int *value)
{
	const struct config_setting_t *t = NULL;

	nullpo_retr(false, setting);
	nullpo_retr(false, name);
	nullpo_retr(false, value);

	if ((t = libconfig->setting_get_member(setting, name)) == NULL) {
		return false;
	}

	if (config_setting_is_scalar(t)) {
		const char *str = NULL;

		if (config_setting_is_number(t)) {
			*value = libconfig->setting_get_int(t);
			return true;
		}

		if ((str = libconfig->setting_get_string(t)) != NULL) {
			int i32 = -1;
			if (script->get_constant(str, &i32) && i32 >= 0) {
				*value = i32;
				return true;
			}
		}

		return false;
	}

	if (config_setting_is_aggregate(t) && libconfig->setting_length(t) >= 1) {
		const struct config_setting_t *elem = NULL;
		int i = 0;

		*value = 0;

		while ((elem = libconfig->setting_get_elem(t, i++)) != NULL) {
			const char *str = libconfig->setting_get_string(elem);
			int i32 = -1;

			if (str == NULL)
				return false;

			if (!script->get_constant(str, &i32) || i32 < 0)
				return false;

			*value |= i32;
		}

		return true;
	}

	return false;
}

/*=======================================
 *  MySQL Init
 *---------------------------------------*/
static int map_sql_init(void)
{
	// main db connection
	map->mysql_handle = SQL->Malloc();

	ShowInfo("Connecting to the Map DB Server....\n");
	if( SQL_ERROR == SQL->Connect(map->mysql_handle, map->server_id, map->server_pw, map->server_ip, map->server_port, map->server_db) )
		exit(EXIT_FAILURE);
	ShowStatus("connect success! (Map Server Connection)\n");

	if (map->default_codepage[0] != '\0')
		if ( SQL_ERROR == SQL->SetEncoding(map->mysql_handle, map->default_codepage) )
			Sql_ShowDebug(map->mysql_handle);

	return 0;
}

static int map_sql_close(void)
{
	ShowStatus("Close Map DB Connection....\n");
	SQL->Free(map->mysql_handle);
	map->mysql_handle = NULL;
	if (logs->config.sql_logs) {
		logs->sql_final();
	}
	return 0;
}

/**
 * Merges two zones into a new one
 * @param main the zone whose data must override the others upon conflict,
 *        e.g. enabling pvp on a town means that main is the pvp zone, while "other" is the towns previous zone
 *
 * @return the newly created zone from merging main and other
 **/
static struct map_zone_data *map_merge_zone(struct map_zone_data *main, struct map_zone_data *other)
{
	char newzone[MAP_ZONE_NAME_LENGTH * 2];
	struct map_zone_data *zone = NULL;
	int cursor, i, j;

	nullpo_retr(NULL, main);
	nullpo_retr(NULL, other);

	if (snprintf(newzone, MAP_ZONE_NAME_LENGTH * 2, "%s+%s", main->name, other->name) >= MAP_ZONE_NAME_LENGTH) {
		// In the unlikely case the name is too long and won't fit, there's a chance two different zones might be truncated to the same name.
		// Hash the concatenation of the two names if that happens, to minimize the chance of collisions.
		char newzone_temp[33];
		md5->string(newzone, newzone_temp);
		STATIC_ASSERT(MAP_ZONE_NAME_LENGTH > 32 + 12 + 12 + 2, "The next lines needs to be adjusted if MAP_ZONE_NAME_LENGTH is changed");
		snprintf(newzone, MAP_ZONE_NAME_LENGTH, "%s_", newzone_temp);
		size_t len = strlen(newzone);
		safestrncpy(newzone + len, main->name, len + 12);
		len = strlen(newzone);
		newzone[len++] = '+';
		newzone[len] = '\0';
		safestrncpy(newzone + len, main->name, len + 12);
	}

	if( (zone = strdb_get(map->zone_db, newzone)) )
		return zone;/* this zone has already been merged */

	CREATE(zone, struct map_zone_data, 1);
	safestrncpy(zone->name, newzone, MAP_ZONE_NAME_LENGTH);
	zone->merge_type = MZMT_NEVERMERGE;
	zone->disabled_skills_count = main->disabled_skills_count + other->disabled_skills_count;
	zone->disabled_items_count = main->disabled_items_count + other->disabled_items_count;
	zone->mapflags_count = main->mapflags_count + other->mapflags_count;
	zone->disabled_commands_count = main->disabled_commands_count + other->disabled_commands_count;
	zone->capped_skills_count = main->capped_skills_count + other->capped_skills_count;

	CREATE(zone->disabled_skills, struct map_zone_disabled_skill_entry *, zone->disabled_skills_count );
	for(i = 0, cursor = 0; i < main->disabled_skills_count; i++, cursor++ ) {
		CREATE(zone->disabled_skills[cursor], struct map_zone_disabled_skill_entry, 1 );
		memcpy(zone->disabled_skills[cursor], main->disabled_skills[i], sizeof(struct map_zone_disabled_skill_entry));
	}

	for(i = 0; i < other->disabled_skills_count; i++, cursor++ ) {
		CREATE(zone->disabled_skills[cursor], struct map_zone_disabled_skill_entry, 1 );
		memcpy(zone->disabled_skills[cursor], other->disabled_skills[i], sizeof(struct map_zone_disabled_skill_entry));
	}

	for(j = 0; j < main->cant_disable_items_count; j++) {
		for(i = 0; i < other->disabled_items_count; i++) {
			if( other->disabled_items[i] == main->cant_disable_items[j] ) {
				zone->disabled_items_count--;
				break;
			}
		}
	}

	CREATE(zone->disabled_items, int, zone->disabled_items_count );
	for(i = 0, cursor = 0; i < main->disabled_items_count; i++, cursor++ ) {
		zone->disabled_items[cursor] = main->disabled_items[i];
	}

	for(i = 0; i < other->disabled_items_count; i++) {
		for(j = 0; j < main->cant_disable_items_count; j++) {
			if( other->disabled_items[i] == main->cant_disable_items[j] ) {
				break;
			}
		}
		if( j != main->cant_disable_items_count )
			continue;
		zone->disabled_items[cursor] = other->disabled_items[i];
		cursor++;
	}

	CREATE(zone->mapflags, char *, zone->mapflags_count );
	for(i = 0, cursor = 0; i < main->mapflags_count; i++, cursor++ ) {
		CREATE(zone->mapflags[cursor], char, MAP_ZONE_MAPFLAG_LENGTH );
		safestrncpy(zone->mapflags[cursor], main->mapflags[i], MAP_ZONE_MAPFLAG_LENGTH);
	}

	for(i = 0; i < other->mapflags_count; i++, cursor++ ) {
		CREATE(zone->mapflags[cursor], char, MAP_ZONE_MAPFLAG_LENGTH );
		safestrncpy(zone->mapflags[cursor], other->mapflags[i], MAP_ZONE_MAPFLAG_LENGTH);
	}

	CREATE(zone->disabled_commands, struct map_zone_disabled_command_entry *, zone->disabled_commands_count);
	for(i = 0, cursor = 0; i < main->disabled_commands_count; i++, cursor++ ) {
		CREATE(zone->disabled_commands[cursor], struct map_zone_disabled_command_entry, 1);
		memcpy(zone->disabled_commands[cursor], main->disabled_commands[i], sizeof(struct map_zone_disabled_command_entry));
	}

	for(i = 0; i < other->disabled_commands_count; i++, cursor++ ) {
		CREATE(zone->disabled_commands[cursor], struct map_zone_disabled_command_entry, 1);
		memcpy(zone->disabled_commands[cursor], other->disabled_commands[i], sizeof(struct map_zone_disabled_command_entry));
	}

	CREATE(zone->capped_skills, struct map_zone_skill_damage_cap_entry *, zone->capped_skills_count);
	for(i = 0, cursor = 0; i < main->capped_skills_count; i++, cursor++ ) {
		CREATE(zone->capped_skills[cursor], struct map_zone_skill_damage_cap_entry, 1);
		memcpy(zone->capped_skills[cursor], main->capped_skills[i], sizeof(struct map_zone_skill_damage_cap_entry));
	}

	for(i = 0; i < other->capped_skills_count; i++, cursor++ ) {
		CREATE(zone->capped_skills[cursor], struct map_zone_skill_damage_cap_entry, 1);
		memcpy(zone->capped_skills[cursor], other->capped_skills[i], sizeof(struct map_zone_skill_damage_cap_entry));
	}

	zone->info.merged = 1;
	strdb_put(map->zone_db, newzone, zone);
	return zone;
}

static void map_zone_change2(int m, struct map_zone_data *zone)
{
	const char *empty = "";

	if (zone == NULL)
		return;
	Assert_retv(m >= 0 && m < map->count);
	if( map->list[m].zone == zone )
		return;

	if( !map->list[m].zone->info.merged ) /* we don't update it for merged zones! */
		map->list[m].prev_zone = map->list[m].zone;

	if( map->list[m].zone_mf_count )
		map->zone_remove(m);

	if( zone->merge_type == MZMT_MERGEABLE && map->list[m].prev_zone->merge_type != MZMT_NEVERMERGE ) {
		zone = map->merge_zone(zone,map->list[m].prev_zone);
	}

	map->zone_apply(m,zone,empty,empty,empty);
}
/* when changing from a mapflag to another during runtime */
static void map_zone_change(int m, struct map_zone_data *zone, const char *start, const char *buffer, const char *filepath)
{
	Assert_retv(m >= 0 && m < map->count);
	map->list[m].prev_zone = map->list[m].zone;

	if( map->list[m].zone_mf_count )
		map->zone_remove(m);
	map->zone_apply(m,zone,start,buffer,filepath);
}
/* removes previous mapflags from this map */
static void map_zone_remove(int m)
{
	char flag[MAP_ZONE_MAPFLAG_LENGTH], params[MAP_ZONE_MAPFLAG_LENGTH];
	unsigned short k;
	const char *empty = "";
	Assert_retv(m >= 0 && m < map->count);
	for(k = 0; k < map->list[m].zone_mf_count; k++) {
		size_t len = strlen(map->list[m].zone_mf[k]),j;
		params[0] = '\0';
		memcpy(flag, map->list[m].zone_mf[k], MAP_ZONE_MAPFLAG_LENGTH);
		for(j = 0; j < len; j++) {
			if( flag[j] == '\t' ) {
				memcpy(params, &flag[j+1], len - j);
				flag[j] = '\0';
				break;
			}
		}

		npc->parse_mapflag(map->list[m].name,empty,flag,params,empty,empty,empty, NULL);
		aFree(map->list[m].zone_mf[k]);
		map->list[m].zone_mf[k] = NULL;
	}

	aFree(map->list[m].zone_mf);
	map->list[m].zone_mf = NULL;
	map->list[m].zone_mf_count = 0;
}
// this one removes every flag, even if they were previously turned on before
// the current zone was applied
static void map_zone_remove_all(int m)
{
	Assert_retv(m >= 0 && m < map->count);

	for (unsigned short k = 0; k < map->list[m].zone_mf_count; k++) {
		char flag[MAP_ZONE_MAPFLAG_LENGTH];

		memcpy(flag, map->list[m].zone_mf[k], MAP_ZONE_MAPFLAG_LENGTH);
		strtok(flag, "\t");

		npc->parse_mapflag(map->list[m].name, "", flag, "off", "", "", "", NULL);
		aFree(map->list[m].zone_mf[k]);
		map->list[m].zone_mf[k] = NULL;
	}

	aFree(map->list[m].zone_mf);
	map->list[m].zone_mf = NULL;
	map->list[m].zone_mf_count = 0;
}
static inline void map_zone_mf_cache_add(int m, char *rflag)
{
	Assert_retv(m >= 0 && m < map->count);
	RECREATE(map->list[m].zone_mf, char *, ++map->list[m].zone_mf_count);
	CREATE(map->list[m].zone_mf[map->list[m].zone_mf_count - 1], char, MAP_ZONE_MAPFLAG_LENGTH);
	safestrncpy(map->list[m].zone_mf[map->list[m].zone_mf_count - 1], rflag, MAP_ZONE_MAPFLAG_LENGTH);
}

/* TODO: introduce enumerations to each mapflag so instead of reading the string a number of times we read it only once and use its value wherever we need */
/* cache previous values to revert */
static bool map_zone_mf_cache(int m, char *flag, char *params)
{
	char rflag[MAP_ZONE_MAPFLAG_LENGTH];
	int state = 1;

	nullpo_retr(false, flag);
	nullpo_retr(false, params);
	Assert_retr(false, m >= 0 && m < map->count);

	if (params[0] != '\0' && strcmpi(params, "off") == 0)
		state = 0;

	if (strcmpi(flag, "nomemo") == 0) {
		if (state != 0 && map->list[m].flag.nomemo != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "nomemo\toff");
			else if (map->list[m].flag.nomemo == 0)
				map_zone_mf_cache_add(m, "nomemo");
		}
	} else if (strcmpi(flag, "noteleport") == 0) {
		if (state != 0 && map->list[m].flag.noteleport != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "noteleport\toff");
			else if (map->list[m].flag.noteleport == 0)
				map_zone_mf_cache_add(m, "noteleport");
		}
	} else if (strcmpi(flag, "nosave") == 0) {
#if 0 /* not yet supported to be reversed */
		char savemap[32];
		int savex, savey;
		if (state == 0) {
			if (map->list[m].flag.nosave != 0) {
				sprintf(rflag, "nosave\tSavePoint");
				map_zone_mf_cache_add(m, nosave);
			}
		} else if (strcmpi(params, "SavePoint") == 0) {
			if (map->list[m].save.map) {
				sprintf(rflag, "nosave\t%s,%d,%d", mapindex_id2name(map->list[m].save.map), map->list[m].save.x, map->list[m].save.y);
			} else
				sprintf(rflag, "nosave\t%s,%d,%d", mapindex_id2name(map->list[m].save.map), map->list[m].save.x, map->list[m].save.y);
			map_zone_mf_cache_add(m, nosave);
		} else if (sscanf(params, "%31[^,],%d,%d", savemap, &savex, &savey) == 3) {
			if (map->list[m].save.map) {
				sprintf(rflag, "nosave\t%s,%d,%d", mapindex_id2name(map->list[m].save.map), map->list[m].save.x, map->list[m].save.y);
				map_zone_mf_cache_add(m, nosave);
			}
		}
#endif // 0
	} else if (strcmpi(flag, "nobranch") == 0) {
		if (state != 0 && map->list[m].flag.nobranch != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "nobranch\toff");
			else if (map->list[m].flag.nobranch != 0)
				map_zone_mf_cache_add(m, "nobranch");
		}
	} else if (strcmpi(flag, "nozenypenalty") == 0) {
		if (state != 0 && map->list[m].flag.nozenypenalty != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "nozenypenalty\toff");
			else if (map->list[m].flag.nozenypenalty != 0)
				map_zone_mf_cache_add(m, "nozenypenalty");
		}
	} else if (strcmpi(flag, "pvp") == 0) {
		if (state != 0 && map->list[m].flag.pvp != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "pvp\toff");
			else if (map->list[m].flag.pvp != 0)
				map_zone_mf_cache_add(m, "pvp");
		}
	} else if (strcmpi(flag, "pvp_noparty") == 0) {
		if (state != 0 && map->list[m].flag.pvp_noparty != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "pvp_noparty\toff");
			else if (map->list[m].flag.pvp_noparty != 0)
				map_zone_mf_cache_add(m, "pvp_noparty");
		}
	} else if (strcmpi(flag, "pvp_noguild") == 0) {
		if (state != 0 && map->list[m].flag.pvp_noguild != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "pvp_noguild\toff");
			else if (map->list[m].flag.pvp_noguild != 0)
				map_zone_mf_cache_add(m, "pvp_noguild");
		}
	} else if (strcmpi(flag, "gvg") == 0) {
		if (state != 0 && map->list[m].flag.gvg != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "gvg\toff");
			else if (map->list[m].flag.gvg != 0)
				map_zone_mf_cache_add(m, "gvg");
		}
	} else if (strcmpi(flag, "gvg_noparty") == 0) {
		if (state != 0 && map->list[m].flag.gvg_noparty != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "gvg_noparty\toff");
			else if (map->list[m].flag.gvg_noparty != 0)
				map_zone_mf_cache_add(m, "gvg_noparty");
		}
	} else if (strcmpi(flag, "notrade") == 0) {
		if (state != 0 && map->list[m].flag.notrade != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "notrade\toff");
			else if (map->list[m].flag.notrade != 0)
				map_zone_mf_cache_add(m, "notrade");
		}
	} else if (strcmpi(flag, "noskill") == 0) {
		if (state != 0 && map->list[m].flag.noskill != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "noskill\toff");
			else if (map->list[m].flag.noskill != 0)
				map_zone_mf_cache_add(m, "noskill");
		}
	} else if (strcmpi(flag, "nowarp") == 0) {
		if (state != 0 && map->list[m].flag.nowarp != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "nowarp\toff");
			else if (map->list[m].flag.nowarp == 0)
				map_zone_mf_cache_add(m, "nowarp");
		}
	} else if (strcmpi(flag, "partylock") == 0) {
		if (state != 0 && map->list[m].flag.partylock != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "partylock\toff");
			else if (map->list[m].flag.partylock != 0)
				map_zone_mf_cache_add(m, "partylock");
		}
	} else if (strcmpi(flag, "noicewall") == 0) {
		if (state != 0 && map->list[m].flag.noicewall != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "noicewall\toff");
			else if (map->list[m].flag.noicewall != 0)
				map_zone_mf_cache_add(m, "noicewall");
		}
	} else if (strcmpi(flag, "snow") == 0) {
		if (state != 0 && map->list[m].flag.snow != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "snow\toff");
			else if (map->list[m].flag.snow != 0)
				map_zone_mf_cache_add(m, "snow");
		}
	} else if (strcmpi(flag, "fog") == 0) {
		if (state != 0 && map->list[m].flag.fog != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "fog\toff");
			else if (map->list[m].flag.fog != 0)
				map_zone_mf_cache_add(m, "fog");
		}
	} else if (strcmpi(flag, "sakura") == 0) {
		if (state != 0 && map->list[m].flag.sakura != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "sakura\toff");
			else if (map->list[m].flag.sakura != 0)
				map_zone_mf_cache_add(m, "sakura");
		}
	} else if (strcmpi(flag, "leaves") == 0) {
		if (state != 0 && map->list[m].flag.leaves != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "leaves\toff");
			else if (map->list[m].flag.leaves != 0)
				map_zone_mf_cache_add(m, "leaves");
		}
	} else if (strcmpi(flag, "clouds") == 0) {
		if (state != 0 && map->list[m].flag.clouds != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "clouds\toff");
			else if (map->list[m].flag.clouds != 0)
				map_zone_mf_cache_add(m, "clouds");
		}
	} else if (strcmpi(flag, "clouds2") == 0) {
		if (state != 0 && map->list[m].flag.clouds2 != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "clouds2\toff");
			else if (map->list[m].flag.clouds2 != 0)
				map_zone_mf_cache_add(m, "clouds2");
		}
	} else if (strcmpi(flag, "fireworks") == 0) {
		if (state != 0 && map->list[m].flag.fireworks != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "fireworks\toff");
			else if (map->list[m].flag.fireworks != 0)
				map_zone_mf_cache_add(m, "fireworks");
		}
	} else if (strcmpi(flag, "gvg_castle") == 0) {
		if (state != 0 && map->list[m].flag.gvg_castle != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "gvg_castle\toff");
			else if (map->list[m].flag.gvg_castle != 0)
				map_zone_mf_cache_add(m, "gvg_castle");
		}
	} else if (strcmpi(flag, "gvg_dungeon") == 0) {
		if (state != 0 && map->list[m].flag.gvg_dungeon != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "gvg_dungeon\toff");
			else if (map->list[m].flag.gvg_dungeon != 0)
				map_zone_mf_cache_add(m, "gvg_dungeon");
		}
	} else if (strcmpi(flag, "nightenabled") == 0) {
		if (state != 0 && map->list[m].flag.nightenabled != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "nightenabled\toff");
			else if (map->list[m].flag.nightenabled != 0)
				map_zone_mf_cache_add(m, "nightenabled");
		}
	} else if (strcmpi(flag, "nobaseexp") == 0) {
		if (state != 0 && map->list[m].flag.nobaseexp != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "nobaseexp\toff");
			else if (map->list[m].flag.nobaseexp != 0)
				map_zone_mf_cache_add(m, "nobaseexp");
		}
	} else if (strcmpi(flag, "nojobexp") == 0) {
		if (state != 0 && map->list[m].flag.nojobexp != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "nojobexp\toff");
			else if (map->list[m].flag.nojobexp != 0)
				map_zone_mf_cache_add(m, "nojobexp");
		}
	} else if (strcmpi(flag, "nomobloot") == 0) {
		if (state != 0 && map->list[m].flag.nomobloot != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "nomobloot\toff");
			else if (map->list[m].flag.nomobloot != 0)
				map_zone_mf_cache_add(m, "nomobloot");
		}
	} else if (strcmpi(flag, "nomvploot") == 0) {
		if (state != 0 && map->list[m].flag.nomvploot != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "nomvploot\toff");
			else if (map->list[m].flag.nomvploot != 0)
				map_zone_mf_cache_add(m, "nomvploot");
		}
	} else if (strcmpi(flag, "noreturn") == 0) {
		if (state != 0 && map->list[m].flag.noreturn != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "noreturn\toff");
			else if (map->list[m].flag.noreturn != 0)
				map_zone_mf_cache_add(m, "noreturn");
		}
	} else if (strcmpi(flag, "nowarpto") == 0) {
		if (state != 0 && map->list[m].flag.nowarpto != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "nowarpto\toff");
			else if (map->list[m].flag.nowarpto == 0)
				map_zone_mf_cache_add(m, "nowarpto");
		}
	} else if (strcmpi(flag, "pvp_nightmaredrop") == 0) {
		if (state != 0 && map->list[m].flag.pvp_nightmaredrop != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "pvp_nightmaredrop\toff");
			else if (map->list[m].flag.pvp_nightmaredrop != 0)
				map_zone_mf_cache_add(m, "pvp_nightmaredrop");
		}
#if 0 /* not yet fully supported */
		char drop_arg1[16], drop_arg2[16];
		int drop_per = 0; noexp
			if (sscanf(w4, "%15[^,],%15[^,],%d", drop_arg1, drop_arg2, &drop_per) == 3) {
				int drop_id = 0, drop_type = 0;
				if (strcmpi(drop_arg1, "random") == 0)
					drop_id = -1;
				else if (itemdb->exists((drop_id = atoi(drop_arg1))) == NULL)
					drop_id = 0;
				if (strcmpi(drop_arg2, "inventory") == 0)
					drop_type = 1;
				else if (strcmpi(drop_arg2, "equip") == 0)
					drop_type = 2;
				else if (strcmpi(drop_arg2, "all") == 0)
					drop_type = 3;

				if (drop_id != 0) {
					int i;
					for (i = 0; i < MAX_DROP_PER_MAP; i++) {
						if (map->list[m].drop_list[i].drop_id == 0) {
							map->list[m].drop_list[i].drop_id = drop_id;
							map->list[m].drop_list[i].drop_type = drop_type;
							map->list[m].drop_list[i].drop_per = drop_per;
							break;
						}
					}
					map->list[m].flag.pvp_nightmaredrop = 1;
				}
			} else if (state == 0) //Disable
				map->list[m].flag.pvp_nightmaredrop = 0;
#endif // 0
	} else if (strcmpi(flag, "zone") == 0) {
		ShowWarning("You can't add a zone through a zone! ERROR, skipping for '%s'...\n", map->list[m].name);
		return true;
	} else if (strcmpi(flag, "nocommand") == 0) {
		/* implementation may be incomplete */
		if (state != 0 && sscanf(params, "%d", &state) == 1) {
			sprintf(rflag, "nocommand\t%s", params);
			map_zone_mf_cache_add(m, rflag);
		} else if (!state && map->list[m].nocommand != 0) {
			sprintf(rflag, "nocommand\t%d", map->list[m].nocommand);
			map_zone_mf_cache_add(m, rflag);
		} else if (map->list[m].nocommand != 0) {
			map_zone_mf_cache_add(m, "nocommand\toff");
		}
	} else if (strcmpi(flag, "nodrop") == 0) {
		if (state != 0 && map->list[m].flag.nodrop != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "nodrop\toff");
			else if (map->list[m].flag.nodrop != 0)
				map_zone_mf_cache_add(m, "nodrop");
		}
	} else if (strcmpi(flag, "jexp") == 0) {
		if (state == 0) {
			if (map->list[m].jexp != 100) {
				sprintf(rflag, "jexp\t%d", map->list[m].jexp);
				map_zone_mf_cache_add(m, rflag);
			}
		}
		if (sscanf(params, "%d", &state) == 1) {
			if (state != map->list[m].jexp) {
				sprintf(rflag, "jexp\t%s", params);
				map_zone_mf_cache_add(m, rflag);
			}
		}
	} else if (strcmpi(flag, "bexp") == 0) {
		if (state == 0) {
			if (map->list[m].bexp != 100) {
				sprintf(rflag, "bexp\t%d", map->list[m].bexp);
				map_zone_mf_cache_add(m, rflag);
			}
		}
		if (sscanf(params, "%d", &state) == 1) {
			if (state != map->list[m].bexp) {
				sprintf(rflag, "bexp\t%s", params);
				map_zone_mf_cache_add(m, rflag);
			}
		}
	} else if (strcmpi(flag, "specialpopup") == 0) {
		if (state != 0) {
			if (sscanf(params, "%d", &state) == 1) {
				if (state != map->list[m].flag.specialpopup) {
					sprintf(rflag, "specialpopup\t%s", params);
					map_zone_mf_cache_add(m, rflag);
				}
			}
		} else {
			map_zone_mf_cache_add(m, "specialpopup\toff");
		}
	} else if (strcmpi(flag, "novending") == 0) {
		if (state != 0 && map->list[m].flag.novending != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "novending\toff");
			else if (map->list[m].flag.novending != 0)
				map_zone_mf_cache_add(m, "novending");
		}
	} else if (strcmpi(flag, "loadevent") == 0) {
		if (state != 0 && map->list[m].flag.loadevent != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "loadevent\toff");
			else if (map->list[m].flag.loadevent != 0)
				map_zone_mf_cache_add(m, "loadevent");
		}
	} else if (strcmpi(flag, "nochat") == 0) {
		if (state != 0 && map->list[m].flag.nochat != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "nochat\toff");
			else if (map->list[m].flag.nochat != 0)
				map_zone_mf_cache_add(m, "nochat");
		}
	} else if (strcmpi(flag, "noexppenalty") == 0) {
		if (state != 0 && map->list[m].flag.noexppenalty != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "noexppenalty\toff");
			else if (map->list[m].flag.noexppenalty != 0)
				map_zone_mf_cache_add(m, "noexppenalty");
		}
	} else if (strcmpi(flag, "guildlock") == 0) {
		if (state != 0 && map->list[m].flag.guildlock != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "guildlock\toff");
			else if (map->list[m].flag.guildlock != 0)
				map_zone_mf_cache_add(m, "guildlock");
		}
	} else if (strcmpi(flag, "town") == 0) {
		if (state != 0 && map->list[m].flag.town != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "town\toff");
			else if (map->list[m].flag.town == 0)
				map_zone_mf_cache_add(m, "town");
		}
	} else if (strcmpi(flag, "autotrade") == 0) {
		if (state != 0 && map->list[m].flag.autotrade != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "autotrade\toff");
			else if (map->list[m].flag.autotrade == 0)
				map_zone_mf_cache_add(m, "autotrade");
		}
	} else if (strcmpi(flag, "allowks") == 0) {
		if (state != 0 && map->list[m].flag.allowks != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "allowks\toff");
			else if (map->list[m].flag.allowks == 0)
				map_zone_mf_cache_add(m, "allowks");
		}
	} else if (strcmpi(flag, "monster_noteleport") == 0) {
		if (state != 0 && map->list[m].flag.monster_noteleport != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "monster_noteleport\toff");
			else if (map->list[m].flag.monster_noteleport != 0)
				map_zone_mf_cache_add(m, "monster_noteleport");
		}
	} else if (strcmpi(flag, "pvp_nocalcrank") == 0) {
		if (state != 0 && map->list[m].flag.pvp_nocalcrank != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "pvp_nocalcrank\toff");
			else if (map->list[m].flag.pvp_nocalcrank != 0)
				map_zone_mf_cache_add(m, "pvp_nocalcrank");
		}
	} else if (strcmpi(flag, "battleground") == 0) {
		if (state != 0 && map->list[m].flag.battleground != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "battleground\toff");
			else if (map->list[m].flag.battleground != 0)
				map_zone_mf_cache_add(m, "battleground");
		}
	} else if (strcmpi(flag, "cvc") == 0) {
		if (state != 0 && map->list[m].flag.cvc != 0) {
			;/* nothing to do */
		} else {
			if (state != 0)
				map_zone_mf_cache_add(m, "cvc\toff");
			else if (map->list[m].flag.cvc)
				map_zone_mf_cache_add(m, "cvc");
		}
	} else if (strcmpi(flag, "reset") == 0) {
		if (state != 0 && map->list[m].flag.reset != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "reset\toff");
			else if (map->list[m].flag.reset != 0)
				map_zone_mf_cache_add(m, "reset");
		}
	} else if (strcmpi(flag, "notomb") == 0) {
		if (state != 0 && map->list[m].flag.notomb != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "notomb\toff");
			else if (map->list[m].flag.notomb != 0)
				map_zone_mf_cache_add(m, "notomb");
		}
	} else if (strcmpi(flag, "nocashshop") == 0) {
		if (state != 0 && map->list[m].flag.nocashshop != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "nocashshop\toff");
			else if (map->list[m].flag.nocashshop != 0)
				map_zone_mf_cache_add(m, "nocashshop");
		}
	} else if (strcmpi(flag, "noautoloot") == 0) {
		if (state != 0 && map->list[m].flag.noautoloot != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "noautoloot\toff");
			else if (map->list[m].flag.noautoloot != 0)
				map_zone_mf_cache_add(m, "noautoloot");
		}
	} else if (strcmpi(flag, "noviewid") == 0) {
		if (state != 0 && map->list[m].flag.noviewid != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "noviewid\toff");
			else if (map->list[m].flag.noviewid != 0)
				map_zone_mf_cache_add(m, "noviewid");
		}
	} else if (strcmpi(flag, "pairship_startable") == 0) {
		if (state != 0 && map->list[m].flag.pairship_startable != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "pairship_startable\toff");
			else if (map->list[m].flag.pairship_startable != 0)
				map_zone_mf_cache_add(m, "pairship_startable");
		}
	} else if (strcmpi(flag, "pairship_endable") == 0) {
		if (state != 0 && map->list[m].flag.pairship_endable != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "pairship_endable\toff");
			else if (map->list[m].flag.pairship_endable != 0)
				map_zone_mf_cache_add(m, "pairship_endable");
		}
	} else if (strcmpi(flag, "nostorage") == 0) {
		if (state == 0) {
			if (map->list[m].flag.nostorage != 0) {
				sprintf(rflag, "nostorage\t%d", map->list[m].flag.nostorage);
				map_zone_mf_cache_add(m, rflag);
			}
		}
		if (sscanf(params, "%d", &state) == 1) {
			if (state != map->list[m].flag.nostorage) {
				sprintf(rflag, "nostorage\t%d", state);
				map_zone_mf_cache_add(m, rflag);
			}
		}
	} else if (strcmpi(flag, "nogstorage") == 0) {
		if (state == 0) {
			if (map->list[m].flag.nogstorage != 0) {
				sprintf(rflag, "nogstorage\t%d", map->list[m].flag.nogstorage);
				map_zone_mf_cache_add(m, rflag);
			}
		}
		if (sscanf(params, "%d", &state) == 1) {
			if (state != map->list[m].flag.nogstorage) {
				sprintf(rflag, "nogstorage\t%d", state);
				map_zone_mf_cache_add(m, rflag);
			}
		}
	} else if (strcmpi(flag, "nomapchannelautojoin") == 0) {
		if (state != 0 && map->list[m].flag.chsysnolocalaj != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "nomapchannelautojoin\toff");
			else if (map->list[m].flag.chsysnolocalaj != 0)
				map_zone_mf_cache_add(m, "nomapchannelautojoin");
		}
	} else if (strcmpi(flag, "noknockback") == 0) {
		if (state != 0 && map->list[m].flag.noknockback != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "noknockback\toff");
			else if (map->list[m].flag.noknockback != 0)
				map_zone_mf_cache_add(m, "noknockback");
		}
	} else if (strcmpi(flag, "src4instance") == 0) {
		if (state != 0 && map->list[m].flag.src4instance != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "src4instance\toff");
			else if (map->list[m].flag.src4instance != 0)
				map_zone_mf_cache_add(m, "src4instance");
		}
	} else if (strcmpi(flag, "cvc") == 0) {
		if (state != 0 && map->list[m].flag.cvc != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "cvc\toff");
			else if (map->list[m].flag.cvc != 0)
				map_zone_mf_cache_add(m, "cvc");
		}
	} else if (strcmpi(flag, "nopenalty") == 0) {
		if (state != 0 && map->list[m].flag.noexppenalty != 0) /* they are applied together, no need to check both */
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "nopenalty\toff");
			else if (map->list[m].flag.noexppenalty != 0)
				map_zone_mf_cache_add(m, "nopenalty");
		}
	} else if (strcmpi(flag, "noexp") == 0) {
		if (state != 0 && map->list[m].flag.nobaseexp != 0) /* they are applied together, no need to check both */
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "noexp\toff");
			else if (map->list[m].flag.nobaseexp != 0)
				map_zone_mf_cache_add(m, "noexp");
		}
	} else if (strcmpi(flag, "noloot") == 0) {
		if (state != 0 && map->list[m].flag.nomobloot != 0) /* they are applied together, no need to check both */
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "noloot\toff");
			else if (map->list[m].flag.nomobloot != 0)
				map_zone_mf_cache_add(m, "noloot");
		}
	} else if (strcmpi(flag, "nosendmail") == 0) {
		if (state != 0 && map->list[m].flag.nosendmail != 0)
			;/* nothing to do */
		else {
			if (state != 0)
				map_zone_mf_cache_add(m, "nosendmail\toff");
			else if (map->list[m].flag.nosendmail != 0)
				map_zone_mf_cache_add(m, "nosendmail");
		}
	}

	// Map Zones
	else if (strcmpi(flag, "adjust_unit_duration") == 0) {
		int skill_id;
		char skill_name[MAX_SKILL_NAME_LENGTH], modifier[MAP_ZONE_MAPFLAG_LENGTH];

		modifier[0] = '\0';
		safestrncpy(skill_name, params, MAX_SKILL_NAME_LENGTH);
		int len = (int)strlen(skill_name);

		for (int k = 0; k < len; k++) {
			if (skill_name[k] == '\t') {
				memcpy(modifier, &skill_name[k + 1], len - k);
				skill_name[k] = '\0';
				break;
			}
		}

		if (modifier[0] == '\0'
		 || (skill_id = skill->name2id(skill_name)) == 0
		 || skill->get_unit_id(skill->name2id(skill_name), 1, 0) == 0
		 || atoi(modifier) < 1 || atoi(modifier) > USHRT_MAX
		   ) {
			;/* we don't mind it, the server will take care of it next. */
		} else {
			int idx = map->list[m].unit_count;
			int k;
			ARR_FIND(0, idx, k, map->list[m].units[k]->skill_id == skill_id);

			if (k < idx) {
				if (atoi(modifier) != map->list[m].units[k]->modifier) {
					sprintf(rflag, "adjust_unit_duration\t%s\t%d", skill_name, map->list[m].units[k]->modifier);
					map_zone_mf_cache_add(m, rflag);
				}
			} else {
				sprintf(rflag, "adjust_unit_duration\t%s\t100", skill_name);
				map_zone_mf_cache_add(m, rflag);
			}
		}
	} else if (strcmpi(flag, "adjust_skill_damage") == 0) {
		int skill_id;
		char skill_name[MAX_SKILL_NAME_LENGTH], modifier[MAP_ZONE_MAPFLAG_LENGTH];

		modifier[0] = '\0';
		safestrncpy(skill_name, params, MAX_SKILL_NAME_LENGTH);
		int len = (int)strlen(skill_name);

		for (int k = 0; k < len; k++) {
			if (skill_name[k] == '\t') {
				memcpy(modifier, &skill_name[k + 1], len - k);
				skill_name[k] = '\0';
				break;
			}
		}

		if (modifier[0] == '\0'
		 || (skill_id = skill->name2id(skill_name)) == 0
		 || atoi(modifier) < 1
		 || atoi(modifier) > USHRT_MAX
		   ) {
			;/* we don't mind it, the server will take care of it next. */
		} else {
			int idx = map->list[m].skill_count;
			int k;
			ARR_FIND(0, idx, k, map->list[m].skills[k]->skill_id == skill_id);

			if (k < idx) {
				if (atoi(modifier) != map->list[m].skills[k]->modifier) {
					sprintf(rflag, "adjust_skill_damage\t%s\t%d", skill_name, map->list[m].skills[k]->modifier);
					map_zone_mf_cache_add(m, rflag);
				}
			} else {
				sprintf(rflag, "adjust_skill_damage\t%s\t100", skill_name);
				map_zone_mf_cache_add(m, rflag);
			}

		}
	} else if (strcmpi(flag, "invincible_time_inc") == 0) {
		if (state == 0) {
			if (map->list[m].invincible_time_inc != 0) {
				sprintf(rflag, "invincible_time_inc\t%u", map->list[m].invincible_time_inc);
				map_zone_mf_cache_add(m, rflag);
			}
		}
		if (sscanf(params, "%d", &state) == 1) {
			if (state != map->list[m].invincible_time_inc) {
				sprintf(rflag, "invincible_time_inc\t%s", params);
				map_zone_mf_cache_add(m, rflag);
			}
		}
	} else if (strcmpi(flag, "weapon_damage_rate") == 0) {
		if (state == 0) {
			if (map->list[m].weapon_damage_rate != 100) {
				sprintf(rflag, "weapon_damage_rate\t%d", map->list[m].weapon_damage_rate);
				map_zone_mf_cache_add(m, rflag);
			}
		}
		if (sscanf(params, "%d", &state) == 1) {
			if (state != map->list[m].weapon_damage_rate) {
				sprintf(rflag, "weapon_damage_rate\t%s", params);
				map_zone_mf_cache_add(m, rflag);
			}
		}
	} else if (strcmpi(flag, "magic_damage_rate")) {
		if (state == 0) {
			if (map->list[m].magic_damage_rate != 100) {
				sprintf(rflag, "magic_damage_rate\t%d", map->list[m].magic_damage_rate);
				map_zone_mf_cache_add(m, rflag);
			}
		}
		if (sscanf(params, "%d", &state) == 1) {
			if (state != map->list[m].magic_damage_rate) {
				sprintf(rflag, "magic_damage_rate\t%s", params);
				map_zone_mf_cache_add(m, rflag);
			}
		}
	} else if (strcmpi(flag, "misc_damage_rate") == 0) {
		if (state == 0) {
			if (map->list[m].misc_damage_rate != 100) {
				sprintf(rflag, "misc_damage_rate\t%d", map->list[m].misc_damage_rate);
				map_zone_mf_cache_add(m, rflag);
			}
		}
		if (sscanf(params, "%d", &state) == 1) {
			if (state != map->list[m].misc_damage_rate) {
				sprintf(rflag, "misc_damage_rate\t%s", params);
				map_zone_mf_cache_add(m, rflag);
			}
		}
	} else if (strcmpi(flag, "short_damage_rate") == 0) {
		if (state == 0) {
			if (map->list[m].short_damage_rate != 100) {
				sprintf(rflag, "short_damage_rate\t%d", map->list[m].short_damage_rate);
				map_zone_mf_cache_add(m, rflag);
			}
		}
		if (sscanf(params, "%d", &state) == 1) {
			if (state != map->list[m].short_damage_rate) {
				sprintf(rflag, "short_damage_rate\t%s", params);
				map_zone_mf_cache_add(m, rflag);
			}
		}
	} else if (strcmpi(flag, "long_damage_rate") == 0) {
		if (state == 0) {
			if (map->list[m].long_damage_rate != 100) {
				sprintf(rflag, "long_damage_rate\t%d", map->list[m].long_damage_rate);
				map_zone_mf_cache_add(m, rflag);
			}
		}
		if (sscanf(params, "%d", &state) == 1) {
			if (state != map->list[m].long_damage_rate) {
				sprintf(rflag, "long_damage_rate\t%s", params);
				map_zone_mf_cache_add(m, rflag);
			}
		}
	} else if (strcmpi(flag, "nopet") == 0) {
		if (state == 0) {
			if (map->list[m].flag.nopet != 0) {
				sprintf(rflag, "nopet\t%d", map->list[m].flag.nopet);
				map_zone_mf_cache_add(m, rflag);
			}
		}
		if (sscanf(params, "%d", &state) == 1) {
			if (state != map->list[m].flag.nopet) {
				sprintf(rflag, "nopet\t%d", state);
				map_zone_mf_cache_add(m, rflag);
			}
		}
	}

	return false;
}

static void map_zone_apply(int m, struct map_zone_data *zone, const char *start, const char *buffer, const char *filepath)
{
	int i;
	const char *empty = "";
	char flag[MAP_ZONE_MAPFLAG_LENGTH], params[MAP_ZONE_MAPFLAG_LENGTH];
	Assert_retv(m >= 0 && m < map->count);
	nullpo_retv(zone);
	map->list[m].zone = zone;
	for(i = 0; i < zone->mapflags_count; i++) {
		size_t len = strlen(zone->mapflags[i]);
		int k;
		params[0] = '\0';
		memcpy(flag, zone->mapflags[i], MAP_ZONE_MAPFLAG_LENGTH);
		for(k = 0; k < len; k++) {
			if( flag[k] == '\t' ) {
				memcpy(params, &flag[k+1], len - k);
				flag[k] = '\0';
				break;
			}
		}

		if( map->zone_mf_cache(m,flag,params) )
			continue;

		npc->parse_mapflag(map->list[m].name, empty, flag, params, start, buffer, filepath, NULL);
	}
}
/* used on npc load and reload to apply all "Normal" and "PK Mode" zones */
static void map_zone_init(void)
{
	char flag[MAP_ZONE_MAPFLAG_LENGTH], params[MAP_ZONE_MAPFLAG_LENGTH];
	struct map_zone_data *zone;
	const char *empty = "";
	int i,k,j;

	zone = &map->zone_all;

	for(i = 0; i < zone->mapflags_count; i++) {
		size_t len = strlen(zone->mapflags[i]);
		params[0] = '\0';
		memcpy(flag, zone->mapflags[i], MAP_ZONE_MAPFLAG_LENGTH);
		for(k = 0; k < len; k++) {
			if( flag[k] == '\t' ) {
				memcpy(params, &flag[k+1], len - k);
				flag[k] = '\0';
				break;
			}
		}

		for(j = 0; j < map->count; j++) {
			if( map->list[j].zone == zone ) {
				if( map->zone_mf_cache(j,flag,params) )
					break;
				npc->parse_mapflag(map->list[j].name, empty, flag, params, empty, empty, empty, NULL);
			}
		}
	}

	if( battle_config.pk_mode ) {
		zone = &map->zone_pk;
		for(i = 0; i < zone->mapflags_count; i++) {
			size_t len = strlen(zone->mapflags[i]);
			params[0] = '\0';
			memcpy(flag, zone->mapflags[i], MAP_ZONE_MAPFLAG_LENGTH);
			for(k = 0; k < len; k++) {
				if( flag[k] == '\t' ) {
					memcpy(params, &flag[k+1], len - k);
					flag[k] = '\0';
					break;
				}
			}
			for(j = 0; j < map->count; j++) {
				if( map->list[j].zone == zone ) {
					if( map->zone_mf_cache(j,flag,params) )
						break;
					npc->parse_mapflag(map->list[j].name, empty, flag, params, empty, empty, empty, NULL);
				}
			}
		}
	}

}
static int map_zone_str2itemid(const char *name)
{
	struct item_data *data;

	if( !name )
		return 0;
	if (name[0] == 'I' && name[1] == 'D' && strlen(name) <= 12) {
		if( !( data = itemdb->exists(atoi(name+2))) ) {
			return 0;
		}
	} else {
		if( !( data = itemdb->search_name(name) ) ) {
			return 0;
		}
	}
	return data->nameid;
}
static unsigned short map_zone_str2skillid(const char *name)
{
	unsigned short nameid = 0;

	if( !name )
		return 0;

	if (name[0] == 'I' && name[1] == 'D' && strlen(name) <= 12) {
		if( !skill->get_index((nameid = atoi(name+2))) )
			return 0;
	} else {
		if( !( nameid = strdb_iget(skill->name2id_db, name) ) ) {
			return 0;
		}
	}
	return nameid;
}
static enum bl_type map_zone_bl_type(const char *entry, enum map_zone_skill_subtype *subtype)
{
	char temp[200], *parse;
	enum bl_type bl = BL_NUL;

	nullpo_retr(BL_NUL, subtype);
	*subtype = MZS_NONE;
	if( !entry )
		return BL_NUL;

	safestrncpy(temp, entry, 200);

	parse = strtok(temp,"|");

	while (parse != NULL) {
		normalize_name(parse," ");
		if( strcmpi(parse,"player") == 0 )
			bl |= BL_PC;
		else if( strcmpi(parse,"homun") == 0 )
			bl |= BL_HOM;
		else if( strcmpi(parse,"mercenary") == 0 )
			bl |= BL_MER;
		else if( strcmpi(parse,"monster") == 0 )
			bl |= BL_MOB;
		else if( strcmpi(parse,"clone") == 0 ) {
			bl |= BL_MOB;
			*subtype |= MZS_CLONE;
		} else if( strcmpi(parse,"mob_boss") == 0 ) {
			bl |= BL_MOB;
			*subtype |= MZS_BOSS;
		} else if( strcmpi(parse,"elemental") == 0 )
			bl |= BL_ELEM;
		else if( strcmpi(parse,"pet") == 0 )
			bl |= BL_PET;
		else if( strcmpi(parse,"all") == 0 ) {
			bl |= BL_ALL;
			*subtype |= MZS_ALL;
		} else if( strcmpi(parse,"none") == 0 ) {
			bl = BL_NUL;
		} else {
			ShowError("map_zone_db: '%s' unknown type, skipping...\n",parse);
		}
		parse = strtok(NULL,"|");
	}
	return bl;
}
/* [Ind/Hercules] */
static void read_map_zone_db(void)
{
	struct config_t map_zone_db;
	struct config_setting_t *zones = NULL;
	char config_filename[256];
	libconfig->format_db_path(DBPATH"map_zone_db.conf", config_filename, sizeof(config_filename));
	if (!libconfig->load_file(&map_zone_db, config_filename))
		return;

	zones = libconfig->lookup(&map_zone_db, "zones");

	if (zones != NULL) {
		struct map_zone_data *zone;
		struct config_setting_t *zone_e;
		struct config_setting_t *skills;
		struct config_setting_t *items;
		struct config_setting_t *mapflags;
		struct config_setting_t *commands;
		struct config_setting_t *caps;
		const char *name;
		const char *zonename;
		int i,h,v,j;
		int zone_count = 0, disabled_skills_count = 0, disabled_items_count = 0, mapflags_count = 0,
			disabled_commands_count = 0, capped_skills_count = 0;
		enum map_zone_skill_subtype subtype;

		zone_count = libconfig->setting_length(zones);
		for (i = 0; i < zone_count; ++i) {
			bool is_all = false;

			zone_e = libconfig->setting_get_elem(zones, i);

			if (!libconfig->setting_lookup_string(zone_e, "name", &zonename)) {
				ShowError("map_zone_db: missing zone name, skipping... (%s:%u)\n",
					config_setting_source_file(zone_e), config_setting_source_line(zone_e));
				libconfig->setting_remove_elem(zones,i);/* remove from the tree */
				--zone_count;
				--i;
				continue;
			}

			if( strdb_exists(map->zone_db, zonename) ) {
				ShowError("map_zone_db: duplicate zone name '%s', skipping...\n",zonename);
				libconfig->setting_remove_elem(zones,i);/* remove from the tree */
				--zone_count;
				--i;
				continue;
			}

			/* is this the global template? */
			if( strncmpi(zonename,MAP_ZONE_NORMAL_NAME,MAP_ZONE_NAME_LENGTH) == 0 ) {
				zone = &map->zone_all;
				zone->merge_type = MZMT_NEVERMERGE;
				is_all = true;
			} else if( strncmpi(zonename,MAP_ZONE_PK_NAME,MAP_ZONE_NAME_LENGTH) == 0 ) {
				zone = &map->zone_pk;
				zone->merge_type = MZMT_NEVERMERGE;
				is_all = true;
			} else {
				CREATE( zone, struct map_zone_data, 1 );
				zone->merge_type = MZMT_NORMAL;
				zone->disabled_skills_count = 0;
				zone->disabled_items_count  = 0;
			}
			safestrncpy(zone->name, zonename, MAP_ZONE_NAME_LENGTH/2);

			if( (skills = libconfig->setting_get_member(zone_e, "disabled_skills")) != NULL ) {
				disabled_skills_count = libconfig->setting_length(skills);
				/* validate */
				for(h = 0; h < libconfig->setting_length(skills); h++) {
					struct config_setting_t *skillinfo = libconfig->setting_get_elem(skills, h);
					name = config_setting_name(skillinfo);
					if( !map->zone_str2skillid(name) ) {
						ShowError("map_zone_db: unknown skill (%s) in disabled_skills for zone '%s', skipping skill...\n",name,zone->name);
						libconfig->setting_remove_elem(skills,h);
						--disabled_skills_count;
						--h;
						continue;
					}
					if( !map->zone_bl_type(libconfig->setting_get_string_elem(skills,h),&subtype) )/* we don't remove it from the three due to inheritance */
						--disabled_skills_count;
				}
				/* all ok, process */
				CREATE( zone->disabled_skills, struct map_zone_disabled_skill_entry *, disabled_skills_count );
				for(h = 0, v = 0; h < libconfig->setting_length(skills); h++) {
					struct config_setting_t *skillinfo = libconfig->setting_get_elem(skills, h);
					struct map_zone_disabled_skill_entry * entry;
					enum bl_type type;
					name = config_setting_name(skillinfo);

					if( (type = map->zone_bl_type(libconfig->setting_get_string_elem(skills,h),&subtype)) ) { /* only add if enabled */
						CREATE( entry, struct map_zone_disabled_skill_entry, 1 );

						entry->nameid = map->zone_str2skillid(name);
						entry->type = type;
						entry->subtype = subtype;

						zone->disabled_skills[v++] = entry;
					}

				}
				zone->disabled_skills_count = disabled_skills_count;
			}

			if( (items = libconfig->setting_get_member(zone_e, "disabled_items")) != NULL ) {
				disabled_items_count = libconfig->setting_length(items);
				/* validate */
				for(h = 0; h < libconfig->setting_length(items); h++) {
					struct config_setting_t *item = libconfig->setting_get_elem(items, h);
					name = config_setting_name(item);
					if( !map->zone_str2itemid(name) ) {
						ShowError("map_zone_db: unknown item (%s) in disabled_items for zone '%s', skipping item...\n",name,zone->name);
						libconfig->setting_remove_elem(items,h);
						--disabled_items_count;
						--h;
						continue;
					}
					if( !libconfig->setting_get_bool(item) )/* we don't remove it from the three due to inheritance */
						--disabled_items_count;
				}
				/* all ok, process */
				CREATE( zone->disabled_items, int, disabled_items_count );
				if( (libconfig->setting_length(items) - disabled_items_count) > 0 ) { //Some are forcefully enabled
					zone->cant_disable_items_count = libconfig->setting_length(items) - disabled_items_count;
					CREATE(zone->cant_disable_items, int, zone->cant_disable_items_count);
				}
				for(h = 0, v = 0, j = 0; h < libconfig->setting_length(items); h++) {
					struct config_setting_t *item = libconfig->setting_get_elem(items, h);

					name = config_setting_name(item);
					if( libconfig->setting_get_bool(item) ) { /* only add if enabled */
						zone->disabled_items[v++] = map->zone_str2itemid(name);
					} else { /** forcefully enabled **/
						zone->cant_disable_items[j++] = map->zone_str2itemid(name);
					}

				}
				zone->disabled_items_count = disabled_items_count;
			}

			if( (mapflags = libconfig->setting_get_member(zone_e, "mapflags")) != NULL ) {
				mapflags_count = libconfig->setting_length(mapflags);
				/* mapflags are not validated here, so we save all anyway */
				CREATE( zone->mapflags, char *, mapflags_count );
				for(h = 0; h < mapflags_count; h++) {
					CREATE( zone->mapflags[h], char, MAP_ZONE_MAPFLAG_LENGTH );

					name = libconfig->setting_get_string_elem(mapflags, h);

					safestrncpy(zone->mapflags[h], name, MAP_ZONE_MAPFLAG_LENGTH);

				}
				zone->mapflags_count = mapflags_count;
			}

			if( (commands = libconfig->setting_get_member(zone_e, "disabled_commands")) != NULL ) {
				disabled_commands_count = libconfig->setting_length(commands);
				/* validate */
				for(h = 0; h < libconfig->setting_length(commands); h++) {
					struct config_setting_t *command = libconfig->setting_get_elem(commands, h);
					name = config_setting_name(command);
					if( !atcommand->exists(name) ) {
						ShowError("map_zone_db: unknown command '%s' in disabled_commands for zone '%s', skipping entry...\n",name,zone->name);
						libconfig->setting_remove_elem(commands,h);
						--disabled_commands_count;
						--h;
						continue;
					}
					if( !libconfig->setting_get_int(command) )/* we don't remove it from the three due to inheritance */
						--disabled_commands_count;
				}
				/* all ok, process */
				CREATE( zone->disabled_commands, struct map_zone_disabled_command_entry *, disabled_commands_count );
				for(h = 0, v = 0; h < libconfig->setting_length(commands); h++) {
					struct config_setting_t *command = libconfig->setting_get_elem(commands, h);
					struct map_zone_disabled_command_entry * entry;
					int group_lv;
					name = config_setting_name(command);

					if( (group_lv = libconfig->setting_get_int(command)) ) { /* only add if enabled */
						CREATE( entry, struct map_zone_disabled_command_entry, 1 );

						entry->cmd  = atcommand->exists(name)->func;
						entry->group_lv = group_lv;

						zone->disabled_commands[v++] = entry;
					}
				}
				zone->disabled_commands_count = disabled_commands_count;
			}

			if( (caps = libconfig->setting_get_member(zone_e, "skill_damage_cap")) != NULL ) {
				capped_skills_count = libconfig->setting_length(caps);
				/* validate */
				for(h = 0; h < libconfig->setting_length(caps); h++) {
					struct config_setting_t *cap = libconfig->setting_get_elem(caps, h);
					name = config_setting_name(cap);
					if( !map->zone_str2skillid(name) ) {
						ShowError("map_zone_db: unknown skill (%s) in skill_damage_cap for zone '%s', skipping skill...\n",name,zone->name);
						libconfig->setting_remove_elem(caps,h);
						--capped_skills_count;
						--h;
						continue;
					}
					if( !map->zone_bl_type(libconfig->setting_get_string_elem(cap,1),&subtype) )/* we don't remove it from the three due to inheritance */
						--capped_skills_count;
				}
				/* all ok, process */
				CREATE( zone->capped_skills, struct map_zone_skill_damage_cap_entry *, capped_skills_count );
				for(h = 0, v = 0; h < libconfig->setting_length(caps); h++) {
					struct config_setting_t *cap = libconfig->setting_get_elem(caps, h);
					struct map_zone_skill_damage_cap_entry * entry;
					enum bl_type type;
					name = config_setting_name(cap);

					if( (type = map->zone_bl_type(libconfig->setting_get_string_elem(cap,1),&subtype)) ) { /* only add if enabled */
						CREATE( entry, struct map_zone_skill_damage_cap_entry, 1 );

						entry->nameid = map->zone_str2skillid(name);
						entry->cap = libconfig->setting_get_int_elem(cap,0);
						entry->type = type;
						entry->subtype = subtype;
						zone->capped_skills[v++] = entry;
					}
				}
				zone->capped_skills_count = capped_skills_count;
			}

			if( !is_all ) /* global template doesn't go into db -- since it isn't a alloc'd piece of data */
				strdb_put(map->zone_db, zonename, zone);

		}

		/* process inheritance, aka loop through the whole thing again :P */
		for (i = 0; i < zone_count; ++i) {
			struct config_setting_t *inherit_tree = NULL;
			struct config_setting_t *new_entry = NULL;
			int inherit_count;

			zone_e = libconfig->setting_get_elem(zones, i);
			libconfig->setting_lookup_string(zone_e, "name", &zonename);

			if( strncmpi(zonename,MAP_ZONE_ALL_NAME,MAP_ZONE_NAME_LENGTH) == 0 ) {
				continue;/* all zone doesn't inherit anything (if it did, everything would link to each other and boom endless loop) */
			}

			if( (inherit_tree = libconfig->setting_get_member(zone_e, "inherit")) != NULL ) {
				/* append global zone to this */
				new_entry = libconfig->setting_add(inherit_tree,MAP_ZONE_ALL_NAME,CONFIG_TYPE_STRING);
				libconfig->setting_set_string(new_entry,MAP_ZONE_ALL_NAME);
			} else {
				/* create inherit member and add global zone to it */
				inherit_tree = libconfig->setting_add(zone_e, "inherit",CONFIG_TYPE_ARRAY);
				new_entry = libconfig->setting_add(inherit_tree,MAP_ZONE_ALL_NAME,CONFIG_TYPE_STRING);
				libconfig->setting_set_string(new_entry,MAP_ZONE_ALL_NAME);
			}
			inherit_count = libconfig->setting_length(inherit_tree);
			for(h = 0; h < inherit_count; h++) {
				struct map_zone_data *izone; /* inherit zone */
				int disabled_skills_count_i = 0; /* disabled skill count from inherit zone */
				int disabled_items_count_i = 0; /* disabled item count from inherit zone */
				int mapflags_count_i = 0; /* mapflag count from inherit zone */
				int disabled_commands_count_i = 0; /* commands count from inherit zone */
				int capped_skills_count_i = 0; /* skill capped count from inherit zone */

				name = libconfig->setting_get_string_elem(inherit_tree, h);
				libconfig->setting_lookup_string(zone_e, "name", &zonename);/* will succeed for we validated it earlier */

				if( !(izone = strdb_get(map->zone_db, name)) ) {
					ShowError("map_zone_db: Unknown zone '%s' being inherit by zone '%s', skipping...\n",name,zonename);
					continue;
				}

				if( strncmpi(zonename,MAP_ZONE_NORMAL_NAME,MAP_ZONE_NAME_LENGTH) == 0 ) {
					zone = &map->zone_all;
				} else if( strncmpi(zonename,MAP_ZONE_PK_NAME,MAP_ZONE_NAME_LENGTH) == 0 ) {
					zone = &map->zone_pk;
				} else
					zone = strdb_get(map->zone_db, zonename);/* will succeed for we just put it in here */

				disabled_skills_count_i = izone->disabled_skills_count;
				disabled_items_count_i = izone->disabled_items_count;
				mapflags_count_i = izone->mapflags_count;
				disabled_commands_count_i = izone->disabled_commands_count;
				capped_skills_count_i = izone->capped_skills_count;

				/* process everything to override, paying attention to config_setting_get_bool */
				if( disabled_skills_count_i ) {
					if( (skills = libconfig->setting_get_member(zone_e, "disabled_skills")) == NULL )
						skills = libconfig->setting_add(zone_e, "disabled_skills",CONFIG_TYPE_GROUP);
					disabled_skills_count = libconfig->setting_length(skills);
					for(j = 0; j < disabled_skills_count_i; j++) {
						int k;
						for(k = 0; k < disabled_skills_count; k++) {
							struct config_setting_t *skillinfo = libconfig->setting_get_elem(skills, k);
							if( map->zone_str2skillid(config_setting_name(skillinfo)) == izone->disabled_skills[j]->nameid ) {
								break;
							}
						}
						if( k == disabled_skills_count ) {/* we didn't find it */
							struct map_zone_disabled_skill_entry *entry;
							RECREATE( zone->disabled_skills, struct map_zone_disabled_skill_entry *, ++zone->disabled_skills_count );
							CREATE( entry, struct map_zone_disabled_skill_entry, 1 );
							entry->nameid = izone->disabled_skills[j]->nameid;
							entry->type = izone->disabled_skills[j]->type;
							entry->subtype = izone->disabled_skills[j]->subtype;
							zone->disabled_skills[zone->disabled_skills_count-1] = entry;
						}
					}
				}

				if( disabled_items_count_i ) {
					if( (items = libconfig->setting_get_member(zone_e, "disabled_items")) == NULL )
						items = libconfig->setting_add(zone_e, "disabled_items",CONFIG_TYPE_GROUP);
					disabled_items_count = libconfig->setting_length(items);
					for(j = 0; j < disabled_items_count_i; j++) {
						int k;
						for(k = 0; k < disabled_items_count; k++) {
							struct config_setting_t *item = libconfig->setting_get_elem(items, k);

							name = config_setting_name(item);

							if( map->zone_str2itemid(name) == izone->disabled_items[j] ) {
								if( libconfig->setting_get_bool(item) )
									continue;
								break;
							}
						}
						if( k == disabled_items_count ) {/* we didn't find it */
							RECREATE( zone->disabled_items, int, ++zone->disabled_items_count );
							zone->disabled_items[zone->disabled_items_count-1] = izone->disabled_items[j];
						}
					}
				}

				if( mapflags_count_i ) {
					if( (mapflags = libconfig->setting_get_member(zone_e, "mapflags")) == NULL )
						mapflags = libconfig->setting_add(zone_e, "mapflags",CONFIG_TYPE_ARRAY);
					mapflags_count = libconfig->setting_length(mapflags);
					for(j = 0; j < mapflags_count_i; j++) {
						int k;
						for(k = 0; k < mapflags_count; k++) {
							name = libconfig->setting_get_string_elem(mapflags, k);

							if( strcmpi(name,izone->mapflags[j]) == 0 ) {
								break;
							}
						}
						if( k == mapflags_count ) {/* we didn't find it */
							RECREATE( zone->mapflags, char*, ++zone->mapflags_count );
							CREATE( zone->mapflags[zone->mapflags_count-1], char, MAP_ZONE_MAPFLAG_LENGTH );
							safestrncpy(zone->mapflags[zone->mapflags_count-1], izone->mapflags[j], MAP_ZONE_MAPFLAG_LENGTH);
						}
					}
				}

				if( disabled_commands_count_i ) {
					if( (commands = libconfig->setting_get_member(zone_e, "disabled_commands")) == NULL )
						commands = libconfig->setting_add(zone_e, "disabled_commands",CONFIG_TYPE_GROUP);

					disabled_commands_count = libconfig->setting_length(commands);
					for(j = 0; j < disabled_commands_count_i; j++) {
						int k;
						for(k = 0; k < disabled_commands_count; k++) {
							struct config_setting_t *command = libconfig->setting_get_elem(commands, k);
							if( atcommand->exists(config_setting_name(command))->func == izone->disabled_commands[j]->cmd ) {
								break;
							}
						}
						if( k == disabled_commands_count ) {/* we didn't find it */
							struct map_zone_disabled_command_entry *entry;
							RECREATE( zone->disabled_commands, struct map_zone_disabled_command_entry *, ++zone->disabled_commands_count );
							CREATE( entry, struct map_zone_disabled_command_entry, 1 );
							entry->cmd = izone->disabled_commands[j]->cmd;
							entry->group_lv = izone->disabled_commands[j]->group_lv;
							zone->disabled_commands[zone->disabled_commands_count-1] = entry;
						}
					}
				}

				if( capped_skills_count_i ) {
					if( (caps = libconfig->setting_get_member(zone_e, "skill_damage_cap")) == NULL )
						caps = libconfig->setting_add(zone_e, "skill_damage_cap",CONFIG_TYPE_GROUP);

					capped_skills_count = libconfig->setting_length(caps);
					for(j = 0; j < capped_skills_count_i; j++) {
						int k;
						for(k = 0; k < capped_skills_count; k++) {
							struct config_setting_t *cap = libconfig->setting_get_elem(caps, k);
							if( map->zone_str2skillid(config_setting_name(cap)) == izone->capped_skills[j]->nameid ) {
								break;
							}
						}
						if( k == capped_skills_count ) {/* we didn't find it */
							struct map_zone_skill_damage_cap_entry *entry;
							RECREATE( zone->capped_skills, struct map_zone_skill_damage_cap_entry *, ++zone->capped_skills_count );
							CREATE( entry, struct map_zone_skill_damage_cap_entry, 1 );
							entry->nameid = izone->capped_skills[j]->nameid;
							entry->cap = izone->capped_skills[j]->cap;
							entry->type = izone->capped_skills[j]->type;
							entry->subtype = izone->capped_skills[j]->subtype;
							zone->capped_skills[zone->capped_skills_count-1] = entry;
						}
					}
				}

			}
		}

		ShowStatus("Done reading '"CL_WHITE"%d"CL_RESET"' zones in '"CL_WHITE"%s"CL_RESET"'.\n", zone_count, config_filename);

		/* post-load processing */
		if( (zone = strdb_get(map->zone_db, MAP_ZONE_PVP_NAME)) )
			zone->merge_type = MZMT_MERGEABLE;
		if( (zone = strdb_get(map->zone_db, MAP_ZONE_GVG_NAME)) )
			zone->merge_type = MZMT_MERGEABLE;
		if( (zone = strdb_get(map->zone_db, MAP_ZONE_BG_NAME)) )
			zone->merge_type = MZMT_MERGEABLE;
		if ((zone = strdb_get(map->zone_db, MAP_ZONE_CVC_NAME)))
		  zone->merge_type = MZMT_MERGEABLE;
	}
	/* not supposed to go in here but in skill_final whatever */
	libconfig->destroy(&map_zone_db);
}

static int map_get_new_bonus_id(void)
{
	return map->bonus_id++;
}

static bool map_add_questinfo(int m, struct npc_data *nd)
{
	nullpo_retr(false, nd);
	Assert_retr(false, m >= 0 && m < map->count);

	int i;
	ARR_FIND(0, VECTOR_LENGTH(map->list[m].qi_list), i, VECTOR_INDEX(map->list[m].qi_list, i) == nd);

	if (i < VECTOR_LENGTH(map->list[m].qi_list)) {
		return false;
	}

	VECTOR_ENSURE(map->list[m].qi_list, 1, 1);
	VECTOR_PUSH(map->list[m].qi_list, nd);
	return true;
}

static bool map_remove_questinfo(int m, struct npc_data *nd)
{

	nullpo_retr(false, nd);
	Assert_retr(false, m >= 0 && m < map->count);

	int i;
	ARR_FIND(0, VECTOR_LENGTH(map->list[m].qi_list), i, VECTOR_INDEX(map->list[m].qi_list, i) == nd);
	if (i != VECTOR_LENGTH(map->list[m].qi_list)) {
		VECTOR_ERASE(map->list[m].qi_list, i);
		return true;
	}
	return false;
}

/**
 * @see DBApply
 */
static int nick_db_final(union DBKey key, struct DBData *data, va_list args)
{
	struct charid2nick* p = DB->data2ptr(data);
	struct charid_request* req;

	if( p == NULL )
		return 0;
	while( p->requests )
	{
		req = p->requests;
		p->requests = req->next;
		aFree(req);
	}
	aFree(p);
	return 0;
}

static int cleanup_sub(struct block_list *bl, va_list ap)
{
	nullpo_ret(bl);

	switch(bl->type) {
		case BL_PC:
			map->quit(BL_UCAST(BL_PC, bl));
			break;
		case BL_NPC:
			npc->unload(BL_UCAST(BL_NPC, bl), false, true);
			break;
		case BL_MOB:
			unit->free(bl,CLR_OUTSIGHT);
			break;
		case BL_PET:
			//There is no need for this, the pet is removed together with the player. [Skotlex]
			break;
		case BL_ITEM:
			map->clearflooritem(bl);
			break;
		case BL_SKILL:
			skill->delunit(BL_UCAST(BL_SKILL, bl));
			break;
		case BL_NUL:
		case BL_HOM:
		case BL_MER:
		case BL_CHAT:
		case BL_ELEM:
		case BL_ALL:
			break;
	}

	return 1;
}

/**
 * @see DBApply
 */
static int cleanup_db_sub(union DBKey key, struct DBData *data, va_list va)
{
	return map->cleanup_sub(DB->data2ptr(data), va);
}

static void map_lock_check(const char *file, const char *func, int line, int lock_count)
{
	if (map->block_free_lock != lock_count) {
		if (map->block_free_lock > lock_count) {
			ShowError("map_lock_check: found missing call to map->freeblock_unlock: %s %s:%d\n", file, func, line);
		} else {
			ShowError("map_lock_check: found extra call to map->freeblock_unlock: %s %s:%d\n", file, func, line);
		}
		Assert_report(0);
		map->block_free_lock = lock_count;
	}
}

/*==========================================
 * map destructor
 *------------------------------------------*/
int do_final(void)
{
	int i;
	struct map_session_data* sd;
	struct s_mapiterator* iter;

	ShowStatus("Terminating...\n");

	channel->config->closing = true;
	HPM->event(HPET_FINAL);

	if (map->cpsd) aFree(map->cpsd);

	//Ladies and babies first.
	iter = mapit_getallusers();
	for (sd = BL_UCAST(BL_PC, mapit->first(iter)); mapit->exists(iter); sd = BL_UCAST(BL_PC, mapit->next(iter)))
		map->quit(sd);
	mapit->free(iter);

	/* prepares npcs for a faster shutdown process */
	npc->do_clear_npc();

	// remove all objects on maps
	for (i = 0; i < map->count; i++) {
		ShowStatus("Cleaning up maps [%d/%d]: %s..."CL_CLL"\r", i+1, map->count, map->list[i].name);
		if (map->list[i].m >= 0)
			map->foreachinmap(map->cleanup_sub, i, BL_ALL);
	}
	ShowStatus("Cleaned up %d maps."CL_CLL"\n", map->count);

	if (map->extra_scripts) {
		for (i = 0; i < map->extra_scripts_count; i++)
			aFree(map->extra_scripts[i]);
		aFree(map->extra_scripts);
		map->extra_scripts = NULL;
		map->extra_scripts_count = 0;
	}

	map->id_db->foreach(map->id_db,map->cleanup_db_sub);
	chrif->char_reset_offline();
	chrif->flush();

	atcommand->final();
	battle->final();
	ircbot->final();/* before channel. */
	channel->final();
	chrif->final();
	clan->final();
	clif->final();
	npc->final();
	quest->final();
	script->final();
	itemdb->final();
	instance->final();
	gstorage->final();
	guild->final();
	party->final();
	pc->final();
	pet->final();
	mob->final();
	homun->final();
	atcommand->final_msg();
	skill->final();
	status->final();
	refine->final();
	grader->final();
	unit->final();
	bg->final();
	duel->final();
	elemental->final();
	map->list_final();
	vending->final();
	rodex->final();
	achievement->final();
	stylist->final();
	enchantui->final();
	goldpc->final();
	mapiif->final();
	intif->final();
	extraconf->final();

	HPM_map_do_final();

	mapindex->final();
	if (map->enable_grf)
		grfio->final();

	db_destroy(map->id_db);
	db_destroy(map->pc_db);
	db_destroy(map->mobid_db);
	db_destroy(map->bossid_db);
	map->nick_db->destroy(map->nick_db, map->nick_db_final);
	db_destroy(map->charid_db);
	db_destroy(map->iwall_db);
	db_destroy(map->regen_db);

	map->sql_close();
	ers_destroy(map->iterator_ers);
	ers_destroy(map->flooritem_ers);

	for (i = 0; i < map->count; ++i) {
		if (map->list[i].cell_buf.data != NULL)
			aFree(map->list[i].cell_buf.data);
		map->list[i].cell_buf.len = 0;
	}
	aFree(map->list);

	if( map->block_free )
		aFree(map->block_free);
#ifdef SANITIZE
	if (map->block_free_sanitize)
		aFree(map->block_free_sanitize);
#endif
	if( map->bl_list )
		aFree(map->bl_list);

	aFree(map->MAP_CONF_NAME);
	aFree(map->BATTLE_CONF_FILENAME);
	aFree(map->ATCOMMAND_CONF_FILENAME);
	aFree(map->SCRIPT_CONF_NAME);
	aFree(map->MSG_CONF_NAME);
	aFree(map->GRF_PATH_FILENAME);
	aFree(map->INTER_CONF_NAME);
	aFree(map->LOG_CONF_NAME);

	HPM->event(HPET_POST_FINAL);

	ShowStatus("Finished.\n");
	return map->retval;
}

static int map_abort_sub(struct map_session_data *sd, va_list ap)
{
	chrif->save(sd,1);
	return 1;
}

//------------------------------
// Function called when the server
// has received a crash signal.
//------------------------------
void do_abort(void)
{
	static int run = 0;
	//Save all characters and then flush the inter-connection.
	if (run) {
		ShowFatalError("Server has crashed while trying to save characters. Character data can't be saved!\n");
		return;
	}
	run = 1;
	if (!chrif->isconnected())
	{
		if (db_size(map->pc_db))
			ShowFatalError("Server has crashed without a connection to the char-server, %u characters can't be saved!\n", db_size(map->pc_db));
		return;
	}
	ShowError("Server received crash signal! Attempting to save all online characters!\n");
	map->foreachpc(map->abort_sub);
	chrif->flush();
}

void set_server_type(void)
{
	SERVER_TYPE = SERVER_TYPE_MAP;
}

/// Called when a terminate signal is received.
static void do_shutdown(void)
{
	if( core->runflag != MAPSERVER_ST_SHUTDOWN )
	{
		core->runflag = MAPSERVER_ST_SHUTDOWN;
		ShowStatus("Shutting down...\n");
		{
			struct map_session_data* sd;
			struct s_mapiterator* iter = mapit_getallusers();
			for (sd = BL_UCAST(BL_PC, mapit->first(iter)); mapit->exists(iter); sd = BL_UCAST(BL_PC, mapit->next(iter)))
				clif->GM_kick(NULL, sd);
			mapit->free(iter);
			sockt->flush_fifos();
		}
		chrif->check_shutdown();
	}
}

static CPCMD(gm_position)
{
	int x = 0, y = 0, m = 0;
	char map_name[25];

	if( line == NULL || sscanf(line, "%d %d %24s",&x,&y,map_name) < 3 ) {
		ShowError("gm:info invalid syntax. use '"CL_WHITE"gm:info xCord yCord map_name"CL_RESET"'\n");
		return;
	}

	if ((m = map->mapname2mapid(map_name)) <= 0) {
		ShowError("gm:info '"CL_WHITE"%s"CL_RESET"' is not a known map\n",map_name);
		return;
	}

	if( x < 0 || x >= map->list[m].xs || y < 0 || y >= map->list[m].ys ) {
		ShowError("gm:info '"CL_WHITE"%d %d"CL_RESET"' is out of '"CL_WHITE"%s"CL_RESET"' map bounds!\n",x,y,map_name);
		return;
	}

	ShowInfo("HCP: updated console's game position to '"CL_WHITE"%d %d %s"CL_RESET"'\n",x,y,map_name);
	map->cpsd->bl.x = x;
	map->cpsd->bl.y = y;
	map->cpsd->bl.m = m;
	map->cpsd->mapindex = map_id2index(m);
}
static CPCMD(gm_use)
{

	if( line == NULL ) {
		ShowError("gm:use invalid syntax. use '"CL_WHITE"gm:use @command <optional params>"CL_RESET"'\n");
		return;
	}

	map->cpsd_active = true;

	if( !atcommand->exec(map->cpsd->fd, map->cpsd, line, false) )
		ShowInfo("HCP: '"CL_WHITE"%s"CL_RESET"' failed\n",line);
	else
		ShowInfo("HCP: '"CL_WHITE"%s"CL_RESET"' was used\n",line);

	map->cpsd_active = false;
}
/* Hercules Console Parser */
static void map_cp_defaults(void)
{
#ifdef CONSOLE_INPUT
	/* default HCP data */
	map->cpsd = pc->get_dummy_sd();
	strcpy(map->cpsd->status.name, "Hercules Console");
	map->cpsd->bl.x = mapindex->default_x;
	map->cpsd->bl.y = mapindex->default_y;
	map->cpsd->bl.m = map->mapname2mapid(mapindex->default_map);
	Assert_retv(map->cpsd->bl.m >= 0);
	map->cpsd->mapindex = map_id2index(map->cpsd->bl.m);

	console->input->addCommand("gm:info",CPCMD_A(gm_position));
	console->input->addCommand("gm:use",CPCMD_A(gm_use));
#endif
}

static void map_load_defaults(void)
{
	extraconf_defaults();
	mapindex_defaults();
	map_defaults();
	mapit_defaults();
	/* */
	atcommand_defaults();
	battle_defaults();
	battleground_defaults();
	buyingstore_defaults();
	channel_defaults();
	clan_defaults();
	clif_defaults();
	chrif_defaults();
	guild_defaults();
	gstorage_defaults();
	homunculus_defaults();
	instance_defaults();
	ircbot_defaults();
	itemdb_defaults();
	log_defaults();
	macro_defaults();
	mail_defaults();
	npc_defaults();
	script_defaults();
	searchstore_defaults();
	skill_defaults();
	vending_defaults();
	pc_defaults();
	pc_groups_defaults();
	party_defaults();
	storage_defaults();
	trade_defaults();
	status_defaults();
	chat_defaults();
	duel_defaults();
	elemental_defaults();
	intif_defaults();
	mercenary_defaults();
	mob_defaults();
	unit_defaults();
	mapreg_defaults();
	pet_defaults();
	path_defaults();
	quest_defaults();
	achievement_defaults();
	npc_chat_defaults();
	rodex_defaults();
	stylist_defaults();
	refine_defaults();
	grader_defaults();
	enchantui_defaults();
	goldpc_defaults();
	mapiif_defaults();
}
/**
 * --run-once handler
 *
 * Causes the server to run its loop once, and shutdown. Useful for testing.
 * @see cmdline->exec
 */
static CMDLINEARG(runonce)
{
	core->runflag = CORE_ST_STOP;
	return true;
}
/**
 * --map-config handler
 *
 * Overrides the default map-server configuration filename.
 * @see cmdline->exec
 */
static CMDLINEARG(mapconfig)
{
	aFree(map->MAP_CONF_NAME);
	map->MAP_CONF_NAME = aStrdup(params);
	return true;
}
/**
 * --battle-config handler
 *
 * Overrides the default battle configuration filename.
 * @see cmdline->exec
 */
static CMDLINEARG(battleconfig)
{
	aFree(map->BATTLE_CONF_FILENAME);
	map->BATTLE_CONF_FILENAME = aStrdup(params);
	return true;
}
/**
 * --atcommand-config handler
 *
 * Overrides the default atcommands configuration filename.
 * @see cmdline->exec
 */
static CMDLINEARG(atcommandconfig)
{
	aFree(map->ATCOMMAND_CONF_FILENAME);
	map->ATCOMMAND_CONF_FILENAME = aStrdup(params);
	return true;
}
/**
 * --script-config handler
 *
 * Overrides the default script configuration filename.
 * @see cmdline->exec
 */
static CMDLINEARG(scriptconfig)
{
	aFree(map->SCRIPT_CONF_NAME);
	map->SCRIPT_CONF_NAME = aStrdup(params);
	return true;
}
/**
 * --msg-config handler
 *
 * Overrides the default messages configuration filename.
 * @see cmdline->exec
 */
static CMDLINEARG(msgconfig)
{
	aFree(map->MSG_CONF_NAME);
	map->MSG_CONF_NAME = aStrdup(params);
	return true;
}
/**
 * --grf-path handler
 *
 * Overrides the default grf configuration filename.
 * @see cmdline->exec
 */
static CMDLINEARG(grfpath)
{
	aFree(map->GRF_PATH_FILENAME);
	map->GRF_PATH_FILENAME = aStrdup(params);
	return true;
}
/**
 * --inter-config handler
 *
 * Overrides the default inter-server configuration filename.
 * @see cmdline->exec
 */
static CMDLINEARG(interconfig)
{
	aFree(map->INTER_CONF_NAME);
	map->INTER_CONF_NAME = aStrdup(params);
	return true;
}
/**
 * --log-config handler
 *
 * Overrides the default log configuration filename.
 * @see cmdline->exec
 */
static CMDLINEARG(logconfig)
{
	aFree(map->LOG_CONF_NAME);
	map->LOG_CONF_NAME = aStrdup(params);
	return true;
}
/**
 * --script-check handler
 *
 * Enables script-check mode. Checks scripts and quits without running.
 * @see cmdline->exec
 */
static CMDLINEARG(scriptcheck)
{
	map->minimal = true;
	core->runflag = CORE_ST_STOP;
	map->scriptcheck = true;
	return true;
}
/**
 * --load-script handler
 *
 * Adds a filename to the script auto-load list.
 * @see cmdline->exec
 */
static CMDLINEARG(loadscript)
{
	RECREATE(map->extra_scripts, char *, ++map->extra_scripts_count);
	map->extra_scripts[map->extra_scripts_count-1] = aStrdup(params);
	return true;
}

/**
 * Defines the local command line arguments
 */
void cmdline_args_init_local(void)
{
	CMDLINEARG_DEF2(run-once, runonce, "Closes server after loading (testing).", CMDLINE_OPT_NORMAL);
	CMDLINEARG_DEF2(map-config, mapconfig, "Alternative map-server configuration.", CMDLINE_OPT_NORMAL|CMDLINE_OPT_PARAM);
	CMDLINEARG_DEF2(battle-config, battleconfig, "Alternative battle configuration.", CMDLINE_OPT_NORMAL|CMDLINE_OPT_PARAM);
	CMDLINEARG_DEF2(atcommand-config, atcommandconfig, "Alternative atcommand configuration.", CMDLINE_OPT_NORMAL|CMDLINE_OPT_PARAM);
	CMDLINEARG_DEF2(script-config, scriptconfig, "Alternative script configuration.", CMDLINE_OPT_NORMAL|CMDLINE_OPT_PARAM);
	CMDLINEARG_DEF2(msg-config, msgconfig, "Alternative message configuration.", CMDLINE_OPT_NORMAL|CMDLINE_OPT_PARAM);
	CMDLINEARG_DEF2(grf-path, grfpath, "Alternative GRF path configuration.", CMDLINE_OPT_NORMAL|CMDLINE_OPT_PARAM);
	CMDLINEARG_DEF2(inter-config, interconfig, "Alternative inter-server configuration.", CMDLINE_OPT_NORMAL|CMDLINE_OPT_PARAM);
	CMDLINEARG_DEF2(log-config, logconfig, "Alternative logging configuration.", CMDLINE_OPT_NORMAL|CMDLINE_OPT_PARAM);
	CMDLINEARG_DEF2(script-check, scriptcheck, "Doesn't run the server, only tests the scripts passed through --load-script.", CMDLINE_OPT_SILENT);
	CMDLINEARG_DEF2(load-script, loadscript, "Loads an additional script (can be repeated).", CMDLINE_OPT_NORMAL|CMDLINE_OPT_PARAM);
}

int do_init(int argc, char *argv[])
{
	bool minimal = false;
	int i;

#ifdef GCOLLECT
	GC_enable_incremental();
#endif

	map_load_defaults();
	extraconf->init();

	map->INTER_CONF_NAME         = aStrdup("conf/common/inter-server.conf");
	map->LOG_CONF_NAME           = aStrdup("conf/map/logs.conf");
	map->MAP_CONF_NAME           = aStrdup("conf/map/map-server.conf");
	map->BATTLE_CONF_FILENAME    = aStrdup("conf/map/battle.conf");
	map->ATCOMMAND_CONF_FILENAME = aStrdup("conf/atcommand.conf");
	map->SCRIPT_CONF_NAME        = aStrdup("conf/map/script.conf");
	map->MSG_CONF_NAME           = aStrdup("conf/messages.conf");
	map->GRF_PATH_FILENAME       = aStrdup("conf/grf-files.txt");

	HPM_map_do_init();
	cmdline->exec(argc, argv, CMDLINE_OPT_PREINIT);
	HPM->config_read();

	HPM->event(HPET_PRE_INIT);

	cmdline->exec(argc, argv, CMDLINE_OPT_NORMAL);
	minimal = map->minimal;/* temp (perhaps make minimal a mask with options of what to load? e.g. plugin 1 does minimal |= mob_db; */
	if (!minimal) {
		map->config_read(map->MAP_CONF_NAME, false);

		{
			// TODO: Remove this when no longer needed.
#define CHECK_OLD_LOCAL_CONF(oldname, newname) do { \
	if (stat((oldname), &fileinfo) == 0 && fileinfo.st_size > 0) { \
		ShowWarning("An old configuration file \"%s\" was found.\n", (oldname)); \
		ShowWarning("If it contains settings you wish to keep, please merge them into \"%s\".\n", (newname)); \
		ShowWarning("Otherwise, just delete it.\n"); \
		ShowInfo("Resuming in 10 seconds...\n"); \
		HSleep(10); \
	} \
} while (0)
		struct stat fileinfo;

		CHECK_OLD_LOCAL_CONF("conf/import/map_conf.txt", "conf/import/map-server.conf");
		CHECK_OLD_LOCAL_CONF("conf/import/inter_conf.txt", "conf/import/inter-server.conf");
		CHECK_OLD_LOCAL_CONF("conf/import/log_conf.txt", "conf/import/logs.conf");
		CHECK_OLD_LOCAL_CONF("conf/import/script_conf.txt", "conf/import/script.conf");
		CHECK_OLD_LOCAL_CONF("conf/import/packet_conf.txt", "conf/import/socket.conf");
		CHECK_OLD_LOCAL_CONF("conf/import/battle_conf.txt", "conf/import/battle.conf");

#undef CHECK_OLD_LOCAL_CONF
		}

		// loads npcs
		map->reloadnpc(false);

		chrif->checkdefaultlogin();

		if (!map->ip_set || !map->char_ip_set) {
			char ip_str[16];
			sockt->ip2str(sockt->addr_[0], ip_str);

#ifndef BUILDBOT
			ShowWarning("Not all IP addresses in /conf/map/map-server.conf configured, auto-detecting...\n");
#endif

			if (sockt->naddr_ == 0)
				ShowError("Unable to determine your IP address...\n");
			else if (sockt->naddr_ > 1)
				ShowNotice("Multiple interfaces detected...\n");

			ShowInfo("Defaulting to %s as our IP address\n", ip_str);

			if (!map->ip_set)
				clif->setip(ip_str);
			if (!map->char_ip_set)
				chrif->setip(ip_str);
		}

		extraconf->read_emblems();
		battle->config_read(map->BATTLE_CONF_FILENAME, false);
		atcommand->msg_read(map->MSG_CONF_NAME, false);
		map->inter_config_read(map->INTER_CONF_NAME, false);
		logs->config_read(map->LOG_CONF_NAME, false);
	} else {
		battle->config_read(map->BATTLE_CONF_FILENAME, false);
	}
	script->config_read(map->SCRIPT_CONF_NAME, false);

	map->id_db     = idb_alloc(DB_OPT_BASE);
	map->pc_db     = idb_alloc(DB_OPT_BASE); //Added for reliable map->id2sd() use. [Skotlex]
	map->mobid_db  = idb_alloc(DB_OPT_BASE); //Added to lower the load of the lazy mob AI. [Skotlex]
	map->bossid_db = idb_alloc(DB_OPT_BASE); // Used for Convex Mirror quick MVP search
	map->nick_db   = idb_alloc(DB_OPT_BASE);
	map->charid_db = idb_alloc(DB_OPT_BASE);
	map->regen_db  = idb_alloc(DB_OPT_BASE); // efficient status_natural_heal processing
	map->iwall_db  = strdb_alloc(DB_OPT_DUP_KEY|DB_OPT_RELEASE_DATA, 2*NAME_LENGTH+2+1); // [Zephyrus] Invisible Walls
	map->zone_db   = strdb_alloc(DB_OPT_DUP_KEY|DB_OPT_RELEASE_DATA, MAP_ZONE_NAME_LENGTH);

	map->iterator_ers = ers_new(sizeof(struct s_mapiterator),"map.c::map_iterator_ers",ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK);
	ers_chunk_size(map->iterator_ers, 25);

	map->flooritem_ers = ers_new(sizeof(struct flooritem_data),"map.c::map_flooritem_ers",ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK);
	ers_chunk_size(map->flooritem_ers, 100);

	if (!minimal) {
		map->sql_init();
		if (logs->config.sql_logs)
			logs->sql_init();
	}

	i = mapindex->init();

	if (minimal) {
		// Pretend all maps from the mapindex are on this mapserver
		CREATE(map->list,struct map_data,i);

		for( i = 0; i < MAX_MAPINDEX; i++ ) {
			if (mapindex_exists(i)) {
				map->addmap(mapindex_id2name(i));
			}
		}
	}

	if (map->enable_grf)
		grfio->init(map->GRF_PATH_FILENAME);

	map->readallmaps();

	if (!minimal) {
		timer->add_func_list(map->freeblock_timer, "map_freeblock_timer");
		timer->add_func_list(map->clearflooritem_timer, "map_clearflooritem_timer");
		timer->add_func_list(map->removemobs_timer, "map_removemobs_timer");
		timer->add_interval(timer->gettick()+1000, map->freeblock_timer, 0, 0, 60*1000);

	}
	HPM->event(HPET_INIT);

	atcommand->init(minimal);
	battle->init(minimal);
	instance->init(minimal);
	channel->init(minimal);
	chrif->init(minimal);
	clif->init(minimal);
	ircbot->init(minimal);
	script->init(minimal);
	itemdb->init(minimal);
	clan->init(minimal);
	skill->init(minimal);
	if (!minimal)
		map->read_zone_db();/* read after item and skill initialization */
	mob->init(minimal);
	pc->init(minimal);
	refine->init(minimal);
	grader->init(minimal);
	status->init(minimal);
	party->init(minimal);
	guild->init(minimal);
	gstorage->init(minimal);
	pet->init(minimal);
	homun->init(minimal);
	mercenary->init(minimal);
	elemental->init(minimal);
	quest->init(minimal);
	achievement->init(minimal);
	stylist->init(minimal);
	macro->init(minimal);
	enchantui->init(minimal);
	goldpc->init(minimal);
	npc->init(minimal);
	unit->init(minimal);
	bg->init(minimal);
	duel->init(minimal);
	vending->init(minimal);
	rodex->init(minimal);
	mapiif->init(minimal);

	if (map->scriptcheck) {
		bool failed = map->extra_scripts_count > 0 ? false : true;
		for (i = 0; i < map->extra_scripts_count; i++) {
			if (npc->parsesrcfile(map->extra_scripts[i], false) != EXIT_SUCCESS)
				failed = true;
		}
		if (failed)
			exit(EXIT_FAILURE);
		exit(EXIT_SUCCESS);
	}

	if (minimal) {
		HPM->event(HPET_READY);
		HPM->event(HPET_FINAL);
		battle->final();
		HPM_map_do_final();
		HPM->event(HPET_POST_FINAL);
		exit(EXIT_SUCCESS);
	}

	npc->event_do_oninit( false ); // Init npcs (OnInit)
	npc->market_fromsql(); /* after OnInit */
	npc->barter_fromsql(); /* after OnInit */
	npc->expanded_barter_fromsql(); /* after OnInit */

	if (battle_config.pk_mode)
		ShowNotice("Server is running on '"CL_WHITE"PK Mode"CL_RESET"'.\n");

	Sql_HerculesUpdateCheck(map->mysql_handle);

#ifdef CONSOLE_INPUT
	console->input->setSQL(map->mysql_handle);
	if (!minimal && core->runflag != CORE_ST_STOP)
		console->display_gplnotice();
#endif

	ShowStatus("Server is '"CL_GREEN"ready"CL_RESET"' and listening on port '"CL_WHITE"%d"CL_RESET"'.\n\n", map->port);

	if( core->runflag != CORE_ST_STOP ) {
		core->shutdown_callback = map->do_shutdown;
		core->runflag = MAPSERVER_ST_RUNNING;
	}

	map_cp_defaults();

	HPM->event(HPET_READY);

	return 0;
}

/*=====================================
 * Default Functions : map.h
 * Generated by HerculesInterfaceMaker
 * created by Susu
 *-------------------------------------*/
void map_defaults(void)
{
	map = &map_s;

	/* */
	map->minimal = false;
	map->scriptcheck = false;
	map->count = 0;
	map->retval = EXIT_SUCCESS;

	map->extra_scripts = NULL;
	map->extra_scripts_count = 0;

	sprintf(map->db_path ,"db");
	libconfig->set_db_path(map->db_path);
	sprintf(map->help_txt ,"conf/help.txt");
	sprintf(map->charhelp_txt ,"conf/charhelp.txt");

	sprintf(map->wisp_server_name ,"Server"); // can be modified in char-server configuration file

	map->autosave_interval = DEFAULT_MAP_AUTOSAVE_INTERVAL;
	map->minsave_interval = 100;
	map->save_settings = 0xFFFF;
	map->agit_flag = 0;
	map->agit2_flag = 0;
	map->night_flag = 0; // 0=day, 1=night [Yor]
	map->enable_spy = 0; //To enable/disable @spy commands, which consume too much cpu time when sending packets. [Skotlex]

	map->INTER_CONF_NAME="conf/common/inter-server.conf";
	map->LOG_CONF_NAME="conf/map/logs.conf";
	map->MAP_CONF_NAME = "conf/map/map-server.conf";
	map->BATTLE_CONF_FILENAME = "conf/map/battle.conf";
	map->ATCOMMAND_CONF_FILENAME = "conf/atcommand.conf";
	map->SCRIPT_CONF_NAME = "conf/map/script.conf";
	map->MSG_CONF_NAME = "conf/messages.conf";
	map->GRF_PATH_FILENAME = "conf/grf-files.txt";

	map->default_codepage[0] = '\0';
	map->server_port = 3306;
	sprintf(map->server_ip,"127.0.0.1");
	sprintf(map->server_id,"ragnarok");
	sprintf(map->server_pw,"ragnarok");
	sprintf(map->server_db,"ragnarok");
	map->mysql_handle = NULL;
	map->default_lang_str[0] = '\0';

	map->cpsd_active = false;

	map->port = 0;
	map->users = 0;
	map->ip_set = 0;
	map->char_ip_set = 0;
	map->enable_grf = 0;

	memset(&map->index2mapid, -1, sizeof(map->index2mapid));

	map->id_db = NULL;
	map->pc_db = NULL;
	map->mobid_db = NULL;
	map->bossid_db = NULL;
	map->nick_db = NULL;
	map->charid_db = NULL;
	map->regen_db = NULL;
	map->zone_db = NULL;
	map->iwall_db = NULL;

	map->block_free = NULL;
#ifdef SANITIZE
	map->block_free_sanitize = NULL;
#endif
	map->block_free_count = 0;
	map->block_free_lock = 0;
	map->block_free_list_size = 0;
	map->bl_list = NULL;
	map->bl_list_count = 0;
	map->bl_list_size = 0;

	//all in a big chunk, respects order
PRAGMA_GCC9(GCC diagnostic push)
PRAGMA_GCC9(GCC diagnostic ignored "-Warray-bounds")
	memset(ZEROED_BLOCK_POS(map), 0, ZEROED_BLOCK_SIZE(map));
PRAGMA_GCC9(GCC diagnostic pop)

	map->cpsd = NULL;
	map->list = NULL;

	map->iterator_ers = NULL;

	map->flooritem_ers = NULL;
	/* */
	map->bonus_id = SP_LAST_KNOWN;
	/* funcs */
	map->zone_init = map_zone_init;
	map->zone_remove = map_zone_remove;
	map->zone_remove_all = map_zone_remove_all;
	map->zone_apply = map_zone_apply;
	map->zone_change = map_zone_change;
	map->zone_change2 = map_zone_change2;
	map->zone_reload = map_zonedb_reload;

	map->getcell = map_getcell;
	map->setgatcell = map_setgatcell;

	map->cellfromcache = map_cellfromcache;
	// users
	map->setusers = map_setusers;
	map->getusers = map_getusers;
	map->usercount = map_usercount;
	// blocklist lock
	map->freeblock = map_freeblock;
	map->freeblock_lock = map_freeblock_lock;
	map->freeblock_unlock = map_freeblock_unlock;
	// blocklist manipulation
	map->addblock = map_addblock;
	map->delblock = map_delblock;
	map->moveblock = map_moveblock;
	//blocklist nb in one cell
	map->count_oncell = map_count_oncell;
	map->find_skill_unit_oncell = map_find_skill_unit_oncell;
	// search and creation
	map->get_new_object_id = map_get_new_object_id;
	map->search_free_cell = map_search_free_cell;
	map->closest_freecell = map_closest_freecell;
	//
	map->quit = map_quit;
	// npc
	map->addnpc = map_addnpc;
	// map item
	map->clearflooritem_timer = map_clearflooritem_timer;
	map->removemobs_timer = map_removemobs_timer;
	map->clearflooritem = map_clearflooritem;
	map->addflooritem = map_addflooritem;
	// player to map session
	map->addnickdb = map_addnickdb;
	map->delnickdb = map_delnickdb;
	map->reqnickdb = map_reqnickdb;
	map->charid2nick = map_charid2nick;
	map->charid2sd = map_charid2sd;

	map->vforeachpc = map_vforeachpc;
	map->foreachpc = map_foreachpc;
	map->vforeachmob = map_vforeachmob;
	map->foreachmob = map_foreachmob;
	map->vforeachnpc = map_vforeachnpc;
	map->foreachnpc = map_foreachnpc;
	map->vforeachregen = map_vforeachregen;
	map->foreachregen = map_foreachregen;
	map->vforeachiddb = map_vforeachiddb;
	map->foreachiddb = map_foreachiddb;

	map->vforeachinrange = map_vforeachinrange;
	map->foreachinrange = map_foreachinrange;
	map->vforeachinshootrange = map_vforeachinshootrange;
	map->foreachinshootrange = map_foreachinshootrange;
	map->vforeachinarea = map_vforeachinarea;
	map->foreachinarea = map_foreachinarea;
	map->vforcountinrange = map_vforcountinrange;
	map->forcountinrange = map_forcountinrange;
	map->vforcountinarea = map_vforcountinarea;
	map->forcountinarea = map_forcountinarea;
	map->vforeachinmovearea = map_vforeachinmovearea;
	map->foreachinmovearea = map_foreachinmovearea;
	map->vforeachincell = map_vforeachincell;
	map->foreachincell = map_foreachincell;
	map->vforeachinpath = map_vforeachinpath;
	map->foreachinpath = map_foreachinpath;
	map->vforeachinmap = map_vforeachinmap;
	map->foreachinmap = map_foreachinmap;
	map->forcountinmap = map_forcountinmap;
	map->vforeachininstance = map_vforeachininstance;
	map->foreachininstance = map_foreachininstance;

	map->id2sd = map_id2sd;
	map->id2nd = map_id2nd;
	map->id2md = map_id2md;
	map->id2fi = map_id2fi;
	map->id2cd = map_id2cd;
	map->id2su = map_id2su;
	map->id2pd = map_id2pd;
	map->id2hd = map_id2hd;
	map->id2mc = map_id2mc;
	map->id2ed = map_id2ed;
	map->id2bl = map_id2bl;
	map->blid_exists = map_blid_exists;
	map->mapindex2mapid = map_mapindex2mapid;
	map->mapname2mapid = map_mapname2mapid;
	map->addiddb = map_addiddb;
	map->deliddb = map_deliddb;
	/* */
	map->nick2sd = map_nick2sd;
	map->getmob_boss = map_getmob_boss;
	map->id2boss = map_id2boss;
	map->race_id2mask = map_race_id2mask;
	// reload config file looking only for npcs
	map->reloadnpc = map_reloadnpc;

	map->check_dir = map_check_dir;
	map->calc_dir = map_calc_dir;
	map->get_random_cell = map_get_random_cell;
	map->get_random_cell_in_range = map_get_random_cell_in_range;
	map->random_dir = map_random_dir; // [Skotlex]

	map->cleanup_sub = cleanup_sub;

	map->delmap = map_delmap;
	map->flags_init = map_flags_init;

	map->iwall_set = map_iwall_set;
	map->iwall_get = map_iwall_get;
	map->iwall_remove = map_iwall_remove;

	map->addmobtolist = map_addmobtolist; // [Wizputer]
	map->spawnmobs = map_spawnmobs; // [Wizputer]
	map->removemobs = map_removemobs; // [Wizputer]
	map->addmap2db = map_addmap2db;
	map->removemapdb = map_removemapdb;
	map->clean = map_clean;

	map->do_shutdown = do_shutdown;

	map->freeblock_timer = map_freeblock_timer;
	map->searchrandfreecell = map_searchrandfreecell;
	map->count_sub = map_count_sub;
	map->create_charid2nick = create_charid2nick;
	map->removemobs_sub = map_removemobs_sub;
	map->gat2cell = map_gat2cell;
	map->cell2gat = map_cell2gat;
	map->getcellp = map_getcellp;
	map->setcell = map_setcell;
	map->sub_getcellp = map_sub_getcellp;
	map->sub_setcell = map_sub_setcell;
	map->iwall_nextxy = map_iwall_nextxy;
	map->readfromcache = map_readfromcache;
	map->readfromcache_v1 = map_readfromcache_v1;
	map->addmap = map_addmap;
	map->delmapid = map_delmapid;
	map->zone_db_clear = map_zone_db_clear;
	map->list_final = do_final_maps;
	map->waterheight = map_waterheight;
	map->readgat = map_readgat;
	map->readallmaps = map_readallmaps;
	map->config_read = map_config_read;

	map->config_read_console = map_config_read_console;
	map->config_read_connection = map_config_read_connection;
	map->config_read_inter = map_config_read_inter;
	map->config_read_database = map_config_read_database;
	map->config_read_map_list = map_config_read_map_list;

	map->read_npclist = map_read_npclist;
	map->inter_config_read = inter_config_read;
	map->inter_config_read_database_names = inter_config_read_database_names;
	map->inter_config_read_connection = inter_config_read_connection;
	map->setting_lookup_const = map_setting_lookup_const;
	map->setting_lookup_const_mask = map_setting_lookup_const_mask;
	map->sql_init = map_sql_init;
	map->sql_close = map_sql_close;
	map->zone_mf_cache = map_zone_mf_cache;
	map->zone_str2itemid = map_zone_str2itemid;
	map->zone_str2skillid = map_zone_str2skillid;
	map->zone_bl_type = map_zone_bl_type;
	map->read_zone_db = read_map_zone_db;
	map->nick_db_final = nick_db_final;
	map->cleanup_db_sub = cleanup_db_sub;
	map->abort_sub = map_abort_sub;

	map->update_cell_bl = map_update_cell_bl;
	map->get_new_bonus_id = map_get_new_bonus_id;

	map->add_questinfo = map_add_questinfo;
	map->remove_questinfo = map_remove_questinfo;

	map->merge_zone = map_merge_zone;
	map->zone_clear_single = map_zone_clear_single;

	map->lock_check = map_lock_check;
}

void mapit_defaults(void)
{
	/**
	 * mapit interface
	 **/
	mapit = &mapit_s;

	mapit->alloc = mapit_alloc;
	mapit->free = mapit_free;
	mapit->first = mapit_first;
	mapit->last = mapit_last;
	mapit->next = mapit_next;
	mapit->prev = mapit_prev;
	mapit->exists = mapit_exists;
}
