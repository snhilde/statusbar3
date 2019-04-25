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
	enum sb_process_e process;
	void *(*routine)(void *thunk);
} thread_routines[] = {
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
	{ -1, NULL }
};

int main(int argc, char *argv[])
{
	int           i;
	int           index;
	sb_process_t *process;
	int           j;

	for (i = 0; configs[i].process >= 0; i++) {
		index            = configs[i].process;
		process          = process_array + index;

		/* set flag for this process */
		sb_flags_active |= 1<<index;

		/* set process-specific settings in process object */
		process->bar     = configs[i].bar;

		/* determine proper routine for this thread */
		for (j = 0; thread_routines[j].process >= 0; j++) {
			if (thread_routines[j].process == configs[i].process)
				break;
		}

		/* TODO: where to catch fallthroughs? */

		pthread_create(&(process->thread), NULL, thread_routines[j].routine, (void *)process);
	}

	return 0;
}
