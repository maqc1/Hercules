/**
 * This file is part of Hercules.
 * http://herc.ws - http://github.com/HerculesWS/Hercules
 *
 * Copyright (C) 2014-2025 Hercules Dev Team
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

/*
 * NOTE: This file was auto-generated and should never be manually edited,
 *       as it will get overwritten.
 */

/* GENERATED FILE DO NOT EDIT */

#ifndef HPM_DATA_CHECK_H
#define HPM_DATA_CHECK_H

#if !defined(HPMHOOKGEN)
#include "common/HPMSymbols.inc.h"
#endif // ! HPMHOOKGEN
#ifdef HPM_SYMBOL
#undef HPM_SYMBOL
#endif // HPM_SYMBOL

HPExport const struct s_HPMDataCheck HPMDataCheck[] = {
	#ifdef API_ACLIF_H
		{ "aclif_interface", sizeof(struct aclif_interface), SERVER_TYPE_UNKNOWN },
		{ "char_server_data", sizeof(struct char_server_data), SERVER_TYPE_UNKNOWN },
		{ "online_api_login_data", sizeof(struct online_api_login_data), SERVER_TYPE_UNKNOWN },
	#else
		#define API_ACLIF_H
	#endif // API_ACLIF_H
	#ifdef API_ALOGINIF_H
		{ "aloginif_interface", sizeof(struct aloginif_interface), SERVER_TYPE_UNKNOWN },
	#else
		#define API_ALOGINIF_H
	#endif // API_ALOGINIF_H
	#ifdef API_APISESSIONDATA_H
		{ "api_session_data", sizeof(struct api_session_data), SERVER_TYPE_UNKNOWN },
	#else
		#define API_APISESSIONDATA_H
	#endif // API_APISESSIONDATA_H
	#ifdef API_API_H
		{ "api_interface", sizeof(struct api_interface), SERVER_TYPE_UNKNOWN },
	#else
		#define API_API_H
	#endif // API_API_H
	#ifdef API_HTTPHANDLER_H
		{ "HttpHandler", sizeof(struct HttpHandler), SERVER_TYPE_UNKNOWN },
	#else
		#define API_HTTPHANDLER_H
	#endif // API_HTTPHANDLER_H
	#ifdef API_HTTPPARSER_H
		{ "httpparser_interface", sizeof(struct httpparser_interface), SERVER_TYPE_UNKNOWN },
	#else
		#define API_HTTPPARSER_H
	#endif // API_HTTPPARSER_H
	#ifdef API_HTTPSENDER_H
		{ "httpsender_interface", sizeof(struct httpsender_interface), SERVER_TYPE_UNKNOWN },
	#else
		#define API_HTTPSENDER_H
	#endif // API_HTTPSENDER_H
	#ifdef API_IMAGEPARSER_H
		{ "gif_user_data", sizeof(struct gif_user_data), SERVER_TYPE_UNKNOWN },
		{ "imageparser_interface", sizeof(struct imageparser_interface), SERVER_TYPE_UNKNOWN },
	#else
		#define API_IMAGEPARSER_H
	#endif // API_IMAGEPARSER_H
	#ifdef API_JSONPARSER_H
		{ "jsonparser_interface", sizeof(struct jsonparser_interface), SERVER_TYPE_UNKNOWN },
	#else
		#define API_JSONPARSER_H
	#endif // API_JSONPARSER_H
	#ifdef API_JSONWRITER_H
		{ "jsonwriter_interface", sizeof(struct jsonwriter_interface), SERVER_TYPE_UNKNOWN },
	#else
		#define API_JSONWRITER_H
	#endif // API_JSONWRITER_H
	#ifdef API_MIMEPART_H
		{ "MimePart", sizeof(struct MimePart), SERVER_TYPE_UNKNOWN },
	#else
		#define API_MIMEPART_H
	#endif // API_MIMEPART_H
	#ifdef CHAR_CAPIIF_H
		{ "capiif_interface", sizeof(struct capiif_interface), SERVER_TYPE_CHAR },
	#else
		#define CHAR_CAPIIF_H
	#endif // CHAR_CAPIIF_H
	#ifdef CHAR_CHAR_H
		{ "char_auth_node", sizeof(struct char_auth_node), SERVER_TYPE_CHAR },
		{ "char_interface", sizeof(struct char_interface), SERVER_TYPE_CHAR },
		{ "char_session_data", sizeof(struct char_session_data), SERVER_TYPE_CHAR },
		{ "mmo_map_server", sizeof(struct mmo_map_server), SERVER_TYPE_CHAR },
		{ "online_char_data", sizeof(struct online_char_data), SERVER_TYPE_CHAR },
		{ "online_char_data2", sizeof(struct online_char_data2), SERVER_TYPE_CHAR },
	#else
		#define CHAR_CHAR_H
	#endif // CHAR_CHAR_H
	#ifdef CHAR_GEOIP_H
		{ "geoip_interface", sizeof(struct geoip_interface), SERVER_TYPE_CHAR },
		{ "s_geoip", sizeof(struct s_geoip), SERVER_TYPE_CHAR },
	#else
		#define CHAR_GEOIP_H
	#endif // CHAR_GEOIP_H
	#ifdef CHAR_INTER_H
		{ "inter_interface", sizeof(struct inter_interface), SERVER_TYPE_CHAR },
	#else
		#define CHAR_INTER_H
	#endif // CHAR_INTER_H
	#ifdef CHAR_INT_ACHIEVEMENT_H
		{ "inter_achievement_interface", sizeof(struct inter_achievement_interface), SERVER_TYPE_CHAR },
	#else
		#define CHAR_INT_ACHIEVEMENT_H
	#endif // CHAR_INT_ACHIEVEMENT_H
	#ifdef CHAR_INT_ADVENTURER_AGENCY_H
		{ "inter_adventurer_agency_interface", sizeof(struct inter_adventurer_agency_interface), SERVER_TYPE_CHAR },
	#else
		#define CHAR_INT_ADVENTURER_AGENCY_H
	#endif // CHAR_INT_ADVENTURER_AGENCY_H
	#ifdef CHAR_INT_AUCTION_H
		{ "inter_auction_interface", sizeof(struct inter_auction_interface), SERVER_TYPE_CHAR },
	#else
		#define CHAR_INT_AUCTION_H
	#endif // CHAR_INT_AUCTION_H
	#ifdef CHAR_INT_CLAN_H
		{ "inter_clan_interface", sizeof(struct inter_clan_interface), SERVER_TYPE_CHAR },
	#else
		#define CHAR_INT_CLAN_H
	#endif // CHAR_INT_CLAN_H
	#ifdef CHAR_INT_ELEMENTAL_H
		{ "inter_elemental_interface", sizeof(struct inter_elemental_interface), SERVER_TYPE_CHAR },
	#else
		#define CHAR_INT_ELEMENTAL_H
	#endif // CHAR_INT_ELEMENTAL_H
	#ifdef CHAR_INT_GUILD_H
		{ "inter_guild_interface", sizeof(struct inter_guild_interface), SERVER_TYPE_CHAR },
	#else
		#define CHAR_INT_GUILD_H
	#endif // CHAR_INT_GUILD_H
	#ifdef CHAR_INT_HOMUN_H
		{ "inter_homunculus_interface", sizeof(struct inter_homunculus_interface), SERVER_TYPE_CHAR },
	#else
		#define CHAR_INT_HOMUN_H
	#endif // CHAR_INT_HOMUN_H
	#ifdef CHAR_INT_MAIL_H
		{ "inter_mail_interface", sizeof(struct inter_mail_interface), SERVER_TYPE_CHAR },
	#else
		#define CHAR_INT_MAIL_H
	#endif // CHAR_INT_MAIL_H
	#ifdef CHAR_INT_MERCENARY_H
		{ "inter_mercenary_interface", sizeof(struct inter_mercenary_interface), SERVER_TYPE_CHAR },
	#else
		#define CHAR_INT_MERCENARY_H
	#endif // CHAR_INT_MERCENARY_H
	#ifdef CHAR_INT_PARTY_H
		{ "inter_party_interface", sizeof(struct inter_party_interface), SERVER_TYPE_CHAR },
	#else
		#define CHAR_INT_PARTY_H
	#endif // CHAR_INT_PARTY_H
	#ifdef CHAR_INT_PET_H
		{ "inter_pet_interface", sizeof(struct inter_pet_interface), SERVER_TYPE_CHAR },
	#else
		#define CHAR_INT_PET_H
	#endif // CHAR_INT_PET_H
	#ifdef CHAR_INT_QUEST_H
		{ "inter_quest_interface", sizeof(struct inter_quest_interface), SERVER_TYPE_CHAR },
	#else
		#define CHAR_INT_QUEST_H
	#endif // CHAR_INT_QUEST_H
	#ifdef CHAR_INT_RODEX_H
		{ "inter_rodex_interface", sizeof(struct inter_rodex_interface), SERVER_TYPE_CHAR },
	#else
		#define CHAR_INT_RODEX_H
	#endif // CHAR_INT_RODEX_H
	#ifdef CHAR_INT_STORAGE_H
		{ "inter_storage_interface", sizeof(struct inter_storage_interface), SERVER_TYPE_CHAR },
	#else
		#define CHAR_INT_STORAGE_H
	#endif // CHAR_INT_STORAGE_H
	#ifdef CHAR_INT_USERCONFIG_H
		{ "inter_userconfig_dbs", sizeof(struct inter_userconfig_dbs), SERVER_TYPE_CHAR },
		{ "inter_userconfig_interface", sizeof(struct inter_userconfig_interface), SERVER_TYPE_CHAR },
	#else
		#define CHAR_INT_USERCONFIG_H
	#endif // CHAR_INT_USERCONFIG_H
	#ifdef CHAR_LOGINIF_H
		{ "loginif_interface", sizeof(struct loginif_interface), SERVER_TYPE_CHAR },
	#else
		#define CHAR_LOGINIF_H
	#endif // CHAR_LOGINIF_H
	#ifdef CHAR_MAPIF_H
		{ "mapif_interface", sizeof(struct mapif_interface), SERVER_TYPE_CHAR },
	#else
		#define CHAR_MAPIF_H
	#endif // CHAR_MAPIF_H
	#ifdef CHAR_PINCODE_H
		{ "pincode_interface", sizeof(struct pincode_interface), SERVER_TYPE_CHAR },
	#else
		#define CHAR_PINCODE_H
	#endif // CHAR_PINCODE_H
	#ifdef COMMON_APIPACKETS_H
		{ "PACKET_API_PROXY", sizeof(struct PACKET_API_PROXY), SERVER_TYPE_ALL },
		{ "PACKET_API_PROXY0", sizeof(struct PACKET_API_PROXY0), SERVER_TYPE_ALL },
		{ "PACKET_API_PROXY_CHUNKED", sizeof(struct PACKET_API_PROXY_CHUNKED), SERVER_TYPE_ALL },
		{ "PACKET_API_REPLY_emblem_download", sizeof(struct PACKET_API_REPLY_emblem_download), SERVER_TYPE_ALL },
		{ "PACKET_API_REPLY_emblem_upload", sizeof(struct PACKET_API_REPLY_emblem_upload), SERVER_TYPE_ALL },
		{ "PACKET_API_REPLY_party_add", sizeof(struct PACKET_API_REPLY_party_add), SERVER_TYPE_ALL },
		{ "PACKET_API_REPLY_party_del", sizeof(struct PACKET_API_REPLY_party_del), SERVER_TYPE_ALL },
		{ "PACKET_API_REPLY_party_get", sizeof(struct PACKET_API_REPLY_party_get), SERVER_TYPE_ALL },
		{ "PACKET_API_REPLY_party_info", sizeof(struct PACKET_API_REPLY_party_info), SERVER_TYPE_ALL },
		{ "PACKET_API_REPLY_party_list", sizeof(struct PACKET_API_REPLY_party_list), SERVER_TYPE_ALL },
		{ "PACKET_API_REPLY_userconfig_load_emotes", sizeof(struct PACKET_API_REPLY_userconfig_load_emotes), SERVER_TYPE_ALL },
		{ "PACKET_API_REPLY_userconfig_load_hotkeys_tab", sizeof(struct PACKET_API_REPLY_userconfig_load_hotkeys_tab), SERVER_TYPE_ALL },
		{ "PACKET_API_emblem_download", sizeof(struct PACKET_API_emblem_download), SERVER_TYPE_ALL },
		{ "PACKET_API_emblem_download_data", sizeof(struct PACKET_API_emblem_download_data), SERVER_TYPE_ALL },
		{ "PACKET_API_emblem_upload_guild_id", sizeof(struct PACKET_API_emblem_upload_guild_id), SERVER_TYPE_ALL },
		{ "PACKET_API_emblem_upload_guild_id_data", sizeof(struct PACKET_API_emblem_upload_guild_id_data), SERVER_TYPE_ALL },
		{ "PACKET_API_party_add", sizeof(struct PACKET_API_party_add), SERVER_TYPE_ALL },
		{ "PACKET_API_party_add_data", sizeof(struct PACKET_API_party_add_data), SERVER_TYPE_ALL },
		{ "PACKET_API_party_del", sizeof(struct PACKET_API_party_del), SERVER_TYPE_ALL },
		{ "PACKET_API_party_del_data", sizeof(struct PACKET_API_party_del_data), SERVER_TYPE_ALL },
		{ "PACKET_API_party_info", sizeof(struct PACKET_API_party_info), SERVER_TYPE_ALL },
		{ "PACKET_API_party_info_data", sizeof(struct PACKET_API_party_info_data), SERVER_TYPE_ALL },
		{ "PACKET_API_party_list", sizeof(struct PACKET_API_party_list), SERVER_TYPE_ALL },
		{ "PACKET_API_party_list_data", sizeof(struct PACKET_API_party_list_data), SERVER_TYPE_ALL },
		{ "PACKET_API_userconfig_save_emotes", sizeof(struct PACKET_API_userconfig_save_emotes), SERVER_TYPE_ALL },
		{ "PACKET_API_userconfig_save_emotes_data", sizeof(struct PACKET_API_userconfig_save_emotes_data), SERVER_TYPE_ALL },
		{ "PACKET_API_userconfig_save_userhotkey_v2", sizeof(struct PACKET_API_userconfig_save_userhotkey_v2), SERVER_TYPE_ALL },
		{ "PACKET_API_userconfig_save_userhotkey_v2_data", sizeof(struct PACKET_API_userconfig_save_userhotkey_v2_data), SERVER_TYPE_ALL },
		{ "adventuter_agency_entry", sizeof(struct adventuter_agency_entry), SERVER_TYPE_ALL },
		{ "adventuter_agency_page", sizeof(struct adventuter_agency_page), SERVER_TYPE_ALL },
		{ "party_add_data", sizeof(struct party_add_data), SERVER_TYPE_ALL },
		{ "userconfig_emotes", sizeof(struct userconfig_emotes), SERVER_TYPE_ALL },
		{ "userconfig_save_userhotkey_key", sizeof(struct userconfig_save_userhotkey_key), SERVER_TYPE_ALL },
		{ "userconfig_userhotkeys_v2", sizeof(struct userconfig_userhotkeys_v2), SERVER_TYPE_ALL },
	#else
		#define COMMON_APIPACKETS_H
	#endif // COMMON_APIPACKETS_H
	#ifdef COMMON_BASE62_H
		{ "base62_interface", sizeof(struct base62_interface), SERVER_TYPE_ALL },
	#else
		#define COMMON_BASE62_H
	#endif // COMMON_BASE62_H
	#ifdef COMMON_CHARLOGINPACKETS_H
		{ "PACKET_CHARLOGIN_ONLINE_ACCOUNTS", sizeof(struct PACKET_CHARLOGIN_ONLINE_ACCOUNTS), SERVER_TYPE_ALL },
		{ "PACKET_CHARLOGIN_SET_ACCOUNT_ONLINE", sizeof(struct PACKET_CHARLOGIN_SET_ACCOUNT_ONLINE), SERVER_TYPE_ALL },
	#else
		#define COMMON_CHARLOGINPACKETS_H
	#endif // COMMON_CHARLOGINPACKETS_H
	#ifdef COMMON_CHARMAPPACKETS_H
		{ "PACKET_CHARMAP_AGENCY_JOIN_PARTY", sizeof(struct PACKET_CHARMAP_AGENCY_JOIN_PARTY), SERVER_TYPE_ALL },
		{ "PACKET_CHARMAP_GUILD_EMBLEM", sizeof(struct PACKET_CHARMAP_GUILD_EMBLEM), SERVER_TYPE_ALL },
		{ "PACKET_CHARMAP_GUILD_INFO", sizeof(struct PACKET_CHARMAP_GUILD_INFO), SERVER_TYPE_ALL },
		{ "PACKET_CHARMAP_GUILD_INFO_EMBLEM", sizeof(struct PACKET_CHARMAP_GUILD_INFO_EMBLEM), SERVER_TYPE_ALL },
		{ "PACKET_CHARMAP_GUILD_INFO_EMPTY", sizeof(struct PACKET_CHARMAP_GUILD_INFO_EMPTY), SERVER_TYPE_ALL },
	#else
		#define COMMON_CHARMAPPACKETS_H
	#endif // COMMON_CHARMAPPACKETS_H
	#ifdef COMMON_CONF_H
		{ "libconfig_interface", sizeof(struct libconfig_interface), SERVER_TYPE_ALL },
	#else
		#define COMMON_CONF_H
	#endif // COMMON_CONF_H
	#ifdef COMMON_CONSOLE_H
		{ "CParseEntry", sizeof(struct CParseEntry), SERVER_TYPE_ALL },
		{ "console_input_interface", sizeof(struct console_input_interface), SERVER_TYPE_ALL },
		{ "console_interface", sizeof(struct console_interface), SERVER_TYPE_ALL },
	#else
		#define COMMON_CONSOLE_H
	#endif // COMMON_CONSOLE_H
	#ifdef COMMON_CORE_H
		{ "CmdlineArgData", sizeof(struct CmdlineArgData), SERVER_TYPE_ALL },
		{ "cmdline_interface", sizeof(struct cmdline_interface), SERVER_TYPE_ALL },
		{ "core_interface", sizeof(struct core_interface), SERVER_TYPE_ALL },
	#else
		#define COMMON_CORE_H
	#endif // COMMON_CORE_H
	#ifdef COMMON_DB_H
		{ "DBData", sizeof(struct DBData), SERVER_TYPE_ALL },
		{ "DBIterator", sizeof(struct DBIterator), SERVER_TYPE_ALL },
		{ "DBMap", sizeof(struct DBMap), SERVER_TYPE_ALL },
		{ "db_interface", sizeof(struct db_interface), SERVER_TYPE_ALL },
		{ "linkdb_node", sizeof(struct linkdb_node), SERVER_TYPE_ALL },
	#else
		#define COMMON_DB_H
	#endif // COMMON_DB_H
	#ifdef COMMON_DES_H
		{ "des_bit64", sizeof(struct des_bit64), SERVER_TYPE_ALL },
		{ "des_interface", sizeof(struct des_interface), SERVER_TYPE_ALL },
	#else
		#define COMMON_DES_H
	#endif // COMMON_DES_H
	#ifdef COMMON_ERS_H
		{ "eri", sizeof(struct eri), SERVER_TYPE_ALL },
	#else
		#define COMMON_ERS_H
	#endif // COMMON_ERS_H
	#ifdef COMMON_EXTRACONF_H
		{ "config_data", sizeof(struct config_data), SERVER_TYPE_ALL },
		{ "config_data_old", sizeof(struct config_data_old), SERVER_TYPE_ALL },
		{ "emblems_config", sizeof(struct emblems_config), SERVER_TYPE_ALL },
		{ "extraconf_interface", sizeof(struct extraconf_interface), SERVER_TYPE_ALL },
	#else
		#define COMMON_EXTRACONF_H
	#endif // COMMON_EXTRACONF_H
	#ifdef COMMON_GRFIO_H
		{ "grfio_interface", sizeof(struct grfio_interface), SERVER_TYPE_MAP },
	#else
		#define COMMON_GRFIO_H
	#endif // COMMON_GRFIO_H
	#ifdef COMMON_HPMI_H
		{ "HPMi_interface", sizeof(struct HPMi_interface), SERVER_TYPE_ALL },
		{ "hplugin_info", sizeof(struct hplugin_info), SERVER_TYPE_ALL },
		{ "s_HPMDataCheck", sizeof(struct s_HPMDataCheck), SERVER_TYPE_ALL },
	#else
		#define COMMON_HPMI_H
	#endif // COMMON_HPMI_H
	#ifdef COMMON_MAPCHARPACKETS_H
		{ "PACKET_MAPCHAR_AGENCY_JOIN_PARTY_REQ", sizeof(struct PACKET_MAPCHAR_AGENCY_JOIN_PARTY_REQ), SERVER_TYPE_ALL },
		{ "PACKET_MAPCHAR_AUTH_REQ", sizeof(struct PACKET_MAPCHAR_AUTH_REQ), SERVER_TYPE_ALL },
		{ "PACKET_MAPCHAR_GUILD_EMBLEM", sizeof(struct PACKET_MAPCHAR_GUILD_EMBLEM), SERVER_TYPE_ALL },
	#else
		#define COMMON_MAPCHARPACKETS_H
	#endif // COMMON_MAPCHARPACKETS_H
	#ifdef COMMON_MAPINDEX_H
		{ "mapindex_interface", sizeof(struct mapindex_interface), SERVER_TYPE_CHAR|SERVER_TYPE_MAP },
	#else
		#define COMMON_MAPINDEX_H
	#endif // COMMON_MAPINDEX_H
	#ifdef COMMON_MD5CALC_H
		{ "md5_interface", sizeof(struct md5_interface), SERVER_TYPE_ALL },
	#else
		#define COMMON_MD5CALC_H
	#endif // COMMON_MD5CALC_H
	#ifdef COMMON_MEMMGR_H
		{ "malloc_interface", sizeof(struct malloc_interface), SERVER_TYPE_ALL },
	#else
		#define COMMON_MEMMGR_H
	#endif // COMMON_MEMMGR_H
	#ifdef COMMON_MMO_H
		{ "achievement", sizeof(struct achievement), SERVER_TYPE_ALL },
		{ "auction_data", sizeof(struct auction_data), SERVER_TYPE_ALL },
		{ "char_achievements", sizeof(struct char_achievements), SERVER_TYPE_ALL },
		{ "clan", sizeof(struct clan), SERVER_TYPE_ALL },
		{ "clan_buff", sizeof(struct clan_buff), SERVER_TYPE_ALL },
		{ "clan_member", sizeof(struct clan_member), SERVER_TYPE_ALL },
		{ "clan_relationship", sizeof(struct clan_relationship), SERVER_TYPE_ALL },
		{ "fame_list", sizeof(struct fame_list), SERVER_TYPE_ALL },
		{ "guild", sizeof(struct guild), SERVER_TYPE_ALL },
		{ "guild_alliance", sizeof(struct guild_alliance), SERVER_TYPE_ALL },
		{ "guild_castle", sizeof(struct guild_castle), SERVER_TYPE_ALL },
		{ "guild_expulsion", sizeof(struct guild_expulsion), SERVER_TYPE_ALL },
		{ "guild_member", sizeof(struct guild_member), SERVER_TYPE_ALL },
		{ "guild_position", sizeof(struct guild_position), SERVER_TYPE_ALL },
		{ "guild_skill", sizeof(struct guild_skill), SERVER_TYPE_ALL },
		{ "guild_storage", sizeof(struct guild_storage), SERVER_TYPE_ALL },
		{ "hotkey", sizeof(struct hotkey), SERVER_TYPE_ALL },
		{ "item", sizeof(struct item), SERVER_TYPE_ALL },
		{ "item_option", sizeof(struct item_option), SERVER_TYPE_ALL },
		{ "mail_data", sizeof(struct mail_data), SERVER_TYPE_ALL },
		{ "mail_message", sizeof(struct mail_message), SERVER_TYPE_ALL },
		{ "mmo_charstatus", sizeof(struct mmo_charstatus), SERVER_TYPE_ALL },
		{ "party", sizeof(struct party), SERVER_TYPE_ALL },
		{ "party_member", sizeof(struct party_member), SERVER_TYPE_ALL },
		{ "point", sizeof(struct point), SERVER_TYPE_ALL },
		{ "quest", sizeof(struct quest), SERVER_TYPE_ALL },
		{ "rodex_item", sizeof(struct rodex_item), SERVER_TYPE_ALL },
		{ "rodex_maillist", sizeof(struct rodex_maillist), SERVER_TYPE_ALL },
		{ "rodex_message", sizeof(struct rodex_message), SERVER_TYPE_ALL },
		{ "s_elemental", sizeof(struct s_elemental), SERVER_TYPE_ALL },
		{ "s_friend", sizeof(struct s_friend), SERVER_TYPE_ALL },
		{ "s_homunculus", sizeof(struct s_homunculus), SERVER_TYPE_ALL },
		{ "s_mercenary", sizeof(struct s_mercenary), SERVER_TYPE_ALL },
		{ "s_pet", sizeof(struct s_pet), SERVER_TYPE_ALL },
		{ "s_skill", sizeof(struct s_skill), SERVER_TYPE_ALL },
		{ "script_reg_num", sizeof(struct script_reg_num), SERVER_TYPE_ALL },
		{ "script_reg_state", sizeof(struct script_reg_state), SERVER_TYPE_ALL },
		{ "script_reg_str", sizeof(struct script_reg_str), SERVER_TYPE_ALL },
		{ "status_change_data", sizeof(struct status_change_data), SERVER_TYPE_ALL },
		{ "storage_data", sizeof(struct storage_data), SERVER_TYPE_ALL },
	#else
		#define COMMON_MMO_H
	#endif // COMMON_MMO_H
	#ifdef COMMON_MUTEX_H
		{ "mutex_interface", sizeof(struct mutex_interface), SERVER_TYPE_ALL },
	#else
		#define COMMON_MUTEX_H
	#endif // COMMON_MUTEX_H
	#ifdef COMMON_NULLPO_H
		{ "nullpo_interface", sizeof(struct nullpo_interface), SERVER_TYPE_ALL },
	#else
		#define COMMON_NULLPO_H
	#endif // COMMON_NULLPO_H
	#ifdef COMMON_PACKETS_H
		{ "packets_interface", sizeof(struct packets_interface), SERVER_TYPE_ALL },
	#else
		#define COMMON_PACKETS_H
	#endif // COMMON_PACKETS_H
	#ifdef COMMON_PACKETS_STRUCT_H
		{ "PACKET_INTER_CREATE_PET", sizeof(struct PACKET_INTER_CREATE_PET), SERVER_TYPE_ALL },
	#else
		#define COMMON_PACKETS_STRUCT_H
	#endif // COMMON_PACKETS_STRUCT_H
	#ifdef COMMON_RANDOM_H
		{ "rnd_interface", sizeof(struct rnd_interface), SERVER_TYPE_ALL },
	#else
		#define COMMON_RANDOM_H
	#endif // COMMON_RANDOM_H
	#ifdef COMMON_SHOWMSG_H
		{ "showmsg_interface", sizeof(struct showmsg_interface), SERVER_TYPE_ALL },
	#else
		#define COMMON_SHOWMSG_H
	#endif // COMMON_SHOWMSG_H
	#ifdef COMMON_SOCKET_H
		{ "hSockOpt", sizeof(struct hSockOpt), SERVER_TYPE_ALL },
		{ "s_subnet", sizeof(struct s_subnet), SERVER_TYPE_ALL },
		{ "s_subnet_vector", sizeof(struct s_subnet_vector), SERVER_TYPE_ALL },
		{ "socket_data", sizeof(struct socket_data), SERVER_TYPE_ALL },
		{ "socket_interface", sizeof(struct socket_interface), SERVER_TYPE_ALL },
	#else
		#define COMMON_SOCKET_H
	#endif // COMMON_SOCKET_H
	#ifdef COMMON_SPINLOCK_H
		{ "spin_lock", sizeof(struct spin_lock), SERVER_TYPE_ALL },
	#else
		#define COMMON_SPINLOCK_H
	#endif // COMMON_SPINLOCK_H
	#ifdef COMMON_SQL_H
		{ "sql_interface", sizeof(struct sql_interface), SERVER_TYPE_ALL },
	#else
		#define COMMON_SQL_H
	#endif // COMMON_SQL_H
	#ifdef COMMON_STRLIB_H
		{ "StringBuf", sizeof(struct StringBuf), SERVER_TYPE_ALL },
		{ "s_svstate", sizeof(struct s_svstate), SERVER_TYPE_ALL },
		{ "stringbuf_interface", sizeof(struct stringbuf_interface), SERVER_TYPE_ALL },
		{ "strlib_interface", sizeof(struct strlib_interface), SERVER_TYPE_ALL },
		{ "sv_interface", sizeof(struct sv_interface), SERVER_TYPE_ALL },
	#else
		#define COMMON_STRLIB_H
	#endif // COMMON_STRLIB_H
	#ifdef COMMON_SYSINFO_H
		{ "sysinfo_interface", sizeof(struct sysinfo_interface), SERVER_TYPE_ALL },
	#else
		#define COMMON_SYSINFO_H
	#endif // COMMON_SYSINFO_H
	#ifdef COMMON_THREAD_H
		{ "thread_interface", sizeof(struct thread_interface), SERVER_TYPE_ALL },
	#else
		#define COMMON_THREAD_H
	#endif // COMMON_THREAD_H
	#ifdef COMMON_TIMER_H
		{ "TimerData", sizeof(struct TimerData), SERVER_TYPE_ALL },
		{ "timer_interface", sizeof(struct timer_interface), SERVER_TYPE_ALL },
	#else
		#define COMMON_TIMER_H
	#endif // COMMON_TIMER_H
	#ifdef COMMON_UTILS_H
		{ "HCache_interface", sizeof(struct HCache_interface), SERVER_TYPE_ALL },
	#else
		#define COMMON_UTILS_H
	#endif // COMMON_UTILS_H
	#ifdef LOGIN_ACCOUNT_H
		{ "Account_engine", sizeof(struct Account_engine), SERVER_TYPE_LOGIN },
		{ "AccountDB", sizeof(struct AccountDB), SERVER_TYPE_LOGIN },
		{ "AccountDB_SQL", sizeof(struct AccountDB_SQL), SERVER_TYPE_LOGIN },
		{ "AccountDBIterator", sizeof(struct AccountDBIterator), SERVER_TYPE_LOGIN },
		{ "AccountDBIterator_SQL", sizeof(struct AccountDBIterator_SQL), SERVER_TYPE_LOGIN },
		{ "account_interface", sizeof(struct account_interface), SERVER_TYPE_LOGIN },
		{ "mmo_account", sizeof(struct mmo_account), SERVER_TYPE_LOGIN },
	#else
		#define LOGIN_ACCOUNT_H
	#endif // LOGIN_ACCOUNT_H
	#ifdef LOGIN_IPBAN_H
		{ "ipban_interface", sizeof(struct ipban_interface), SERVER_TYPE_LOGIN },
		{ "s_ipban_dbs", sizeof(struct s_ipban_dbs), SERVER_TYPE_LOGIN },
	#else
		#define LOGIN_IPBAN_H
	#endif // LOGIN_IPBAN_H
	#ifdef LOGIN_LAPIIF_H
		{ "lapiif_interface", sizeof(struct lapiif_interface), SERVER_TYPE_LOGIN },
	#else
		#define LOGIN_LAPIIF_H
	#endif // LOGIN_LAPIIF_H
	#ifdef LOGIN_LCLIF_H
		{ "lclif_interface", sizeof(struct lclif_interface), SERVER_TYPE_LOGIN },
		{ "login_packet_db", sizeof(struct login_packet_db), SERVER_TYPE_LOGIN },
	#else
		#define LOGIN_LCLIF_H
	#endif // LOGIN_LCLIF_H
	#ifdef LOGIN_LCLIF_P_H
		{ "lclif_interface_dbs", sizeof(struct lclif_interface_dbs), SERVER_TYPE_LOGIN },
		{ "lclif_interface_private", sizeof(struct lclif_interface_private), SERVER_TYPE_LOGIN },
	#else
		#define LOGIN_LCLIF_P_H
	#endif // LOGIN_LCLIF_P_H
	#ifdef LOGIN_LOGINLOG_H
		{ "loginlog_interface", sizeof(struct loginlog_interface), SERVER_TYPE_LOGIN },
		{ "s_loginlog_dbs", sizeof(struct s_loginlog_dbs), SERVER_TYPE_LOGIN },
	#else
		#define LOGIN_LOGINLOG_H
	#endif // LOGIN_LOGINLOG_H
	#ifdef LOGIN_LOGIN_H
		{ "Login_Config", sizeof(struct Login_Config), SERVER_TYPE_LOGIN },
		{ "client_hash_node", sizeof(struct client_hash_node), SERVER_TYPE_LOGIN },
		{ "lchrif_interface", sizeof(struct lchrif_interface), SERVER_TYPE_LOGIN },
		{ "login_auth_node", sizeof(struct login_auth_node), SERVER_TYPE_LOGIN },
		{ "login_interface", sizeof(struct login_interface), SERVER_TYPE_LOGIN },
		{ "login_session_data", sizeof(struct login_session_data), SERVER_TYPE_LOGIN },
		{ "mmo_api_server", sizeof(struct mmo_api_server), SERVER_TYPE_LOGIN },
		{ "mmo_char_server", sizeof(struct mmo_char_server), SERVER_TYPE_LOGIN },
		{ "online_login_data", sizeof(struct online_login_data), SERVER_TYPE_LOGIN },
		{ "s_login_dbs", sizeof(struct s_login_dbs), SERVER_TYPE_LOGIN },
	#else
		#define LOGIN_LOGIN_H
	#endif // LOGIN_LOGIN_H
	#ifdef LOGIN_PACKETS_AC_STRUCT_H
		{ "PACKET_AC_ACCEPT_LOGIN", sizeof(struct PACKET_AC_ACCEPT_LOGIN), SERVER_TYPE_LOGIN },
		{ "PACKET_AC_REFUSE_LOGIN", sizeof(struct PACKET_AC_REFUSE_LOGIN), SERVER_TYPE_LOGIN },
		{ "PACKET_AC_REFUSE_LOGIN_R2", sizeof(struct PACKET_AC_REFUSE_LOGIN_R2), SERVER_TYPE_LOGIN },
		{ "PACKET_SC_NOTIFY_BAN", sizeof(struct PACKET_SC_NOTIFY_BAN), SERVER_TYPE_LOGIN },
	#else
		#define LOGIN_PACKETS_AC_STRUCT_H
	#endif // LOGIN_PACKETS_AC_STRUCT_H
	#ifdef LOGIN_PACKETS_CA_STRUCT_H
		{ "PACKET_CA_APISERVERCONNECT", sizeof(struct PACKET_CA_APISERVERCONNECT), SERVER_TYPE_LOGIN },
		{ "PACKET_CA_CHARSERVERCONNECT", sizeof(struct PACKET_CA_CHARSERVERCONNECT), SERVER_TYPE_LOGIN },
		{ "PACKET_CA_CONNECT_INFO_CHANGED", sizeof(struct PACKET_CA_CONNECT_INFO_CHANGED), SERVER_TYPE_LOGIN },
		{ "PACKET_CA_EXE_HASHCHECK", sizeof(struct PACKET_CA_EXE_HASHCHECK), SERVER_TYPE_LOGIN },
		{ "PACKET_CA_LOGIN", sizeof(struct PACKET_CA_LOGIN), SERVER_TYPE_LOGIN },
		{ "PACKET_CA_LOGIN2", sizeof(struct PACKET_CA_LOGIN2), SERVER_TYPE_LOGIN },
		{ "PACKET_CA_LOGIN3", sizeof(struct PACKET_CA_LOGIN3), SERVER_TYPE_LOGIN },
		{ "PACKET_CA_LOGIN4", sizeof(struct PACKET_CA_LOGIN4), SERVER_TYPE_LOGIN },
		{ "PACKET_CA_LOGIN_HAN", sizeof(struct PACKET_CA_LOGIN_HAN), SERVER_TYPE_LOGIN },
		{ "PACKET_CA_LOGIN_OTP", sizeof(struct PACKET_CA_LOGIN_OTP), SERVER_TYPE_LOGIN },
		{ "PACKET_CA_LOGIN_PCBANG", sizeof(struct PACKET_CA_LOGIN_PCBANG), SERVER_TYPE_LOGIN },
		{ "PACKET_CA_SSO_LOGIN_REQ", sizeof(struct PACKET_CA_SSO_LOGIN_REQ), SERVER_TYPE_LOGIN },
	#else
		#define LOGIN_PACKETS_CA_STRUCT_H
	#endif // LOGIN_PACKETS_CA_STRUCT_H
	#ifdef MAP_ACHIEVEMENT_H
		{ "achievement_data", sizeof(struct achievement_data), SERVER_TYPE_MAP },
		{ "achievement_interface", sizeof(struct achievement_interface), SERVER_TYPE_MAP },
		{ "achievement_objective", sizeof(struct achievement_objective), SERVER_TYPE_MAP },
		{ "achievement_reward_item", sizeof(struct achievement_reward_item), SERVER_TYPE_MAP },
		{ "achievement_rewards", sizeof(struct achievement_rewards), SERVER_TYPE_MAP },
	#else
		#define MAP_ACHIEVEMENT_H
	#endif // MAP_ACHIEVEMENT_H
	#ifdef MAP_ATCOMMAND_H
		{ "AliasInfo", sizeof(struct AliasInfo), SERVER_TYPE_MAP },
		{ "AtCommandInfo", sizeof(struct AtCommandInfo), SERVER_TYPE_MAP },
		{ "atcmd_binding_data", sizeof(struct atcmd_binding_data), SERVER_TYPE_MAP },
		{ "atcommand_interface", sizeof(struct atcommand_interface), SERVER_TYPE_MAP },
	#else
		#define MAP_ATCOMMAND_H
	#endif // MAP_ATCOMMAND_H
	#ifdef MAP_BATTLEGROUND_H
		{ "battleground_data", sizeof(struct battleground_data), SERVER_TYPE_MAP },
		{ "battleground_interface", sizeof(struct battleground_interface), SERVER_TYPE_MAP },
		{ "battleground_member_data", sizeof(struct battleground_member_data), SERVER_TYPE_MAP },
		{ "bg_arena", sizeof(struct bg_arena), SERVER_TYPE_MAP },
	#else
		#define MAP_BATTLEGROUND_H
	#endif // MAP_BATTLEGROUND_H
	#ifdef MAP_BATTLE_H
		{ "Battle_Config", sizeof(struct Battle_Config), SERVER_TYPE_MAP },
		{ "Damage", sizeof(struct Damage), SERVER_TYPE_MAP },
		{ "battle_interface", sizeof(struct battle_interface), SERVER_TYPE_MAP },
		{ "delay_damage", sizeof(struct delay_damage), SERVER_TYPE_MAP },
	#else
		#define MAP_BATTLE_H
	#endif // MAP_BATTLE_H
	#ifdef MAP_BUYINGSTORE_H
		{ "buyingstore_interface", sizeof(struct buyingstore_interface), SERVER_TYPE_MAP },
		{ "s_buyingstore", sizeof(struct s_buyingstore), SERVER_TYPE_MAP },
		{ "s_buyingstore_item", sizeof(struct s_buyingstore_item), SERVER_TYPE_MAP },
	#else
		#define MAP_BUYINGSTORE_H
	#endif // MAP_BUYINGSTORE_H
	#ifdef MAP_CHANNEL_H
		{ "Channel_Config", sizeof(struct Channel_Config), SERVER_TYPE_MAP },
		{ "channel_ban_entry", sizeof(struct channel_ban_entry), SERVER_TYPE_MAP },
		{ "channel_data", sizeof(struct channel_data), SERVER_TYPE_MAP },
		{ "channel_interface", sizeof(struct channel_interface), SERVER_TYPE_MAP },
	#else
		#define MAP_CHANNEL_H
	#endif // MAP_CHANNEL_H
	#ifdef MAP_CHAT_H
		{ "chat_data", sizeof(struct chat_data), SERVER_TYPE_MAP },
		{ "chat_interface", sizeof(struct chat_interface), SERVER_TYPE_MAP },
	#else
		#define MAP_CHAT_H
	#endif // MAP_CHAT_H
	#ifdef MAP_CHRIF_H
		{ "auth_node", sizeof(struct auth_node), SERVER_TYPE_MAP },
		{ "chrif_interface", sizeof(struct chrif_interface), SERVER_TYPE_MAP },
	#else
		#define MAP_CHRIF_H
	#endif // MAP_CHRIF_H
	#ifdef MAP_CLAN_H
		{ "clan_interface", sizeof(struct clan_interface), SERVER_TYPE_MAP },
	#else
		#define MAP_CLAN_H
	#endif // MAP_CLAN_H
	#ifdef MAP_CLIF_H
		{ "attendance_entry", sizeof(struct attendance_entry), SERVER_TYPE_MAP },
		{ "barter_itemlist_entry", sizeof(struct barter_itemlist_entry), SERVER_TYPE_MAP },
		{ "barteritemlist", sizeof(struct barteritemlist), SERVER_TYPE_MAP },
		{ "cdelayed_damage", sizeof(struct cdelayed_damage), SERVER_TYPE_MAP },
		{ "clif_interface", sizeof(struct clif_interface), SERVER_TYPE_MAP },
		{ "hCSData", sizeof(struct hCSData), SERVER_TYPE_MAP },
		{ "merge_item", sizeof(struct merge_item), SERVER_TYPE_MAP },
		{ "s_packet_db", sizeof(struct s_packet_db), SERVER_TYPE_MAP },
	#else
		#define MAP_CLIF_H
	#endif // MAP_CLIF_H
	#ifdef MAP_DUEL_H
		{ "duel", sizeof(struct duel), SERVER_TYPE_MAP },
		{ "duel_interface", sizeof(struct duel_interface), SERVER_TYPE_MAP },
	#else
		#define MAP_DUEL_H
	#endif // MAP_DUEL_H
	#ifdef MAP_ELEMENTAL_H
		{ "elemental_data", sizeof(struct elemental_data), SERVER_TYPE_MAP },
		{ "elemental_interface", sizeof(struct elemental_interface), SERVER_TYPE_MAP },
		{ "elemental_skill", sizeof(struct elemental_skill), SERVER_TYPE_MAP },
		{ "s_elemental_db", sizeof(struct s_elemental_db), SERVER_TYPE_MAP },
	#else
		#define MAP_ELEMENTAL_H
	#endif // MAP_ELEMENTAL_H
	#ifdef MAP_ENCHANTUI_H
		{ "enchant_info", sizeof(struct enchant_info), SERVER_TYPE_MAP },
		{ "enchant_info_normal", sizeof(struct enchant_info_normal), SERVER_TYPE_MAP },
		{ "enchant_info_perfect", sizeof(struct enchant_info_perfect), SERVER_TYPE_MAP },
		{ "enchant_info_perfect_entry", sizeof(struct enchant_info_perfect_entry), SERVER_TYPE_MAP },
		{ "enchant_info_reset", sizeof(struct enchant_info_reset), SERVER_TYPE_MAP },
		{ "enchant_info_upgrade", sizeof(struct enchant_info_upgrade), SERVER_TYPE_MAP },
		{ "enchant_info_upgrade_entry", sizeof(struct enchant_info_upgrade_entry), SERVER_TYPE_MAP },
		{ "enchant_item_list", sizeof(struct enchant_item_list), SERVER_TYPE_MAP },
		{ "enchant_item_rate_entry", sizeof(struct enchant_item_rate_entry), SERVER_TYPE_MAP },
		{ "enchant_slot_info", sizeof(struct enchant_slot_info), SERVER_TYPE_MAP },
		{ "enchantui_interface", sizeof(struct enchantui_interface), SERVER_TYPE_MAP },
	#else
		#define MAP_ENCHANTUI_H
	#endif // MAP_ENCHANTUI_H
	#ifdef MAP_GOLDPC_H
		{ "goldpc_interface", sizeof(struct goldpc_interface), SERVER_TYPE_MAP },
		{ "goldpc_mode", sizeof(struct goldpc_mode), SERVER_TYPE_MAP },
	#else
		#define MAP_GOLDPC_H
	#endif // MAP_GOLDPC_H
	#ifdef MAP_GRADER_H
		{ "grade_blessing", sizeof(struct grade_blessing), SERVER_TYPE_MAP },
		{ "grade_interface_dbs", sizeof(struct grade_interface_dbs), SERVER_TYPE_MAP },
		{ "grade_material", sizeof(struct grade_material), SERVER_TYPE_MAP },
		{ "grader_interface", sizeof(struct grader_interface), SERVER_TYPE_MAP },
		{ "s_grade_info", sizeof(struct s_grade_info), SERVER_TYPE_MAP },
	#else
		#define MAP_GRADER_H
	#endif // MAP_GRADER_H
	#ifdef MAP_GUILD_H
		{ "eventlist", sizeof(struct eventlist), SERVER_TYPE_MAP },
		{ "guardian_data", sizeof(struct guardian_data), SERVER_TYPE_MAP },
		{ "guild_expcache", sizeof(struct guild_expcache), SERVER_TYPE_MAP },
		{ "guild_interface", sizeof(struct guild_interface), SERVER_TYPE_MAP },
		{ "s_guild_skill_tree", sizeof(struct s_guild_skill_tree), SERVER_TYPE_MAP },
	#else
		#define MAP_GUILD_H
	#endif // MAP_GUILD_H
	#ifdef MAP_HOMUNCULUS_H
		{ "h_stats", sizeof(struct h_stats), SERVER_TYPE_MAP },
		{ "homun_data", sizeof(struct homun_data), SERVER_TYPE_MAP },
		{ "homun_dbs", sizeof(struct homun_dbs), SERVER_TYPE_MAP },
		{ "homun_skill_tree_entry", sizeof(struct homun_skill_tree_entry), SERVER_TYPE_MAP },
		{ "homunculus_interface", sizeof(struct homunculus_interface), SERVER_TYPE_MAP },
		{ "s_homunculus_db", sizeof(struct s_homunculus_db), SERVER_TYPE_MAP },
	#else
		#define MAP_HOMUNCULUS_H
	#endif // MAP_HOMUNCULUS_H
	#ifdef MAP_INSTANCE_H
		{ "instance_data", sizeof(struct instance_data), SERVER_TYPE_MAP },
		{ "instance_interface", sizeof(struct instance_interface), SERVER_TYPE_MAP },
	#else
		#define MAP_INSTANCE_H
	#endif // MAP_INSTANCE_H
	#ifdef MAP_INTIF_H
		{ "intif_interface", sizeof(struct intif_interface), SERVER_TYPE_MAP },
	#else
		#define MAP_INTIF_H
	#endif // MAP_INTIF_H
	#ifdef MAP_IRC_BOT_H
		{ "irc_func", sizeof(struct irc_func), SERVER_TYPE_MAP },
		{ "ircbot_interface", sizeof(struct ircbot_interface), SERVER_TYPE_MAP },
		{ "message_flood", sizeof(struct message_flood), SERVER_TYPE_MAP },
	#else
		#define MAP_IRC_BOT_H
	#endif // MAP_IRC_BOT_H
	#ifdef MAP_ITEMDB_H
		{ "item_chain", sizeof(struct item_chain), SERVER_TYPE_MAP },
		{ "item_chain_entry", sizeof(struct item_chain_entry), SERVER_TYPE_MAP },
		{ "item_combo", sizeof(struct item_combo), SERVER_TYPE_MAP },
		{ "item_data", sizeof(struct item_data), SERVER_TYPE_MAP },
		{ "item_group", sizeof(struct item_group), SERVER_TYPE_MAP },
		{ "item_lapineddukddak", sizeof(struct item_lapineddukddak), SERVER_TYPE_MAP },
		{ "item_lapineupgrade", sizeof(struct item_lapineupgrade), SERVER_TYPE_MAP },
		{ "item_package", sizeof(struct item_package), SERVER_TYPE_MAP },
		{ "item_package_must_entry", sizeof(struct item_package_must_entry), SERVER_TYPE_MAP },
		{ "item_package_rand_entry", sizeof(struct item_package_rand_entry), SERVER_TYPE_MAP },
		{ "item_package_rand_group", sizeof(struct item_package_rand_group), SERVER_TYPE_MAP },
		{ "item_reform", sizeof(struct item_reform), SERVER_TYPE_MAP },
		{ "itemdb_interface", sizeof(struct itemdb_interface), SERVER_TYPE_MAP },
		{ "itemdb_option", sizeof(struct itemdb_option), SERVER_TYPE_MAP },
		{ "itemlist", sizeof(struct itemlist), SERVER_TYPE_MAP },
		{ "itemlist_entry", sizeof(struct itemlist_entry), SERVER_TYPE_MAP },
	#else
		#define MAP_ITEMDB_H
	#endif // MAP_ITEMDB_H
	#ifdef MAP_LOG_H
		{ "log_interface", sizeof(struct log_interface), SERVER_TYPE_MAP },
	#else
		#define MAP_LOG_H
	#endif // MAP_LOG_H
	#ifdef MAP_MACRO_H
		{ "captcha_data", sizeof(struct captcha_data), SERVER_TYPE_MAP },
		{ "macro_detect", sizeof(struct macro_detect), SERVER_TYPE_MAP },
		{ "macro_interface", sizeof(struct macro_interface), SERVER_TYPE_MAP },
		{ "macroaidlist", sizeof(struct macroaidlist), SERVER_TYPE_MAP },
	#else
		#define MAP_MACRO_H
	#endif // MAP_MACRO_H
	#ifdef MAP_MAIL_H
		{ "mail_interface", sizeof(struct mail_interface), SERVER_TYPE_MAP },
	#else
		#define MAP_MAIL_H
	#endif // MAP_MAIL_H
	#ifdef MAP_MAPIIF_H
		{ "mapiif_interface", sizeof(struct mapiif_interface), SERVER_TYPE_MAP },
	#else
		#define MAP_MAPIIF_H
	#endif // MAP_MAPIIF_H
	#ifdef MAP_MAPREG_H
		{ "mapreg_interface", sizeof(struct mapreg_interface), SERVER_TYPE_MAP },
		{ "mapreg_save", sizeof(struct mapreg_save), SERVER_TYPE_MAP },
	#else
		#define MAP_MAPREG_H
	#endif // MAP_MAPREG_H
	#ifdef MAP_MAP_H
		{ "block_list", sizeof(struct block_list), SERVER_TYPE_MAP },
		{ "charid2nick", sizeof(struct charid2nick), SERVER_TYPE_MAP },
		{ "charid_request", sizeof(struct charid_request), SERVER_TYPE_MAP },
		{ "flooritem_data", sizeof(struct flooritem_data), SERVER_TYPE_MAP },
		{ "iwall_data", sizeof(struct iwall_data), SERVER_TYPE_MAP },
		{ "map_cache_header", sizeof(struct map_cache_header), SERVER_TYPE_MAP },
		{ "map_data", sizeof(struct map_data), SERVER_TYPE_MAP },
		{ "map_drop_list", sizeof(struct map_drop_list), SERVER_TYPE_MAP },
		{ "map_interface", sizeof(struct map_interface), SERVER_TYPE_MAP },
		{ "map_zone_data", sizeof(struct map_zone_data), SERVER_TYPE_MAP },
		{ "map_zone_disabled_command_entry", sizeof(struct map_zone_disabled_command_entry), SERVER_TYPE_MAP },
		{ "map_zone_disabled_skill_entry", sizeof(struct map_zone_disabled_skill_entry), SERVER_TYPE_MAP },
		{ "map_zone_skill_damage_cap_entry", sizeof(struct map_zone_skill_damage_cap_entry), SERVER_TYPE_MAP },
		{ "mapcell", sizeof(struct mapcell), SERVER_TYPE_MAP },
		{ "mapflag_skill_adjust", sizeof(struct mapflag_skill_adjust), SERVER_TYPE_MAP },
		{ "mapit_interface", sizeof(struct mapit_interface), SERVER_TYPE_MAP },
		{ "spawn_data", sizeof(struct spawn_data), SERVER_TYPE_MAP },
	#else
		#define MAP_MAP_H
	#endif // MAP_MAP_H
	#ifdef MAP_MERCENARY_H
		{ "mercenary_data", sizeof(struct mercenary_data), SERVER_TYPE_MAP },
		{ "mercenary_interface", sizeof(struct mercenary_interface), SERVER_TYPE_MAP },
		{ "s_mercenary_db", sizeof(struct s_mercenary_db), SERVER_TYPE_MAP },
	#else
		#define MAP_MERCENARY_H
	#endif // MAP_MERCENARY_H
	#ifdef MAP_MOB_H
		{ "item_drop", sizeof(struct item_drop), SERVER_TYPE_MAP },
		{ "item_drop_list", sizeof(struct item_drop_list), SERVER_TYPE_MAP },
		{ "mob_chat", sizeof(struct mob_chat), SERVER_TYPE_MAP },
		{ "mob_data", sizeof(struct mob_data), SERVER_TYPE_MAP },
		{ "mob_db", sizeof(struct mob_db), SERVER_TYPE_MAP },
		{ "mob_drop", sizeof(struct mob_drop), SERVER_TYPE_MAP },
		{ "mob_group", sizeof(struct mob_group), SERVER_TYPE_MAP },
		{ "mob_interface", sizeof(struct mob_interface), SERVER_TYPE_MAP },
		{ "mob_skill", sizeof(struct mob_skill), SERVER_TYPE_MAP },
		{ "optdrop_group", sizeof(struct optdrop_group), SERVER_TYPE_MAP },
		{ "optdrop_group_option", sizeof(struct optdrop_group_option), SERVER_TYPE_MAP },
		{ "optdrop_group_optslot", sizeof(struct optdrop_group_optslot), SERVER_TYPE_MAP },
		{ "spawn_info", sizeof(struct spawn_info), SERVER_TYPE_MAP },
	#else
		#define MAP_MOB_H
	#endif // MAP_MOB_H
	#ifdef MAP_NPC_H
		{ "event_data", sizeof(struct event_data), SERVER_TYPE_MAP },
		{ "npc_barter_currency", sizeof(struct npc_barter_currency), SERVER_TYPE_MAP },
		{ "npc_chat_interface", sizeof(struct npc_chat_interface), SERVER_TYPE_MAP },
		{ "npc_data", sizeof(struct npc_data), SERVER_TYPE_MAP },
		{ "npc_interface", sizeof(struct npc_interface), SERVER_TYPE_MAP },
		{ "npc_item_list", sizeof(struct npc_item_list), SERVER_TYPE_MAP },
		{ "npc_label_list", sizeof(struct npc_label_list), SERVER_TYPE_MAP },
		{ "npc_parse", sizeof(struct npc_parse), SERVER_TYPE_MAP },
		{ "npc_path_data", sizeof(struct npc_path_data), SERVER_TYPE_MAP },
		{ "npc_shop_data", sizeof(struct npc_shop_data), SERVER_TYPE_MAP },
		{ "npc_src_list", sizeof(struct npc_src_list), SERVER_TYPE_MAP },
		{ "npc_timerevent_list", sizeof(struct npc_timerevent_list), SERVER_TYPE_MAP },
		{ "pcre_interface", sizeof(struct pcre_interface), SERVER_TYPE_MAP },
		{ "pcrematch_entry", sizeof(struct pcrematch_entry), SERVER_TYPE_MAP },
		{ "pcrematch_set", sizeof(struct pcrematch_set), SERVER_TYPE_MAP },
	#else
		#define MAP_NPC_H
	#endif // MAP_NPC_H
	#ifdef MAP_PACKETS_STRUCT_H
		{ "CZ_PURCHASE_ITEM_FROMMC", sizeof(struct CZ_PURCHASE_ITEM_FROMMC), SERVER_TYPE_MAP },
		{ "EQUIPITEM_INFO", sizeof(struct EQUIPITEM_INFO), SERVER_TYPE_MAP },
		{ "EQUIPSLOTINFO", sizeof(struct EQUIPSLOTINFO), SERVER_TYPE_MAP },
		{ "GUILD_MEMBER_INFO", sizeof(struct GUILD_MEMBER_INFO), SERVER_TYPE_MAP },
		{ "GUILD_SKILLDATA", sizeof(struct GUILD_SKILLDATA), SERVER_TYPE_MAP },
		{ "ItemOptions", sizeof(struct ItemOptions), SERVER_TYPE_MAP },
		{ "NORMALITEM_INFO", sizeof(struct NORMALITEM_INFO), SERVER_TYPE_MAP },
		{ "PACKET_CZ_ADD_ITEM_TO_MAIL", sizeof(struct PACKET_CZ_ADD_ITEM_TO_MAIL), SERVER_TYPE_MAP },
		{ "PACKET_CZ_CONTACTNPC", sizeof(struct PACKET_CZ_CONTACTNPC), SERVER_TYPE_MAP },
		{ "PACKET_CZ_MEMORIALDUNGEON_COMMAND", sizeof(struct PACKET_CZ_MEMORIALDUNGEON_COMMAND), SERVER_TYPE_MAP },
		{ "PACKET_CZ_MOVE_ITEM_FROM_BODY_TO_CART", sizeof(struct PACKET_CZ_MOVE_ITEM_FROM_BODY_TO_CART), SERVER_TYPE_MAP },
		{ "PACKET_CZ_OPEN_UI", sizeof(struct PACKET_CZ_OPEN_UI), SERVER_TYPE_MAP },
		{ "PACKET_CZ_PARTY_CONFIG", sizeof(struct PACKET_CZ_PARTY_CONFIG), SERVER_TYPE_MAP },
		{ "PACKET_CZ_PC_BUY_CASH_POINT_ITEM", sizeof(struct PACKET_CZ_PC_BUY_CASH_POINT_ITEM), SERVER_TYPE_MAP },
		{ "PACKET_CZ_PC_BUY_CASH_POINT_ITEM_sub", sizeof(struct PACKET_CZ_PC_BUY_CASH_POINT_ITEM_sub), SERVER_TYPE_MAP },
		{ "PACKET_CZ_PC_PURCHASE_ITEMLIST", sizeof(struct PACKET_CZ_PC_PURCHASE_ITEMLIST), SERVER_TYPE_MAP },
		{ "PACKET_CZ_PC_PURCHASE_ITEMLIST_FROMMC", sizeof(struct PACKET_CZ_PC_PURCHASE_ITEMLIST_FROMMC), SERVER_TYPE_MAP },
		{ "PACKET_CZ_PC_PURCHASE_ITEMLIST_FROMMC2", sizeof(struct PACKET_CZ_PC_PURCHASE_ITEMLIST_FROMMC2), SERVER_TYPE_MAP },
		{ "PACKET_CZ_PC_PURCHASE_ITEMLIST_sub", sizeof(struct PACKET_CZ_PC_PURCHASE_ITEMLIST_sub), SERVER_TYPE_MAP },
		{ "PACKET_CZ_PET_EVOLUTION", sizeof(struct PACKET_CZ_PET_EVOLUTION), SERVER_TYPE_MAP },
		{ "PACKET_CZ_PRIVATE_AIRSHIP_REQUEST", sizeof(struct PACKET_CZ_PRIVATE_AIRSHIP_REQUEST), SERVER_TYPE_MAP },
		{ "PACKET_CZ_REQ_DELETE_MAIL", sizeof(struct PACKET_CZ_REQ_DELETE_MAIL), SERVER_TYPE_MAP },
		{ "PACKET_CZ_REQ_GUILD_EMBLEM_IMG1", sizeof(struct PACKET_CZ_REQ_GUILD_EMBLEM_IMG1), SERVER_TYPE_MAP },
		{ "PACKET_CZ_REQ_ITEM_FROM_MAIL", sizeof(struct PACKET_CZ_REQ_ITEM_FROM_MAIL), SERVER_TYPE_MAP },
		{ "PACKET_CZ_REQ_ITEMREPAIR1", sizeof(struct PACKET_CZ_REQ_ITEMREPAIR1), SERVER_TYPE_MAP },
		{ "PACKET_CZ_REQ_MAKINGITEM", sizeof(struct PACKET_CZ_REQ_MAKINGITEM), SERVER_TYPE_MAP },
		{ "PACKET_CZ_REQ_NEXT_MAIL_LIST", sizeof(struct PACKET_CZ_REQ_NEXT_MAIL_LIST), SERVER_TYPE_MAP },
		{ "PACKET_CZ_REQ_OPEN_BUYING_STORE", sizeof(struct PACKET_CZ_REQ_OPEN_BUYING_STORE), SERVER_TYPE_MAP },
		{ "PACKET_CZ_REQ_OPEN_BUYING_STORE_sub", sizeof(struct PACKET_CZ_REQ_OPEN_BUYING_STORE_sub), SERVER_TYPE_MAP },
		{ "PACKET_CZ_REQ_OPEN_MAIL", sizeof(struct PACKET_CZ_REQ_OPEN_MAIL), SERVER_TYPE_MAP },
		{ "PACKET_CZ_REQ_OPEN_WRITE_MAIL", sizeof(struct PACKET_CZ_REQ_OPEN_WRITE_MAIL), SERVER_TYPE_MAP },
		{ "PACKET_CZ_REQ_READ_MAIL", sizeof(struct PACKET_CZ_REQ_READ_MAIL), SERVER_TYPE_MAP },
		{ "PACKET_CZ_REQ_REFRESH_MAIL_LIST", sizeof(struct PACKET_CZ_REQ_REFRESH_MAIL_LIST), SERVER_TYPE_MAP },
		{ "PACKET_CZ_REQ_REMAINTIME", sizeof(struct PACKET_CZ_REQ_REMAINTIME), SERVER_TYPE_MAP },
		{ "PACKET_CZ_REQ_REMOVE_ITEM_MAIL", sizeof(struct PACKET_CZ_REQ_REMOVE_ITEM_MAIL), SERVER_TYPE_MAP },
		{ "PACKET_CZ_REQ_STYLE_CHANGE", sizeof(struct PACKET_CZ_REQ_STYLE_CHANGE), SERVER_TYPE_MAP },
		{ "PACKET_CZ_REQ_STYLE_CHANGE2", sizeof(struct PACKET_CZ_REQ_STYLE_CHANGE2), SERVER_TYPE_MAP },
		{ "PACKET_CZ_REQ_TRADE_BUYING_STORE", sizeof(struct PACKET_CZ_REQ_TRADE_BUYING_STORE), SERVER_TYPE_MAP },
		{ "PACKET_CZ_REQ_TRADE_BUYING_STORE_sub", sizeof(struct PACKET_CZ_REQ_TRADE_BUYING_STORE_sub), SERVER_TYPE_MAP },
		{ "PACKET_CZ_REQ_ZENY_FROM_MAIL", sizeof(struct PACKET_CZ_REQ_ZENY_FROM_MAIL), SERVER_TYPE_MAP },
		{ "PACKET_CZ_REQMAKINGITEM", sizeof(struct PACKET_CZ_REQMAKINGITEM), SERVER_TYPE_MAP },
		{ "PACKET_CZ_REQUEST_ACTNPC", sizeof(struct PACKET_CZ_REQUEST_ACTNPC), SERVER_TYPE_MAP },
		{ "PACKET_CZ_SEARCH_STORE_INFO", sizeof(struct PACKET_CZ_SEARCH_STORE_INFO), SERVER_TYPE_MAP },
		{ "PACKET_CZ_SEARCH_STORE_INFO_item", sizeof(struct PACKET_CZ_SEARCH_STORE_INFO_item), SERVER_TYPE_MAP },
		{ "PACKET_CZ_SEND_MAIL", sizeof(struct PACKET_CZ_SEND_MAIL), SERVER_TYPE_MAP },
		{ "PACKET_CZ_SKILL_SELECT_RESPONSE", sizeof(struct PACKET_CZ_SKILL_SELECT_RESPONSE), SERVER_TYPE_MAP },
		{ "PACKET_CZ_SSILIST_ITEM_CLICK", sizeof(struct PACKET_CZ_SSILIST_ITEM_CLICK), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ACK_BAN_GUILD1", sizeof(struct PACKET_ZC_ACK_BAN_GUILD1), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ACK_BAN_GUILD2", sizeof(struct PACKET_ZC_ACK_BAN_GUILD2), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ACK_BAN_GUILD3", sizeof(struct PACKET_ZC_ACK_BAN_GUILD3), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ACK_CLAN_LEAVE", sizeof(struct PACKET_ZC_ACK_CLAN_LEAVE), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ACK_DELETE_MAIL", sizeof(struct PACKET_ZC_ACK_DELETE_MAIL), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ACK_ITEM_FROM_MAIL", sizeof(struct PACKET_ZC_ACK_ITEM_FROM_MAIL), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ACK_ITEMLIST_BUYING_STORE", sizeof(struct PACKET_ZC_ACK_ITEMLIST_BUYING_STORE), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ACK_ITEMLIST_BUYING_STORE_sub", sizeof(struct PACKET_ZC_ACK_ITEMLIST_BUYING_STORE_sub), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ACK_LEAVE_GUILD1", sizeof(struct PACKET_ZC_ACK_LEAVE_GUILD1), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ACK_LEAVE_GUILD2", sizeof(struct PACKET_ZC_ACK_LEAVE_GUILD2), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ACK_MAKE_GROUP", sizeof(struct PACKET_ZC_ACK_MAKE_GROUP), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ACK_OPEN_WRITE_MAIL", sizeof(struct PACKET_ZC_ACK_OPEN_WRITE_MAIL), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ACK_RANKING_name", sizeof(struct PACKET_ZC_ACK_RANKING_name), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ACK_RANKING_points", sizeof(struct PACKET_ZC_ACK_RANKING_points), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ACK_RANKING_sub", sizeof(struct PACKET_ZC_ACK_RANKING_sub), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ACK_REMOVE_ITEM_MAIL", sizeof(struct PACKET_ZC_ACK_REMOVE_ITEM_MAIL), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ACK_REQMAKINGITEM", sizeof(struct PACKET_ZC_ACK_REQMAKINGITEM), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ACK_REQNAMEALL", sizeof(struct PACKET_ZC_ACK_REQNAMEALL), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ACK_REQNAMEALL_NPC", sizeof(struct PACKET_ZC_ACK_REQNAMEALL_NPC), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ACK_SCHEDULER_CASHITEM", sizeof(struct PACKET_ZC_ACK_SCHEDULER_CASHITEM), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ACK_SCHEDULER_CASHITEM_sub", sizeof(struct PACKET_ZC_ACK_SCHEDULER_CASHITEM_sub), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ACK_TOUSESKILL", sizeof(struct PACKET_ZC_ACK_TOUSESKILL), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ACK_WEAPONREFINE", sizeof(struct PACKET_ZC_ACK_WEAPONREFINE), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ACK_ZENY_FROM_MAIL", sizeof(struct PACKET_ZC_ACK_ZENY_FROM_MAIL), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ADD_EXCHANGE_ITEM", sizeof(struct PACKET_ZC_ADD_EXCHANGE_ITEM), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ADD_ITEM_TO_CART", sizeof(struct PACKET_ZC_ADD_ITEM_TO_CART), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ADD_ITEM_TO_STORE", sizeof(struct PACKET_ZC_ADD_ITEM_TO_STORE), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ADD_MEMBER_TO_GROUP", sizeof(struct PACKET_ZC_ADD_MEMBER_TO_GROUP), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ADD_SKILL", sizeof(struct PACKET_ZC_ADD_SKILL), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ATTACK_FAILURE_FOR_DISTANCE", sizeof(struct PACKET_ZC_ATTACK_FAILURE_FOR_DISTANCE), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ATTACK_RANGE", sizeof(struct PACKET_ZC_ATTACK_RANGE), SERVER_TYPE_MAP },
		{ "PACKET_ZC_AUTORUN_SKILL", sizeof(struct PACKET_ZC_AUTORUN_SKILL), SERVER_TYPE_MAP },
		{ "PACKET_ZC_BAN_LIST", sizeof(struct PACKET_ZC_BAN_LIST), SERVER_TYPE_MAP },
		{ "PACKET_ZC_BAN_LIST_sub", sizeof(struct PACKET_ZC_BAN_LIST_sub), SERVER_TYPE_MAP },
		{ "PACKET_ZC_CASH_ITEM_DELETE", sizeof(struct PACKET_ZC_CASH_ITEM_DELETE), SERVER_TYPE_MAP },
		{ "PACKET_ZC_CASH_TIME_COUNTER", sizeof(struct PACKET_ZC_CASH_TIME_COUNTER), SERVER_TYPE_MAP },
		{ "PACKET_ZC_CHANGE_GUILD", sizeof(struct PACKET_ZC_CHANGE_GUILD), SERVER_TYPE_MAP },
		{ "PACKET_ZC_CHANGESTATE_PET", sizeof(struct PACKET_ZC_CHANGESTATE_PET), SERVER_TYPE_MAP },
		{ "PACKET_ZC_COUPLESTATUS", sizeof(struct PACKET_ZC_COUPLESTATUS), SERVER_TYPE_MAP },
		{ "PACKET_ZC_DELETE_MEMBER_FROM_GROUP", sizeof(struct PACKET_ZC_DELETE_MEMBER_FROM_GROUP), SERVER_TYPE_MAP },
		{ "PACKET_ZC_FAILED_TRADE_BUYING_STORE_TO_SELLER", sizeof(struct PACKET_ZC_FAILED_TRADE_BUYING_STORE_TO_SELLER), SERVER_TYPE_MAP },
		{ "PACKET_ZC_FEED_MER", sizeof(struct PACKET_ZC_FEED_MER), SERVER_TYPE_MAP },
		{ "PACKET_ZC_FEED_PET", sizeof(struct PACKET_ZC_FEED_PET), SERVER_TYPE_MAP },
		{ "PACKET_ZC_FORMATSTRING_MSG", sizeof(struct PACKET_ZC_FORMATSTRING_MSG), SERVER_TYPE_MAP },
		{ "PACKET_ZC_FORMATSTRING_MSG_COLOR", sizeof(struct PACKET_ZC_FORMATSTRING_MSG_COLOR), SERVER_TYPE_MAP },
		{ "PACKET_ZC_GROUP_ISALIVE", sizeof(struct PACKET_ZC_GROUP_ISALIVE), SERVER_TYPE_MAP },
		{ "PACKET_ZC_GROUP_LIST", sizeof(struct PACKET_ZC_GROUP_LIST), SERVER_TYPE_MAP },
		{ "PACKET_ZC_GROUP_LIST_SUB", sizeof(struct PACKET_ZC_GROUP_LIST_SUB), SERVER_TYPE_MAP },
		{ "PACKET_ZC_GUILD_EMBLEM_IMG", sizeof(struct PACKET_ZC_GUILD_EMBLEM_IMG), SERVER_TYPE_MAP },
		{ "PACKET_ZC_GUILD_INFO", sizeof(struct PACKET_ZC_GUILD_INFO), SERVER_TYPE_MAP },
		{ "PACKET_ZC_GUILD_POSITION", sizeof(struct PACKET_ZC_GUILD_POSITION), SERVER_TYPE_MAP },
		{ "PACKET_ZC_GUILD_SKILLINFO", sizeof(struct PACKET_ZC_GUILD_SKILLINFO), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ITEM_ENTRY", sizeof(struct PACKET_ZC_ITEM_ENTRY), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ITEM_PICKUP_ACK", sizeof(struct PACKET_ZC_ITEM_PICKUP_ACK), SERVER_TYPE_MAP },
		{ "PACKET_ZC_LONGPAR_CHANGE", sizeof(struct PACKET_ZC_LONGPAR_CHANGE), SERVER_TYPE_MAP },
		{ "PACKET_ZC_MAIL_LIST", sizeof(struct PACKET_ZC_MAIL_LIST), SERVER_TYPE_MAP },
		{ "PACKET_ZC_MAKABLEITEMLIST", sizeof(struct PACKET_ZC_MAKABLEITEMLIST), SERVER_TYPE_MAP },
		{ "PACKET_ZC_MAKABLEITEMLIST_sub", sizeof(struct PACKET_ZC_MAKABLEITEMLIST_sub), SERVER_TYPE_MAP },
		{ "PACKET_ZC_MAKINGARROW_LIST", sizeof(struct PACKET_ZC_MAKINGARROW_LIST), SERVER_TYPE_MAP },
		{ "PACKET_ZC_MAKINGARROW_LIST_sub", sizeof(struct PACKET_ZC_MAKINGARROW_LIST_sub), SERVER_TYPE_MAP },
		{ "PACKET_ZC_MAKINGITEM_LIST", sizeof(struct PACKET_ZC_MAKINGITEM_LIST), SERVER_TYPE_MAP },
		{ "PACKET_ZC_MAKINGITEM_LIST_sub", sizeof(struct PACKET_ZC_MAKINGITEM_LIST_sub), SERVER_TYPE_MAP },
		{ "PACKET_ZC_MEMBERMGR_INFO", sizeof(struct PACKET_ZC_MEMBERMGR_INFO), SERVER_TYPE_MAP },
		{ "PACKET_ZC_MSG_COLOR", sizeof(struct PACKET_ZC_MSG_COLOR), SERVER_TYPE_MAP },
		{ "PACKET_ZC_MVP_GETTING_ITEM", sizeof(struct PACKET_ZC_MVP_GETTING_ITEM), SERVER_TYPE_MAP },
		{ "PACKET_ZC_MYGUILD_BASIC_INFO", sizeof(struct PACKET_ZC_MYGUILD_BASIC_INFO), SERVER_TYPE_MAP },
		{ "PACKET_ZC_MYITEMLIST_BUYING_STORE", sizeof(struct PACKET_ZC_MYITEMLIST_BUYING_STORE), SERVER_TYPE_MAP },
		{ "PACKET_ZC_MYITEMLIST_BUYING_STORE_sub", sizeof(struct PACKET_ZC_MYITEMLIST_BUYING_STORE_sub), SERVER_TYPE_MAP },
		{ "PACKET_ZC_NOTIFY_CARTITEM_COUNTINFO", sizeof(struct PACKET_ZC_NOTIFY_CARTITEM_COUNTINFO), SERVER_TYPE_MAP },
		{ "PACKET_ZC_NOTIFY_CHAT", sizeof(struct PACKET_ZC_NOTIFY_CHAT), SERVER_TYPE_MAP },
		{ "PACKET_ZC_NOTIFY_CHAT_PARTY", sizeof(struct PACKET_ZC_NOTIFY_CHAT_PARTY), SERVER_TYPE_MAP },
		{ "PACKET_ZC_NOTIFY_CLAN_CHAT", sizeof(struct PACKET_ZC_NOTIFY_CLAN_CHAT), SERVER_TYPE_MAP },
		{ "PACKET_ZC_NOTIFY_CLAN_CONNECTINFO", sizeof(struct PACKET_ZC_NOTIFY_CLAN_CONNECTINFO), SERVER_TYPE_MAP },
		{ "PACKET_ZC_NOTIFY_GROUNDSKILL", sizeof(struct PACKET_ZC_NOTIFY_GROUNDSKILL), SERVER_TYPE_MAP },
		{ "PACKET_ZC_NOTIFY_HP_TO_GROUPM", sizeof(struct PACKET_ZC_NOTIFY_HP_TO_GROUPM), SERVER_TYPE_MAP },
		{ "PACKET_ZC_NOTIFY_PLAYERCHAT", sizeof(struct PACKET_ZC_NOTIFY_PLAYERCHAT), SERVER_TYPE_MAP },
		{ "PACKET_ZC_NOTIFY_POSITION_TO_GROUPM", sizeof(struct PACKET_ZC_NOTIFY_POSITION_TO_GROUPM), SERVER_TYPE_MAP },
		{ "PACKET_ZC_NOTIFY_SKILL", sizeof(struct PACKET_ZC_NOTIFY_SKILL), SERVER_TYPE_MAP },
		{ "PACKET_ZC_NOTIFY_SKILL_POSITION", sizeof(struct PACKET_ZC_NOTIFY_SKILL_POSITION), SERVER_TYPE_MAP },
		{ "PACKET_ZC_NOTIFY_UNREADMAIL", sizeof(struct PACKET_ZC_NOTIFY_UNREADMAIL), SERVER_TYPE_MAP },
		{ "PACKET_ZC_NOTIFY_WEAPONITEMLIST", sizeof(struct PACKET_ZC_NOTIFY_WEAPONITEMLIST), SERVER_TYPE_MAP },
		{ "PACKET_ZC_NOTIFY_WEAPONITEMLIST_sub", sizeof(struct PACKET_ZC_NOTIFY_WEAPONITEMLIST_sub), SERVER_TYPE_MAP },
		{ "PACKET_ZC_NPC_MARKET_PURCHASE_RESULT_sub", sizeof(struct PACKET_ZC_NPC_MARKET_PURCHASE_RESULT_sub), SERVER_TYPE_MAP },
		{ "PACKET_ZC_OVERWEIGHT_PERCENT", sizeof(struct PACKET_ZC_OVERWEIGHT_PERCENT), SERVER_TYPE_MAP },
		{ "PACKET_ZC_PAR_CHANGE", sizeof(struct PACKET_ZC_PAR_CHANGE), SERVER_TYPE_MAP },
		{ "PACKET_ZC_PARTY_JOIN_REQ", sizeof(struct PACKET_ZC_PARTY_JOIN_REQ), SERVER_TYPE_MAP },
		{ "PACKET_ZC_PARTY_JOIN_REQ_ACK", sizeof(struct PACKET_ZC_PARTY_JOIN_REQ_ACK), SERVER_TYPE_MAP },
		{ "PACKET_ZC_PC_CASH_POINT_ITEMLIST", sizeof(struct PACKET_ZC_PC_CASH_POINT_ITEMLIST), SERVER_TYPE_MAP },
		{ "PACKET_ZC_PC_CASH_POINT_ITEMLIST_sub", sizeof(struct PACKET_ZC_PC_CASH_POINT_ITEMLIST_sub), SERVER_TYPE_MAP },
		{ "PACKET_ZC_PC_PURCHASE_ITEMLIST", sizeof(struct PACKET_ZC_PC_PURCHASE_ITEMLIST), SERVER_TYPE_MAP },
		{ "PACKET_ZC_PC_PURCHASE_ITEMLIST_FROMMC", sizeof(struct PACKET_ZC_PC_PURCHASE_ITEMLIST_FROMMC), SERVER_TYPE_MAP },
		{ "PACKET_ZC_PC_PURCHASE_ITEMLIST_FROMMC_sub", sizeof(struct PACKET_ZC_PC_PURCHASE_ITEMLIST_FROMMC_sub), SERVER_TYPE_MAP },
		{ "PACKET_ZC_PC_PURCHASE_ITEMLIST_sub", sizeof(struct PACKET_ZC_PC_PURCHASE_ITEMLIST_sub), SERVER_TYPE_MAP },
		{ "PACKET_ZC_PC_PURCHASE_MYITEMLIST", sizeof(struct PACKET_ZC_PC_PURCHASE_MYITEMLIST), SERVER_TYPE_MAP },
		{ "PACKET_ZC_PC_PURCHASE_MYITEMLIST_sub", sizeof(struct PACKET_ZC_PC_PURCHASE_MYITEMLIST_sub), SERVER_TYPE_MAP },
		{ "PACKET_ZC_POSITION_ID_NAME_INFO", sizeof(struct PACKET_ZC_POSITION_ID_NAME_INFO), SERVER_TYPE_MAP },
		{ "PACKET_ZC_POSITION_INFO", sizeof(struct PACKET_ZC_POSITION_INFO), SERVER_TYPE_MAP },
		{ "PACKET_ZC_PRIVATE_AIRSHIP_RESPONSE", sizeof(struct PACKET_ZC_PRIVATE_AIRSHIP_RESPONSE), SERVER_TYPE_MAP },
		{ "PACKET_ZC_PROPERTY_PET", sizeof(struct PACKET_ZC_PROPERTY_PET), SERVER_TYPE_MAP },
		{ "PACKET_ZC_REMOVE_EFFECT", sizeof(struct PACKET_ZC_REMOVE_EFFECT), SERVER_TYPE_MAP },
		{ "PACKET_ZC_REPAIRITEMLIST", sizeof(struct PACKET_ZC_REPAIRITEMLIST), SERVER_TYPE_MAP },
		{ "PACKET_ZC_REQ_WEAR_EQUIP_ACK", sizeof(struct PACKET_ZC_REQ_WEAR_EQUIP_ACK), SERVER_TYPE_MAP },
		{ "PACKET_ZC_ROLE_CHANGE", sizeof(struct PACKET_ZC_ROLE_CHANGE), SERVER_TYPE_MAP },
		{ "PACKET_ZC_SAY_DIALOG", sizeof(struct PACKET_ZC_SAY_DIALOG), SERVER_TYPE_MAP },
		{ "PACKET_ZC_SAY_DIALOG2", sizeof(struct PACKET_ZC_SAY_DIALOG2), SERVER_TYPE_MAP },
		{ "PACKET_ZC_SHOW_IMAGE", sizeof(struct PACKET_ZC_SHOW_IMAGE), SERVER_TYPE_MAP },
		{ "PACKET_ZC_SKILL_SCALE", sizeof(struct PACKET_ZC_SKILL_SCALE), SERVER_TYPE_MAP },
		{ "PACKET_ZC_SKILL_SELECT_REQUEST", sizeof(struct PACKET_ZC_SKILL_SELECT_REQUEST), SERVER_TYPE_MAP },
		{ "PACKET_ZC_SKILLINFO_LIST", sizeof(struct PACKET_ZC_SKILLINFO_LIST), SERVER_TYPE_MAP },
		{ "PACKET_ZC_SKILLINFO_UPDATE2", sizeof(struct PACKET_ZC_SKILLINFO_UPDATE2), SERVER_TYPE_MAP },
		{ "PACKET_ZC_SOUND", sizeof(struct PACKET_ZC_SOUND), SERVER_TYPE_MAP },
		{ "PACKET_ZC_SPIRITS", sizeof(struct PACKET_ZC_SPIRITS), SERVER_TYPE_MAP },
		{ "PACKET_ZC_SPIRITS2", sizeof(struct PACKET_ZC_SPIRITS2), SERVER_TYPE_MAP },
		{ "PACKET_ZC_SPRITE_CHANGE", sizeof(struct PACKET_ZC_SPRITE_CHANGE), SERVER_TYPE_MAP },
		{ "PACKET_ZC_START_CAPTURE", sizeof(struct PACKET_ZC_START_CAPTURE), SERVER_TYPE_MAP },
		{ "PACKET_ZC_STATE_CHANGE", sizeof(struct PACKET_ZC_STATE_CHANGE), SERVER_TYPE_MAP },
		{ "PACKET_ZC_STATUS_CHANGE", sizeof(struct PACKET_ZC_STATUS_CHANGE), SERVER_TYPE_MAP },
		{ "PACKET_ZC_STATUS_CHANGE_ACK", sizeof(struct PACKET_ZC_STATUS_CHANGE_ACK), SERVER_TYPE_MAP },
		{ "PACKET_ZC_STORE_ENTRY", sizeof(struct PACKET_ZC_STORE_ENTRY), SERVER_TYPE_MAP },
		{ "PACKET_ZC_STYLE_CHANGE_RES", sizeof(struct PACKET_ZC_STYLE_CHANGE_RES), SERVER_TYPE_MAP },
		{ "PACKET_ZC_TALKBOX_CHATCONTENTS", sizeof(struct PACKET_ZC_TALKBOX_CHATCONTENTS), SERVER_TYPE_MAP },
		{ "PACKET_ZC_TRYCAPTURE_MONSTER", sizeof(struct PACKET_ZC_TRYCAPTURE_MONSTER), SERVER_TYPE_MAP },
		{ "PACKET_ZC_UI_ACTION", sizeof(struct PACKET_ZC_UI_ACTION), SERVER_TYPE_MAP },
		{ "PACKET_ZC_UPDATE_GDID", sizeof(struct PACKET_ZC_UPDATE_GDID), SERVER_TYPE_MAP },
		{ "PACKET_ZC_UPDATE_ITEM_FROM_BUYING_STORE", sizeof(struct PACKET_ZC_UPDATE_ITEM_FROM_BUYING_STORE), SERVER_TYPE_MAP },
		{ "PACKET_ZC_USE_ITEM_ACK", sizeof(struct PACKET_ZC_USE_ITEM_ACK), SERVER_TYPE_MAP },
		{ "PACKET_ZC_USE_SKILL", sizeof(struct PACKET_ZC_USE_SKILL), SERVER_TYPE_MAP },
		{ "PACKET_ZC_WAIT_DIALOG", sizeof(struct PACKET_ZC_WAIT_DIALOG), SERVER_TYPE_MAP },
		{ "PACKET_ZC_WAIT_DIALOG2", sizeof(struct PACKET_ZC_WAIT_DIALOG2), SERVER_TYPE_MAP },
		{ "PACKET_ZC_WARPLIST", sizeof(struct PACKET_ZC_WARPLIST), SERVER_TYPE_MAP },
		{ "PACKET_ZC_WARPLIST_sub", sizeof(struct PACKET_ZC_WARPLIST_sub), SERVER_TYPE_MAP },
		{ "PACKET_ZC_WHISPER", sizeof(struct PACKET_ZC_WHISPER), SERVER_TYPE_MAP },
		{ "PACKET_ZC_WRITE_MAIL_RESULT", sizeof(struct PACKET_ZC_WRITE_MAIL_RESULT), SERVER_TYPE_MAP },
		{ "RELATED_GUILD_INFO", sizeof(struct RELATED_GUILD_INFO), SERVER_TYPE_MAP },
		{ "REPAIRITEM_INFO1", sizeof(struct REPAIRITEM_INFO1), SERVER_TYPE_MAP },
		{ "SKILLDATA", sizeof(struct SKILLDATA), SERVER_TYPE_MAP },
		{ "ZC_PROGRESS_ACTOR", sizeof(struct ZC_PROGRESS_ACTOR), SERVER_TYPE_MAP },
		{ "ZC_STORE_ITEMLIST_EQUIP", sizeof(struct ZC_STORE_ITEMLIST_EQUIP), SERVER_TYPE_MAP },
		{ "ZC_STORE_ITEMLIST_NORMAL", sizeof(struct ZC_STORE_ITEMLIST_NORMAL), SERVER_TYPE_MAP },
		{ "ach_list_info", sizeof(struct ach_list_info), SERVER_TYPE_MAP },
		{ "maillistinfo", sizeof(struct maillistinfo), SERVER_TYPE_MAP },
		{ "packet_ZC_REFUSE_LOGIN", sizeof(struct packet_ZC_REFUSE_LOGIN), SERVER_TYPE_MAP },
		{ "packet_achievement_list", sizeof(struct packet_achievement_list), SERVER_TYPE_MAP },
		{ "packet_achievement_reward_ack", sizeof(struct packet_achievement_reward_ack), SERVER_TYPE_MAP },
		{ "packet_achievement_update", sizeof(struct packet_achievement_update), SERVER_TYPE_MAP },
		{ "packet_authok", sizeof(struct packet_authok), SERVER_TYPE_MAP },
		{ "packet_banking_check", sizeof(struct packet_banking_check), SERVER_TYPE_MAP },
		{ "packet_banking_deposit_ack", sizeof(struct packet_banking_deposit_ack), SERVER_TYPE_MAP },
		{ "packet_banking_deposit_req", sizeof(struct packet_banking_deposit_req), SERVER_TYPE_MAP },
		{ "packet_banking_withdraw_ack", sizeof(struct packet_banking_withdraw_ack), SERVER_TYPE_MAP },
		{ "packet_banking_withdraw_req", sizeof(struct packet_banking_withdraw_req), SERVER_TYPE_MAP },
		{ "packet_bgqueue_ack", sizeof(struct packet_bgqueue_ack), SERVER_TYPE_MAP },
		{ "packet_bgqueue_battlebegin_ack", sizeof(struct packet_bgqueue_battlebegin_ack), SERVER_TYPE_MAP },
		{ "packet_bgqueue_battlebegins", sizeof(struct packet_bgqueue_battlebegins), SERVER_TYPE_MAP },
		{ "packet_bgqueue_checkstate", sizeof(struct packet_bgqueue_checkstate), SERVER_TYPE_MAP },
		{ "packet_bgqueue_notice_delete", sizeof(struct packet_bgqueue_notice_delete), SERVER_TYPE_MAP },
		{ "packet_bgqueue_notify_entry", sizeof(struct packet_bgqueue_notify_entry), SERVER_TYPE_MAP },
		{ "packet_bgqueue_register", sizeof(struct packet_bgqueue_register), SERVER_TYPE_MAP },
		{ "packet_bgqueue_revoke_req", sizeof(struct packet_bgqueue_revoke_req), SERVER_TYPE_MAP },
		{ "packet_bgqueue_update_info", sizeof(struct packet_bgqueue_update_info), SERVER_TYPE_MAP },
		{ "packet_cart_additem_ack", sizeof(struct packet_cart_additem_ack), SERVER_TYPE_MAP },
		{ "packet_chat_message", sizeof(struct packet_chat_message), SERVER_TYPE_MAP },
		{ "packet_damage", sizeof(struct packet_damage), SERVER_TYPE_MAP },
		{ "packet_dropflooritem", sizeof(struct packet_dropflooritem), SERVER_TYPE_MAP },
		{ "packet_equip_item", sizeof(struct packet_equip_item), SERVER_TYPE_MAP },
		{ "packet_gm_monster_item", sizeof(struct packet_gm_monster_item), SERVER_TYPE_MAP },
		{ "packet_graffiti_entry", sizeof(struct packet_graffiti_entry), SERVER_TYPE_MAP },
		{ "packet_idle_unit", sizeof(struct packet_idle_unit), SERVER_TYPE_MAP },
		{ "packet_idle_unit2", sizeof(struct packet_idle_unit2), SERVER_TYPE_MAP },
		{ "packet_item_drop_announce", sizeof(struct packet_item_drop_announce), SERVER_TYPE_MAP },
		{ "packet_itemlist_equip", sizeof(struct packet_itemlist_equip), SERVER_TYPE_MAP },
		{ "packet_itemlist_normal", sizeof(struct packet_itemlist_normal), SERVER_TYPE_MAP },
		{ "packet_maptypeproperty2", sizeof(struct packet_maptypeproperty2), SERVER_TYPE_MAP },
		{ "packet_mission_info_sub", sizeof(struct packet_mission_info_sub), SERVER_TYPE_MAP },
		{ "packet_monster_hp", sizeof(struct packet_monster_hp), SERVER_TYPE_MAP },
		{ "packet_notify_bounditem", sizeof(struct packet_notify_bounditem), SERVER_TYPE_MAP },
		{ "packet_party_leader_changed", sizeof(struct packet_party_leader_changed), SERVER_TYPE_MAP },
		{ "packet_quest_add_header", sizeof(struct packet_quest_add_header), SERVER_TYPE_MAP },
		{ "packet_quest_hunt_info", sizeof(struct packet_quest_hunt_info), SERVER_TYPE_MAP },
		{ "packet_quest_hunt_info_sub", sizeof(struct packet_quest_hunt_info_sub), SERVER_TYPE_MAP },
		{ "packet_quest_hunt_sub", sizeof(struct packet_quest_hunt_sub), SERVER_TYPE_MAP },
		{ "packet_quest_list_header", sizeof(struct packet_quest_list_header), SERVER_TYPE_MAP },
		{ "packet_quest_list_info", sizeof(struct packet_quest_list_info), SERVER_TYPE_MAP },
		{ "packet_quest_update_header", sizeof(struct packet_quest_update_header), SERVER_TYPE_MAP },
		{ "packet_quest_update_hunt", sizeof(struct packet_quest_update_hunt), SERVER_TYPE_MAP },
		{ "packet_reqname_ack", sizeof(struct packet_reqname_ack), SERVER_TYPE_MAP },
		{ "packet_roulette_close_ack", sizeof(struct packet_roulette_close_ack), SERVER_TYPE_MAP },
		{ "packet_roulette_generate_ack", sizeof(struct packet_roulette_generate_ack), SERVER_TYPE_MAP },
		{ "packet_roulette_info_ack", sizeof(struct packet_roulette_info_ack), SERVER_TYPE_MAP },
		{ "packet_roulette_itemrecv_ack", sizeof(struct packet_roulette_itemrecv_ack), SERVER_TYPE_MAP },
		{ "packet_roulette_itemrecv_req", sizeof(struct packet_roulette_itemrecv_req), SERVER_TYPE_MAP },
		{ "packet_roulette_open_ack", sizeof(struct packet_roulette_open_ack), SERVER_TYPE_MAP },
		{ "packet_sc_notick", sizeof(struct packet_sc_notick), SERVER_TYPE_MAP },
		{ "packet_script_clear", sizeof(struct packet_script_clear), SERVER_TYPE_MAP },
		{ "packet_skill_entry", sizeof(struct packet_skill_entry), SERVER_TYPE_MAP },
		{ "packet_spawn_unit", sizeof(struct packet_spawn_unit), SERVER_TYPE_MAP },
		{ "packet_spawn_unit2", sizeof(struct packet_spawn_unit2), SERVER_TYPE_MAP },
		{ "packet_status_change", sizeof(struct packet_status_change), SERVER_TYPE_MAP },
		{ "packet_status_change2", sizeof(struct packet_status_change2), SERVER_TYPE_MAP },
		{ "packet_status_change_end", sizeof(struct packet_status_change_end), SERVER_TYPE_MAP },
		{ "packet_unequipitem_ack", sizeof(struct packet_unequipitem_ack), SERVER_TYPE_MAP },
		{ "packet_unit_walking", sizeof(struct packet_unit_walking), SERVER_TYPE_MAP },
		{ "packet_whisper_message", sizeof(struct packet_whisper_message), SERVER_TYPE_MAP },
		{ "packet_wis_end", sizeof(struct packet_wis_end), SERVER_TYPE_MAP },
		{ "pet_evolution_items", sizeof(struct pet_evolution_items), SERVER_TYPE_MAP },
	#else
		#define MAP_PACKETS_STRUCT_H
	#endif // MAP_PACKETS_STRUCT_H
	#ifdef MAP_PARTY_H
		{ "party_booking_ad_info", sizeof(struct party_booking_ad_info), SERVER_TYPE_MAP },
		{ "party_booking_detail", sizeof(struct party_booking_detail), SERVER_TYPE_MAP },
		{ "party_data", sizeof(struct party_data), SERVER_TYPE_MAP },
		{ "party_interface", sizeof(struct party_interface), SERVER_TYPE_MAP },
		{ "party_member_data", sizeof(struct party_member_data), SERVER_TYPE_MAP },
	#else
		#define MAP_PARTY_H
	#endif // MAP_PARTY_H
	#ifdef MAP_PATH_H
		{ "path_interface", sizeof(struct path_interface), SERVER_TYPE_MAP },
		{ "shootpath_data", sizeof(struct shootpath_data), SERVER_TYPE_MAP },
		{ "walkpath_data", sizeof(struct walkpath_data), SERVER_TYPE_MAP },
	#else
		#define MAP_PATH_H
	#endif // MAP_PATH_H
	#ifdef MAP_PC_GROUPS_H
		{ "GroupSettings", sizeof(struct GroupSettings), SERVER_TYPE_MAP },
		{ "pc_groups_interface", sizeof(struct pc_groups_interface), SERVER_TYPE_MAP },
		{ "pc_groups_new_permission", sizeof(struct pc_groups_new_permission), SERVER_TYPE_MAP },
		{ "pc_groups_permission_table", sizeof(struct pc_groups_permission_table), SERVER_TYPE_MAP },
	#else
		#define MAP_PC_GROUPS_H
	#endif // MAP_PC_GROUPS_H
	#ifdef MAP_PC_H
		{ "autocast_data", sizeof(struct autocast_data), SERVER_TYPE_MAP },
		{ "autotrade_vending", sizeof(struct autotrade_vending), SERVER_TYPE_MAP },
		{ "class_exp_group", sizeof(struct class_exp_group), SERVER_TYPE_MAP },
		{ "class_exp_tables", sizeof(struct class_exp_tables), SERVER_TYPE_MAP },
		{ "item_cd", sizeof(struct item_cd), SERVER_TYPE_MAP },
		{ "map_session_data", sizeof(struct map_session_data), SERVER_TYPE_MAP },
		{ "pc_combos", sizeof(struct pc_combos), SERVER_TYPE_MAP },
		{ "pc_interface", sizeof(struct pc_interface), SERVER_TYPE_MAP },
		{ "s_add_drop", sizeof(struct s_add_drop), SERVER_TYPE_MAP },
		{ "s_addeffect", sizeof(struct s_addeffect), SERVER_TYPE_MAP },
		{ "s_addeffectonskill", sizeof(struct s_addeffectonskill), SERVER_TYPE_MAP },
		{ "s_autobonus", sizeof(struct s_autobonus), SERVER_TYPE_MAP },
		{ "s_autospell", sizeof(struct s_autospell), SERVER_TYPE_MAP },
		{ "sg_data", sizeof(struct sg_data), SERVER_TYPE_MAP },
		{ "skill_tree_entry", sizeof(struct skill_tree_entry), SERVER_TYPE_MAP },
		{ "skill_tree_requirement", sizeof(struct skill_tree_requirement), SERVER_TYPE_MAP },
		{ "weapon_data", sizeof(struct weapon_data), SERVER_TYPE_MAP },
	#else
		#define MAP_PC_H
	#endif // MAP_PC_H
	#ifdef MAP_PET_H
		{ "pet_bonus", sizeof(struct pet_bonus), SERVER_TYPE_MAP },
		{ "pet_data", sizeof(struct pet_data), SERVER_TYPE_MAP },
		{ "pet_evolve_data", sizeof(struct pet_evolve_data), SERVER_TYPE_MAP },
		{ "pet_interface", sizeof(struct pet_interface), SERVER_TYPE_MAP },
		{ "pet_loot", sizeof(struct pet_loot), SERVER_TYPE_MAP },
		{ "pet_recovery", sizeof(struct pet_recovery), SERVER_TYPE_MAP },
		{ "pet_skill_attack", sizeof(struct pet_skill_attack), SERVER_TYPE_MAP },
		{ "pet_skill_support", sizeof(struct pet_skill_support), SERVER_TYPE_MAP },
		{ "s_pet_db", sizeof(struct s_pet_db), SERVER_TYPE_MAP },
	#else
		#define MAP_PET_H
	#endif // MAP_PET_H
	#ifdef MAP_QUEST_H
		{ "quest_db", sizeof(struct quest_db), SERVER_TYPE_MAP },
		{ "quest_dropitem", sizeof(struct quest_dropitem), SERVER_TYPE_MAP },
		{ "quest_interface", sizeof(struct quest_interface), SERVER_TYPE_MAP },
		{ "quest_objective", sizeof(struct quest_objective), SERVER_TYPE_MAP },
		{ "questinfo", sizeof(struct questinfo), SERVER_TYPE_MAP },
		{ "questinfo_itemreq", sizeof(struct questinfo_itemreq), SERVER_TYPE_MAP },
		{ "questinfo_qreq", sizeof(struct questinfo_qreq), SERVER_TYPE_MAP },
	#else
		#define MAP_QUEST_H
	#endif // MAP_QUEST_H
	#ifdef MAP_REFINE_H
		{ "refine_interface", sizeof(struct refine_interface), SERVER_TYPE_MAP },
		{ "s_refine_requirement", sizeof(struct s_refine_requirement), SERVER_TYPE_MAP },
	#else
		#define MAP_REFINE_H
	#endif // MAP_REFINE_H
	#ifdef MAP_REFINE_P_H
		{ "refine_interface_dbs", sizeof(struct refine_interface_dbs), SERVER_TYPE_MAP },
		{ "refine_interface_private", sizeof(struct refine_interface_private), SERVER_TYPE_MAP },
		{ "s_refine_info", sizeof(struct s_refine_info), SERVER_TYPE_MAP },
	#else
		#define MAP_REFINE_P_H
	#endif // MAP_REFINE_P_H
	#ifdef MAP_RODEX_H
		{ "rodex_interface", sizeof(struct rodex_interface), SERVER_TYPE_MAP },
	#else
		#define MAP_RODEX_H
	#endif // MAP_RODEX_H
	#ifdef MAP_SCRIPT_H
		{ "Script_Config", sizeof(struct Script_Config), SERVER_TYPE_MAP },
		{ "casecheck_data", sizeof(struct casecheck_data), SERVER_TYPE_MAP },
		{ "reg_db", sizeof(struct reg_db), SERVER_TYPE_MAP },
		{ "script_array", sizeof(struct script_array), SERVER_TYPE_MAP },
		{ "script_buf", sizeof(struct script_buf), SERVER_TYPE_MAP },
		{ "script_code", sizeof(struct script_code), SERVER_TYPE_MAP },
		{ "script_data", sizeof(struct script_data), SERVER_TYPE_MAP },
		{ "script_function", sizeof(struct script_function), SERVER_TYPE_MAP },
		{ "script_interface", sizeof(struct script_interface), SERVER_TYPE_MAP },
		{ "script_label_entry", sizeof(struct script_label_entry), SERVER_TYPE_MAP },
		{ "script_queue", sizeof(struct script_queue), SERVER_TYPE_MAP },
		{ "script_queue_iterator", sizeof(struct script_queue_iterator), SERVER_TYPE_MAP },
		{ "script_retinfo", sizeof(struct script_retinfo), SERVER_TYPE_MAP },
		{ "script_stack", sizeof(struct script_stack), SERVER_TYPE_MAP },
		{ "script_state", sizeof(struct script_state), SERVER_TYPE_MAP },
		{ "script_string_buf", sizeof(struct script_string_buf), SERVER_TYPE_MAP },
		{ "script_syntax_data", sizeof(struct script_syntax_data), SERVER_TYPE_MAP },
		{ "str_data_struct", sizeof(struct str_data_struct), SERVER_TYPE_MAP },
		{ "string_translation", sizeof(struct string_translation), SERVER_TYPE_MAP },
		{ "string_translation_entry", sizeof(struct string_translation_entry), SERVER_TYPE_MAP },
	#else
		#define MAP_SCRIPT_H
	#endif // MAP_SCRIPT_H
	#ifdef MAP_SEARCHSTORE_H
		{ "s_search_store_info", sizeof(struct s_search_store_info), SERVER_TYPE_MAP },
		{ "s_search_store_info_item", sizeof(struct s_search_store_info_item), SERVER_TYPE_MAP },
		{ "searchstore_interface", sizeof(struct searchstore_interface), SERVER_TYPE_MAP },
	#else
		#define MAP_SEARCHSTORE_H
	#endif // MAP_SEARCHSTORE_H
	#ifdef MAP_SKILL_H
		{ "s_autospell_db", sizeof(struct s_autospell_db), SERVER_TYPE_MAP },
		{ "s_skill_abra_db", sizeof(struct s_skill_abra_db), SERVER_TYPE_MAP },
		{ "s_skill_arrow_db", sizeof(struct s_skill_arrow_db), SERVER_TYPE_MAP },
		{ "s_skill_changematerial_db", sizeof(struct s_skill_changematerial_db), SERVER_TYPE_MAP },
		{ "s_skill_db", sizeof(struct s_skill_db), SERVER_TYPE_MAP },
		{ "s_skill_dbs", sizeof(struct s_skill_dbs), SERVER_TYPE_MAP },
		{ "s_skill_improvise_db", sizeof(struct s_skill_improvise_db), SERVER_TYPE_MAP },
		{ "s_skill_magicmushroom_db", sizeof(struct s_skill_magicmushroom_db), SERVER_TYPE_MAP },
		{ "s_skill_produce_db", sizeof(struct s_skill_produce_db), SERVER_TYPE_MAP },
		{ "s_skill_spellbook_db", sizeof(struct s_skill_spellbook_db), SERVER_TYPE_MAP },
		{ "s_skill_unit_layout", sizeof(struct s_skill_unit_layout), SERVER_TYPE_MAP },
		{ "skill_cd", sizeof(struct skill_cd), SERVER_TYPE_MAP },
		{ "skill_cd_entry", sizeof(struct skill_cd_entry), SERVER_TYPE_MAP },
		{ "skill_condition", sizeof(struct skill_condition), SERVER_TYPE_MAP },
		{ "skill_interface", sizeof(struct skill_interface), SERVER_TYPE_MAP },
		{ "skill_required_item_data", sizeof(struct skill_required_item_data), SERVER_TYPE_MAP },
		{ "skill_timerskill", sizeof(struct skill_timerskill), SERVER_TYPE_MAP },
		{ "skill_unit", sizeof(struct skill_unit), SERVER_TYPE_MAP },
		{ "skill_unit_group", sizeof(struct skill_unit_group), SERVER_TYPE_MAP },
		{ "skill_unit_group_tickset", sizeof(struct skill_unit_group_tickset), SERVER_TYPE_MAP },
		{ "skill_unit_save", sizeof(struct skill_unit_save), SERVER_TYPE_MAP },
	#else
		#define MAP_SKILL_H
	#endif // MAP_SKILL_H
	#ifdef MAP_STATUS_H
		{ "regen_data", sizeof(struct regen_data), SERVER_TYPE_MAP },
		{ "regen_data_sub", sizeof(struct regen_data_sub), SERVER_TYPE_MAP },
		{ "s_maxhp_entry", sizeof(struct s_maxhp_entry), SERVER_TYPE_MAP },
		{ "s_status_dbs", sizeof(struct s_status_dbs), SERVER_TYPE_MAP },
		{ "s_unit_params", sizeof(struct s_unit_params), SERVER_TYPE_MAP },
		{ "sc_display_entry", sizeof(struct sc_display_entry), SERVER_TYPE_MAP },
		{ "status_change", sizeof(struct status_change), SERVER_TYPE_MAP },
		{ "status_change_entry", sizeof(struct status_change_entry), SERVER_TYPE_MAP },
		{ "status_data", sizeof(struct status_data), SERVER_TYPE_MAP },
		{ "status_interface", sizeof(struct status_interface), SERVER_TYPE_MAP },
		{ "weapon_atk", sizeof(struct weapon_atk), SERVER_TYPE_MAP },
	#else
		#define MAP_STATUS_H
	#endif // MAP_STATUS_H
	#ifdef MAP_STORAGE_H
		{ "guild_storage_interface", sizeof(struct guild_storage_interface), SERVER_TYPE_MAP },
		{ "storage_interface", sizeof(struct storage_interface), SERVER_TYPE_MAP },
	#else
		#define MAP_STORAGE_H
	#endif // MAP_STORAGE_H
	#ifdef MAP_STYLIST_H
		{ "stylist_data_entry", sizeof(struct stylist_data_entry), SERVER_TYPE_MAP },
		{ "stylist_interface", sizeof(struct stylist_interface), SERVER_TYPE_MAP },
	#else
		#define MAP_STYLIST_H
	#endif // MAP_STYLIST_H
	#ifdef MAP_TRADE_H
		{ "trade_interface", sizeof(struct trade_interface), SERVER_TYPE_MAP },
	#else
		#define MAP_TRADE_H
	#endif // MAP_TRADE_H
	#ifdef MAP_UNIT_H
		{ "unit_data", sizeof(struct unit_data), SERVER_TYPE_MAP },
		{ "unit_interface", sizeof(struct unit_interface), SERVER_TYPE_MAP },
		{ "view_data", sizeof(struct view_data), SERVER_TYPE_MAP },
	#else
		#define MAP_UNIT_H
	#endif // MAP_UNIT_H
	#ifdef MAP_VENDING_H
		{ "s_vending", sizeof(struct s_vending), SERVER_TYPE_MAP },
		{ "vending_interface", sizeof(struct vending_interface), SERVER_TYPE_MAP },
	#else
		#define MAP_VENDING_H
	#endif // MAP_VENDING_H
};
HPExport unsigned int HPMDataCheckLen = ARRAYLENGTH(HPMDataCheck);
HPExport int HPMDataCheckVer = 1;

#endif /* HPM_DATA_CHECK_H */
