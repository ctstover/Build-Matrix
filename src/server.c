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

int serve(struct buildmatrix_context *ctx)
{
 char *tmp;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: serve()\n", ctx->error_prefix);

 if((tmp = resolve_configuration_value(ctx, "writeable")) == NULL)
 {
  fprintf(stderr, "%s: Can't figure out if I can write to myself.\n", ctx->error_prefix);
  return 1;
 }

 if((ctx->project_name = resolve_configuration_value(ctx, "projectname")) == NULL)
 {
  fprintf(stderr, "%s: Can't figure out my project name.\n", ctx->error_prefix);
  return 1;
 }
 
 if(strcmp(tmp, "yes") == 0)
 {
  ctx->write_master = 1;
 } else {
  if(ctx->verbose)
   fprintf(stderr, "%s: I don't seem to be able to write to my self.\n", ctx->error_prefix);
 }
 free(tmp);

 ctx->service_simple_lists_strategy.start = service_list_strategy_net_start;
 ctx->service_simple_lists_strategy.iterative = service_list_strategy_net_iterative;
 ctx->service_simple_lists_strategy.done = service_list_strategy_net_done;
 ctx->service_simple_lists_strategy.strategy_context = NULL;

 ctx->service_list_builds_strategy.start = service_list_builds_strategy_net_start;
 ctx->service_list_builds_strategy.iterative = service_list_builds_strategy_net_iterative;
 ctx->service_list_builds_strategy.done = service_list_builds_strategy_net_done;
 ctx->service_list_builds_strategy.strategy_context = NULL;

 ctx->process_build_strategy = service_submit;

 ctx->yes_no_prompt = NULL;

 while(service_input(ctx) == 0)
 {
  if(ctx->connection_initialized == 0)
   break;
 }

 return 0;
}

int service_list_tests(struct buildmatrix_context *ctx)
{
 return 1;
}

int rollback_submit(struct buildmatrix_context *ctx)
{
 if(ctx->verbose)
  fprintf(stderr, "%s: rollback_submit() - fix me\n", ctx->error_prefix);
 
 ctx->build_id = -1;

 if(close_database(ctx))
  return 1;

 return 0;
}

int service_submit(struct buildmatrix_context * const ctx)
{
 if(ctx->verbose > 1)
  fprintf(stderr, "%s: service_submit()\n", ctx->error_prefix);

 if(ctx->job_name == NULL)
 {
  fprintf(stderr, "%s: can't submit with no job name\n", ctx->error_prefix);
  return 1;
 }

 if(ctx->build_id < 1)
 {
  fprintf(stderr, "%s: can't submit with no build id\n", ctx->error_prefix);
  return 1;
 }

 if(add_build(ctx))
 {
  rollback_submit(ctx);
  send_line(ctx, 7, "failed\n");
  return 1;
 }

 if(ctx->verbose)
  fprintf(stderr, "%s: saved build in database: job(%s), branch(%s), unique identifier(%s), internal db build id (%d)\n",
          ctx->error_prefix, ctx->job_name, ctx->branch_name, ctx->unique_identifier, ctx->build_id);

 if(close_database(ctx))
  return 1;

 if(clean_state(ctx))
  return 1;

 return 0;
}

int generate_unique_identifier(struct buildmatrix_context *ctx)
{
 char temp[256], hostname[256], *h;
 unsigned char digest[16];
 int length, c, i, j;
 time_t now;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: generate_unique_identifier()\n", ctx->error_prefix);

 if(ctx->job_name == NULL)
 {
  fprintf(stderr, "%s: generate_unique_identifier(): I don't have a job name yet.\n",
          ctx->error_prefix);
  return 1;
 }

 if(ctx->branch_name == NULL)
 {
  fprintf(stderr, "%s: generate_unique_identifier(): I don't have a branch name yet.\n",
          ctx->error_prefix);
  return 1;
 }

 if(ctx->build_node == NULL)
 {
  if(gethostname(hostname, 256))
  {
   perror(ctx->error_prefix);
   snprintf(hostname, 256, "failed");
  }
  h = hostname;
 } else {
  h = ctx->build_node;
 }

 time(&now);

 length = snprintf(temp, 256, "%s%s%s%d%d%s", ctx->job_name, ctx->branch_name, h,
                   (int) now, rand(), ctx->revision);

 if(build_matrix_md5_wrapper(ctx, (unsigned char *) temp, length, digest))
  return 1;

 if((ctx->unique_identifier = (char *) malloc(21)) == NULL)
 {
  fprintf(stderr, "%s: generate_unique_identifier(): malloc(21) failed: %s\n",
          ctx->error_prefix, strerror(errno));
  return 1;
 }

 j = 0;
 for(i=0; i<16; i++)
 {
  c = digest[i] & 63;
  if( c < 26)
   ctx->unique_identifier[j++] = 'A' + c;
  else if( c < 52)
   ctx->unique_identifier[j++] = 'a' + (c - 26);
  else {
   if(c > 61)
    c -= 10;

   ctx->unique_identifier[j++] = '0' + (c - 52);
  }

  if(i % 4 == 3)
   ctx->unique_identifier[j++] = '-';
 }
 ctx->unique_identifier[--j] = 0;

 return 0;
}


