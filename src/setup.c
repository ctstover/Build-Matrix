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

void help(void)
{
 fprintf(stderr, 
         "\nBuild Matrix, © 2013 Stover Enterprises, LLC. Written by C. Thomas Stover \n"
         "This program is licensed under the terms of GNU General Public License version 3.\n\n"
         " usage: bldmtrx [options] mode\n"
         " where options are:\n"
         "\n  [general options]\n"
         "  --verbose\n"
         "  --force\n"
         "  --noninteractive\n"
         "  --help\n"
         "\n  [data entry related]\n"
         "  --alias name\n"
         "  --uid n\n"
         "  --user name\n"
         "  --email email_address\n"
         "  --privillages \"submit|scratch|get|list\"\n"
         "  --suite suitename\n"
         "  --branch branchname\n"
         "  --revision string\n"
         "  --job jobname\n"
         "  --identifier unique_identifier\n"
         "  --id build_id\n"
         "  --buildhost host\n"
         "  --testpasses n\n"
         "  --testfailures n\n"
         "  --testincompletes n\n"
         "  --buildreport filename (in/out file for build report)\n"
         "  --buildoutput filename (in/out file for build output)\n"
         "  --success\n"
         "  --failure\n"
         "  --checksum filename (in/out file for build output checksum / signature)\n"
         "  --testresults filename (in/out file for test scores)\n"
         "  --parameters filename (in/out file for parameters)\n"
         "\n  [network related]\n"
         "  --local\n"
         "  --localproject directory (project directory on this machine)\n"
         "  --remoteproject directory (project directory on server)\n"
         "  --bldmtrxpath path (server binary path)\n"
         "  --server server\n"
         "  --sshbin command\n"
         "  --ssh\n"
         "  --project string\n"
         "\n  [modify behavior of mode(s)]\n"
         "  --default\n"
         "  --dontgrowtesttables\n"
         " and mode is either:\n"
         "  init (create a project)\n"
         "  submit (submit a build for a job)\n"
         "  jobs (lists jobs)\n"
         "  branches (lists branches)\n"
         "  hosts (lists build hosts\\nodes)\n"
         "  builds (lists builds)\n"
         "  build (get build details)\n"
         "  tests (lists tests)\n"
         "  scratch (erase a build)\n"
         "  pull (retrieve build report, build output, or test scores)\n"
         "  parse (verify input file are correctly formated and exit)\n"
         "  createuser\n"
         "  deleteuser\n"
         "  moduser\n"
         "  users\n"
         "  reconfigure\n"
         "  serve\n"
         "  connect\n"
         "  connections\n"
         "  createalias\n"
         "  deletealias\n"
         "  aliases\n"
         "  variables\n"
         "  report\n"
         "\n");
}

struct buildmatrix_context *setup(int argc, char **argv)
{
 struct buildmatrix_context *ctx;
 int allocation_size, current_arg = 1, handled, length, i;

 allocation_size = sizeof(struct buildmatrix_context);
 if((ctx = (struct buildmatrix_context *) malloc(allocation_size)) == NULL)
 {
  perror("bld mtrx malloc");
  return NULL;
 }
 memset(ctx, 0, allocation_size);

 ctx->build_result = -1;
 ctx->grow_test_tables = 1;
 ctx->error_prefix = "build matrix";

 if(argc < 2)
 {
  help();
  free(ctx);
  return NULL;
 }

 while(current_arg < argc)
 {
  handled = 0;

  /* general options */
  if(strcmp(argv[current_arg], "--verbose") == 0)
  {
   ctx->verbose++;
   handled = 1;
  }

  if(strcmp(argv[current_arg], "--quiet") == 0)
  {
   ctx->quiet = 1;
   handled = 1;
  }

  if(strcmp(argv[current_arg], "--help") == 0)
  {
   help();
   free(ctx);
   return NULL;
  }

  if(strcmp(argv[current_arg], "--force") == 0)
  {
   ctx->force = 1;
   handled = 1;
  }

  if(strcmp(argv[current_arg], "--noninteractive") == 0)
  {
   ctx->non_interactive = 1;
   handled = 1;
  }

  if(strcmp(argv[current_arg], "--local") == 0)
  {
   ctx->local = 1;
   handled = 1;
  }

  /* data entry related */

  if(strcmp(argv[current_arg], "--buildhost") == 0)
  {
   if(current_arg + 1 == argc)
   {
    fprintf(stderr, "--buildhost requires an argument\n");
    return NULL;
   }
   ctx->build_node = strdup(argv[++current_arg]); //compatibility with free() logic
   if(check_string(ctx, ctx->build_node))
    return NULL;
   handled = 1;
  }

  if(strcmp(argv[current_arg], "--buildreport") == 0)
  {
   if(current_arg + 1 == argc)
   {
    fprintf(stderr, "--buildreport requires an argument\n");
    return NULL;
   }
   ctx->build_report = argv[++current_arg];
   if(check_string(ctx, ctx->build_report))
    return NULL;
   handled = 1;
  }

  if(strcmp(argv[current_arg], "--buildoutput") == 0)
  {
   if(current_arg + 1 == argc)
   {
    fprintf(stderr, "--buildoutput requires an argument\n");
    return NULL;
   }
   ctx->build_output = argv[++current_arg];
   if(check_string(ctx, ctx->build_output))
    return NULL;
   handled = 1;
  }

  if(strcmp(argv[current_arg], "--checksum") == 0)
  {
   if(current_arg + 1 == argc)
   {
    fprintf(stderr, "--checksum requires an argument\n");
    return NULL;
   }
   ctx->checksum = argv[++current_arg];
   if(check_string(ctx, ctx->checksum))
    return NULL;
   handled = 1;
  }

  if(strcmp(argv[current_arg], "--parameters") == 0)
  {
   if(current_arg + 1 == argc)
   {
    fprintf(stderr, "--parameters requires an argument\n");
    return NULL;
   }
   ctx->parameters_file = argv[++current_arg];
   handled = 1;
  }

  if(strcmp(argv[current_arg], "--testresults") == 0)
  {
   if(current_arg + 1 == argc)
   {
    fprintf(stderr, "--testresults requires an argument\n");
    return NULL;
   }
   ctx->test_results_file = argv[++current_arg];

   handled = 1;
  }

  if(strcmp(argv[current_arg], "--job") == 0)
  {
   if(current_arg + 1 == argc)
   {
    fprintf(stderr, "--job requires an argument\n");
    return NULL;
   }
   ctx->job_name = strdup(argv[++current_arg]); //compatibility with free() logic
   if(check_string(ctx, ctx->job_name))
    return NULL;
   handled = 1;
  }

  if(strcmp(argv[current_arg], "--branch") == 0)
  {
   if(current_arg + 1 == argc)
   {
    fprintf(stderr, "--branch requires an argument\n");
    return NULL;
   }
   ctx->branch_name = strdup(argv[++current_arg]); //compatibility with free() logic
   if(check_string(ctx, ctx->branch_name))
    return NULL;
   handled = 1;
  }

  if(strcmp(argv[current_arg], "--revision") == 0)
  {
   if(current_arg + 1 == argc)
   {
    fprintf(stderr, "--revision requires an argument\n");
    return NULL;
   }
   ctx->revision = strdup(argv[++current_arg]); //compatibility with free() logic
   if(check_string(ctx, ctx->revision))
    return NULL;
   handled = 1;
  }

  if(strcmp(argv[current_arg], "--uid") == 0)
  {
   if(current_arg + 1 == argc)
   {
    fprintf(stderr, "--uid requires an integer\n");
    return NULL;
   }

   sscanf(argv[++current_arg], "%d", &(ctx->posix_uid));
   handled = 1;
  }

  if(strcmp(argv[current_arg], "--alias") == 0)
  {
   if(current_arg + 1 == argc)
   {
    fprintf(stderr, "--alias requires a string\n");
    return NULL;
   }

   ctx->alias = strdup(argv[++current_arg]);  //compatibility with free() logic
   if(check_string(ctx, ctx->alias))
    return NULL;
   handled = 1;
  }

  if(strcmp(argv[current_arg], "--user") == 0)
  {
   if(current_arg + 1 == argc)
   {
    fprintf(stderr, "--user requires a string\n");
    return NULL;
   }

   ctx->user = strdup(argv[++current_arg]);  //compatibility with free() logic
   if(check_string(ctx, ctx->user))
    return NULL;
   handled = 1;
  }

  if(strcmp(argv[current_arg], "--privillages") == 0)
  {
   if(current_arg + 1 == argc)
   {
    fprintf(stderr, "--privillages requires a string\n");
    return NULL;
   }

   ctx->user_mode_bits = 0;
   length = strlen(argv[++current_arg]);
   i = 0;
   while(i < length)
   {
    if(strncmp(argv[current_arg] + i, "scratch", 7) == 0)
    {
     ctx->user_mode_bits |= BLDMTRX_ACCESS_BIT_SCRATCH;
     i += 7;
     continue;
    }   

    if(strncmp(argv[current_arg] + i, "get", 3) == 0)
    {
     ctx->user_mode_bits |= BLDMTRX_ACCESS_BIT_GET;
     i += 3;
     continue;
    }

    if(strncmp(argv[current_arg] + i, "list", 4) == 0)
    {
     ctx->user_mode_bits |= BLDMTRX_ACCESS_BIT_LIST;
     i += 4;
     continue;
    }   

    if(strncmp(argv[current_arg] + i, "submit", 6) == 0)
    {
     ctx->user_mode_bits |= BLDMTRX_ACCESS_BIT_SUBMIT;
     i += 6;
     continue;
    }   

    i++;
   }

   handled = 1;
  }

  if(strcmp(argv[current_arg], "--email") == 0)
  {
   if(current_arg + 1 == argc)
   {
    fprintf(stderr, "--email requires a string\n");
    return NULL;
   }

   ctx->email_address = strdup(argv[++current_arg]);  //compatibility with free() logic
   if(check_string(ctx, ctx->email_address))
    return NULL;
   handled = 1;
  }

  if(strcmp(argv[current_arg], "--identifier") == 0)
  {
   if(current_arg + 1 == argc)
   {
    fprintf(stderr, "--identifier requires a string\n");
    return NULL;
   }
   ctx->unique_identifier = strdup(argv[++current_arg]); //compatibility with free() logic
   if(check_string(ctx, ctx->unique_identifier))
    return NULL;
   handled = 1;
  }

  if(strcmp(argv[current_arg], "--id") == 0)
  {
   if(current_arg + 1 == argc)
   {
    fprintf(stderr, "--id requires an integer\n");
    return NULL;
   }

   sscanf(argv[++current_arg], "%d", &(ctx->build_id));
   handled = 1;
  }

  if(strcmp(argv[current_arg], "--project") == 0)
  {
   if(current_arg + 1 == argc)
   {
    fprintf(stderr, "--project requires a string\n");
    return NULL;
   }
   ctx->project_name = strdup(argv[++current_arg]); //compatibility with free() logic
   if(check_string(ctx, ctx->project_name))
    return NULL;
   handled = 1;
  }

  if(strcmp(argv[current_arg], "--success") == 0)
  {
   if(ctx->build_result != -1)
   {
    fprintf(stderr, "--success & --failure are exclusive and to be used only once\n");
    return NULL;
   }
   ctx->build_result = 1;
   handled = 1;
  }

  if(strcmp(argv[current_arg], "--failure") == 0)
  {
   if(ctx->build_result != -1)
   {
    fprintf(stderr, "--success & --failure are exclusive and to be used only once\n");
    return NULL;
   }
   ctx->build_result = 0;
   handled = 1;
  }

  if(strcmp(argv[current_arg], "--testpasses") == 0)
  {
   if(current_arg + 1 == argc)
   {
    fprintf(stderr, "--testpasses requires an argument\n");
    return NULL;
   }

   sscanf(argv[++current_arg], "%d", &(ctx->test_totals[1]));
   ctx->test_totals[0] += ctx->test_totals[1];
   handled = 1;
  }

  if(strcmp(argv[current_arg], "--testfailures") == 0)
  {
   if(current_arg + 1 == argc)
   {
    fprintf(stderr, "--testfailures requires an argument\n");
    return NULL;
   }

   sscanf(argv[++current_arg], "%d", &(ctx->test_totals[2]));
   ctx->test_totals[0] += ctx->test_totals[2];
   handled = 1;
  }

  if(strcmp(argv[current_arg], "--testincompletes") == 0)
  {
   if(current_arg + 1 == argc)
   {
    fprintf(stderr, "--testincompletes requires an argument\n");
    return NULL;
   }

   sscanf(argv[++current_arg], "%d", &(ctx->test_totals[3]));
   ctx->test_totals[0] += ctx->test_totals[3];
   handled = 1;
  }

  /* network related] */

  if(strcmp(argv[current_arg], "--server") == 0)
  {
   if(current_arg + 1 == argc)
   {
    fprintf(stderr, "--server requires an argument\n");
    return NULL;
   }
   ctx->server = argv[++current_arg];
   if(check_string(ctx, ctx->server))
    return NULL;
   handled = 1;
  }

  if(strcmp(argv[current_arg], "--default") == 0)
  {
   ctx->default_connection = 1;
   handled = 1;
  }

  if(strcmp(argv[current_arg], "--sshbin") == 0)
  {
   if(current_arg + 1 == argc)
   {
    fprintf(stderr, "--sshbin requires an argument\n");
    return NULL;
   }
   ctx->ssh_bin = argv[++current_arg];
   if(check_string(ctx, ctx->ssh_bin))
    return NULL;
   ctx->ssh = 1;
   handled = 1;
  }

  if(strcmp(argv[current_arg], "--ssh") == 0)
  {
   ctx->ssh = 1;
   handled = 1;
  }

  if(strcmp(argv[current_arg], "--localproject") == 0)
  {
   if(current_arg + 1 == argc)
   {
    fprintf(stderr, "--localproject requires an argument\n");
    return NULL;
   }
   ctx->local_project_directory = argv[++current_arg];
   handled = 1;
  }

  if(strcmp(argv[current_arg], "--remoteproject") == 0)
  {
   if(current_arg + 1 == argc)
   {
    fprintf(stderr, "--remoteproject requires an argument\n");
    return NULL;
   }
   ctx->remote_project_directory = argv[++current_arg];
   if(check_string(ctx, ctx->remote_project_directory))
    return NULL;
   handled = 1;
  }

  /* modify behavior of mode(s) */

  if(strcmp(argv[current_arg], "--dontgrowtesttables") == 0)
  {
   ctx->grow_test_tables = 0;
   handled = 1;
  }

  if(strcmp(argv[current_arg], "--bldmtrxpath") == 0)
  {
   if(current_arg + 1 == argc)
   {
    fprintf(stderr, "--bldmtrxpath requires an argument\n");
    return NULL;
   }
   ctx->remote_bin_path = argv[++current_arg];
   if(check_string(ctx, ctx->remote_bin_path))
    return NULL;
   handled = 1;
  }

  if(handled == 0)
  {
   if(argc - 1 == current_arg)
   {
    
    if(handled == 0)
    {

     if(strcmp(argv[current_arg], "submit") == 0)
     {
      ctx->mode = BLDMTRX_MODE_SUBMIT;
      handled = 1;
     }

     if(strcmp(argv[current_arg], "scratch") == 0)
     {
      ctx->mode = BLDMTRX_MODE_SCRATCH;
      handled = 1;
     }

     if(strcmp(argv[current_arg], "tests") == 0)
     {
      ctx->mode = BLDMTRX_MODE_LIST_TESTS;
      handled = 1;
     }

     if(strcmp(argv[current_arg], "builds") == 0)
     {
      ctx->mode = BLDMTRX_MODE_LIST_BUILDS;
      handled = 1;
     }

     if(strcmp(argv[current_arg], "build") == 0)
     {
      ctx->mode = BLDMTRX_MODE_SHOW_BUILD;
      handled = 1;
     }

     if(strcmp(argv[current_arg], "jobs") == 0)
     {
      ctx->mode = BLDMTRX_MODE_LIST_JOBS;
      handled = 1;
     }

     if(strcmp(argv[current_arg], "branches") == 0)
     {
      ctx->mode = BLDMTRX_MODE_LIST_BRANCHES;
      handled = 1;
     }

     if(strcmp(argv[current_arg], "hosts") == 0)
     {
      ctx->mode = BLDMTRX_MODE_LIST_HOSTS;
      handled = 1;
     }

     if(strcmp(argv[current_arg], "users") == 0)
     {
      ctx->mode = BLDMTRX_MODE_LIST_USERS;
      handled = 1;
     }

     if(strcmp(argv[current_arg], "pull") == 0)
     {
      ctx->mode = BLDMTRX_MODE_PULL;
      handled = 1;
     }

      if(strcmp(argv[current_arg], "parse") == 0)
     {
      ctx->mode = BLDMTRX_MODE_PARSE;
      handled = 1;
     }

     if(strcmp(argv[current_arg], "serve") == 0)
     {
      ctx->error_prefix = "build matrix server";
      ctx->connection_initialized = 1;
      ctx->in = 0;
      ctx->out = 1;
      ctx->mode = BLDMTRX_MODE_SERVE;
      handled = 1;
     }

     if(strcmp(argv[current_arg], "createuser") == 0)
     {
      ctx->mode = BLDMTRX_MODE_ADD_USER;
      handled = 1;
     }

     if(strcmp(argv[current_arg], "deleteuser") == 0)
     {
      ctx->mode = BLDMTRX_MODE_DEL_USER;
      handled = 1;
     }

     if(strcmp(argv[current_arg], "moduser") == 0)
     {
      ctx->mode = BLDMTRX_MODE_MOD_USER;
      handled = 1;
     }

     if(strcmp(argv[current_arg], "init") == 0)
     {
      ctx->mode = BLDMTRX_MODE_INIT;
      handled = 1;
     }

     if(strcmp(argv[current_arg], "connect") == 0)
     {
      ctx->mode = BLDMTRX_MODE_CONNECT;
      handled = 1;
     }

     if(strcmp(argv[current_arg], "connections") == 0)
     {
      ctx->mode = BLDMTRX_MODE_CONNECTIONS;
      handled = 1;
     }

     if(strcmp(argv[current_arg], "reconfigure") == 0)
     {
      ctx->mode = BLDMTRX_MODE_RECONFIGURE;
      handled = 1;
     }

     if(strcmp(argv[current_arg], "createalias") == 0)
     {
      ctx->mode = BLDMTRX_MODE_ADD_ALIAS;
      handled = 1;
     }

     if(strcmp(argv[current_arg], "deletealias") == 0)
     {
      ctx->mode = BLDMTRX_MODE_DEL_ALIAS;
      handled = 1;
     }

     if(strcmp(argv[current_arg], "aliases") == 0)
     {
      ctx->mode = BLDMTRX_MODE_LIST_ALIASES;
      handled = 1;
     }

     if(strcmp(argv[current_arg], "variables") == 0)
     {
      ctx->mode = BLDMTRX_MODE_LIST_VARIABLES;
      handled = 1;
     }

     if(strcmp(argv[current_arg], "report") == 0)
     {
      ctx->mode = BLDMTRX_MODE_GEN_REPORT;
      handled = 1;
     }

     if(handled == 0)
     {
      fprintf(stderr, "%s: unknown mode, '%s'\n", ctx->error_prefix, argv[current_arg]);
      free(ctx);
      return NULL;
     }
    }
    break;
   } else {
    fprintf(stderr, "%s: unkown or out of place parameter, '%s'\n",
            ctx->error_prefix, argv[current_arg]);
    free(ctx);
    return NULL;
   }
  }

  current_arg++;
 }

 if(ctx->mode == BLDMTRX_MODE_SHOW_BUILD)
 {
  if(ctx->unique_identifier == NULL)
  {
   fprintf(stderr, "%s: must specify a unique identifer for a get build operation\n",
           ctx->error_prefix);
   return NULL;
  }
 }

 if(ctx->mode == BLDMTRX_MODE_SCRATCH)
 {
  if(ctx->unique_identifier == NULL)
  {  
   fprintf(stderr, "%s: must specify a unique identifer for a get scratch operation\n",
           ctx->error_prefix);
   return NULL;
  }
 }

 if(ctx->parameters_file != NULL)
 {
  if(parameters_from_file(ctx, ctx->parameters_file))
  {
   fprintf(stderr, "%s: parameters_from_file() failed\n", ctx->error_prefix);
   free(ctx);
   return NULL;
  }
 } else {
  if(parameters_from_environ(ctx))
  {
   fprintf(stderr, "%s: parameters_from_environ() failed\n", ctx->error_prefix);
   free(ctx);
   return NULL;
  }
 }

 if(ctx->test_results_file != NULL)
 {
  if(load_test_results(ctx, ctx->test_results_file, &(ctx->test_results)))
  {
   fprintf(stderr, "%s: load_test_results() failed\n", ctx->error_prefix);
   free(ctx);
   return NULL;
  }
 }

 if(ctx->verbose > 1)
 {
  fprintf(stderr, "%s: Command Line Data:\n", ctx->error_prefix);

  if(ctx->local_project_directory != NULL)
   fprintf(stderr, "%s: local project dir = '%s'\n",
           ctx->error_prefix, ctx->local_project_directory);

  if(ctx->job_name != NULL)
   fprintf(stderr, "%s: job = '%s'\n", ctx->error_prefix, ctx->job_name);

  if(ctx->branch_name != NULL)
   fprintf(stderr, "%s: branch = '%s'\n", ctx->error_prefix, ctx->branch_name);

  if(ctx->user != NULL)
   fprintf(stderr, "%s: user = '%s'\n", ctx->error_prefix, ctx->user);

  if(ctx->user_mode_bits > 0)
  {
   fprintf(stderr, "%s: privillages: ", ctx->error_prefix);

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

 }

 return ctx;
}

int clean_state(struct buildmatrix_context *ctx)
{
 if(ctx->verbose > 1)
  fprintf(stderr, "%s: clean_state()\n", ctx->error_prefix);

 if(ctx->user != NULL)
 {
  free(ctx->user);
  ctx->user = NULL;
 }

 if(ctx->branch_name != NULL)
 {
  free(ctx->branch_name);
  ctx->branch_name = NULL;
 }

 if(ctx->job_name != NULL)
 {
  free(ctx->job_name);
  ctx->job_name = NULL;
 }

 if(ctx->build_node != NULL)
 {
  free(ctx->job_name);
  ctx->build_node = NULL;
 }

 if(ctx->unique_identifier != NULL)
 {
  free(ctx->unique_identifier);
  ctx->unique_identifier = NULL;
 }

 if(ctx->revision != NULL)
 {
  free(ctx->revision);
  ctx->revision = NULL;
 }

 if(ctx->build_report != NULL)
 {
  free(ctx->build_report);
  ctx->build_report = NULL;
 }

 if(ctx->build_output != NULL)
 {
  free(ctx->build_output);
  ctx->build_output = NULL;
 }

 if(ctx->checksum != NULL)
 {
  free(ctx->checksum);
  ctx->checksum = NULL;
 }

 if(ctx->parameters != NULL)
 {
  free(ctx->parameters);
  ctx->parameters = NULL;
 }

 if(ctx->test_results != NULL)
 {
  free(ctx->test_results);
  ctx->test_results = NULL;
 }

 ctx->build_id = -1;
 ctx->user_id = -1;
 return 0;
}

