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

int yes_no_prompt(struct buildmatrix_context *ctx, char *message)
{
 char in[4];

 printf("%s ", message);

 if(ctx->force)
 {
  printf("yes/[no]? (prompt overiden by --force) yes\n");
  return 1;
 }

 if(ctx->non_interactive)
 {
  printf("yes/[no]? (prompt overiden by --noninteractive) no\n");
  return 0;
 }

 printf("yes/[no]? ");

 if(scanf("%3s", in) != 1)
  return 0;

 if(strcmp("yes", in) == 0)
  return 1;

 return 0;
}

struct service_list_strategy_console_context
{
 char *header;
 int count;
};


int service_list_strategy_console_start(const struct buildmatrix_context *ctx, 
                                        void * const strategy_context, const void *ptr)
{
 struct service_list_strategy_console_context *sc;
 sc = (struct service_list_strategy_console_context *) strategy_context;

 if(ctx->verbose > 2)
  fprintf(stderr, "%s: service_list_strategy_console_start()\n", ctx->error_prefix);

 sc->count = 0;
 if(ctx->quiet == 0)
 {
  if(ptr != NULL)
   printf("%s\n", (char *) ptr); 
  else 
   printf("%s\n", sc->header);
 }

 return 0;
}

int service_list_strategy_console_iterative(const struct buildmatrix_context *ctx, 
                                            void * const strategy_context, const void *ptr)
{
 struct service_list_strategy_console_context *sc;
 sc = (struct service_list_strategy_console_context *) strategy_context;

 if(ctx->verbose > 2)
  fprintf(stderr, "%s: service_list_strategy_console_iterative()\n", ctx->error_prefix);

 printf("%s\n", (char *) ptr);

 sc->count++;
 return 0;
}

int service_list_strategy_console_done(const struct buildmatrix_context *ctx, 
                                       void * const strategy_context, const void *ptr)
{
 struct service_list_strategy_console_context *sc;
 sc = (struct service_list_strategy_console_context *) strategy_context;

 if(ctx->verbose > 2)
  fprintf(stderr, "%s: service_list_strategy_console_done()\n", ctx->error_prefix);

 if(ctx->quiet == 0)
  printf("(%d) entries returned\n", sc->count);

 return 0;
}

int service_list_builds_strategy_console_start(const struct buildmatrix_context *ctx, 
                                               void * const strategy_context, const void *ptr)
{
 struct service_list_strategy_console_context *sc;
 sc = (struct service_list_strategy_console_context *) strategy_context;

 if(ctx->verbose > 2)
  fprintf(stderr, "%s: service_list_builds_strategy_console_start()\n", ctx->error_prefix);

 sc->count = 0;
 if(ctx->quiet == 0)
 {
  printf("Builds meeting criterea:\n");
 }

 return 0;
}

int service_list_builds_strategy_console_iterative(const struct buildmatrix_context *ctx, 
                                                   void * const strategy_context, const void *ptr)
{
 struct service_list_strategy_console_context *sc;
 sc = (struct service_list_strategy_console_context *) strategy_context;
 struct list_builds_data *data = (struct list_builds_data *) ptr;
 char time_string[128];
 struct tm tm;
 int items, tt;
 time_t ptime;

 if(ctx->verbose > 2)
  fprintf(stderr, "%s: service_list_builds_strategy_console_iterative()\n", ctx->error_prefix);

 ptime = data->build_time;
 localtime_r(&ptime, &tm);
 strftime(time_string, 128, "%c", &tm);

 printf("%d: Job \"%s\" ", data->build_id, data->job_name);

 if(data->build_result)
  printf("succeeded ");
 else
  printf("failed ");

 printf("on revision %s of branch \"%s\"", data->revision, data->branch);

 printf(", run by %s on %s at %s ", data->user, data->build_node, time_string);

 printf("saved with the unique identifier %s. ", data->unique_identifier);

 tt = data->passed + data->failed + data->incompleted;
 
 if(tt)
  printf("The test totalls are %d/%d/%d", data->passed, data->failed, data->incompleted);

 items = data->has_parameters + data->has_tests + 
         (data->report_name != NULL) +
         (data->output_name != NULL) + 
         (data->checksum_name != NULL);

 if(items)
 {
  if(tt)
   printf(" and the build includes ");  
  else
   printf("The build includes ");
 }

 if(data->has_parameters)
 {
  printf("a parameter set");
  items--;
  if(items > 1)
   printf(", ");
  else if(items > 0)
   printf(", and ");
 }

 if(data->has_tests)
 {
  printf("test results");
  items--;
  if(items > 1)
   printf(", ");
  else if(items > 0)
   printf(", and ");
 }

 if(data->report_name != NULL)
 {
  printf("a build report");
  items--;
  if(items > 1)
   printf(", ");
  else if(items > 0)
   printf(", and ");
 }

 if(data->output_name != NULL)
 {
  printf("a build output");
  items--;
  if(items > 1)
   printf(", ");
  else if(items > 0)
   printf(", and ");
 }

 if(data->checksum_name != NULL)
 {
  printf("a checkum/signiture");
 }

 if(tt || data->has_parameters + data->has_tests + 
          (data->report_name != NULL) +
          (data->output_name != NULL) + 
          (data->checksum_name != NULL) );

 printf(".");

 printf("\n");

 sc->count++;
 return 0;
}

int service_list_builds_strategy_console_done(const struct buildmatrix_context *ctx, 
                                              void * const strategy_context, const void *ptr)
{
 struct service_list_strategy_console_context *sc;
 sc = (struct service_list_strategy_console_context *) strategy_context;

 if(ctx->verbose > 2)
  fprintf(stderr, "%s: service_list_builds_strategy_console_done()\n", ctx->error_prefix);

 if(ctx->quiet == 0)
  printf("(%d) builds returned\n", sc->count);

 return 0;
}

int process_build_strategy_console(struct buildmatrix_context * const ctx)
{
 char *scores_out_filename, *parameters_out_filename;
 int code = 0;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: process_builds_strategy_console()\n", ctx->error_prefix);

 if(ctx->quiet == 0)
 {
  /* title */
  printf("\nDetails retrieved for Unique Identifier %s of Job: \"%s\" on Branch: \"%s\" at Revision: \"%s\".\n",
         ctx->unique_identifier, ctx->job_name, ctx->branch_name, ctx->revision);

  /* result */
  printf("\tBuild Result: %s\n", ctx->build_result ? "success" : "failure");

  /* totals */
  if(ctx->test_totals[0])
   printf("\tTest Totals: %d passed, %d failed, %d incompleted\n",
          ctx->test_totals[1], ctx->test_totals[2], ctx->test_totals[3]);
  else
   printf("\tTest Totals: N/A\n");

  /* host */
  printf("\tBuild Host: %s\n", ctx->build_node);

  /* time */
  printf("\tBuild Time: %lld\n", ctx->build_time);

  /* user */
  printf("\tBuild User: %s\n", ctx->user);

  /* test scores */
  if(ctx->test_results == NULL)
   printf("\tTest Scores: N/A\n");
  else {
   if((scores_out_filename = ctx->test_results_file) == NULL)
    scores_out_filename = "./test-results.bmt";

   if(export_test_results_bldmtrx_format(ctx, ctx->test_results, scores_out_filename))
   {
    printf("\tTest Scores: export failure\n");
    code = 1;
   } else {
    printf("\tTest Scores: written in Build Matrix format to %s\n", scores_out_filename);   
   }
  }

  /* parameters */
  if(ctx->parameters == NULL)
   printf("\tParameters: N/A\n");
  else {
   if((parameters_out_filename = ctx->parameters_file) == NULL)
    parameters_out_filename = "./parameters";

   if(parameters_to_file(ctx, parameters_out_filename))
   {
    printf("\tParameters: export failure\n");
    code = 1;
   } else {
    printf("\tParameters: written in Build Matrix format to %s\n", parameters_out_filename);   
   }
  }

  /* output files */
  if(ctx->output_written)
   printf("\tBuild Output: written to \"%s\"\n", ctx->output_out_filename);
  else
   printf("\tBuild Output: N/A\n");

  if(ctx->report_written)
   printf("\tBuild Report: written to \"%s\"\n", ctx->report_out_filename);
  else
   printf("\tBuild Report: N/A\n");

  if(ctx->checksum_written)
   printf("\tBuild Checksum/Signature: written to \"%s\"\n", ctx->checksum_out_filename);
  else
   printf("\tBuild Checksum/Signature: N/A\n");
 }

 printf("\n");


 return code;
}


int setup_console(struct buildmatrix_context * const ctx)
{
 struct service_list_strategy_console_context *sc;
 struct iterative_strategy s;

 ctx->yes_no_prompt = yes_no_prompt;

 s.start = service_list_strategy_console_start;
 s.iterative = service_list_strategy_console_iterative;
 s.done = service_list_strategy_console_done;
 if((sc = (struct service_list_strategy_console_context *)
          malloc(sizeof(struct service_list_strategy_console_context))) == NULL)
  return 1;

 sc->header = "Unique job names in use:";
 s.strategy_context = sc;

 if(ctx->local)
  ctx->service_simple_lists_strategy = s;
 else
  ctx->list_ops_strategy = s;

 s.start = service_list_builds_strategy_console_start;
 s.iterative = service_list_builds_strategy_console_iterative;
 s.done = service_list_builds_strategy_console_done;
 if((sc = (struct service_list_strategy_console_context *)
          malloc(sizeof(struct service_list_strategy_console_context))) == NULL)
  return 1;

 s.strategy_context = sc;

 if(ctx->local)
  ctx->service_list_builds_strategy = s;
 else
  ctx->build_list_strategy = s;

 ctx->process_build_strategy = process_build_strategy_console;

 return 0;
}

