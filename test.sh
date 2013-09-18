#!/bin/bash

#    Copyright 2013 Stover Enterprises, LLC (An Alabama Limited Liability Corporation) 
#    Written by C. Thomas Stover
#
#    This file is part of the program Build Matrix.
#    See http://buildmatrix.stoverenterprises.com for more information.
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.


CLIENTBIN=`pwd`/native/bldmtrxc
SERVERBIN=`pwd`/native/bldmtrxs
PROJECT_DIR=`pwd`/test-project
SSH=`which ssh`
SERVER=localhost
VERBOSE="--verbose --verbose"

#rm -rf $PROJECT_DIR
#mkdir $PROJECT_DIR
#$SERVERBIN --init $PROJECT_DIR
#$SERVERBIN --createjob alpha --description "description of job alpha" $PROJECT_DIR
#$CLIENTBIN $VERBOSE --bldmtrxspath $SERVERBIN --project $PROJECT_DIR --sshbin $SSH \
#           --server $SERVER --branch all jobs

#$CLIENTBIN $VERBOSE --bldmtrxspath $SERVERBIN --project $PROJECT_DIR --sshbin $SSH \
#           --server $SERVER --job alpha --branch trunk \
#           --testresults ../buildconfigurationadjust/test.sh-results  \
#           submit

$CLIENTBIN $VERBOSE --bldmtrxspath $SERVERBIN --project $PROJECT_DIR --sshbin $SSH \
           --server $SERVER --job alpha --build 1 build
