#ifndef M_JOB_CTRL
#define M_JOB_CTRL

#include "ds/cmd.h" // job

extern _Bool interactive;
extern int shell_term;

_Bool initialize_job_control(void);
void put_job_in_foreground(job *j, _Bool cont);
void put_job_in_background(job *j, _Bool cont);
_Bool mark_cmd_status(pid_t pid, int status);
void update_status(void);
void wait_for_job(job *j);
void format_job_info(job *j, char const *msg);
int do_job_notification(void);
void continue_job(job *j);
job *find_job(pid_t pgid, job const *crawler);
_Bool job_is_stopped(job *j);
_Bool job_is_completed(job *j);
_Bool register_job(job *j);
#endif
