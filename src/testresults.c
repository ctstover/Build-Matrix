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

int load_test_results(struct buildmatrix_context *ctx, char *filename,
                      struct test_results **results)
{
 char *file_ctxtents, *suite_name;
 struct test_results *r;
 struct test *t;
 struct suite *s, *suites;
 int n_suites = 0, n_tests = 0, line_length, line_number, handled, code, pass = 1;
 size_t allocation_size = 0, filesize, index;
 char temp[1024];
 int test_names_array_size = 0, mark, markB, result, x;
 char **suite_names_array = NULL, **test_names_array = NULL;
 void *base_ptr = NULL, *tests_base_ptr, *strings_base_ptr; 

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: load_test_results()\n", ctx->error_prefix);
 
 if((file_ctxtents = read_disk_file(ctx, filename, &filesize)) == NULL)
 {
  return 1;
 }

 while(pass < 3)
 {
  index = 0;
  if(pass == 2)
  {
   free_string_array(suite_names_array, n_suites);
   free_string_array(test_names_array, test_names_array_size);
   suite_names_array = NULL;
   test_names_array = NULL;

   allocation_size += sizeof(struct test_results);

   if((base_ptr = malloc(allocation_size)) == NULL)
   {
    fprintf(stderr, "%s: malloc() failed\n", ctx->error_prefix);
    free(file_ctxtents);
    return 1;    
   }

   r = (struct test_results *) base_ptr;
   base_ptr += sizeof(struct test_results);
   r->allocation_size = allocation_size;
   r->n_suites = n_suites;
   r->n_tests = test_names_array_size;

   suites = (struct suite *) base_ptr;
   tests_base_ptr = base_ptr + (sizeof(struct suite) * n_suites);
   strings_base_ptr = tests_base_ptr + (sizeof(struct test) * test_names_array_size);
   n_suites = 0;
   n_tests = 0;

   r->suites = suites;
   r->string_space = (base_ptr + allocation_size) - strings_base_ptr;
  }

  line_number = 0;
  while(index < filesize)
  {
   line_length = 0;
   line_number++;
   while(1)
   {
    if(index + line_length == filesize)
    {
     line_length--;
     break;
    }

    if(file_ctxtents[index + line_length] == '\n')
     break;
  
    line_length++;
   }

   if(line_length < 9)
   {
    break;
   }

   handled = 0;

   if(strncmp(file_ctxtents + index, "suite: ", 7) == 0)
   {
    memcpy(temp, file_ctxtents + index + 7, line_length - 7);
    temp[line_length - 7] = 0;

    if(pass == 1)
    {
     if(check_string(ctx, temp))
     {
      fprintf(stderr, 
              "%s: file %s, line %d: string validation error\n", 
             ctx->error_prefix, filename, line_number);
      free(file_ctxtents);
      return 1;
     }

     if((code = add_to_string_array(&suite_names_array, n_suites, 
                                    temp, line_length - 7, 1)) == -1)
     {
      fprintf(stderr, "%s: add_to_string_array() failed\n", 
             ctx->error_prefix);
      free(file_ctxtents);
      return 1;
     }

     if(code == 0)
     {
      suite_name = suite_names_array[n_suites++];
      allocation_size += sizeof(struct suite);
      allocation_size += line_length - 6;
     } else {
      fprintf(stderr, 
              "%s: file %s, line %d: suite \"%s\" has already been listed.\n", 
              ctx->error_prefix, filename, line_number, temp);
      free(file_ctxtents);
      return 1;
     }
    } else {
     x=0;
     s = NULL;

     while(x<n_suites)
     {
      if(strcmp(temp, suites[x].name) == 0)
       s = &(suites[x]);
      x++;
     }
     if(s == NULL)
     {
      s = &(suites[n_suites++]);
      s->name = (char *) strings_base_ptr;
      strings_base_ptr += line_length - 6;
      snprintf(s->name, line_length - 6, "%s", temp);
      s->n_tests = 0;
      s->tests = (struct test *) tests_base_ptr;
     }

    }
    handled = 1;
   }

   if(strncmp(file_ctxtents + index, "test: ", 6) == 0)
   {
    /* test: testname: result[: data] */
    mark = -1;
    markB = -1;
    x = 6;
    while(x < line_length)
    {
     if(file_ctxtents[index + x] == ':')
     {
      if(mark == -1)
      {
       mark = x;
      } else {
       markB = x;
       break;
      }
     }
     x++;
    }
    if(mark == -1)
    {
     file_ctxtents[index + line_length] = 0;
     fprintf(stderr, 
             "%s: file %s, line %d: expected ':' after test name, \"%s\"\n", 
            ctx->error_prefix, filename, line_number, file_ctxtents + index);
     if(base_ptr != NULL)
      free(base_ptr);
     free(file_ctxtents);
     return 1;
    }

    if(markB == -1)
     markB = line_length;

    if(pass == 1)
    {
     x = snprintf(temp, 1024, "%s.", suite_name);
     memcpy(temp + x, file_ctxtents + index + 6, mark - 6);
     temp[x + mark - 6] = 0;

     if(check_string(ctx, temp))
     {
      fprintf(stderr, 
              "%s: file %s, line %d: string validation error\n", 
             ctx->error_prefix, filename, line_number);
      if(base_ptr != NULL)
       free(base_ptr);
      free(file_ctxtents);
      return 1;
     }

     if((code = add_to_string_array(&test_names_array, test_names_array_size, 
                                    temp, x + mark - 6, 1)) == -1)
     {
      fprintf(stderr, "%s: add_to_string_array() failed\n", 
             ctx->error_prefix);
      if(base_ptr != NULL)
       free(base_ptr);
      free(file_ctxtents);
       return 1;
     }

     if(code == 1)
     {
      fprintf(stderr, 
              "%s: file %s, line %d: results for test %s were already given\n", 
             ctx->error_prefix, filename, line_number, temp);
      return 1;
     }

     allocation_size += (mark - 6) + 1;
     allocation_size += line_length - markB;
     allocation_size += sizeof(struct test);
     test_names_array_size++;
    } 

    result = -1;

    if(strncmp(file_ctxtents + index + mark + 2, "passed", 6) == 0)
    {
     result = 1;
    }
   
    if(strncmp(file_ctxtents + index + mark + 2, "failed", 6) == 0)
    {
     result = 2;
    }

    if(strncmp(file_ctxtents + index + mark + 2, "incomplete", 10) == 0)
    {
     result = 3;
    }

    if(result == -1)
    {
     file_ctxtents[index + line_length] = 0;
     fprintf(stderr, 
             "%s: file %s, line %d: results for test %s not understood, \"%s\"\n", 
             ctx->error_prefix, filename, line_number, temp, file_ctxtents + index + mark + 2);

     free(file_ctxtents);
     return 1;
    }

    if( (mark - 6) > 256)
    {
     fprintf(stderr, 
             "%s: file %s, line %d: test name length is limited to 256 bytes\n", 
             ctx->error_prefix, filename, line_number);

     free(file_ctxtents);
     return 1;
    }

    if( (line_length - markB) > 1024)
    {
     fprintf(stderr, 
             "%s: file %s, line %d: test data is limited to 1024 bytes\n", 
             ctx->error_prefix, filename, line_number);

     free(file_ctxtents);
     return 1;
    }

    if(pass == 2)
    {
     t = &(s->tests[s->n_tests++]);

     t->name = (char *) strings_base_ptr;
     strings_base_ptr += mark + 1 - 6;
     memcpy(t->name, file_ctxtents + index + 6, mark - 6);
     t->name[mark - 6] = 0;
     t->result = result;

     if((line_length - markB) == 0)
     {
      t->data = NULL;
     } else {
      t->data = (char *) strings_base_ptr;
      strings_base_ptr += line_length - markB - 1;
      memcpy(t->data, file_ctxtents + index + markB + 2, line_length - markB);
      t->data[line_length - markB - 2] = 0;
     }
    }

    handled = 1;
   }

   if(handled == 0)
   {
    file_ctxtents[index + 9] = 0;
    fprintf(stderr, 
            "%s: file %s, line %d: expecting line to start with \"suite: \", or \"test: \", "
            "instead of, \"%s\"\n", 
           ctx->error_prefix, filename, line_number, file_ctxtents + index);
     if(base_ptr != NULL)
      free(base_ptr);
    free(file_ctxtents);
    return 1;
   }
  
   index += line_length + 1;
  }

  pass++;
 }

 *results = r;

 return 0;
}

int print_suite_array(struct buildmatrix_context *ctx, struct test_results *results)
{
 int x, y;
 struct test *t;
 struct suite *s;

 if(results == NULL)
 {
  fprintf(stderr, "print_suite_array(NULL)\n");
  return 0;
 } else {
  fprintf(stderr, "print_suite_array(%d)\n", results->n_suites);
 }

 for(x=0; x<results->n_suites; x++)
 {
  s = &(results->suites[x]);
  fprintf(stderr, "%s: (%d/%d) suite=\"%s\", n_tests=%d\n", 
          ctx->error_prefix, x, results->n_suites, s->name, s->n_tests);
  for(y=0; y<s->n_tests; y++)
  {
   t = &(s->tests[y]);

   fprintf(stderr, "%s: suite=\"%s\", test=\"%s\", result=%d, data=\"%s\"\n",
           ctx->error_prefix, s->name, t->name, t->result, t->data);
  }
 }
 
 return 0;
}

int send_test_results(struct buildmatrix_context *ctx, struct test_results *results)
{
 int n_bytes, buffer_length, suite_i, test_i;
 char buffer[2048];

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: send_test_results()\n", ctx->error_prefix);

 if(results == NULL)
  return 0;

 if(ctx->verbose)
  fprintf(stderr, "%s: sending test results\n", ctx->error_prefix);

 while(1)
 {
  buffer_length = 
  snprintf(buffer, 512, "test results: %d %d %d\n",
           results->n_suites, results->n_tests, results->string_space);

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
   fprintf(stderr, "%s: peer rejected test results\n",
           ctx->error_prefix);
   return 1;
  }

  if(strncmp(buffer, "ok", 2) == 0)
  {
   break;
  }

  fprintf(stderr, "%s: unexpected response to test restults, \"%s\"\n",
          ctx->error_prefix, buffer);

  return 1;
 }

 for(suite_i = 0; suite_i < results->n_suites; suite_i++)
 {
  buffer_length = 
  snprintf(buffer, 512, "suite|%s\n", results->suites[suite_i].name);

  if(send_line(ctx, buffer_length, buffer))
   return 1;

  if(receive_line(ctx, 32, &n_bytes, buffer))
   return 1;

  buffer[n_bytes] = 0;
  if(strncmp(buffer, "ok", 2) != 0)
  {
   fprintf(stderr, "%s: response durring results transmission was not ok, \"%s\"\n",
           ctx->error_prefix, buffer);
   return 1;
  }

  for(test_i = 0; test_i < results->suites[suite_i].n_tests; test_i++)
  {
   buffer_length = 
   snprintf(buffer, 2048, "test|%s|%d|%s\n", 
            results->suites[suite_i].tests[test_i].name,
            results->suites[suite_i].tests[test_i].result,
            (results->suites[suite_i].tests[test_i].data == NULL) ? "" :
            results->suites[suite_i].tests[test_i].data);

   if(send_line(ctx, buffer_length, buffer))
    return 1;

   if(receive_line(ctx, 32, &n_bytes, buffer))
    return 1;

   buffer[n_bytes] = 0;
   if(strncmp(buffer, "ok", 2) != 0)
   {
    fprintf(stderr, "%s: response durring results transmission was not ok, \"%s\"\n",
            ctx->error_prefix, buffer);
    return 1;
   }
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
   fprintf(stderr, "%s: test results sent.\n", ctx->error_prefix);

  return 0;
 }

 fprintf(stderr, "%s: unexpected response to test restults, \"%s\"\n",
         ctx->error_prefix, buffer);
 return 1;
}

int receive_test_results(struct buildmatrix_context *ctx, struct test_results **results)
{
 int allocation_size, n_suites, n_tests, string_space, buffer_length, 
     suite_i, test_i, handled, i, marks[3], n_marks;
 struct test_results *r;
 char buffer[2048], temp[8];
 struct suite *s;
 struct test *t;
 void *base_ptr, *tests_base_ptr, *strings_base_ptr;

 if(ctx->verbose)
  fprintf(stderr, "%s: receiving test results\n", ctx->error_prefix);

 if(receive_line(ctx, 512, &buffer_length, buffer))
  return 1;

 if(strncmp(buffer, "test results: ", 14) != 0)
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: expected test results SOT, not \"%s\"\n", ctx->error_prefix, buffer);

  buffer_length = snprintf(buffer, 512, "failed\n");
  send_line(ctx, buffer_length, buffer);
  return 1;
 }

 sscanf(buffer + 13, "%d %d %d\n", &n_suites, &n_tests, &string_space);

 allocation_size = (sizeof(struct suite) * n_suites) +
                   (sizeof(struct test) * n_tests) +
                   sizeof(struct test_results) +
                   string_space;

 if(allocation_size < sizeof(struct test_results) + sizeof(struct suite) + sizeof(struct test))
 {
  buffer_length = 
  snprintf(buffer, 2048, "failed\n");

  if(ctx->verbose)
   fprintf(stderr, "%s: problem with test results SOT\n",
           ctx->error_prefix);

  send_line(ctx, buffer_length, buffer);
  return 1;
 }

 if((base_ptr = malloc(allocation_size)) == NULL)
 {
  buffer_length = 
  snprintf(buffer, 2048, "failed\n");

  if(ctx->verbose)
   fprintf(stderr, "%s: malloc(%d) failed\n",
           ctx->error_prefix, allocation_size);

  send_line(ctx, buffer_length, buffer);
  return 1;
 }
 memset(base_ptr, 0, allocation_size);

 r = (struct test_results *) base_ptr;
 base_ptr += sizeof(struct test_results);
 r->allocation_size = allocation_size;
 r->n_suites = 0;
 r->n_tests = 0;
 s = (struct suite *) base_ptr;
 tests_base_ptr = base_ptr + (sizeof(struct suite) * n_suites);
 strings_base_ptr = tests_base_ptr + (sizeof(struct test) * n_tests);
 suite_i = -1;
 r->suites = s;
 r->string_space = string_space;
 string_space = 0;

 buffer_length = snprintf(buffer, 512, "ok\n");

 if(send_line(ctx, buffer_length, buffer))
 {
  free(r);
  return 1;
 }

 while(1)
 {
  if(receive_line(ctx, 512, &buffer_length, buffer))
  {
   free(r);
   return 1;
  }
  buffer[buffer_length] = 0;
  handled = 0;

  if(strncmp(buffer, "done", 4) == 0)
  {
   buffer_length = snprintf(buffer, 512, "ok\n");

   if(send_line(ctx, buffer_length, buffer))
   {
    free(r);
    return 1;
   }

   if(ctx->verbose)
    fprintf(stderr, "%s: test results received\n",
            ctx->error_prefix);

   *results = r;
   break;
  }

  if(strncmp(buffer, "suite|", 6) == 0)
  {
   handled = 1;
   if(check_string(ctx, buffer + 7))
   {
    buffer_length = snprintf(buffer, 2048, "failed\n");

    if(ctx->verbose)
     fprintf(stderr, "%s: invalid suite name, \"%s\"\n",
             ctx->error_prefix, buffer + 7);

    send_line(ctx, buffer_length, buffer);
    free(r);
    return 1;
   }

   if(r->n_suites + 1 > n_suites)
   {
    buffer_length = snprintf(buffer, 2048, "failed\n");

    if(ctx->verbose)
     fprintf(stderr, "%s: SOT only specified %d suites\n", ctx->error_prefix, n_suites);

    send_line(ctx, buffer_length, buffer);
    free(r);
    return 1;
   }

   if(r->string_space < string_space + (buffer_length - 6))
   {
    buffer_length = snprintf(buffer, 2048, "failed\n");

    if(ctx->verbose)
     fprintf(stderr, "%s: SOT only specified %d string space, I need at least %d.\n",
             ctx->error_prefix, r->string_space + (buffer_length - 6), string_space);

    send_line(ctx, buffer_length, buffer);
    free(r);
    return 1;
   }

   suite_i++;
   test_i = 0;
   r->n_suites++;
   s = &(r->suites[suite_i]);
   s->name = (char *) strings_base_ptr + string_space;
   string_space += (buffer_length - 6);
   snprintf(s->name, buffer_length - 5, "%s", buffer + 6);
   s->tests = (struct test *) tests_base_ptr;
  }

  if(strncmp(buffer, "test|", 5) == 0)
  {
   handled = 1;

   if(r->n_tests + 1 > n_tests)
   {
    buffer_length = snprintf(buffer, 2048, "failed\n");

    if(ctx->verbose)
     fprintf(stderr, "%s: SOT only specified %d tests\n", ctx->error_prefix, n_tests);

    send_line(ctx, buffer_length, buffer);
    free(r);
    return 1;
   }

   t = &(s->tests[test_i++]);
   tests_base_ptr += sizeof(struct test);
   s->n_tests++;

   n_marks = 0;
   i = 4;
   while(i < buffer_length)
   {
    if(buffer[i] == '|')
    {
     if(n_marks == 3)
     {
      buffer_length = snprintf(buffer, 2048, "failed\n");
 
      if(ctx->verbose)
       fprintf(stderr, "%s: bad format of test line\n", ctx->error_prefix);

      send_line(ctx, buffer_length, buffer);
      free(r);
      return 1;
     }

     marks[n_marks++] = i;
    }
    i++;
   }

   if(n_marks != 3)
   {
    if(ctx->verbose)
     fprintf(stderr, "%s: malformed test line, \"%s\"\n", ctx->error_prefix, buffer);

    buffer_length = snprintf(buffer, 2048, "failed\n");

    send_line(ctx, buffer_length, buffer);
    free(r);
    return 1;
   }

   i = marks[2] - (marks[1] + 1);
   if( (i < 1) || (i > 3) )
   {
    buffer_length = snprintf(buffer, 2048, "failed\n");
 
    if(ctx->verbose)
     fprintf(stderr, "%s: test line result code field wrong length\n", ctx->error_prefix);

    send_line(ctx, buffer_length, buffer);
    free(r);
    return 1;
   }

   memcpy(temp, buffer + marks[1] + 1, i);
   temp[i] = 0;
   sscanf(temp, "%d", &t->result);

   if( (t->result < 1) || (t->result > 3) )
   {
    if(ctx->verbose)
     fprintf(stderr, "%s: invalid test result code %d, from field '%s' in line '%s'\n", 
             ctx->error_prefix, t->result, temp, buffer);

    buffer_length = snprintf(buffer, 2048, "failed\n");
    send_line(ctx, buffer_length, buffer);
    free(r);
    return 1;
   }

   i = marks[1] - (marks[0] + 1);
   if( (i < 1) || (i > 256) )
   {
    buffer_length = snprintf(buffer, 2048, "failed\n");
 
    if(ctx->verbose)
     fprintf(stderr, "%s: test line name field wrong length\n", ctx->error_prefix);

    send_line(ctx, buffer_length, buffer);
    free(r);
    return 1;
   }

   if(r->string_space < string_space + i)
   {
    buffer_length = snprintf(buffer, 2048, "failed\n");

    if(ctx->verbose)
     fprintf(stderr, "%s: SOT only specified %d string space, I need at least %d\n",
             ctx->error_prefix, r->string_space, string_space + i);

    send_line(ctx, buffer_length, buffer);
    free(r);
    return 1;
   }

   t->name = (char *) strings_base_ptr + string_space;
   string_space += i + 1; 
   memcpy(t->name, buffer + marks[0] + 1, i);
   t->name[i] = 0;

   if(check_string(ctx, t->name))
   {
    buffer_length = snprintf(buffer, 2048, "failed\n");

    if(ctx->verbose)
     fprintf(stderr, "%s: invalid test name, \"%s\"\n",
             ctx->error_prefix, t->name);

    send_line(ctx, buffer_length, buffer);
    free(r);
    return 1;
   }

   i = buffer_length - (marks[2] + 2);
   if(i > 1024)
   {
    buffer_length = snprintf(buffer, 2048, "failed\n");
 
    if(ctx->verbose)
     fprintf(stderr, "%s: test line data field too long\n", ctx->error_prefix);

    send_line(ctx, buffer_length, buffer);
    free(r);
    return 1;
   }

   if(i == 0)
   {
    t->data = NULL;
   } else {

    if(r->string_space < string_space + i + 1)
    {
     buffer_length = snprintf(buffer, 2048, "failed\n");

     if(ctx->verbose)
      fprintf(stderr, "%s: SOT only specified %d string space, I need at least %d\n",
              ctx->error_prefix, r->string_space, string_space  + i);

     send_line(ctx, buffer_length, buffer);
     free(r);
     return 1;
    }

    t->data = (char *) strings_base_ptr + string_space;
    string_space += (i + 1);
    memcpy(t->data, buffer + marks[2] + 1, i);
    t->data[i] = 0;

   }
  }

  if(handled == 0)
  {
   buffer_length = 
   snprintf(buffer, 2048, "failed\n");

   if(ctx->verbose)
    fprintf(stderr, "%s: unrecognized results transmission line, \"%s\"\n",
            ctx->error_prefix, buffer);

   send_line(ctx, buffer_length, buffer);
   free(r);
   return 1;
  }

  if(send_line(ctx, 3, "ok\n"))
   return 1;
 }

 return 0;
}

int export_test_results_bldmtrx_format(struct buildmatrix_context *ctx, 
                                       struct test_results *results, char *filename)
{
 FILE *output;
 int x, y;
 struct test *t;
 struct suite *s;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: export_test_results_bldmtrx_format()\n", ctx->error_prefix);

 if(results == NULL)
  return 1;
 
 if((output = fopen(filename, "w")) == NULL)
 {
  fprintf(stderr, "%s: fopen(%s, \"w\") failed: %s\n", 
          ctx->error_prefix, filename, strerror(errno));
  return 1;
 } 


 for(x=0; x<results->n_suites; x++)
 {
  s = &(results->suites[x]);

  fprintf(output, "suite: %s\n", s->name);

  for(y=0; y<s->n_tests; y++)
  {
   t = &(s->tests[y]);

   fprintf(output, "test: %s: ", t->name);
   switch(t->result)
   {
    case 1:
         fprintf(output, "passed");
         break;

    case 2:
         fprintf(output, "fail");
         break;

    case 3:
         fprintf(output, "incomplete");
         break;


    default:
         fclose(output);
         fprintf(stderr, "%s: export_test_results_bldmtrx_format(): "
                 "suite %s, test %s has invalid result value %d\n", 
                 ctx->error_prefix, s->name, t->name, t->result);
         return 1;

   }

   if(t->data != NULL)
   {
    fprintf(output, ": %s", t->data);
   }

   fprintf(output, "\n");
  }
 }
 
 fclose(output);
 return 0;
}

int file_test_results(struct buildmatrix_context *ctx, struct test_results *results)
{
 int suite_i, test_i, code, suite_id, test_id;
 char *sql;
 struct sqlite3_stmt *statement = NULL;

 if(ctx->verbose > 1)
  fprintf(stderr, 
          "%s: file_test_results()\n", ctx->error_prefix);

 if(ctx->build_id < 1)
 {
  fprintf(stderr, "%s: file_test_results(): I don't have a build id.\n",
          ctx->error_prefix);
  return 1;
 }

 for(suite_i = 0; suite_i < results->n_suites; suite_i++)
 {
  sql = "SELECT suite_id FROM suites WHERE name = ?;";

  if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
  {
   fprintf(stderr, "%s: file_test_results(): sqlite3_prepare(%s) failed, '%s'\n",
           ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx)); 
   return 1;
  }

  if(sqlite3_bind_text(statement, 1, results->suites[suite_i].name, 
                       strlen(results->suites[suite_i].name), 
                       SQLITE_TRANSIENT) != SQLITE_OK)
  {
   fprintf(stderr, "%s: file_test_results(): sqlite3_bind_text() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }

  suite_id = -1;

  while((code = sqlite3_step(statement)) == SQLITE_ROW)
  {

   if(sqlite3_column_type(statement, 0) != SQLITE_INTEGER)
   {
    fprintf(stderr, "%s: file_test_results(): suites.suite_id is not an integer\n",
            ctx->error_prefix);
    sqlite3_finalize(statement);
    return 1;
   }
   suite_id = sqlite3_column_int(statement, 0);
  }

  if(code != SQLITE_DONE)
  {
   fprintf(stderr, "%s: file_test_results(): sqlite3_step() failed, '%s'\n",
           ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
   sqlite3_finalize(statement);
   return 1;
  }

  sqlite3_finalize(statement);

  if(suite_id == -1)
  {
   if(ctx->verbose > 1)
   fprintf(stderr, 
           "%s: file_test_results(): suite '%s' not in suites table, adding...\n", 
           ctx->error_prefix, results->suites[suite_i].name);

   sql = "INSERT INTO suites (name) VALUES (?);";

   if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
   {
    fprintf(stderr, "%s: file_test_results(): sqlite3_prepare(%s) failed, '%s'\n",
            ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx)); 
    return 1;
   }

   if(sqlite3_bind_text(statement, 1, results->suites[suite_i].name, 
                        strlen(results->suites[suite_i].name), 
                        SQLITE_TRANSIENT) != SQLITE_OK)
   {
    fprintf(stderr, "%s: file_test_results(): sqlite3_bind_text() failed, '%s'\n",
            ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
    sqlite3_finalize(statement);
    return 1;
   }

   code = sqlite3_step(statement);
   if(code != SQLITE_DONE)
   {
    fprintf(stderr, "%s: file_test_results(): sqlite3_step() failed, '%s'\n",
            ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
    sqlite3_finalize(statement);
    close_database(ctx);
    return 1;
   }

   sqlite3_finalize(statement);

   sql = "SELECT suite_id FROM suites WHERE name = ?;";

   if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
   {
    fprintf(stderr, "%s: file_test_results(): sqlite3_prepare(%s) failed, '%s'\n",
            ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx)); 
    return 1;
   }

   if(sqlite3_bind_text(statement, 1, results->suites[suite_i].name, 
                        strlen(results->suites[suite_i].name), 
                        SQLITE_TRANSIENT) != SQLITE_OK)
   {
    fprintf(stderr, "%s: file_test_results(): sqlite3_bind_text() failed, '%s'\n",
            ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
    sqlite3_finalize(statement);
    return 1;
   }

   while((code = sqlite3_step(statement)) == SQLITE_ROW)
   {

    if(sqlite3_column_type(statement, 0) != SQLITE_INTEGER)
    {
     fprintf(stderr, "%s: file_test_results(): suites.suite_id is not an integer\n",
             ctx->error_prefix);
     sqlite3_finalize(statement);
     return 1;
    }
    suite_id = sqlite3_column_int(statement, 0);
   }

   if(code != SQLITE_DONE)
   {
    fprintf(stderr, "%s: file_test_results(): sqlite3_step() failed, '%s'\n",
            ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
    sqlite3_finalize(statement);
    return 1;
   }

   sqlite3_finalize(statement);

   if(suite_id == -1)
   {
    fprintf(stderr, "%s: file_test_results(): failed\n", ctx->error_prefix); 
    return 1;
   }

   if(ctx->verbose > 1)
   fprintf(stderr, 
           "%s: file_test_results(): inserted suite '%s' now suite_id %d\n", 
           ctx->error_prefix, results->suites[suite_i].name, suite_id);
  }


  for(test_i = 0; test_i < results->suites[suite_i].n_tests; test_i++)
  {
   test_id = -1;
   sql = "SELECT test_id FROM tests WHERE suite_id = ? AND name = ?;";

   if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
   {
    fprintf(stderr, "%s: file_test_results(): sqlite3_prepare(%s) failed, '%s'\n",
            ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx)); 
    return 1;
   }

   if(sqlite3_bind_int(statement, 1, suite_id) != SQLITE_OK)
   {
    fprintf(stderr, "%s: file_test_results(): sqlite3_bind_int() failed, '%s'\n",
            ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
    sqlite3_finalize(statement);
    return 1;
   }

   if(sqlite3_bind_text(statement, 2, results->suites[suite_i].tests[test_i].name, 
                        strlen(results->suites[suite_i].tests[test_i].name), 
                        SQLITE_TRANSIENT) != SQLITE_OK)
   {
    fprintf(stderr, "%s: file_test_results(): sqlite3_bind_text() failed, '%s'\n",
            ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
    sqlite3_finalize(statement);
    return 1;
   }

   while((code = sqlite3_step(statement)) == SQLITE_ROW)
   {
    if(sqlite3_column_type(statement, 0) != SQLITE_INTEGER)
    {
     fprintf(stderr, "%s: file_test_results(): tests.test_id is not an integer\n", 
             ctx->error_prefix);
     sqlite3_finalize(statement);
     return 1;
    }
    test_id = sqlite3_column_int(statement, 0);

   }

   if(code != SQLITE_DONE)
   {
    fprintf(stderr, "%s: file_test_results(): sqlite3_step() failed, '%s'\n",
            ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
    sqlite3_finalize(statement);
    return 1;
   }

   sqlite3_finalize(statement);

   if(test_id == -1)
   {
    if(ctx->verbose > 1)
     fprintf(stderr, 
             "%s: file_test_results(): test '%s.%s' not in tests table, adding...\n", 
             ctx->error_prefix, results->suites[suite_i].name, 
             results->suites[suite_i].tests[test_i].name);

    sql = "INSERT INTO tests (suite_id, name) VALUES (?, ?);";

    if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
    {
     fprintf(stderr, "%s: file_test_results(): sqlite3_prepare(%s) failed, '%s'\n",
             ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx)); 
     return 1;
    }

    if(sqlite3_bind_int(statement, 1, suite_id) != SQLITE_OK)
    {
     fprintf(stderr, "%s: file_test_results(): sqlite3_bind_int() failed, '%s'\n",
             ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
     sqlite3_finalize(statement);
     return 1;
    }

    if(sqlite3_bind_text(statement, 2, results->suites[suite_i].tests[test_i].name, 
                         strlen(results->suites[suite_i].tests[test_i].name), 
                         SQLITE_TRANSIENT) != SQLITE_OK)
    {
     fprintf(stderr, "%s: file_test_results(): sqlite3_bind_text() failed, '%s'\n",
             ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
     sqlite3_finalize(statement);
     return 1;
    }

    code = sqlite3_step(statement);
    if(code != SQLITE_DONE)
    {
     fprintf(stderr, "%s: file_test_results(): sqlite3_step() failed, '%s'\n",
             ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
     sqlite3_finalize(statement);
     close_database(ctx);
     return 1;
    }

    sqlite3_finalize(statement);

    sql = "SELECT test_id FROM tests WHERE suite_id = ? AND name = ?;";

    if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
    {
     fprintf(stderr, "%s: file_test_results(): sqlite3_prepare(%s) failed, '%s'\n",
             ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx)); 
     return 1;
    }

    if(sqlite3_bind_int(statement, 1, suite_id) != SQLITE_OK)
    {
     fprintf(stderr, "%s: file_test_results(): sqlite3_bind_int() failed, '%s'\n",
             ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
     sqlite3_finalize(statement);
     return 1;
    }

    if(sqlite3_bind_text(statement, 2, results->suites[suite_i].tests[test_i].name, 
                         strlen(results->suites[suite_i].tests[test_i].name), 
                         SQLITE_TRANSIENT) != SQLITE_OK)
    {
     fprintf(stderr, "%s: file_test_results(): sqlite3_bind_text() failed, '%s'\n",
             ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
     sqlite3_finalize(statement);
     return 1;
    }

    while((code = sqlite3_step(statement)) == SQLITE_ROW)
    {
     if(sqlite3_column_type(statement, 0) != SQLITE_INTEGER)
     {
      fprintf(stderr, "%s: file_test_results(): tests.test_id is not an integer\n",
              ctx->error_prefix);
      sqlite3_finalize(statement);
      return 1;
     }
     test_id = sqlite3_column_int(statement, 0);
    }

    if(code != SQLITE_DONE)
    {
     fprintf(stderr, "%s: file_test_results(): sqlite3_step() failed, '%s'\n",
             ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
     sqlite3_finalize(statement);
     return 1;
    }

    sqlite3_finalize(statement);

    if(test_id == -1)
    {
     fprintf(stderr, "%s: file_test_results(): failed\n", ctx->error_prefix);
     return 1;
    }

    if(ctx->verbose > 1)
     fprintf(stderr, 
             "%s: file_test_results(): inserted %s.%s now test_id %d\n", ctx->error_prefix, 
             results->suites[suite_i].name, results->suites[suite_i].tests[test_i].name, test_id);
   }

   if(results->suites[suite_i].tests[test_i].data != NULL)
    sql = "INSERT INTO scores (build_id, test_id, result, data) VALUES (?, ?, ?, ?);";
   else
    sql = "INSERT INTO scores (build_id, test_id, result) VALUES (?, ?, ?);";

   if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
   {
    fprintf(stderr, "%s: file_test_results(): sqlite3_prepare(%s) failed, '%s'\n",
            ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx)); 
    return 1;
   }

   if(sqlite3_bind_int(statement, 1, ctx->build_id) != SQLITE_OK)
   {
    fprintf(stderr, "%s: file_test_results(): sqlite3_bind_int() failed, '%s'\n",
            ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
    sqlite3_finalize(statement);
    return 1;
   }

   if(sqlite3_bind_int(statement, 2, test_id) != SQLITE_OK)
   {
    fprintf(stderr, "%s: file_test_results(): sqlite3_bind_int() failed, '%s'\n",
            ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
    sqlite3_finalize(statement);
    return 1;
   }

   if(sqlite3_bind_int(statement, 3, results->suites[suite_i].tests[test_i].result) != SQLITE_OK)
   {
    fprintf(stderr, "%s: file_test_results(): sqlite3_bind_int() failed, '%s'\n",
            ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
    sqlite3_finalize(statement);
    return 1;
   }

   if(results->suites[suite_i].tests[test_i].data != NULL)
   {
    if(sqlite3_bind_text(statement, 4, results->suites[suite_i].tests[test_i].data, 
                          strlen(results->suites[suite_i].tests[test_i].data), 
                          SQLITE_TRANSIENT) != SQLITE_OK)
    {
     fprintf(stderr, "%s: file_test_results(): sqlite3_bind_text() failed, '%s'\n",
             ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
     sqlite3_finalize(statement);
     return 1;
    }
   }

   code = sqlite3_step(statement);
   if(code != SQLITE_DONE)
   {
    fprintf(stderr, "%s: file_test_results(): sqlite3_step() failed, '%s'\n",
            ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
    sqlite3_finalize(statement);
    close_database(ctx);
    return 1;
   }

   sqlite3_finalize(statement);
  }
 }

 return 0;
}

int pull_test_results(struct buildmatrix_context *ctx, struct test_results **results)
{
 const char *sql, *suite_name, *test_name, *data;
 char *last_suite_name;
 struct sqlite3_stmt *statement = NULL;
 int code, result, n_suites = 0, n_tests = 0, string_space = 0, length, next_suite;
 struct test_results *r;
 struct test *tests;
 size_t allocation_size;
 void *base_ptr = NULL;
 char *strings_base_ptr; 

 if(ctx->verbose)
  fprintf(stderr, "%s: pull_test_results()\n", ctx->error_prefix);

 if(ctx->build_id < 1)
 {
  fprintf(stderr, "%s: pull_test_results(): no build_id in context\n", ctx->error_prefix);
  return 1;
 }

 if(open_database(ctx))
  return 1;

 sql = "SELECT b.suite_name, b.test_name, scores.result, scores.data FROM scores JOIN "
           "(SELECT tests.test_id, suites.name AS suite_name, tests.name AS test_name from tests "
                  " JOIN suites ON tests.suite_id = suites.suite_id) AS b "
       "ON scores.test_id = b.test_id WHERE build_id = ?;"; 


 if(sqlite3_prepare_v2(ctx->db_ctx, sql, strlen(sql) + 1, &statement, NULL) != SQLITE_OK)
 {
  fprintf(stderr, "%s: pull_test_results(): sqlite3_prepare(%s) failed, '%s'\n",
          ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx)); 
  return 1;
 }

 if(sqlite3_bind_int(statement, 1, ctx->build_id) != SQLITE_OK)
 {
  fprintf(stderr, "%s: pull_test_results(): sqlite3_bind_int() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  return 1;
 }

 /* first pass just verifies types, and counts the space to allocate */
 last_suite_name = NULL;
 while((code = sqlite3_step(statement)) == SQLITE_ROW)
 {
  if(sqlite3_column_type(statement, 0) != SQLITE_TEXT)
  {
   fprintf(stderr, "%s: pull_test_results(): suites.name is not TEXT\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   close_database(ctx);
   return 1;
  }
  suite_name = (const char *) sqlite3_column_text(statement, 0);

  if(sqlite3_column_type(statement, 1) != SQLITE_TEXT)
  {
   fprintf(stderr, "%s: pull_test_results(): tests.name is not TEXT\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   close_database(ctx);
   return 1;
  }
  test_name = (const char *) sqlite3_column_text(statement, 1);

  if(sqlite3_column_type(statement, 2) != SQLITE_INTEGER)
  {
   fprintf(stderr, "%s: load_test_results(): tests.result is not an integer\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   return 1;
  }
  result = sqlite3_column_int(statement, 2);

  if( (result < 1) || (result > 3) )
  {
   fprintf(stderr, "%s: pull_test_results(): tests.result is unknown value\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   close_database(ctx);
   return 1;
  }

  if((code = sqlite3_column_type(statement, 3)) != SQLITE_NULL)
  {
   if(code != SQLITE_TEXT)
   {
    fprintf(stderr, "%s: pull_test_results(): tests.name is not TEXT\n",
            ctx->error_prefix);
    sqlite3_finalize(statement);
    close_database(ctx);
    return 1;
   }
   data = (const char *) sqlite3_column_text(statement, 3);
  } else {
   data = NULL;
  }

  string_space += strlen(test_name) + 1;
  n_tests++;

  if(data != NULL)
   string_space += strlen(data) + 1;

  if(last_suite_name == NULL)
  {
   last_suite_name = strdup(suite_name);
   n_suites++;
   string_space += strlen(suite_name) + 1;
  } else {
   if(strcmp(last_suite_name, suite_name) != 0)
   {
    free(last_suite_name);
    last_suite_name = strdup(suite_name);
    string_space += strlen(suite_name) + 1;
    n_suites++;
   }
  }
 }

 if(last_suite_name != NULL)
  free(last_suite_name);

 if(code != SQLITE_DONE)
 {
  fprintf(stderr, "%s: pull_test_results() sqlite3_step() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  close_database(ctx);
  return 1;
 }

 if(n_suites == 0)
 {
  if(ctx->verbose)
   fprintf(stderr, "%s: pull_test_results() no test scores found for:"
           " job \"%s\", branch \"%s\", unique identifier \"%s\", build_id %d\n",
           ctx->error_prefix, ctx->job_name, ctx->branch_name, ctx->unique_identifier, ctx->build_id);

  close_database(ctx);

  *results = NULL;

  return 0;
 }

 allocation_size = sizeof(struct suite) * n_suites;
 allocation_size += sizeof(struct test) * n_tests;
 allocation_size += string_space;
 allocation_size += sizeof(struct test_results);
 
 if((base_ptr = malloc(allocation_size)) == NULL)
 {
  fprintf(stderr, "%s: malloc(%d) failed: %s\n",
          ctx->error_prefix, (int) allocation_size, strerror(errno));
  sqlite3_finalize(statement);
  close_database(ctx);
  return 1;    
 }

 r = (struct test_results *) base_ptr;
 r->allocation_size = allocation_size;
 r->n_suites = n_suites;
 r->n_tests = n_tests;

 base_ptr += sizeof(struct test_results);
 r->suites = (struct suite *) base_ptr;

 base_ptr += (sizeof(struct suite) * n_suites);
 tests = (struct test *) base_ptr;

 base_ptr += (sizeof(struct test) * n_tests);
 strings_base_ptr = (char *) base_ptr;

 n_suites = 0;
 n_tests = 0;

 r->string_space = string_space;

 if(sqlite3_reset(statement) != SQLITE_OK)
 {
  fprintf(stderr, "%s: pull_test_results(): sqlite3_reset(%s) failed, '%s'\n",
          ctx->error_prefix, sql, sqlite3_errmsg(ctx->db_ctx)); 
  return 1;
 }

 last_suite_name = NULL;
 string_space = 0;
 while((code = sqlite3_step(statement)) == SQLITE_ROW)
 {
  suite_name = (const char *) sqlite3_column_text(statement, 0);
  test_name = (const char *) sqlite3_column_text(statement, 1);
  result = sqlite3_column_int(statement, 2);

  if(sqlite3_column_type(statement, 3) != SQLITE_NULL)
   data = (const char *) sqlite3_column_text(statement, 3);
  else
   data = NULL;

  next_suite = 0;
  if(last_suite_name == NULL)
  {
   last_suite_name = strdup(suite_name);
   next_suite = 1;
  } else {
   if(strcmp(last_suite_name, suite_name) != 0)
   {
    free(last_suite_name);
    last_suite_name = strdup(suite_name);
    n_suites++;
    next_suite = 1;
   }
  }

  if(next_suite)
  {
   length = strlen(suite_name) + 1;

   r->suites[n_suites].n_tests = 0;
   r->suites[n_suites].tests = &(tests[n_tests]);

   r->suites[n_suites].name = strings_base_ptr;
   strings_base_ptr += length;
   string_space += length;

   if(r->string_space < string_space)
   {
    fprintf(stderr, "%s: pull_test_results() string_space indicates problem\n",
            ctx->error_prefix);
    sqlite3_finalize(statement);
    close_database(ctx);
    return 1;
   }

   snprintf(r->suites[n_suites].name, length, "%s", suite_name);
  }

  length = strlen(test_name) + 1;
  r->suites[n_suites].tests[r->suites[n_suites].n_tests].result = result;
  r->suites[n_suites].tests[r->suites[n_suites].n_tests].name = strings_base_ptr;
  strings_base_ptr += length;
  string_space += length;

  if(r->string_space < string_space)
  {
   fprintf(stderr, "%s: pull_test_results() string_space indicates problem\n",
           ctx->error_prefix);
   sqlite3_finalize(statement);
   close_database(ctx);
   return 1;
  }

  snprintf(r->suites[n_suites].tests[r->suites[n_suites].n_tests].name, length, "%s", test_name);

  if(data == NULL)
  {
   r->suites[n_suites].tests[r->suites[n_suites].n_tests].data = NULL;
  } else {
   r->suites[n_suites].tests[r->suites[n_suites].n_tests].data = strings_base_ptr;
   length = strlen(data) + 1;
   strings_base_ptr += length;
   string_space += length;
  
   if(r->string_space < string_space)
   {
    fprintf(stderr, "%s: pull_test_results() string_space indicates problem\n",
            ctx->error_prefix);
    sqlite3_finalize(statement);
    close_database(ctx);
    return 1;
   }

   snprintf(r->suites[n_suites].tests[r->suites[n_suites].n_tests].data, length, "%s", data);
  }

  r->suites[n_suites].n_tests++;
  n_tests++;
 }

 if(code != SQLITE_DONE)
 {
  fprintf(stderr, "%s: pull_test_results() sqlite3_step() failed, '%s'\n",
          ctx->error_prefix, sqlite3_errmsg(ctx->db_ctx));
  sqlite3_finalize(statement);
  close_database(ctx);
  return 1;
 }

 sqlite3_finalize(statement);
 close_database(ctx);

 *results = r;

 return 0;
}

