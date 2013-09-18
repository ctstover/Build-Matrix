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

int default_configuration(struct buildmatrix_context *ctx);

int main(int argc, char **argv)
{
 struct buildmatrix_context *ctx;
 char string[512], hostname[256];
 int return_code = 0;
 time_t ti;

 if((ctx = setup(argc, argv)) == NULL)
  return 1;

 srand((unsigned int) time(NULL));

 if(ctx->verbose)
 {
  fprintf(stderr, "%s: I'm alive with mode %d\n", ctx->error_prefix, ctx->mode);
 }

 if(ctx->mode == BLDMTRX_MODE_PARSE)
 {
  /* if we have made it this far, then all input is verified */
  free(ctx);
  return 0;
 }

 if( (ctx->mode != BLDMTRX_MODE_INIT) &&
     (ctx->mode != BLDMTRX_MODE_CONNECT) )
 {
  if(limbo_cleanup(ctx))
   return 1;
 }

 if(setup_console(ctx))
  return 1;

 if((ctx->current_working_directory = get_current_dir_name()) == NULL)
 {
  fprintf(stderr, "%s: getwd() failed, %s\n", ctx->error_prefix, strerror(errno));
  return 1;
 }

 if(ctx->mode > 100)
 {
  if(ctx->local == 0)
  {
   if(resolve_connection_details(ctx))
    return 1;

   if(start_server(ctx))
   {
    stop_server(ctx);
    free(ctx);
    return 1;
   }
  }
 }

 switch(ctx->mode)
 {
  case BLDMTRX_MODE_INIT:
       if(ctx->project_name == NULL)
       {
        fprintf(stderr, "%s: need a project name.\n", ctx->error_prefix);
        return_code = 1;
        break;
       }

       if((return_code = check_string(ctx, ctx->project_name)) == 1)
        break;

       if(ctx->local_project_directory == NULL)
       {
        snprintf(string, 512, 
                 "Local project directory was not specified with --localproject.\n"
                 "Use current directory (%s) instead? ", ctx->current_working_directory);
        if(ctx->yes_no_prompt(ctx, string) == 0)
         return 1;

        ctx->local_project_directory = ctx->current_working_directory;
       }

       if((return_code = open_database(ctx)) == 1)
        break;

       if((return_code = new_database(ctx)) == 1)
        break;

       if((return_code = default_configuration(ctx)) == 1)
        break;

       close_database(ctx);

       if(ctx->verbose)
        fprintf(stderr, "%s: created new project at '%s'\n", 
                ctx->error_prefix, ctx->local_project_directory);
       break;

  case BLDMTRX_MODE_CONNECTIONS:
       return_code = list_connections(ctx);
       break;

  case BLDMTRX_MODE_CONNECT:
       if(ctx->local_project_directory == NULL)
       {
        snprintf(string, 512, 
                 "Local project directory was not specified with --localproject.\n"
                 "Use current directory (%s) instead? ", ctx->current_working_directory);
        if(ctx->yes_no_prompt(ctx, string) == 0)
         return 1;

        ctx->local_project_directory = ctx->current_working_directory;
       }

       if((return_code = open_database(ctx)) == 1)
        break;

       if((return_code = new_database(ctx)) == 1)
        break;

       if((return_code = default_configuration(ctx)) == 1)
        break;

       if((return_code = start_server(ctx)) == 1)
       {
        stop_server(ctx);
        break;
       }

       if((return_code = stop_server(ctx)) == 1)
        break;

       if((return_code = save_connection_details(ctx)) == 1)
        break;

       if(ctx->default_connection)
       {
        if(save_configurtation_value(ctx, "DefaultConnection", ctx->server, 1))
         return 1;
       }

       if(ctx->verbose)
        fprintf(stderr, "%s: created connected project at '%s'\n", 
                ctx->error_prefix, ctx->local_project_directory);
       break;

  case BLDMTRX_MODE_RECONFIGURE:
       if(ctx->local_project_directory == NULL)
       {
        snprintf(string, 512, 
                 "Local project directory was not specified with --localproject.\n"
                 "Use current directory (%s) instead? ", ctx->current_working_directory);
        if(ctx->yes_no_prompt(ctx, string) == 0)
         return 1;

        ctx->local_project_directory = ctx->current_working_directory;
       }

       if((return_code = open_database(ctx)) == 1)
        break;
       
       if((return_code = start_server(ctx)) == 1)
       {
        stop_server(ctx);
        break;
       }

       if((return_code = stop_server(ctx)) == 1)
        break;

       if((return_code = update_connection_details(ctx)) == 1)
        break;
       
       if(ctx->default_connection)
        if((return_code = save_configurtation_value(ctx, "DefaultConnection", ctx->server, 1)) == 1)
         break;

       close_database(ctx);

       if(ctx->verbose)
        fprintf(stderr, "%s: connected project reconfigured '%s'\n", 
                ctx->error_prefix, ctx->local_project_directory);
       break;

  case BLDMTRX_MODE_ADD_USER:
       return_code = add_user(ctx);
       break;

  case BLDMTRX_MODE_DEL_USER:
       return_code = remove_user(ctx);
       break;

  case BLDMTRX_MODE_MOD_USER:
       return_code = mod_user(ctx);
       break;

  case BLDMTRX_MODE_LIST_USERS:
       return_code = list_users(ctx);
       break;

  case BLDMTRX_MODE_ADD_ALIAS:
       return_code = add_alias(ctx);
       break;

  case BLDMTRX_MODE_DEL_ALIAS:
       return_code = remove_alias(ctx);
       break;

  case BLDMTRX_MODE_LIST_ALIASES:
       return_code = list_aliases(ctx);
       break;

  case BLDMTRX_MODE_LIST_BRANCHES:
       if(ctx->local == 0)
        return_code = list_branches(ctx);
       else
        return_code = service_list_branches(ctx);
       break;

  case BLDMTRX_MODE_LIST_HOSTS:
       if(ctx->local == 0)
        return_code =list_hosts(ctx);
       else
        return_code = service_list_hosts(ctx);
       break;

  case BLDMTRX_MODE_LIST_JOBS:
       if(ctx->local == 0)
        return_code = list_jobs(ctx);
       else
        return_code = service_list_jobs(ctx);
       break;

  case BLDMTRX_MODE_LIST_BUILDS:
       if(ctx->local == 0)
        return_code = list_builds(ctx);
       else
        return_code = service_list_builds(ctx);
       break;

  case BLDMTRX_MODE_SHOW_BUILD:
       if(ctx->build_output != NULL)
       {
        ctx->output_out_filename = ctx->build_output;
        ctx->build_output = NULL;
       }
       if(ctx->build_report != NULL)
       {
        ctx->report_out_filename = ctx->build_report;
        ctx->build_report = NULL;
       }
       if(ctx->checksum != NULL)
       {
        ctx->checksum_out_filename = ctx->checksum;
        ctx->checksum = NULL;
       }
       if(ctx->local == 0)
       {
        if((return_code = show_build(ctx)) == 1)
         break;

        return_code = ctx->process_build_strategy(ctx);

       } else {
        if((return_code = service_get_build(ctx)) == 1)
         break;

        if((return_code = ctx->process_build_strategy(ctx)) == 1)
         break;

        if((return_code = pull_test_results(ctx, &(ctx->test_results))) == 1)
         break;

        return_code = ctx->process_build_strategy(ctx);
       }
       break;

  case BLDMTRX_MODE_LIST_TESTS:
       return_code = list_tests(ctx);
       break;

//list parameters

  case BLDMTRX_MODE_SUBMIT:
       if(ctx->build_node == NULL)
       {
        if(gethostname(hostname, 256))
        {
         perror(ctx->error_prefix);
         ctx->build_node = NULL;
        } else {
         ctx->build_node = hostname;
        }
       }

       if(ctx->branch_name == NULL)
        ctx->branch_name = "trunk";

       if(ctx->job_name == NULL)
        ctx->job_name = "build";

       if(ctx->build_time == 0)
       {
        time(&ti);
        ctx->build_time = (long long int) ti;
       }

       return_code = submit(ctx, 0);
       break;

  case BLDMTRX_MODE_PULL:
       return_code = pull(ctx);
       break;

  case BLDMTRX_MODE_SCRATCH:
       if(ctx->local == 0)
        return_code = scratch(ctx);
       else
        return_code = service_scratch(ctx);
       break;

  case BLDMTRX_MODE_SERVE:
       return_code = serve(ctx);
       break;

  case BLDMTRX_MODE_LIST_VARIABLES:
       return_code = list_configurtation_values(ctx);      
       break;

  case BLDMTRX_MODE_GEN_REPORT:
       return_code = generate_report(ctx);      
       break;

  default:
      fprintf(stderr, "%s: I don't know what to do.\n", ctx->error_prefix);
      return_code = 1;
 }

 if(ctx->mode > 100)
 {
  if(ctx->local == 0)
   stop_server(ctx);
 }

 free(ctx);
 return return_code;
}

int default_configuration(struct buildmatrix_context *ctx)
{
 if(save_configurtation_value(ctx, "writeable", "yes", 0))
  return 1;

 if(save_configurtation_value(ctx, "securitymodel", "simple", 0))
  return 1;

 if(save_configurtation_value(ctx, "autoaddusers", "yes", 0))
  return 1;

 if(save_configurtation_value(ctx, "growtesttables", "yes", 0))
  return 1;

 if(save_configurtation_value(ctx, "newuserscratchdefault", "yes", 0))
  return 1;

 if(save_configurtation_value(ctx, "newuseradddefault", "yes", 0))
  return 1;

 if(save_configurtation_value(ctx, "newuserlistdefault", "yes", 0))
  return 1;

 if(save_configurtation_value(ctx, "newusergetdefault", "yes", 0))
  return 1;

 if(ctx->mode == BLDMTRX_MODE_INIT)
  if(save_configurtation_value(ctx, "projectname", ctx->project_name, 0))
   return 1;

 return 0;
}

