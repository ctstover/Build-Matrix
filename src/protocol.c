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

int send_line(const struct buildmatrix_context *ctx, int length, char *buffer)
{
 int bytes_to_send = length, n_bytes;

 if(ctx->connection_initialized == 0)
 {
  fprintf(stderr, "%s: connection to peer not initialized\n", ctx->error_prefix);
  return 1;
 }

 while(bytes_to_send > 0)
 {
  if((n_bytes = write(ctx->out, buffer + (length - bytes_to_send), bytes_to_send)) < 1)
  {
   perror(ctx->error_prefix);
   return 1;
  }

  bytes_to_send -= n_bytes;
 }

 return 0;
}

int receive_line(struct buildmatrix_context *ctx, int size, int *length, char *buffer)
{
 int bytes_read, n_bytes, i;
 static char line_buffer[4096];
 static int line_buffer_length = 0;

 if(ctx->connection_initialized == 0)
 {
  fprintf(stderr, "%s: connection to peer not initialized\n", ctx->error_prefix);
  return 1;
 }

 if(line_buffer_length > size)
 {
  fprintf(stderr, "%s: receive_line() more in line buffer than value of size. fix this\n", 
          ctx->error_prefix);
  return 1;
 }

 for(i=0; i<line_buffer_length; i++)
 {
  if(line_buffer[i] == '\n')
  {
   memcpy(buffer, line_buffer, i);
   buffer[i] = 0;
   memmove(line_buffer, line_buffer + i + 1, line_buffer_length - (i + 1));
   line_buffer_length -= (i + 1);
   *length = i + 1;
   return 0;
  }
 }

 bytes_read = i;

 while(1)
 {
  if(bytes_read == size)
  {
   buffer[bytes_read] = 0;
   fprintf(stderr, "%s: unexpected receive '%s'\n", ctx->error_prefix, buffer);
   break;
  }

  if((n_bytes = read(ctx->in, buffer + bytes_read, size - bytes_read)) < 1)
  {
   if(n_bytes < 0)
    perror(ctx->error_prefix);
   else
    fprintf(stderr, "%s: looks like peer is dead\n", ctx->error_prefix);
   return 1;
  }

  for(i=bytes_read; i < bytes_read + n_bytes; i++)
  {
   if(buffer[i] == '\n')
   {
    line_buffer_length = n_bytes - (i + 1 - bytes_read);
    memcpy(line_buffer, buffer + i + 1, line_buffer_length);
    *length = i + 1;
    buffer[i] = 0;
    return 0;
   }
  }

  bytes_read += n_bytes;
 }

 return 1;
}

int send_build_result(struct buildmatrix_context *ctx)
{
 int n_bytes, length;
 char buffer[4096];

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: send_build_result()\n", ctx->error_prefix);

 if(ctx->verbose)
  fprintf(stderr, "%s: sending build result\n", ctx->error_prefix);

 length = snprintf(buffer, 4096, "result: %d\n", ctx->build_result);

 if(send_line(ctx, length, buffer))
  return 1;

 if(receive_line(ctx, 4096, &n_bytes, buffer))
  return 1;

 buffer[n_bytes] = 0;
 if(strncmp(buffer, "failed", 6) == 0)
 {
  fprintf(stderr, "%s: peer rejected build result, \"%s\"\n", ctx->error_prefix, buffer);
  return 1;
 }

 if(strncmp(buffer, "ok", 2) == 0)
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: build result sent.\n", ctx->error_prefix);

  return 0;
 }

 fprintf(stderr, "%s: unexpected response to build result, \"%s\"\n",
         ctx->error_prefix, buffer);
 return 1;
}

int send_user_name(struct buildmatrix_context *ctx)
{
 int n_bytes, length;
 char buffer[4096];

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: send_user_name()\n", ctx->error_prefix);

 if(ctx->user == NULL)
  return 0;

 if(ctx->verbose)
  fprintf(stderr, "%s: sending user name\n", ctx->error_prefix);

 length = snprintf(buffer, 4096, "user name: %s\n", ctx->user);

 if(send_line(ctx, length, buffer))
  return 1;

 if(receive_line(ctx, 4096, &n_bytes, buffer))
  return 1;

 buffer[n_bytes] = 0;
 if(strncmp(buffer, "failed", 6) == 0)
 {
  fprintf(stderr, "%s: peer rejected user name, \"%s\"\n", ctx->error_prefix, buffer);
  return 1;
 }

 if(strncmp(buffer, "ok", 2) == 0)
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: user name sent.\n", ctx->error_prefix);

  return 0;
 }

 fprintf(stderr, "%s: unexpected response to user name, \"%s\"\n", 
         ctx->error_prefix, buffer);
 return 1;
}

int send_revision(struct buildmatrix_context *ctx)
{
 int n_bytes, length;
 char buffer[4096];

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: send_revision()\n", ctx->error_prefix);

 if(ctx->user == NULL)
  return 0;

 if(ctx->verbose)
  fprintf(stderr, "%s: sending revision\n", ctx->error_prefix);

 length = snprintf(buffer, 4096, "revision: %s\n", ctx->revision);

 if(send_line(ctx, length, buffer))
  return 1;

 if(receive_line(ctx, 4096, &n_bytes, buffer))
  return 1;

 buffer[n_bytes] = 0;
 if(strncmp(buffer, "failed", 6) == 0)
 {
  fprintf(stderr, "%s: peer rejected revision, \"%s\"\n", ctx->error_prefix, buffer);
  return 1;
 }

 if(strncmp(buffer, "ok", 2) == 0)
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: revision sent.\n", ctx->error_prefix);

  return 0;
 }

 fprintf(stderr, "%s: unexpected response to revision, \"%s\"\n", 
         ctx->error_prefix, buffer);
 return 1;
}

int send_job_name(struct buildmatrix_context *ctx)
{
 int n_bytes, length;
 char buffer[4096];

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: send_job_name()\n", ctx->error_prefix);

 if(ctx->job_name == NULL)
  return 0;

 if(ctx->verbose)
  fprintf(stderr, "%s: sending job name\n", ctx->error_prefix);

 length = snprintf(buffer, 4096, "job name: %s\n", ctx->job_name);

 if(send_line(ctx, length, buffer))
  return 1;

 if(receive_line(ctx, 4096, &n_bytes, buffer))
  return 1;

 buffer[n_bytes] = 0;
 if(strncmp(buffer, "failed", 6) == 0)
 {
  fprintf(stderr, "%s: peer rejected job name, \"%s\"\n", ctx->error_prefix, buffer);
  return 1;
 }

 if(strncmp(buffer, "ok", 2) == 0)
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: job name sent.\n", ctx->error_prefix);

  return 0;
 }

 fprintf(stderr, "%s: unexpected response to job name, \"%s\"\n", 
         ctx->error_prefix, buffer);
 return 1;
}

int send_branch_name(struct buildmatrix_context *ctx)
{
 int n_bytes, length;
 char buffer[4096];

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: send_branch_name()\n", ctx->error_prefix);

 if(ctx->branch_name == NULL)
  return 0;

 if(ctx->verbose)
  fprintf(stderr, "%s: sending branch name\n", ctx->error_prefix);

 length = snprintf(buffer, 4096, "branch name: %s\n", ctx->branch_name);

 if(send_line(ctx, length, buffer))
  return 1;

 if(receive_line(ctx, 4096, &n_bytes, buffer))
  return 1;

 buffer[n_bytes] = 0;
 if(strncmp(buffer, "failed", 6) == 0)
 {
  fprintf(stderr, "%s: peer rejected branch name, \"%s\"\n", ctx->error_prefix, buffer);
  return 1;
 }

 if(strncmp(buffer, "ok", 2) == 0)
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: branch name sent.\n", ctx->error_prefix);

  return 0;
 }

 fprintf(stderr, "%s: unexpected response to branch name, \"%s\"\n", 
         ctx->error_prefix, buffer);
 return 1;
}

int send_build_host_name(struct buildmatrix_context *ctx)
{
 int n_bytes, length;
 char buffer[4096];

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: send_build_host_name()\n", ctx->error_prefix);

 if(ctx->build_node == NULL)
  return 0;

 if(ctx->verbose)
  fprintf(stderr, "%s: sending build host name\n", ctx->error_prefix);

 length = snprintf(buffer, 4096, "build host: %s\n", ctx->build_node);

 if(send_line(ctx, length, buffer))
  return 1;

 if(receive_line(ctx, 4096, &n_bytes, buffer))
  return 1;

 buffer[n_bytes] = 0;
 if(strncmp(buffer, "failed", 6) == 0)
 {
  fprintf(stderr, "%s: peer rejected build host name, \"%s\"\n", ctx->error_prefix, buffer);
  return 1;
 }

 if(strncmp(buffer, "ok", 2) == 0)
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: build host name sent.\n", ctx->error_prefix);

  return 0;
 }

 fprintf(stderr, "%s: unexpected response to build host, \"%s\"\n", 
         ctx->error_prefix, buffer);
 return 1;
}

int send_unique_identifier(struct buildmatrix_context *ctx)
{
 int n_bytes, length;
 char buffer[256];

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: send_unique_identifier()\n", ctx->error_prefix);

 if(ctx->unique_identifier == NULL)
  return 0;

 if(ctx->verbose)
  fprintf(stderr, "%s: sending unique identifier\n", ctx->error_prefix);

 length = snprintf(buffer, 256, "unique identifier %s\n", ctx->unique_identifier);

 if(send_line(ctx, length, buffer))
  return 1;

 if(receive_line(ctx, 256, &n_bytes, buffer))
  return 1;

 buffer[n_bytes] = 0;
 if(strncmp(buffer, "failed", 6) == 0)
 {
  fprintf(stderr, "%s: peer rejected unique identifier, %s\n", ctx->error_prefix, buffer);
  return 1;
 }

 if(strncmp(buffer, "ok", 2) == 0)
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: unique identifier sent.\n", ctx->error_prefix);

  return 0;
 }

 fprintf(stderr, "%s: unexpected response to unique identifier, \"%s\"\n", 
         ctx->error_prefix, buffer);
 return 1;
}

int start_build_submission(struct buildmatrix_context *ctx)
{
 int n_bytes, allocation_size;
 char buffer[256], *unique_identifier = NULL;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: start_build_submission()\n", ctx->error_prefix);

 if(send_line(ctx, 13, "start submit\n"))
  return 1;

 if(receive_line(ctx, 256, &n_bytes, buffer))
  return 1;

 buffer[n_bytes] = 0;
 if(strncmp(buffer, "failed", 6) == 0)
 {
  fprintf(stderr, "%s: peer rejected start submit, %s\n", ctx->error_prefix, buffer);
  return 1;
 }

 if( (n_bytes > 3) && (strncmp(buffer, "ok", 2) == 0) )
 {
  allocation_size = n_bytes - 3;

  if(ctx->unique_identifier != NULL)
  {
   /* in this case we are the server, and the client is telling us the identifer for the build,
      which is suppose to be the identifer we (the server) have already sent over as in the 
      case of a pull or get operation */

   if((unique_identifier = malloc(allocation_size)) == NULL)
   {
    fprintf(stderr, "%s: malloc(%d) failed: %s\n", 
           ctx->error_prefix, allocation_size, strerror(errno));
    ctx->connection_initialized = 0;
    return 1;
   }
   memcpy(unique_identifier, buffer + 3, allocation_size - 1);
   unique_identifier[allocation_size - 1] = 0;

   if(check_string(ctx, unique_identifier))
   {
    fprintf(stderr, "%s: unique identifier fails check_string()\n", ctx->error_prefix);
    ctx->connection_initialized = 0;
    return 1;
   }

   if(strcmp(unique_identifier, ctx->unique_identifier) != 0)
   {
    fprintf(stderr, "%s: start_build_submission(): %s != %s\n",
            ctx->error_prefix, unique_identifier, ctx->unique_identifier);
    free(unique_identifier);
    return 1;
   }
   free(unique_identifier);

  } else {
   /* in this case we are the are client and the server is telling us what identifier it 
      has generated for the build */

   if((ctx->unique_identifier = malloc(allocation_size)) == NULL)
   {
    fprintf(stderr, "%s: malloc(%d) failed: %s\n", 
           ctx->error_prefix, allocation_size, strerror(errno));
    ctx->connection_initialized = 0;
    return 1;
   }
   memcpy(ctx->unique_identifier, buffer + 3, allocation_size - 1);
   ctx->unique_identifier[allocation_size] = 0;

   if(check_string(ctx, ctx->unique_identifier))
   {
    fprintf(stderr, "%s: unique identifier fails check_string()\n", ctx->error_prefix);
    ctx->connection_initialized = 0;
    return 1;
   }

  }

  if(ctx->verbose)
   fprintf(stderr, "%s: submit started for build %s.\n",
           ctx->error_prefix, ctx->unique_identifier);

  return 0;
 }

 fprintf(stderr, "%s: unexpected response to start submit, \"%s\"\n", 
         ctx->error_prefix, buffer);
 return 1;
}

int send_build_time(struct buildmatrix_context *ctx)
{
 int n_bytes, length;
 char buffer[256];
 long long int p;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: send_build_time()\n", ctx->error_prefix);

 if(ctx->build_time == 0)
 {
  fprintf(stderr, "%s: send_build_time(): I don't have a build time\n", ctx->error_prefix);
  return 1;
 }

 if(ctx->verbose)
  fprintf(stderr, "%s: sending build time\n", ctx->error_prefix);

 p = (long long int) ctx->build_time;

 length = snprintf(buffer, 256, "build time %lld\n", p);

 if(send_line(ctx, length, buffer))
  return 1;

 if(receive_line(ctx, 256, &n_bytes, buffer))
  return 1;

 buffer[n_bytes] = 0;
 if(strncmp(buffer, "failed", 6) == 0)
 {
  fprintf(stderr, "%s: peer rejected build time, %s\n", ctx->error_prefix, buffer);
  return 1;
 }

 if(strncmp(buffer, "ok", 2) == 0)
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: build time sent.\n", ctx->error_prefix);

  return 0;
 }

 fprintf(stderr, "%s: unexpected response to build time, \"%s\"\n", 
         ctx->error_prefix, buffer);
 return 1;
}

int send_checksum(struct buildmatrix_context *ctx, int from_db)
{
 int n_bytes;
 char buffer[32];

 if(ctx->checksum == NULL)
  return 0;

 if(ctx->verbose)
  fprintf(stderr, "%s: sending checksum\n", ctx->error_prefix);

 if(send_line(ctx, 9, "checksum\n"))
  return 1;

 if(receive_line(ctx, 32, &n_bytes, buffer))
  return 1;

 buffer[n_bytes] = 0;
 if(strncmp(buffer, "failed", 6) == 0)
 {
  fprintf(stderr, "%s: peer rejected checksum\n", ctx->error_prefix);
  return 1;
 }

 if(strncmp(buffer, "ok", 2) == 0)
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: checksum going to send_file().\n", ctx->error_prefix);


  if(from_db)
  {
   if(send_db_file(ctx, ctx->checksum))
   {
    fprintf(stderr, "%s: peer rejected checksum.\n", ctx->error_prefix);
    return 1;
   }
  } else {
   if(send_disk_file(ctx, ctx->checksum, NULL))
   {
    fprintf(stderr, "%s: peer rejected checksum.\n", ctx->error_prefix);
    return 1;
   }
  }

  if(ctx->verbose)
   fprintf(stderr, "%s: checksum sent.\n", ctx->error_prefix);

  return 0;
 }

 fprintf(stderr, "%s: unexpected response to checksum output, \"%s\"\n",
         ctx->error_prefix, buffer);
 return 1;
}

int send_build_output(struct buildmatrix_context *ctx, int from_db)
{
 int n_bytes;
 char buffer[32];

 if(ctx->build_output == NULL)
  return 0;

 if(ctx->verbose)
  fprintf(stderr, "%s: sending build output\n", ctx->error_prefix);

 if(send_line(ctx, 13, "build output\n"))
  return 1;

 if(receive_line(ctx, 32, &n_bytes, buffer))
  return 1;

 buffer[n_bytes] = 0;
 if(strncmp(buffer, "failed", 6) == 0)
 {
  fprintf(stderr, "%s: peer rejected build output\n", ctx->error_prefix);
  return 1;
 }

 if(strncmp(buffer, "ok", 2) == 0)
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: build output going to send_file().\n", ctx->error_prefix);

  if(from_db)
  {
   if(send_db_file(ctx, ctx->build_output))
   {
    fprintf(stderr, "%s: peer rejected build output.\n", ctx->error_prefix);
    return 1;
   }
  } else {
   if(send_disk_file(ctx, ctx->build_output, NULL))
   {
    fprintf(stderr, "%s: peer rejected build output.\n", ctx->error_prefix);
    return 1;
   }
  }

  if(ctx->verbose)
   fprintf(stderr, "%s: build output sent.\n", ctx->error_prefix);

  return 0;
 }

 fprintf(stderr, "%s: unexpected response to build output, \"%s\"\n",
         buffer, ctx->error_prefix);
 return 1;
}

int send_build_report(struct buildmatrix_context *ctx, int from_db)
{
 int n_bytes;
 char buffer[32];

 if(ctx->build_report == NULL)
  return 0;

 if(ctx->verbose)
  fprintf(stderr, "%s: sending build report\n", ctx->error_prefix);

 if(send_line(ctx, 13, "build report\n"))
  return 1;

 if(receive_line(ctx, 32, &n_bytes, buffer))
  return 1;

 buffer[n_bytes] = 0;
 if(strncmp(buffer, "failed", 6) == 0)
 {
  fprintf(stderr, "%s: peer rejected build report\n", ctx->error_prefix);
  return 1;
 }

 if(strncmp(buffer, "ok", 2) == 0)
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: build report going to send_file().\n", ctx->error_prefix);

  if(from_db)
  {
   if(send_db_file(ctx, ctx->build_report))
   {
    fprintf(stderr, "%s: peer rejected build report.\n", ctx->error_prefix);
    return 1;
   }
  } else {
   if(send_disk_file(ctx, ctx->build_report, NULL))
   {
    fprintf(stderr, "%s: peer rejected build report.\n", ctx->error_prefix);
    return 1;
   }
  }

  if(ctx->verbose)
   fprintf(stderr, "%s: build report sent.\n", ctx->error_prefix);

  return 0;
 }

 fprintf(stderr, "%s: unexpected response to build report, \"%s\"\n",
         ctx->error_prefix, buffer);
 return 1;
}

int send_test_totals(struct buildmatrix_context *ctx)
{
 int n_bytes, buffer_length;
 char buffer[512];

 if(ctx->verbose)
  fprintf(stderr, "%s: sending test totals\n", ctx->error_prefix);

 buffer_length = 
 snprintf(buffer, 512, "test totals %d %d %d %d\n",
          ctx->test_totals[0], ctx->test_totals[1], ctx->test_totals[2], ctx->test_totals[3]);

 if(send_line(ctx, buffer_length, buffer))
  return 1;

 if(receive_line(ctx, 32, &n_bytes, buffer))
  return 1;

 buffer[n_bytes] = 0;
 if(strncmp(buffer, "failed", 6) == 0)
 {
  fprintf(stderr, "%s: peer rejected test totals\n", ctx->error_prefix);
  return 1;
 }

 if(strncmp(buffer, "ok", 2) == 0)
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: test totals sent.\n", ctx->error_prefix);
  return 0;
 }

 fprintf(stderr, "%s: unexpected response to test totals, \"%s\"\n", buffer, ctx->error_prefix);
 return 1;
}

int service_input(struct buildmatrix_context *ctx)
{
 int n_bytes, allocation_size, save_in_database, code;
 char buffer[4096];
 struct test_results *test_results;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: service_input()\n", ctx->error_prefix);

 if(ctx->write_master == 1)
  save_in_database = 1;

 if(ctx->mode == BLDMTRX_MODE_PULL)
  save_in_database = 1;

 if(receive_line(ctx, 4096, &n_bytes, buffer))
  return 1;

 if(ctx->verbose > 2)
  fprintf(stderr, "%s: line from peer (%d) \"%s\"\n",
          ctx->error_prefix, n_bytes, buffer);

 /* done */
 if( (n_bytes == 5) &&
     (strncmp(buffer, "done", 4) == 0) )
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: peer finished with opertion\n", ctx->error_prefix);

  return 2;
 }

 /* initialize */
 if( (n_bytes > 23) &&
     (strncmp(buffer, "build matrix initialize", 23) == 0) )
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: peer requesting connection initialization\n", ctx->error_prefix);

  n_bytes = snprintf(buffer, 64, "connection initialized|%s\n", ctx->project_name);

  if(send_line(ctx, n_bytes, buffer))
   return 1;

  ctx->connection_initialized = 1;
  return 0;
 }

 /* shutdown */
 if( (n_bytes == 9) &&
     (strncmp(buffer, "shutdown", 8) == 0) )
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: peer requesting shutdown\n", ctx->error_prefix);

  ctx->connection_initialized = 0;
  return 0;
 }

 /* start time */
 if( (n_bytes > 11) &&
     (strncmp(buffer, "build time ", 11) == 0) )
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: peer sending build time\n", ctx->error_prefix);

  sscanf(buffer + 11, "%lld", &(ctx->build_time));

  if(send_line(ctx, 3, "ok\n"))
   return 1;

  if(ctx->verbose)
   fprintf(stderr, "%s: build time received, %lld.\n", ctx->error_prefix, ctx->build_time);

  return 0;
 }
 
 /* pull request */
 if( (n_bytes > 5) &&
     (strncmp(buffer, "pull ", 5) == 0) )
 {
  sscanf(buffer + 5, "%d", &(ctx->pull_build_id));

  if(ctx->mode == BLDMTRX_MODE_PULL)
  {
   if(ctx->verbose)
    fprintf(stderr, "%s: peer updating our pull build id for next time (%d)\n",
            ctx->error_prefix, ctx->pull_build_id);

   if(send_line(ctx, 3, "ok\n"))
    return 1;

   return 0;
  }  

  if(ctx->verbose)
   fprintf(stderr, "%s: peer requesting pull\n", ctx->error_prefix);

  if(send_line(ctx, 3, "ok\n"))
   return 1;

  if(ctx->verbose)
   fprintf(stderr, "%s: pulling from build_id %d\n", ctx->error_prefix, ctx->pull_build_id);

  if(open_database(ctx))
    return 1;

  if(clean_state(ctx))
   return 1;

  while((code = service_pull(ctx)) == 0)
  {
   if(send_unique_identifier(ctx))
    return 1;

   if(submit(ctx, 1))
    return 1;

   ctx->pull_build_id = ctx->build_id;

   if(clean_state(ctx))
    return 1;
  }

  if(code == 1)
   return 1;

  close_database(ctx);

  if(--(ctx->pull_build_id) > 0)
  {
   n_bytes = snprintf(buffer, 32, "pull %d\n", ctx->pull_build_id);

   if(send_line(ctx, n_bytes, buffer))
    return 1;

   if(receive_line(ctx, 4096, &n_bytes, buffer))
    return 1;

   if( (strncmp(buffer, "ok", 2) != 0) ||
       (n_bytes != 3) )
   {
    fprintf(stderr, "%s: unexepected response to update pull from id: %s\n", 
            ctx->error_prefix, buffer);
    return 1;
   }
  }

  if(send_line(ctx, 5, "done\n"))
   return 1;

  return 0;
 }

 /* test totals */
 if( (n_bytes > 12) &&
     (strncmp(buffer, "test totals ", 12) == 0) )
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: peer sending test totals\n", ctx->error_prefix);

  if( (ctx->unique_identifier == NULL) || (ctx->build_id < 0) )
  {
   fprintf(stderr, "%s: I should have a build number and a build id.\n", ctx->error_prefix);
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  sscanf(buffer + 12, "%d %d %d %d",
         &(ctx->test_totals[0]), &(ctx->test_totals[1]), 
         &(ctx->test_totals[2]), &(ctx->test_totals[3]));

  if( (ctx->test_totals[0] < 0) || 
      (ctx->test_totals[1] < 0) ||
      (ctx->test_totals[2] < 0) ||
      (ctx->test_totals[3] < 0) ||
      (ctx->test_totals[0] != (ctx->test_totals[1] + ctx->test_totals[2] + ctx->test_totals[3])) )
  {
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  if(send_line(ctx, 3, "ok\n"))
   return 1;

  if(ctx->verbose)
   fprintf(stderr, "%s: test totals received.\n", ctx->error_prefix);

  return 0;
 }

 /* unique identifier */
 if( (n_bytes > 18) &&
     (strncmp(buffer, "unique identifier ", 18) == 0) )
 {
  if(save_in_database == 0)
  {
   if(ctx->verbose)
    fprintf(stderr, "%s: peer should not be sending unique identifier this way\n",
            ctx->error_prefix);
   send_line(ctx, 7, "failed\n");
   ctx->connection_initialized = 0;
   return 1;
  }

  allocation_size = n_bytes - 17;
  if((ctx->unique_identifier = malloc(allocation_size)) == NULL)
  {
   fprintf(stderr, "%s: malloc(%d) failed: %s\n", 
          ctx->error_prefix, allocation_size, strerror(errno));
   ctx->connection_initialized = 0;
   return 1;
  }
  memcpy(ctx->unique_identifier, buffer + 18, allocation_size - 1);
  ctx->unique_identifier[allocation_size] = 0;

  if(check_string(ctx, ctx->unique_identifier))
  {
   fprintf(stderr, "%s: unique identifier fails check_string()\n", ctx->error_prefix);
   ctx->connection_initialized = 0;
   return 1;
  }

  if(ctx->verbose)
   fprintf(stderr, "%s: peer sent unique identifier %s\n",
           ctx->error_prefix, ctx->unique_identifier);

  if(ctx->mode != BLDMTRX_MODE_PULL)
  {
   if(lookup_build_id(ctx))
   {
    send_line(ctx, 7, "failed\n");
    return 1;
   }
  }

  if(send_line(ctx, 3, "ok\n"))
   return 1;

/*
  if(ctx->verbose)
  {
   if(ctx->mode == BLDMTRX_MODE_PULL)
    fprintf(stderr, "%s: build number %d accepted as part of a pull operation.\n", 
            ctx->error_prefix, ctx->build_number);
   else
    fprintf(stderr, "%s: build number %d checks out.\n", 
            ctx->error_prefix, ctx->build_number);
  }
*/

  return 0;;
 }

 /* build report */
 if( (n_bytes == 13) &&
     (strncmp(buffer, "build report", 12) == 0) )
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: peer sending build report\n", ctx->error_prefix);

  if(save_in_database)
  {
   if(open_database(ctx))
   {
    send_line(ctx, 7, "failed\n");
    return 1;
   }

   if(receive_disk_file(ctx, &(ctx->build_report), 1))
   {
    fprintf(stderr, "%s: build report transfer failed.\n", ctx->error_prefix);
    return 1;
   }

  } else {
   if(send_line(ctx, 3, "ok\n"))
    return 1;

   if(receive_disk_file(ctx, &(ctx->report_out_filename), 0))
    return 1;

   ctx->report_written = 1;
  }
 
  if(ctx->verbose)
   fprintf(stderr, "%s: build report received.\n", ctx->error_prefix);

  return 0;
 }

 /* build output */
 if( (n_bytes == 13) &&
     (strncmp(buffer, "build output", 12) == 0) )
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: peer sending build output\n",
           ctx->error_prefix);

  if(save_in_database)
  {
   if(open_database(ctx))
   {
    send_line(ctx, 7, "failed\n");
    return 1;
   }

   if(send_line(ctx, 3, "ok\n"))
    return 1;

   if(receive_disk_file(ctx, &(ctx->build_output), 1))
   {
    fprintf(stderr, "%s: build output transfer failed.\n", ctx->error_prefix);
    return 1;
   }

  } else {
   if(send_line(ctx, 3, "ok\n"))
    return 1;

   if(receive_disk_file(ctx, &(ctx->output_out_filename), 0))
    return 1;

   ctx->output_written = 1;
  }

  if(ctx->verbose)
   fprintf(stderr, "%s: build output received.\n", ctx->error_prefix);

  return 0;
 }

 /* checksum */
 if( (n_bytes == 9) &&
     (strncmp(buffer, "checksum", 8) == 0) )
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: peer sending checksum\n", ctx->error_prefix);

  if(save_in_database)
  {
   if(open_database(ctx))
   { 
    send_line(ctx, 7, "failed\n");
    return 1;
   }

   if(send_line(ctx, 3, "ok\n"))
    return 1;
   if(receive_disk_file(ctx, &(ctx->checksum), 1))
   {
    fprintf(stderr, "%s: checksum transfer failed.\n", ctx->error_prefix);
    return 1;
   }

  } else {
   if(send_line(ctx, 3, "ok\n"))
    return 1;

   if(receive_disk_file(ctx, &(ctx->checksum_out_filename), 0))
    return 1;

   ctx->checksum_written = 1;
  }

  if(ctx->verbose)
   fprintf(stderr, "%s: checksum received.\n", ctx->error_prefix);

  return 0;
 }

 /* user name */
 if( (n_bytes > 11) &&
     (strncmp(buffer, "user name: ", 11) == 0) )
 {
  if(ctx->user != NULL)
   free(ctx->user);

  allocation_size = n_bytes - 10;
  if((ctx->user = (char *) malloc(allocation_size)) == NULL)
  {
   fprintf(stderr, "%s: malloc(%d) failed\n", ctx->error_prefix, allocation_size);
   return 1;
  }
  memcpy(ctx->user, buffer + 11, allocation_size - 1);
  ctx->user[allocation_size] = 0;

  if(ctx->verbose)
   fprintf(stderr, "%s: peer sending user name, \"%s\"\n", ctx->error_prefix, ctx->user);

  if(check_string(ctx, ctx->user))
  {
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  if(resolve_user_id(ctx))
  {
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  if(send_line(ctx, 3, "ok\n"))
   return 1;

  if(ctx->verbose)
   fprintf(stderr, "%s: user name received, '%s'\n",
           ctx->error_prefix, ctx->user);

  return 0;
 }

 /* job name */
 if( (n_bytes > 10) &&
     (strncmp(buffer, "job name: ", 10) == 0) )
 {
  if(ctx->job_name != NULL)
   free(ctx->job_name);

  allocation_size = n_bytes - 9;
  if((ctx->job_name = (char *) malloc(allocation_size)) == NULL)
  {
   fprintf(stderr, "%s: malloc(%d) failed\n", ctx->error_prefix, allocation_size);
   return 1;
  }
  memcpy(ctx->job_name, buffer + 10, allocation_size - 1);
  ctx->job_name[allocation_size] = 0;

  if(ctx->verbose)
   fprintf(stderr, "%s: peer sending job name, \"%s\"\n", ctx->error_prefix, ctx->job_name);

  if(check_string(ctx, ctx->job_name))
  {
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  if(send_line(ctx, 3, "ok\n"))
   return 1;

  if(ctx->verbose)
   fprintf(stderr, "%s: job name received\n",
           ctx->error_prefix);

  return 0;
 }

 /* branch name */
 if( (n_bytes > 14) &&
     (strncmp(buffer, "branch name: ", 13) == 0) )
 {
  if(ctx->branch_name != NULL)
   free(ctx->branch_name);

  allocation_size = n_bytes - 13;

  if((ctx->branch_name = (char *) malloc(allocation_size)) == NULL)
  {
   fprintf(stderr, "%s: malloc(%d) failed\n",
           ctx->error_prefix, allocation_size);
   return 1;
  }
  memcpy(ctx->branch_name, buffer + 13, allocation_size - 1);
  ctx->branch_name[allocation_size - 1] = 0;

  if(ctx->verbose)
   fprintf(stderr, "%s: peer sending branch name, \"%s\"\n",
           ctx->error_prefix, ctx->branch_name);

  if(check_string(ctx, ctx->branch_name))
  {
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  if(send_line(ctx, 3, "ok\n"))
   return 1;

  if(ctx->verbose)
   fprintf(stderr, "%s: branch name received.\n", ctx->error_prefix);

  return 0;
 }

 /* revision */
 if( (n_bytes > 10) &&
     (strncmp(buffer, "revision: ", 10) == 0) )
 {
  if(ctx->revision != NULL)
   free(ctx->revision);

  allocation_size = n_bytes - 9;

  if((ctx->revision = (char *) malloc(allocation_size)) == NULL)
  {
   fprintf(stderr, "%s: malloc(%d) failed\n",
           ctx->error_prefix, allocation_size);
   return 1;
  }
  memcpy(ctx->revision, buffer + 10, allocation_size - 1);
  ctx->revision[allocation_size - 1] = 0;

  if(ctx->verbose)
   fprintf(stderr, "%s: peer sending revision, \"%s\"\n",
           ctx->error_prefix, ctx->revision);

  if(check_string(ctx, ctx->revision))
  {
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  if(send_line(ctx, 3, "ok\n"))
   return 1;

  if(ctx->verbose)
   fprintf(stderr, "%s: revision received.\n", ctx->error_prefix);

  return 0;
 }

 /* build host */
 if( (n_bytes > 13) &&
     (strncmp(buffer, "build host: ", 12) == 0) )
 {
  if(ctx->build_node != NULL)
   free(ctx->build_node);

  allocation_size = n_bytes - 12;
  if((ctx->build_node = (char *) malloc(allocation_size)) == NULL)
  {
   fprintf(stderr, "%s: malloc(%d) failed\n",
           ctx->error_prefix, allocation_size);
   return 1;
  }
  memcpy(ctx->build_node, buffer + 12, allocation_size - 1);
  ctx->build_node[allocation_size - 1] = 0;

  if(ctx->verbose)
   fprintf(stderr, "%s: peer sending build host, \"%s\"\n", 
           ctx->error_prefix, ctx->build_node);

  if(check_string(ctx, ctx->branch_name))
  {
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  if(send_line(ctx, 3, "ok\n"))
   return 1;

  if(ctx->verbose)
   fprintf(stderr, "%s: build host received.\n", ctx->error_prefix);
 
  return 0;
 }

 /* build result */
 if( (n_bytes > 8) &&
     (strncmp(buffer, "result: ", 8) == 0) )
 {
  ctx->build_result = -1;
  sscanf(buffer + 8, "%d", &(ctx->build_result));

  if(ctx->verbose)
   fprintf(stderr, "%s: peer sending build result, %d\n", 
           ctx->error_prefix, ctx->build_result);

  if(send_line(ctx, 3, "ok\n"))
   return 1;

  if(ctx->verbose)
   fprintf(stderr, "%s: build result received.\n", ctx->error_prefix);

  return 0;
 }

 /* proceed to receive_test_results() */
 if( (n_bytes > 14) &&
     (strncmp(buffer, "test results: ", 14) == 0) )
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: sending \"again\" and going to receive_test_results()\n",
           ctx->error_prefix);

  if(send_line(ctx, 6, "again\n"))
   return 1;

  if(receive_test_results(ctx, &(ctx->test_results)))
   return 1;

  if(save_in_database)
  {
   if(file_test_results(ctx, ctx->test_results))
    return 1;
  }

  return 0;
 }

 /* proceed to receive_parameters() */
 if( (n_bytes > 12) &&
     (strncmp(buffer, "parameters: ", 12) == 0) )
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: sending \"again\" and going to receive_parameters()\n",
           ctx->error_prefix);

  if(send_line(ctx, 6, "again\n"))
   return 1;

  if(receive_parameters(ctx))
   return 1;

  if(save_in_database)
  {
   if(save_parameters_to_db(ctx))
    return 1;
  }

  return 0;
 }

 /* submit build */
 if( (n_bytes == 7) &&
     (strncmp(buffer, "submit", 6) == 0) )
 {
  if(authorize_submit(ctx))
  {
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  if(save_in_database == 0)
  {
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  if(ctx->process_build_strategy(ctx))
  {
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  if(send_line(ctx, 3, "ok\n"))
   return 1;

  ctx->build_id = 0;
  return 0;
 }

 if( (n_bytes == 13) &&
     (strncmp(buffer, "start submit", 12) == 0) )
 {
  if(save_in_database == 0)
  {
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  if(ready_build_submission(ctx))
  {
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  n_bytes = snprintf(buffer, 1024, "ok %s\n", ctx->unique_identifier);

  if(send_line(ctx, n_bytes, buffer))
   return 1;

  return 0;
 }

 /* reset the dialog state */
 if(strncmp(buffer, "failed", 6) == 0)
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: generic fail catch\n", ctx->error_prefix);

  if(clean_state(ctx))
   return 1;

  return 0;
 }

 /* list jobs */ 
 if( (n_bytes == 10) &&
     (strncmp(buffer, "list jobs", 9) == 0) )
 {
  if(open_database(ctx))
  {
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  if(authorize_list(ctx))
  {
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  if(send_line(ctx, 3, "ok\n"))
   return 1;

  if(service_list_jobs(ctx))
  {
   send_line(ctx, 7, "failed\n");
   close_database(ctx);
   return 1;
  }

  return 0;
 }

 /* list branches */ 
 if( (n_bytes == 14) &&
     (strncmp(buffer, "list branches", 13) == 0) )
 {
  if(open_database(ctx))
  {
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  if(authorize_list(ctx))
  {
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  if(send_line(ctx, 3, "ok\n"))
   return 1;

  if(service_list_branches(ctx))
  {
   send_line(ctx, 7, "failed\n");
   close_database(ctx);
   return 1;
  }

  return 0;
 }

 /* list hosts */ 
 if( (n_bytes == 11) &&
     (strncmp(buffer, "list hosts", 10) == 0) )
 {
  if(open_database(ctx))
  {
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  if(authorize_list(ctx))
  {
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  if(send_line(ctx, 3, "ok\n"))
   return 1;

  if(service_list_hosts(ctx))
  {
   send_line(ctx, 7, "failed\n");
   close_database(ctx);
   return 1;
  }

  return 0;
 }

 /* list buids */
 if( (n_bytes == 12) &&
     (strncmp(buffer, "list builds", 11) == 0) )
 {
  if(open_database(ctx))
  {
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  if(authorize_list(ctx))
  {
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  if(send_line(ctx, 3, "ok\n"))
   return 1;

  if(service_list_builds(ctx))
  {
   send_line(ctx, 7, "failed\n");
   close_database(ctx);
   return 1;
  }

  if(close_database(ctx))
  {
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  return 0;
 }

 /* list tests */
 if( (n_bytes == 11) &&
     (strncmp(buffer, "list tests", 10) == 0) )
 {
  if(authorize_list(ctx))
  {
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  if(service_list_tests(ctx))
  {
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  if(send_line(ctx, 3, "ok\n"))
   return 1;

  return 0;
 }

 /* build details */
 if( (n_bytes == 10) &&
     (strncmp(buffer, "get build", 9) == 0) )
 {
  if(open_database(ctx))
  {
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  if(authorize_get(ctx))
  {
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  if(service_get_build(ctx))
  {
   send_line(ctx, 7, "failed\n");
   return 1;
  } else {
   if(send_line(ctx, 3, "ok\n"))
    return 1;
  }

  if(send_job_name(ctx))
   return 1;
  free(ctx->job_name);
  ctx->job_name = NULL;

  if(send_branch_name(ctx))
   return 1;
  free(ctx->branch_name);
  ctx->branch_name = NULL;

  if(send_revision(ctx))
   return 1;
  free(ctx->revision);
  ctx->revision = NULL;

  if(send_user_name(ctx))
   return 1;
  free(ctx->user);
  ctx->user = NULL;

  if(send_build_host_name(ctx))
   return 1;
  free(ctx->build_node);
  ctx->build_node = NULL;

  if(send_build_result(ctx))
   return 1;

  if(send_build_report(ctx, 1))
   return 1;

  if(send_build_output(ctx, 1))
   return 1;

  if(send_checksum(ctx, 1))
   return 1;

  if(pull_test_results(ctx, &test_results))
   return 1;

  if(send_test_results(ctx, test_results))
   return 1;

  free(test_results);

  if(load_parameters_from_db(ctx))
   return 1;

  if(send_parameters(ctx))
   return 1;

  if(close_database(ctx))
  {
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  if(send_line(ctx, 5, "done\n"))
   return 1;

  return 0;
 }

 /* scratch build */
 if( (n_bytes == 8) &&
      (strncmp(buffer, "scratch", 7) == 0) )
 {
  if(authorize_scratch(ctx))
  {
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  if(service_scratch(ctx))
  {
   send_line(ctx, 7, "failed\n");
   return 1;
  }

  if(send_line(ctx, 3, "ok\n"))
   return 1;
 
  return 0;
 }

 /* catch all */
 fprintf(stderr, "%s: non-understood traffic from peer (%d bytes):  \"%s\"\n", 
         ctx->error_prefix, n_bytes, buffer);

 send_line(ctx, 7, "failed\n");
 return 1;
}


int service_list_strategy_net_start(const struct buildmatrix_context *ctx,
                                    void * const strategy_context, const void *ptr)
{
 if(ctx->verbose > 2)
  fprintf(stderr, "%s: service_list_strategy_net_start()\n",
          ctx->error_prefix);

 return 0;
}

int service_list_strategy_net_iterative(const struct buildmatrix_context *ctx,
                                        void * const strategy_context, const void *ptr)
{
 char buffer[4096];
 int length;
 const char *job_name = ptr;

 if(ctx->verbose > 2)
  fprintf(stderr, "%s: service_list_strategy_net_iterative()\n",
          ctx->error_prefix);

 length = snprintf(buffer, 4096, "%s\n", job_name);

 if(send_line(ctx, length, buffer))
 {
  fprintf(stderr, "%s: service_list_strategy_net_iterative(): send_line() failed\n",
          ctx->error_prefix);
  return 1;
 }

 return 0;
}

int service_list_strategy_net_done(const struct buildmatrix_context *ctx, 
                                   void * const strategy_context, const void *ptr)
{
 if(ctx->verbose > 2)
  fprintf(stderr, "%s: service_list_strategy_net_done()\n",
          ctx->error_prefix);

 return send_line(ctx, 5, "done\n");
}

int service_list_builds_strategy_net_start(const struct buildmatrix_context *ctx,
                                           void * const strategy_context, const void *ptr)
{
 if(ctx->verbose > 2)
  fprintf(stderr, "%s: service_list_build_strategy_net_start()\n",
          ctx->error_prefix);

 return 0;
}

int service_list_builds_strategy_net_iterative(const struct buildmatrix_context *ctx,
                                                void * const strategy_context, const void *ptr)
{
 char buffer[4096];
 int length;
 struct list_builds_data *data = (struct list_builds_data *) ptr;

 if(ctx->verbose > 2)
  fprintf(stderr, "%s: service_list_build_strategy_net_iterative()\n",
          ctx->error_prefix);


 length = 
 snprintf(buffer, 4096, 
          "%d|%s|%s|%s|%s|%s|%d|%lld|%d|%d|%d|%d|%d|%d|%s|%s|%s|%s|\n", 
          data->build_id, data->branch, data->job_name, data->build_node, data->unique_identifier,
          data->user, data->build_result, data->build_time,
          (data->passed + data->failed + data->incompleted), 
          data->passed, data->failed, data->incompleted, data->has_parameters, data->has_tests,
          data->report_name == NULL ? "NULL" : data->report_name,
          data->output_name == NULL ? "NULL" : data->output_name,
          data->checksum_name == NULL ? "NULL" : data->checksum_name,
          data->revision);

 if(send_line(ctx, length, buffer))
 {
  fprintf(stderr, "%s: service_list_strategy_net_iterative(): send_line() failed\n",
          ctx->error_prefix);
  return 1;
 }

 return 0;
}

int service_list_builds_strategy_net_done(const struct buildmatrix_context *ctx, 
                                          void * const strategy_context, const void *ptr)
{
 if(ctx->verbose > 2)
  fprintf(stderr, "%s: service_list_build_strategy_net_done()\n",
          ctx->error_prefix);

 return send_line(ctx, 5, "done\n");
}

