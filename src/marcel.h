/*
 * Marcel the Shell -- a shell written in C
 * Copyright (C) 2016 Chad Sharp
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MARCEL_H
#define MARCEL_H

#if ! defined (__unix__) && (!(defined (__APPLE__) && defined (__MACH__)))
#error "Marcel requires a POSIX compliant OS"
#endif

extern char *program_invocation_short_name;
#define NAME program_invocation_short_name
#define VERSION "1.0"


// Most recent exit code
extern int exit_code;

enum {
    M_SUCCESS = 0,
    M_SIGINT = -1,
    M_FAILED_INIT = -2,
    M_FAILED_EXEC = -3,
    M_FAILED_ALLOC = -4,
    M_FAILED_IO = -5,
};


#endif
