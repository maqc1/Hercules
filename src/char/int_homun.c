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

#include "int_homun.h"

#include "char/char.h"
#include "char/inter.h"
#include "char/mapif.h"
#include "common/memmgr.h"
#include "common/mmo.h"
#include "common/nullpo.h"
#include "common/showmsg.h"
#include "common/socket.h"
#include "common/sql.h"
#include "common/strlib.h"
#include "common/utils.h"

#include <stdio.h>
#include <stdlib.h>

static struct inter_homunculus_interface inter_homunculus_s;
struct inter_homunculus_interface *inter_homunculus;

static int inter_homunculus_sql_init(void)
{
	return 0;
}
static void inter_homunculus_sql_final(void)
{
	return;
}

/**
 * Creates a new homunculus with the given data.
 *
 * @remark
 *   The homunculus ID is expected to be 0, and will be filled with the newly
 *   assigned ID.
 *
 * @param[in,out] hd The new homunculus' data.
 * @retval false in case of errors.
 */
static bool inter_homunculus_create(struct s_homunculus *hd)
{
	char esc_name[NAME_LENGTH*2+1];

	nullpo_retr(false, hd);
	Assert_retr(false, hd->hom_id == 0);

	SQL->EscapeStringLen(inter->sql_handle, esc_name, hd->name, strnlen(hd->name, NAME_LENGTH));

	if (SQL_ERROR == SQL->Query(inter->sql_handle, "INSERT INTO `%s` "
			"(`char_id`, `class`,`prev_class`,`name`,`level`,`exp`,`intimacy`,`hunger`, `str`, `agi`, `vit`, `int`, `dex`, `luk`, `hp`,`max_hp`,`sp`,`max_sp`,`skill_point`, `rename_flag`, `vaporize`, `autofeed`) "
			"VALUES ('%d', '%d', '%d', '%s', '%d', '%"PRIu64"', '%u', '%d', '%d', %d, '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d')",
			homunculus_db, hd->char_id, hd->class_, hd->prev_class, esc_name, hd->level, hd->exp, hd->intimacy, hd->hunger, hd->str, hd->agi, hd->vit, hd->int_, hd->dex, hd->luk,
			hd->hp, hd->max_hp, hd->sp, hd->max_sp, hd->skillpts, hd->rename_flag, hd->vaporize, hd->autofeed)) {
		Sql_ShowDebug(inter->sql_handle);
		return false;
	}
	hd->hom_id = (int)SQL->LastInsertId(inter->sql_handle);
	return true;
}

/**
 * Saves an existing homunculus.
 *
 * @param hd The homunculus' data.
 * @retval false in case of errors.
 */
static bool inter_homunculus_save(const struct s_homunculus *hd)
{
	bool flag = true;
	char esc_name[NAME_LENGTH*2+1];

	nullpo_retr(false, hd);
	Assert_retr(false, hd->hom_id > 0);

	SQL->EscapeStringLen(inter->sql_handle, esc_name, hd->name, strnlen(hd->name, NAME_LENGTH));

	if (SQL_ERROR == SQL->Query(inter->sql_handle, "UPDATE `%s` SET `char_id`='%d', `class`='%d',`prev_class`='%d',`name`='%s',`level`='%d',`exp`='%"PRIu64"',`intimacy`='%u',`hunger`='%d', `str`='%d', `agi`='%d', `vit`='%d', `int`='%d', `dex`='%d', `luk`='%d', `hp`='%d',`max_hp`='%d',`sp`='%d',`max_sp`='%d',`skill_point`='%d', `rename_flag`='%d', `vaporize`='%d', `autofeed`='%d' WHERE `homun_id`='%d'",
			homunculus_db, hd->char_id, hd->class_, hd->prev_class, esc_name, hd->level, hd->exp, hd->intimacy, hd->hunger, hd->str, hd->agi, hd->vit, hd->int_, hd->dex, hd->luk,
			hd->hp, hd->max_hp, hd->sp, hd->max_sp, hd->skillpts, hd->rename_flag, hd->vaporize, hd->autofeed, hd->hom_id)) {
		Sql_ShowDebug(inter->sql_handle);
		flag = false;
	} else {
		int i;
		struct SqlStmt *stmt = SQL->StmtMalloc(inter->sql_handle);

		if (SQL_ERROR == SQL->StmtPrepare(stmt, "REPLACE INTO `%s` (`homun_id`, `id`, `lv`) VALUES (%d, ?, ?)", skill_homunculus_db, hd->hom_id)) {
			SqlStmt_ShowDebug(stmt);
			flag = false;
		} else {
			for (i = 0; i < MAX_HOMUNSKILL; ++i) {
				if (hd->hskill[i].id > 0 && hd->hskill[i].lv != 0) {
					if (SQL_ERROR == SQL->StmtBindParam(stmt, 0, SQLDT_USHORT, &hd->hskill[i].id, sizeof hd->hskill[i].id)
					 || SQL_ERROR == SQL->StmtBindParam(stmt, 1, SQLDT_UCHAR,  &hd->hskill[i].lv, sizeof hd->hskill[i].lv)
					 || SQL_ERROR == SQL->StmtExecute(stmt)) {
						SqlStmt_ShowDebug(stmt);
						flag = false;
						break;
					}
				}
			}
		}
		SQL->StmtFree(stmt);
	}

	return flag;
}

// Load an homunculus
static bool inter_homunculus_load(int homun_id, struct s_homunculus *hd)
{
	char* data;
	size_t len;

	nullpo_ret(hd);
	memset(hd, 0, sizeof(*hd));

	if (SQL_ERROR == SQL->Query(inter->sql_handle, "SELECT `homun_id`,`char_id`,`class`,`prev_class`,`name`,`level`,`exp`,`intimacy`,`hunger`, `str`, `agi`, `vit`, `int`, `dex`, `luk`, `hp`,`max_hp`,`sp`,`max_sp`,`skill_point`,`rename_flag`, `vaporize`, `autofeed` FROM `%s` WHERE `homun_id`='%d'", homunculus_db, homun_id))
	{
		Sql_ShowDebug(inter->sql_handle);
		return false;
	}

	if (!SQL->NumRows(inter->sql_handle)) {
		//No homunculus found.
		SQL->FreeResult(inter->sql_handle);
		return false;
	}
	if( SQL_SUCCESS != SQL->NextRow(inter->sql_handle) )
	{
		Sql_ShowDebug(inter->sql_handle);
		SQL->FreeResult(inter->sql_handle);
		return false;
	}

	hd->hom_id = homun_id;
	SQL->GetData(inter->sql_handle,  1, &data, NULL); hd->char_id = atoi(data);
	SQL->GetData(inter->sql_handle,  2, &data, NULL); hd->class_ = atoi(data);
	SQL->GetData(inter->sql_handle,  3, &data, NULL); hd->prev_class = atoi(data);
	SQL->GetData(inter->sql_handle,  4, &data, &len); safestrncpy(hd->name, data, sizeof(hd->name));
	SQL->GetData(inter->sql_handle,  5, &data, NULL); hd->level = atoi(data);
	SQL->GetData(inter->sql_handle,  6, &data, NULL); hd->exp = atol(data);
	SQL->GetData(inter->sql_handle,  7, &data, NULL); hd->intimacy = (unsigned int)strtoul(data, NULL, 10);
	SQL->GetData(inter->sql_handle,  8, &data, NULL); hd->hunger = atoi(data);
	SQL->GetData(inter->sql_handle,  9, &data, NULL); hd->str = atoi(data);
	SQL->GetData(inter->sql_handle, 10, &data, NULL); hd->agi = atoi(data);
	SQL->GetData(inter->sql_handle, 11, &data, NULL); hd->vit = atoi(data);
	SQL->GetData(inter->sql_handle, 12, &data, NULL); hd->int_ = atoi(data);
	SQL->GetData(inter->sql_handle, 13, &data, NULL); hd->dex = atoi(data);
	SQL->GetData(inter->sql_handle, 14, &data, NULL); hd->luk = atoi(data);
	SQL->GetData(inter->sql_handle, 15, &data, NULL); hd->hp = atoi(data);
	SQL->GetData(inter->sql_handle, 16, &data, NULL); hd->max_hp = atoi(data);
	SQL->GetData(inter->sql_handle, 17, &data, NULL); hd->sp = atoi(data);
	SQL->GetData(inter->sql_handle, 18, &data, NULL); hd->max_sp = atoi(data);
	SQL->GetData(inter->sql_handle, 19, &data, NULL); hd->skillpts = atoi(data);
	SQL->GetData(inter->sql_handle, 20, &data, NULL); hd->rename_flag = atoi(data);
	SQL->GetData(inter->sql_handle, 21, &data, NULL); hd->vaporize = atoi(data);
	SQL->GetData(inter->sql_handle, 22, &data, NULL); hd->autofeed = atoi(data);
	SQL->FreeResult(inter->sql_handle);

	hd->intimacy = cap_value(hd->intimacy, 0, 100000);
	hd->hunger = cap_value(hd->hunger, 0, 100);

	// Load Homunculus Skill
	if (SQL_ERROR == SQL->Query(inter->sql_handle, "SELECT `id`,`lv` FROM `%s` WHERE `homun_id`=%d", skill_homunculus_db, homun_id)) {
		Sql_ShowDebug(inter->sql_handle);
		return false;
	}
	while (SQL_SUCCESS == SQL->NextRow(inter->sql_handle)) {
		int idx;
		// id
		SQL->GetData(inter->sql_handle, 0, &data, NULL);
		idx = atoi(data);
		if (idx < HM_SKILLBASE || idx >= HM_SKILLBASE + MAX_HOMUNSKILL)
			continue;// invalid skill id
		idx -= HM_SKILLBASE;
		hd->hskill[idx].id = (unsigned short)atoi(data);

		// lv
		SQL->GetData(inter->sql_handle, 1, &data, NULL);
		hd->hskill[idx].lv = (unsigned char)atoi(data);
	}
	SQL->FreeResult(inter->sql_handle);

	if (chr->show_save_log)
		ShowInfo("Homunculus loaded (%d - %s).\n", hd->hom_id, hd->name);

	return true;
}

static bool inter_homunculus_delete(int homun_id)
{
	if (SQL_ERROR == SQL->Query(inter->sql_handle, "DELETE FROM `%s` WHERE `homun_id` = '%d'", homunculus_db, homun_id)
	 || SQL_ERROR == SQL->Query(inter->sql_handle, "DELETE FROM `%s` WHERE `homun_id` = '%d'", skill_homunculus_db, homun_id)
	) {
		Sql_ShowDebug(inter->sql_handle);
		return false;
	}
	return true;
}

static bool inter_homunculus_rename(const char *name)
{
	int i;

	nullpo_ret(name);
	// Check Authorized letters/symbols in the name of the homun
	if( char_name_option == 1 )
	{// only letters/symbols in char_name_letters are authorized
		for( i = 0; i < NAME_LENGTH && name[i]; i++ )
			if( strchr(char_name_letters, name[i]) == NULL )
				return false;
	} else
	if( char_name_option == 2 )
	{// letters/symbols in char_name_letters are forbidden
		for( i = 0; i < NAME_LENGTH && name[i]; i++ )
			if( strchr(char_name_letters, name[i]) != NULL )
				return false;
	}

	return true;
}

/*==========================================
 * Inter Packets
 *------------------------------------------*/
static int inter_homunculus_parse_frommap(int fd)
{
	unsigned short cmd = RFIFOW(fd,0);

	switch (cmd) {
		case 0x3090: mapif->parse_homunculus_create(fd, RFIFOW(fd,2), RFIFOL(fd,4), RFIFOP(fd,8)); break;
		case 0x3091: mapif->parse_homunculus_load  (fd, RFIFOL(fd,2), RFIFOL(fd,6)); break;
		case 0x3092: mapif->parse_homunculus_save  (fd, RFIFOW(fd,2), RFIFOL(fd,4), RFIFOP(fd,8)); break;
		case 0x3093: mapif->parse_homunculus_delete(fd, RFIFOL(fd,2)); break;
		case 0x3094: mapif->parse_homunculus_rename(fd, RFIFOL(fd,2), RFIFOL(fd,6), RFIFOP(fd,10)); break;
		default:
			return 0;
	}
	return 1;
}

void inter_homunculus_defaults(void)
{
	inter_homunculus = &inter_homunculus_s;

	inter_homunculus->sql_init = inter_homunculus_sql_init;
	inter_homunculus->sql_final = inter_homunculus_sql_final;
	inter_homunculus->parse_frommap = inter_homunculus_parse_frommap;

	inter_homunculus->create = inter_homunculus_create;
	inter_homunculus->save = inter_homunculus_save;
	inter_homunculus->load = inter_homunculus_load;
	inter_homunculus->delete = inter_homunculus_delete;
	inter_homunculus->rename = inter_homunculus_rename;
}
