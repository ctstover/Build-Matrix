/*
    Copyright 2013 Stover Enterprises, LLC (An Alabama Limited Liability Corporation) 
    Written by C. Thomas Stover

    This file is part of the program Build Matrix.
    See http://buildmatrix.stoverenterprises.com for more information.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "prototypes.h"


int resolve_user_id(struct buildmatrix_context *ctx)
{
 const char *sql, *user;
 char *value;
 int code, create = 0;
 struct sqlite3_stmt *statement = NULL;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: resolve_user_id(%s)\n",
          ctx->error_prefix, ctx->user);

 ctx->user_id = -1;

 if(ctx->user == NULL)
 {
  fprintf(stderr, "%s: resolve_user_id(): no user name\n",
           ctx->error_prefix);
  return 1;
 }

 if(open_database(ctx))
  return 1;

 /* first see check the aliases table */

 sql = "SELECT user_id FROM aliases WHERE name = ?;";

 if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
 {
  fprintf(stderr, 
          "%s: resolve_user_id(): sqlite3_prepare(%s) failed, '%s'\n",
          ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx));
  return 1;
 }

 if(sqlite3_bind_text(statement, 1, ctx->user, strlen(ctx->user),
                      SQLITE_TRANSIENT) != SQLITE_OK)
 {
  fprintf(stderr, "%s: resolve_user_id(): sqlite3_bind_text() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 code = sqlite3_step(statement);

 if(code == SQLITE_ROW)
 {
  if(sqlite3_column_type(statement, 0) != SQLITE_INTEGER)
  {
   fprintf(stderr, "%s: resolve_user_id(): aliases.user_id is not an integer\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }

  ctx->user_id = sqlite3_column_int(statement, 0);
 }

 sqlite3_finalize(statement);

 if(ctx->user_id > 0)
 {
  /* the alias gave us the user_id, now we need the user name */  
  sql = "SELECT name FROM users WHERE user_id = ?";

  if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
  {
   fprintf(stderr, 
           "%s: resolve_user_id(): sqlite3_prepare(%s) failed, '%s'\n",
           ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx));
   return 1;
  }

  if(sqlite3_bind_int(statement, 1, ctx->user_id) != SQLITE_OK)
  {
   fprintf(stderr, "%s: resolve_user_id(): sqlite3_bind_int() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }

  code = sqlite3_step(statement);

  if(code == SQLITE_ROW)
  {
   if(sqlite3_column_type(statement, 0) != SQLITE_TEXT)
   {
    fprintf(stderr, "%s: resolve_user_id(): name is not text\n",
            ctx->error_prefix);
    sqlite3_finalize(statement);
    return 1;
   }
   user = (const char *) sqlite3_column_text(statement, 0);

   if(ctx->verbose > 1)
    fprintf(stderr, "%s: resolve_user_id(): %s -> %s.\n", ctx->error_prefix, ctx->user, user);
 
   free(ctx->user);
   ctx->user = strdup(user);
  }

 } else {
  /* no alias was used. We still have a username, but no user_id */
  sql = "SELECT user_id FROM users WHERE name = ?;";

  if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
  {
   fprintf(stderr, 
           "%s: resolve_user_id(): sqlite3_prepare(%s) failed, '%s'\n",
           ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx));
   return 1;
  }

  if(sqlite3_bind_text(statement, 1, ctx->user, strlen(ctx->user),
                       SQLITE_TRANSIENT) != SQLITE_OK)
  {
   fprintf(stderr, "%s: resolve_user_id(): sqlite3_bind_text() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }

  code = sqlite3_step(statement);

  if(code == SQLITE_ROW)
  {
   if(sqlite3_column_type(statement, 0) != SQLITE_INTEGER)
   {
    fprintf(stderr, "%s: resolve_user_id(): users.user_id is not an integer\n",
            ctx->error_prefix);
    sqlite3_finalize(statement);
    return 1;
   }
  
   ctx->user_id = sqlite3_column_int(statement, 0);
  }
 }

 sqlite3_finalize(statement);

 code = 1;
 if(ctx->mode == BLDMTRX_MODE_SERVE)
  code = 0;

 if(ctx->mode == BLDMTRX_MODE_SHOW_BUILD)
  code = 0;

 if(ctx->mode == BLDMTRX_MODE_PULL)
  code = 0;

 if(code == 1)
  return ctx->user_id < 1;

 code = 0;
 if(ctx->user_id < 1)
 {
  /* Still no user_id has been found. Check to see if we can add names to the users table, 
     or if we should fail now. */
  code = 1;
  if((value = resolve_configuration_value(ctx, "autoaddusers")) != NULL)
  {
   if(strcmp(value, "yes") == 0)
    create = 1;
   free(value);

   if(ctx->mode == BLDMTRX_MODE_PULL)
    create = 1;

   if(create == 1)
   {
    code = add_user(ctx);
   }
  }
 }

 if(code == 0)
  code = advanced_security_check(ctx);

 close_database(ctx);

 return code;
}

int add_user(struct buildmatrix_context *ctx)
{
 char *sql;
 int code, mode_bits = 0;
 struct sqlite3_stmt *statement = NULL;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: add_user()\n", ctx->error_prefix);

 if(ctx->user == NULL)
 {
  fprintf(stderr, "%s: add_user(): no user name\n", ctx->error_prefix);
  return 1;
 }

 if(open_database(ctx))
  return 1;

 if(derive_mode_bitmask_for_new_user(ctx, &mode_bits))
  return 1;

 sql = "INSERT INTO users (name, posix_uid, email_address, mode_bitmask) VALUES (?, ?, ?, ?);";

 if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
 {
  fprintf(stderr, 
          "%s: add_user(): sqlite3_prepare(%s) failed, '%s'\n",
          ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx));
  return 1;
 }

 if(sqlite3_bind_text(statement, 1, ctx->user, strlen(ctx->user),
                      SQLITE_TRANSIENT) != SQLITE_OK)
 {
  fprintf(stderr, "%s: add_user(): sqlite3_bind_text() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 if(ctx->posix_uid < 1)
 {
  if(sqlite3_bind_null(statement, 2) != SQLITE_OK)
  {
   fprintf(stderr, "%s: add_user(): sqlite3_bind_null() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }
 } else {
  if(sqlite3_bind_int(statement, 2, ctx->posix_uid) != SQLITE_OK)
  {
   fprintf(stderr, "%s: add_user(): sqlite3_bind_int() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }
 }

 if(ctx->email_address == NULL)
 {
  if(sqlite3_bind_null(statement, 3) != SQLITE_OK)
  {
   fprintf(stderr, "%s: add_user(): sqlite3_bind_null() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }
 } else {
  if(sqlite3_bind_text(statement, 3, ctx->email_address, strlen(ctx->email_address),
                       SQLITE_TRANSIENT) != SQLITE_OK)
  {
   fprintf(stderr, "%s: add_user(): sqlite3_bind_text() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }
 }

 if(sqlite3_bind_int(statement, 4, mode_bits) != SQLITE_OK)
 {
  fprintf(stderr, "%s: add_user(): sqlite3_bind_int() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 code = sqlite3_step(statement);

 if(code != SQLITE_DONE)
 {
  fprintf(stderr, "%s: add_user(): sqlite3_step() failed, '%s'\n",
         ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 sqlite3_finalize(statement);

 /* now get the user_id for the newly created row */
 ctx->user_id = -1;
 
 sql = "SELECT user_id FROM users WHERE name = ?;";

 if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
 {
  fprintf(stderr, 
          "%s: user_add(): sqlite3_prepare(%s) failed, '%s'\n",
          ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx));
  return 1;
 }

 if(sqlite3_bind_text(statement, 1, ctx->user, strlen(ctx->user),
                      SQLITE_TRANSIENT) != SQLITE_OK)
 {
  fprintf(stderr, "%s: user_add(): sqlite3_bind_text() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 code = sqlite3_step(statement);

 if(code == SQLITE_ROW)
 {
  if(sqlite3_column_type(statement, 0) != SQLITE_INTEGER)
  {
   fprintf(stderr, "%s: user_add(): users.user_id is not an integer\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  
  ctx->user_id = sqlite3_column_int(statement, 0);
 }

 sqlite3_finalize(statement);

 if(ctx->user_id == -1)
 {
  fprintf(stderr, "%s: user_add(): something went wrong!\n", ctx->error_prefix);
  return 1;
 }

 if(ctx->verbose)
 {
  fprintf(stderr, "%s: user_add(): added user '%s' at internal user_id %d\n", 
          ctx->error_prefix, ctx->user, ctx->user_id);
 }

 close_database(ctx);

 return 0;
}

int list_users(struct buildmatrix_context *ctx)
{
 const char *sql, *name, *email;
 int code, mode_bitmask, posix_uid, user_id, count = 0;
 struct sqlite3_stmt *statement = NULL;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: list_users()\n", ctx->error_prefix);

 if(open_database(ctx))
  return 1;

 sql = "SELECT user_id, posix_uid, name, email_address, mode_bitmask FROM users;";

 if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
 {
  fprintf(stderr, 
          "%s: list_users(): sqlite3_prepare(%s) failed, '%s'\n",
          ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx));
  return 1;
 }

 printf("Local Users:\n");

 while((code = sqlite3_step(statement)) == SQLITE_ROW)
 {

  if(sqlite3_column_type(statement, 0) != SQLITE_INTEGER)
  {
   fprintf(stderr, "%s: list_users(): users.user_id is not an integer\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  user_id = sqlite3_column_int(statement, 0);

  posix_uid = -1;
  if(sqlite3_column_type(statement, 1) != SQLITE_NULL)
  {
   if(sqlite3_column_type(statement, 1) != SQLITE_INTEGER)
   {
    fprintf(stderr, "%s: list_users(): users.posix_uid is not an integer\n",
            ctx->error_prefix);
    sqlite3_finalize(statement);
    return 1;
   }
   posix_uid = sqlite3_column_int(statement, 1);
  }

  if(sqlite3_column_type(statement, 2) != SQLITE_TEXT)
  {
   fprintf(stderr, "%s: lists_users(): name is not text\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  name = (const char *) sqlite3_column_text(statement, 2);

  if(sqlite3_column_type(statement, 3) != SQLITE_NULL)
  {
   if(sqlite3_column_type(statement, 3) != SQLITE_TEXT)
   {
    fprintf(stderr, "%s: lists_users(): email is not text\n",
            ctx->error_prefix);
    sqlite3_finalize(statement);
    return 1;
   }
   email = (const char *) sqlite3_column_text(statement, 3);
  } else {
   email = NULL;
  }

  if(sqlite3_column_type(statement, 4) != SQLITE_INTEGER)
  {
   fprintf(stderr, "%s: list_users(): users.mode_bitmask is not an integer\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  mode_bitmask = sqlite3_column_int(statement, 4);

  count++;

  printf("%d \"%s\" (%s) ", user_id, name, email == NULL ? "no email" : email);


  if(posix_uid > 0)
   printf("posix UID = %d ", posix_uid);

  printf("privillages: ");

  if(mode_bitmask == 0)
   printf("none ");

  if((mode_bitmask & BLDMTRX_ACCESS_BIT_SUBMIT) == BLDMTRX_ACCESS_BIT_SUBMIT)
   printf("submit ");

  if((mode_bitmask & BLDMTRX_ACCESS_BIT_SCRATCH) == BLDMTRX_ACCESS_BIT_SCRATCH)
   printf("scratch ");

  if((mode_bitmask & BLDMTRX_ACCESS_BIT_LIST) == BLDMTRX_ACCESS_BIT_LIST)
   printf("list ");

  if((mode_bitmask & BLDMTRX_ACCESS_BIT_GET) == BLDMTRX_ACCESS_BIT_GET)
   printf("get ");

  printf("\n");
 }

 if(code != SQLITE_DONE)
 {
  fprintf(stderr, "%s: list_users(): sqlite3_step() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 sqlite3_finalize(statement);

 printf(" (%d) users\n", count);

 close_database(ctx);

 return 0;
}

int mod_user(struct buildmatrix_context *ctx)
{
 const char *sql;
 int code;
 struct sqlite3_stmt *statement = NULL;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: mod_user()\n", ctx->error_prefix);

 if(ctx->user == NULL)
 {
  fprintf(stderr, "%s: mod_user(): need a user name\n", ctx->error_prefix);
  return 1;
 }

 if(ctx->verbose)
 {
  fprintf(stderr, "%s: Atempting to change user settings for \"%s\" to: ",
          ctx->error_prefix, ctx->user);

  if(ctx->email_address != NULL)
   fprintf(stderr, "email address = %s, ", ctx->email_address);
  else
   fprintf(stderr, "email address = none, ");

  if(ctx->posix_uid > 0)
   fprintf(stderr, "posix_uid = %d, ", ctx->posix_uid);
  else
   fprintf(stderr, "posix_uid = none, ");

  fprintf(stderr, "privillages = ");

  if(ctx->user_mode_bits == 0)
   fprintf(stderr, "none");

  if((ctx->user_mode_bits & BLDMTRX_ACCESS_BIT_SUBMIT) == BLDMTRX_ACCESS_BIT_SUBMIT)
   fprintf(stderr, "submit ");

  if((ctx->user_mode_bits & BLDMTRX_ACCESS_BIT_SCRATCH) == BLDMTRX_ACCESS_BIT_SCRATCH)
   fprintf(stderr, "scratch ");

  if((ctx->user_mode_bits & BLDMTRX_ACCESS_BIT_LIST) == BLDMTRX_ACCESS_BIT_LIST)
   fprintf(stderr, "list ");

  if((ctx->user_mode_bits & BLDMTRX_ACCESS_BIT_GET) == BLDMTRX_ACCESS_BIT_GET)
   fprintf(stderr, "get ");

  fprintf(stderr, "\n");
 }

 if(open_database(ctx))
  return 1;

 sql = "UPDATE users SET posix_uid=?, email_address=?, mode_bitmask=? WHERE name=?;";

 if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
 {
  fprintf(stderr, 
          "%s: mod_user(): sqlite3_prepare(%s) failed, '%s'\n",
          ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx));
  return 1;
 }

 if(sqlite3_bind_text(statement, 4, ctx->user, strlen(ctx->user),
                      SQLITE_TRANSIENT) != SQLITE_OK)
 {
  fprintf(stderr, "%s: mod_user(): sqlite3_bind_text() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 if(ctx->posix_uid < 1)
 {
  if(sqlite3_bind_null(statement, 1) != SQLITE_OK)
  {
   fprintf(stderr, "%s: mod_user(): sqlite3_bind_null() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }
 } else {
  if(sqlite3_bind_int(statement, 1, ctx->posix_uid) != SQLITE_OK)
  {
   fprintf(stderr, "%s: mod_user(): sqlite3_bind_int() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }
 }

 if(ctx->email_address == NULL)
 {
  if(sqlite3_bind_null(statement, 2) != SQLITE_OK)
  {
   fprintf(stderr, "%s: mod_user(): sqlite3_bind_null() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }
 } else {
  if(sqlite3_bind_text(statement, 2, ctx->email_address, strlen(ctx->email_address),
                       SQLITE_TRANSIENT) != SQLITE_OK)
  {
   fprintf(stderr, "%s: mod_user(): sqlite3_bind_text() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }
 }

 if(sqlite3_bind_int(statement, 3, ctx->user_mode_bits) != SQLITE_OK)
 {
  fprintf(stderr, "%s: mod_user(): sqlite3_bind_int() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 code = sqlite3_step(statement);

 if(code != SQLITE_DONE)
 {
  fprintf(stderr, "%s: mod_user(): sqlite3_step() failed, '%s'\n",
         ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 sqlite3_finalize(statement);

 if(sqlite3_changes(ctx->db_ctx) != 1)
 {
  if(ctx->quiet == 0)
   fprintf(stderr, "%s: mod_user(): nothing was changed\n", ctx->error_prefix);
  return 1;
 } else {
  if(ctx->verbose)
  {
   fprintf(stderr, "%s: User settings for \"%s\" have been updated.\n",
           ctx->error_prefix, ctx->user);
  }
 }

 close_database(ctx);

 return 0;
}

int remove_user(struct buildmatrix_context *ctx)
{
 const char *sql, *alias;
 int code;
 struct sqlite3_stmt *statement = NULL;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: remove_user()\n", ctx->error_prefix);

 if(ctx->user == NULL)
  fprintf(stderr, "%s: remove_user(): need a user name\n", ctx->error_prefix);

 if(open_database(ctx))
  return 1;

 if(resolve_user_id(ctx))
 {
  fprintf(stderr, "%s: remove_user(): could not resolve user name %s\n",
          ctx->error_prefix, ctx->user);
  return 1;
 }

 sql = "SELECT name FROM aliases WHERE user_id = ?";

 if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
 {
  fprintf(stderr, 
          "%s: list_users(): sqlite3_prepare(%s) failed, '%s'\n",
          ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx));
  return 1;
 }

 if(sqlite3_bind_int(statement, 1, ctx->user_id) != SQLITE_OK)
 {
  fprintf(stderr, "%s: remove_user(): sqlite3_bind_int() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 while((code = sqlite3_step(statement)) == SQLITE_ROW)
 {
  if(sqlite3_column_type(statement, 0) != SQLITE_TEXT)
  {
   fprintf(stderr, "%s: remove_user(): aliases.name is not text\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  alias = (const char *) sqlite3_column_text(statement, 0);

  fprintf(stderr, "%s: remove_user(): can not delete user '%s', because alias '%s' still points "
          "to it. User deletealias first.\n",  ctx->error_prefix, ctx->user, alias);

  sqlite3_finalize(statement);
  return 1;
 }

 if(code != SQLITE_DONE)
 {
  fprintf(stderr, "%s: remove_user(): sqlite3_step() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 sqlite3_finalize(statement);

 sql = "SELECT unique_identifier FROM builds WHERE user_id = ?";

 if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
 {
  fprintf(stderr, 
          "%s: list_users(): sqlite3_prepare(%s) failed, '%s'\n",
          ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx));
  return 1;
 }

 if(sqlite3_bind_int(statement, 1, ctx->user_id) != SQLITE_OK)
 {
  fprintf(stderr, "%s: remove_user(): sqlite3_bind_int() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 while((code = sqlite3_step(statement)) == SQLITE_ROW)
 {
  if(sqlite3_column_type(statement, 0) != SQLITE_TEXT)
  {
   fprintf(stderr, "%s: remove_user(): builds.unique_identifier is not text\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  alias = (const char *) sqlite3_column_text(statement, 0);

  fprintf(stderr, "%s: remove_user(): can not delete user '%s', because build %s still points "
          "to it. User scratch first, or alternatively just moduser to no privillages.\n",
          ctx->error_prefix, ctx->user, alias);

  sqlite3_finalize(statement);
  return 1;
 }

 if(code != SQLITE_DONE)
 {
  fprintf(stderr, "%s: remove_user(): sqlite3_step() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 sqlite3_finalize(statement);

 sql = "DELETE FROM users WHERE user_id = ?";

 if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
 {
  fprintf(stderr, 
          "%s: list_users(): sqlite3_prepare(%s) failed, '%s'\n",
          ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx));
  return 1;
 }

 if(sqlite3_bind_int(statement, 1, ctx->user_id) != SQLITE_OK)
 {
  fprintf(stderr, "%s: remove_user(): sqlite3_bind_int() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 code = sqlite3_step(statement);
 if(code != SQLITE_DONE)
 {
  fprintf(stderr, "%s: remove_user(): sqlite3_step() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 sqlite3_finalize(statement);

 if(sqlite3_changes(ctx->db_ctx) != 1)
 {
  if(ctx->quiet == 0)
   fprintf(stderr, "%s: remove_user(): nothing was changed\n", ctx->error_prefix);
  return 1;
 }

 close_database(ctx);

 if(ctx->verbose)
  fprintf(stderr, "%s: remove_user(): local user %s removed\n", ctx->error_prefix, ctx->user);

 return 0;
}

int advanced_security_check(struct buildmatrix_context *ctx)
{
 int handled, code, uid = -1;
 char *value, *sql;
 struct sqlite3_stmt *statement = NULL;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: advanced_security_check()\n", ctx->error_prefix);

 if(ctx->user_id < 1)
 {
  fprintf(stderr, "%s: advanced_security_check(): no user_id\n", ctx->error_prefix);
  return 1;
 }

 if((value = resolve_configuration_value(ctx, "securitymodel")) == NULL)
 {
  fprintf(stderr, "%s: advanced_security_check(): can not resolve configuration "
          "value for variable \"securitymodel\"\n", ctx->error_prefix);
  return 1;
 }

 handled = 0;
 if(strcmp(value, "simple") == 0)
 {
  free(value);
  return 0;
 }

 if(strcmp(value, "advanced") == 0)
 {
  handled = 1;
 }

 if(handled == 0)
 {
  fprintf(stderr, "%s: advanced_security_check(): invalid value for configuration "
          "variable securitymodel, \"%s\"\n",  ctx->error_prefix, value);
  free(value);
  return 1;
 }

 free(value);

 if(open_database(ctx))
  return 1;

 sql = "SELECT posix_uid FROM users WHERE user_id = ?;";

 if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
 {
  fprintf(stderr, 
          "%s: advanced_security_check(): sqlite3_prepare(%s) failed, '%s'\n",
          ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx));
  return 1;
 }

 if(sqlite3_bind_int(statement, 1, ctx->user_id) != SQLITE_OK)
 {
  fprintf(stderr, "%s: advanced_security_check(): sqlite3_bind_int() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 code = sqlite3_step(statement);

 if(code == SQLITE_ROW)
 {
  if(sqlite3_column_type(statement, 0) != SQLITE_NULL)
  {
   if(sqlite3_column_type(statement, 0) != SQLITE_INTEGER)
   {
    fprintf(stderr, "%s: advanced_security_check(): users.posix_uid is not an integer\n",
            ctx->error_prefix);
    sqlite3_finalize(statement);
    return 1;
   }

   uid = sqlite3_column_int(statement, 0);
  }
 }

 sqlite3_finalize(statement);

 close_database(ctx);

 if(uid < 1)
 {
  fprintf(stderr, "%s: advanced_security_check(): user %s has no posix uid needed for the "
          "advanced security model.\n", ctx->error_prefix, ctx->user);
  return 1;
 }

 if(uid != getuid())
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: advanced_security_check(): failed for user %s.\n",
           ctx->error_prefix, ctx->user);
  return 1;
 }

 return 0;
}

int get_mode_bitmask(struct buildmatrix_context *ctx, int *mode)
{
 int code, mode_bitmask = 0;
 char *sql;
 struct sqlite3_stmt *statement;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: get_mode_bitmask()\n", ctx->error_prefix);

 if(ctx->user_id < 1)
 {
  fprintf(stderr, "%s: get_mode_bitmask(): no user id\n", ctx->error_prefix);
  return 1;
 }

 if(open_database(ctx))
  return 1;

 sql = "SELECT mode_bitmask FROM users WHERE user_id = ?;";

 if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
 {
  fprintf(stderr, 
          "%s: get_mode_bitmask(): sqlite3_prepare(%s) failed, '%s'\n",
          ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx));
  return 1;
 }

 if(sqlite3_bind_int(statement, 1, ctx->user_id) != SQLITE_OK)
 {
  fprintf(stderr, "%s: get_mode_bitmask(): sqlite3_bind_int() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 code = sqlite3_step(statement);

 if(code == SQLITE_ROW)
 {
  if(sqlite3_column_type(statement, 0) != SQLITE_INTEGER)
  {
   fprintf(stderr, "%s: get_mode_bitmask(): users.mode_bitmask is not an integer\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }

  mode_bitmask = sqlite3_column_int(statement, 0);
 }

 sqlite3_finalize(statement);

 close_database(ctx);

 *mode = mode_bitmask;
 return 0;
}

int authorize_submit(struct buildmatrix_context *ctx)
{
 int mode_bitmask;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: authorize_submit()\n", ctx->error_prefix);

 if(ctx->user == NULL)
 {
  fprintf(stderr, "%s: authorize_submit(): no user name\n", ctx->error_prefix);
  return 1;
 }

 if(get_mode_bitmask(ctx, &mode_bitmask))
  return 1;

 if((mode_bitmask & BLDMTRX_ACCESS_BIT_SUBMIT) == BLDMTRX_ACCESS_BIT_SUBMIT)
  return 0;

 fprintf(stderr, "%s: authorize_submit(): user %s is not authorized for submit operation\n", 
         ctx->error_prefix, ctx->user);

 return 1;
}

int authorize_scratch(struct buildmatrix_context *ctx)
{
 int mode_bitmask;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: authorize_scratch()\n", ctx->error_prefix);

 if(ctx->user == NULL)
 {
  fprintf(stderr, "%s: authorize_scratch(): no user name\n", ctx->error_prefix);
  return 1;
 }

 if(get_mode_bitmask(ctx, &mode_bitmask))
  return 1;

 if((mode_bitmask & BLDMTRX_ACCESS_BIT_SCRATCH) == BLDMTRX_ACCESS_BIT_SCRATCH)
  return 0;

 fprintf(stderr, "%s: authorize_scratch(): user %s is not authorized for scratch operation\n", 
         ctx->error_prefix, ctx->user);

 return 1;
}

int authorize_list(struct buildmatrix_context *ctx)
{
 int mode_bitmask;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: authorize_list()\n", ctx->error_prefix);

 if(ctx->user == NULL)
 {
  fprintf(stderr, "%s: authorize_list(): no user name\n", ctx->error_prefix);
  return 1;
 }

 if(get_mode_bitmask(ctx, &mode_bitmask))
  return 1;

 if((mode_bitmask & BLDMTRX_ACCESS_BIT_LIST) == BLDMTRX_ACCESS_BIT_LIST)
  return 0;

 fprintf(stderr, "%s: authorize_list(): user %s is not authorized for list operation\n", 
         ctx->error_prefix, ctx->user);

 return 1;
}

int authorize_get(struct buildmatrix_context *ctx)
{
 int mode_bitmask;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: authorize_get()\n", ctx->error_prefix);

 if(ctx->user == NULL)
 {
  fprintf(stderr, "%s: authorize_get(): no user name\n", ctx->error_prefix);
  return 1;
 }

 if(get_mode_bitmask(ctx, &mode_bitmask))
  return 1;

 if((mode_bitmask & BLDMTRX_ACCESS_BIT_GET) == BLDMTRX_ACCESS_BIT_GET)
  return 0;

 fprintf(stderr, "%s: authorize_get(): user %s is not authorized for get operation - %d\n", 
         ctx->error_prefix, ctx->user, mode_bitmask);

 return 1;
}

int derive_mode_bitmask_for_new_user(struct buildmatrix_context *ctx, int *mode)
{
 int bit_mask = 0, submit = 0, scratch = 0, list = 0, get = 0;
 char *value;

 if((value = resolve_configuration_value(ctx, "newuserscratchdefault")) == NULL)
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: derive_mode_bitmask_for_new_user(): can not resolve configuration "
           "value for variable \"newuserscratchdefault\"\n", ctx->error_prefix);
 } else {
  if(strcmp(value, "yes") == 0)
   scratch = 1; 
  free(value);
 }

 if((value = resolve_configuration_value(ctx, "newuseradddefault")) == NULL)
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: derive_mode_bitmask_for_new_user(): can not resolve configuration "
           "value for variable \"newuseradddefault\"\n", ctx->error_prefix);
 } else {
  if(strcmp(value, "yes") == 0)
   submit = 1; 
  free(value);
 }

 if((value = resolve_configuration_value(ctx, "newuserlistdefault")) == NULL)
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: derive_mode_bitmask_for_new_user(): can not resolve configuration "
           "value for variable \"newuserlistdefault\"\n", ctx->error_prefix);
 } else {
  if(strcmp(value, "yes") == 0)
   list = 1; 
  free(value);
 }

 if((value = resolve_configuration_value(ctx, "newusergetdefault")) == NULL)
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: derive_mode_bitmask_for_new_user(): can not resolve configuration "
           "value for variable \"newusergetdefault\"\n", ctx->error_prefix);
 } else {
  if(strcmp(value, "yes") == 0)
   get = 1; 
  free(value);
 }

 if((ctx->user_mode_bits & BLDMTRX_ACCESS_BIT_SUBMIT) == BLDMTRX_ACCESS_BIT_SUBMIT)
  submit = 1;

 if((ctx->user_mode_bits & BLDMTRX_ACCESS_BIT_SCRATCH) == BLDMTRX_ACCESS_BIT_SCRATCH)
  scratch = 1;

 if((ctx->user_mode_bits & BLDMTRX_ACCESS_BIT_LIST) == BLDMTRX_ACCESS_BIT_LIST)
  list = 1;

 if((ctx->user_mode_bits & BLDMTRX_ACCESS_BIT_GET) == BLDMTRX_ACCESS_BIT_GET)
  get = 1;
 
 if(submit)
  bit_mask |= BLDMTRX_ACCESS_BIT_SUBMIT;

 if(scratch)
  bit_mask |= BLDMTRX_ACCESS_BIT_SCRATCH;

 if(list)
  bit_mask |= BLDMTRX_ACCESS_BIT_LIST;

 if(get)
  bit_mask |= BLDMTRX_ACCESS_BIT_GET;

 *mode = bit_mask;
 return 0;
}

int add_alias(struct buildmatrix_context *ctx)
{
 char *sql;
 int code;
 struct sqlite3_stmt *statement = NULL;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: add_alias()\n", ctx->error_prefix);

 if(ctx->user == NULL)
 {
  fprintf(stderr, "%s: add_alias(): no user name\n", ctx->error_prefix);
  return 1;
 }

 if(ctx->alias == NULL)
 {
  fprintf(stderr, "%s: add_alias(): no alias\n", ctx->error_prefix);
  return 1;
 }

 if(open_database(ctx))
  return 1;

 if(resolve_user_id(ctx))
 {
  fprintf(stderr, "%s: add_alias(): I can't resolve user %s\n", ctx->error_prefix, ctx->user);
  return 1;
 }

 sql = "INSERT INTO aliases (user_id, name) VALUES (?, ?);";

 if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
 {
  fprintf(stderr, 
          "%s: add_alias(): sqlite3_prepare(%s) failed, '%s'\n",
          ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx));
  return 1;
 }

 if(sqlite3_bind_int(statement, 1, ctx->user_id) != SQLITE_OK)
 {
  fprintf(stderr, "%s: add_alias(): sqlite3_bind_int() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 if(sqlite3_bind_text(statement, 2, ctx->alias, strlen(ctx->alias),
                      SQLITE_TRANSIENT) != SQLITE_OK)
 {
  fprintf(stderr, "%s: add_alias(): sqlite3_bind_text() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 code = sqlite3_step(statement);

 if(code != SQLITE_DONE)
 {
  fprintf(stderr, "%s: add_alias(): sqlite3_step() failed, '%s'\n",
         ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 sqlite3_finalize(statement);

 close_database(ctx);

 return 0;
}

int list_aliases(struct buildmatrix_context *ctx)
{
 const char *sql, *alias, *user;
 int code, count = 0;
 struct sqlite3_stmt *statement = NULL;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: list_aliases()\n", ctx->error_prefix);

 if(open_database(ctx))
  return 1;

 sql = "SELECT aliases.name, users.name FROM aliases JOIN users on aliases.user_id;";

 if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
 {
  fprintf(stderr, 
          "%s: list_aliases(): sqlite3_prepare(%s) failed, '%s'\n",
          ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx));
  return 1;
 }

 printf("Local User Aliases:\n");

 while((code = sqlite3_step(statement)) == SQLITE_ROW)
 {

  if(sqlite3_column_type(statement, 0) != SQLITE_TEXT)
  {
   fprintf(stderr, "%s: lists_aliases(): aliases.name is not text\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  alias = (const char *) sqlite3_column_text(statement, 0);

  if(sqlite3_column_type(statement, 1) != SQLITE_TEXT)
  {
   fprintf(stderr, "%s: lists_aliases(): users.name is not text\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  user = (const char *) sqlite3_column_text(statement, 1);

  count++;

  printf("%d \"%s\" -> \"%s\"\n", count, alias, user);
 }

 if(code != SQLITE_DONE)
 {
  fprintf(stderr, "%s: list_aliases(): sqlite3_step() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 sqlite3_finalize(statement);

 printf(" (%d) aliasess\n", count);

 close_database(ctx);

 return 0;
}

int remove_alias(struct buildmatrix_context *ctx)
{
 char *sql;
 int code;
 struct sqlite3_stmt *statement = NULL;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: remove_alias()\n", ctx->error_prefix);

 if(ctx->alias == NULL)
 {
  fprintf(stderr, "%s: remove_alias(): need an alias\n", ctx->error_prefix);
  return 1;
 }

 if(open_database(ctx))
  return 1;

 sql = "DELETE FROM aliases WHERE name = ?;";

 if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
 {
  fprintf(stderr, "%s: remove_alias(): sqlite3_prepare(%s) failed, '%s'\n",
          ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx)); 
  return 1;
 }

 if(sqlite3_bind_text(statement, 1, ctx->alias, strlen(ctx->alias),
                      SQLITE_TRANSIENT) != SQLITE_OK)
 {
  fprintf(stderr, "%s: remove_alias(): sqlite3_bind_text() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 code = sqlite3_step(statement);
 if(code != SQLITE_DONE)
 {
  fprintf(stderr, "%s: remove_alias(): sqlite3_step() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 sqlite3_finalize(statement);

 if(sqlite3_changes(ctx->db_ctx) != 1)
 {
  if(ctx->quiet == 0)
   fprintf(stderr, "%s: remove_alias(): nothing was changed\n", ctx->error_prefix);
  return 1;
 }

 close_database(ctx);

 return 1;
}
