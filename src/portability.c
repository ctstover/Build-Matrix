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

#include <openssl/md5.h>

int build_matrix_md5_wrapper(const struct buildmatrix_context *ctx,
                             const unsigned char *input, unsigned int length,
                             unsigned char *output)
{
 if(ctx->verbose > 1)
  fprintf(stderr, "%s: build_matrix_md5_wrapper()\n", ctx->error_prefix);

 if( (input == NULL) || (output == NULL) )
  return 1;

 MD5(input, length, output);

 return 0;
}



