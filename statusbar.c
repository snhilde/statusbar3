#include "statusbar.h"
#include "config.h"


static void *backup_routine(void *thunk)
{
	
	return NULL;
}

static void *battery_routine(void *thunk)
{
	
	return NULL;
}

static void *brightness_routine(void *thunk)
{
	
	return NULL;
}

static void *cpu_temp_routine(void *thunk)
{
	
	return NULL;
}

static void *cpu_usage_routine(void *thunk)
{
	
	return NULL;
}

static void *disk_routine(void *thunk)
{
	
	return NULL;
}

static void *fan_routine(void *thunk)
{
	
	return NULL;
}

static void *load_routine(void *thunk)
{
	
	return NULL;
}

static void *log_routine(void *thunk)
{
	
	return NULL;
}

static void *network_routine(void *thunk)
{
	
	return NULL;
}

static void *ram_routine(void *thunk)
{
	
	return NULL;
}

static void *time_routine(void *thunk)
{
	
	return NULL;
}

static void *todo_routine(void *thunk)
{
	
	return NULL;
}

static void *volume_routine(void *thunk)
{
	
	return NULL;
}

static void *weather_routine(void *thunk)
{
	
	return NULL;
}

static void *wifi_routine(void *thunk)
{
	
	return NULL;
}


static const struct thread_routines_t {
	enum sb_routine_e routine;
	void *(*callback)(void *thunk);
} possible_routines[] = {
	{ BACKUP,     backup_routine     },
	{ BATTERY,    battery_routine    },
	{ BRIGHTNESS, brightness_routine },
	{ CPU_TEMP,   cpu_temp_routine   },
	{ CPU_USAGE,  cpu_usage_routine  },
	{ DISK,       disk_routine       },
	{ FAN,        fan_routine        },
	{ LOAD,       load_routine       },
	{ LOG,        log_routine        },
	{ NETWORK,    network_routine    },
	{ RAM,        ram_routine        },
	{ TIME,       time_routine       },
	{ TODO,       todo_routine       },
	{ VOLUME,     volume_routine     },
	{ WEATHER,    weather_routine    },
	{ WIFI,       wifi_routine       },
	{ ENDOFLIST, NULL }
};

int main(int argc, char *argv[])
{
	int                i;
	enum sb_routine_e  chosen_routine;
	int                j;
	sb_routine_t      *routine_object;
	void              *join_ret = NULL;

	/* step through each routine chosen in config.h and set it up */
	for (i = 0; chosen_routines[i] != ENDOFLIST; i++) {
		chosen_routine = chosen_routines[i];
		if (chosen_routine == DELIMITER)
			continue; /* stub */

		/* First, match chosen routine to master list of routines (possible_routines) to determine
		 * callback function. If a match is not found, throw an error and exit. */
		for (j = 0; possible_routines[j].routine != ENDOFLIST; j++) {
			if (possible_routines[j].routine == chosen_routine)
				break;
		}

		if (possible_routines[j].routine == ENDOFLIST) {
			/* we have an unknown value */
			fprintf(stderr, "Unknown routine in index %d of chosen_routines", i);
			/* clean up all threads */
			return 1;
		}

		/* set flag for this routine */
		sb_flags_active |= 1<<chosen_routine;

		/* start initializing the routine */
		routine_object = routine_array + chosen_routine;

		/* set routine-specific settings in routine object */


		/* create thread */
		pthread_create(&(routine_object->thread), NULL, possible_routines[j].callback, (void *)routine_object);
	}

	/* block until all threads exit */
	for (i = 0; chosen_routines[i] != ENDOFLIST; i++) {
		chosen_routine = chosen_routines[i];
		if (chosen_routine == DELIMITER)
			continue;

		if (pthread_join(routine_array[chosen_routine].thread, &join_ret) != 0)
			fprintf(stderr, "%s thread did not exit cleanly (%s)\n", routine_names[chosen_routine], (char *)join_ret);
		free(join_ret);
	}

	return 0;
}
