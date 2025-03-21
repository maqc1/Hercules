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

#include "config/core.h" // DBPATH, RENEWAL
#include "itemdb.h"

#include "map/battle.h" // struct battle_config
#include "map/map.h"
#include "map/mob.h"    // MAX_MOB_DB
#include "map/pc.h"     // W_MUSICAL, W_WHIP
#include "map/refine.h"
#include "map/script.h" // item script processing
#include "common/HPM.h"
#include "common/conf.h"
#include "common/memmgr.h"
#include "common/nullpo.h"
#include "common/random.h"
#include "common/showmsg.h"
#include "common/strlib.h"
#include "common/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct itemdb_interface itemdb_s;
struct itemdb_interface *itemdb;

/**
 * Search for item name
 * name = item alias, so we should find items aliases first. if not found then look for "jname" (full name)
 * @see DBApply
 */
static int itemdb_searchname_sub(union DBKey key, struct DBData *data, va_list ap)
{
	struct item_data *item = DB->data2ptr(data), **dst, **dst2;
	char *str;
	str=va_arg(ap,char *);
	nullpo_ret(str);
	dst=va_arg(ap,struct item_data **);
	nullpo_ret(dst);
	dst2=va_arg(ap,struct item_data **);
	nullpo_ret(dst2);
	if (item == &itemdb->dummy) return 0;

	//Absolute priority to Aegis code name.
	if (*dst != NULL) return 0;
	if ( battle_config.case_sensitive_aegisnames && strcmp(item->name,str) == 0 )
		*dst=item;
	else if ( !battle_config.case_sensitive_aegisnames && strcasecmp(item->name,str) == 0 )
		*dst=item;

	//Second priority to Client displayed name.
	if (*dst2 != NULL) return 0;
	if( strcmpi(item->jname,str)==0 )
		*dst2=item;
	return 0;
}

/*==========================================
 * Return item data from item name. (lookup)
 *------------------------------------------*/
static struct item_data *itemdb_searchname(const char *str)
{
	struct item_data* item;
	struct item_data* item2=NULL;
	int i;

	nullpo_retr(NULL, str);
	for( i = 0; i < ARRAYLENGTH(itemdb->array); ++i ) {
		item = itemdb->array[i];
		if( item == NULL )
			continue;

		// Absolute priority to Aegis code name.
		if ( battle_config.case_sensitive_aegisnames && strcmp(item->name,str) == 0 )
			return item;
		else if ( !battle_config.case_sensitive_aegisnames && strcasecmp(item->name,str) == 0 )
			return item;

		//Second priority to Client displayed name.
		if (!item2 && strcasecmp(item->jname,str) == 0)
			item2 = item;
	}

	item = NULL;
	itemdb->other->foreach(itemdb->other,itemdb->searchname_sub,str,&item,&item2);
	return item?item:item2;
}
/* name to item data */
static struct item_data *itemdb_name2id(const char *str)
{
	return strdb_get(itemdb->names,str);
}

/**
 * @see DBMatcher
 */
static int itemdb_searchname_array_sub(union DBKey key, struct DBData data, va_list ap)
{
	struct item_data *itd = DB->data2ptr(&data);
	const char *str = va_arg(ap, const char *);
	enum item_name_search_flag flag = va_arg(ap, enum item_name_search_flag);

	nullpo_ret(str);

	if (itd == &itemdb->dummy)
		return 1; //Invalid item.

	if (
		(flag == IT_SEARCH_NAME_PARTIAL
		 && (stristr(itd->jname, str) != NULL
			 || (battle_config.case_sensitive_aegisnames && strstr(itd->name, str))
			 || (!battle_config.case_sensitive_aegisnames && stristr(itd->name, str))
			 ))
		|| (flag == IT_SEARCH_NAME_EXACT
			&& (strcmp(itd->jname, str) == 0
				|| (battle_config.case_sensitive_aegisnames && strcmp(itd->name, str) == 0)
				|| (!battle_config.case_sensitive_aegisnames && strcasecmp(itd->name, str) == 0)
				))
		) {

		return 0;
	} else {
		return 1;
	}
}

/**
 * Finds up to passed size matches
 * @param data array of struct item_data for returning the results in
 * @param size size of the array
 * @param str string used in this search
 * @param flag search mode refer to enum item_name_search_flag for possible values
 * @return returns all found matches in the database which could be bigger than size
 **/
static int itemdb_searchname_array(struct item_data **data, const int size, const char *str, enum item_name_search_flag flag)
{
	nullpo_ret(data);
	nullpo_ret(str);
	Assert_ret(flag >= IT_SEARCH_NAME_PARTIAL && flag < IT_SEARCH_NAME_MAX);
	Assert_ret(size > 0);

	int
		results_count = 0,
		length = 0;

	// Search in array
	for (int i = 0; i < ARRAYLENGTH(itemdb->array); ++i) {
		struct item_data *itd = itemdb->array[i];

		if (itd == NULL)
			continue;

		if (
			(flag == IT_SEARCH_NAME_PARTIAL
			 && (stristr(itd->jname, str) != NULL
				 || (battle_config.case_sensitive_aegisnames && strstr(itd->name, str))
				 || (!battle_config.case_sensitive_aegisnames && stristr(itd->name, str))
				 ))
			|| (flag == IT_SEARCH_NAME_EXACT
				&& (strcmp(itd->jname, str) == 0
					|| (battle_config.case_sensitive_aegisnames && strcmp(itd->name, str) == 0)
					|| (!battle_config.case_sensitive_aegisnames && strcasecmp(itd->name, str) == 0)
					))
			) {
			if (length < size) {
				data[length] = itd;
				++length;
			}

			++results_count;
		}
	}

	// Search in dbmap
	int dbmap_size = size - length;
	if (dbmap_size > 0) {
		struct DBData **dbmap_data = NULL;
		int dbmap_count = 0;
		CREATE(dbmap_data, struct DBData *, dbmap_size);

		dbmap_count = itemdb->other->getall(itemdb->other, dbmap_data, dbmap_size, itemdb->searchname_array_sub, str, flag);
		dbmap_size = min(dbmap_count, dbmap_size);

		for (int i = 0; i < dbmap_size; ++i) {
			data[length] = DB->data2ptr(dbmap_data[i]);
			++length;
		}

		results_count += dbmap_count;
		aFree(dbmap_data);
	} else { // We got all matches we can return, so we only need to count now.
		results_count += itemdb->other->getall(itemdb->other, NULL, 0, itemdb->searchname_array_sub, str, flag);
	}

	return results_count;
}

/* [Ind/Hercules] */
static int itemdb_chain_item(unsigned short chain_id, int *rate)
{
	struct item_chain_entry *entry;

	if( chain_id >= itemdb->chain_count ) {
		ShowError("itemdb_chain_item: unknown chain id %d\n", chain_id);
		return UNKNOWN_ITEM_ID;
	}

	entry = &itemdb->chains[chain_id].items[ rnd()%itemdb->chains[chain_id].qty ];

	if( rnd()%10000 >= entry->rate )
		return 0;

	if( rate )
		rate[0] = entry->rate;
	return entry->id;
}
/* [Ind/Hercules] */
static void itemdb_package_item(struct map_session_data *sd, struct item_package *package)
{
	int i = 0, get_count, j, flag;

	nullpo_retv(sd);
	nullpo_retv(package);
	for( i = 0; i < package->must_qty; i++ ) {
		struct item it;
		memset(&it, 0, sizeof(it));

		it.nameid = package->must_items[i].id;
		it.identify = 1;

		if( package->must_items[i].hours ) {
			it.expire_time = (unsigned int)(time(NULL) + ((package->must_items[i].hours*60)*60));
		}

		if( package->must_items[i].named ) {
			it.card[0] = CARD0_FORGE;
			it.card[1] = 0;
			it.card[2] = GetWord(sd->status.char_id, 0);
			it.card[3] = GetWord(sd->status.char_id, 1);
		}

		if( package->must_items[i].announce )
			clif->package_announce(sd, package->must_items[i].id, package->id, it.refine);

		if ( package->must_items[i].force_serial )
			it.unique_id = itemdb->unique_id(sd);

		get_count = itemdb->isstackable(package->must_items[i].id) ? package->must_items[i].qty : 1;

		it.amount = get_count == 1 ? 1 : get_count;

		for( j = 0; j < package->must_items[i].qty; j += get_count ) {
			if ( ( flag = pc->additem(sd, &it, get_count, LOG_TYPE_SCRIPT) ) )
				clif->additem(sd, 0, 0, flag);
		}
	}

	if( package->random_qty ) {
		for( i = 0; i < package->random_qty; i++ ) {
			struct item_package_rand_entry *entry;

			entry = &package->random_groups[i].random_list[rnd()%package->random_groups[i].random_qty];

			while( 1 ) {
				if( rnd()%10000 >= entry->rate ) {
					entry = entry->next;
					continue;
				} else {
					struct item it;
					memset(&it, 0, sizeof(it));

					it.nameid = entry->id;
					it.identify = 1;

					if( entry->hours ) {
						it.expire_time = (unsigned int)(time(NULL) + ((entry->hours*60)*60));
					}

					if( entry->named ) {
						it.card[0] = CARD0_FORGE;
						it.card[1] = 0;
						it.card[2] = GetWord(sd->status.char_id, 0);
						it.card[3] = GetWord(sd->status.char_id, 1);
					}

					if( entry->announce )
						clif->package_announce(sd, entry->id, package->id, it.refine);

					get_count = itemdb->isstackable(entry->id) ? entry->qty : 1;

					it.amount = get_count == 1 ? 1 : get_count;

					for( j = 0; j < entry->qty; j += get_count ) {
						if ( ( flag = pc->additem(sd, &it, get_count, LOG_TYPE_SCRIPT) ) )
							clif->additem(sd, 0, 0, flag);
					}
					break;
				}
			}
		}
	}
}

/*==========================================
 * Return a random item id from group. (takes into account % chance giving/tot group)
 *------------------------------------------*/
static int itemdb_searchrandomid(struct item_group *group)
{

	nullpo_retr(UNKNOWN_ITEM_ID, group);
	if (group->qty)
		return group->nameid[rnd()%group->qty];

	ShowError("itemdb_searchrandomid: No item entries for group id %d\n", group->id);
	return UNKNOWN_ITEM_ID;
}
static bool itemdb_in_group(struct item_group *group, int nameid)
{
	int i;

	nullpo_retr(false, group);
	for( i = 0; i < group->qty; i++ )
		if( group->nameid[i] == nameid )
			return true;

	return false;
}

/**
 * Search for an item group.
 *
 * @param nameid item group id to search
 * @return A pointer to the group
 * @retval NULL if the group was not found
 */
static const struct item_group *itemdb_search_group(int nameid)
{
	int i;
	ARR_FIND(0, itemdb->group_count, i, itemdb->groups[i].id == nameid);
	if (i != itemdb->group_count)
		return &itemdb->groups[i];
	return NULL;
}

/// Searches for the item_data.
/// Returns the item_data or NULL if it does not exist.
static struct item_data *itemdb_exists(int nameid)
{
	struct item_data* item;

	if( nameid >= 0 && nameid < ARRAYLENGTH(itemdb->array) )
		return itemdb->array[nameid];
	item = (struct item_data*)idb_get(itemdb->other,nameid);
	if( item == &itemdb->dummy )
		return NULL;// dummy data, doesn't exist
	return item;
}

/**
 * Searches for the item_option data.
 * @param option_index as the index of the item option (client side).
 * @return pointer to struct itemdb_option data or NULL.
 */
static struct itemdb_option *itemdb_option_exists(int idx)
{
	return (struct itemdb_option *)idb_get(itemdb->options, idx);
}

/**
 * Searches for the item_reform data.
 * @param index as the index of the item reform.
 * @return pointer to struct itemdb_reform data or NULL.
 */
static struct item_reform *itemdb_reform_exists(int idx)
{
	return (struct item_reform *)idb_get(itemdb->reform, idx);
}

/// Returns human readable name for given item type.
/// @param type Type id to retrieve name for ( IT_* ).
static const char *itemdb_typename(enum item_types type)
{
	switch(type)
	{
		case IT_HEALING:        return "Potion/Food";
		case IT_USABLE:         return "Usable";
		case IT_ETC:            return "Etc.";
		case IT_WEAPON:         return "Weapon";
		case IT_ARMOR:          return "Armor";
		case IT_CARD:           return "Card";
		case IT_PETEGG:         return "Pet Egg";
		case IT_PETARMOR:       return "Pet Accessory";
		case IT_AMMO:           return "Arrow/Ammunition";
		case IT_DELAYCONSUME:   return "Delay-Consume Usable";
		case IT_SELECTPACKAGE:  return "Selection Package Usable";
		case IT_CASH:           return "Cash Usable";
		case IT_UNKNOWN2:
		case IT_MAX:
		case IT_UNKNOWN:        return "Unknown Type";
	}
	return "Unknown Type";
}

/**
 * Converts the JobID to the format used by map-server to check item
 * restriction as per job.
 *
 * @param bclass Pointer to the variable containing the new format
 * @param job_id Variable containing JobID
 * @param enable Boolean value which (un)set the restriction.
 *
 * @author Dastgir
 */
static void itemdb_jobid2mapid(uint64 *bclass, int job_class, bool enable)
{
	uint64 mask[3] = { 0 };
	int i;

	nullpo_retv(bclass);

	switch (job_class) {
		// Base Classes
		case JOB_NOVICE:
		case JOB_SUPER_NOVICE:
			mask[0] = 1ULL << MAPID_NOVICE;
			mask[1] = 1ULL << MAPID_NOVICE;
			break;
		case JOB_SWORDMAN:
			mask[0] = 1ULL << MAPID_SWORDMAN;
			break;
		case JOB_MAGE:
			mask[0] = 1ULL << MAPID_MAGE;
			break;
		case JOB_ARCHER:
			mask[0] = 1ULL << MAPID_ARCHER;
			break;
		case JOB_ACOLYTE:
			mask[0] = 1ULL << MAPID_ACOLYTE;
			break;
		case JOB_MERCHANT:
			mask[0] = 1ULL << MAPID_MERCHANT;
			break;
		case JOB_THIEF:
			mask[0] = 1ULL << MAPID_THIEF;
			break;
		// 2-1 Classes
		case JOB_KNIGHT:
			mask[1] = 1ULL << MAPID_SWORDMAN;
			break;
		case JOB_PRIEST:
			mask[1] = 1ULL << MAPID_ACOLYTE;
			break;
		case JOB_WIZARD:
			mask[1] = 1ULL << MAPID_MAGE;
			break;
		case JOB_BLACKSMITH:
			mask[1] = 1ULL << MAPID_MERCHANT;
			break;
		case JOB_HUNTER:
			mask[1] = 1ULL << MAPID_ARCHER;
			break;
		case JOB_ASSASSIN:
			mask[1] = 1ULL << MAPID_THIEF;
			break;
		// 2-2 Classes
		case JOB_CRUSADER:
			mask[2] = 1ULL << MAPID_SWORDMAN;
			break;
		case JOB_MONK:
			mask[2] = 1ULL << MAPID_ACOLYTE;
			break;
		case JOB_SAGE:
			mask[2] = 1ULL << MAPID_MAGE;
			break;
		case JOB_ALCHEMIST:
			mask[2] = 1ULL << MAPID_MERCHANT;
			break;
		case JOB_BARD:
			mask[2] = 1ULL << MAPID_ARCHER;
			break;
		case JOB_ROGUE:
			mask[2] = 1ULL << MAPID_THIEF;
			break;
		// Extended Classes
		case JOB_TAEKWON:
			mask[0] = 1ULL << MAPID_TAEKWON;
			break;
		case JOB_STAR_GLADIATOR:
			mask[1] = 1ULL << MAPID_TAEKWON;
			break;
		case JOB_SOUL_LINKER:
			mask[2] = 1ULL << MAPID_TAEKWON;
			break;
		case JOB_GUNSLINGER:
			mask[0] = 1ULL << MAPID_GUNSLINGER;
			mask[1] = 1ULL << MAPID_GUNSLINGER;
			break;
		case JOB_NINJA:
			mask[0] = 1ULL << MAPID_NINJA;
			mask[1] = 1ULL << MAPID_NINJA;
			break;
		case JOB_KAGEROU:
		case JOB_OBORO:
			mask[1] = 1ULL << MAPID_NINJA;
			break;
		case JOB_REBELLION:
			mask[1] = 1ULL << MAPID_GUNSLINGER;
			break;
		case JOB_SUMMONER:
			mask[0] = 1ULL << MAPID_SUMMONER;
			break;
		// Other Classes
		case JOB_GANGSI: //Bongun/Munak
			mask[0] = 1ULL << MAPID_GANGSI;
			break;
		case JOB_DEATH_KNIGHT:
			mask[1] = 1ULL << MAPID_GANGSI;
			break;
		case JOB_DARK_COLLECTOR:
			mask[2] = 1ULL << MAPID_GANGSI;
			break;
	}

	for (i = 0; i < ARRAYLENGTH(mask); i++) {
		if (mask[i] == 0)
			continue;
		if (enable)
			bclass[i] |= mask[i];
		else
			bclass[i] &= ~mask[i];
	}
}

/**
 * Converts the JobMask to the format used by map-server to check item
 * restriction as per job.
 *
 * @param bclass  Pointer to the variable containing the new format.
 * @param jobmask Variable containing JobMask.
 */
static void itemdb_jobmask2mapid(uint64 *bclass, uint64 jobmask)
{
	nullpo_retv(bclass);
	bclass[0] = bclass[1] = bclass[2] = 0;
	//Base classes
	if (jobmask & 1ULL<<JOB_NOVICE) {
		//Both Novice/Super-Novice are counted with the same ID
		bclass[0] |= 1ULL<<MAPID_NOVICE;
		bclass[1] |= 1ULL<<MAPID_NOVICE;
	}
	if (jobmask & 1ULL<<JOB_SWORDMAN)
		bclass[0] |= 1ULL<<MAPID_SWORDMAN;
	if (jobmask & 1ULL<<JOB_MAGE)
		bclass[0] |= 1ULL<<MAPID_MAGE;
	if (jobmask & 1ULL<<JOB_ARCHER)
		bclass[0] |= 1ULL<<MAPID_ARCHER;
	if (jobmask & 1ULL<<JOB_ACOLYTE)
		bclass[0] |= 1ULL<<MAPID_ACOLYTE;
	if (jobmask & 1ULL<<JOB_MERCHANT)
		bclass[0] |= 1ULL<<MAPID_MERCHANT;
	if (jobmask & 1ULL<<JOB_THIEF)
		bclass[0] |= 1ULL<<MAPID_THIEF;
	//2-1 classes
	if (jobmask & 1ULL<<JOB_KNIGHT)
		bclass[1] |= 1ULL<<MAPID_SWORDMAN;
	if (jobmask & 1ULL<<JOB_PRIEST)
		bclass[1] |= 1ULL<<MAPID_ACOLYTE;
	if (jobmask & 1ULL<<JOB_WIZARD)
		bclass[1] |= 1ULL<<MAPID_MAGE;
	if (jobmask & 1ULL<<JOB_BLACKSMITH)
		bclass[1] |= 1ULL<<MAPID_MERCHANT;
	if (jobmask & 1ULL<<JOB_HUNTER)
		bclass[1] |= 1ULL<<MAPID_ARCHER;
	if (jobmask & 1ULL<<JOB_ASSASSIN)
		bclass[1] |= 1ULL<<MAPID_THIEF;
	//2-2 classes
	if (jobmask & 1ULL<<JOB_CRUSADER)
		bclass[2] |= 1ULL<<MAPID_SWORDMAN;
	if (jobmask & 1ULL<<JOB_MONK)
		bclass[2] |= 1ULL<<MAPID_ACOLYTE;
	if (jobmask & 1ULL<<JOB_SAGE)
		bclass[2] |= 1ULL<<MAPID_MAGE;
	if (jobmask & 1ULL<<JOB_ALCHEMIST)
		bclass[2] |= 1ULL<<MAPID_MERCHANT;
	if (jobmask & 1ULL<<JOB_BARD)
		bclass[2] |= 1ULL<<MAPID_ARCHER;
#if 0 // Bard/Dancer share the same slot now.
	if (jobmask & 1ULL<<JOB_DANCER)
		bclass[2] |= 1ULL<<MAPID_ARCHER;
#endif // 0
	if (jobmask & 1ULL<<JOB_ROGUE)
		bclass[2] |= 1ULL<<MAPID_THIEF;
	//Special classes that don't fit above.
	if (jobmask & 1ULL<<21) //Taekwon boy
		bclass[0] |= 1ULL<<MAPID_TAEKWON;
	if (jobmask & 1ULL<<22) //Star Gladiator
		bclass[1] |= 1ULL<<MAPID_TAEKWON;
	if (jobmask & 1ULL<<23) //Soul Linker
		bclass[2] |= 1ULL<<MAPID_TAEKWON;
	if (jobmask & 1ULL<<JOB_GUNSLINGER) {
		//Rebellion job can equip Gunslinger equips. [Rytech]
		bclass[0] |= 1ULL<<MAPID_GUNSLINGER;
		bclass[1] |= 1ULL<<MAPID_GUNSLINGER;
	}
	if (jobmask & 1ULL<<JOB_NINJA) {
		//Kagerou/Oboro jobs can equip Ninja equips. [Rytech]
		bclass[0] |= 1ULL<<MAPID_NINJA;
		bclass[1] |= 1ULL<<MAPID_NINJA;
	}
	if (jobmask & 1ULL<<26) //Bongun/Munak
		bclass[0] |= 1ULL<<MAPID_GANGSI;
	if (jobmask & 1ULL<<27) //Death Knight
		bclass[1] |= 1ULL<<MAPID_GANGSI;
	if (jobmask & 1ULL<<28) //Dark Collector
		bclass[2] |= 1ULL<<MAPID_GANGSI;
	if (jobmask & 1ULL<<29) //Kagerou / Oboro
		bclass[1] |= 1ULL<<MAPID_NINJA;
	if (jobmask & 1ULL<<30) //Rebellion
		bclass[1] |= 1ULL<<MAPID_GUNSLINGER;
	if (jobmask & 1ULL<<31) //Summoner
		bclass[0] |= 1ULL<<MAPID_SUMMONER;
}

static void create_dummy_data(void)
{
	memset(&itemdb->dummy, 0, sizeof(struct item_data));
	itemdb->dummy.nameid=500;
	itemdb->dummy.weight=1;
	itemdb->dummy.value_sell=1;
	itemdb->dummy.type=IT_ETC; //Etc item
	safestrncpy(itemdb->dummy.name,"UNKNOWN_ITEM",sizeof(itemdb->dummy.name));
	safestrncpy(itemdb->dummy.jname,"UNKNOWN_ITEM",sizeof(itemdb->dummy.jname));
	itemdb->dummy.view_id=UNKNOWN_ITEM_ID;
}

static struct item_data *create_item_data(int nameid)
{
	struct item_data *id;
	CREATE(id, struct item_data, 1);
	id->nameid = nameid;
	id->weight = 1;
	id->type = IT_ETC;
	return id;
}

/*==========================================
 * Loads (and creates if not found) an item from the db.
 *------------------------------------------*/
static struct item_data *itemdb_load(int nameid)
{
	struct item_data *id;

	if( nameid >= 0 && nameid < ARRAYLENGTH(itemdb->array) )
	{
		id = itemdb->array[nameid];
		if( id == NULL || id == &itemdb->dummy )
			id = itemdb->array[nameid] = itemdb->create_item_data(nameid);
		return id;
	}

	id = (struct item_data*)idb_get(itemdb->other, nameid);
	if( id == NULL || id == &itemdb->dummy )
	{
		id = itemdb->create_item_data(nameid);
		idb_put(itemdb->other, nameid, id);
	}
	return id;
}

/*==========================================
 * Loads an item from the db. If not found, it will return the dummy item.
 *------------------------------------------*/
static struct item_data *itemdb_search(int nameid)
{
	struct item_data* id;
	if( nameid >= 0 && nameid < ARRAYLENGTH(itemdb->array) )
		id = itemdb->array[nameid];
	else
		id = (struct item_data*)idb_get(itemdb->other, nameid);

	if( id == NULL )
	{
		ShowWarning("itemdb_search: Item ID %d does not exists in the item_db. Using dummy data.\n", nameid);
		id = &itemdb->dummy;
		itemdb->dummy.nameid = nameid;
	}
	return id;
}

/*==========================================
 * Returns if given item is a player-equippable piece.
 *------------------------------------------*/
static int itemdb_isequip(int nameid)
{
	enum item_types type = itemdb_type(nameid);
	switch (type) {
		case IT_WEAPON:
		case IT_ARMOR:
		case IT_AMMO:
			return 1;
		case IT_HEALING:
		case IT_UNKNOWN:
		case IT_USABLE:
		case IT_ETC:
		case IT_CARD:
		case IT_PETEGG:
		case IT_PETARMOR:
		case IT_UNKNOWN2:
		case IT_DELAYCONSUME:
		case IT_SELECTPACKAGE:
		case IT_CASH:
		case IT_MAX:
		default:
			return 0;
	}
}

/*==========================================
 * Alternate version of itemdb_isequip
 *------------------------------------------*/
static int itemdb_isequip2(struct item_data *data)
{
	nullpo_ret(data);
	switch (data->type) {
		case IT_WEAPON:
		case IT_ARMOR:
		case IT_AMMO:
			return 1;
		default:
			return 0;
	}
}

/*==========================================
 * Returns if given item's type is stackable.
 *------------------------------------------*/
static int itemdb_isstackable(int nameid)
{
	enum item_types type = itemdb_type(nameid);
	switch (type) {
		case IT_WEAPON:
		case IT_ARMOR:
		case IT_PETEGG:
		case IT_PETARMOR:
			return 0;
		case IT_HEALING:
		case IT_UNKNOWN:
		case IT_USABLE:
		case IT_ETC:
		case IT_CARD:
		case IT_UNKNOWN2:
		case IT_AMMO:
		case IT_DELAYCONSUME:
		case IT_SELECTPACKAGE:
		case IT_CASH:
		case IT_MAX:
		default:
			return 1;
	}
}

/*==========================================
 * Alternate version of itemdb_isstackable
 *------------------------------------------*/
static int itemdb_isstackable2(struct item_data *data)
{
	nullpo_ret(data);
	switch (data->type) {
		case IT_WEAPON:
		case IT_ARMOR:
		case IT_PETEGG:
		case IT_PETARMOR:
			return 0;
		case IT_HEALING:
		case IT_UNKNOWN:
		case IT_USABLE:
		case IT_ETC:
		case IT_CARD:
		case IT_UNKNOWN2:
		case IT_AMMO:
		case IT_DELAYCONSUME:
		case IT_CASH:
		case IT_MAX:
		default:
			return 1;
	}
}

/*==========================================
 * Trade Restriction functions [Skotlex]
 *------------------------------------------*/
static int itemdb_isdropable_sub(struct item_data *item, int gmlv, int unused)
{
	return (item && (!(item->flag.trade_restriction&ITR_NODROP) || gmlv >= item->gm_lv_trade_override));
}

static int itemdb_cantrade_sub(struct item_data *item, int gmlv, int gmlv2)
{
	return (item && (!(item->flag.trade_restriction&ITR_NOTRADE) || gmlv >= item->gm_lv_trade_override || gmlv2 >= item->gm_lv_trade_override));
}

static int itemdb_canpartnertrade_sub(struct item_data *item, int gmlv, int gmlv2)
{
	return (item && (item->flag.trade_restriction&ITR_PARTNEROVERRIDE || gmlv >= item->gm_lv_trade_override || gmlv2 >= item->gm_lv_trade_override));
}

static int itemdb_cansell_sub(struct item_data *item, int gmlv, int unused)
{
	return (item && (!(item->flag.trade_restriction&ITR_NOSELLTONPC) || gmlv >= item->gm_lv_trade_override));
}

static int itemdb_cancartstore_sub(struct item_data *item, int gmlv, int unused)
{
	return (item && (!(item->flag.trade_restriction&ITR_NOCART) || gmlv >= item->gm_lv_trade_override));
}

static int itemdb_canstore_sub(struct item_data *item, int gmlv, int unused)
{
	return (item && (!(item->flag.trade_restriction&ITR_NOSTORAGE) || gmlv >= item->gm_lv_trade_override));
}

static int itemdb_canguildstore_sub(struct item_data *item, int gmlv, int unused)
{
	return (item && (!(item->flag.trade_restriction&ITR_NOGSTORAGE) || gmlv >= item->gm_lv_trade_override));
}

static int itemdb_canmail_sub(struct item_data *item, int gmlv, int unused)
{
	return (item && (!(item->flag.trade_restriction&ITR_NOMAIL) || gmlv >= item->gm_lv_trade_override));
}

static int itemdb_canauction_sub(struct item_data *item, int gmlv, int unused)
{
	return (item && (!(item->flag.trade_restriction&ITR_NOAUCTION) || gmlv >= item->gm_lv_trade_override));
}

static int itemdb_isrestricted(struct item *item, int gmlv, int gmlv2, int (*func)(struct item_data*, int, int))
{
	struct item_data* item_data;
	int i;

	nullpo_ret(item);
	item_data = itemdb->search(item->nameid);
	if (!func(item_data, gmlv, gmlv2))
		return 0;

	if(item_data->slot == 0 || itemdb_isspecial(item->card[0]))
		return 1;

	for(i = 0; i < item_data->slot; i++) {
		if (!item->card[i]) continue;
		if (!func(itemdb->search(item->card[i]), gmlv, gmlv2))
			return 0;
	}
	return 1;
}

/*==========================================
 * Specifies if item-type should drop unidentified.
 *------------------------------------------*/
static int itemdb_isidentified(int nameid)
{
	enum item_types type = itemdb_type(nameid);
	switch (type) {
		case IT_WEAPON:
		case IT_ARMOR:
		case IT_PETARMOR:
			return 0;
		case IT_HEALING:
		case IT_UNKNOWN:
		case IT_USABLE:
		case IT_ETC:
		case IT_CARD:
		case IT_PETEGG:
		case IT_UNKNOWN2:
		case IT_AMMO:
		case IT_DELAYCONSUME:
		case IT_SELECTPACKAGE:
		case IT_CASH:
		case IT_MAX:
		default:
			return 1;
	}
}
/* same as itemdb_isidentified but without a lookup */
static int itemdb_isidentified2(struct item_data *data)
{
	nullpo_ret(data);
	switch (data->type) {
		case IT_WEAPON:
		case IT_ARMOR:
		case IT_PETARMOR:
			return 0;
		default:
			return 1;
	}
}

static void itemdb_read_groups(void)
{
	struct config_t item_group_conf;
	struct config_setting_t *itg = NULL, *it = NULL;
	char config_filename[256];
	libconfig->format_db_path(DBPATH"item_group.conf", config_filename, sizeof(config_filename));
	const char *itname;
	int i = 0, count = 0, c;
	unsigned int *gsize = NULL;

	if (!libconfig->load_file(&item_group_conf, config_filename))
		return;

	gsize = aMalloc( libconfig->setting_length(item_group_conf.root) * sizeof(unsigned int) );

	for(i = 0; i < libconfig->setting_length(item_group_conf.root); i++)
		gsize[i] = 0;

	i = 0;
	while( (itg = libconfig->setting_get_elem(item_group_conf.root,i++)) ) {
		const char *name = config_setting_name(itg);

		if( !itemdb->name2id(name) ) {
			ShowWarning("itemdb_read_groups: unknown group item '%s', skipping..\n",name);
			config_setting_remove(item_group_conf.root, name);
			--i;
			continue;
		}

		c = 0;
		while( (it = libconfig->setting_get_elem(itg,c++)) ) {
			if( config_setting_is_list(it) )
				gsize[ i - 1 ] += libconfig->setting_get_int_elem(it,1);
			else
				gsize[ i - 1 ] += 1;
		}
	}

	i = 0;
	CREATE(itemdb->groups, struct item_group, libconfig->setting_length(item_group_conf.root));
	itemdb->group_count = (unsigned short)libconfig->setting_length(item_group_conf.root);

	while( (itg = libconfig->setting_get_elem(item_group_conf.root,i++)) ) {
		struct item_data *data = itemdb->name2id(config_setting_name(itg));
		int ecount = 0;

		data->group = &itemdb->groups[count];

		itemdb->groups[count].id = data->nameid;
		itemdb->groups[count].qty = gsize[ count ];

		CREATE(itemdb->groups[count].nameid, int, gsize[count] + 1);
		c = 0;
		while( (it = libconfig->setting_get_elem(itg,c++)) ) {
			int repeat = 1;
			if( config_setting_is_list(it) ) {
				itname = libconfig->setting_get_string_elem(it,0);
				repeat = libconfig->setting_get_int_elem(it,1);
			} else
				itname = libconfig->setting_get_string_elem(itg,c - 1);

			if (itname[0] == 'I' && itname[1] == 'D' && strlen(itname) <= 12) {
				if( !( data = itemdb->exists(atoi(itname+2)) ) )
					ShowWarning("itemdb_read_groups: unknown item ID '%d' in group '%s'!\n",atoi(itname+2),config_setting_name(itg));
			} else if( !( data = itemdb->name2id(itname) ) )
				ShowWarning("itemdb_read_groups: unknown item '%s' in group '%s'!\n",itname,config_setting_name(itg));

			itemdb->groups[count].nameid[ecount] = data ? data->nameid : 0;
			if( repeat > 1 ) {
				//memset would be better? I failed to get the following to work though hu
				//memset(&itemdb->groups[count].nameid[ecount+1],itemdb->groups[count].nameid[ecount],sizeof(itemdb->groups[count].nameid[0])*repeat);
				int z;
				for( z = ecount+1; z < ecount+repeat; z++ )
					itemdb->groups[count].nameid[z] = itemdb->groups[count].nameid[ecount];
			}
			ecount += repeat;
		}
		count++;
	}

	libconfig->destroy(&item_group_conf);
	aFree(gsize);
	ShowStatus("Done reading '"CL_WHITE"%d"CL_RESET"' entries in '"CL_WHITE"%s"CL_RESET"'.\n", count, config_filename);
}

/* [Ind/Hercules] - HCache for Packages */
static void itemdb_write_cached_packages(const char *config_filename)
{
	FILE *file;
	unsigned short pcount = itemdb->package_count;
	unsigned short i;

	nullpo_retv(config_filename);
	if( !(file = HCache->open(config_filename,"wb")) ) {
		return;
	}

	// first 2 bytes = package count
	hwrite(&pcount,sizeof(pcount),1,file);

	for(i = 0; i < pcount; i++) {
		int id = itemdb->packages[i].id;
		unsigned short random_qty = itemdb->packages[i].random_qty, must_qty = itemdb->packages[i].must_qty;
		unsigned short c;
		//into a package, first 2 bytes = id.
		hwrite(&id,sizeof(id),1,file);
		//next 2 bytes = must count
		hwrite(&must_qty,sizeof(must_qty),1,file);
		//next 2 bytes = random count
		hwrite(&random_qty,sizeof(random_qty),1,file);
		//now we loop into must
		for(c = 0; c < must_qty; c++) {
			struct item_package_must_entry *entry = &itemdb->packages[i].must_items[c];
			unsigned char announce = entry->announce == 1 ? 1 : 0, named = entry->named == 1 ? 1 : 0, force_serial = entry->force_serial == 1 ? 1 : 0;
			//first 2 byte = item id
			hwrite(&entry->id,sizeof(entry->id),1,file);
			//next 2 byte = qty
			hwrite(&entry->qty,sizeof(entry->qty),1,file);
			//next 2 byte = hours
			hwrite(&entry->hours,sizeof(entry->hours),1,file);
			//next 1 byte = announce (1:0)
			hwrite(&announce,sizeof(announce),1,file);
			//next 1 byte = named (1:0)
			hwrite(&named,sizeof(named),1,file);
			//next 1 byte = ForceSerial (1:0)
			hwrite(&force_serial,sizeof(force_serial),1,file);
		}
		//now we loop into random groups
		for(c = 0; c < random_qty; c++) {
			struct item_package_rand_group *group = &itemdb->packages[i].random_groups[c];
			unsigned short group_qty = group->random_qty, h;

			//next 2 bytes = how many entries in this group
			hwrite(&group_qty,sizeof(group_qty),1,file);
			//now we loop into the group's list
			for(h = 0; h < group_qty; h++) {
				struct item_package_rand_entry *entry = &itemdb->packages[i].random_groups[c].random_list[h];
				unsigned char announce = entry->announce == 1 ? 1 : 0, named = entry->named == 1 ? 1 : 0, force_serial = entry->force_serial == 1 ? 1 : 0;
				//first 2 byte = item id
				hwrite(&entry->id,sizeof(entry->id),1,file);
				//next 2 byte = qty
				hwrite(&entry->qty,sizeof(entry->qty),1,file);
				//next 2 byte = rate
				hwrite(&entry->rate,sizeof(entry->rate),1,file);
				//next 2 byte = hours
				hwrite(&entry->hours,sizeof(entry->hours),1,file);
				//next 1 byte = announce (1:0)
				hwrite(&announce,sizeof(announce),1,file);
				//next 1 byte = named (1:0)
				hwrite(&named,sizeof(named),1,file);
				//next 1 byte = ForceSerial (1:0)
				hwrite(&force_serial,sizeof(force_serial),1,file);
			}
		}
	}
	fclose(file);

	return;
}

static bool itemdb_read_cached_packages(const char *config_filename)
{
	FILE *file;
	unsigned short pcount = 0;
	unsigned short i;

	nullpo_retr(false, config_filename);
	if( !(file = HCache->open(config_filename,"rb")) ) {
		return false;
	}

	// first 2 bytes = package count
	hread(&pcount,sizeof(pcount),1,file);

	CREATE(itemdb->packages, struct item_package, pcount);
	itemdb->package_count = pcount;

	for( i = 0; i < pcount; i++ ) {
		int id = 0;
		unsigned short random_qty = 0, must_qty = 0;
		struct item_data *pdata;
		struct item_package *package = &itemdb->packages[i];
		unsigned short c;

		//into a package, first 4 bytes = id.
		hread(&id,sizeof(id),1,file);
		//next 2 bytes = must count
		hread(&must_qty,sizeof(must_qty),1,file);
		//next 2 bytes = random count
		hread(&random_qty,sizeof(random_qty),1,file);

		if( !(pdata = itemdb->exists(id)) )
			ShowWarning("itemdb_read_cached_packages: unknown package item '%d', skipping..\n",id);
		else
			pdata->package = &itemdb->packages[i];

		package->id = id;
		package->random_qty = random_qty;
		package->must_qty = must_qty;
		package->must_items = NULL;
		package->random_groups = NULL;

		if( package->must_qty ) {
			CREATE(package->must_items, struct item_package_must_entry, package->must_qty);
			//now we loop into must
			for(c = 0; c < package->must_qty; c++) {
				struct item_package_must_entry *entry = &itemdb->packages[i].must_items[c];
				int mid = 0;
				unsigned short qty = 0, hours = 0;
				unsigned char announce = 0, named = 0, force_serial = 0;
				struct item_data *data;
				//first 4 byte = item id
				hread(&mid,sizeof(mid),1,file);
				//next 2 byte = qty
				hread(&qty,sizeof(qty),1,file);
				//next 2 byte = hours
				hread(&hours,sizeof(hours),1,file);
				//next 1 byte = announce (1:0)
				hread(&announce,sizeof(announce),1,file);
				//next 1 byte = named (1:0)
				hread(&named,sizeof(named),1,file);
				//next 1 byte = ForceSerial (1:0)
				hread(&force_serial,sizeof(force_serial),1,file);

				if( !(data = itemdb->exists(mid)) )
					ShowWarning("itemdb_read_cached_packages: unknown item '%d' in package '%s'!\n",mid,itemdb_name(package->id));

				entry->id = data ? data->nameid : 0;
				entry->hours = hours;
				entry->qty = qty;
				entry->announce = announce ? 1 : 0;
				entry->named = named ? 1 : 0;
				entry->force_serial = force_serial ? 1 : 0;
			}
		}
		if( package->random_qty ) {
			//now we loop into random groups
			CREATE(package->random_groups, struct item_package_rand_group, package->random_qty);
			for(c = 0; c < package->random_qty; c++) {
				unsigned short group_qty = 0, h;
				struct item_package_rand_entry *prev = NULL;

				//next 2 bytes = how many entries in this group
				hread(&group_qty,sizeof(group_qty),1,file);

				package->random_groups[c].random_qty = group_qty;
				CREATE(package->random_groups[c].random_list, struct item_package_rand_entry, package->random_groups[c].random_qty);

				//now we loop into the group's list
				for(h = 0; h < group_qty; h++) {
					struct item_package_rand_entry *entry = &itemdb->packages[i].random_groups[c].random_list[h];
					int mid = 0;
					unsigned short qty = 0, hours = 0, rate = 0;
					unsigned char announce = 0, named = 0, force_serial = 0;
					struct item_data *data;

					if( prev ) prev->next = entry;

					//first 2 byte = item id
					hread(&mid,sizeof(mid),1,file);
					//next 2 byte = qty
					hread(&qty,sizeof(qty),1,file);
					//next 2 byte = rate
					hread(&rate,sizeof(rate),1,file);
					//next 2 byte = hours
					hread(&hours,sizeof(hours),1,file);
					//next 1 byte = announce (1:0)
					hread(&announce,sizeof(announce),1,file);
					//next 1 byte = named (1:0)
					hread(&named,sizeof(named),1,file);
					//next 1 byte = ForceSerial (1:0)
					hread(&force_serial,sizeof(force_serial),1,file);

					if( !(data = itemdb->exists(mid)) )
						ShowWarning("itemdb_read_cached_packages: unknown item '%d' in package '%s'!\n",mid,itemdb_name(package->id));

					entry->id = data ? data->nameid : 0;
					entry->rate = rate;
					entry->hours = hours;
					entry->qty = qty;
					entry->announce = announce ? 1 : 0;
					entry->named = named ? 1 : 0;
					entry->force_serial = force_serial ? 1 : 0;
					prev = entry;
				}
				if( prev )
					prev->next = &itemdb->packages[i].random_groups[c].random_list[0];
			}
		}
	}
	fclose(file);
	ShowStatus("Done reading '"CL_WHITE"%hu"CL_RESET"' entries in '"CL_WHITE"%s"CL_RESET"' ("CL_GREEN"C"CL_RESET").\n", pcount, config_filename);

	return true;
}
static void itemdb_read_packages(void)
{
	struct config_t item_packages_conf;
	struct config_setting_t *itg = NULL, *it = NULL, *t = NULL;
	char config_filename[256];
	libconfig->format_db_path(DBPATH"item_packages.conf", config_filename, sizeof(config_filename));
	const char *itname;
	int i = 0, count = 0, c = 0, highest_gcount = 0;
	unsigned int *must = NULL, *random = NULL, *rgroup = NULL, **rgroups = NULL;
	struct item_package_rand_entry **prev = NULL;

	if( HCache->check(config_filename) ) {
		if( itemdb->read_cached_packages(config_filename) )
			return;
	}

	if (!libconfig->load_file(&item_packages_conf, config_filename))
		return;

	must = aMalloc( libconfig->setting_length(item_packages_conf.root) * sizeof(unsigned int) );
	random = aMalloc( libconfig->setting_length(item_packages_conf.root) * sizeof(unsigned int) );
	rgroup = aMalloc( libconfig->setting_length(item_packages_conf.root) * sizeof(unsigned int) );
	rgroups = aMalloc( libconfig->setting_length(item_packages_conf.root) * sizeof(unsigned int *) );

	for(i = 0; i < libconfig->setting_length(item_packages_conf.root); i++) {
		must[i] = 0;
		random[i] = 0;
		rgroup[i] = 0;
		rgroups[i] = NULL;
	}

	/* validate tree, drop poisonous fruits! */
	i = 0;
	while( (itg = libconfig->setting_get_elem(item_packages_conf.root,i++)) ) {
		const char *name = config_setting_name(itg);

		if( !itemdb->name2id(name) ) {
			ShowWarning("itemdb_read_packages: unknown package item '%s', skipping..\n",name);
			libconfig->setting_remove(item_packages_conf.root, name);
			--i;
			continue;
		}

		c = 0;
		while( (it = libconfig->setting_get_elem(itg,c++)) ) {
			int rval = 0;
			if( !( t = libconfig->setting_get_member(it, "Random") ) || (rval = libconfig->setting_get_int(t)) < 0 ) {
				ShowWarning("itemdb_read_packages: invalid 'Random' value (%d) for item '%s' in package '%s', defaulting to must!\n",rval,config_setting_name(it),name);
				libconfig->setting_remove(it, config_setting_name(it));
				--c;
				continue;
			}

			if( rval == 0 )
				must[ i - 1 ] += 1;
			else {
				random[ i - 1 ] += 1;
				if( rval > rgroup[i - 1] )
					rgroup[i - 1] = rval;
				if( rval > highest_gcount )
					highest_gcount = rval;
			}
		}
	}

	CREATE(prev, struct item_package_rand_entry *, highest_gcount);
	for(i = 0; i < highest_gcount; i++) {
		prev[i] = NULL;
	}

	for(i = 0; i < libconfig->setting_length(item_packages_conf.root); i++ ) {
		rgroups[i] = aMalloc( rgroup[i] * sizeof(unsigned int) );
		for( c = 0; c < rgroup[i]; c++ ) {
			rgroups[i][c] = 0;
		}
	}

	/* grab the known sizes */
	i = 0;
	while( (itg = libconfig->setting_get_elem(item_packages_conf.root,i++)) ) {
	   c = 0;
	   while( (it = libconfig->setting_get_elem(itg,c++)) ) {
		   int rval = 0;
		   if ((t = libconfig->setting_get_member(it, "Random")) != NULL && (rval = libconfig->setting_get_int(t)) > 0) {
			   rgroups[i - 1][rval - 1] += 1;
		   }
		}
	}

	CREATE(itemdb->packages, struct item_package, libconfig->setting_length(item_packages_conf.root));
	itemdb->package_count = (unsigned short)libconfig->setting_length(item_packages_conf.root);

	/* write */
	i = 0;
	while( (itg = libconfig->setting_get_elem(item_packages_conf.root,i++)) ) {
		struct item_data *data = itemdb->name2id(config_setting_name(itg));
		int r = 0, m = 0;

		for(r = 0; r < highest_gcount; r++) {
			prev[r] = NULL;
		}

		data->package = &itemdb->packages[count];

		itemdb->packages[count].id  = data->nameid;
		itemdb->packages[count].random_groups = NULL;
		itemdb->packages[count].must_items = NULL;
		itemdb->packages[count].random_qty = rgroup[ i - 1 ];
		itemdb->packages[count].must_qty = must[ i - 1 ];

		if( itemdb->packages[count].random_qty ) {
			CREATE(itemdb->packages[count].random_groups, struct item_package_rand_group, itemdb->packages[count].random_qty);
			for( c = 0; c < itemdb->packages[count].random_qty; c++ ) {
				if( !rgroups[ i - 1 ][c] )
					ShowError("itemdb_read_packages: package '%s' missing 'Random' field %d! there must not be gaps!\n",config_setting_name(itg),c+1);
				else
					CREATE(itemdb->packages[count].random_groups[c].random_list, struct item_package_rand_entry, rgroups[ i - 1 ][c]);
				itemdb->packages[count].random_groups[c].random_qty = 0;
			}
		}
		if( itemdb->packages[count].must_qty )
			CREATE(itemdb->packages[count].must_items, struct item_package_must_entry, itemdb->packages[count].must_qty);

		c = 0;
		while( (it = libconfig->setting_get_elem(itg,c++)) ) {
			int icount = 1, expire = 0, rate = 10000, gid = 0;
			bool announce = false, named = false, force_serial = false;

			itname = config_setting_name(it);

			if (itname[0] == 'I' && itname[1] == 'D' && strlen(itname) <= 12) {
				if( !( data = itemdb->exists(atoi(itname+2)) ) )
					ShowWarning("itemdb_read_packages: unknown item ID '%d' in package '%s'!\n",atoi(itname+2),config_setting_name(itg));
			} else if( !( data = itemdb->name2id(itname) ) )
				ShowWarning("itemdb_read_packages: unknown item '%s' in package '%s'!\n",itname,config_setting_name(itg));

			if( ( t = libconfig->setting_get_member(it, "Count")) )
				icount = libconfig->setting_get_int(t);

			if( ( t = libconfig->setting_get_member(it, "Expire")) )
				expire = libconfig->setting_get_int(t);

			if( ( t = libconfig->setting_get_member(it, "Rate")) ) {
				if( (rate = (unsigned short)libconfig->setting_get_int(t)) > 10000 ) {
					ShowWarning("itemdb_read_packages: invalid rate (%d) for item '%s' in package '%s'!\n",rate,itname,config_setting_name(itg));
					rate = 10000;
				}
			}

			if( ( t = libconfig->setting_get_member(it, "Announce")) && libconfig->setting_get_bool(t) )
				announce = true;

			if( ( t = libconfig->setting_get_member(it, "Named")) && libconfig->setting_get_bool(t) )
				named = true;

			if( ( t = libconfig->setting_get_member(it, "ForceSerial")) && libconfig->setting_get_bool(t) )
				force_serial = true;

			if( !( t = libconfig->setting_get_member(it, "Random") ) ) {
				ShowWarning("itemdb_read_packages: missing 'Random' field for item '%s' in package '%s', defaulting to must!\n",itname,config_setting_name(itg));
				gid = 0;
			} else
				gid = libconfig->setting_get_int(t);

			if( gid == 0 ) {
				itemdb->packages[count].must_items[m].id = data ? data->nameid : 0;
				itemdb->packages[count].must_items[m].qty = icount;
				itemdb->packages[count].must_items[m].hours = expire;
				itemdb->packages[count].must_items[m].announce = announce == true ? 1 : 0;
				itemdb->packages[count].must_items[m].named = named == true ? 1 : 0;
				itemdb->packages[count].must_items[m].force_serial = force_serial == true ? 1 : 0;
				m++;
			} else {
				int gidx = gid - 1;

				r = itemdb->packages[count].random_groups[gidx].random_qty;

				if( prev[gidx] )
					prev[gidx]->next = &itemdb->packages[count].random_groups[gidx].random_list[r];

				itemdb->packages[count].random_groups[gidx].random_list[r].id = data ? data->nameid : 0;
				itemdb->packages[count].random_groups[gidx].random_list[r].qty = icount;
				if( (itemdb->packages[count].random_groups[gidx].random_list[r].rate = rate) == 10000 ) {
					ShowWarning("itemdb_read_packages: item '%s' in '%s' has 100%% drop rate!! set this item as 'Random: 0' or other items won't drop!!!\n",itname,config_setting_name(itg));
				}
				itemdb->packages[count].random_groups[gidx].random_list[r].hours = expire;
				itemdb->packages[count].random_groups[gidx].random_list[r].announce = announce == true ? 1 : 0;
				itemdb->packages[count].random_groups[gidx].random_list[r].named = named == true ? 1 : 0;
				itemdb->packages[count].random_groups[gidx].random_list[r].force_serial = force_serial == true ? 1 : 0;
				itemdb->packages[count].random_groups[gidx].random_qty += 1;

				prev[gidx] = &itemdb->packages[count].random_groups[gidx].random_list[r];
			}
		}

		for(r = 0; r < highest_gcount; r++) {
			if( prev[r] )
				prev[r]->next = &itemdb->packages[count].random_groups[r].random_list[0];
		}

		for( r = 0; r < itemdb->packages[count].random_qty; r++  ) {
			if( itemdb->packages[count].random_groups[r].random_qty == 1 ) {
				//item packages don't stop looping until something comes out of them, so if you have only one item in it the drop is guaranteed.
				ShowWarning("itemdb_read_packages: in '%s' 'Random: %d' group has only 1 random option, drop rate will be 100%%!\n",
				            itemdb_name(itemdb->packages[count].id),r+1);
				itemdb->packages[count].random_groups[r].random_list[0].rate = 10000;
			}
		}
		count++;
	}

	aFree(must);
	aFree(random);
	for(i = 0; i < libconfig->setting_length(item_packages_conf.root); i++ ) {
		aFree(rgroups[i]);
	}
	aFree(rgroups);
	aFree(rgroup);
	aFree(prev);

	libconfig->destroy(&item_packages_conf);

	if( HCache->enabled )
		itemdb->write_cached_packages(config_filename);

	ShowStatus("Done reading '"CL_WHITE"%d"CL_RESET"' entries in '"CL_WHITE"%s"CL_RESET"'.\n", count, config_filename);
}

/**
 * Processes any (plugin-defined) additional fields for a itemdb_options entry.
 *
 * @param[in,out] entry  The destination ito entry, already initialized
 *                       (item_opt.index, status.mode are expected to be already set).
 * @param[in]     t      The libconfig entry.
 * @param[in]     source Source of the entry (file name), to be displayed in
 *                       case of validation errors.
 */
static void itemdb_readdb_options_additional_fields(struct itemdb_option *ito, struct config_setting_t *t, const char *source)
{
	// do nothing. plugins can do their own work
}

/**
 * Reads the Item Options configuration file.
 */
static void itemdb_read_options(void)
{
	struct config_t item_options_db;
	struct config_setting_t *ito = NULL, *conf = NULL;
	int index = 0, count = 0;
	char filepath[256];
	libconfig->format_db_path("item_options.conf", filepath, sizeof(filepath));
	VECTOR_DECL(int) duplicate_id;

	if (!libconfig->load_file(&item_options_db, filepath))
		return;

#ifdef ENABLE_CASE_CHECK
	script->parser_current_file = filepath;
#endif // ENABLE_CASE_CHECK

	if ((ito=libconfig->setting_get_member(item_options_db.root, "item_options_db")) == NULL) {
		ShowError("itemdb_read_options: '%s' could not be loaded.\n", filepath);
		libconfig->destroy(&item_options_db);
		return;
	}

	VECTOR_INIT(duplicate_id);

	VECTOR_ENSURE(duplicate_id, libconfig->setting_length(ito), 1);

	while ((conf = libconfig->setting_get_elem(ito, index++))) {
		struct itemdb_option t_opt = { 0 }, *s_opt = NULL;
		const char *str = NULL;
		int i = 0;

		/* Id Lookup */
		if (!libconfig->setting_lookup_int16(conf, "Id", &t_opt.index) || t_opt.index <= 0) {
			ShowError("itemdb_read_options: Invalid Option Id provided for entry %d in '%s', skipping...\n", t_opt.index, filepath);
			continue;
		}

		/* Checking for duplicate entries. */
		ARR_FIND(0, VECTOR_LENGTH(duplicate_id), i, VECTOR_INDEX(duplicate_id, i) == t_opt.index);

		if (i != VECTOR_LENGTH(duplicate_id)) {
			ShowError("itemdb_read_options: Duplicate entry for Option Id %d in '%s', skipping...\n", t_opt.index, filepath);
			continue;
		}

		VECTOR_PUSH(duplicate_id, t_opt.index);

		/* Name Lookup */
		if (!libconfig->setting_lookup_string(conf, "Name", &str)) {
			ShowError("itemdb_read_options: Invalid Option Name '%s' provided for Id %d in '%s', skipping...\n", str, t_opt.index, filepath);
			continue;
		}

		/* check for illegal characters in the constant. */
		{
			const char *c = str;

			while (ISALNUM(*c) || *c == '_')
				++c;

			if (*c != '\0') {
				ShowError("itemdb_read_options: Invalid characters in Option Name '%s' for Id %d in '%s', skipping...\n", str, t_opt.index, filepath);
				continue;
			}
		}

		/* Set name as a script constant with index as value. */
		script->set_constant2(str, t_opt.index, false, false);

		/* Script Code Lookup */
		if (!libconfig->setting_lookup_string(conf, "Script", &str)) {
			ShowError("itemdb_read_options: Script code not found for entry %s (Id: %d) in '%s', skipping...\n", str, t_opt.index, filepath);
			continue;
		}

		/* Set Script */
		t_opt.script = *str ? script->parse(str, filepath, t_opt.index, SCRIPT_IGNORE_EXTERNAL_BRACKETS, NULL) : NULL;

		/* Additional fields through plugins */
		itemdb->readdb_options_additional_fields(&t_opt, ito, filepath);

		/* Allocate memory and copy contents */
		CREATE(s_opt, struct itemdb_option, 1);

		*s_opt = t_opt;

		/* Store ptr in the database */
		idb_put(itemdb->options, t_opt.index, s_opt);

		count++;
	}

#ifdef ENABLE_CASE_CHECK
	script->parser_current_file = NULL;
#endif // ENABLE_CASE_CHECK

	libconfig->destroy(&item_options_db);

	VECTOR_CLEAR(duplicate_id);

	ShowStatus("Done reading '"CL_WHITE"%d"CL_RESET"' entries in '"CL_WHITE"%s"CL_RESET"'.\n", count, filepath);
}

static void itemdb_read_chains(void)
{
	struct config_t item_chain_conf;
	struct config_setting_t *itc = NULL;
	char config_filename[256];
	libconfig->format_db_path(DBPATH"item_chain.conf", config_filename, sizeof(config_filename));
	int i = 0, count = 0;

	if (!libconfig->load_file(&item_chain_conf, config_filename))
		return;

	CREATE(itemdb->chains, struct item_chain, libconfig->setting_length(item_chain_conf.root));
	itemdb->chain_count = (unsigned short)libconfig->setting_length(item_chain_conf.root);

#ifdef ENABLE_CASE_CHECK
	script->parser_current_file = config_filename;
#endif // ENABLE_CASE_CHECK
	while( (itc = libconfig->setting_get_elem(item_chain_conf.root,i++)) ) {
		struct item_data *data = NULL;
		struct item_chain_entry *prev = NULL;
		const char *name = config_setting_name(itc);
		int c = 0;
		struct config_setting_t *entry = NULL;

		script->set_constant2(name, i-1, false, false);
		itemdb->chains[count].qty = (unsigned short)libconfig->setting_length(itc);

		CREATE(itemdb->chains[count].items, struct item_chain_entry, libconfig->setting_length(itc));

		while( (entry = libconfig->setting_get_elem(itc,c++)) ) {
			const char *itname = config_setting_name(entry);
			if (itname[0] == 'I' && itname[1] == 'D' && strlen(itname) <= 12) {
				if( !( data = itemdb->exists(atoi(itname+2)) ) )
					ShowWarning("itemdb_read_chains: unknown item ID '%d' in chain '%s'!\n",atoi(itname+2),name);
			} else if( !( data = itemdb->name2id(itname) ) )
				ShowWarning("itemdb_read_chains: unknown item '%s' in chain '%s'!\n",itname,name);

			struct item_chain_entry *item = &itemdb->chains[count].items[c - 1];

			if( prev )
				prev->next = item;

			item->id = data ? data->nameid : 0;

			int rate = data ? libconfig->setting_get_int(entry) : 0;
			if (battle_config.item_rate_add_chain != 100)
				rate = rate * battle_config.item_rate_add_chain / 100;

			item->rate = cap_value(rate, battle_config.item_drop_add_chain_min, battle_config.item_drop_add_chain_max);

			prev = item;
		}

		if( prev )
			prev->next = &itemdb->chains[count].items[0];

		count++;
	}
#ifdef ENABLE_CASE_CHECK
	script->parser_current_file = NULL;
#endif // ENABLE_CASE_CHECK

	libconfig->destroy(&item_chain_conf);

	if( !script->get_constant("ITMCHAIN_ORE",&i) )
		ShowWarning("itemdb_read_chains: failed to find 'ITMCHAIN_ORE' chain to link to cache!\n");
	else
		itemdb->chain_cache[ECC_ORE] = i;

	if (!script->get_constant("ITMCHAIN_SIEGFRIED", &i))
		ShowWarning("itemdb_read_chains: failed to find 'ITMCHAIN_SIEGFRIED' chain to link to cache!\n");
	else
		itemdb->chain_cache[ECC_SIEGFRIED] = i;

	if (!script->get_constant("ITMCHAIN_NEO_INSURANCE", &i))
		ShowWarning("itemdb_read_chains: failed to find 'ITMCHAIN_NEO_INSURANCE' chain to link to cache!\n");
	else
		itemdb->chain_cache[ECC_NEO_INSURANCE] = i;

	ShowStatus("Done reading '"CL_WHITE"%d"CL_RESET"' entries in '"CL_WHITE"%s"CL_RESET"'.\n", count, config_filename);
}

static bool itemdb_read_combodb_libconfig(void)
{
	struct config_t combo_conf;
	char filepath[290];
	snprintf(filepath, sizeof(filepath), "%s/%s", map->db_path, DBPATH"item_combo_db.conf");

	if (libconfig->load_file(&combo_conf, filepath) == CONFIG_FALSE) {
		ShowError("itemdb_read_combodb_libconfig: can't read %s\n", filepath);
		return false;
	}

	struct config_setting_t *combo_db = NULL;
	if ((combo_db = libconfig->setting_get_member(combo_conf.root, "combo_db")) == NULL) {
		ShowError("itemdb_read_combodb_libconfig: can't read %s\n", filepath);
		return false;
	}

	int i = 0;
	int count = 0;
	struct config_setting_t *it = NULL;

	while ((it = libconfig->setting_get_elem(combo_db, i++)) != NULL) {
		if (itemdb->read_combodb_libconfig_sub(it, i - 1, filepath))
			++count;
	}

	libconfig->destroy(&combo_conf);
	ShowStatus("Done reading '"CL_WHITE"%d"CL_RESET"' entries in '"CL_WHITE"%s"CL_RESET"'.\n", count, filepath);
	return true;
}

static bool itemdb_read_combodb_libconfig_sub(struct config_setting_t *it, int idx, const char *source)
{
	nullpo_retr(false, it);
	nullpo_retr(false, source);

	struct config_setting_t *t = NULL;

	if ((t = libconfig->setting_get_member(it, "Items")) == NULL) {
		ShowWarning("itemdb_read_combodb_libconfig_sub: invalid item list for combo (%d), in (%s), skipping..\n", idx, source);
		return false;
	}

	if (!config_setting_is_array(t)) {
		ShowWarning("itemdb_read_combodb_libconfig_sub: the combo (%d) item list must be an array, in (%s), skipping..\n", idx, source);
		return false;
	}

	int len = libconfig->setting_length(t);
	if (len > MAX_ITEMS_PER_COMBO) {
		ShowWarning("itemdb_read_combodb_libconfig_sub: the size of combo (%d) item list is too big (%d, max = %d), in (%s), skipping..\n", idx, len, MAX_ITEMS_PER_COMBO, source);
		return false;
	}

	struct item_combo *combo = NULL;
	RECREATE(itemdb->combos, struct item_combo *, ++itemdb->combo_count);
	CREATE(combo, struct item_combo, 1);

	combo->id = itemdb->combo_count - 1;
	combo->count = len;

	for (int i = 0; i < len; i++) {
		struct item_data *item = NULL;
		const char *name = libconfig->setting_get_string_elem(t, i);

		if ((item = itemdb->name2id(name)) == NULL) {
			ShowWarning("itemdb_read_combodb_libconfig_sub: unknown item '%s', in (%s), skipping..\n", name, source);
			--itemdb->combo_count;
			aFree(combo);
			return false;
		}
		combo->nameid[i] = item->nameid;
	}

	const char *str = NULL;
	if (libconfig->setting_lookup_string(it, "Script", &str) == CONFIG_TRUE) {
		combo->script = *str ? script->parse(str, source, -idx, SCRIPT_IGNORE_EXTERNAL_BRACKETS, NULL) : NULL;
	} else {
		ShowWarning("itemdb_read_combodb_libconfig_sub: invalid script for combo (%d) in (%s), skipping..\n", idx, source);
		--itemdb->combo_count;
		aFree(combo);
		return false;
	}

	itemdb->combos[combo->id] = combo;

	/* populate the items to refer to this combo */
	for (int i = 0; i < len; i++) {
		struct item_data *item = itemdb->exists(combo->nameid[i]);
		RECREATE(item->combos, struct item_combo *, ++item->combos_count);
		item->combos[item->combos_count - 1] = combo;
	}
	return true;
}

/*======================================
 * Applies gender restrictions according to settings. [Skotlex]
 *======================================*/
static int itemdb_gendercheck(struct item_data *id)
{
	nullpo_ret(id);
	if (id->nameid == WEDDING_RING_M) //Grom Ring
		return 1;
	if (id->nameid == WEDDING_RING_F) //Bride Ring
		return 0;
	if (id->subtype == W_MUSICAL && id->type == IT_WEAPON) //Musical instruments are always male-only
		return 1;
	if (id->subtype == W_WHIP && id->type == IT_WEAPON) //Whips are always female-only
		return 0;

	return (battle_config.ignore_items_gender) ? 2 : id->sex;
}

/**
 * Validates an item DB entry and inserts it into the database.
 * This function is called after preparing the item entry data, and it takes
 * care of inserting it and cleaning up any remainders of the previous one.
 *
 * @param entry  Pointer to the new item_data entry. Ownership is NOT taken,
 *               but the content is modified to reflect the validation.
 * @param n      Ordinal number of the entry, to be displayed in case of
 *               validation errors.
 * @param source Source of the entry (file name), to be displayed in case of
 *               validation errors.
 * @return Nameid of the validated entry, or 0 in case of failure.
 *
 * Note: This is safe to call if the new entry is a shallow copy of the old one
 * (i.e.  item_db2 inheritance), as it will make sure not to free any scripts
 * still in use by the new entry.
 */
static int itemdb_validate_entry(struct item_data *entry, int n, const char *source)
{
	struct item_data *item;

	nullpo_ret(entry);
	nullpo_ret(source);
#if PACKETVER_MAIN_NUM >= 20181121 || PACKETVER_RE_NUM >= 20180704 || PACKETVER_ZERO_NUM >= 20181114
	if (entry->nameid <= 0 || entry->nameid > MAX_ITEM_ID) {
#else
	if (entry->nameid <= 0) {
#endif
		// item id wrong for any packet versions
		ShowWarning("itemdb_validate_entry: Invalid item ID %d in entry %d of '%s', allowed values 0 < ID < %d (MAX_ITEM_ID), skipping.\n",
				entry->nameid, n, source, MAX_ITEM_ID);
		if (entry->script) {
			script->free_code(entry->script);
			entry->script = NULL;
		}
		if (entry->equip_script) {
			script->free_code(entry->equip_script);
			entry->equip_script = NULL;
		}
		if (entry->unequip_script) {
			script->free_code(entry->unequip_script);
			entry->unequip_script = NULL;
		}
		if (entry->rental_start_script != NULL) {
			script->free_code(entry->rental_start_script);
			entry->rental_start_script = NULL;
		}
		if (entry->rental_end_script != NULL) {
			script->free_code(entry->rental_end_script);
			entry->rental_end_script = NULL;
		}
		return 0;
#if PACKETVER_MAIN_NUM >= 20181121 || PACKETVER_RE_NUM >= 20180704 || PACKETVER_ZERO_NUM >= 20181114
	}
#else
	} else if (entry->nameid > MAX_ITEM_ID) {
		// item id too big for packet version before item id in 4 bytes
		entry->view_id = UNKNOWN_ITEM_ID;
	}
#endif

	{
		const char *c = entry->name;
		while (ISALNUM(*c) || *c == '_')
			++c;

		if (*c != '\0') {
			ShowWarning("itemdb_validate_entry: Invalid characters in the AegisName '%s' for item %d in '%s'. Skipping.\n",
					entry->name, entry->nameid, source);
			if (entry->script) {
				script->free_code(entry->script);
				entry->script = NULL;
			}
			if (entry->equip_script) {
				script->free_code(entry->equip_script);
				entry->equip_script = NULL;
			}
			if (entry->unequip_script) {
				script->free_code(entry->unequip_script);
				entry->unequip_script = NULL;
			}
			if (entry->rental_start_script != NULL) {
				script->free_code(entry->rental_start_script);
				entry->rental_start_script = NULL;
			}
			if (entry->rental_end_script != NULL) {
				script->free_code(entry->rental_end_script);
				entry->rental_end_script = NULL;
			}
			return 0;
		}
	}

	if( entry->type < 0 || entry->type == IT_UNKNOWN || entry->type == IT_UNKNOWN2
	 || (entry->type > IT_SELECTPACKAGE && entry->type < IT_CASH ) || entry->type >= IT_MAX
	) {
		// catch invalid item types
		ShowWarning("itemdb_validate_entry: Invalid item type %d for item %d in '%s'. IT_ETC will be used.\n",
		            entry->type, entry->nameid, source);
		entry->type = IT_ETC;
	} else if( entry->type == IT_DELAYCONSUME ) {
		//Items that are consumed only after target confirmation
		entry->type = IT_USABLE;
		entry->flag.delay_consume = 1;
	} else if (entry->type == IT_SELECTPACKAGE) {
		entry->type = IT_USABLE;
		entry->flag.select_package = 1;
	}

	//When a particular price is not given, we should base it off the other one
	//(it is important to make a distinction between 'no price' and 0z)
	if( entry->value_buy < 0 && entry->value_sell < 0 ) {
		entry->value_buy = entry->value_sell = 0;
	} else if( entry->value_buy < 0 ) {
		entry->value_buy = entry->value_sell * 2;
	} else if( entry->value_sell < 0 ) {
		entry->value_sell = entry->value_buy / 2;
	}
	if( entry->value_buy/124. < entry->value_sell/75. ) {
		ShowWarning("itemdb_validate_entry: Buying/Selling [%d/%d] price of item %d (%s) in '%s' "
		            "allows Zeny making exploit through buying/selling at discounted/overcharged prices!\n",
		            entry->value_buy, entry->value_sell, entry->nameid, entry->jname, source);
	}

	if( entry->slot > MAX_SLOTS ) {
		ShowWarning("itemdb_validate_entry: Item %d (%s) in '%s' specifies %d slots, but the server only supports up to %d. Using %d slots.\n",
		            entry->nameid, entry->jname, source, entry->slot, MAX_SLOTS, MAX_SLOTS);
		entry->slot = MAX_SLOTS;
	}

	if (!entry->equip && itemdb->isequip2(entry)) {
		ShowWarning("itemdb_validate_entry: Item %d (%s) in '%s' is an equipment with no equip-field! Making it an etc item.\n",
		            entry->nameid, entry->jname, source);
		entry->type = IT_ETC;
	}

	if (entry->flag.trade_restriction > ITR_ALL) {
		ShowWarning("itemdb_validate_entry: Invalid trade restriction flag 0x%x for item %d (%s) in '%s', defaulting to none.\n",
		            (unsigned int)entry->flag.trade_restriction, entry->nameid, entry->jname, source);
		entry->flag.trade_restriction = ITR_NONE;
	}

	if (entry->gm_lv_trade_override < 0 || entry->gm_lv_trade_override > 100) {
		ShowWarning("itemdb_validate_entry: Invalid trade-override GM level %d for item %d (%s) in '%s', defaulting to none.\n",
		            entry->gm_lv_trade_override, entry->nameid, entry->jname, source);
		entry->gm_lv_trade_override = 0;
	}
	if (entry->gm_lv_trade_override == 0) {
		// Default value if none or an ivalid one was specified
		entry->gm_lv_trade_override = 100;
	}

	if (entry->item_usage.flag > INR_ALL) {
		ShowWarning("itemdb_validate_entry: Invalid nouse flag 0x%x for item %d (%s) in '%s', defaulting to none.\n",
		            entry->item_usage.flag, entry->nameid, entry->jname, source);
		entry->item_usage.flag = INR_NONE;
	}

	if (entry->item_usage.override > 100) {
		ShowWarning("itemdb_validate_entry: Invalid nouse-override GM level %d for item %d (%s) in '%s', defaulting to none.\n",
		            entry->item_usage.override, entry->nameid, entry->jname, source);
		entry->item_usage.override = 0;
	}
	if (entry->item_usage.override == 0) {
		// Default value if none or an ivalid one was specified
		entry->item_usage.override = 100;
	}

	if (entry->stack.amount > 0 && !itemdb->isstackable2(entry)) {
		ShowWarning("itemdb_validate_entry: Item %d (%s) of type %d is not stackable, ignoring stack settings in '%s'.\n",
		            entry->nameid, entry->jname, entry->type, source);
		memset(&entry->stack, '\0', sizeof(entry->stack));
	}

	if (entry->type == IT_WEAPON && (entry->subtype <= 0 || entry->subtype >= MAX_SINGLE_WEAPON_TYPE)) {
		ShowWarning("itemdb_validate_entry: Invalid View for weapon items. View value %d for item %d (%s) in '%s', defaulting to W_DAGGER.\n",
		            entry->subtype, entry->nameid, entry->jname, source);
		entry->subtype = W_DAGGER;
	} else if (entry->type == IT_AMMO && (entry->subtype <= 0 || entry->subtype >= MAX_AMMO_TYPE)) {
		ShowWarning("itemdb_validate_entry: Invalid View for ammunition items. View value %d for item %d (%s) in '%s', defaulting to A_ARROW.\n",
		            entry->subtype, entry->nameid, entry->jname, source);
		entry->subtype = A_ARROW;
	}

	entry->wlv = cap_value(entry->wlv, REFINE_TYPE_ARMOR, REFINE_TYPE_MAX);

	if( !entry->elvmax )
		entry->elvmax = MAX_LEVEL;
	else if( entry->elvmax < entry->elv )
		entry->elvmax = entry->elv;

	if( entry->type != IT_ARMOR && entry->type != IT_WEAPON && !entry->flag.no_refine )
		entry->flag.no_refine = 1;

	if (entry->type != IT_ARMOR && entry->type != IT_WEAPON && !entry->flag.no_options)
		entry->flag.no_options = 1;

	if (entry->flag.available != 1) {
		entry->flag.available = 1;
		entry->view_id = 0;
	}

	entry->sex = itemdb->gendercheck(entry); //Apply gender filtering.

	// Validated. Finally insert it
	item = itemdb->load(entry->nameid);

	if (item->script && item->script != entry->script) { // Don't free if it's inheriting the same script
		script->free_code(item->script);
		item->script = NULL;
	}
	if (item->equip_script && item->equip_script != entry->equip_script) { // Don't free if it's inheriting the same script
		script->free_code(item->equip_script);
		item->equip_script = NULL;
	}
	if (item->unequip_script && item->unequip_script != entry->unequip_script) { // Don't free if it's inheriting the same script
		script->free_code(item->unequip_script);
		item->unequip_script = NULL;
	}
	if (item->rental_start_script != NULL && item->rental_start_script != entry->rental_start_script) { // Don't free if it's inheriting the same script
		script->free_code(item->rental_start_script);
		item->rental_start_script = NULL;
	}
	if (item->rental_end_script != NULL && item->rental_end_script != entry->rental_end_script) { // Don't free if it's inheriting the same script
		script->free_code(item->rental_end_script);
		item->rental_end_script = NULL;
	}
	*item = *entry;
	return item->nameid;
}

static void itemdb_readdb_additional_fields(int itemid, struct config_setting_t *it, int n, const char *source, struct DBMap *itemconst_db)
{
	// do nothing. plugins can do own work
}

/**
 * Processes job names and changes it into mapid format.
 *
 * @param id item_data entry.
 * @param t  Libconfig setting entry. It is expected to be valid and it won't
 *           be freed (it is care of the caller to do so if necessary).
 */
static void itemdb_readdb_job_sub(struct item_data *id, struct config_setting_t *t)
{
	int idx = 0;
	struct config_setting_t *it = NULL;
	bool enable_all = false;

	id->class_base[0] = id->class_base[1] = id->class_base[2] = 0;

	if (libconfig->setting_lookup_bool_real(t, "All", &enable_all) && enable_all) {
		itemdb->jobmask2mapid(id->class_base, UINT64_MAX);
	}
	while ((it = libconfig->setting_get_elem(t, idx++)) != NULL) {
		const char *job_name = config_setting_name(it);
		int job_id;

		if (strcmp(job_name, "All") == 0)
			continue;

		if ((job_id = pc->check_job_name(job_name)) == -1) {
			ShowWarning("itemdb_readdb_job_sub: unknown job name '%s'!\n", job_name);
		} else {
			itemdb->jobid2mapid(id->class_base, job_id, libconfig->setting_get_bool(it));
		}
	}
}

/**
 * Processes one itemdb entry from the libconfig backend, loading and inserting
 * it into the item database.
 *
 * @param it     Libconfig setting entry. It is expected to be valid and it
 *               won't be freed (it is care of the caller to do so if
 *               necessary)
 * @param n      Ordinal number of the entry, to be displayed in case of
 *               validation errors.
 * @param source Source of the entry (file name), to be displayed in case of
 *               validation errors.
 * @return Nameid of the validated entry, or 0 in case of failure.
 */
static int itemdb_readdb_libconfig_sub(struct config_setting_t *it, int n, const char *source, struct DBMap *itemconst_db)
{
	struct item_data id = { 0 };
	struct config_setting_t *t = NULL;
	const char *str = NULL;
	int i32 = 0;
	bool inherit = false;

	nullpo_ret(it);
	nullpo_ret(itemconst_db);
	/*
	 * // Mandatory fields
	 * Id: ID
	 * AegisName: "Aegis_Name"
	 * Name: "Item Name"
	 * // Optional fields
	 * Type: Item Type
	 * Buy: Buy Price
	 * Sell: Sell Price
	 * Weight: Item Weight
	 * Atk: Attack
	 * Matk: Attack
	 * Def: Defense
	 * Range: Attack Range
	 * Slots: Slots
	 * Job: Job mask
	 * Upper: Upper mask
	 * Gender: Gender
	 * Loc: Equip location
	 * WeaponLv: Weapon Level
	 * EquipLv: Equip required level or [min, max]
	 * Refine: Refineable
	 * BindOnEquip: (true or false)
	 * BuyingStore: (true or false)
	 * Delay: Delay to use item
	 * ForceSerial: (true or false)
	 * IgnoreDiscount: (true or false)
	 * IgnoreOvercharge: (true or false)
	 * Trade: {
	 *   override: Group to override
	 *   nodrop: (true or false)
	 *   notrade: (true or false)
	 *   partneroverride: (true or false)
	 *   noselltonpc: (true or false)
	 *   nocart: (true or false)
	 *   nostorage: (true or false)
	 *   nogstorage: (true or false)
	 *   nomail: (true or false)
	 *   noauction: (true or false)
	 * }
	 * Nouse: {
	 *   override: Group to override
	 *   sitting: (true or false)
	 * }
	 * Stack: [Stackable Amount, Stack Type]
	 * Sprite: SpriteID
	 * Script: <"
	 *     Script
	 *     (it can be multi-line)
	 * ">
	 * OnEquipScript: <" OnEquip Script ">
	 * OnUnequipScript: <" OnUnequip Script ">
	 * OnRentalStartScript: <" on renting script ">
	 * OnRentalEndScript: <" on renting end script ">
	 * Inherit: inherit or override
	 */
	if(!map->setting_lookup_const(it, "Id", &i32)) {
		ShowWarning("itemdb_readdb_libconfig_sub: Invalid or missing id in \"%s\", entry #%d, skipping.\n", source, n);
		return 0;
	}
	id.nameid = i32;

	if( (t = libconfig->setting_get_member(it, "Inherit")) && (inherit = libconfig->setting_get_bool(t)) ) {
		if( !itemdb->exists(id.nameid) ) {
			ShowWarning("itemdb_readdb_libconfig_sub: Trying to inherit nonexistent item %d, default values will be used instead.\n", id.nameid);
			inherit = false;
		} else {
			// Use old entry as default
			struct item_data *old_entry = itemdb->load(id.nameid);
			memcpy(&id, old_entry, sizeof(id));
		}
	}

	bool clone = false;
	if ((t = libconfig->setting_get_member(it, "CloneItem")) != NULL) {
		int clone_id;

		if (t->type == CONFIG_TYPE_STRING) {
			const char *clone_name = libconfig->setting_get_string(t);
			clone_id = strdb_iget(itemconst_db, clone_name);

			if (clone_id == 0) {
				ShowWarning("%s: Could not find item \"%s\" to clone in item %d of \"%s\". Skipping.\n", __func__, clone_name, id.nameid, source);
				return 0;
			}
		} else {
			clone_id = libconfig->setting_get_int(t);
		}

		struct item_data *base_entry = itemdb->exists(clone_id);
		if (base_entry == NULL) {
			ShowWarning("%s: Trying to clone nonexistent item %d in item %d of \"%s\". Skipping.\n", __func__, clone_id, id.nameid, source);
			return 0;
		}

		int new_id = id.nameid;
		char existing_name[ITEM_NAME_LENGTH];
		strncpy(existing_name, id.name, sizeof(existing_name));

		clone = true;
		memcpy(&id, base_entry, sizeof(id));

		// Restore fields that cloning shouldn't replace. ID and AegisName are unique fields, so should not be cloned.
		id.nameid = new_id;
		strncpy(id.name, existing_name, sizeof(id.name));
	}

	if (!libconfig->setting_lookup_string(it, "AegisName", &str) || !*str) {
		if (!inherit) {
			ShowWarning("itemdb_readdb_libconfig_sub: Missing AegisName in item %d of \"%s\", skipping.\n", id.nameid, source);
			return 0;
		}
	} else {
		safestrncpy(id.name, str, sizeof(id.name));
	}

	if( !libconfig->setting_lookup_string(it, "Name", &str) || !*str ) {
		if (!inherit && !clone) {
			ShowWarning("itemdb_readdb_libconfig_sub: Missing Name in item %d of \"%s\", skipping.\n", id.nameid, source);
			return 0;
		}
	} else {
		safestrncpy(id.jname, str, sizeof(id.jname));
	}

	if (map->setting_lookup_const(it, "Type", &i32))
		id.type = i32;
	else if (!inherit && !clone)
		id.type = IT_ETC;

	if (map->setting_lookup_const(it, "Subtype", &i32) && i32 >= 0) {
		if (id.type == IT_WEAPON || id.type == IT_AMMO)
			id.subtype = i32;
		else
			ShowWarning("itemdb_readdb_libconfig_sub: Field 'Subtype' is only allowed for IT_WEAPON or IT_AMMO (Item #%d: %s). Ignoring.\n",
					id.nameid, id.name);
	}

	if (map->setting_lookup_const(it, "Buy", &i32))
		id.value_buy = i32;
	else if (!inherit && !clone)
		id.value_buy = -1;
	if (map->setting_lookup_const(it, "Sell", &i32))
		id.value_sell = i32;
	else if (!inherit && !clone)
		id.value_sell = -1;

	if (map->setting_lookup_const(it, "Weight", &i32) && i32 >= 0)
		id.weight = i32;

	if (map->setting_lookup_const(it, "Atk", &i32) && i32 >= 0)
		id.atk = i32;

	if (map->setting_lookup_const(it, "Matk", &i32) && i32 >= 0)
		id.matk = i32;

	if (map->setting_lookup_const(it, "Def", &i32) && i32 >= 0)
		id.def = i32;

	if (map->setting_lookup_const(it, "Range", &i32) && i32 >= 0)
		id.range = i32;

	if (map->setting_lookup_const(it, "Slots", &i32) && i32 >= 0)
		id.slot = i32;

	if ((t = libconfig->setting_get_member(it, "Job")) != NULL) {
		if (config_setting_is_group(t)) {
			itemdb->readdb_job_sub(&id, t);
		} else if (map->setting_lookup_const(it, "Job", &i32)) { // This is an unsigned value, do not check for >= 0
			itemdb->jobmask2mapid(id.class_base, (uint64)i32);
		} else if (!inherit && !clone) {
			itemdb->jobmask2mapid(id.class_base, UINT64_MAX);
		}
	} else if (!inherit) {
		itemdb->jobmask2mapid(id.class_base, UINT64_MAX);
	}

	if (map->setting_lookup_const_mask(it, "Upper", &i32) && i32 >= 0)
		id.class_upper = (unsigned int)i32;
	else if (!inherit && !clone)
		id.class_upper = ITEMUPPER_ALL;

	if (map->setting_lookup_const(it, "Gender", &i32) && i32 >= 0)
		id.sex = i32;
	else if (!inherit && !clone)
		id.sex = 2;

	if (map->setting_lookup_const_mask(it, "Loc", &i32) && i32 >= 0)
		id.equip = i32;

	if (map->setting_lookup_const(it, "WeaponLv", &i32) && i32 >= 0)
		id.wlv = i32;

	if( (t = libconfig->setting_get_member(it, "EquipLv")) ) {
		if( config_setting_is_aggregate(t) ) {
			if( libconfig->setting_length(t) >= 2 )
				id.elvmax = libconfig->setting_get_int_elem(t, 1);
			if( libconfig->setting_length(t) >= 1 )
				id.elv = libconfig->setting_get_int_elem(t, 0);
		} else {
			id.elv = libconfig->setting_get_int(t);
		}
	}

	if( (t = libconfig->setting_get_member(it, "Refine")) )
		id.flag.no_refine = libconfig->setting_get_bool(t) ? 0 : 1;

	if( (t = libconfig->setting_get_member(it, "Grade")) )
		id.flag.no_grade = libconfig->setting_get_bool(t) ? 1 : 0;

	if ((t = libconfig->setting_get_member(it, "DisableOptions")))
		id.flag.no_options = libconfig->setting_get_bool(t) ? 1 : 0;

	if ((t = libconfig->setting_get_member(it, "ShowDropEffect")))
		id.flag.showdropeffect = libconfig->setting_get_bool(t) ? 1 : 0;

	if (map->setting_lookup_const(it, "DropEffectMode", &i32) && i32 >= 0)
		id.dropeffectmode = i32;

	if (map->setting_lookup_const(it, "ViewSprite", &i32) && i32 >= 0)
		id.view_sprite = i32;

	if( (t = libconfig->setting_get_member(it, "BindOnEquip")) )
		id.flag.bindonequip = libconfig->setting_get_bool(t) ? 1 : 0;

	if( (t = libconfig->setting_get_member(it, "ForceSerial")) )
		id.flag.force_serial = libconfig->setting_get_bool(t) ? 1 : 0;

	if ( (t = libconfig->setting_get_member(it, "BuyingStore")) )
		id.flag.buyingstore = libconfig->setting_get_bool(t) ? 1 : 0;

	if ((t = libconfig->setting_get_member(it, "KeepAfterUse")))
		id.flag.keepafteruse = libconfig->setting_get_bool(t) ? 1 : 0;

	if ((t = libconfig->setting_get_member(it, "DropAnnounce")))
		id.flag.drop_announce = libconfig->setting_get_bool(t) ? 1 : 0;

	if (map->setting_lookup_const(it, "Delay", &i32) && i32 >= 0)
		id.delay = i32;

	if ((t = libconfig->setting_get_member(it, "IgnoreDiscount")))
		id.flag.ignore_discount = libconfig->setting_get_bool(t) ? 1 : 0;

	if ((t = libconfig->setting_get_member(it, "IgnoreOvercharge")))
		id.flag.ignore_overcharge = libconfig->setting_get_bool(t) ? 1 : 0;

	if ( (t = libconfig->setting_get_member(it, "Trade")) ) {
		if (config_setting_is_group(t)) {
			struct config_setting_t *tt = NULL;

			if ((tt = libconfig->setting_get_member(t, "override"))) {
				id.gm_lv_trade_override = libconfig->setting_get_int(tt);
			}

			if ((tt = libconfig->setting_get_member(t, "nodrop"))) {
				id.flag.trade_restriction &= ~ITR_NODROP;
				if (libconfig->setting_get_bool(tt))
					id.flag.trade_restriction |= ITR_NODROP;
			}

			if ((tt = libconfig->setting_get_member(t, "notrade"))) {
				id.flag.trade_restriction &= ~ITR_NOTRADE;
				if (libconfig->setting_get_bool(tt))
					id.flag.trade_restriction |= ITR_NOTRADE;
			}

			if ((tt = libconfig->setting_get_member(t, "partneroverride"))) {
				id.flag.trade_restriction &= ~ITR_PARTNEROVERRIDE;
				if (libconfig->setting_get_bool(tt))
					id.flag.trade_restriction |= ITR_PARTNEROVERRIDE;
			}

			if ((tt = libconfig->setting_get_member(t, "noselltonpc"))) {
				id.flag.trade_restriction &= ~ITR_NOSELLTONPC;
				if (libconfig->setting_get_bool(tt))
					id.flag.trade_restriction |= ITR_NOSELLTONPC;
			}

			if ((tt = libconfig->setting_get_member(t, "nocart"))) {
				id.flag.trade_restriction &= ~ITR_NOCART;
				if (libconfig->setting_get_bool(tt))
					id.flag.trade_restriction |= ITR_NOCART;
			}

			if ((tt = libconfig->setting_get_member(t, "nostorage"))) {
				id.flag.trade_restriction &= ~ITR_NOSTORAGE;
				if (libconfig->setting_get_bool(tt))
					id.flag.trade_restriction |= ITR_NOSTORAGE;
			}

			if ((tt = libconfig->setting_get_member(t, "nogstorage"))) {
				id.flag.trade_restriction &= ~ITR_NOGSTORAGE;
				if (libconfig->setting_get_bool(tt))
					id.flag.trade_restriction |= ITR_NOGSTORAGE;
			}

			if ((tt = libconfig->setting_get_member(t, "nomail"))) {
				id.flag.trade_restriction &= ~ITR_NOMAIL;
				if (libconfig->setting_get_bool(tt))
					id.flag.trade_restriction |= ITR_NOMAIL;
			}

			if ((tt = libconfig->setting_get_member(t, "noauction"))) {
				id.flag.trade_restriction &= ~ITR_NOAUCTION;
				if (libconfig->setting_get_bool(tt))
					id.flag.trade_restriction |= ITR_NOAUCTION;
			}
		} else { // Fallback to int if it's not a group
			id.flag.trade_restriction = libconfig->setting_get_int(t);
		}
	}

	if ((t = libconfig->setting_get_member(it, "Nouse"))) {
		if (config_setting_is_group(t)) {
			struct config_setting_t *nt = NULL;

			if ((nt = libconfig->setting_get_member(t, "override"))) {
				id.item_usage.override = libconfig->setting_get_int(nt);
			}

			if ((nt = libconfig->setting_get_member(t, "sitting"))) {
				id.item_usage.flag &= ~INR_SITTING;
				if (libconfig->setting_get_bool(nt))
					id.item_usage.flag |= INR_SITTING;
			}

		} else { // Fallback to int if it's not a group
			id.item_usage.flag = libconfig->setting_get_int(t);
		}
	}

	if ((t = libconfig->setting_get_member(it, "Stack"))) {
		if (config_setting_is_aggregate(t) && libconfig->setting_length(t) >= 1) {
			int stack_flag = libconfig->setting_get_int_elem(t, 1);
			int stack_amount = libconfig->setting_get_int_elem(t, 0);
			if (stack_amount >= 0) {
				id.stack.amount = cap_value(stack_amount, 0, USHRT_MAX);
				id.stack.inventory = (stack_flag&1)!=0;
				id.stack.cart = (stack_flag&2)!=0;
				id.stack.storage = (stack_flag&4)!=0;
				id.stack.guildstorage = (stack_flag&8)!=0;
			}
		}
	}

	if (map->setting_lookup_const(it, "Sprite", &i32) && i32 >= 0) {
		id.flag.available = 1;
		id.view_id = i32;
	}

	if (libconfig->setting_lookup_string(it, "Script", &str))
		id.script = *str ? script->parse(str, source, -id.nameid, SCRIPT_IGNORE_EXTERNAL_BRACKETS, NULL) : NULL;
	else if (clone && id.script != NULL)
		id.script = script->clone_script(id.script);

	if (libconfig->setting_lookup_string(it, "OnEquipScript", &str))
		id.equip_script = *str ? script->parse(str, source, -id.nameid, SCRIPT_IGNORE_EXTERNAL_BRACKETS, NULL) : NULL;
	else if (clone && id.equip_script != NULL)
		id.equip_script = script->clone_script(id.equip_script);

	if (libconfig->setting_lookup_string(it, "OnUnequipScript", &str))
		id.unequip_script = *str ? script->parse(str, source, -id.nameid, SCRIPT_IGNORE_EXTERNAL_BRACKETS, NULL) : NULL;
	else if (clone && id.unequip_script != NULL)
		id.unequip_script = script->clone_script(id.unequip_script);

	if (libconfig->setting_lookup_string(it, "OnRentalStartScript", &str) != CONFIG_FALSE)
		id.rental_start_script = (*str != '\0') ? script->parse(str, source, -id.nameid, SCRIPT_IGNORE_EXTERNAL_BRACKETS, NULL) : NULL;
	else if (clone && id.rental_start_script != NULL)
		id.rental_start_script = script->clone_script(id.rental_start_script);

	if (libconfig->setting_lookup_string(it, "OnRentalEndScript", &str) != CONFIG_FALSE)
		id.rental_end_script = (*str != '\0') ? script->parse(str, source, -id.nameid, SCRIPT_IGNORE_EXTERNAL_BRACKETS, NULL) : NULL;
	else if (clone && id.rental_end_script != NULL)
		id.rental_end_script = script->clone_script(id.rental_end_script);

	return itemdb->validate_entry(&id, n, source);
}

/**
 * Reads from a libconfig-formatted itemdb file and inserts the found entries into the
 * item database, overwriting duplicate ones (i.e. item_db2 overriding item_db.)
 *
 * @param filename File name, relative to the database path.
 * @return The number of found entries.
 */
static int itemdb_readdb_libconfig(const char *filename, struct DBMap *itemconst_db)
{
	bool duplicate[MAX_ITEMDB];
	struct DBMap *duplicate_db;
	struct config_t item_db_conf;
	struct config_setting_t *itdb, *it;
	char filepath[260];
	int i = 0, count = 0;

	nullpo_ret(filename);
	nullpo_ret(itemconst_db);

	snprintf(filepath, sizeof(filepath), "%s/%s", map->db_path, filename);
	if (!libconfig->load_file(&item_db_conf, filepath))
		return 0;

	if ((itdb = libconfig->setting_get_member(item_db_conf.root, "item_db")) == NULL) {
		ShowError("can't read %s\n", filepath);
		return 0;
	}

	// TODO add duplicates check for itemdb->other
	memset(&duplicate,0,sizeof(duplicate));
	duplicate_db = idb_alloc(DB_OPT_BASE);

	while( (it = libconfig->setting_get_elem(itdb,i++)) ) {
		int nameid = itemdb->readdb_libconfig_sub(it, i-1, filename, itemconst_db);

		if (nameid <= 0 || nameid > MAX_ITEM_ID)
			continue;

		itemdb->readdb_additional_fields(nameid, it, i - 1, filename, itemconst_db);
		count++;

		strdb_iput(itemconst_db, itemdb_name(nameid), nameid);

		if (nameid < MAX_ITEMDB) {
			if (duplicate[nameid]) {
				ShowWarning("itemdb_readdb:%s: duplicate entry of ID #%d (%s/%s)\n",
						filename, nameid, itemdb_name(nameid), itemdb_jname(nameid));
			} else {
				duplicate[nameid] = true;
			}
		} else {
			if (idb_exists(duplicate_db, nameid)) {
				ShowWarning("itemdb_readdb:%s: duplicate entry of ID #%d (%s/%s)\n",
						filename, nameid, itemdb_name(nameid), itemdb_jname(nameid));
			} else {
				idb_iput(duplicate_db, nameid, true);
			}
		}
	}
	db_destroy(duplicate_db);
	libconfig->destroy(&item_db_conf);
	ShowStatus("Done reading '"CL_WHITE"%d"CL_RESET"' entries in '"CL_WHITE"%s"CL_RESET"'.\n", count, filepath);
	return count;
}

/*==========================================
 * Unique item ID function
 * Only one operation by once
 *------------------------------------------*/
static uint64 itemdb_unique_id(struct map_session_data *sd)
{

	nullpo_ret(sd);
	return ((uint64)sd->status.char_id << 32) | sd->status.uniqueitem_counter++;
}

static bool itemdb_read_libconfig_lapineddukddak(void)
{
	struct config_t item_lapineddukddak;
	struct config_setting_t *it = NULL;
	char filepath[290];

	int i = 0;
	int count = 0;

	snprintf(filepath, sizeof(filepath), "%s/%s", map->db_path, DBPATH"item_lapineddukddak.conf");
	if (libconfig->load_file(&item_lapineddukddak, filepath) == CONFIG_FALSE)
		return false;

	while ((it = libconfig->setting_get_elem(item_lapineddukddak.root, i++)) != NULL) {
		if (itemdb->read_libconfig_lapineddukddak_sub(it, filepath))
			++count;
	}

	libconfig->destroy(&item_lapineddukddak);
	ShowStatus("Done reading '"CL_WHITE"%d"CL_RESET"' entries in '"CL_WHITE"%s"CL_RESET"'.\n", count, filepath);
	return true;
}

static bool itemdb_read_libconfig_lapineddukddak_sub(struct config_setting_t *it, const char *source)
{
	nullpo_retr(false, it);
	nullpo_retr(false, source);

	struct item_data *data = NULL;
	const char *name = config_setting_name(it);
	const char *str = NULL;
	int i32 = 0;

	if ((data = itemdb->name2id(name)) == NULL) {
		ShowWarning("itemdb_read_libconfig_lapineddukddak_sub: unknown item '%s', skipping..\n", name);
		return false;
	}

	data->lapineddukddak = aCalloc(1, sizeof(struct item_lapineddukddak));
	if (libconfig->setting_lookup_int(it, "NeedCount", &i32) == CONFIG_TRUE)
		data->lapineddukddak->NeedCount = (int16)i32;

	if (libconfig->setting_lookup_int(it, "NeedRefineMin", &i32) == CONFIG_TRUE)
		data->lapineddukddak->NeedRefineMin = (int8)i32;

	if (libconfig->setting_lookup_int(it, "NeedRefineMax", &i32) == CONFIG_TRUE)
		data->lapineddukddak->NeedRefineMax = (int8)i32;

	struct config_setting_t *sources = libconfig->setting_get_member(it, "SourceItems");
	itemdb->read_libconfig_lapineddukddak_sub_sources(sources, data);

	if (libconfig->setting_lookup_string(it, "Script", &str) == CONFIG_TRUE)
		data->lapineddukddak->script = *str ? script->parse(str, source, -data->nameid, SCRIPT_IGNORE_EXTERNAL_BRACKETS, NULL) : NULL;
	return true;
}

static bool itemdb_read_libconfig_lapineddukddak_sub_sources(struct config_setting_t *sources, struct item_data *data)
{
	nullpo_retr(false, data);
	nullpo_retr(false, data->lapineddukddak);

	int i = 0;
	struct config_setting_t *entry = NULL;

	if (sources == NULL || !config_setting_is_group(sources))
		return false;

	VECTOR_INIT(data->lapineddukddak->SourceItems);
	while ((entry = libconfig->setting_get_elem(sources, i++)) != NULL) {
		struct item_data *edata = NULL;
		struct itemlist_entry item = { 0 };
		const char *name = config_setting_name(entry);
		int i32 = 0;

		if ((edata = itemdb->name2id(name)) == NULL) {
			ShowWarning("itemdb_read_libconfig_lapineddukddak_sub: unknown item '%s', skipping..\n", name);
			continue;
		}
		item.id = edata->nameid;

		if ((i32 = libconfig->setting_get_int(entry)) == CONFIG_TRUE && (i32 <= 0 || i32 > MAX_AMOUNT)) {
			ShowWarning("itemdb_read_libconfig_lapineddukddak_sub: invalid amount (%d) for source item '%s', skipping..\n", i32, name);
			continue;
		}
		item.amount = i32;

		VECTOR_ENSURE(data->lapineddukddak->SourceItems, 1, 1);
		VECTOR_PUSH(data->lapineddukddak->SourceItems, item);
	}
	return true;
}

static bool itemdb_read_libconfig_lapineupgrade(void)
{
	struct config_t item_lapineupgrade;
	struct config_setting_t *it = NULL;
	char filepath[290];

	int i = 0;
	int count = 0;

	snprintf(filepath, sizeof(filepath), "%s/%s", map->db_path, DBPATH"item_lapineupgrade.conf");
	if (libconfig->load_file(&item_lapineupgrade, filepath) == CONFIG_FALSE)
		return false;

	while ((it = libconfig->setting_get_elem(item_lapineupgrade.root, i++)) != NULL) {
		if (itemdb->read_libconfig_lapineupgrade_sub(it, filepath))
			++count;
	}

	libconfig->destroy(&item_lapineupgrade);
	ShowStatus("Done reading '"CL_WHITE"%d"CL_RESET"' entries in '"CL_WHITE"%s"CL_RESET"'.\n", count, filepath);
	return true;
}

static bool itemdb_read_libconfig_lapineupgrade_sub(struct config_setting_t *it, const char *source)
{
	nullpo_retr(false, it);
	nullpo_retr(false, source);

	struct item_data *data = NULL;
	const char *name = config_setting_name(it);
	const char *str = NULL;
	int i32 = 0;
	bool real_bool = false;

	if ((data = itemdb->name2id(name)) == NULL) {
		ShowWarning("itemdb_read_libconfig_lapineupgrade_sub: unknown item '%s', skipping..\n", name);
		return false;
	}

	data->lapineupgrade = aCalloc(1, sizeof(struct item_lapineupgrade));

	if (libconfig->setting_lookup_int(it, "NeedRefineMin", &i32) == CONFIG_TRUE)
		data->lapineupgrade->NeedRefineMin = (int8)i32;

	if (libconfig->setting_lookup_int(it, "NeedRefineMax", &i32) == CONFIG_TRUE)
		data->lapineupgrade->NeedRefineMax = (int8)i32;

	if (libconfig->setting_lookup_int(it, "NeedOptionMin", &i32) == CONFIG_TRUE)
		data->lapineupgrade->NeedOptionMin = (int8)i32;

	if (libconfig->setting_lookup_bool_real(it, "NoEnchants", &real_bool) == CONFIG_TRUE)
		data->lapineupgrade->NoEnchant = real_bool;

	struct config_setting_t *targets = libconfig->setting_get_member(it, "TargetItems");
	itemdb->read_libconfig_lapineupgrade_sub_targets(targets, data);

	if (libconfig->setting_lookup_string(it, "Script", &str) == CONFIG_TRUE)
		data->lapineupgrade->script = *str ? script->parse(str, source, -data->nameid, SCRIPT_IGNORE_EXTERNAL_BRACKETS, NULL) : NULL;
	return true;
}

static bool itemdb_read_libconfig_lapineupgrade_sub_targets(struct config_setting_t *targets, struct item_data *data)
{
	nullpo_retr(false, data);
	nullpo_retr(false, data->lapineupgrade);

	int i = 0;
	struct config_setting_t *entry = NULL;

	if (targets == NULL || !config_setting_is_group(targets))
		return false;

	VECTOR_INIT(data->lapineupgrade->TargetItems);
	while ((entry = libconfig->setting_get_elem(targets, i++)) != NULL) {
		struct item_data *edata = NULL;
		struct itemlist_entry item = {0};
		const char *name = config_setting_name(entry);
		int i32 = 0;

		if ((edata = itemdb->name2id(name)) == NULL) {
			ShowWarning("itemdb_read_libconfig_lapineupgrade_sub_targets: unknown item '%s', skipping..\n", name);
			continue;
		}
		item.id = edata->nameid;

		if ((i32 = libconfig->setting_get_int(entry)) == CONFIG_TRUE && (i32 <= 0 || i32 > MAX_AMOUNT)) {
			ShowWarning("itemdb_read_libconfig_lapineupgrade_sub_targets: invalid amount (%d) for target item '%s', skipping..\n", i32, name);
			continue;
		}
		item.amount = i32;

		VECTOR_ENSURE(data->lapineupgrade->TargetItems, 1, 1);
		VECTOR_PUSH(data->lapineupgrade->TargetItems, item);
	}
	return true;
}

static bool itemdb_read_libconfig_item_reform_info(void)
{
	struct config_t item_reform;
	char filepath[290];

	snprintf(filepath, sizeof(filepath), "%s/%s", map->db_path, DBPATH"item_reform_info.conf");
	if (libconfig->load_file(&item_reform, filepath) == CONFIG_FALSE)
		return false;

	int i = 0;
	int count = 0;
	struct config_setting_t *it = NULL;
	struct config_setting_t *rdb = libconfig->lookup(&item_reform, "item_reform_info");
	while ((it = libconfig->setting_get_elem(rdb, i++)) != NULL) {
		if (itemdb->read_libconfig_item_reform_info_sub(it, filepath))
			++count;
	}

	libconfig->destroy(&item_reform);
	ShowStatus("Done reading '"CL_WHITE"%d"CL_RESET"' entries in '"CL_WHITE"%s"CL_RESET"'.\n", count, filepath);
	return true;
}

static bool itemdb_read_libconfig_item_reform_info_sub(struct config_setting_t *it, const char *source)
{
	nullpo_retr(false, it);
	nullpo_retr(false, source);

	struct item_reform ir = { 0 };

	if (libconfig->setting_lookup_int(it, "Id", &ir.Id) == CONFIG_FALSE || ir.Id < 0) {
		ShowWarning("%s: invalid entry Id %d at %s, skipping..\n", __func__, ir.Id, source);
		return false;
	}

	if (!map->setting_lookup_const(it, "BaseItem", &ir.BaseItem)) {
		ShowWarning("%s: invalid BaseItem for entry with Id %d, skipping..\n", __func__, ir.Id);
		return false;
	}

	if (!map->setting_lookup_const(it, "ResultItem", &ir.ResultItem)) {
		ShowWarning("%s: invalid ResultItem for entry with Id %d, skipping..\n", __func__, ir.Id);
		return false;
	}

	struct config_setting_t *materials = libconfig->setting_get_member(it, "Materials");
	if (materials != NULL && !itemdb->read_libconfig_item_reform_info_materials(materials, &ir))
		return false;

	struct config_setting_t *reqinfo = libconfig->setting_get_member(it, "RequirementInfo");
	if (reqinfo != NULL && !itemdb->read_libconfig_item_reform_info_reqinfo(reqinfo, &ir))
		return false;

	struct config_setting_t *behinfo = libconfig->setting_get_member(it, "BehaviorInfo");
	if (behinfo != NULL && !itemdb->read_libconfig_item_reform_info_behinfo(behinfo, &ir))
		return false;

	/* Allocate memory and copy contents */
	struct item_reform *s_ir = aCalloc(1, sizeof(struct item_reform));
	*s_ir = ir;

	/* Store ptr in the database */
	idb_put(itemdb->reform, ir.Id, s_ir);
	return true;
}

static bool itemdb_read_libconfig_item_reform_info_materials(struct config_setting_t *it, struct item_reform *ir)
{
	nullpo_retr(false, it);
	nullpo_retr(false, ir);

	if (!config_setting_is_group(it)) {
		ShowWarning("%s: Materials for entry with Id %d must be a group, skipping..\n", __func__, ir->Id);
		return false;
	}

	VECTOR_INIT(ir->Materials);
	int i = 0;
	struct config_setting_t *entry = NULL;
	while ((entry = libconfig->setting_get_elem(it, i++)) != NULL) {
		const char *name = config_setting_name(entry);
		struct item_data *idata = itemdb->name2id(name);
		struct itemlist_entry item = { 0 };

		if (idata == NULL) {
			ShowWarning("%s: unknown item '%s' for entry with Id %d, skipping..\n", __func__, name, ir->Id);
			continue;
		}
		item.id = idata->nameid;

		int i32 = 0;
		if ((i32 = libconfig->setting_get_int(entry)) == CONFIG_TRUE && (i32 <= 0 || i32 > MAX_AMOUNT)) {
			ShowWarning("%s: invalid amount (%d) for materials item '%s' at entry with Id %d, skipping..\n", __func__, i32, name, ir->Id);
			continue;
		}
		item.amount = i32;

		VECTOR_ENSURE(ir->Materials, 1, 1);
		VECTOR_PUSH(ir->Materials, item);
	}
	return true;
}

static bool itemdb_read_libconfig_item_reform_info_reqinfo(struct config_setting_t *it, struct item_reform *ir)
{
	nullpo_retr(false, it);
	nullpo_retr(false, ir);

	int i32 = 0;
	if (libconfig->setting_lookup_int(it, "NeedRefineMin", &i32) == CONFIG_TRUE)
		ir->NeedRefineMin = cap_value(i32, 0, MAX_REFINE);

	if (libconfig->setting_lookup_int(it, "NeedRefineMax", &i32) == CONFIG_TRUE)
		ir->NeedRefineMax = cap_value(i32, 0, MAX_REFINE);

	if (libconfig->setting_lookup_int(it, "NeedOptionNumMin", &i32) == CONFIG_TRUE)
		ir->NeedOptionNumMin = cap_value(i32, 0, MAX_ITEM_OPTIONS);

	if (libconfig->setting_lookup_bool(it, "IsEmptySocket", &i32) == CONFIG_TRUE)
		ir->IsEmptySocket = (bool)i32;

	return true;
}

static bool itemdb_read_libconfig_item_reform_info_behinfo(struct config_setting_t *it, struct item_reform *ir)
{
	nullpo_retr(false, it);
	nullpo_retr(false, ir);

	int i32 = 0;
	if (libconfig->setting_lookup_int(it, "ChangeRefineValue", &i32) == CONFIG_TRUE)
		ir->ChangeRefineValue = cap_value(i32, -MAX_REFINE, MAX_REFINE);

	if (libconfig->setting_lookup_bool(it, "PreserveSocketItem", &i32) == CONFIG_TRUE)
		ir->PreserveSocketItem = (bool)i32;

	if (libconfig->setting_lookup_bool(it, "PreserveOptions", &i32) == CONFIG_TRUE)
		ir->PreserveOptions = (bool)i32;

	if (libconfig->setting_lookup_bool(it, "PreserveGrade", &i32) == CONFIG_TRUE)
		ir->PreserveGrade = (bool)i32;

	return true;
}

static bool itemdb_read_libconfig_item_reform_list(void)
{
	struct config_t item_reform;
	char filepath[290];

	snprintf(filepath, sizeof(filepath), "%s/%s", map->db_path, DBPATH"item_reform_list.conf");
	if (libconfig->load_file(&item_reform, filepath) == CONFIG_FALSE)
		return false;

	int i = 0;
	int count = 0;
	struct config_setting_t *irl = libconfig->lookup(&item_reform, "item_reform_list");
	if (irl != NULL) {
		struct config_setting_t *gr = libconfig->setting_get_elem(irl, 0);
		struct config_setting_t *it = NULL;
		while ((it = libconfig->setting_get_elem(gr, i++)) != NULL) {
			if (itemdb->read_libconfig_item_reform_list_sub(it, filepath))
				++count;
		}
	}

	libconfig->destroy(&item_reform);
	ShowStatus("Done reading '"CL_WHITE"%d"CL_RESET"' entries in '"CL_WHITE"%s"CL_RESET"'.\n", count, filepath);
	return true;
}

static bool itemdb_read_libconfig_item_reform_list_sub(struct config_setting_t *it, const char *source)
{
	nullpo_retr(false, it);
	nullpo_retr(false, source);

	struct item_data *itd = NULL;
	const char *name = config_setting_name(it);

	if ((itd = itemdb->name2id(name)) == NULL) {
		ShowWarning("%s: unknown item '%s', in (%s), skipping..\n", __func__, name, source);
		return false;
	}

	if (!config_setting_is_array(it)) {
		ShowWarning("%s: reform list for item (%s) must be an array, in (%s), skipping..\n", __func__, name, source);
		return false;
	}

	const int len = libconfig->setting_length(it);
	for (int i = 0; i < len; i++) {
		const int reform_id = libconfig->setting_get_int_elem(it, i);
		const struct item_reform *ir = itemdb->reform_exists(reform_id);
		if (ir == NULL) {
			ShowWarning("%s: unknown reform id #%d for item '%s', in (%s), skipping..\n", __func__, reform_id, name, source);
			continue;
		}
		if (itemdb->search_reform_baseitem(itd, ir->BaseItem) != NULL) {
			ShowWarning("%s: duplicated BaseItem in reform id #%d for item '%s', in (%s), skipping..\n", __func__, reform_id, name, source);
			continue;
		}
		VECTOR_ENSURE(itd->reform_list, len, 1);
		VECTOR_PUSH(itd->reform_list, reform_id);
	}
	return true;
}

static void itemdb_item_reform(struct map_session_data *sd, const struct item_reform *ir, int idx)
{
	nullpo_retv(sd);
	nullpo_retv(ir);
	Assert_retv(idx >= 0 && idx < sd->status.inventorySize);

	const struct item *itr = &sd->status.inventory[idx];

	// Validate refine rate requirement
	if ((itemdb_type(itr->nameid) == IT_ARMOR || itemdb_type(itr->nameid) == IT_WEAPON)
		&& (itr->refine < ir->NeedRefineMin || itr->refine > ir->NeedRefineMax))
		return;

	// Validate random option requirement
	int options = 0;
	for (int i = 0; i < MAX_ITEM_OPTIONS; ++i) {
		if (itr->option[i].index != 0)
			options++;
	}

	if (ir->NeedOptionNumMin > options)
		return;

	// Validate empty slots requirement
	if (ir->IsEmptySocket) {
		int cards = 0;
		for (int i = 0; i < MAX_SLOTS; ++i) {
			if (itr->card[i] != 0)
				cards++;
		}

		if (cards > 0)
			return;
	}

	// Validate materials requirement
	for (int i = 0; i < VECTOR_LENGTH(ir->Materials); ++i) {
		int material_idx = pc->search_inventory(sd, VECTOR_INDEX(ir->Materials, i).id);

		if (material_idx == INDEX_NOT_FOUND || sd->status.inventory[material_idx].amount < VECTOR_INDEX(ir->Materials, i).amount) {
			clif->item_reform_result(sd, idx, IT_REFORM_NOT_ENOUGH_MATERIALS);
			return;
		}
	}

	// Create the result item
	struct item item_tmp;
	memset(&item_tmp, 0, sizeof(item_tmp));
	item_tmp.nameid = ir->ResultItem;
	item_tmp.identify = 1;

	// Apply reform changes
	if (ir->PreserveSocketItem)
		memcpy(&item_tmp.card, itr->card, sizeof(item_tmp.card));
	if (ir->PreserveOptions)
		memcpy(&item_tmp.option, itr->option, sizeof(item_tmp.option));
	if (ir->PreserveGrade)
		item_tmp.grade = itr->grade;
	item_tmp.refine = cap_value(itr->refine + ir->ChangeRefineValue, 0, MAX_REFINE);

	// Consume the required materials
	for (int i = 0; i < VECTOR_LENGTH(ir->Materials); ++i) {
		int material_idx = pc->search_inventory(sd, VECTOR_INDEX(ir->Materials, i).id);
		pc->delitem(sd, material_idx, VECTOR_INDEX(ir->Materials, i).amount, 0, DELITEM_NORMAL, LOG_TYPE_SCRIPT);
	}
	pc->delitem(sd, idx, 1, 0, DELITEM_NORMAL, LOG_TYPE_SCRIPT);

	// Give the reformed item
	pc->additem(sd, &item_tmp, 1, LOG_TYPE_SCRIPT);
	clif->item_reform_result(sd, idx, IT_REFORM_SUCCESS);
}

static const struct item_reform *itemdb_search_reform_baseitem(const struct item_data *itd, int nameid)
{
	nullpo_retr(NULL, itd);
	nullpo_retr(NULL, itemdb->exists(nameid));

	for (int i = 0; i < VECTOR_LENGTH(itd->reform_list); ++i) {
		const struct item_reform *ir = itemdb->reform_exists(VECTOR_INDEX(itd->reform_list, i));
		if (ir != NULL && ir->BaseItem == nameid)
			return ir;
	}
	return NULL;
}

/**
 * Reads all item-related databases.
 */
static void itemdb_read(bool minimal)
{
	int i;
	struct DBData prev;

	const char *filename[] = {
		DBPATH"item_db.conf",
		"item_db2.conf",
	};

	// temporary itemconst db for item cloning because it happens before itemdb->name_constants()
	struct DBMap *itemconst_db = strdb_alloc(DB_OPT_BASE, ITEM_NAME_LENGTH);

	for (i = 0; i < ARRAYLENGTH(filename); i++)
		itemdb->readdb_libconfig(filename[i], itemconst_db);

	db_destroy(itemconst_db);

	// TODO check duplicate names also in itemdb->other
	for( i = 0; i < ARRAYLENGTH(itemdb->array); ++i ) {
		if( itemdb->array[i] ) {
			if( itemdb->names->put(itemdb->names,DB->str2key(itemdb->array[i]->name),DB->ptr2data(itemdb->array[i]),&prev) ) {
				struct item_data *data = DB->data2ptr(&prev);
				ShowError("itemdb_read: duplicate AegisName '%s' in item ID %d and %d\n",itemdb->array[i]->name,itemdb->array[i]->nameid,data->nameid);
			}
		}
	}

	itemdb->other->foreach(itemdb->other, itemdb->addname_sub);

	itemdb->read_options();

	if (minimal)
		return;

	itemdb->name_constants();

	itemdb->read_combodb_libconfig();
	itemdb->read_groups();
	itemdb->read_chains();
	itemdb->read_packages();
	itemdb->read_libconfig_lapineddukddak();
	itemdb->read_libconfig_lapineupgrade();
	itemdb->read_libconfig_item_reform_info();
	itemdb->read_libconfig_item_reform_list();
}

/**
 * Add item name with high id into map
 * @see DBApply
 */
static int itemdb_addname_sub(union DBKey key, struct DBData *data, va_list ap)
{
	struct item_data *item = DB->data2ptr(data);
	struct DBData prev;

	if (itemdb->names->put(itemdb->names, DB->str2key(item->name), DB->ptr2data(item), &prev)) {
		struct item_data *oldItem = DB->data2ptr(&prev);
		ShowError("itemdb_read: duplicate AegisName '%s' in item ID %d and %d\n", item->name, item->nameid, oldItem->nameid);
	}

	return 0;
}

/**
 * retrieves item_combo data by combo id
 **/
static struct item_combo *itemdb_id2combo(int id)
{
	if( id > itemdb->combo_count )
		return NULL;
	return itemdb->combos[id];
}

/**
 * check is item have usable type
 **/
static bool itemdb_is_item_usable(struct item_data *item)
{
	nullpo_retr(false, item);
	return item->type == IT_HEALING || item->type == IT_USABLE || item->type == IT_CASH;
}

/*==========================================
 * Initialize / Finalize
 *------------------------------------------*/

/// Destroys the item_data.
static void destroy_item_data(struct item_data *self, int free_self)
{
	if( self == NULL )
		return;
	// free scripts
	if( self->script )
		script->free_code(self->script);
	if( self->equip_script )
		script->free_code(self->equip_script);
	if( self->unequip_script )
		script->free_code(self->unequip_script);
	if (self->rental_start_script != NULL)
		script->free_code(self->rental_start_script);
	if (self->rental_end_script != NULL)
		script->free_code(self->rental_end_script);
	if( self->combos )
		aFree(self->combos);
	if (self->lapineddukddak != NULL) {
		if (self->lapineddukddak->script != NULL)
			script->free_code(self->lapineddukddak->script);
		VECTOR_CLEAR(self->lapineddukddak->SourceItems);
		aFree(self->lapineddukddak);
	}
	if (self->lapineupgrade != NULL) {
		if (self->lapineupgrade->script != NULL)
			script->free_code(self->lapineupgrade->script);
		VECTOR_CLEAR(self->lapineupgrade->TargetItems);
		aFree(self->lapineupgrade);
	}
	VECTOR_CLEAR(self->reform_list);
	HPM->data_store_destroy(&self->hdata);
#if defined(DEBUG)
	// trash item
	memset(self, 0xDD, sizeof(struct item_data));
#endif
	// free self
	if( free_self )
		aFree(self);
}

/**
 * @see DBApply
 */
static int itemdb_final_sub(union DBKey key, struct DBData *data, va_list ap)
{
	struct item_data *id = DB->data2ptr(data);

	if( id != &itemdb->dummy )
		itemdb->destroy_item_data(id, 1);

	return 0;
}

static int itemdb_options_final_sub(union DBKey key, struct DBData *data, va_list ap)
{
	struct itemdb_option *ito = DB->data2ptr(data);

	if (ito->script != NULL)
		script->free_code(ito->script);

	return 0;
}

static int itemdb_reform_final_sub(union DBKey key, struct DBData *data, va_list ap)
{
	struct item_reform *ito = DB->data2ptr(data);

	VECTOR_CLEAR(ito->Materials);

	return 0;
}

static void itemdb_clear(bool total)
{
	int i;
	// clear the previous itemdb data
	for( i = 0; i < ARRAYLENGTH(itemdb->array); ++i ) {
		if( itemdb->array[i] )
			itemdb->destroy_item_data(itemdb->array[i], 1);
	}

	if( itemdb->groups )
	{
		for( i = 0; i < itemdb->group_count; i++ ) {
			if( itemdb->groups[i].nameid )
				aFree(itemdb->groups[i].nameid);
		}
		aFree(itemdb->groups);
	}

	itemdb->groups = NULL;
	itemdb->group_count = 0;

	if (itemdb->chains) {
		for (i = 0; i < itemdb->chain_count; i++) {
			if (itemdb->chains[i].items)
				aFree(itemdb->chains[i].items);
		}
		aFree(itemdb->chains);
	}

	itemdb->chains = NULL;
	itemdb->chain_count = 0;

	if (itemdb->packages) {
		for (i = 0; i < itemdb->package_count; i++) {
			if (itemdb->packages[i].random_groups) {
				int j;
				for (j = 0; j < itemdb->packages[i].random_qty; j++)
					aFree(itemdb->packages[i].random_groups[j].random_list);
				aFree(itemdb->packages[i].random_groups);
			}
			if( itemdb->packages[i].must_items )
				aFree(itemdb->packages[i].must_items);
		}
		aFree(itemdb->packages);
		itemdb->packages = NULL;
	}
	itemdb->package_count = 0;

	if (itemdb->combos) {
		for (i = 0; i < itemdb->combo_count; i++) {
			if (itemdb->combos[i]->script) // Check if script was loaded
				script->free_code(itemdb->combos[i]->script);
			aFree(itemdb->combos[i]);
		}
		aFree(itemdb->combos);
	}

	itemdb->combos = NULL;
	itemdb->combo_count = 0;

	if (total)
		return;

	itemdb->other->clear(itemdb->other, itemdb->final_sub);
	itemdb->options->clear(itemdb->options, itemdb->options_final_sub);
	itemdb->reform->clear(itemdb->reform, itemdb->reform_final_sub);

	memset(itemdb->array, 0, sizeof(itemdb->array));

	db_clear(itemdb->names);
}

static void itemdb_reload(void)
{
	struct s_mapiterator* iter;
	struct map_session_data* sd;

	int i,d,k;

	itemdb->clear(false);

	// read new data
	itemdb->read(false);

	//Epoque's awesome @reloaditemdb fix - thanks! [Ind]
	//- Fixes the need of a @reloadmobdb after a @reloaditemdb to re-link monster drop data
	for (i = 0; i < MAX_MOB_DB; i++) {
		struct mob_db *entry;
		if ((i >= MOBID_TREASURE_BOX1 && i <= MOBID_TREASURE_BOX40) || (i >= MOBID_TREASURE_BOX41 && i <= MOBID_TREASURE_BOX49))
			continue;
		entry = mob->db(i);
		for(d = 0; d < MAX_MOB_DROP; d++) {
			struct item_data *id;
			if( !entry->dropitem[d].nameid )
				continue;
			id = itemdb->search(entry->dropitem[d].nameid);

			for (k = 0; k < MAX_SEARCH; k++) {
				if (id->mob[k].chance <= entry->dropitem[d].p)
					break;
			}

			if (k == MAX_SEARCH)
				continue;

			if (id->mob[k].id != i && k != MAX_SEARCH - 1)
				memmove(&id->mob[k+1], &id->mob[k], (MAX_SEARCH-k-1)*sizeof(id->mob[0]));
			id->mob[k].chance = entry->dropitem[d].p;
			id->mob[k].id = i;
		}
	}

	// readjust itemdb pointer cache for each player
	iter = mapit_geteachpc();
	for (sd = BL_UCAST(BL_PC, mapit->first(iter)); mapit->exists(iter); sd = BL_UCAST(BL_PC, mapit->next(iter))) {
		memset(sd->item_delay, 0, sizeof(sd->item_delay));  // reset item delays
		pc->setinventorydata(sd);

		if (battle->bc->item_check != PCCHECKITEM_NONE) // Check and flag items for inspection.
			sd->itemcheck = (enum pc_checkitem_types) battle->bc->item_check;

		/* clear combo bonuses */
		if (sd->combo_count) {
			aFree(sd->combos);
			sd->combos = NULL;
			sd->combo_count = 0;
			if( pc->load_combo(sd) > 0 )
				status_calc_pc(sd,SCO_FORCE);
		}

		// Check for and delete unavailable/disabled items.
		pc->checkitem(sd);
	}
	mapit->free(iter);
}
static void itemdb_name_constants(void)
{
	struct DBIterator *iter = db_iterator(itemdb->names);
	struct item_data *data;

#ifdef ENABLE_CASE_CHECK
	script->parser_current_file = "Item Database (Likely an invalid or conflicting AegisName)";
#endif // ENABLE_CASE_CHECK
	for( data = dbi_first(iter); dbi_exists(iter); data = dbi_next(iter) )
		script->set_constant2(data->name, data->nameid, false, false);
#ifdef ENABLE_CASE_CHECK
	script->parser_current_file = NULL;
#endif // ENABLE_CASE_CHECK

	dbi_destroy(iter);
}
static void do_final_itemdb(void)
{
	itemdb->clear(true);

	itemdb->other->destroy(itemdb->other, itemdb->final_sub);
	itemdb->options->destroy(itemdb->options, itemdb->options_final_sub);
	itemdb->reform->destroy(itemdb->reform, itemdb->reform_final_sub);
	itemdb->destroy_item_data(&itemdb->dummy, 0);
	db_destroy(itemdb->names);
	VECTOR_CLEAR(clif->attendance_data);
}

static void do_init_itemdb(bool minimal)
{
	memset(itemdb->array, 0, sizeof(itemdb->array));
	itemdb->other = idb_alloc(DB_OPT_BASE);
	itemdb->options = idb_alloc(DB_OPT_RELEASE_DATA);
	itemdb->reform = idb_alloc(DB_OPT_RELEASE_DATA);
	itemdb->names = strdb_alloc(DB_OPT_BASE,ITEM_NAME_LENGTH);
	itemdb->create_dummy_data(); //Dummy data item.
	itemdb->read(minimal);

	if (minimal)
		return;

	clif->cashshop_load();

	/** it failed? we disable it **/
	if (battle_config.feature_roulette == 1 && !clif->parse_roulette_db())
		battle_config.feature_roulette = 0;
	VECTOR_INIT(clif->attendance_data);
	clif->pAttendanceDB();
}
void itemdb_defaults(void)
{
	itemdb = &itemdb_s;

	itemdb->init = do_init_itemdb;
	itemdb->final = do_final_itemdb;
	itemdb->reload = itemdb_reload;
	itemdb->name_constants = itemdb_name_constants;
	/* */
	itemdb->groups = NULL;
	itemdb->group_count = 0;
	/* */
	itemdb->chains = NULL;
	itemdb->chain_count = 0;
	/* */
	itemdb->packages = NULL;
	itemdb->package_count = 0;
	/* */
	itemdb->combos = NULL;
	itemdb->combo_count = 0;
	/* */
	itemdb->names = NULL;
	/* */
	/* itemdb->array is cleared on itemdb->init() */
	itemdb->other = NULL;
	memset(&itemdb->dummy, 0, sizeof(struct item_data));
	/* */
	itemdb->read_groups = itemdb_read_groups;
	itemdb->read_chains = itemdb_read_chains;
	itemdb->read_packages = itemdb_read_packages;
	itemdb->read_options = itemdb_read_options;
	/* */
	itemdb->write_cached_packages = itemdb_write_cached_packages;
	itemdb->read_cached_packages = itemdb_read_cached_packages;
	/* */
	itemdb->name2id = itemdb_name2id;
	itemdb->search_name = itemdb_searchname;
	itemdb->search_name_array = itemdb_searchname_array;
	itemdb->load = itemdb_load;
	itemdb->search = itemdb_search;
	itemdb->exists = itemdb_exists;
	itemdb->option_exists = itemdb_option_exists;
	itemdb->reform_exists = itemdb_reform_exists;
	itemdb->in_group = itemdb_in_group;
	itemdb->search_group = itemdb_search_group;
	itemdb->group_item = itemdb_searchrandomid;
	itemdb->chain_item = itemdb_chain_item;
	itemdb->package_item = itemdb_package_item;
	itemdb->searchname_sub = itemdb_searchname_sub;
	itemdb->searchname_array_sub = itemdb_searchname_array_sub;
	itemdb->searchrandomid = itemdb_searchrandomid;
	itemdb->typename = itemdb_typename;
	itemdb->jobmask2mapid = itemdb_jobmask2mapid;
	itemdb->jobid2mapid = itemdb_jobid2mapid;
	itemdb->create_dummy_data = create_dummy_data;
	itemdb->create_item_data = create_item_data;
	itemdb->isequip = itemdb_isequip;
	itemdb->isequip2 = itemdb_isequip2;
	itemdb->isstackable = itemdb_isstackable;
	itemdb->isstackable2 = itemdb_isstackable2;
	itemdb->isdropable_sub = itemdb_isdropable_sub;
	itemdb->cantrade_sub = itemdb_cantrade_sub;
	itemdb->canpartnertrade_sub = itemdb_canpartnertrade_sub;
	itemdb->cansell_sub = itemdb_cansell_sub;
	itemdb->cancartstore_sub = itemdb_cancartstore_sub;
	itemdb->canstore_sub = itemdb_canstore_sub;
	itemdb->canguildstore_sub = itemdb_canguildstore_sub;
	itemdb->canmail_sub = itemdb_canmail_sub;
	itemdb->canauction_sub = itemdb_canauction_sub;
	itemdb->isrestricted = itemdb_isrestricted;
	itemdb->isidentified = itemdb_isidentified;
	itemdb->isidentified2 = itemdb_isidentified2;
	itemdb->read_combodb_libconfig = itemdb_read_combodb_libconfig;
	itemdb->read_combodb_libconfig_sub = itemdb_read_combodb_libconfig_sub;
	itemdb->gendercheck = itemdb_gendercheck;
	itemdb->validate_entry = itemdb_validate_entry;
	itemdb->readdb_options_additional_fields = itemdb_readdb_options_additional_fields;
	itemdb->readdb_additional_fields = itemdb_readdb_additional_fields;
	itemdb->readdb_job_sub = itemdb_readdb_job_sub;
	itemdb->readdb_libconfig_sub = itemdb_readdb_libconfig_sub;
	itemdb->readdb_libconfig = itemdb_readdb_libconfig;
	itemdb->unique_id = itemdb_unique_id;
	itemdb->read = itemdb_read;
	itemdb->destroy_item_data = destroy_item_data;
	itemdb->final_sub = itemdb_final_sub;
	itemdb->options_final_sub = itemdb_options_final_sub;
	itemdb->reform_final_sub = itemdb_reform_final_sub;
	itemdb->clear = itemdb_clear;
	itemdb->id2combo = itemdb_id2combo;
	itemdb->is_item_usable = itemdb_is_item_usable;
	itemdb->addname_sub = itemdb_addname_sub;
	itemdb->read_libconfig_lapineddukddak = itemdb_read_libconfig_lapineddukddak;
	itemdb->read_libconfig_lapineddukddak_sub = itemdb_read_libconfig_lapineddukddak_sub;
	itemdb->read_libconfig_lapineddukddak_sub_sources = itemdb_read_libconfig_lapineddukddak_sub_sources;
	itemdb->read_libconfig_lapineupgrade = itemdb_read_libconfig_lapineupgrade;
	itemdb->read_libconfig_lapineupgrade_sub = itemdb_read_libconfig_lapineupgrade_sub;
	itemdb->read_libconfig_lapineupgrade_sub_targets = itemdb_read_libconfig_lapineupgrade_sub_targets;
	itemdb->read_libconfig_item_reform_info = itemdb_read_libconfig_item_reform_info;
	itemdb->read_libconfig_item_reform_info_sub = itemdb_read_libconfig_item_reform_info_sub;
	itemdb->read_libconfig_item_reform_info_materials = itemdb_read_libconfig_item_reform_info_materials;
	itemdb->read_libconfig_item_reform_info_reqinfo = itemdb_read_libconfig_item_reform_info_reqinfo;
	itemdb->read_libconfig_item_reform_info_behinfo = itemdb_read_libconfig_item_reform_info_behinfo;
	itemdb->read_libconfig_item_reform_list = itemdb_read_libconfig_item_reform_list;
	itemdb->read_libconfig_item_reform_list_sub = itemdb_read_libconfig_item_reform_list_sub;
	itemdb->item_reform = itemdb_item_reform;
	itemdb->search_reform_baseitem = itemdb_search_reform_baseitem;
}
