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

extern char **environ;

struct parameters *
allocate_parameter_structure(const struct buildmatrix_context *ctx, const int n_pairs,
                             const int string_space, char **string_arena)
{
 struct parameters *p;
 int allocation_size;

 allocation_size = sizeof(struct parameters)
                 + string_space
                 + (n_pairs + 1) * sizeof(char **) * 2;
 
 if((p = (struct parameters *) malloc(allocation_size)) == NULL)
 {
  fprintf(stderr, "%s: allocate_prameter_structure(): malloc(%d) failed: %s\n",
          ctx->error_prefix, allocation_size, strerror(errno));
  return NULL;
 }

 // structure|keys array|values array|strings space
 p->keys = (char **) (((char *) p) + sizeof(struct parameters));
 p->values = &(p->keys[n_pairs]);
 p->n_pairs = 0;
 *string_arena = ((char *) p) + sizeof(struct parameters) + (n_pairs * sizeof(char **) * 2);
 p->string_space = 0;

 return p;
}

int parameters_from_environ(struct buildmatrix_context *ctx)
{
// int i = 0, allocation_size;
// char *e;
// struct parameters *p;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: parameters_from_environ()\n", ctx->error_prefix);
/*
 while((e = environ[i]) != NULL)
 {
  printf("--'%s'\n", e);
  i++;
 }
*/
 return 0;
}

int print_parameters(struct buildmatrix_context *ctx)
{
 int x;

 if(ctx->parameters == NULL)
  return 1;

 for(x=0; x < ctx->parameters->n_pairs; x++)
 {
  fprintf(stderr, "%d: %s=%s\n", x + 1, ctx->parameters->keys[x], ctx->parameters->values[x]);
 }
 fprintf(stderr, " (%d) Parameter Values\n", x);

 return 0;
}

int parameters_to_file(struct buildmatrix_context *ctx, char *filename)
{
 FILE *out;
 int x;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: parameters_to_file()\n", ctx->error_prefix);

 if(filename == NULL)
  return 1;

 if((out = fopen(filename, "w")) == NULL)
 {
  fprintf(stderr, "%s: parameters_to_file(): fopen(%s) failed: %s\n",
          ctx->error_prefix, filename, strerror(errno));
  return 1;
 }

 for(x=0; x < ctx->parameters->n_pairs; x++)
 {
  fprintf(out, "%s|%s\n", ctx->parameters->keys[x], ctx->parameters->values[x]);
 }
 fprintf(out, "\n");

 fclose(out);
 return 0;
}

int parameters_from_file(struct buildmatrix_context *ctx, char *filename)
{
 char *contents, *str_ptr;
 int line_length, mark, n_pairs = 0, string_space = 0,
     pass = 1, x, key_length, value_length, line_number;
 size_t filesize, i;
 struct parameters *p;

 if(filename == NULL)
  return 1;

 if((contents = read_disk_file(ctx, filename, &filesize)) == NULL)
  return 1;

 while(pass < 3)
 {
  i = 0;
  line_number = 1;
  while(i<filesize)
  {
   line_length = 0;  
   while(1)
   {
    if(contents[i + line_length] == '\n')
     break;

    if(i + line_length == filesize)
    {
     fprintf(stderr, "%s: parameters_from_file(): %s line %d has no newline\n",
             ctx->error_prefix, filename, line_number);
     free(contents);
     return 1;
    }

    if(line_length > 511)
    {
     fprintf(stderr, "%s: parameters_from_file(): %s line %d is too long\n",
             ctx->error_prefix, filename, line_number);
     free(contents);
     return 1;
    }

    line_length++;
   }

   if(line_length < 1)
    break;

   x = i;
   mark = -1;
   while(x < i + line_length)
   {
    if(contents[x] == '|')
    {
     mark = x;
     break;
    }
    x++;
   }
   if(mark < 0)
   {
    contents[i + line_length] = 0;
    fprintf(stderr, "%s: parameters_from_file(): %s line %d has no | character, \"%s\"\n",
            ctx->error_prefix, filename, line_number, contents + i);
    free(contents);
    return 1;
   }

   key_length = mark - i;
   if( (key_length < 2) || (key_length > 256) )
   {
    fprintf(stderr, "%s: parameters_from_file(): %s line %d: key is wrong length\n",
            ctx->error_prefix, filename, line_number);
    free(contents);
    return 1;
   }

   value_length = line_length  - (mark - i) - 1;
   if( (value_length < 2) || (value_length > 256) )
   {
    fprintf(stderr, "%s: parameters_from_file(): %s line %d: value is wrong length\n",
            ctx->error_prefix, filename, line_number);
    free(contents);
    return 1;
   }

   /* first pass just counts sizes */
   if(pass == 1)
   {
    string_space += key_length + value_length + 2;
    n_pairs++;
   } else {

    if(p->n_pairs + 1 > n_pairs)
    {
     fprintf(stderr, "%s: parameters_from_file(): failure. p->npairs =%d n_pairs %d\n",
             ctx->error_prefix, p->n_pairs , n_pairs);
     free(contents);
     return 1;
    }

    if(p->string_space + key_length + 1 > string_space)
    {
     fprintf(stderr, "%s: parameters_from_file(): string space problem\n",
             ctx->error_prefix);
     free(contents);
     return 1;
    }
    p->keys[p->n_pairs] = str_ptr;
    memcpy(str_ptr, contents + i, key_length);
    p->keys[p->n_pairs][key_length] = 0;
    p->string_space += key_length + 1;
    str_ptr += key_length + 1;

    if(p->string_space + value_length + 1 > string_space)
    {
     fprintf(stderr, "%s: parameters_from_file(): string space problem\n",
             ctx->error_prefix);
     free(contents);
     return 1;
    }
    p->values[p->n_pairs] = str_ptr;
    memcpy(p->values[p->n_pairs], contents + mark + 1, value_length);
    p->values[p->n_pairs][value_length] = 0;
    p->string_space += value_length + 1;
    str_ptr += value_length + 1;

    p->n_pairs++;
   }

   line_number++;
   i += line_length + 1;
  }

  if(pass == 1)
  {
   if((p = allocate_parameter_structure(ctx, n_pairs, string_space, &str_ptr)) == NULL)
   {
    free(contents);
    return 1;
   }
  }

  /* now fill in values */
  pass++;
 }

 ctx->parameters = p;
 free(contents);
 return 0;
}

int send_parameters(struct buildmatrix_context *ctx)
{
 struct parameters *p;
 int n_bytes, buffer_length, pair_i;
 char buffer[512];

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: send_parameters()\n", ctx->error_prefix);

 if((p = ctx->parameters) == NULL)
  return 0;

 if(ctx->verbose)
  fprintf(stderr, "%s: sending parameters\n", ctx->error_prefix);

 while(1)
 {
  buffer_length = 
  snprintf(buffer, 512, "parameters: %d %d\n", p->n_pairs, p->string_space);

  if(send_line(ctx, buffer_length, buffer))
   return 1;

  if(receive_line(ctx, 32, &n_bytes, buffer))
   return 1;

  if(strncmp(buffer, "again", 5) == 0)
  {
   if(ctx->verbose)
    fprintf(stderr, "%s: peer requesting retransmission of SOT\n",
           ctx->error_prefix);
   continue;
  }

  if(strncmp(buffer, "failed", 6) == 0)
  {
   fprintf(stderr, "%s: peer rejected parameters\n", ctx->error_prefix);
   return 1;
  }

  if(strncmp(buffer, "ok", 2) == 0)
  {
   break;
  }

  fprintf(stderr, "%s: unexpected response to parameters, \"%s\"\n",
          ctx->error_prefix, buffer);
  return 1;
 }

 for(pair_i = 0; pair_i < p->n_pairs; pair_i++)
 {
  buffer_length = 
  snprintf(buffer, 512, "%s|%s\n", p->keys[pair_i], p->values[pair_i]);

  if(send_line(ctx, buffer_length, buffer))
   return 1;

  if(receive_line(ctx, 32, &n_bytes, buffer))
   return 1;

  buffer[n_bytes] = 0;
  if(strncmp(buffer, "ok", 2) != 0)
  {
   fprintf(stderr, "%s: response durring parameters transmission was not ok, \"%s\"\n",
           ctx->error_prefix, buffer);
   return 1;
  }
 }

 buffer_length = snprintf(buffer, 512, "done\n");
 if(send_line(ctx, buffer_length, buffer))
  return 1;

 if(receive_line(ctx, 32, &n_bytes, buffer))
  return 1;

 if(strncmp(buffer, "ok", 2) == 0)
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: parameters sent.\n", ctx->error_prefix);

  return 0;
 }

 fprintf(stderr, "%s: unexpected response to parameters, \"%s\"\n",
         ctx->error_prefix, buffer);
 return 1;
}

int receive_parameters(struct buildmatrix_context *ctx)
{
 int n_pairs, string_space, buffer_length, 
     mark, key_length, value_length, x;
 char buffer[512], *str_ptr;
 struct parameters *p;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: receive_parameters()\n", ctx->error_prefix);
 else if(ctx->verbose)
  fprintf(stderr, "%s: receiving parameters\n", ctx->error_prefix);

 if(receive_line(ctx, 512, &buffer_length, buffer))
  return 1;

 if(strncmp(buffer, "parameters: ", 12) != 0)
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: expected parameters SOT, not \"%s\"\n", ctx->error_prefix, buffer);

  buffer_length = snprintf(buffer, 512, "failed\n");
  send_line(ctx, buffer_length, buffer);
  return 1;
 }

 sscanf(buffer + 12, "%d %d\n", &n_pairs, &string_space);

 if((p = allocate_parameter_structure(ctx, n_pairs, string_space, &str_ptr)) == NULL)
 {
  buffer_length = snprintf(buffer, 512, "failed\n");
  send_line(ctx, buffer_length, buffer);
  return 1;
 }

 buffer_length = snprintf(buffer, 512, "ok\n");

 if(send_line(ctx, buffer_length, buffer))
 {
  free(p);
  return 1;
 }

 while(1)
 {
  if(receive_line(ctx, 512, &buffer_length, buffer))
  {
   free(p);
   return 1;
  }
  buffer[buffer_length] = 0;

  if(strncmp(buffer, "done", 4) == 0)
  {
   buffer_length = snprintf(buffer, 512, "ok\n");

   if(send_line(ctx, buffer_length, buffer))
   {
    free(p);
    return 1;
   }

   if(ctx->verbose)
    fprintf(stderr, "%s: parameters received\n", ctx->error_prefix);

   ctx->parameters = p;
   break;
  }

  x = 0;
  mark = -1;
  while(x < buffer_length)
  {
   if(buffer[x] == '|')
   {
    mark = x;
    break;
   }
   x++;
  }
  if(mark < 0)
  {
   fprintf(stderr, "%s: receive_parameters(): line no | character, \"%s\"\n",
           ctx->error_prefix, buffer);
   free(p);
   return 1;
  }

  key_length = mark;
  if( (key_length < 2) || (key_length > 256) )
  {
   fprintf(stderr, "%s: receive_parameters(): key is wrong length\n", ctx->error_prefix);
   free(p);
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  value_length = buffer_length  - (mark + 2);
  if( (value_length < 2) || (value_length > 256) )
  {
   fprintf(stderr, "%s: receive_parameters(): value is wrong length\n", ctx->error_prefix);
   free(p);
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  if(p->n_pairs + 1 > n_pairs)
  {
   fprintf(stderr, "%s: receive_parameters(): failure\n", ctx->error_prefix);
   free(p);
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  if(p->string_space + key_length + 1 > string_space)
  {
   fprintf(stderr, "%s: receive_parameters(): string space problem\n", ctx->error_prefix);
   free(p);
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  p->keys[p->n_pairs] = str_ptr;
  memcpy(p->keys[p->n_pairs], buffer, key_length);
  p->keys[p->n_pairs][key_length] = 0;
  p->string_space += key_length + 1;
  str_ptr += key_length + 1;

  if(p->string_space + value_length + 1 > string_space)
  {
   fprintf(stderr, "%s: receive_parameters(): string space problem\n", ctx->error_prefix);
   free(p);
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  p->values[p->n_pairs] = str_ptr;
  memcpy(p->values[p->n_pairs], buffer + mark + 1, value_length);
  p->values[p->n_pairs][value_length] = 0;
  p->string_space += value_length + 1;
  str_ptr += value_length + 1;

  p->n_pairs++;

  if(send_line(ctx, 3, "ok\n"))
   return 1;
 }

 return 0;
}

int load_parameters_from_db(struct buildmatrix_context *ctx)
{
 const char *sql, *key, *value;
 char *str_ptr;
 int code;
 struct sqlite3_stmt *statement = NULL;
 struct parameters *p;
 int n_pairs = 0, string_space = 0, pass = 1, key_length, value_length;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: load_parameters_from_db()\n", ctx->error_prefix);

 if(ctx->build_id < 1)
 {
  fprintf(stderr, "%s: load_parameters_from_db(): need build id\n", ctx->error_prefix);
  return 1;
 }

 if(open_database(ctx))
  return 1;
 
 pass = 1;
 while(pass < 3)
 {
  sql = "SELECT variable, value FROM parameters WHERE build_id = ?;";

  if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
  {
   fprintf(stderr, "%s: load_parameters_from_db(): sqlite3_prepare(%s) failed, '%s'\n",
           ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx)); 
   return 1;
  }

  if(sqlite3_bind_int(statement, 1, ctx->build_id) != SQLITE_OK)
  {
   fprintf(stderr, "%s: loaad_parameters_from_db(): sqlite3_bind_int() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }

  while((code = sqlite3_step(statement)) == SQLITE_ROW)
  {
   if(pass == 2)
   {
    if(p->n_pairs + 1 > n_pairs)
    {
     fprintf(stderr, "%s: load_parameters_from_db(): failure\n", ctx->error_prefix);
     free(p);
     sqlite3_finalize(statement);
     return 1;
    }
   }

   if(sqlite3_column_type(statement, 0) != SQLITE_TEXT)
   {
    fprintf(stderr, "%s: load_parameters_from_db(): key is not text\n",
            ctx->error_prefix);
    sqlite3_finalize(statement);
    return 1;
   }
   if(pass == 1)
   {
    string_space += sqlite3_column_bytes(statement, 0) + 1;
   } else {
    key = (const char *) sqlite3_column_text(statement, 0);
    key_length = sqlite3_column_bytes(statement, 0);

    if(p->string_space + key_length + 1 > string_space)
    {
     fprintf(stderr, "%s: load_parameters_from_db(): string space problem\n", ctx->error_prefix);
     free(p);
     sqlite3_finalize(statement);
     return 1;
    }

    p->keys[p->n_pairs] = str_ptr;
    memcpy(str_ptr, key, key_length);
    p->keys[p->n_pairs][key_length] = 0;
    p->string_space += key_length + 1;
    str_ptr += key_length + 1;
   }

   if(sqlite3_column_type(statement, 1) != SQLITE_TEXT)
   {
    fprintf(stderr, "%s: load_parameters_from_db(): value is not text\n",
            ctx->error_prefix);
    sqlite3_finalize(statement);
    return 1;
   }

   if(pass == 1)
   {
    string_space += sqlite3_column_bytes(statement, 1) + 1;
   } else {
    value = (const char *) sqlite3_column_text(statement, 1);
    value_length = sqlite3_column_bytes(statement, 1);

    if(p->string_space + value_length + 1 > string_space)
    {
     fprintf(stderr, "%s: load_parameters_from_db(): string space problem\n", ctx->error_prefix);
     free(p);
     sqlite3_finalize(statement);
     return 1;
    }

    p->values[p->n_pairs] = str_ptr;
    memcpy(str_ptr, value, value_length);
    p->values[p->n_pairs][value_length] = 0;
    p->string_space += value_length + 1;
    str_ptr += value_length + 1;
   }
 
   if(pass == 1)
    n_pairs++;
   else 
    p->n_pairs++;
  }

  if(code != SQLITE_DONE)
  {
   fprintf(stderr, "%s: load_parameters_from_db(): sqlite3_step() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }

  sqlite3_finalize(statement);

  if(pass == 1)
  {
   if((p = allocate_parameter_structure(ctx, n_pairs, string_space, &str_ptr)) == NULL)
   {
    sqlite3_finalize(statement);
    return 1;
   }
  }

  pass++; 
 }

 close_database(ctx);

 ctx->parameters = p;

 return 0;
}

int save_parameters_to_db(struct buildmatrix_context *ctx)
{
 const char *sql;
 int code, x;
 struct sqlite3_stmt *statement = NULL;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: save_parameters_to_db()\n", ctx->error_prefix);

 if(ctx->parameters == NULL)
 {
  fprintf(stderr, 
          "%s: save_parameters_to_db(): parameters == NULL\n", ctx->error_prefix);
  return 1;
 }

 if(ctx->build_id < 1)
 {
  fprintf(stderr, 
          "%s: save_parameters_to_db(): I need a build id\n", ctx->error_prefix);
  return 1;
 }

 if(open_database(ctx))
  return 1;

 for(x=0; x < ctx->parameters->n_pairs; x++)
 {
  sql = "INSERT INTO parameters (build_id, variable, value) VALUES (?, ?, ?);";

  if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
  {
   fprintf(stderr, 
           "%s: save_parameters_to_db(): sqlite3_prepare(%s) failed, '%s'\n",
           ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx));
   return 1;
  }

  if(sqlite3_bind_int(statement, 1, ctx->build_id) != SQLITE_OK)
  {
   fprintf(stderr, "%s: save_parameters_to_db(): sqlite3_bind_int() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }

  if(sqlite3_bind_text(statement, 2, ctx->parameters->keys[x], strlen(ctx->parameters->keys[x]),
                       SQLITE_TRANSIENT) != SQLITE_OK)
  {
   fprintf(stderr, "%s: save_parameters_to_db(): sqlite3_bind_text() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }

  if(sqlite3_bind_text(statement, 3, ctx->parameters->values[x], strlen(ctx->parameters->values[x]),
                       SQLITE_TRANSIENT) != SQLITE_OK)
  {
   fprintf(stderr, "%s: save_parameters_to_db(): sqlite3_bind_text() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }

  code = sqlite3_step(statement);
  if(code != SQLITE_DONE)
  {
   fprintf(stderr, "%s: save_parameters_to_db(): sqlite3_step() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }

 }

 sqlite3_finalize(statement);

 close_database(ctx);

 return 0;
}


