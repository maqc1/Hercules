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

#include "inter.h"

#include "char/char.h"
#include "char/geoip.h"
#include "char/int_adventurer_agency.h"
#include "char/int_auction.h"
#include "char/int_clan.h"
#include "char/int_elemental.h"
#include "char/int_guild.h"
#include "char/int_homun.h"
#include "char/int_mail.h"
#include "char/int_mercenary.h"
#include "char/int_party.h"
#include "char/int_pet.h"
#include "char/int_quest.h"
#include "char/int_rodex.h"
#include "char/int_storage.h"
#include "char/int_achievement.h"
#include "char/mapif.h"
#include "common/cbasetypes.h"
#include "common/conf.h"
#include "common/db.h"
#include "common/memmgr.h"
#include "common/mmo.h"
#include "common/msgtable.h"
#include "common/nullpo.h"
#include "common/showmsg.h"
#include "common/socket.h"
#include "common/sql.h"
#include "common/strlib.h"
#include "common/timer.h"
#include "common/packets.h"

#include <stdio.h>
#include <stdlib.h>

static struct inter_interface inter_s;
struct inter_interface *inter;

static int char_server_port = 3306;
static char char_server_ip[32] = "127.0.0.1";
static char char_server_id[32] = "ragnarok";
static char char_server_pw[100] = "ragnarok";
static char char_server_db[32] = "ragnarok";
static char default_codepage[32] = ""; //Feature by irmin.

int party_share_level = 10;

#define MAX_JOB_NAMES 150
static char *msg_table[MAX_JOB_NAMES]; //  messages 550 ~ 699 are job names

static const char *inter_msg_txt(int msg_number)
{
	msg_number -= 550;
	if (msg_number >= 0 && msg_number < MAX_JOB_NAMES &&
	    msg_table[msg_number] != NULL && msg_table[msg_number][0] != '\0')
		return msg_table[msg_number];

	return "Unknown";
}

/**
 * Reads Message Data.
 *
 * This is a modified version of the mapserver's inter->msg_config_read to
 * only read messages with IDs between 550 and 550+MAX_JOB_NAMES.
 *
 * @param[in] cfg_name       configuration filename to read.
 * @param[in] allow_override whether to allow duplicate message IDs to override the original value.
 * @return success state.
 */
static bool inter_msg_config_read(const char *cfg_name, bool allow_override)
{
	int msg_number;
	char line[1024], w1[1024], w2[1024];
	FILE *fp;
	static int called = 1;

	nullpo_ret(cfg_name);
	if ((fp = fopen(cfg_name, "r")) == NULL) {
		ShowError("Messages file not found: %s\n", cfg_name);
		return 1;
	}

	if ((--called) == 0)
		memset(msg_table, 0, sizeof(msg_table[0]) * MAX_JOB_NAMES);

	while(fgets(line, sizeof(line), fp) ) {
		if (line[0] == '/' && line[1] == '/')
			continue;
		if (sscanf(line, "%1023[^:]: %1023[^\r\n]", w1, w2) != 2)
			continue;

		if (strcmpi(w1, "import") == 0)
			inter->msg_config_read(w2, true);
		else {
			msg_number = atoi(w1);
			if( msg_number < 550 || msg_number > (550+MAX_JOB_NAMES) )
				continue;
			msg_number -= 550;
			if (msg_number >= 0 && msg_number < MAX_JOB_NAMES) {
				if (msg_table[msg_number] != NULL) {
					if (!allow_override) {
						ShowError("Duplicate message: ID '%d' was already used for '%s'. Message '%s' will be ignored.\n",
						          msg_number, w2, msg_table[msg_number]);
						continue;
					}
					aFree(msg_table[msg_number]);
				}
				msg_table[msg_number] = (char *)aMalloc((strlen(w2) + 1)*sizeof (char));
				strcpy(msg_table[msg_number],w2);
			}
		}
	}

	fclose(fp);

	return 0;
}

/*==========================================
 * Cleanup Message Data
 *------------------------------------------*/
static void inter_do_final_msg(void)
{
	int i;
	for (i = 0; i < MAX_JOB_NAMES; i++)
		aFree(msg_table[i]);
}

static const char *inter_job_name(int class)
{
	switch (class) {
#define JOB_ENUM_VALUE(name, id, msgtbl) case JOB_ ## name: return inter->msg_txt(MSGTBL_ ## msgtbl);
#include "common/class.h"
#include "common/class_hidden.h"
#include "common/class_special.h"
#undef JOB_ENUM_VALUE

		default:
			return inter->msg_txt(MSGTBL_UNKNOWN_JOB);
	}
}

/**
 * Argument-list version of inter_msg_to_fd
 * @see inter_msg_to_fd
 */
static void inter_vmsg_to_fd(int fd, int u_fd, int aid, char *msg, va_list ap) __attribute__((format(printf, 4, 0)));
static void inter_vmsg_to_fd(int fd, int u_fd, int aid, char *msg, va_list ap)
{
	char msg_out[512];
	va_list apcopy;
	int len = 1;/* yes we start at 1 */

	nullpo_retv(msg);
	va_copy(apcopy, ap);
	len += vsnprintf(msg_out, 512, msg, apcopy);
	va_end(apcopy);

	WFIFOHEAD(fd,12 + len);

	WFIFOW(fd,0) = 0x3807;
	WFIFOW(fd,2) = 12 + (unsigned short)len;
	WFIFOL(fd,4) = u_fd;
	WFIFOL(fd,8) = aid;
	safestrncpy(WFIFOP(fd,12), msg_out, len);

	WFIFOSET(fd,12 + len);

	return;
}

/**
 * Sends a message to map server (fd) to a user (u_fd) although we use fd we
 * keep aid for safe-check.
 * @param fd   Mapserver's fd
 * @param u_fd Recipient's fd
 * @param aid  Recipient's expected for sanity checks on the mapserver
 * @param msg  Message format string
 * @param ...  Additional parameters for (v)sprinf
 */
static void inter_msg_to_fd(int fd, int u_fd, int aid, char *msg, ...) __attribute__((format(printf, 4, 5)));
static void inter_msg_to_fd(int fd, int u_fd, int aid, char *msg, ...)
{
	va_list ap;
	va_start(ap,msg);
	inter->vmsg_to_fd(fd, u_fd, aid, msg, ap);
	va_end(ap);
}

/* [Dekamaster/Nightroad] */
static void inter_accinfo(int u_fd, int aid, int castergroup, const char *query, int map_fd)
{
	char query_esq[NAME_LENGTH*2+1];
	int account_id;
	char *data;

	SQL->EscapeString(inter->sql_handle, query_esq, query);

	account_id = atoi(query);

	if (account_id < START_ACCOUNT_NUM) {
		// is string
		if ( SQL_ERROR == SQL->Query(inter->sql_handle, "SELECT `account_id`,`name`,`class`,`base_level`,`job_level`,`online` FROM `%s` WHERE `name` LIKE '%s' LIMIT 10", char_db, query_esq)
				|| SQL->NumRows(inter->sql_handle) == 0 ) {
			if( SQL->NumRows(inter->sql_handle) == 0 ) {
				inter->msg_to_fd(map_fd, u_fd, aid, "No matches were found for your criteria, '%s'",query);
			} else {
				Sql_ShowDebug(inter->sql_handle);
				inter->msg_to_fd(map_fd, u_fd, aid, "An error occurred, bother your admin about it.");
			}
			SQL->FreeResult(inter->sql_handle);
			return;
		} else {
			if( SQL->NumRows(inter->sql_handle) == 1 ) {//we found a perfect match
				SQL->NextRow(inter->sql_handle);
				SQL->GetData(inter->sql_handle, 0, &data, NULL); account_id = atoi(data);
				SQL->FreeResult(inter->sql_handle);
			} else {// more than one, listing... [Dekamaster/Nightroad]
				inter->msg_to_fd(map_fd, u_fd, aid, "Your query returned the following %d results, please be more specific...",(int)SQL->NumRows(inter->sql_handle));
				while ( SQL_SUCCESS == SQL->NextRow(inter->sql_handle) ) {
					int class;
					int base_level, job_level, online;
					char name[NAME_LENGTH];

					SQL->GetData(inter->sql_handle, 0, &data, NULL); account_id = atoi(data);
					SQL->GetData(inter->sql_handle, 1, &data, NULL); safestrncpy(name, data, sizeof(name));
					SQL->GetData(inter->sql_handle, 2, &data, NULL); class = atoi(data);
					SQL->GetData(inter->sql_handle, 3, &data, NULL); base_level = atoi(data);
					SQL->GetData(inter->sql_handle, 4, &data, NULL); job_level = atoi(data);
					SQL->GetData(inter->sql_handle, 5, &data, NULL); online = atoi(data);

					inter->msg_to_fd(map_fd, u_fd, aid, "[AID: %d] %s | %s | Level: %d/%d | %s", account_id, name, inter->job_name(class), base_level, job_level, online?"Online":"Offline");
				}
				SQL->FreeResult(inter->sql_handle);
				return;
			}
		}
	}

	/* it will only get here if we have a single match */
	/* and we will send packet with account id to login server asking for account info */
	if( account_id ) {
		mapif->on_parse_accinfo(account_id, u_fd, aid, castergroup, map_fd);
	}

	return;
}

static void inter_accinfo2(bool success, int map_fd, int u_fd, int u_aid, int account_id, const char *userid, const char *user_pass,
		const char *email, const char *last_ip, const char *lastlogin, const char *pin_code, const char *birthdate,
		int group_id, int logincount, int state)
{
	nullpo_retv(userid);
	nullpo_retv(user_pass);
	nullpo_retv(email);
	nullpo_retv(last_ip);
	nullpo_retv(lastlogin);
	nullpo_retv(birthdate);
	if (map_fd <= 0 || !sockt->session_is_active(map_fd))
		return; // check if we have a valid fd

	if (!success) {
		inter->msg_to_fd(map_fd, u_fd, u_aid, "No account with ID '%d' was found.", account_id);
		return;
	}

	inter->msg_to_fd(map_fd, u_fd, u_aid, "-- Account %d --", account_id);
	inter->msg_to_fd(map_fd, u_fd, u_aid, "User: %s | GM Group: %d | State: %d", userid, group_id, state);

// enable this if you really know what you doing.
#if 0
	if (*user_pass != '\0') { /* password is only received if your gm level is greater than the one you're searching for */
		if (pin_code && *pin_code != '\0')
			inter->msg_to_fd(map_fd, u_fd, u_aid, "Password: %s (PIN:%s)", user_pass, pin_code);
		else
			inter->msg_to_fd(map_fd, u_fd, u_aid, "Password: %s", user_pass );
	}
#endif

	inter->msg_to_fd(map_fd, u_fd, u_aid, "Account e-mail: %s | Birthdate: %s", email, birthdate);
	inter->msg_to_fd(map_fd, u_fd, u_aid, "Last IP: %s (%s)", last_ip, geoip->getcountry(sockt->str2ip(last_ip)));
	inter->msg_to_fd(map_fd, u_fd, u_aid, "This user has logged %d times, the last time were at %s", logincount, lastlogin);
	inter->msg_to_fd(map_fd, u_fd, u_aid, "-- Character Details --");

	if ( SQL_ERROR == SQL->Query(inter->sql_handle, "SELECT `char_id`, `name`, `char_num`, `class`, `base_level`, `job_level`, `online` "
	                                         "FROM `%s` WHERE `account_id` = '%d' ORDER BY `char_num` LIMIT %d", char_db, account_id, MAX_CHARS)
	  || SQL->NumRows(inter->sql_handle) == 0 ) {
		if (SQL->NumRows(inter->sql_handle) == 0) {
			inter->msg_to_fd(map_fd, u_fd, u_aid, "This account doesn't have characters.");
		} else {
			inter->msg_to_fd(map_fd, u_fd, u_aid, "An error occurred, bother your admin about it.");
			Sql_ShowDebug(inter->sql_handle);
		}
	} else {
		while ( SQL_SUCCESS == SQL->NextRow(inter->sql_handle) ) {
			char *data;
			int char_id, class;
			int char_num, base_level, job_level, online;
			char name[NAME_LENGTH];

			SQL->GetData(inter->sql_handle, 0, &data, NULL); char_id = atoi(data);
			SQL->GetData(inter->sql_handle, 1, &data, NULL); safestrncpy(name, data, sizeof(name));
			SQL->GetData(inter->sql_handle, 2, &data, NULL); char_num = atoi(data);
			SQL->GetData(inter->sql_handle, 3, &data, NULL); class = atoi(data);
			SQL->GetData(inter->sql_handle, 4, &data, NULL); base_level = atoi(data);
			SQL->GetData(inter->sql_handle, 5, &data, NULL); job_level = atoi(data);
			SQL->GetData(inter->sql_handle, 6, &data, NULL); online = atoi(data);

			inter->msg_to_fd(map_fd, u_fd, u_aid, "[Slot/CID: %d/%d] %s | %s | Level: %d/%d | %s", char_num, char_id, name, inter->job_name(class), base_level, job_level, online?"On":"Off");
		}
	}
	SQL->FreeResult(inter->sql_handle);

	return;
}
/**
 * Handles save reg data from map server and distributes accordingly.
 *
 * @param val either str or int, depending on type
 * @param type false when int, true otherwise
 **/
static void inter_savereg(int account_id, int char_id, const char *key, unsigned int index, intptr_t val, bool is_string)
{
	char val_esq[1000];
	nullpo_retv(key);
	/* to login server we go! */
	if( key[0] == '#' && key[1] == '#' ) {/* global account reg */
		if (sockt->session_is_valid(chr->login_fd))
			chr->global_accreg_to_login_add(key,index,val,is_string);
		else {
			ShowError("Login server unavailable, cant perform update on '%s' variable for AID:%d CID:%d\n",key,account_id,char_id);
		}
	} else if ( key[0] == '#' ) {/* local account reg */
		if( is_string ) {
			if( val ) {
				SQL->EscapeString(inter->sql_handle, val_esq, (char*)val);
				if( SQL_ERROR == SQL->Query(inter->sql_handle, "REPLACE INTO `%s` (`account_id`,`key`,`index`,`value`) VALUES ('%d','%s','%u','%s')", acc_reg_str_db, account_id, key, index, val_esq) )
					Sql_ShowDebug(inter->sql_handle);
			} else {
				if( SQL_ERROR == SQL->Query(inter->sql_handle, "DELETE FROM `%s` WHERE `account_id` = '%d' AND `key` = '%s' AND `index` = '%u' LIMIT 1", acc_reg_str_db, account_id, key, index) )
					Sql_ShowDebug(inter->sql_handle);
			}
		} else {
			if( val ) {
				if( SQL_ERROR == SQL->Query(inter->sql_handle, "REPLACE INTO `%s` (`account_id`,`key`,`index`,`value`) VALUES ('%d','%s','%u','%d')", acc_reg_num_db, account_id, key, index, (int)val) )
					Sql_ShowDebug(inter->sql_handle);
			} else {
				if( SQL_ERROR == SQL->Query(inter->sql_handle, "DELETE FROM `%s` WHERE `account_id` = '%d' AND `key` = '%s' AND `index` = '%u' LIMIT 1", acc_reg_num_db, account_id, key, index) )
					Sql_ShowDebug(inter->sql_handle);
			}
		}
	} else { /* char reg */
		if( is_string ) {
			if( val ) {
				SQL->EscapeString(inter->sql_handle, val_esq, (char*)val);
				if( SQL_ERROR == SQL->Query(inter->sql_handle, "REPLACE INTO `%s` (`char_id`,`key`,`index`,`value`) VALUES ('%d','%s','%u','%s')", char_reg_str_db, char_id, key, index, val_esq) )
					Sql_ShowDebug(inter->sql_handle);
			} else {
				if( SQL_ERROR == SQL->Query(inter->sql_handle, "DELETE FROM `%s` WHERE `char_id` = '%d' AND `key` = '%s' AND `index` = '%u' LIMIT 1", char_reg_str_db, char_id, key, index) )
					Sql_ShowDebug(inter->sql_handle);
			}
		} else {
			if( val ) {
				if( SQL_ERROR == SQL->Query(inter->sql_handle, "REPLACE INTO `%s` (`char_id`,`key`,`index`,`value`) VALUES ('%d','%s','%u','%d')", char_reg_num_db, char_id, key, index, (int)val) )
					Sql_ShowDebug(inter->sql_handle);
			} else {
				if( SQL_ERROR == SQL->Query(inter->sql_handle, "DELETE FROM `%s` WHERE `char_id` = '%d' AND `key` = '%s' AND `index` = '%u' LIMIT 1", char_reg_num_db, char_id, key, index) )
					Sql_ShowDebug(inter->sql_handle);
			}
		}
	}
}

// Load account_reg from sql (type=2)
static int inter_accreg_fromsql(int account_id, int char_id, int fd, int type)
{
	char* data;
	size_t len;
	unsigned int plen = 0;

	switch( type ) {
		case 3: //char reg
			if( SQL_ERROR == SQL->Query(inter->sql_handle, "SELECT `key`, `index`, `value` FROM `%s` WHERE `char_id`='%d'", char_reg_str_db, char_id) )
				Sql_ShowDebug(inter->sql_handle);
			break;
		case 2: //account reg
			if( SQL_ERROR == SQL->Query(inter->sql_handle, "SELECT `key`, `index`, `value` FROM `%s` WHERE `account_id`='%d'", acc_reg_str_db, account_id) )
				Sql_ShowDebug(inter->sql_handle);
			break;
		case 1: //account2 reg
			ShowError("inter->accreg_fromsql: Char server shouldn't handle type 1 registry values (##). That is the login server's work!\n");
			return 0;
		default:
			ShowError("inter->accreg_fromsql: Invalid type %d\n", type);
			return 0;
	}

	WFIFOHEAD(fd, 60000 + 300);
	WFIFOW(fd, 0) = 0x3804;
	/* 0x2 = length, set prior to being sent */
	WFIFOL(fd, 4) = account_id;
	WFIFOL(fd, 8) = char_id;
	WFIFOB(fd, 12) = 0;/* var type (only set when all vars have been sent, regardless of type) */
	WFIFOB(fd, 13) = 1;/* is string type */
	WFIFOW(fd, 14) = 0;/* count */
	plen = 16;

	/**
	 * Vessel!
	 *
	 * str type
	 * { keyLength(B), key(<keyLength>), index(L), valLength(B), val(<valLength>) }
	 **/
	while ( SQL_SUCCESS == SQL->NextRow(inter->sql_handle) ) {
		SQL->GetData(inter->sql_handle, 0, &data, NULL);
		len = strlen(data)+1;

		WFIFOB(fd, plen) = (unsigned char)len;/* won't be higher; the column size is 32 */
		plen += 1;

		safestrncpy(WFIFOP(fd,plen), data, len);
		plen += len;

		SQL->GetData(inter->sql_handle, 1, &data, NULL);

		WFIFOL(fd, plen) = (unsigned int)atol(data);
		plen += 4;

		SQL->GetData(inter->sql_handle, 2, &data, NULL);
		len = strlen(data);

		WFIFOB(fd, plen) = (unsigned char)len; // Won't be higher; the column size is 255.
		plen += 1;

		safestrncpy(WFIFOP(fd, plen), data, len + 1);
		plen += len + 1;

		WFIFOW(fd, 14) += 1;

		if( plen > 60000 ) {
			WFIFOW(fd, 2) = plen;
			WFIFOSET(fd, plen);

			/* prepare follow up */
			WFIFOHEAD(fd, 60000 + 300);
			WFIFOW(fd, 0) = 0x3804;
			/* 0x2 = length, set prior to being sent */
			WFIFOL(fd, 4) = account_id;
			WFIFOL(fd, 8) = char_id;
			WFIFOB(fd, 12) = 0;/* var type (only set when all vars have been sent, regardless of type) */
			WFIFOB(fd, 13) = 1;/* is string type */
			WFIFOW(fd, 14) = 0;/* count */
			plen = 16;
		}
	}

	/* mark & go. */
	WFIFOW(fd, 2) = plen;
	WFIFOSET(fd, plen);

	SQL->FreeResult(inter->sql_handle);

	switch( type ) {
		case 3: //char reg
			if( SQL_ERROR == SQL->Query(inter->sql_handle, "SELECT `key`, `index`, `value` FROM `%s` WHERE `char_id`='%d'", char_reg_num_db, char_id) )
				Sql_ShowDebug(inter->sql_handle);
			break;
		case 2: //account reg
			if( SQL_ERROR == SQL->Query(inter->sql_handle, "SELECT `key`, `index`, `value` FROM `%s` WHERE `account_id`='%d'", acc_reg_num_db, account_id) )
				Sql_ShowDebug(inter->sql_handle);
			break;
#if 0 // This is already checked above.
		case 1: //account2 reg
			ShowError("inter->accreg_fromsql: Char server shouldn't handle type 1 registry values (##). That is the login server's work!\n");
			return 0;
#endif // 0
	}

	WFIFOHEAD(fd, 60000 + 300);
	WFIFOW(fd, 0) = 0x3804;
	/* 0x2 = length, set prior to being sent */
	WFIFOL(fd, 4) = account_id;
	WFIFOL(fd, 8) = char_id;
	WFIFOB(fd, 12) = 0;/* var type (only set when all vars have been sent, regardless of type) */
	WFIFOB(fd, 13) = 0;/* is int type */
	WFIFOW(fd, 14) = 0;/* count */
	plen = 16;

	/**
	 * Vessel!
	 *
	 * int type
	 * { keyLength(B), key(<keyLength>), index(L), value(L) }
	 **/
	while ( SQL_SUCCESS == SQL->NextRow(inter->sql_handle) ) {
		SQL->GetData(inter->sql_handle, 0, &data, NULL);
		len = strlen(data)+1;

		WFIFOB(fd, plen) = (unsigned char)len;/* won't be higher; the column size is 32 */
		plen += 1;

		safestrncpy(WFIFOP(fd,plen), data, len);
		plen += len;

		SQL->GetData(inter->sql_handle, 1, &data, NULL);

		WFIFOL(fd, plen) = (unsigned int)atol(data);
		plen += 4;

		SQL->GetData(inter->sql_handle, 2, &data, NULL);

		WFIFOL(fd, plen) = atoi(data);
		plen += 4;

		WFIFOW(fd, 14) += 1;

		if( plen > 60000 ) {
			WFIFOW(fd, 2) = plen;
			WFIFOSET(fd, plen);

			/* prepare follow up */
			WFIFOHEAD(fd, 60000 + 300);
			WFIFOW(fd, 0) = 0x3804;
			/* 0x2 = length, set prior to being sent */
			WFIFOL(fd, 4) = account_id;
			WFIFOL(fd, 8) = char_id;
			WFIFOB(fd, 12) = 0;/* var type (only set when all vars have been sent, regardless of type) */
			WFIFOB(fd, 13) = 0;/* is int type */
			WFIFOW(fd, 14) = 0;/* count */
			plen = 16;
		}
	}

	/* mark as complete & go. */
	WFIFOB(fd, 12) = type;
	WFIFOW(fd, 2) = plen;
	WFIFOSET(fd, plen);

	SQL->FreeResult(inter->sql_handle);
	return 1;
}

/**
 * Reads the 'inter_configuration/log' config entry and initializes required variables.
 *
 * @param filename Path to configuration file (used in error and warning messages).
 * @param config   The current config being parsed.
 * @param imported Whether the current config is imported from another file.
 *
 * @retval false in case of error.
 */
static bool inter_config_read_log(const char *filename, const struct config_t *config, bool imported)
{
	const struct config_setting_t *setting = NULL;

	nullpo_retr(false, filename);
	nullpo_retr(false, config);

	if ((setting = libconfig->lookup(config, "inter_configuration/log")) == NULL) {
		if (imported)
			return true;
		ShowError("sql_config_read: inter_configuration/log was not found in %s!\n", filename);
		return false;
	}

	libconfig->setting_lookup_bool_real(setting, "log_inter", &inter->enable_logs);

	return true;
}

/**
 * Reads the 'char_configuration/sql_connection' config entry and initializes required variables.
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

	if ((setting = libconfig->lookup(config, "char_configuration/sql_connection")) == NULL) {
		if (imported)
			return true;
		ShowError("char_config_read: char_configuration/sql_connection was not found in %s!\n", filename);
		ShowWarning("inter_config_read_connection: Defaulting sql_connection...\n");
		return false;
	}

	libconfig->setting_lookup_int(setting, "db_port", &char_server_port);
	libconfig->setting_lookup_mutable_string(setting, "db_hostname", char_server_ip, sizeof(char_server_ip));
	libconfig->setting_lookup_mutable_string(setting, "db_username", char_server_id, sizeof(char_server_id));
	libconfig->setting_lookup_mutable_string(setting, "db_password", char_server_pw, sizeof(char_server_pw));
	libconfig->setting_lookup_mutable_string(setting, "db_database", char_server_db, sizeof(char_server_db));
	libconfig->setting_lookup_mutable_string(setting, "default_codepage", default_codepage, sizeof(default_codepage));

	return true;
}

/**
 * Reads the 'inter_configuration' config file and initializes required variables.
 *
 * @param filename Path to configuration file
 * @param imported Whether the current config is from an imported file.
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
	libconfig->setting_lookup_int(setting, "party_share_level", &party_share_level);

	if (!inter->config_read_log(filename, &config, imported))
		retval = false;

	ShowInfo("Done reading %s.\n", filename);

	// import should overwrite any previous configuration, so it should be called last
	if (libconfig->lookup_string(&config, "import", &import) == CONFIG_TRUE) {
		if (strcmp(import, filename) == 0 || strcmp(import, chr->INTER_CONF_NAME) == 0) {
			ShowWarning("inter_config_read: Loop detected in %s! Skipping 'import'...\n", filename);
		} else {
			if (!inter->config_read(import, true))
				retval = false;
		}
	}

	libconfig->destroy(&config);
	return retval;
}

/**
 * Save interlog into sql (arglist version)
 * @see inter_log
 */
static int inter_vlog(char *fmt, va_list ap) __attribute__((format(printf, 1, 0)));
static int inter_vlog(char *fmt, va_list ap)
{
	char str[255];
	char esc_str[sizeof(str)*2+1];// escaped str
	va_list apcopy;

	va_copy(apcopy, ap);
	vsnprintf(str, sizeof(str), fmt, apcopy);
	va_end(apcopy);

	SQL->EscapeStringLen(inter->sql_handle, esc_str, str, strnlen(str, sizeof(str)));
	if( SQL_ERROR == SQL->Query(inter->sql_handle, "INSERT INTO `%s` (`time`, `log`) VALUES (NOW(),  '%s')", interlog_db, esc_str) )
		Sql_ShowDebug(inter->sql_handle);

	return 0;
}

/**
 * Save interlog into sql
 * @param fmt Message's format string
 * @param ... Additional (printf-like) arguments
 * @return Always 0 // FIXME
 */
static int inter_log(char *fmt, ...) __attribute__((format(printf, 1, 2)));
static int inter_log(char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap,fmt);
	ret = inter->vlog(fmt, ap);
	va_end(ap);

	return ret;
}

// initialize
static int inter_init_sql(const char *file)
{
	inter->config_read(file, false);

	//DB connection initialized
	inter->sql_handle = SQL->Malloc();
	ShowInfo("Connect Character DB server.... (Character Server)\n");
	if( SQL_ERROR == SQL->Connect(inter->sql_handle, char_server_id, char_server_pw, char_server_ip, (uint16)char_server_port, char_server_db) )
	{
		Sql_ShowDebug(inter->sql_handle);
		SQL->Free(inter->sql_handle);
		exit(EXIT_FAILURE);
	}

	if( *default_codepage ) {
		if( SQL_ERROR == SQL->SetEncoding(inter->sql_handle, default_codepage) )
			Sql_ShowDebug(inter->sql_handle);
	}

	inter_guild->sql_init();
	inter_storage->sql_init();
	inter_party->sql_init();
	inter_pet->sql_init();
	inter_homunculus->sql_init();
	inter_mercenary->sql_init();
	inter_elemental->sql_init();
	inter_mail->sql_init();
	inter_auction->sql_init();
	inter_rodex->sql_init();
	inter_achievement->sql_init();

	geoip->init();
	inter->msg_config_read("conf/messages.conf", false);
	return 0;
}

// finalize
static void inter_final(void)
{
	inter_guild->sql_final();
	inter_storage->sql_final();
	inter_party->sql_final();
	inter_pet->sql_final();
	inter_homunculus->sql_final();
	inter_mercenary->sql_final();
	inter_elemental->sql_final();
	inter_mail->sql_final();
	inter_auction->sql_final();
	inter_rodex->sql_final();
	inter_achievement->sql_final();

	geoip->final(true);
	inter->do_final_msg();
	return;
}

static int inter_mapif_init(int fd)
{
	return 0;
}

//--------------------------------------------------------

/// Returns the length of the next complete packet to process,
/// or 0 if no complete packet exists in the queue.
///
/// @param length The minimum allowed length, or -1 for dynamic lookup
static int inter_check_length(int fd, int length)
{
	if( length == -1 )
	{// variable-length packet
		if( RFIFOREST(fd) < 4 )
			return 0;
		length = RFIFOW(fd,2);
	}

	if( (int)RFIFOREST(fd) < length )
		return 0;

	return length;
}

static int inter_parse_frommap(int fd)
{
	int cmd;
	int len = 0;
	cmd = RFIFOW(fd,0);
	// Check is valid packet entry
	if (cmd < MIN_INTER_PACKET_DB || cmd >= MAX_INTER_PACKET_DB || packets->inter_db[cmd - MIN_INTER_PACKET_DB] == 0)
		return 0;

	// Check packet length
	if ((len = inter->check_length(fd, packets->inter_db[cmd - MIN_INTER_PACKET_DB])) == 0)
		return 2;

	switch(cmd) {
	case 0x3004: mapif->parse_Registry(fd); break;
	case 0x3005: mapif->parse_RegistryRequest(fd); break;
	case 0x3006: mapif->parse_NameChangeRequest(fd); break;
	case 0x3007: mapif->parse_accinfo(fd); break;
	default:
		if(  inter_party->parse_frommap(fd)
		  || inter_guild->parse_frommap(fd)
		  || inter_storage->parse_frommap(fd)
		  || inter_pet->parse_frommap(fd)
		  || inter_homunculus->parse_frommap(fd)
		  || inter_mercenary->parse_frommap(fd)
		  || inter_elemental->parse_frommap(fd)
		  || inter_mail->parse_frommap(fd)
		  || inter_auction->parse_frommap(fd)
		  || inter_quest->parse_frommap(fd)
		  || inter_rodex->parse_frommap(fd)
		  || inter_clan->parse_frommap(fd)
		  || inter_achievement->parse_frommap(fd)
		  || inter_adventurer_agency->parse_frommap(fd)
		   )
			break;
		else
			return 0;
	}

	RFIFOSKIP(fd, len);
	return 1;
}

void inter_defaults(void)
{
	inter = &inter_s;

	inter->enable_logs = true;
	inter->sql_handle = NULL;

	inter->msg_txt = inter_msg_txt;
	inter->msg_config_read = inter_msg_config_read;
	inter->do_final_msg = inter_do_final_msg;
	inter->job_name = inter_job_name;
	inter->vmsg_to_fd = inter_vmsg_to_fd;
	inter->msg_to_fd = inter_msg_to_fd;
	inter->savereg = inter_savereg;
	inter->accreg_fromsql = inter_accreg_fromsql;
	inter->config_read = inter_config_read;
	inter->vlog = inter_vlog;
	inter->log = inter_log;
	inter->init_sql = inter_init_sql;
	inter->mapif_init = inter_mapif_init;
	inter->check_length = inter_check_length;
	inter->parse_frommap = inter_parse_frommap;
	inter->final = inter_final;
	inter->config_read_log = inter_config_read_log;
	inter->config_read_connection = inter_config_read_connection;
	inter->accinfo = inter_accinfo;
	inter->accinfo2 = inter_accinfo2;
}
