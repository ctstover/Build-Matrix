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

int check_string(struct buildmatrix_context *ctx, char *string)
{
 int x = 0;

 if(string == NULL)
  return 0;

 while(string[x] != 0)
 {
  if(string[x] == '|')
  {
   fprintf(stderr, "%s: input string \"%s\" can not include \"|\" characters.\n", 
           ctx->error_prefix, string);
   return 1;
  }

  if(string[x] == ' ')
  {
   fprintf(stderr, "%s: input string \"%s\" can not include spaces.\n", 
           ctx->error_prefix, string);
   return 1;
  }

  x++;
 }

 return 0;
}

int add_to_string_array(char ***array, int array_size, 
                        const char *string, int string_length,
                        int prevent_duplicates)
{
 char **new_ptr, *new_string;
 int allocation_size, i;

 if(prevent_duplicates)
 {
  i = 0;
  while(i<array_size)
  {
   if(strcmp(string, (*array)[i]) == 0)
    return 1;
   i++;
  }
 }

 if(string_length == -1)
  string_length = strlen(string);

 allocation_size = (array_size + 1) * sizeof(char *);

 if((new_ptr = (char **) realloc(*array, allocation_size)) == NULL)
 {
  perror("BCA: realloc()");
  return -1;
 }
   
 *array = new_ptr;

 if(string != NULL)
 {
  allocation_size = string_length + 1;
  if((new_string = (char *) malloc(allocation_size)) == NULL)
  {
   perror("BCA: malloc()");
   return -1;
  }
  snprintf(new_string, allocation_size, "%s", string);
  (*array)[array_size] = new_string;
 } else {
  (*array)[array_size] = NULL;
 }

 return 0;
}

int free_string_array(char **array, int n_elements)
{
 int x;

 for(x=0; x<n_elements; x++)
 {
  if(array[x] != NULL)
   free(array[x]);
 }
 free(array);

 return 0;
}

int file_size_to_string(unsigned long long int file_size, char *string, int size)
{
 double f;
 char u[7];

 if(file_size < 1024)
 {
  f = file_size;
  return snprintf(string, size, "%d bytes", f);
 }

 if(file_size > 1024 * 1024 * 1024)
 {
  f = file_size / (1024.0 * 1024 * 1024);
  snprintf(u, 7, "GiB");
 } else if(file_size > 1024 * 1024) {
  f = file_size / (1024.0 * 1024);
  snprintf(u, 7, "MiB");
 } else {
  f = file_size / (1024.0);
  snprintf(u, 7, "KiB");
 }

 return snprintf(string, size, "%'.1f%s", f, u);
}


