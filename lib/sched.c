#include <env.h>
#include <pmap.h>
#include <printf.h>
int time_slices = 0;
/* Overview:
 *  Implement simple round-robin scheduling.
 *
 *
 * Hints:
 *  1. The variable which is for counting should be defined as 'static'.
 *  2. Use variable 'env_sched_list', which is a pointer array.
 *  3. CANNOT use `return` statement!
 */
/*** exercise 3.15 ***/
void sched_yield(void)
{
    static int count = 0; // remaining time slices of current env
    static int point = 0; // current env_sched_list index
	static struct Env * e = NULL;
	if (count == 0 || e == NULL || e -> env_status != ENV_RUNNABLE) {
		if (e != NULL) {
			LIST_REMOVE(e, env_sched_link);
			if (e -> env_status != ENV_FREE)
				LIST_INSERT_TAIL(&env_sched_list[1 - point], e, env_sched_link);
		}
		while (1) {
			while (LIST_EMPTY(&env_sched_list[point])) {
				point = 1 - point;
			}
			e = LIST_FIRST(&env_sched_list[point]);
			if (e -> env_status == ENV_FREE) {
				LIST_REMOVE(e, env_sched_link);
			} else if (e -> env_status == ENV_NOT_RUNNABLE) {
				LIST_REMOVE(e, env_sched_link);
				LIST_INSERT_TAIL(&env_sched_list[1 - point], e, env_sched_link);
			} else {
				count = e -> env_pri;
				break;
			}
		}
	}
	if (e != NULL) {
		count--;
		env_run(e);
	}
}

