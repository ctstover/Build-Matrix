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
#include <dirent.h>

char *read_disk_file(struct buildmatrix_context *ctx, char *filename, size_t *filesize)
{
 int fd, n_bytes, length;
 size_t bytes_read = 0, allocation_size;
 char *buffer;
 struct stat stat_buffer;

 if((length = strlen(filename)) > 4000)
 {
  fprintf(stderr, "%s: filename \"%s\" too long\n", ctx->error_prefix, filename);
  return NULL;
 }

 if((fd = open(filename, O_RDONLY)) < 0)
 {
  fprintf(stderr, "%s: read_disk_file(): open(%s) failed: %s\n", 
          ctx->error_prefix, filename, strerror(errno));
  return NULL;
 }

 if(fstat(fd, &stat_buffer))
 {
  fprintf(stderr, "%s: read_disk_file(): fstat() failed: %s\n",
           ctx->error_prefix, strerror(errno));
  close(fd);
  return NULL;
 }

 allocation_size = stat_buffer.st_size + 2;
 if((buffer = malloc(allocation_size)) == NULL)
 {
  fprintf(stderr, "%s: malloc(%d) failed\n", ctx->error_prefix, (int) allocation_size);
  close(fd);
  return NULL;
 }

 while(bytes_read < (allocation_size - 2))
 {

  if((n_bytes = read(fd, buffer + bytes_read, allocation_size - bytes_read)) < 0)
  {
   fprintf(stderr, "%s: read_disk_file(): read() failed: %s\n",
           ctx->error_prefix, strerror(errno));
   close(fd);
   free(buffer);
   return NULL;
  }

  bytes_read += n_bytes;
 }
 buffer[bytes_read] = 0;
 buffer[bytes_read + 1] = 0;

 close(fd);

 if(filesize != NULL)
  *filesize = allocation_size;
 return buffer;
}

int remove_db_file(struct buildmatrix_context *ctx, const char *filename)
{
 char *path, dir[3];
 int allocation_size;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: remove_db_file(%s)\n", ctx->error_prefix, filename);

 if(filename == NULL)
  return 1;

 if(ctx->unique_identifier == NULL)
 {
  fprintf(stderr, "%s: remove_db_file(): unique identifier needed\n", ctx->error_prefix);
  return 1;
 }

 allocation_size = strlen(ctx->local_project_directory)
                 + strlen(ctx->unique_identifier)
                 + strlen(filename)
                 + 10;

 if((path = malloc(allocation_size)) == NULL)
 {
  fprintf(stderr, "%s: remove_db_file(): malloc(%d) failed: %s\n",
          ctx->error_prefix, allocation_size, strerror(errno));
  return 1;
 }

 snprintf(dir, 3, "%s", ctx->unique_identifier);

 snprintf(path, allocation_size, "%s/%s/%s-%s", 
          ctx->local_project_directory, dir, ctx->unique_identifier, filename);

 if(remove_disk_file(ctx, path))
 {
  free(path);
  return 1;
 }

 free(path);
 return 0;
}

int remove_disk_file(struct buildmatrix_context *ctx, const char *filename)
{
 char path[4096];

 if(ctx->verbose)
  fprintf(stderr, "%s: remove_disk_file(%s)\n", ctx->error_prefix, filename);

 if(filename[0] == '/')
  snprintf(path, 4096, "%s", filename);
 else
  snprintf(path, 4096, "./%s", filename);

 if(unlink(path) != 0)
 {
  fprintf(stderr, "%s: remove_disk_file(): unlink(%s) failed: %s\n",
          ctx->error_prefix, path, strerror(errno));
  return 1;
 }

 if(ctx->verbose)
  fprintf(stderr, "%s: unlink(%s)\n", ctx->error_prefix, path);

 return 0;
}

int send_file_from_blob(struct buildmatrix_context *ctx, char *filename, char *blob, int size)
{
 long long int n_bytes, bytes_read = 0, length;
 char buffer[4096]; 

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: send_file_from_blob(%s)\n", ctx->error_prefix, filename);

 if( (filename == NULL) || (blob == NULL) || (size < 1) )
 {
  fprintf(stderr, "%s: send_file_from_blob(): invalid input\n", ctx->error_prefix);
  return 1;
 }

 if(ctx->verbose)
  fprintf(stderr, "%s: trying to send file %s (%d bytes)\n", 
          ctx->error_prefix, filename, size);


 length = snprintf(buffer, 4096, "sendfile %lld %s\n", 
                   (long long int) size, filename);

 if((n_bytes = write(ctx->out, buffer, length)) < 1)
 {
  fprintf(stderr, "%s: write() failed: %s\n", ctx->error_prefix, strerror(errno));
  return 1;
 }

 while(1)
 {
  if((n_bytes = read(ctx->in, buffer, 4095)) < 0)
  {
   fprintf(stderr, "%s: read() failed: %s\n", ctx->error_prefix, strerror(errno));
   return 1;
  }
  buffer[n_bytes] = 0;
 
  if(strncmp(buffer, "failed", 6) == 0)
  {
   fprintf(stderr, "%s: send_file_from_blob(): peer bailing out of file transfer '%s'\n",
           ctx->error_prefix, buffer + 7);
   return 1;
  }

  if(strncmp(buffer, "ok", 2) != 0)
  {
   fprintf(stderr, "%s: send_file_from_blob(): unexpected response in file transfer, \"%s\"\n",
          ctx->error_prefix, buffer);
   return 1;
  } else {
   if(bytes_read == size)
    break;
  }

  if(size - bytes_read > 4096)
   n_bytes = 4096;
  else
   n_bytes = size - bytes_read;

  memcpy(buffer, blob + bytes_read, n_bytes);

  if(ctx->verbose > 2)
   fprintf(stderr, "%s: send_file_from_blob(): sending bytes %lld through %lld of file transfer\n", 
           ctx->error_prefix, bytes_read, bytes_read + n_bytes);

  if((n_bytes = write(ctx->out, buffer, n_bytes)) < 1)
  {
   fprintf(stderr, "%s: send_file_from_blob() write() failed: %s\n", ctx->error_prefix, strerror(errno));
   return 1;
  }

  bytes_read += n_bytes;
 }

 if(ctx->verbose)
  fprintf(stderr, "%s: send_file_from_blob(): transfer complete\n", ctx->error_prefix);

 return 0;
}

int send_db_file(struct buildmatrix_context *ctx, char *filename)
{
 char *path, dir[3];
 int allocation_size;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: send_db_file()\n", ctx->error_prefix);

 if(ctx->unique_identifier == NULL)
 {
  fprintf(stderr, "%s: send_db_file(): unique identifier need\n", ctx->error_prefix);
  return 1;
 }

 allocation_size = strlen(ctx->local_project_directory)
                 + strlen(ctx->unique_identifier)
                 + strlen(filename)
                 + 10;

 if((path = malloc(allocation_size)) == NULL)
 {
  fprintf(stderr, "%s: send_db_file(): malloc(%d) failed: %s\n",
          ctx->error_prefix, allocation_size, strerror(errno));
  return 1;
 }

 snprintf(dir, 3, "%s", ctx->unique_identifier);

 snprintf(path, allocation_size, "%s/%s/%s-%s", 
          ctx->local_project_directory, dir, ctx->unique_identifier, filename);

 if(send_disk_file(ctx, path, NULL))
 {
  free(path);
  return 1;
 }

 free(path);
 return 0;
}


int send_disk_file(struct buildmatrix_context *ctx, char *filename, char *filecontents)
{
 int fd, n_bytes, length, handled, i;
 long long int bytes_read = 0;
 char buffer[4096]; 
 struct stat stat_buffer;

 if(filecontents == NULL)
 {
  if(filename == NULL)
   return 1;

  if((length = strlen(filename)) > 4000)
  {
   fprintf(stderr, "%s: filename \"%s\" too long\n", ctx->error_prefix, filename);
   return 1;
  }

  if(filename[0] == '/')
   snprintf(buffer, 4096, "%s", filename);
  else
   snprintf(buffer, 4096, "./%s", filename);

  if((fd = open(buffer, O_RDONLY)) < 0)
  {
   fprintf(stderr, "%s: send_disk_file(): open(%s) failed: %s\n", 
           ctx->error_prefix, buffer, strerror(errno));
   return 1;
  }
 
  if(fstat(fd, &stat_buffer))
  {
   fprintf(stderr, "%s: send_disk_file(): fstat() failed: %s\n",
           ctx->error_prefix, strerror(errno));
   close(fd);
   return 1;
  }

 } else {
  stat_buffer.st_size = strlen(filecontents);
 }

 if(ctx->verbose)
  fprintf(stderr, "%s: send_disk_file(): trying to send file %s (%d bytes)\n", 
          ctx->error_prefix, filename, (int) stat_buffer.st_size);

 i = length;
 while(i > 0)
 {
  if(filename[i] == '/')
  {
   i++;
   break;
  }
  i--;
 }

 length = snprintf(buffer, 4096, "sendfile %lld %s\n", 
                   (long long int) stat_buffer.st_size, filename + i);

 if((n_bytes = write(ctx->out, buffer, length)) < 1)
 {
  fprintf(stderr, "%s: send_disk_file(): write() failed: %s\n",
          ctx->error_prefix, strerror(errno));
  if(filecontents == NULL)
   close(fd);
  return 1;
 }

 while(1)
 {
  handled = 0;

  if(filecontents == NULL)
  {
   if((n_bytes = read(ctx->in, buffer, 64)) < 1)
   {
    fprintf(stderr, "%s: send_disk_file() read() failed: %s\n",
            ctx->error_prefix, strerror(errno));

    if(filecontents == NULL)
     close(fd);
    return 1;
   }
  } else {
   if(stat_buffer.st_size - bytes_read > 4096)
    n_bytes = 4096;
   else
    n_bytes = stat_buffer.st_size - bytes_read;

   memcpy(buffer, filecontents + bytes_read, n_bytes);
  }

  buffer[n_bytes] = 0;
  if(strncmp(buffer, "failed", 6) == 0)
  {
   fprintf(stderr, "%s: server bailing out of file transfer '%s'\n", ctx->error_prefix, buffer + 7);
   return 1;
  }

  if(strncmp(buffer, "ok", 2) == 0)
  {
   if(bytes_read < stat_buffer.st_size)
   {
    handled = 1;
   } else {
    if(ctx->verbose)
     fprintf(stderr, "%s: transfer complete\n", ctx->error_prefix);
    break;
   }
  }

  if(handled == 0)
  {
   fprintf(stderr, "%s: unexpected response in file transfer, \"%s\"\n", ctx->error_prefix, buffer);
   if(filecontents == NULL)
    close(fd);
   return 1;
  }

  if((n_bytes = read(fd, buffer, 4096)) < 0)
  {
   fprintf(stderr, "%s: send_disk_file(): read() failed: %s\n",
           ctx->error_prefix, strerror(errno));

   if(filecontents == NULL)
    close(fd);
   return 1;
  }

  if(ctx->verbose > 2)
   fprintf(stderr, "%s: sending bytes %lld through %lld of file transfer\n", 
           ctx->error_prefix, bytes_read, bytes_read + n_bytes);

  if((n_bytes = write(ctx->out, buffer, n_bytes)) < 0)
  {
   fprintf(stderr, "%s: send_disk_file(): write() failed: %s\n",
           ctx->error_prefix, strerror(errno));

   if(filecontents == NULL)
    close(fd);
   return 1;
  }

  bytes_read += n_bytes;
 }

 if(filecontents == NULL)
  close(fd);
 return 0;
}

int receive_disk_file(struct buildmatrix_context *ctx, char **filename_ptr, int limbo)
{
 long long int filesize, bytes_read = 0;
 char buffer[4097], filename[4002];
 int n_bytes, fd, bytes_written, i, length;
 struct stat metadata;

 if(ctx->verbose)
  fprintf(stderr, "%s: receive_disk_file()\n", ctx->error_prefix);

 if(limbo && (ctx->unique_identifier == NULL) )
 {
  fprintf(stderr, "%s: receive_disk_file() can not write to limbo with out a unique identifier\n",
          ctx->error_prefix);
  return 1;
 }

 if((n_bytes = read(ctx->in, buffer, 4096)) < 0)
 {
  fprintf(stderr, "%s: receive_disk_file(): read() failed: %s\n",
          ctx->error_prefix, strerror(errno));
  return 1;
 }

 if( (n_bytes < 10) ||
     (strncmp(buffer, "sendfile ", 9) != 0) )
 {
  buffer[n_bytes] = 0;
  fprintf(stderr, "%s: unexpected traffic in file transfer, \"%s\"\n", 
          ctx->error_prefix, buffer);
  return 1;
 }

 if(limbo)
  length = snprintf(filename, 4002, "./%s-", ctx->unique_identifier);
 else
  length = snprintf(filename, 4002, "./");

 sscanf(buffer + 9, "%lld %4000s", &filesize, filename + length);
 if(ctx->verbose)
 {
  fprintf(stderr, 
          "%s: receive_disk_file(): recieving %s (%lld bytes) (local name overide = \"%s\")\n", 
          ctx->error_prefix, filename, filesize, *filename_ptr);
 }

 if(*filename_ptr != NULL)
 {
  if(limbo)
  {
   fprintf(stderr, "%s: receive_disk_file(): can not use name overide in limbo directory\n",
           ctx->error_prefix);
   return 1;
  }
  length = 0;
  snprintf(filename, 4002, "%s", *filename_ptr);  
 }

 if(stat(filename, &metadata) == 0)
 {
  if(S_ISREG(metadata.st_mode) == 0)
  {
   fprintf(stderr, "%s: non-regular local file '%s' already exits\n",
           ctx->error_prefix, filename);
   write(ctx->out, "failed\n", 7);
   return 1;   
  } else {

   if(ctx->yes_no_prompt == NULL) // server operations
   {
    fprintf(stderr, "%s: receive_disk_file(): huh? %s already exists (limbo = %d)\n",
            ctx->error_prefix, filename, limbo);
    write(ctx->out, "failed\n", 7);     
    return 1;
   } else {
    snprintf(buffer, 4096, "Overwrite local file \"%s\"?", filename);
    if(ctx->yes_no_prompt(ctx, buffer) == 0)
    {
     fprintf(stderr, "%s: local file '%s' already exits\n",
             ctx->error_prefix, filename);
     write(ctx->out, "failed\n", 7);
     return 1;  
    }
   }
  }
 } 

 if(filename_ptr != NULL)
 {
  if(*filename_ptr == NULL)
   *filename_ptr = strdup(filename + length);
 }

 if(limbo)
 {
  if(chdir(ctx->limbo_directory))
  {
   fprintf(stderr, "%s: chdir() failed: %s\n", ctx->error_prefix, strerror(errno));
   send_line(ctx, 7, "failed\n");
   return 1;
  }
 }

 if((fd = open(filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR |  S_IRGRP | S_IWGRP)) < 0)
 {
  fprintf(stderr, "%s: open(%s) failed: %s\n", 
          ctx->error_prefix, filename, strerror(errno));
  write(ctx->out, "failed\n", 7);
  return 1;
 }

 if((n_bytes = write(ctx->out, "ok\n", 3)) != 3)
 {
  fprintf(stderr, "%s: receive_disk_file(): write() failed: %s\n",
          ctx->error_prefix, strerror(errno));
  close(fd);
  if(filename_ptr)
   free(*filename_ptr);
  return 1;
 }

 while(bytes_read < filesize)
 {
  if((n_bytes = read(ctx->in, buffer, 4096)) < 0)
  {
   fprintf(stderr, "%s: receive_disk_file(): write() failed: %s\n",
           ctx->error_prefix, strerror(errno));
   close(fd);
   if(filename_ptr)
    free(*filename_ptr);
   return 1;
  }

  errno = 0;
  bytes_written = 0;
  while(bytes_written < n_bytes)
  {
   if((i = write(fd, buffer + bytes_written, n_bytes - bytes_written)) < 1)
   {
    fprintf(stderr, "%s: receive_disk_file(): write() failed: %s\n",
            ctx->error_prefix, strerror(errno));
    write(ctx->out, "failed\n", 7);
    close(fd);
    if(filename_ptr)
     free(*filename_ptr);
    return 1;
   }
   bytes_written += i;
  }

  bytes_read += n_bytes;

  if((n_bytes = write(ctx->out, "ok\n", 3)) != 3)
  {
   fprintf(stderr, "%s: receive_disk_file(): write() failed: %s\n",
           ctx->error_prefix, strerror(errno));
   close(fd);
   if(filename_ptr)
    free(*filename_ptr);
   return 1;
  }

  if(ctx->verbose > 2)
   fprintf(stderr, "%s: received %lld bytes\n",
           ctx->error_prefix, bytes_read);
 }

 close(fd);

 if(limbo)
 {
  if(chdir(ctx->current_working_directory))
  {
   fprintf(stderr, "%s: chdir() failed: %s\n", ctx->error_prefix, strerror(errno));
   send_line(ctx, 7, "failed\n");
   return 1;
  }
 }

 if(ctx->verbose)
  fprintf(stderr, "%s: receive_disk_file(): transfer completed of %s\n",
          ctx->error_prefix, *filename_ptr);

 return 0;
}


int receive_file_as_blob(struct buildmatrix_context *ctx, char **filename_ptr, char **filecontents)
{
 long long int filesize, bytes_read = 0;
 char buffer[4097], filename[4001];
 int n_bytes, allocation_size;

 if(ctx->verbose)
  fprintf(stderr, "%s: receive_file()\n", ctx->error_prefix);

 if((n_bytes = read(ctx->in, buffer, 4096)) < 0)
 {
  fprintf(stderr, "%s: receive_file_as_blob(): read() failed: %s\n",
          ctx->error_prefix, strerror(errno));
  return 1;
 }

 if( (n_bytes < 10) ||
     (strncmp(buffer, "sendfile ", 9) != 0) )
 {
  buffer[n_bytes] = 0;
  fprintf(stderr, "%s: unexpected traffic in file transfer, \"%s\"\n", 
          ctx->error_prefix, buffer);
  return 1;
 }

 filename[0] = '.';
 filename[1] = '/';
 sscanf(buffer + 9, "%lld %4000s", &filesize, filename + 2);
 if(ctx->verbose)
 {
  fprintf(stderr, "%s: recieving %s (%lld bytes)\n", ctx->error_prefix, filename, filesize);
 }

 if(filename_ptr != NULL)
 {
  *filename_ptr = strdup(filename);
 }

 if(filesize > 10 * 1024)
 {
  if(filename_ptr)
   free(*filename_ptr);
  fprintf(stderr, "%s: file size big for this type\n", ctx->error_prefix);
  write(ctx->out, "failed\n", 7);
  return 1;
 }

 allocation_size = filesize;
 if((*filecontents = malloc(allocation_size)) == NULL)
 {
  if(filename_ptr)
   free(*filename_ptr);
  fprintf(stderr, "%s: malloc(%d)\n", ctx->error_prefix, allocation_size);
  write(ctx->out, "failed\n", 7);
  return 1;
 }

 while(bytes_read < filesize)
 {
  if((n_bytes = read(ctx->in, buffer, 4096)) < 0)
  {
   fprintf(stderr, "%s: receive_file_as_blob(): read() failed: %s\n",
           ctx->error_prefix, strerror(errno));
   if(filename_ptr)
    free(*filename_ptr);
   free(*filecontents);
   return 1;
  }

  memcpy(*filecontents + bytes_read, buffer, n_bytes);
  bytes_read += n_bytes;

  if((n_bytes = write(ctx->out, "ok\n", 3)) != 3)
  {
   fprintf(stderr, "%s: receive_file_as_blob(): write() failed: %s\n",
           ctx->error_prefix, strerror(errno));
   if(filename_ptr)
    free(*filename_ptr);
   free(*filecontents);
   return 1;
  }

  if(ctx->verbose > 2)
   fprintf(stderr, "%s: received %lld bytes\n",
           ctx->error_prefix, bytes_read);
 }

 if(ctx->verbose)
  fprintf(stderr, "%s: transfer complete\n", ctx->error_prefix);

 return 0;
}

int limbo_to_database(struct buildmatrix_context *ctx, char *filename)
{
 char buffer[4096], *from_path, *to_path, dir[3];
 int allocation_size, in_fd, out_fd, bytes_in, bytes_out, c;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: limbo_to_database(%s)\n", 
          ctx->error_prefix, filename);

 if(ctx->unique_identifier == NULL)
 {
  fprintf(stderr, "%s: limbo_to_database() unique identifier is null\n",
          ctx->error_prefix);
  return 1;
 }

 if(filename == NULL)
 {
  fprintf(stderr, "%s: limbo_to_database() filename is null\n",
          ctx->error_prefix);
  return 1;
 }

 snprintf(dir, 3, "%s", ctx->unique_identifier);

 allocation_size = strlen(ctx->limbo_directory)
                 + strlen(filename) 
                 + 23;

 if((from_path = malloc(allocation_size)) == NULL)
 {
  fprintf(stderr, "%s: limbo_to_database(): malloc(%d) failed: %s\n",
          ctx->error_prefix, allocation_size, strerror(errno));
  return 1;
 }

 snprintf(from_path, allocation_size, "%s/%s-%s", ctx->limbo_directory,
          ctx->unique_identifier, filename);

 allocation_size = strlen(ctx->local_project_directory)
                 + strlen(filename) 
                 + 27;

 if((to_path = malloc(allocation_size)) == NULL)
 {
  fprintf(stderr, "%s: limbo_to_database(): malloc(%d) failed: %s\n",
          ctx->error_prefix, allocation_size, strerror(errno));
  free(from_path);
  return 1;
 }

 snprintf(to_path, allocation_size, "%s/%s", ctx->local_project_directory, dir);

 mkdir(to_path, S_IRWXU);
 errno = 0;

 snprintf(to_path, allocation_size, "%s/%s/%s-%s", 
          ctx->local_project_directory, dir, ctx->unique_identifier, filename);

 if((in_fd = open(from_path, O_RDONLY)) < 0)
 {
  fprintf(stderr, "%s: limbo_to_database(): open(%s) failed: %s\n", 
          ctx->error_prefix, from_path, strerror(errno));

  free(to_path);
  free(from_path); 
  return 1;
 }

 if((out_fd = open(to_path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR |  S_IRGRP | S_IWGRP)) < 0)
 {
  fprintf(stderr, "%s: limbo_to_database(): open(%s) failed: %s\n", 
          ctx->error_prefix, to_path, strerror(errno));

  close(in_fd);
  free(to_path);
  free(from_path); 
  return 1;
 }

 while((bytes_in = read(in_fd, buffer, 4096)) > 0)
 {
  bytes_out = 0;
  while(bytes_out < bytes_in)
  {
   if((c = write(out_fd, buffer, bytes_in)) < 1)
   {
    fprintf(stderr, "%s: limbo_to_database(): write() failed: %s\n", 
            ctx->error_prefix, strerror(errno));
    close(out_fd);
    close(in_fd);
    free(to_path);
    free(from_path); 
   }
   bytes_out += c;
  }
 }

 close(in_fd);
 close(out_fd);

 if(unlink(from_path))
 {
  fprintf(stderr, "%s: limbo_to_database(): unlink(%s) failed: %s\n", 
          ctx->error_prefix, from_path, strerror(errno));
  free(to_path);
  free(from_path);
  return 1;
 }

 free(to_path);
 free(from_path);
 return 0;
}

int limbo_cleanup(struct buildmatrix_context *ctx)
{
 char path[4096];
 int length;
 DIR *dir;
 struct dirent *dir_entry;

 if(ctx->verbose > 1)
  fprintf(stderr, "%s: limbo_cleanup()\n", ctx->error_prefix);

 if(ctx->local_project_directory == NULL)
 {
  fprintf(stderr, "%s: limbo_cleanup(): I need a local project directory\n", ctx->error_prefix);
  return 1;
 }

 length = strlen(ctx->local_project_directory);
 length += 17;
 if((ctx->limbo_directory = malloc(length)) == NULL)
 {
  fprintf(stderr, "%s: serve(): malloc(%d) failed: %s\n",
          ctx->error_prefix, length, strerror(errno));
  return 1;
 }
 snprintf(ctx->limbo_directory, length, "%s/incomming_files", ctx->local_project_directory);
 mkdir(ctx->limbo_directory, S_IRWXU);
 errno = 0;

 errno = 0;
 if((dir = opendir(ctx->limbo_directory)) == NULL)
 {
  fprintf(stderr, "%s: limbo_cleanup(): opendir(%s) failed: %s\n", 
          ctx->error_prefix, ctx->limbo_directory, strerror(errno));
  return 1;
 }

 while((dir_entry = readdir(dir)) != NULL)
 {
  if(dir_entry->d_type == DT_REG)
  {
   snprintf(path, 4096, "%s/%s", ctx->limbo_directory, dir_entry->d_name);
   if(remove_disk_file(ctx, path))
   {
    closedir(dir);
    return 1;
   }
  }
 }
 if(errno != 0)
 {
  fprintf(stderr, "%s: limbo_cleanup(): readdir(%s) failed: \"%s\"",
          ctx->error_prefix, ctx->limbo_directory, strerror(errno));
  closedir(dir);
  return 1;
 }

 closedir(dir);

 return 0;
}

unsigned long long int file_size(struct buildmatrix_context *ctx, const char *filename)
{
 int fd;
 unsigned long long int file_size = -1;
 struct stat stat_buffer;

 if((fd = open(filename, O_RDONLY)) < 0)
 {
  fprintf(stderr, "%s: file_size(): open(%s) failed: %s\n", 
          ctx->error_prefix, filename, strerror(errno));
  return 0;
 }

 if(fstat(fd, &stat_buffer))
 {
  fprintf(stderr, "%s: file_size(): fstat() failed: %s\n",
           ctx->error_prefix, strerror(errno));
  close(fd);
  return 0;
 }

 file_size = (long long int) stat_buffer.st_size;

 close(fd);

 return file_size;
}

