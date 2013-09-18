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

int open_database(struct buildmatrix_context *ctx)
{
 char filename[2048];
 char *effective = ctx->local_project_directory;
 struct stat metadata;

 ctx->db_ref_count++;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: open_database() - reference count will be %d\n", 
          ctx->error_prefix, ctx->db_ref_count);

 if(ctx->db_ref_count > 1)
  return 0;

 if(ctx->verbose)
  fprintf(stderr, "%s: open_database() opening...\n", ctx->error_prefix);

 filename[0] = 0;

 if( (ctx->local_project_directory == NULL) && 
     (ctx->mode != BLDMTRX_MODE_SERVE) )
 {
  snprintf(filename, 2048, "%s/buildmatrix.sqlite3", ctx->current_working_directory); 
  if(stat(filename, &metadata) == 0)
  {
   if(S_ISREG(metadata.st_mode))
   {
    effective = ctx->current_working_directory;

    if(ctx->verbose)
     fprintf(stderr, "%s: no local project directory specified, going with working directory (%s)\n",
             ctx->error_prefix, effective);
   }
  }

  if(effective != ctx->current_working_directory)
  {
   fprintf(stderr, "%s: can not find project directory, try --localproject\n",
           ctx->error_prefix);
   return 1;
  }
 }

 if(filename[0] == 0)
  snprintf(filename, 2048, "%s/buildmatrix.sqlite3", effective); 

 if(sqlite3_open(filename, &(ctx->db_ctx)) != SQLITE_OK)
 {
  fprintf(stderr, "%s: sqlite3_open('%s') failed, '%s'\n",
          ctx->error_prefix, filename, sqlite3_errmsg(ctx->db_ctx));

  sqlite3_close(ctx->db_ctx);
  return 1;
 }

 ctx->local_project_directory = effective;

 return 0;
}

int close_database(struct buildmatrix_context *ctx)
{
 ctx->db_ref_count--;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: close_database() - reference count will be %d\n", 
          ctx->error_prefix, ctx->db_ref_count);

 if(ctx->db_ref_count > 0)
  return 0;

 if(ctx->db_ref_count == 0)
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: close_database() closing...\n", ctx->error_prefix);
 } else {
 fprintf(stderr, "%s: close_database() reference count is less than 0 (%d)\n",
         ctx->error_prefix, ctx->db_ref_count);
 }

 if(ctx->db_ctx != NULL)
 {
  sqlite3_close(ctx->db_ctx);
  ctx->db_ctx = NULL;
 }

 return 0;
}

int new_database(struct buildmatrix_context *ctx)
{
 char *errmsg = NULL;

 if(ctx->verbose)
  fprintf(stderr, "%s: new_database()\n", 
          ctx->error_prefix);

 if(open_database(ctx))
  return 1;

 if(sqlite3_exec(ctx->db_ctx, 
    "CREATE TABLE configuration (id INTEGER PRIMARY KEY, parameter TEXT, value TEXT, "
                                 "UNIQUE (parameter));",
    NULL, NULL, &errmsg) != SQLITE_OK)
 {
  fprintf(stderr, "%s: could not create configuration table, '%s'\n",
          ctx->error_prefix, errmsg);
  sqlite3_free(errmsg);
  sqlite3_close(ctx->db_ctx);
  return 1;
 }

 if(sqlite3_exec(ctx->db_ctx, 
    "CREATE TABLE servers (id INTEGER PRIMARY KEY, name TEXT, sshbin TEXT, bldmtrxpath TEXT, "
                          "project_directory TEXT, transport INTEGER, last_pulled_id INTEGER, "
                          "user TEXT, UNIQUE (name));",
    NULL, NULL, &errmsg) != SQLITE_OK)
 {
  fprintf(stderr, "%s: could not create configuration table, '%s'\n",
          ctx->error_prefix, errmsg);
  sqlite3_free(errmsg);
  sqlite3_close(ctx->db_ctx);
  return 1;
 }

 if(sqlite3_exec(ctx->db_ctx, 
    "CREATE TABLE users (user_id INTEGER PRIMARY KEY, name TEXT, posix_uid INTEGER, "
                        "email_address TEXT, mode_bitmask INTEGER, UNIQUE (name, posix_uid));",
    NULL, NULL, &errmsg) != SQLITE_OK)
 {
  fprintf(stderr, "%s: could not create users table, '%s'\n", 
          ctx->error_prefix, errmsg);
  sqlite3_free(errmsg);
  sqlite3_close(ctx->db_ctx);
  return 1;
 }

 if(sqlite3_exec(ctx->db_ctx, 
    "CREATE TABLE aliases (id INTEGER PRIMARY KEY, user_id INTEGER REFERENCES users(user_id), "
                          "name TEXT, UNIQUE (name));",
    NULL, NULL, &errmsg) != SQLITE_OK)
 {
  fprintf(stderr, "%s: could not create aliases table, '%s'\n", 
          ctx->error_prefix, errmsg);
  sqlite3_free(errmsg);
  sqlite3_close(ctx->db_ctx);
  return 1;
 }

 if(sqlite3_exec(ctx->db_ctx, 
    "CREATE TABLE builds (build_id INTEGER PRIMARY KEY, "
                         "job TEXT, "
                         "user_id INTEGER REFERENCES users(user_id), "
                         "branch TEXT, unique_identifier TEXT, build_node TEXT, "
                         "build_time INTEGER, result INTEGER, passed_tests INTEGER, "
                         "failed_tests INTEGER, incomplete_tests INTEGER, report TEXT, "
                         "output TEXT, checksum TEXT, has_tests INTEGER, has_parameters INTEGER, "
                         "revision TEXT, UNIQUE (unique_identifier));",
    NULL, NULL, &errmsg) != SQLITE_OK)
 {
  fprintf(stderr, "%s: could not create builds table, '%s'\n",
          ctx->error_prefix, errmsg);
  sqlite3_free(errmsg);
  sqlite3_close(ctx->db_ctx);
  return 1;
 }

 if(sqlite3_exec(ctx->db_ctx, 
                 "CREATE TABLE parameters (id INTEGER PRIMARY KEY, "
                                          "build_id INTEGER REFERENCES builds(build_id), "
                                          "variable TEXT, value TEXT);",
                 NULL, NULL, &errmsg) != SQLITE_OK)
 {
  fprintf(stderr, "%s: could not create parameters table, '%s\n'", 
          ctx->error_prefix, errmsg);
  sqlite3_free(errmsg);
  sqlite3_close(ctx->db_ctx);
  return 1;
 }

 if(sqlite3_exec(ctx->db_ctx, 
                 "CREATE TABLE suites (suite_id INTEGER PRIMARY KEY, name TEXT, "
                                      "description TEXT, UNIQUE(name));",
                 NULL, NULL, &errmsg) != SQLITE_OK)
 {
  fprintf(stderr, "%s: could not create suites table, '%s'\n",
          ctx->error_prefix, errmsg);
  sqlite3_free(errmsg);
  sqlite3_close(ctx->db_ctx);
  return 1;
 }

 if(sqlite3_exec(ctx->db_ctx, 
                 "CREATE TABLE tests (test_id INTEGER PRIMARY KEY, "
                                     "suite_id INTEGER REFERENCES suites(suite_id), "
                                     "name TEXT, description TEXT, UNIQUE(suite_id, name));",
                 NULL, NULL, &errmsg) != SQLITE_OK)
 {
  fprintf(stderr, "%s: could not create tests table, '%s'\n", 
          ctx->error_prefix, errmsg);
  sqlite3_free(errmsg);
  sqlite3_close(ctx->db_ctx);
  return 1;
 }

 if(sqlite3_exec(ctx->db_ctx, 
                 "CREATE TABLE scores (id INTEGER PRIMARY KEY, "
                                      "build_id INTEGER REFERENCES builds(build_id), "
                                      "test_id INTEGER REFERENCES tests(test_id), "
                                      "result INTEGER, data TEXT, "
                                      "UNIQUE(build_id, test_id));",
                 NULL, NULL, &errmsg) != SQLITE_OK)
 {
  fprintf(stderr, "%s: could not create scores table, '%s'\n",
          ctx->error_prefix, errmsg);
  sqlite3_free(errmsg);
  sqlite3_close(ctx->db_ctx);
  return 1;
 }

 close_database(ctx);
 return 0;
}

int lookup_build_id(struct buildmatrix_context *ctx)
{
 char *sql;
 int code, build_id = -1;
 struct sqlite3_stmt *statement = NULL;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: lookup_build_id(%s)\n",
          ctx->error_prefix, ctx->unique_identifier);

 if(ctx->unique_identifier == NULL)
 {
  fprintf(stderr, "%s: lookup_build_id(): I don't have a unique identifier yet.\n",
           ctx->error_prefix);
  return 1;
 }

 if(open_database(ctx))
  return 1;

 sql = "SELECT build_id FROM builds WHERE unique_identifier = ?;";

 if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
 {
  fprintf(stderr, 
          "%s: lookup_build_id(): sqlite3_prepare(%s) failed, '%s'\n",
          ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx));
  return 1;
 }

 if(sqlite3_bind_text(statement, 1, ctx->unique_identifier, strlen(ctx->unique_identifier),
                      SQLITE_TRANSIENT) != SQLITE_OK)
 {
  fprintf(stderr, "%s: lookup_build_id(): sqlite3_bind_text() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 code = sqlite3_step(statement);

 if(code == SQLITE_ROW)
 {
  if(sqlite3_column_type(statement, 0) != SQLITE_INTEGER)
  {
   fprintf(stderr, "%s: lookup_build_id(): builds.build_id is not an integer\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }

  build_id = sqlite3_column_int(statement, 0);
 }

 if(build_id < 1)
 {
  fprintf(stderr, "%s: lookup_build_id(): could not find build %s.\n",
          ctx->error_prefix, ctx->unique_identifier);

  sqlite3_finalize(statement);
  return 1;
 }

 ctx->build_id = build_id;
 sqlite3_finalize(statement);

 close_database(ctx);
 return 0;
}

int ready_build_submission(struct buildmatrix_context *ctx)
{
 char *sql, *errmsg;
 int code;
 struct sqlite3_stmt *statement = NULL;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: ready_build_submission()\n", ctx->error_prefix);

 if(ctx->job_name == NULL)
 {
  fprintf(stderr, "%s: ready_build_submission(): need a job_name\n",
          ctx->error_prefix);
  return 1;
 }

 if(ctx->branch_name == NULL)
 {
  fprintf(stderr, "%s: ready_build_submission(): need a branch_name\n",
          ctx->error_prefix);
  return 1;
 }

 if(ctx->user_id < 1)
 {
  fprintf(stderr, "%s: ready_build_submission(): need a user_id\n",
          ctx->error_prefix);
  return 1;
 }

 if(ctx->unique_identifier == NULL)
 {
  if(generate_unique_identifier(ctx))
  return 1;
 }

 if(open_database(ctx))
  return 1;

 if(sqlite3_exec(ctx->db_ctx, "BEGIN TRANSACTION;",
    NULL, NULL, &errmsg) != SQLITE_OK)
 {
  fprintf(stderr, "%s: ready_build_submission(): sqlite3_exec(\"BEGIN TRANSACTION;\"): '%s'\n",
          ctx->error_prefix, errmsg);
  sqlite3_free(errmsg);
  return 1;
 }

 sql = "INSERT INTO builds (job, branch, unique_identifier, user_id) VALUES (?, ?, ?, ?);";

 if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
 {
  fprintf(stderr, 
          "%s: read_build_submission(): sqlite3_prepare(%s) failed, '%s'\n",
          ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx));
  return 1;
 }

 if(sqlite3_bind_text(statement, 1, ctx->job_name, strlen(ctx->job_name),
                      SQLITE_TRANSIENT) != SQLITE_OK)
 {
  fprintf(stderr, "%s: ready_build_submission(): sqlite3_bind_text() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 if(sqlite3_bind_text(statement, 2, ctx->branch_name, strlen(ctx->branch_name),
                      SQLITE_TRANSIENT) != SQLITE_OK)
 {
  fprintf(stderr, "%s: ready_build_submission(): sqlite3_bind_text() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 if(sqlite3_bind_text(statement, 3, ctx->unique_identifier, strlen(ctx->unique_identifier),
                      SQLITE_TRANSIENT) != SQLITE_OK)
 {
  fprintf(stderr, "%s: ready_build_submission(): sqlite3_bind_text() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 if(sqlite3_bind_int(statement, 4, ctx->user_id) != SQLITE_OK)
 {
  fprintf(stderr, "%s: ready_build_submission(): sqlite3_bind_int() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 code = sqlite3_step(statement);
 if(code != SQLITE_DONE)
 {
  fprintf(stderr, "%s: ready_build_submission(): sqlite3_step() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 sqlite3_finalize(statement);

 if(lookup_build_id(ctx))
  return 1;

 return 0;
}

int add_build(struct buildmatrix_context *ctx)
{
 char *sql, *errmsg;
 struct sqlite3_stmt *statement = NULL;
 int code;

 if(ctx->verbose > 1)
  fprintf(stderr, 
          "%s: add_build(job = %s, branch = %s, unique_identifier = %s, internal db build_id = %d)\n",
          ctx->error_prefix, ctx->job_name, ctx->branch_name, ctx->unique_identifier, ctx->build_id);

 if(ctx->build_id < 1)
 {
  fprintf(stderr, "%s: add_build(): no build_id in context. "
          "ready_build_submission() should have been called first.\n",
          ctx->error_prefix);
  return 1;
 }

 sql = "UPDATE builds SET result=?, build_node=?, report=?, output=?, checksum=?, "
       "build_time=?, passed_tests=?, failed_tests=?, incomplete_tests=?, "
       "has_tests=?, has_parameters=?, revision=? "
       "WHERE build_id = ? AND job=? AND branch=? AND unique_identifier=?";

 if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
 {
  fprintf(stderr, "%s: add_build(): sqlite3_prepare(%s) failed, '%s'\n",
          ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx));
  return 1;
 }

 if(sqlite3_bind_int(statement, 13, ctx->build_id) != SQLITE_OK)
 {
  fprintf(stderr, "%s: add_build(): sqlite3_bind_int() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 /* just the build id should be enough, but we are going over board */ 
 if(sqlite3_bind_text(statement, 14, ctx->job_name, strlen(ctx->job_name),
                      SQLITE_TRANSIENT) != SQLITE_OK)
 {
  fprintf(stderr, "%s: add_build(): sqlite3_bind_text() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 if(sqlite3_bind_text(statement, 15, ctx->branch_name, strlen(ctx->branch_name),
                      SQLITE_TRANSIENT) != SQLITE_OK)
 {
  fprintf(stderr, "%s: add_build(): sqlite3_bind_text() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 if(sqlite3_bind_text(statement, 16, ctx->unique_identifier, strlen(ctx->unique_identifier),
                      SQLITE_TRANSIENT) != SQLITE_OK)
 {
  fprintf(stderr, "%s: add_build(): sqlite3_bind_text() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 /* the columns are the new data if appicable */
 if(sqlite3_bind_int(statement, 1, ctx->build_result) != SQLITE_OK)
 {
  fprintf(stderr, "%s: add_build(): sqlite3_bind_int() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 if(ctx->build_node != NULL)
 {
  if(sqlite3_bind_text(statement, 2, ctx->build_node, strlen(ctx->build_node),
                       SQLITE_TRANSIENT) != SQLITE_OK)
  {
   fprintf(stderr, "%s: add_build(): sqlite3_bind_text() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }
 } else {
  if(sqlite3_bind_null(statement, 2) != SQLITE_OK)
  {
   fprintf(stderr, "%s: add_build(): sqlite3_bind_null() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }
 }

 if(ctx->build_report != NULL)
 {
  if(sqlite3_bind_text(statement, 3, ctx->build_report, strlen(ctx->build_report),
                       SQLITE_TRANSIENT) != SQLITE_OK)
  {
   fprintf(stderr, "%s: add_build(): sqlite3_bind_text() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }
 } else {
  if(sqlite3_bind_null(statement, 3) != SQLITE_OK)
  {
   fprintf(stderr, "%s: add_build(): sqlite3_bind_null() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }
 }

 if(ctx->build_output != NULL)
 {
  if(sqlite3_bind_text(statement, 4, ctx->build_output, strlen(ctx->build_output),
                       SQLITE_TRANSIENT) != SQLITE_OK)
  {
   fprintf(stderr, "%s: add_build(): sqlite3_bind_text() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }
 } else {
  if(sqlite3_bind_null(statement, 4) != SQLITE_OK)
  {
   fprintf(stderr, "%s: add_build(): sqlite3_bind_null() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }
 }

 if(ctx->checksum != NULL)
 {
  if(sqlite3_bind_text(statement, 5, ctx->checksum, strlen(ctx->checksum),
                       SQLITE_TRANSIENT) != SQLITE_OK)
  {
   fprintf(stderr, "%s: add_build(): sqlite3_bind_text() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }
 } else {
  if(sqlite3_bind_null(statement, 5) != SQLITE_OK)
  {
   fprintf(stderr, "%s: add_build(): sqlite3_bind_null() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }
 }

 if(sqlite3_bind_int64(statement, 6, ctx->build_time) != SQLITE_OK)
 {
  fprintf(stderr, "%s: add_build(): sqlite3_bind_int64() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 if(sqlite3_bind_int(statement, 7, ctx->test_totals[1]) != SQLITE_OK)
 {
  fprintf(stderr, "%s: add_build(): sqlite3_bind_int() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 if(sqlite3_bind_int(statement, 8, ctx->test_totals[2]) != SQLITE_OK)
 {
  fprintf(stderr, "%s: add_build(): sqlite3_bind_int() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 if(sqlite3_bind_int(statement, 9, ctx->test_totals[3]) != SQLITE_OK)
 {
  fprintf(stderr, "%s: add_build(): sqlite3_bind_int() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 if(sqlite3_bind_int(statement, 10, (ctx->test_results != NULL) ) != SQLITE_OK)
 {
  fprintf(stderr, "%s: add_build(): sqlite3_bind_int() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 if(sqlite3_bind_int(statement, 11, (ctx->parameters != NULL) ) != SQLITE_OK)
 {
  fprintf(stderr, "%s: add_build(): sqlite3_bind_int() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 if(ctx->revision != NULL)
 {
  if(sqlite3_bind_text(statement, 12, ctx->revision, strlen(ctx->revision),
                       SQLITE_TRANSIENT) != SQLITE_OK)
  {
   fprintf(stderr, "%s: add_build(): sqlite3_bind_text() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }
 } else {
  if(sqlite3_bind_null(statement, 12) != SQLITE_OK)
  {
   fprintf(stderr, "%s: add_build(): sqlite3_bind_null() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }
 }

 code = sqlite3_step(statement);
 if(code != SQLITE_DONE)
 {
  fprintf(stderr, "%s: add_build(): sqlite3_step() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 if(sqlite3_changes(ctx->db_ctx) != 1)
 {
  fprintf(stderr, "%s: add_build(): sqlite3_changes() != 1\n",  ctx->error_prefix);
  sqlite3_finalize(statement);
  return 1;
 }

 /* move the files out of limbo */
 if(ctx->checksum != NULL)
 {
  if(limbo_to_database(ctx, ctx->checksum))
   return 1;
 }

 if(ctx->build_report != NULL)
 {
  if(limbo_to_database(ctx, ctx->build_report))
   return 1;
 }

 if(ctx->build_output != NULL)
 {
  if(limbo_to_database(ctx, ctx->build_output))
   return 1;
 }

 if(sqlite3_exec(ctx->db_ctx, "COMMIT TRANSACTION;",
    NULL, NULL, &errmsg) != SQLITE_OK)
 {
  fprintf(stderr, "%s: add_build(): sqlite3_exec(\"COMMIT TRANSACTION;\"): '%s'\n",
          ctx->error_prefix, errmsg);
  sqlite3_free(errmsg);
  return 1;
 }

 return 0;
}

int service_list_jobs(struct buildmatrix_context *ctx)
{
 char *sql;
 const unsigned char *job_name;
 struct sqlite3_stmt *statement = NULL;
 int code, count = 0;

 if(ctx->verbose > 1)
  fprintf(stderr, 
          "%s: service_list_jobs()\n", ctx->error_prefix);

 if(open_database(ctx))
  return 1;

 if(ctx->branch_name == NULL)
 { 
  if(ctx->build_node == NULL)
  {
   sql = "select distinct job from builds;"; 
  } else {
   sql = "select distinct job from builds where build_node = ?;"; 
  }
 } else {
  if(ctx->build_node == NULL)
  {
   sql = "select distinct job from builds where branch = ?;"; 
  } else {
   sql = "select distinct job from builds where branch = ? and build_node = ?;"; 
  }
 }

 if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
 {
  fprintf(stderr, "%s: service_list_jobs(): sqlite3_prepare(%s) failed, '%s'\n",
          ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx)); 
  return 1;
 }

 if(ctx->branch_name == NULL)
 { 
  if(ctx->build_node != NULL)
  {
   if(sqlite3_bind_text(statement, 1, ctx->build_node, strlen(ctx->build_node), 
                        SQLITE_TRANSIENT) != SQLITE_OK)
   {
    fprintf(stderr, "%s: service_list_jobs(): sqlite3_bind_text() failed, '%s'\n",
            ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
    sqlite3_finalize(statement);
    return 1;
   }
  }
 } else {
  if(sqlite3_bind_text(statement, 1, ctx->branch_name, strlen(ctx->branch_name), 
                       SQLITE_TRANSIENT) != SQLITE_OK)
  {
   fprintf(stderr, "%s: service_list_jobs(): sqlite3_bind_text() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }

  if(ctx->build_node != NULL)
  {
   if(sqlite3_bind_text(statement, 2, ctx->build_node, strlen(ctx->build_node), 
                        SQLITE_TRANSIENT) != SQLITE_OK)
   {
    fprintf(stderr, "%s: service_list_jobs(): sqlite3_bind_text() failed, '%s'\n",
            ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
    sqlite3_finalize(statement);
    return 1;
   }
  }
 }

 if(ctx->service_simple_lists_strategy.start(ctx, 
                                             ctx->service_simple_lists_strategy.strategy_context,
                                             NULL))
 {
  if(ctx->verbose > 1)
   fprintf(stderr, "%s: service_list_jobs(): strategy callback failed\n",
           ctx->error_prefix);
  return 1;
 }

 while((code = sqlite3_step(statement)) == SQLITE_ROW)
 {

  if(sqlite3_column_type(statement, 0) != SQLITE_TEXT)
  {
   fprintf(stderr, "%s: service_list_jobs(): job is not text\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  job_name = sqlite3_column_text(statement, 0);

  if(ctx->service_simple_lists_strategy.iterative(ctx, 
                                                  ctx->service_simple_lists_strategy.strategy_context,
                                                  job_name))
  {
   if(ctx->verbose > 1)
    fprintf(stderr, "%s: service_list_jobs(): strategy callback failed\n",
            ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }

  count++;
 }

 if(code != SQLITE_DONE)
 {
  fprintf(stderr, "%s: service_list_jobs(): sqlite3_step() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 sqlite3_finalize(statement);

 if(ctx->verbose)
  fprintf(stderr, 
          "%s: service_list_jobs(): sent %d job name(s) with build node = %s, and branch = %s\n",
          ctx->error_prefix, count, ctx->build_node, ctx->branch_name);

 if(ctx->service_simple_lists_strategy.done(ctx, ctx->service_simple_lists_strategy.strategy_context, NULL))
 {
  if(ctx->verbose > 1)
   fprintf(stderr, "%s: service_list_jobs(): strategy callback failed\n",
           ctx->error_prefix);
  return 1;
 }

 close_database(ctx);

 return 0;
}

int service_list_branches(struct buildmatrix_context *ctx)
{
 char *sql;
 const unsigned char *name;
 struct sqlite3_stmt *statement = NULL;
 int code, count = 0;

 if(ctx->verbose > 1)
  fprintf(stderr, 
          "%s: service_list_branches()\n", ctx->error_prefix);

 if(open_database(ctx))
  return 1;

 if(ctx->job_name == NULL)
 { 
  if(ctx->build_node == NULL)
  {
   sql = "select distinct branch from builds;"; 
  } else {
   sql = "select distinct branch from builds where build_node = ?;"; 
  }
 } else {
  if(ctx->build_node == NULL)
  {
   sql = "select distinct branch from builds where job = ?;"; 
  } else {
   sql = "select distinct branch from builds where job = ? and build_node = ?;"; 
  }
 }

 if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
 {
  fprintf(stderr, "%s: service_list_branches(): sqlite3_prepare(%s) failed, '%s'\n",
          ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx)); 
  return 1;
 }

 if(ctx->job_name == NULL)
 { 
  if(ctx->build_node != NULL)
  {
   if(sqlite3_bind_text(statement, 1, ctx->build_node, strlen(ctx->build_node), 
                        SQLITE_TRANSIENT) != SQLITE_OK)
   {
    fprintf(stderr, "%s: service_list_branches(): sqlite3_bind_text() failed, '%s'\n",
            ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
    sqlite3_finalize(statement);
    return 1;
   }
  }
 } else {
  if(sqlite3_bind_text(statement, 1, ctx->job_name, strlen(ctx->job_name), 
                       SQLITE_TRANSIENT) != SQLITE_OK)
  {
   fprintf(stderr, "%s: service_list_branches(): sqlite3_bind_text() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }

  if(ctx->build_node != NULL)
  {
   if(sqlite3_bind_text(statement, 2, ctx->build_node, strlen(ctx->build_node), 
                        SQLITE_TRANSIENT) != SQLITE_OK)
   {
    fprintf(stderr, "%s: service_list_jobs(): sqlite3_bind_text() failed, '%s'\n",
            ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
    sqlite3_finalize(statement);
    return 1;
   }
  }
 }

 if(ctx->service_simple_lists_strategy.start(ctx, 
                                             ctx->service_simple_lists_strategy.strategy_context,
                                             NULL))
 {
  if(ctx->verbose > 1)
   fprintf(stderr, "%s: service_list_jobs(): strategy callback failed\n",
           ctx->error_prefix);
  return 1;
 }

 while((code = sqlite3_step(statement)) == SQLITE_ROW)
 {

  if(sqlite3_column_type(statement, 0) != SQLITE_TEXT)
  {
   fprintf(stderr, "%s: service_list_branches(): job is not text\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  name = sqlite3_column_text(statement, 0);

  if(ctx->service_simple_lists_strategy.iterative(ctx, 
                                                  ctx->service_simple_lists_strategy.strategy_context,
                                                  name))
  {
   if(ctx->verbose > 1)
    fprintf(stderr, "%s: service_list_jobs(): strategy callback failed\n",
            ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }

  count++;
 }

 if(code != SQLITE_DONE)
 {
  fprintf(stderr, "%s: service_list_branches(): sqlite3_step() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 sqlite3_finalize(statement);

 if(ctx->verbose)
  fprintf(stderr, 
          "%s: service_list_branches(): sent %d branch name(s) with job name = %s, and build host = %s\n",
          ctx->error_prefix, count, ctx->job_name, ctx->build_node);

 if(ctx->service_simple_lists_strategy.done(ctx, ctx->service_simple_lists_strategy.strategy_context, NULL))
 {
  if(ctx->verbose > 1)
   fprintf(stderr, "%s: service_list_jobs(): strategy callback failed\n",
           ctx->error_prefix);
  return 1;
 }

 close_database(ctx);

 return 0;
}

int service_list_hosts(struct buildmatrix_context *ctx)
{
 char *sql;
 const unsigned char *name;
 struct sqlite3_stmt *statement = NULL;
 int code, count = 0;

 if(ctx->verbose > 1)
  fprintf(stderr, 
          "%s: service_list_hosts()\n", ctx->error_prefix);

 if(open_database(ctx))
  return 1;

 if(ctx->branch_name == NULL)
 { 
  if(ctx->job_name == NULL)
  {
   sql = "select distinct build_node from builds;"; 
  } else {
   sql = "select distinct build_node from builds where job = ?;"; 
  }
 } else {
  if(ctx->job_name == NULL)
  {
   sql = "select distinct build_node from builds where branch = ?;"; 
  } else {
   sql = "select distinct build_node from builds where branch = ? and job = ?;"; 
  }
 }

 if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
 {
  fprintf(stderr, "%s: service_list_hosts(): sqlite3_prepare(%s) failed, '%s'\n",
          ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx)); 
  return 1;
 }

 if(ctx->branch_name == NULL)
 { 
  if(ctx->job_name != NULL)
  {
   if(sqlite3_bind_text(statement, 1, ctx->job_name, strlen(ctx->job_name), 
                        SQLITE_TRANSIENT) != SQLITE_OK)
   {
    fprintf(stderr, "%s: service_list_hosts(): sqlite3_bind_text() failed, '%s'\n",
            ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
    sqlite3_finalize(statement);
    return 1;
   }
  }
 } else {
  if(sqlite3_bind_text(statement, 1, ctx->branch_name, strlen(ctx->branch_name), 
                       SQLITE_TRANSIENT) != SQLITE_OK)
  {
   fprintf(stderr, "%s: service_list_hosts(): sqlite3_bind_text() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }

  if(ctx->job_name != NULL)
  {
   if(sqlite3_bind_text(statement, 2, ctx->job_name, strlen(ctx->job_name), 
                        SQLITE_TRANSIENT) != SQLITE_OK)
   {
    fprintf(stderr, "%s: service_list_hosts(): sqlite3_bind_text() failed, '%s'\n",
            ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
    sqlite3_finalize(statement);
    return 1;
   }
  }
 }

 if(ctx->service_simple_lists_strategy.start(ctx, 
                                             ctx->service_simple_lists_strategy.strategy_context,
                                             NULL))
 {
  if(ctx->verbose > 1)
   fprintf(stderr, "%s: service_list_jobs(): strategy callback failed\n",
           ctx->error_prefix);
  return 1;
 }

 while((code = sqlite3_step(statement)) == SQLITE_ROW)
 {

  if(sqlite3_column_type(statement, 0) != SQLITE_TEXT)
  {
   fprintf(stderr, "%s: service_list_hosts(): job is not text\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  name = sqlite3_column_text(statement, 0);

  if(ctx->service_simple_lists_strategy.iterative(ctx, 
                                                  ctx->service_simple_lists_strategy.strategy_context,
                                                  name))
  {
   if(ctx->verbose > 1)
    fprintf(stderr, "%s: service_list_jobs(): strategy callback failed\n",
            ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }

  count++;
 }

 if(code != SQLITE_DONE)
 {
  fprintf(stderr, "%s: service_list_hosts(): sqlite3_step() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 sqlite3_finalize(statement);

 if(ctx->verbose)
  fprintf(stderr, 
          "%s: service_list_hosts(): sent %d hosts name(s) with job name = %s, and branch = %s\n",
          ctx->error_prefix, count, ctx->job_name, ctx->branch_name);

 if(ctx->service_simple_lists_strategy.done(ctx, ctx->service_simple_lists_strategy.strategy_context, NULL))
 {
  if(ctx->verbose > 1)
   fprintf(stderr, "%s: service_list_jobs(): strategy callback failed\n",
           ctx->error_prefix);
  return 1;
 }

 close_database(ctx);

 return 0;
}

int service_list_users(struct buildmatrix_context *ctx)
{
 char *sql;
 const unsigned char *name;
 struct sqlite3_stmt *statement = NULL;
 int code, count = 0;

 if(ctx->verbose > 1)
  fprintf(stderr, 
          "%s: service_list_users()\n", ctx->error_prefix);

 if(open_database(ctx))
  return 1;

 if(ctx->branch_name == NULL)
 { 
  if(ctx->job_name == NULL)
  {
   sql = "SELECT distinct users.name FROM builds JOIN users ON builds.user_id;"; 
  } else {
   sql = "SELECT distinct users.name FROM builds JOIN users ON builds.user_id WHERE job = ?;"; 
  }
 } else {
  if(ctx->job_name == NULL)
  {
   sql = "SELECT distinct users.name FROM builds JOIN users ON builds.user_id WHERE branch = ?;"; 
  } else {
   sql = "SELECT distinct users.name FROM builds JOIN users ON builds.user_id WHERE branch = ? AND job = ?;"; 
  }
 }

 if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
 {
  fprintf(stderr, "%s: service_list_users(): sqlite3_prepare(%s) failed, '%s'\n",
          ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx)); 
  return 1;
 }

 if(ctx->branch_name == NULL)
 { 
  if(ctx->job_name != NULL)
  {
   if(sqlite3_bind_text(statement, 1, ctx->job_name, strlen(ctx->job_name), 
                        SQLITE_TRANSIENT) != SQLITE_OK)
   {
    fprintf(stderr, "%s: service_list_users(): sqlite3_bind_text() failed, '%s'\n",
            ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
    sqlite3_finalize(statement);
    return 1;
   }
  }
 } else {
  if(sqlite3_bind_text(statement, 1, ctx->branch_name, strlen(ctx->branch_name), 
                       SQLITE_TRANSIENT) != SQLITE_OK)
  {
   fprintf(stderr, "%s: service_list_users(): sqlite3_bind_text() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }

  if(ctx->job_name != NULL)
  {
   if(sqlite3_bind_text(statement, 2, ctx->job_name, strlen(ctx->job_name), 
                        SQLITE_TRANSIENT) != SQLITE_OK)
   {
    fprintf(stderr, "%s: service_list_users(): sqlite3_bind_text() failed, '%s'\n",
            ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
    sqlite3_finalize(statement);
    return 1;
   }
  }
 }

 if(ctx->service_simple_lists_strategy.start(ctx, 
                                             ctx->service_simple_lists_strategy.strategy_context,
                                             NULL))
 {
  if(ctx->verbose > 1)
   fprintf(stderr, "%s: service_list_jobs(): strategy callback failed\n",
           ctx->error_prefix);
  return 1;
 }

 while((code = sqlite3_step(statement)) == SQLITE_ROW)
 {

  if(sqlite3_column_type(statement, 0) != SQLITE_TEXT)
  {
   fprintf(stderr, "%s: service_list_users(): job is not text\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  name = sqlite3_column_text(statement, 0);

  if(ctx->service_simple_lists_strategy.iterative(ctx, 
                                                  ctx->service_simple_lists_strategy.strategy_context,
                                                  name))
  {
   if(ctx->verbose > 1)
    fprintf(stderr, "%s: service_list_jobs(): strategy callback failed\n",
            ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }

  count++;
 }

 if(code != SQLITE_DONE)
 {
  fprintf(stderr, "%s: service_list_users(): sqlite3_step() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 sqlite3_finalize(statement);

 if(ctx->verbose)
  fprintf(stderr, 
          "%s: service_list_users(): sent %d user name(s) with job name = %s, and branch = %s\n",
          ctx->error_prefix, count, ctx->job_name, ctx->branch_name);

 if(ctx->service_simple_lists_strategy.done(ctx, ctx->service_simple_lists_strategy.strategy_context, NULL))
 {
  if(ctx->verbose > 1)
   fprintf(stderr, "%s: service_list_users(): strategy callback failed\n",
           ctx->error_prefix);
  return 1;
 }

 close_database(ctx);

 return 0;
}

int service_list_parameters(struct buildmatrix_context *ctx)
{
 const char *sql;
 const unsigned char *parameter;
 struct sqlite3_stmt *statement = NULL;
 int code, count = 0;

 if(ctx->verbose > 1)
  fprintf(stderr, 
          "%s: service_list_parameters()\n", ctx->error_prefix);

 if(open_database(ctx))
  return 1;

 sql = "SELECT distinct parameters.variable FROM parameters JOIN builds ON parameters.build_id;"; 

 if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
 {
  fprintf(stderr, "%s: service_list_parameters(): sqlite3_prepare(%s) failed, '%s'\n",
          ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx)); 
  return 1;
 }

 if(ctx->service_simple_lists_strategy.start(ctx, 
                                             ctx->service_simple_lists_strategy.strategy_context,
                                             NULL))
 {
  if(ctx->verbose > 1)
   fprintf(stderr, "%s: service_list_parameters(): strategy callback failed\n",
           ctx->error_prefix);
  return 1;
 }

 while((code = sqlite3_step(statement)) == SQLITE_ROW)
 {

  if(sqlite3_column_type(statement, 0) != SQLITE_TEXT)
  {
   fprintf(stderr, "%s: service_list_parameters(): job is not text\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  parameter = sqlite3_column_text(statement, 0);

  if(ctx->service_simple_lists_strategy.iterative(ctx, 
                                                  ctx->service_simple_lists_strategy.strategy_context,
                                                  parameter))
  {
   if(ctx->verbose > 1)
    fprintf(stderr, "%s: service_list_parameters(): strategy callback failed\n",
            ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }

  count++;
 }

 if(code != SQLITE_DONE)
 {
  fprintf(stderr, "%s: service_list_parameters(): sqlite3_step() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 sqlite3_finalize(statement);

 if(ctx->service_simple_lists_strategy.done(ctx, ctx->service_simple_lists_strategy.strategy_context, NULL))
 {
  if(ctx->verbose > 1)
   fprintf(stderr, "%s: service_list_parameters(): strategy callback failed\n",
           ctx->error_prefix);
  return 1;
 }

 close_database(ctx);

 return 0;
}

int service_list_builds(struct buildmatrix_context *ctx)
{
 char sql[1024];
 struct sqlite3_stmt *statement = NULL;
 int code, sql_length, col_number = 1, where = 0, count;
 struct list_builds_data data;

 if(ctx->verbose > 1)
  fprintf(stderr, 
          "%s: service_list_builds()\n", ctx->error_prefix);

 if(open_database(ctx))
  return 1;

 sql_length = 0;

 /* branch name, job name, build node, unique_identifier, user, build result, build time, totals  */

 sql_length =
 snprintf(sql, 1024 - sql_length,  
          "SELECT build_id, branch, job, build_node, unique_identifier, users.name, result, "
          "build_time, passed_tests, failed_tests, incomplete_tests, report, output, checksum, "
          "has_tests, has_parameters, revision FROM builds JOIN users on builds.user_id"); 

 if( (ctx->branch_name != NULL) ||
     (ctx->job_name != NULL) ||
     (ctx->build_node != NULL) )
  where = 1;

 if(where)
  sql_length += snprintf(sql + sql_length, 1024 - sql_length, " WHERE ");

 if(ctx->branch_name != NULL)
 {
  sql_length += 
  snprintf(sql + sql_length, 1024 - sql_length,
           "branch = ? and ");
 }
 
 if(ctx->job_name != NULL)
 {
  sql_length += 
  snprintf(sql + sql_length, 1024 - sql_length,
           "job = ? and ");
 }
 
 if(ctx->build_node != NULL)
 {
  sql_length += 
  snprintf(sql + sql_length, 1024 - sql_length,
           "build_node = ? and ");
 }

 if(where)
  sql_length -= 4;

 sql_length += 
 snprintf(sql + sql_length, 1024 - sql_length, ";");

 if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
 {
  fprintf(stderr, "%s: service_list_builds(): sqlite3_prepare(%s) failed, '%s'\n",
          ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx)); 
  return 1;
 }

 if(ctx->branch_name != NULL)
 {
  if(sqlite3_bind_text(statement, col_number, ctx->branch_name, strlen(ctx->branch_name),
                       SQLITE_TRANSIENT) != SQLITE_OK)
  {
   fprintf(stderr, "%s: service_list_builds(): sqlite3_bind_text() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }
  col_number++;
 }

 if(ctx->job_name != NULL)
 {
  if(sqlite3_bind_text(statement, col_number, ctx->job_name, strlen(ctx->job_name),
                       SQLITE_TRANSIENT) != SQLITE_OK)
  {
   fprintf(stderr, "%s: service_list_builds(): sqlite3_bind_text() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }
  col_number++;
 }

 if(ctx->build_node != NULL)
 {
  if(sqlite3_bind_text(statement, col_number, ctx->build_node, strlen(ctx->build_node),
                       SQLITE_TRANSIENT) != SQLITE_OK)
  {
   fprintf(stderr, "%s: service_list_builds(): sqlite3_bind_text() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }
  col_number++;
 }

 if(ctx->service_list_builds_strategy.start(ctx, 
                                            ctx->service_list_builds_strategy.strategy_context,
                                            NULL))
 {
  if(ctx->verbose > 1)
   fprintf(stderr, "%s: service_list_builds(): strategy callback failed\n",
           ctx->error_prefix);
  return 1;
 }

 count = 0;
 while((code = sqlite3_step(statement)) == SQLITE_ROW)
 {

  if(sqlite3_column_type(statement, 0) != SQLITE_INTEGER)
  {
   fprintf(stderr, "%s: service_list_builds(): builds.build_id is not an integer\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
    return 1;
  }
  data.build_id = sqlite3_column_int(statement, 0);

  if(sqlite3_column_type(statement, 1) != SQLITE_TEXT)
  {
   fprintf(stderr, "%s: service_list_builds(): branch_name is not text\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  data.branch = (const char *) sqlite3_column_text(statement, 1);

  if(sqlite3_column_type(statement, 2) != SQLITE_TEXT)
  {
   fprintf(stderr, "%s: service_list_builds(): job_name is not text\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  data.job_name = (const char *) sqlite3_column_text(statement, 2);

  if(sqlite3_column_type(statement, 3) != SQLITE_TEXT)
  {
   fprintf(stderr, "%s: service_list_builds(): build_node is not text\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  data.build_node = (const char *) sqlite3_column_text(statement, 3);

  if(sqlite3_column_type(statement, 4) != SQLITE_TEXT)
  {
   fprintf(stderr, "%s: service_list_builds(): unique_identifier is not text\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  data.unique_identifier = (const char *) sqlite3_column_text(statement, 4);

  if(sqlite3_column_type(statement, 5) != SQLITE_TEXT)
  {
   fprintf(stderr, "%s: service_list_builds(): users.name is not text\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  data.user = (const char *) sqlite3_column_text(statement, 5);

  if(sqlite3_column_type(statement, 6) != SQLITE_INTEGER)
  {
   fprintf(stderr, "%s: service_list_builds(): builds.result is not an integer\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  data.build_result = sqlite3_column_int(statement, 6);

  if(sqlite3_column_type(statement, 7) != SQLITE_INTEGER)
  {
   fprintf(stderr, "%s: service_list_builds(): builds.build_time is not an integer\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  data.build_time = sqlite3_column_int64(statement, 7);

  if(sqlite3_column_type(statement, 8) != SQLITE_INTEGER)
  {
   fprintf(stderr, "%s: service_list_builds(): builds.passed_tests is not an integer\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  data.passed = sqlite3_column_int(statement, 8);

  if(sqlite3_column_type(statement, 9) != SQLITE_INTEGER)
  {
   fprintf(stderr, "%s: service_list_builds(): builds.failed_tests is not an integer\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  data.failed = sqlite3_column_int(statement, 9);

  if(sqlite3_column_type(statement, 10) != SQLITE_INTEGER)
  {
   fprintf(stderr, "%s: service_list_builds(): builds.incomplete_tests is not an integer\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  data.incompleted = sqlite3_column_int(statement, 10);

  if(sqlite3_column_type(statement, 11) != SQLITE_NULL)
  {
   if(sqlite3_column_type(statement, 11) != SQLITE_TEXT)
   {
    fprintf(stderr, "%s: service_list_builds(): report_name is not text\n",
            ctx->error_prefix);
    sqlite3_finalize(statement);
    return 1;
   }
   data.report_name = (const char *) sqlite3_column_text(statement, 11);
  } else {
   data.report_name = NULL;
  }

  if(sqlite3_column_type(statement, 12) != SQLITE_NULL)
  {
   if(sqlite3_column_type(statement, 12) != SQLITE_TEXT)
   {
    fprintf(stderr, "%s: service_list_builds(): report_name is not text\n",
            ctx->error_prefix);
    sqlite3_finalize(statement);
    return 1;
   }
   data.output_name = (const char *) sqlite3_column_text(statement, 12);
  } else {
   data.output_name = NULL;
  }

  if(sqlite3_column_type(statement, 13) != SQLITE_NULL)
  {
   if(sqlite3_column_type(statement, 13) != SQLITE_TEXT)
   {
    fprintf(stderr, "%s: service_list_builds(): report_name is not text\n",
            ctx->error_prefix);
    sqlite3_finalize(statement);
    return 1;
   }
   data.checksum_name = (const char *) sqlite3_column_text(statement, 13);
  } else {
   data.checksum_name = NULL;
  }

  if(sqlite3_column_type(statement, 14) != SQLITE_INTEGER)
  {
   fprintf(stderr, "%s: service_list_builds(): builds.has_tests is not an integer\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  data.has_tests = sqlite3_column_int(statement, 14);

  if(sqlite3_column_type(statement, 15) != SQLITE_INTEGER)
  {
   fprintf(stderr, "%s: service_list_builds(): builds.has_parameters is not an integer\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  data.has_parameters = sqlite3_column_int(statement, 15);

  if(sqlite3_column_type(statement, 16) != SQLITE_NULL)
  {
   if(sqlite3_column_type(statement, 16) != SQLITE_TEXT)
   {
    fprintf(stderr, "%s: service_list_builds(): builds.revision is not text\n",
            ctx->error_prefix);
    sqlite3_finalize(statement);
    return 1;
   }
   data.revision = (const char *) sqlite3_column_text(statement, 16);
  } else {
   data.revision = "N/A";
  }

  if(ctx->service_list_builds_strategy.iterative(ctx, 
                                                 ctx->service_list_builds_strategy.strategy_context,
                                                 &data))
  {
   if(ctx->verbose > 1)
    fprintf(stderr, "%s: service_list_builds(): strategy callback failed\n",
            ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  count++;
 }

 if(ctx->service_list_builds_strategy.done(ctx, ctx->service_list_builds_strategy.strategy_context,
                                           NULL))
 {
  if(ctx->verbose > 1)
   fprintf(stderr, "%s: service_list_builds(): strategy callback failed\n",
           ctx->error_prefix);
  return 1;
 }

 if(code != SQLITE_DONE)
 {
  fprintf(stderr, "%s: service_list_builds(): sqlite3_step() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 sqlite3_finalize(statement);

 if(ctx->verbose )
  fprintf(stderr, 
          "%s: service_list_builds(): sent %d records\n", ctx->error_prefix, count);

 close_database(ctx);

 return 0;
}


int service_get_build(struct buildmatrix_context *ctx)
{
 char *sql;
 struct sqlite3_stmt *statement = NULL;
 int code;

 if(ctx->verbose > 1)
  fprintf(stderr, 
          "%s: service_get_build()\n", ctx->error_prefix);

 if(open_database(ctx))
  return 1;

 /* branch name, job name, build node, unique_identifier, user, build result, start time, end time, totals  */

 sql = "SELECT build_id, build_node, result, build_time, "
       "passed_tests, failed_tests, incomplete_tests, "
       "report, output, checksum, job, branch, users.name, revision "
       "FROM builds JOIN users on builds.user_id WHERE unique_identifier = ?";

 if(ctx->unique_identifier == NULL)
 {
  if(ctx->verbose)
  fprintf(stderr, "%s: service_get_build(): need unique identifier\n",
          ctx->error_prefix); 
  return 1;
 }

 if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
 {
  fprintf(stderr, "%s: service_get_build(): sqlite3_prepare(%s) failed, '%s'\n",
          ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx)); 
  return 1;
 }

 if(sqlite3_bind_text(statement, 1, ctx->unique_identifier, strlen(ctx->unique_identifier),
                      SQLITE_TRANSIENT) != SQLITE_OK)
 {
  fprintf(stderr, "%s: service_get_build(): sqlite3_bind_text() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 if((code = sqlite3_step(statement)) == SQLITE_ROW)
 {
  if(sqlite3_column_type(statement, 0) != SQLITE_INTEGER)
  {
   fprintf(stderr, "%s: service_get_build(): builds.build_id is not an integer\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
    return 1;
  }
  ctx->build_id = sqlite3_column_int(statement, 0);

  if(sqlite3_column_type(statement, 1) != SQLITE_TEXT)
  {
   fprintf(stderr, "%s: service_get_build(): build_node is not text\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  ctx->build_node = strdup((const char *) sqlite3_column_text(statement, 1));

  if(sqlite3_column_type(statement, 2) != SQLITE_INTEGER)
  {
   fprintf(stderr, "%s: service_get_build(): builds.result is not an integer\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
    return 1;
  }
  ctx->build_result = sqlite3_column_int(statement, 2);

  if(sqlite3_column_type(statement, 3) != SQLITE_INTEGER)
  {
   fprintf(stderr, "%s: service_get_build(): builds.build_time is not an integer\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  ctx->build_time = sqlite3_column_int64(statement, 3);
 
  if(sqlite3_column_type(statement, 4) != SQLITE_INTEGER)
  {
   fprintf(stderr, "%s: service_get_build(): builds.passed_tests is not an integer\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  ctx->test_totals[1] = sqlite3_column_int(statement, 4);
  
  if(sqlite3_column_type(statement, 5) != SQLITE_INTEGER)
  {
   fprintf(stderr, "%s: service_get_build(): builds.failed_tests is not an integer\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  ctx->test_totals[2] = sqlite3_column_int(statement, 5);

  if(sqlite3_column_type(statement, 6) != SQLITE_INTEGER)
  {
   fprintf(stderr, "%s: service_get_build(): builds.incomplete_tests is not an integer - %d\n",
           ctx->error_prefix, sqlite3_column_type(statement, 6));
   sqlite3_finalize(statement);
   return 1;
  }
  ctx->test_totals[3] = sqlite3_column_int(statement, 6);

  if(sqlite3_column_type(statement, 7) != SQLITE_NULL)
  {
   if(sqlite3_column_type(statement, 7) != SQLITE_TEXT)
   {
    fprintf(stderr, "%s: service_get_build(): builds.build_report is not text\n",
            ctx->error_prefix);
    sqlite3_finalize(statement);
    return 1;
   }
   ctx->build_report = strdup((const char *) sqlite3_column_text(statement, 7));
  }

  if(sqlite3_column_type(statement, 8) != SQLITE_NULL)
  {
   if(sqlite3_column_type(statement, 8) != SQLITE_TEXT)
   {
    fprintf(stderr, "%s: service_get_build(): builds.build_output is not text\n",
            ctx->error_prefix);
    sqlite3_finalize(statement);
    return 1;
   }
   ctx->build_output = strdup((const char *) sqlite3_column_text(statement, 8));
  }

  if(sqlite3_column_type(statement, 9) != SQLITE_NULL)
  {
   if(sqlite3_column_type(statement, 9) != SQLITE_TEXT)
   {
    fprintf(stderr, "%s: service_get_build(): builds.checksum is not text\n",
            ctx->error_prefix);
    sqlite3_finalize(statement);
    return 1;
   }
   ctx->checksum = strdup((const char *) sqlite3_column_text(statement, 9));
  }

  if(sqlite3_column_type(statement, 10) != SQLITE_TEXT)
  {
   fprintf(stderr, "%s: service_get_build(): job is not text\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  ctx->job_name = strdup((const char *) sqlite3_column_text(statement, 10));

  if(sqlite3_column_type(statement, 11) != SQLITE_TEXT)
  {
   fprintf(stderr, "%s: service_get_build(): branch is not text\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  ctx->branch_name = strdup((const char *) sqlite3_column_text(statement, 11));

  if(sqlite3_column_type(statement, 12) != SQLITE_TEXT)
  {
   fprintf(stderr, "%s: service_get_build(): users.name is not text\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  ctx->user = strdup((const char *) sqlite3_column_text(statement, 12));

 } else {
  fprintf(stderr, "%s: service_get_build(): sqlite3_step() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 if(sqlite3_column_type(statement, 13) != SQLITE_NULL)
 {
  if(sqlite3_column_type(statement, 13) != SQLITE_TEXT)
  {
   fprintf(stderr, "%s: service_get_build(): builds.revision is not text\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  ctx->revision = strdup((const char *) sqlite3_column_text(statement, 13));
 }

 sqlite3_finalize(statement);
 close_database(ctx);

 return 0;
}

int service_scratch(struct buildmatrix_context *ctx)
{
 char *sql, *errmsg;
 struct sqlite3_stmt *statement = NULL;
 int code;

 if(ctx->verbose)
  fprintf(stderr, 
          "%s: service_scratch()\n", ctx->error_prefix);

 if(ctx->unique_identifier == NULL)
 {
  fprintf(stderr, "%s: service_scratch(): I need a unique identifier\n", ctx->error_prefix);
  return 1;
 }

 if(open_database(ctx))
  return 1;

 if(sqlite3_exec(ctx->db_ctx, "BEGIN TRANSACTION;",
    NULL, NULL, &errmsg) != SQLITE_OK)
 {
  fprintf(stderr, "%s: service_scratch(): sqlite3_exec(\"BEGIN TRANSACTION;\"): '%s'\n",
          ctx->error_prefix, errmsg);
  sqlite3_free(errmsg);
  return 1;
 }

 if(lookup_build_id(ctx))
  return 1;

 sql = "SELECT report, output, checksum FROM builds WHERE build_id = ?";

 if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
 {
  fprintf(stderr, "%s: service_scratch(): sqlite3_prepare(%s) failed, '%s'\n",
          ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx)); 
  return 1;
 }

 if(sqlite3_bind_int(statement, 1, ctx->build_id) != SQLITE_OK)
 {
  fprintf(stderr, "%s: service_scratch(): sqlite3_bind_int() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 code = sqlite3_step(statement);
 if(code != SQLITE_ROW)
 {
  fprintf(stderr, "%s: service_scratch(): sqlite3_step() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 if(sqlite3_column_type(statement, 1) != SQLITE_NULL)
 {
  if(sqlite3_column_type(statement, 1) != SQLITE_TEXT)
  { 
   fprintf(stderr, "%s: service_scratch(): report is not text\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  ctx->build_report = strdup((const char *) sqlite3_column_text(statement, 1));
 }

 if(sqlite3_column_type(statement, 2) != SQLITE_NULL)
 {
  if(sqlite3_column_type(statement, 2) != SQLITE_TEXT)
  {
   fprintf(stderr, "%s: service_scratch(): output is not text\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  ctx->build_output = strdup((const char *) sqlite3_column_text(statement, 2));
 }

 if(sqlite3_column_type(statement, 3) != SQLITE_NULL)
 {
  if(sqlite3_column_type(statement, 3) != SQLITE_TEXT)
  {
   fprintf(stderr, "%s: service_scratch(): checksum is not text\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  ctx->checksum = strdup((const char *) sqlite3_column_text(statement, 3));
 }

 sqlite3_finalize(statement);

 sql = "DELETE FROM builds WHERE unique_identifier = ?;";

 if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
 {
  fprintf(stderr, "%s: service_scratch(): sqlite3_prepare(%s) failed, '%s'\n",
          ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx)); 
  return 1;
 }

 if(sqlite3_bind_text(statement, 1, ctx->unique_identifier, strlen(ctx->unique_identifier),
                      SQLITE_TRANSIENT) != SQLITE_OK)
 {
  fprintf(stderr, "%s: service_scratch(): sqlite3_bind_text() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 code = sqlite3_step(statement);
 if(code != SQLITE_DONE)
 {
  fprintf(stderr, "%s: service_scratch(): sqlite3_step() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 sqlite3_finalize(statement);

 sql = "DELETE FROM scores WHERE build_id = ?;";

 if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
 {
  fprintf(stderr, "%s: service_scratch(): sqlite3_prepare(%s) failed, '%s'\n",
          ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx)); 
  return 1;
 }

 if(sqlite3_bind_int(statement, 1, ctx->build_id) != SQLITE_OK)
 {
  fprintf(stderr, "%s: service_scratch(): sqlite3_bind_int() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 code = sqlite3_step(statement);
 if(code != SQLITE_DONE)
 {
  fprintf(stderr, "%s: service_scratch(): sqlite3_step() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 sqlite3_finalize(statement);

 sql = "DELETE FROM parameters WHERE build_id = ?;";

 if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
 {
  fprintf(stderr, "%s: service_scratch(): sqlite3_prepare(%s) failed, '%s'\n",
          ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx)); 
  return 1;
 }

 if(sqlite3_bind_int(statement, 1, ctx->build_id) != SQLITE_OK)
 {
  fprintf(stderr, "%s: service_scratch(): sqlite3_bind_int() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 code = sqlite3_step(statement);
 if(code != SQLITE_DONE)
 {
  fprintf(stderr, "%s: service_scratch(): sqlite3_step() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 sqlite3_finalize(statement);

 // no point in checking fail, since this doesn't jive with the transaction model anyway
 remove_db_file(ctx, ctx->build_report);
 remove_db_file(ctx, ctx->build_output);
 remove_db_file(ctx, ctx->checksum);

 if(sqlite3_exec(ctx->db_ctx, "COMMIT TRANSACTION;", NULL, NULL, &errmsg) != SQLITE_OK)
 {
  fprintf(stderr, "%s: service_scratch(): sqlite3_exec(\"COMMIT TRANSACTION;\"): '%s'\n",
          ctx->error_prefix, errmsg);
  sqlite3_free(errmsg);
  return 1;
 }

 close_database(ctx);

 return 0;
}

int service_pull(struct buildmatrix_context *ctx)
{
 char *sql;
 struct sqlite3_stmt *statement = NULL;
 int code, has_tests = 0, has_parameters = 0;

 if(ctx->verbose > 1)
  fprintf(stderr, 
          "%s: service_pull() %d\n", ctx->error_prefix, ctx->pull_build_id);

 /* branch name, job name, build node, build number, user, build result, build time, totals  */

 sql = "SELECT build_id, branch, job, unique_identifier, build_node, result, build_time, "
       "passed_tests, failed_tests, incomplete_tests, report, output, checksum, users.name, "
       "revision, has_tests, has_parameters "
       "FROM builds JOIN users on builds.user_id WHERE build_id > ? ORDER BY build_id LIMIT 1";

 if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
 {
  fprintf(stderr, "%s: service_pull(): sqlite3_prepare(%s) failed, '%s'\n",
          ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx)); 
  return 1;
 }

 if(sqlite3_bind_int(statement, 1, ctx->pull_build_id) != SQLITE_OK)
 {
  fprintf(stderr, "%s: service_pull(): sqlite3_bind_int() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 if((code = sqlite3_step(statement)) == SQLITE_ROW)
 {
  if(sqlite3_column_type(statement, 0) != SQLITE_INTEGER)
  {
   fprintf(stderr, "%s: service_pull(): builds.build_id is not an integer\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
    return 1;
  }
  ctx->build_id = sqlite3_column_int(statement, 0);

  if(sqlite3_column_type(statement, 1) != SQLITE_TEXT)
  {
   fprintf(stderr, "%s: service_pull(): branch_name is not text\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  ctx->branch_name = strdup((const char *) sqlite3_column_text(statement, 1));

  if(sqlite3_column_type(statement, 2) != SQLITE_TEXT)
  {
   fprintf(stderr, "%s: service_pull(): job_name is not text\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  ctx->job_name = strdup((const char *) sqlite3_column_text(statement, 2));

  if(sqlite3_column_type(statement, 3) != SQLITE_TEXT)
  {
   fprintf(stderr, "%s: service_pull(): unique_identifier is not text\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  ctx->unique_identifier = strdup((const char *) sqlite3_column_text(statement, 3));

  if(sqlite3_column_type(statement, 4) != SQLITE_TEXT)
  {
   fprintf(stderr, "%s: service_pull(): build_node is not text\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  ctx->build_node = strdup((const char *) sqlite3_column_text(statement, 4));

  if(sqlite3_column_type(statement, 5) != SQLITE_INTEGER)
  {
   fprintf(stderr, "%s: service_pull(): builds.result is not an integer\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
    return 1;
  }
  ctx->build_result = sqlite3_column_int(statement, 5);

  if(sqlite3_column_type(statement, 6) != SQLITE_INTEGER)
  {
   fprintf(stderr, "%s: service_pull(): builds.build_time is not an integer\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  ctx->build_time = sqlite3_column_int64(statement, 6);

  if(sqlite3_column_type(statement, 7) != SQLITE_INTEGER)
  {
   fprintf(stderr, "%s: service_pull(): builds.passed_tests is not an integer\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  ctx->test_totals[1] = sqlite3_column_int(statement, 7);
  
  if(sqlite3_column_type(statement, 8) != SQLITE_INTEGER)
  {
   fprintf(stderr, "%s: service_pull(): builds.failed_tests is not an integer\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  ctx->test_totals[2] = sqlite3_column_int(statement, 8);

  if(sqlite3_column_type(statement, 9) != SQLITE_INTEGER)
  {
   fprintf(stderr, "%s: service_pull(): builds.incomplete_tests is not an integer\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  ctx->test_totals[3] = sqlite3_column_int(statement, 9);

  if(sqlite3_column_type(statement, 10) == SQLITE_TEXT)
   ctx->build_report = strdup((const char *) sqlite3_column_text(statement, 10));
 
  if(sqlite3_column_type(statement, 11) == SQLITE_TEXT)
   ctx->build_output = strdup((const char *) sqlite3_column_text(statement, 11));

  if(sqlite3_column_type(statement, 12) == SQLITE_TEXT)
   ctx->checksum = strdup((const char *) sqlite3_column_text(statement, 12));

  if(sqlite3_column_type(statement, 13) != SQLITE_TEXT)
  {
   fprintf(stderr, "%s: service_pull(): usrs.name is not text\n", ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  ctx->user = strdup((const char *) sqlite3_column_text(statement, 13));

  if(sqlite3_column_type(statement, 14) == SQLITE_TEXT)
   ctx->revision = strdup((const char *) sqlite3_column_text(statement, 14));

  if(sqlite3_column_type(statement, 15) == SQLITE_INTEGER)
   has_tests = sqlite3_column_int(statement, 15);

  if(sqlite3_column_type(statement, 16) == SQLITE_INTEGER)
   has_parameters = sqlite3_column_int(statement, 16);

 } else {
  if(code == SQLITE_DONE)
  {
   sqlite3_finalize(statement);
   return 2;
  }

  fprintf(stderr, "%s: service_pull(): build_id = %d, sqlite3_step() failed, '%s'\n",
          ctx->error_prefix, ctx->build_id, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 sqlite3_finalize(statement);

 if(has_tests)
 {
  if(pull_test_results(ctx, &(ctx->test_results)))
  {
   fprintf(stderr, "%s: service_pull(): pull_test_results() failed\n", ctx->error_prefix);
   return 1;
  }
 }

 if(has_parameters)
 {
  if(load_parameters_from_db(ctx))
  {
   fprintf(stderr, "%s: service_pull(): load_parameters_from_db() failed\n", ctx->error_prefix);
   return 1;
  }
 }

 return 0;
}


