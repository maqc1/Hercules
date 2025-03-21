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
#ifndef LOGIN_ACCOUNT_H
#define LOGIN_ACCOUNT_H

#include "common/cbasetypes.h"
#include "common/mmo.h" // ACCOUNT_REG2_NUM

/* Forward declarations */
struct Sql; // common/sql.h

/* Forward Declarations */
struct config_t; // common/conf.h

typedef struct AccountDB AccountDB;
typedef struct AccountDBIterator AccountDBIterator;

struct mmo_account
{
	int account_id;
	char userid[NAME_LENGTH];
	char pass[32+1];            // 23+1 for plaintext, 32+1 for md5-ed passwords
	char sex;                   // gender (M/F/S)
	char email[40];             // e-mail (by default: a@a.com)
	int group_id;               // player group id
	uint8 char_slots;           // this accounts maximum character slots (maximum is limited to MAX_CHARS define in char server)
	unsigned int state;         // packet 0x006a value + 1 (0: complete OK)
	time_t unban_time;          // (timestamp): ban time limit of the account (0 = no ban)
	time_t expiration_time;     // (timestamp): validity limit of the account (0 = unlimited)
	unsigned int logincount;    // number of successful auth attempts
	unsigned int pincode_change;// (timestamp): last time of pincode change
	char pincode[4+1];          // pincode value
	char lastlogin[24];         // date+time of last successful login
	char last_ip[16];           // save of last IP of connection
	char birthdate[10+1];       // assigned birth date (format: YYYY-MM-DD, default: 0000-00-00)
};

struct AccountDBIterator
{
	/// Destroys this iterator, releasing all allocated memory (including itself).
	///
	/// @param self Iterator
	void (*destroy)(AccountDBIterator* self);

	/// Fetches the next account in the database.
	/// Fills acc with the account data.
	/// @param self Iterator
	/// @param acc Account data
	/// @return true if successful
	bool (*next)(AccountDBIterator* self, struct mmo_account* acc);
};

struct Account_engine {
	AccountDB* (*constructor)(void);
	AccountDB* db;
};

struct AccountDB
{
	/// Initializes this database, making it ready for use.
	/// Call this after setting the properties.
	///
	/// @param self Database
	/// @return true if successful
	bool (*init)(AccountDB* self);

	/// Destroys this database, releasing all allocated memory (including itself).
	///
	/// @param self Database
	void (*destroy)(AccountDB* self);

	/// Gets a property from this database.
	/// These read-only properties must be implemented:
	/// "engine.name" -> "txt", "sql", ...
	/// "engine.version" -> internal version
	/// "engine.comment" -> anything (suggestion: description or specs of the engine)
	///
	/// @param self Database
	/// @param key Property name
	/// @param buf Buffer for the value
	/// @param buflen Buffer length
	/// @return true if successful
	bool (*get_property)(AccountDB* self, const char* key, char* buf, size_t buflen);

	/// Sets a property in this database.
	///
	/// @param self Database
	/// @param config Configuration node
	/// @return true if successful
	bool (*set_property)(AccountDB* self, struct config_t *config, bool imported);

	/// Creates a new account in this database.
	/// If acc->account_id is not -1, the provided value will be used.
	/// Otherwise the account_id will be auto-generated and written to acc->account_id.
	///
	/// @param self Database
	/// @param acc Account data
	/// @return true if successful
	bool (*create)(AccountDB* self, struct mmo_account* acc);

	/// Removes an account from this database.
	///
	/// @param self Database
	/// @param account_id Account id
	/// @return true if successful
	bool (*remove)(AccountDB* self, const int account_id);

	/// Modifies the data of an existing account.
	/// Uses acc->account_id to identify the account.
	///
	/// @param self Database
	/// @param acc Account data
	/// @return true if successful
	bool (*save)(AccountDB* self, const struct mmo_account* acc);

	/// Finds an account with account_id and copies it to acc.
	///
	/// @param self Database
	/// @param acc Pointer that receives the account data
	/// @param account_id Target account id
	/// @return true if successful
	bool (*load_num)(AccountDB* self, struct mmo_account* acc, const int account_id);

	/// Finds an account with userid and copies it to acc.
	///
	/// @param self Database
	/// @param acc Pointer that receives the account data
	/// @param userid Target username
	/// @return true if successful
	bool (*load_str)(AccountDB* self, struct mmo_account* acc, const char* userid);

	/// Returns a new forward iterator.
	///
	/// @param self Database
	/// @return Iterator
	AccountDBIterator* (*iterator)(AccountDB* self);
};

typedef struct AccountDB_SQL
{
	AccountDB vtable;    // public interface

	struct Sql *accounts; // SQL accounts storage

	// Sql settings
	char   db_hostname[32];
	uint16 db_port;
	char   db_username[32];
	char   db_password[100];
	char   db_database[32];
	char   codepage[32];
	// other settings
	bool case_sensitive;
	char account_db[32];
	char global_acc_reg_num_db[32];
	char global_acc_reg_str_db[32];
} AccountDB_SQL;

/// internal structure
typedef struct AccountDBIterator_SQL
{
	AccountDBIterator vtable;    // public interface

	AccountDB_SQL* db;
	int last_account_id;
} AccountDBIterator_SQL;

/**
 * Account.c Interface
 **/
struct account_interface {
	struct Sql* (*db_sql_up) (AccountDB* self);
	void (*mmo_send_accreg2) (AccountDB* self, int fd, int account_id, int char_id);
	void (*mmo_save_accreg2) (AccountDB* self, int fd, int account_id, int char_id);
	bool (*mmo_auth_fromsql) (AccountDB_SQL* db, struct mmo_account* acc, int account_id);
	bool (*mmo_auth_tosql) (AccountDB_SQL* db, const struct mmo_account* acc, bool is_new);

	AccountDB* (*db_sql) (void);
	bool (*db_sql_init) (AccountDB* self);
	void (*db_sql_destroy) (AccountDB* self);
	bool (*db_sql_get_property) (AccountDB* self, const char* key, char* buf, size_t buflen);
	bool (*db_sql_set_property) (AccountDB* self, struct config_t *config, bool imported);
	bool (*db_sql_create) (AccountDB* self, struct mmo_account* acc);
	bool (*db_sql_remove) (AccountDB* self, const int account_id);
	bool (*db_sql_save) (AccountDB* self, const struct mmo_account* acc);
	bool (*db_sql_load_num) (AccountDB* self, struct mmo_account* acc, const int account_id);
	bool (*db_sql_load_str) (AccountDB* self, struct mmo_account* acc, const char* userid);
	AccountDBIterator* (*db_sql_iterator) (AccountDB* self);
	void (*db_sql_iter_destroy) (AccountDBIterator* self);
	bool (*db_sql_iter_next) (AccountDBIterator* self, struct mmo_account* acc);

	bool (*db_read_inter) (AccountDB_SQL *db, const char *filename, bool imported);
};

#ifdef HERCULES_CORE
void account_defaults(void);
#endif // HERCULES_CORE

HPShared struct account_interface *account;

#endif /* LOGIN_ACCOUNT_H */
