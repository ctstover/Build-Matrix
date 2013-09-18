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

#ifndef _PROTOTYPES_H_
#define _PROTOTYPES_H_
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#define BLDMTRX_MODE_PARSE        01
#define BLDMTRX_MODE_SERVE        10
#define BLDMTRX_MODE_INIT         11
#define BLDMTRX_MODE_ADD_USER     15
#define BLDMTRX_MODE_DEL_USER     16
#define BLDMTRX_MODE_MOD_USER     17
#define BLDMTRX_MODE_CONNECTIONS  18
#define BLDMTRX_MODE_LIST_USERS   19
#define BLDMTRX_MODE_ADD_ALIAS    20
#define BLDMTRX_MODE_DEL_ALIAS    21
#define BLDMTRX_MODE_LIST_ALIASES 22
#define BLDMTRX_MODE_LIST_VARIABLES 23
#define BLDMTRX_MODE_GEN_REPORT   30

#define BLDMTRX_MODE_RECONFIGURE    99
#define BLDMTRX_MODE_CONNECT        100

/* modes > 100 require client to connect to a server */
#define BLDMTRX_MODE_SUBMIT         101
#define BLDMTRX_MODE_PULL           102
#define BLDMTRX_MODE_SCRATCH        103
#define BLDMTRX_MODE_LIST_JOBS      104
#define BLDMTRX_MODE_LIST_BRANCHES  105
#define BLDMTRX_MODE_LIST_HOSTS     106
#define BLDMTRX_MODE_LIST_BUILDS    107
#define BLDMTRX_MODE_SHOW_BUILD     108
#define BLDMTRX_MODE_LIST_TESTS     109

#define BLDMTRX_ACCESS_BIT_SUBMIT  1
#define BLDMTRX_ACCESS_BIT_GET     2
#define BLDMTRX_ACCESS_BIT_LIST    4
#define BLDMTRX_ACCESS_BIT_SCRATCH 8

struct test
{
 int result;
 char *name;
 char *data;
};

struct suite
{
 char *name;
 int n_tests;
 struct test *tests;
};

struct test_results
{
 struct suite *suites;
 int n_suites;
 int allocation_size, n_tests, string_space;
};

struct parameters
{
 char **keys;
 char **values;
 int n_pairs, string_space;
};

struct buildmatrix_context;

struct iterative_strategy
{
 int (*start) (const struct buildmatrix_context *ctx, void * const strategy_context, const void *ptr);
 int (*iterative) (const struct buildmatrix_context *ctx, void * const strategy_context, const void *ptr);
 int (*done) (const struct buildmatrix_context *ctx, void * const strategy_context, const void *ptr);
 void *strategy_context;
};

struct list_builds_data
{
 int build_id;
 const char *branch;
 const char *job_name;
 const char *build_node;
 const char *revision;
 const char *user;
 const char *unique_identifier;
 const char *report_name;
 const char *output_name;
 const char *checksum_name;
 int build_result;
 long long int build_time;
 int passed;
 int failed;
 int incompleted;
 int has_parameters;
 int has_tests; 
};

struct buildmatrix_context
{
 /* general */
 int verbose;
 int quiet;
 int force;
 int non_interactive;
 int mode, local;
 int (*yes_no_prompt) (struct buildmatrix_context *, char *);
 char *current_working_directory;

 /* protocol related */
 char *job_name;
 char *branch_name;
 char *revision;
 int build_result;
 char *unique_identifier;
 int test_totals[4];
 char *build_report;
 char *build_output;
 char *test_results_file;
 char *parameters_file;
 char *checksum;
 char *build_node;
 char *project_name;
 struct test_results *test_results;
 struct parameters *parameters;
 int n_parameters;
 long long int build_time;
 char *user;
 char *email_address;
 int sub_dialog_mode;
 struct iterative_strategy list_ops_strategy;
 struct iterative_strategy build_list_strategy;
 int (*process_build_strategy) (struct buildmatrix_context * const);

 /* db related */
 int write_master;
 int user_mode_bits;
 int posix_uid;
 int user_id;
 char *alias;
 int grow_test_tables;
 int build_id;
 int pull_build_id;
 int db_ref_count;
 sqlite3 *db_ctx;
 char *local_project_directory;
 char *limbo_directory;
 struct iterative_strategy service_simple_lists_strategy;
 struct iterative_strategy service_list_builds_strategy;

 /* fork & ssh related */
 char *server_argument_vector[64];
 int n_server_arguments;
 pid_t child;
 int ssh;
 char *ssh_bin;
 char *server;
 char *remote_bin_path;
 char *remote_project_directory;
 int default_connection;

 /* network & io */
 int connection_initialized;
 int in;
 int out;
 char *error_prefix;

 /* output file */
 char *output_out_filename;
 char *report_out_filename;
 char *checksum_out_filename;
 int output_written;
 int report_written;
 int checksum_written;
};

/* protocol.c */
int send_line(const struct buildmatrix_context *ctx, int length, char *buffer);
int receive_line(struct buildmatrix_context *ctx, int size, int *length, char *buffer);
int send_test_results(struct buildmatrix_context *ctx, struct test_results *results);
int receive_test_results(struct buildmatrix_context *ctx, struct test_results **results);
int send_build_result(struct buildmatrix_context *ctx);
int send_branch_name(struct buildmatrix_context *ctx);
int send_revision(struct buildmatrix_context *ctx);
int send_build_host_name(struct buildmatrix_context *ctx);
int send_unique_identifier(struct buildmatrix_context *ctx);
int send_user_name(struct buildmatrix_context *ctx);
int send_job_name(struct buildmatrix_context *ctx);
int send_build_time(struct buildmatrix_context *ctx);
int send_checksum(struct buildmatrix_context *ctx, int from_db);
int send_build_output(struct buildmatrix_context *ctx, int from_db);
int send_build_report(struct buildmatrix_context *ctx, int from_db);
int send_test_totals(struct buildmatrix_context *ctx);
int get_job_script(struct buildmatrix_context *ctx);
int send_job_script(struct buildmatrix_context *ctx);
int edit_job_script(struct buildmatrix_context *ctx);
int start_build_submission(struct buildmatrix_context *ctx);
int service_input(struct buildmatrix_context *ctx);
int service_list_strategy_net_start(const struct buildmatrix_context *ctx, 
                                    void * const strategy_context, const void *ptr);
int service_list_strategy_net_iterative(const struct buildmatrix_context *ctx,
                                        void * const strategy_context, const void *ptr);
int service_list_strategy_net_done(const struct buildmatrix_context *ctx,
                                   void * const strategy_context, const void *ptr);

int service_list_builds_strategy_net_start(const struct buildmatrix_context *ctx,
                                           void * const strategy_context, const void *ptr);
int service_list_builds_strategy_net_iterative(const struct buildmatrix_context *ctx,
                                                void * const strategy_context, const void *ptr);
int service_list_builds_strategy_net_done(const struct buildmatrix_context *ctx,
                                          void * const strategy_context, const void *ptr);

/* setup.c */
struct buildmatrix_context *setup(int argc, char **argv);
void help(void);
int clean_state(struct buildmatrix_context *ctx);

/* files.c */
char *read_disk_file(struct buildmatrix_context *ctx, char *filename, size_t *filesize);
int remove_disk_file(struct buildmatrix_context *ctx, const char *filename);
int remove_db_file(struct buildmatrix_context *ctx, const char *filename);
int send_disk_file(struct buildmatrix_context *ctx, char *filename, char *filecontents);
int send_db_file(struct buildmatrix_context *ctx, char *filename);
int send_file_from_blob(struct buildmatrix_context *ctx, char *filename, char *blob, int size);
int receive_disk_file(struct buildmatrix_context *ctx, char **filename_ptr, int limbo);
int receive_file_as_blob(struct buildmatrix_context *ctx, char **filename_ptr, char **filecontents);
int limbo_to_database(struct buildmatrix_context *ctx, char *filename);
int limbo_cleanup(struct buildmatrix_context *ctx);
unsigned long long int file_size(struct buildmatrix_context *ctx, const char *filename);

/* client.c */
int start_server(struct buildmatrix_context *ctx);
int stop_server(struct buildmatrix_context *ctx);
int list_branches(struct buildmatrix_context *ctx);
int list_hosts(struct buildmatrix_context *ctx);
int list_jobs(struct buildmatrix_context *ctx);
int available_jobs(struct buildmatrix_context *ctx);
int submit(struct buildmatrix_context *ctx, int from_server_to_client);
int scratch(struct buildmatrix_context *ctx);
int list_builds(struct buildmatrix_context *ctx);
int list_tests(struct buildmatrix_context *ctx);
int show_build(struct buildmatrix_context *ctx);
int pull(struct buildmatrix_context *ctx);

/* server.c */
int serve(struct buildmatrix_context *ctx);
int service_list_tests(struct buildmatrix_context *ctx);
int rollback_submit(struct buildmatrix_context *ctx);
int service_submit(struct buildmatrix_context * const ctx);
int generate_unique_identifier(struct buildmatrix_context *ctx);

/* strings.c */
int check_string(struct buildmatrix_context *ctx, char *string);
int add_to_string_array(char ***array, int array_size, 
                        const char *string, int string_length,
                        int prevent_duplicates);
int free_string_array(char **array, int n_elements);
int file_size_to_string(unsigned long long int file_size, char *string, int size);

/* database.c */
int open_database(struct buildmatrix_context *ctx);
int close_database(struct buildmatrix_context *ctx);
int new_database(struct buildmatrix_context *ctx);
int ready_build_submission(struct buildmatrix_context *ctx);
int add_build(struct buildmatrix_context *ctx);
int lookup_build_id(struct buildmatrix_context *ctx);
int serivice_pull(struct buildmatrix_context *ctx);
int service_list_jobs(struct buildmatrix_context *ctx);
int service_list_branches(struct buildmatrix_context *ctx);
int service_list_hosts(struct buildmatrix_context *ctx);
int service_list_builds(struct buildmatrix_context *ctx);
int service_list_users(struct buildmatrix_context *ctx);
int service_list_parameters(struct buildmatrix_context *ctx);
int service_scratch(struct buildmatrix_context *ctx);
int service_get_build(struct buildmatrix_context *ctx);
int service_pull(struct buildmatrix_context *ctx);

/* connections.c */
int save_connection_details(struct buildmatrix_context *ctx);
int update_connection_details(struct buildmatrix_context *ctx);
int resolve_connection_details(struct buildmatrix_context *ctx);
int list_connections(struct buildmatrix_context *ctx);
int update_pull_build_id(struct buildmatrix_context *ctx);

/* testresults.c */
int load_test_results(struct buildmatrix_context *ctx, char *filename,
                      struct test_results **results);
int print_suite_array(struct buildmatrix_context *ctx, struct test_results *results);
int export_test_results_bldmtrx_format(struct buildmatrix_context *ctx, 
                                       struct test_results *results, char *filename);
int file_test_results(struct buildmatrix_context *ctx, struct test_results *results);
int pull_test_results(struct buildmatrix_context *ctx, struct test_results **results);

/* parameters.c */
int parameters_from_environ(struct buildmatrix_context *ctx);
int print_parameters(struct buildmatrix_context *ctx);
int parameters_to_file(struct buildmatrix_context *ctx, char *filename);
int parameters_from_file(struct buildmatrix_context *ctx, char *filename);
int send_parameters(struct buildmatrix_context *ctx);
int receive_parameters(struct buildmatrix_context *ctx);
int load_parameters_from_db(struct buildmatrix_context *ctx);
int save_parameters_to_db(struct buildmatrix_context *ctx);

/* console */
int setup_console(struct buildmatrix_context * const ctx);

/* portability.c */
int build_matrix_md5_wrapper(const struct buildmatrix_context *ctx,
                             const unsigned char *input, unsigned int length,
                             unsigned char *output);

/* users.c */
int resolve_user_id(struct buildmatrix_context *ctx);
int add_user(struct buildmatrix_context *ctx);
int advanced_security_check(struct buildmatrix_context *ctx);
int authorize_submit(struct buildmatrix_context *ctx);
int authorize_scratch(struct buildmatrix_context *ctx);
int authorize_list(struct buildmatrix_context *ctx);
int authorize_get(struct buildmatrix_context *ctx);
int get_mode_bitmask(struct buildmatrix_context *ctx, int *mode);
int derive_mode_bitmask_for_new_user(struct buildmatrix_context *ctx, int *mode);
int list_users(struct buildmatrix_context *ctx);
int mod_user(struct buildmatrix_context *ctx);
int remove_user(struct buildmatrix_context *ctx);
int add_alias(struct buildmatrix_context *ctx);
int remove_alias(struct buildmatrix_context *ctx);
int list_aliases(struct buildmatrix_context *ctx);

/* configuration.c */
char *resolve_configuration_value(struct buildmatrix_context *ctx, char *key);
int save_configurtation_value(struct buildmatrix_context *ctx, char *parameter,
                              char *value, int overwrite);
int list_configurtation_values(struct buildmatrix_context *ctx);

/* report.c */
int generate_report(struct buildmatrix_context *ctx);
struct list_builds_data *copy_list_build_data(const struct buildmatrix_context *const ctx,
                                              const struct list_builds_data * const source);
int append_list_build_data_array(const struct buildmatrix_context *ctx,
                                 const struct list_builds_data *source, 
                                 struct list_builds_data *** const array_ptr,
                                 int * const length, int * const size);

#endif

