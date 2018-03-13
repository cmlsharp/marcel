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

#ifndef M_JOB_CTRL
#define M_JOB_CTRL

#include <stdbool.h>
#include "ds/proc.h" // job

#define SHELL_TERM STDIN_FILENO

extern bool interactive;


bool initialize_job_control(void);
void send_to_foreground(job *j, bool cont);
void send_to_background(job *j, bool cont);
bool mark_proc_status(pid_t pid, int status);
void check_job_status(void);
void wait_for_job(job *j);
void format_job_info(job *j, char const *msg);
int report_job_status(void);
void continue_job(job *j);
job *find_job(pid_t pgid, job const *crawler);
bool is_stopped(job *j);
bool is_completed(job *j);
bool register_job(job *j);
#endif
