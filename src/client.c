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


int start_server(struct buildmatrix_context *ctx)
{
 int server_input[2];
 int server_output[2];
 int n_bytes, i;
 char buffer[64];
 char *remote_project_directory = NULL, *remote_bin_path = NULL;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: start_server() \"%s\" is now \"build matrix client\"\n",
           ctx->error_prefix, ctx->error_prefix);

 ctx->error_prefix = "build matrix client";
 
 if(ctx->remote_project_directory != NULL)
 {
  remote_project_directory = ctx->remote_project_directory;
 } else {
  remote_project_directory = resolve_configuration_value(ctx, "remoteprojectdirectory");
 }

 if(remote_project_directory == NULL)
 {
  fprintf(stderr, 
          "%s: client operations need to use --remoteproject, "
          "or store remote project directory in configuration\n",
          ctx->error_prefix);
  return 1;
 }

 if(ctx->remote_bin_path != NULL)
 {
  remote_bin_path = ctx->remote_bin_path;
 } else {
  remote_bin_path = resolve_configuration_value(ctx, "remotebinpath");
 }

 if(remote_bin_path == NULL)
 {
  fprintf(stderr, 
          "%s: client operations need to use --bldmtrxpath, "
          "or store the remote binary path in configuration\n",
          ctx->error_prefix);
  return 1;
 }

 ctx->server_argument_vector[ctx->n_server_arguments++] = "bldmtrx";

 for(i=0; i<ctx->verbose; i++)
 {
  ctx->server_argument_vector[ctx->n_server_arguments++] = "--verbose";
 }

 ctx->server_argument_vector[ctx->n_server_arguments++] = "--localproject";
 ctx->server_argument_vector[ctx->n_server_arguments++] = remote_project_directory;
 ctx->server_argument_vector[ctx->n_server_arguments++] = "serve";

 if(ctx->ssh == 1)
 {
  if(ctx->ssh_bin == NULL)
  {
   fprintf(stderr, 
           "%s: client operations need to use --sshbin, "
           "or store the ssh binary path in configuration for ssh transport.\n",
           ctx->error_prefix);
    return 1;
  }

  if(ctx->server == NULL)
  {
   if((ctx->server = resolve_configuration_value(ctx, "server")) == NULL)
   {
    fprintf(stderr, 
           "%s: client operations need to use --server, or store the server in configuration\n",
           ctx->error_prefix);
    return 1;
   }
  }

  if(ctx->verbose)
   fprintf(stderr, "%s: transport is ssh to %s\n", ctx->error_prefix, ctx->server);
 } else {
  if(ctx->verbose)
   fprintf(stderr, "%s: transport is fork() & execv()\n", ctx->error_prefix);
 }

 if(pipe(server_input))
 {
  fprintf(stderr, "%s: pipe() failed, %s\n",
          ctx->error_prefix, strerror(errno));
  return 1;
 }

 if(pipe(server_output))
 {
  fprintf(stderr, "%s: pipe() failed, %s\n",
          ctx->error_prefix, strerror(errno));
  close(server_input[0]);
  close(server_output[0]);
  return 1;
 }

 if((ctx->child = fork()) == 0) 
 {
  /* in child */
  close(server_input[1]);
  close(server_output[0]);

  ctx->error_prefix = "build matrix forked child for server";

  if(dup2(server_input[0], 0) == -1)
  {
   fprintf(stderr, "%s: dup2() failed, %s\n",
           ctx->error_prefix, strerror(errno));
   exit(1);
  }

  if(dup2(server_output[1], 1) == -1)
  {
   fprintf(stderr, "%s: dup2() failed, %s\n",
           ctx->error_prefix, strerror(errno));
   exit(1);
  }

  if(ctx->ssh == 0)
  {
   ctx->server_argument_vector[ctx->n_server_arguments] = NULL;
   execv(remote_bin_path, ctx->server_argument_vector);

   fprintf(stderr, "%s: execv(%s) failed, %s. Try --bldmtrxpath\n",
           ctx->error_prefix, remote_bin_path, strerror(errno));

   exit(1);
  }

  int allocation_size, i, x = 0;
  char *arg_vector[4];

  arg_vector[0] = ctx->ssh_bin;
  arg_vector[1] = ctx->server;
  arg_vector[3] = NULL;

  allocation_size = ctx->n_server_arguments + 1;
  allocation_size += strlen(remote_bin_path);

  for(i=1; i<ctx->n_server_arguments; i++)
  {
   allocation_size += strlen(ctx->server_argument_vector[i]);
  }

  if((arg_vector[2] = malloc(allocation_size)) == NULL)
  {
   fprintf(stderr, "%s: malloc(%d) failed, %s\n",
           ctx->error_prefix, allocation_size, strerror(errno));
  }

  x += snprintf(arg_vector[2] + x, allocation_size - x, "%s", remote_bin_path);
  for(i=1; i<ctx->n_server_arguments; i++)
  {
   x += snprintf(arg_vector[2] + x, allocation_size - x, " %s", ctx->server_argument_vector[i]);
  }

  execv(ctx->ssh_bin, arg_vector);
  fprintf(stderr, "%s: execv(%s, [%s, %s, \"%s\"]) failed, %s\n",
          ctx->error_prefix, ctx->ssh_bin, arg_vector[0], arg_vector[1], arg_vector[2],
          strerror(errno));
  fflush(stderr);

  exit(1);
 }

 /* in parrent */
 close(server_input[0]);
 close(server_output[1]);
 ctx->out = server_input[1];
 ctx->in = server_output[0];
 ctx->connection_initialized = 1;

 if(ctx->verbose)
  fprintf(stderr, "%s: initializing dialog with server\n", ctx->error_prefix);

 if(send_line(ctx, 24, "build matrix initialize\n"))
  return 1;

 if(receive_line(ctx, 64, &n_bytes, buffer))
  return 1;

 if(n_bytes == 0)
 {
  fprintf(stderr, "%s: server must be dead.\n", ctx->error_prefix);
  return 1;
 }

 buffer[n_bytes] = 0;

 if( (n_bytes > 23) && (strncmp(buffer, "connection initialized|", 23) == 0) )
 {
  if((ctx->project_name = strdup(buffer + 23)) == NULL)
  {
   fprintf(stderr, "%s: strdup() failed, %s\n", ctx->error_prefix, strerror(errno));   
   return 1;
  }

  if(check_string(ctx, ctx->project_name))
  {
   free(ctx->project_name);
   fprintf(stderr, "%s: server sent project name does not pass check_string()\n", 
           ctx->error_prefix);
   return 1;
  }

  if(ctx->verbose)
   fprintf(stderr, "%s: server connection initialized for project %s\n", 
           ctx->error_prefix, ctx->project_name);
  return 0;
 }

 fprintf(stderr, 
         "%s: wrong initialization response from server (%d bytes) \"%s\"\n",
         ctx->error_prefix, n_bytes, buffer);
 return 1;
}

int stop_server(struct buildmatrix_context *ctx)
{
 int status;

 if(ctx->verbose)
  fprintf(stderr, "%s: shuting down dialog with server\n", ctx->error_prefix);

 send_line(ctx, 9, "shutdown\n");

 if(ctx->child > 0)
  wait(&status);

 return 0;
}

int submit(struct buildmatrix_context *ctx, int from_server_to_client)
{
 int n_bytes;
 char buffer[32];

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: submit()\n", ctx->error_prefix);

 if(send_user_name(ctx))
  return 1;

 if(send_job_name(ctx))
  return 1;

 if(send_branch_name(ctx))
  return 1;

 if(send_revision(ctx))
  return 1;

 if(start_build_submission(ctx))
  return 1;

 if(send_build_host_name(ctx))
  return 1;

 if(send_test_totals(ctx))
  return 1;

 if(send_build_time(ctx))
  return 1;

 if(send_build_report(ctx, from_server_to_client))
  return 1;

 if(send_build_output(ctx, from_server_to_client))
  return 1;

 if(send_build_result(ctx))
  return 1;

 if(send_checksum(ctx, from_server_to_client))
  return 1;

 if(send_parameters(ctx))
  return 1;

 if(send_test_results(ctx, ctx->test_results))
  return 1;

 if(send_line(ctx, 7, "submit\n"))
  return 1;

 if(receive_line(ctx, 32, &n_bytes, buffer))
  return 1;

 if(strncmp(buffer, "ok", 2) == 0)
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: build transfered successfully\n", ctx->error_prefix);
 
  return 0;
 }

 fprintf(stderr, "%s: unexpected response to \"submit\", \"%s\"\n",
         ctx->error_prefix, buffer);

 return 1;
}


int list_jobs(struct buildmatrix_context *ctx)
{
 int n_bytes;
 char buffer[4096];

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: lists_jobs()\n", ctx->error_prefix);

 if(ctx->branch_name != NULL)
 {
  if(send_branch_name(ctx))
  return 1;
 }

 if(ctx->build_node != NULL)
 {
  if(send_build_host_name(ctx))
  return 1;
 }

 if(send_user_name(ctx))
  return 1;


 if(send_line(ctx, 10, "list jobs\n"))
  return 1;

 if(receive_line(ctx, 4096, &n_bytes, buffer))
  return 1;

 if(strncmp(buffer, "ok", 2) != 0)
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: unexepcted response to \"list jobs\" from peer: %s",
           ctx->error_prefix, buffer);
  return 1;
 }

 if(ctx->branch_name == NULL)
 {
  if(ctx->build_node == NULL)
  {
   snprintf(buffer, 4096, "Unique job names in use:");
  } else {
   snprintf(buffer, 4096, "Unique job names in use with build node \"%s\":",
          ctx->build_node);
  }
 } else {
  if(ctx->build_node == NULL)
  {
   snprintf(buffer, 4096, "Unique job names in use with branch \"%s\":",
          ctx->branch_name);
  } else {
   snprintf(buffer, 4096, "Unique job names in use with build node \"%s\" and branch \"%s\":",
          ctx->build_node, ctx->branch_name);
  }
 }

 if(ctx->list_ops_strategy.start(ctx, ctx->list_ops_strategy.strategy_context, buffer))
  return 1;

 while(1)
 {
  if(receive_line(ctx, 4096, &n_bytes, buffer))
   return 1;

  if( (n_bytes == 5) && (strncmp(buffer, "done", 4) == 0) )
   break;
    
  if( (n_bytes > 6) && (strncmp(buffer, "failed", 6) == 0) )
  {
   fprintf(stderr, "%s: list_jobs(): server failure\n", ctx->error_prefix);
   return 1;
  }

  if(ctx->list_ops_strategy.iterative(ctx, ctx->list_ops_strategy.strategy_context, buffer))
   return 1;
 }

 return ctx->list_ops_strategy.done(ctx, ctx->list_ops_strategy.strategy_context, NULL);
}

int list_branches(struct buildmatrix_context *ctx)
{
 int n_bytes;
 char buffer[4096];

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: lists_branches()\n", ctx->error_prefix);

 if(ctx->job_name != NULL)
 {
  if(send_job_name(ctx))
  return 1;
 }

 if(ctx->build_node != NULL)
 {
  if(send_build_host_name(ctx))
  return 1;
 }

 if(send_user_name(ctx))
  return 1;

 if(send_line(ctx, 14, "list branches\n"))
  return 1;

 if(receive_line(ctx, 4096, &n_bytes, buffer))
  return 1;

 if(strncmp(buffer, "ok", 2) != 0)
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: unexepcted response to \"list branches\" from peer: %s\n",
           ctx->error_prefix, buffer);
  return 1;
 }

 if(ctx->job_name == NULL)
 {
  if(ctx->build_node == NULL)
  {
   snprintf(buffer, 4096, "Unique branch names in use:");
  } else {
   snprintf(buffer, 4096, "Unique branch names in use with build node \"%s\":",
          ctx->build_node);
  }
 } else {
  if(ctx->build_node == NULL)
  {
   snprintf(buffer, 4096, "Unique branch names in use with job \"%s\":",
          ctx->job_name);
  } else {
   snprintf(buffer, 4096, "Unique branch names in use with job \"%s\" and build node \"%s\":",
          ctx->job_name, ctx->build_node);
  }
 }

 if(ctx->list_ops_strategy.start(ctx, ctx->list_ops_strategy.strategy_context, buffer))
  return 1;

 while(1)
 {
  if(receive_line(ctx, 4096, &n_bytes, buffer))
   return 1;

  if( (n_bytes == 5) && (strncmp(buffer, "done", 4) == 0) )
   break;
    
  if( (n_bytes > 6) && (strncmp(buffer, "failed", 6) == 0) )
  {
   fprintf(stderr, "%s: list_branches(): server failure\n", ctx->error_prefix);
   return 1;
  }

  if(ctx->list_ops_strategy.iterative(ctx, ctx->list_ops_strategy.strategy_context, buffer))
   return 1;
 }

 return ctx->list_ops_strategy.done(ctx, ctx->list_ops_strategy.strategy_context, NULL);
}

int list_hosts(struct buildmatrix_context *ctx)
{
 int n_bytes;
 char buffer[4096];

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: lists_hosts()\n", ctx->error_prefix);

 if(ctx->branch_name != NULL)
 {
  if(send_branch_name(ctx))
  return 1;
 }

 if(ctx->job_name != NULL)
 {
  if(send_job_name(ctx))
  return 1;
 }

 if(send_user_name(ctx))
  return 1;

 if(send_line(ctx, 11, "list hosts\n"))
  return 1;

 if(receive_line(ctx, 4096, &n_bytes, buffer))
  return 1;

 if(strncmp(buffer, "ok", 2) != 0)
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: unexepcted response to \"list hosts\" from peer: %s\n",
           ctx->error_prefix, buffer);
  return 1;
 }

 if(ctx->branch_name == NULL)
 {
  if(ctx->job_name == NULL)
  {
   snprintf(buffer, 4096, "Unique build hosts in use:");
  } else {
   snprintf(buffer, 4096, "Unique build hosts in use with job name \"%s\":",
          ctx->job_name);
  }
 } else {
  if(ctx->job_name == NULL)
  {
   snprintf(buffer, 4096, "Unique build hosts in use with branch \"%s\":",
          ctx->branch_name);
  } else {
   snprintf(buffer, 4096, "Unique build hosts in use with job name \"%s\" and branch \"%s\":",
          ctx->job_name, ctx->branch_name);
  }
 }
 
 if(ctx->list_ops_strategy.start(ctx, ctx->list_ops_strategy.strategy_context, buffer))
  return 1;

 while(1)
 {
  if(receive_line(ctx, 4096, &n_bytes, buffer))
   return 1;

  if( (n_bytes == 5) && (strncmp(buffer, "done", 4) == 0) )
   break;
    
  if( (n_bytes > 6) && (strncmp(buffer, "failed", 6) == 0) )
  {
   fprintf(stderr, "%s: list_hosts(): server failure\n", ctx->error_prefix);
   return 1;
  }

  if(ctx->list_ops_strategy.iterative(ctx, ctx->list_ops_strategy.strategy_context, buffer))
   return 1;
 }

 return ctx->list_ops_strategy.done(ctx, ctx->list_ops_strategy.strategy_context, NULL);
}


int pull(struct buildmatrix_context *ctx)
{
 int n_bytes, code;
 char buffer[32];

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: pull()\n", ctx->error_prefix);

 if(open_database(ctx))
  return 1;

 ctx->process_build_strategy = service_submit;

 n_bytes = snprintf(buffer, 32, "pull %d\n", ctx->pull_build_id);

 if(send_line(ctx, n_bytes, buffer))
  return 1;

 if(receive_line(ctx, 32, &n_bytes, buffer))
  return 1;

 if(strncmp(buffer, "ok", 2) == 0)
 {
  while((code = service_input(ctx)) == 0);
 
  if(code != 2)
  {
   fprintf(stderr, "%s: Pull did not complete as expected.\n",
           ctx->error_prefix);
   return 1;
  }

 } else {
  fprintf(stderr, "%s: unexpected response to \"pull\", \"%s\"\n",
          ctx->error_prefix, buffer);
 }

 /* ctx->pull_build_id should be updated by the server */
 if(update_pull_build_id(ctx))
 {
  fprintf(stderr, "%s: pull(): update_pull_build_id() failed\n", ctx->error_prefix);
  return 1;
 }

 return 0;
}

int show_build(struct buildmatrix_context *ctx)
{
 int n_bytes, code;
 char buffer[32];

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: show_build()\n", ctx->error_prefix);

 if(send_user_name(ctx))
  return 1;

 if(send_job_name(ctx))
  return 1;

 if(send_branch_name(ctx))
  return 1;

 if(send_unique_identifier(ctx))
  return 1;

 if(send_line(ctx, 10, "get build\n"))
  return 1;

 if(receive_line(ctx, 32, &n_bytes, buffer))
  return 1;

 if(strncmp(buffer, "failed", 6) == 0)
 {
  if(ctx->verbose)
  fprintf(stderr, "%s: peer rejected \"get build\" request: \"%s\"\n",
          ctx->error_prefix, buffer);
  return 1;
 }

 if(strncmp(buffer, "ok", 2) != 0)
 {
  if(ctx->verbose)
  fprintf(stderr, "%s: unexpected response to \"get build\", \"%s\"\n",
          ctx->error_prefix, buffer);

  return 1;
 }

 ctx->build_node = NULL; /* prevent bad free() on data that should not be there */

 while((code = service_input(ctx)) == 0)
 {
  if(ctx->connection_initialized == 0)
   return 1;
 }

 if(code != 2)
  return 1;

 return 0;
}

int list_tests(struct buildmatrix_context *ctx)
{
 int n_bytes;
 char buffer[32];

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: lists_tests()\n", ctx->error_prefix);

 if(send_user_name(ctx))
  return 1;

 if(send_line(ctx, 11, "list tests\n"))
  return 1;

 if(receive_line(ctx, 32, &n_bytes, buffer))
  return 1;

 if(strncmp(buffer, "ok", 2) == 0)
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: entered into list jobs sub dialog mode\n",
           ctx->error_prefix);

  if(send_line(ctx, 5, "done\n"))
   return 1;

  if(receive_line(ctx, 32, &n_bytes, buffer))
   return 1;

  if(strncmp(buffer, "ok", 2) == 0)
  {
   if(ctx->verbose)
    fprintf(stderr, "%s: leaving list tests sub dialog mode\n", ctx->error_prefix);
   return 0;
  }

  fprintf(stderr, "%s: unexpected response to \"done\", \"%s\"\n", 
          ctx->error_prefix, buffer);
  return 1;
 }

 fprintf(stderr, "%s: unexpected response to \"list tests\", \"%s\"\n",
         ctx->error_prefix, buffer);
 return 1;
}

int list_builds(struct buildmatrix_context *ctx)
{
 int n_bytes, i, field, total, start;
 char buffer[4096];

 struct list_builds_data data;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: lists_builds()\n", ctx->error_prefix);

 if(send_user_name(ctx))
  return 1;

 if(ctx->job_name != NULL)
 {
  if(send_job_name(ctx))
   return 1;
 }

 if(ctx->branch_name != NULL)
 {
  if(send_branch_name(ctx))
   return 1;
 }

 if(ctx->build_node != NULL)
 {
  if(send_build_host_name(ctx))
   return 1;
 }

//success or failure

//less than greater than build number

 if(send_line(ctx, 12, "list builds\n"))
  return 1;

 if(receive_line(ctx, 32, &n_bytes, buffer))
  return 1;

 if(strncmp(buffer, "ok", 2) != 0)
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: unexpected response to \"list builds\"\n", ctx->error_prefix);

  return 1;
 }

 if(ctx->build_list_strategy.start(ctx, 
                                   ctx->build_list_strategy.strategy_context,
                                   NULL))
 {
  if(ctx->verbose > 1)
   fprintf(stderr, "%s: list_builds(): strategy callback failed\n",
           ctx->error_prefix);
  return 1;
 }

 while(1)
 {
  if(receive_line(ctx, 4096, &n_bytes, buffer))
   return 1;

  if( (n_bytes == 5) && (strncmp(buffer, "done", 4) == 0) )
  {
   if(ctx->verbose)
    fprintf(stderr, "%s: finsished with list builds querry\n", ctx->error_prefix);
   break;
  }
 
  if( (n_bytes > 6) && (strncmp(buffer, "failed", 6) == 0) )
  {
   fprintf(stderr, "%s: list_builds(): server failure\n", ctx->error_prefix);
   return 1;
  }

  /* branch name, job name, build node, build number, user, build result, start time, end time, totals  */
  start = 0;
  field = 0;
  for(i=0; i<n_bytes; i++)
  {
   if(buffer[i] == '|')
   {
    buffer[i] = 0;
    switch(field)
    {
     case 0:
          sscanf(buffer + start, "%d", &data.build_id);
          start = i + 1;
          field++;
          break;

     case 1:
          data.branch = buffer + start;
          start = i + 1;
          field++;
          break;

     case 2:
          data.job_name = buffer + start;
          start = i + 1;
          field++;
          break;

     case 3:
          data.build_node = buffer + start;
          start = i + 1;
          field++;
          break;

     case 4:
          data.unique_identifier = buffer + start;
          start = i + 1;
          field++;
          break;

     case 5:
          data.user = buffer + start;
          start = i + 1;
          field++;
          break;

     case 6:
          sscanf(buffer + start, "%d", &data.build_result);
          start = i + 1;
          field++;
          break;

     case 7:
          sscanf(buffer + start, "%lld", &data.build_time);
          start = i + 1;
          field++;
          break;

     case 8:
          sscanf(buffer + start, "%d", &total);
          start = i + 1;
          field++;
          break;

     case 9:
          sscanf(buffer + start, "%d", &data.passed);
          start = i + 1;
          field++;
          break;

     case 10:
          sscanf(buffer + start, "%d", &data.failed);
          start = i + 1;
          field++;
          break;

     case 11:
          sscanf(buffer + start, "%d", &data.incompleted);
          start = i + 1;
          field++;
          break;

     case 12:
          sscanf(buffer + start, "%d", &data.has_parameters);
          start = i + 1;
          field++;
          break;

     case 13:
          sscanf(buffer + start, "%d", &data.has_tests);
          start = i + 1;
          field++;
          break;

     case 14:
          data.report_name = buffer + start;
          if(strcmp(data.report_name, "NULL") == 0)
           data.report_name = NULL;
          start = i + 1;
          field++;
          break;

     case 15:
          data.output_name = buffer + start;
          if(strcmp(data.output_name, "NULL") == 0)
           data.output_name = NULL;
          start = i + 1;
          field++;
          break;

     case 16:
          data.checksum_name = buffer + start;
          if(strcmp(data.checksum_name, "NULL") == 0)
           data.checksum_name = NULL;
          start = i + 1;
          field++;
          break;

     case 17:
          data.revision = buffer + start;
          start = i + 1;
          field++;
          break;

    }
   }
  }

  if(field != 18)
  {
   if(ctx->verbose)
    fprintf(stderr, "%s: wrong number of columns (%d instead of 18) in lists builds results (%d bytes) \"%s\"\n",
            ctx->error_prefix, field, n_bytes, buffer);

   send_line(ctx, 7, "failed\n");
   return 1;
  }

  if(total != (data.passed + data.failed + data.incompleted) )
  {
   if(ctx->verbose)
    fprintf(stderr, "%s: test totals failed sanity check\n", ctx->error_prefix);

   send_line(ctx, 7, "failed\n");
   return 1;
  }

  if(ctx->build_list_strategy.iterative(ctx, 
                                       ctx->build_list_strategy.strategy_context,
                                       &data))
  {
   if(ctx->verbose > 1)
    fprintf(stderr, "%s: list_builds(): strategy callback failed\n",
            ctx->error_prefix);
   return 1;
  }

 }

 if(ctx->build_list_strategy.done(ctx, ctx->build_list_strategy.strategy_context,
                                           NULL))
 {
  if(ctx->verbose > 1)
   fprintf(stderr, "%s: list_builds(): strategy callback failed\n",
           ctx->error_prefix);
  return 1;
 }

 return 0;
}

int scratch(struct buildmatrix_context *ctx)
{
 int n_bytes;
 char buffer[32];

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: scratch()\n", ctx->error_prefix);

 if(send_user_name(ctx))
  return 1;

 if(send_job_name(ctx))
  return 1;

 if(send_branch_name(ctx))
  return 1;

 if(send_unique_identifier(ctx))
  return 1;

 if(send_line(ctx, 8, "scratch\n"))
  return 1;

 if(receive_line(ctx, 32, &n_bytes, buffer))
  return 1;

 if(strncmp(buffer, "ok", 2) == 0)
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: scratch successfull\n", ctx->error_prefix);

  return 0;
 }

 fprintf(stderr, "%s: unexpected response to \"scratch\", \"%s\"\n",
         ctx->error_prefix, buffer);

 return 1;
}


