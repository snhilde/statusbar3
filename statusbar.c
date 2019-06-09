#include "statusbar.h"
#include "config.h"

#define SBLENGTH 10240

/* ftime for milliseconds */
/* clock_getres(2) (see test.c) */
static int sb_print_to_sb(void)
{
	Display      *dpy;
	Window        root;
	char          full_output[SBLENGTH];
	size_t        offset;;
	sb_routine_t *routine;
	size_t        len;

	dpy = XOpenDisplay(NULL);
	root = RootWindow(dpy, DefaultScreen(dpy));

	while (1) {
		offset = 0;
		routine = routine_list;
		while (routine != NULL) {
			pthread_mutex_lock(&(routine->mutex));
			len = strlen(routine->output);
			if (offset + len > SBLENGTH - 1) {
				fprintf(stderr, "Exceeded max output length");
				break;
			}

			memcpy(full_output + offset, routine->output, len);
			offset += len;

			pthread_mutex_unlock(&(routine->mutex));
			routine = routine->next;
		}
		full_output[offset] = '\0';

		if (!XStoreName(dpy, root, full_output)) {
			fprintf(stderr, "Failed to set root name");
			return -1;
		}
		if (!XFlush(dpy)) {
			fprintf(stderr, "Failed to flush output buffer");
			return -1;
		}

		/* TODO: sleep for 1 second */
	}

	return 1;
}

static void *sb_backup_routine(void *thunk)
{
	
	return NULL;
}

static void *sb_battery_routine(void *thunk)
{
	
	return NULL;
}

static void *sb_brightness_routine(void *thunk)
{

	return NULL;
}

static void *sb_cpu_temp_routine(void *thunk)
{
	
	return NULL;
}

static void *sb_cpu_usage_routine(void *thunk)
{
	
	return NULL;
}

static void *sb_disk_routine(void *thunk)
{
	
	return NULL;
}

static void *sb_fan_routine(void *thunk)
{
	
	return NULL;
}

static void *sb_load_routine(void *thunk)
{
	
	return NULL;
}

static void *sb_log_routine(void *thunk)
{
	
	return NULL;
}

static void *sb_network_routine(void *thunk)
{
	
	return NULL;
}

static void *sb_ram_routine(void *thunk)
{
	
	return NULL;
}

static void *sb_time_routine(void *thunk)
{
	
	return NULL;
}

static void *sb_todo_routine(void *thunk)
{
	
	return NULL;
}

static void *sb_volume_routine(void *thunk)
{
	
	return NULL;
}

static void *sb_weather_routine(void *thunk)
{
	
	return NULL;
}

static void *sb_wifi_routine(void *thunk)
{
	
	return NULL;
}


static const struct thread_routines_t {
	enum sb_routine_e routine;
	void *(*callback)(void *thunk);
} possible_routines[] = {
	{ BACKUP,     sb_backup_routine     },
	{ BATTERY,    sb_battery_routine    },
	{ BRIGHTNESS, sb_brightness_routine },
	{ CPU_TEMP,   sb_cpu_temp_routine   },
	{ CPU_USAGE,  sb_cpu_usage_routine  },
	{ DISK,       sb_disk_routine       },
	{ FAN,        sb_fan_routine        },
	{ LOAD,       sb_load_routine       },
	{ LOG,        sb_log_routine        },
	{ NETWORK,    sb_network_routine    },
	{ RAM,        sb_ram_routine        },
	{ TIME,       sb_time_routine       },
	{ TODO,       sb_todo_routine       },
	{ VOLUME,     sb_volume_routine     },
	{ WEATHER,    sb_weather_routine    },
	{ WIFI,       sb_wifi_routine       },
};

int main(int argc, char *argv[])
{
	size_t             num_routines;
	int                i;
	enum sb_routine_e  index;
	int                j;
	sb_routine_t      *routine_object;
	void              *join_ret = NULL;

	num_routines = sizeof(chosen_routines) / sizeof(*chosen_routines);
	if (num_routines < 1) {
		fprintf(stderr, "No routines chosen, exiting...");
		return 1;
	}

	routine_list = routine_array + chosen_routines[0];

	/* step through each routine chosen in config.h and set it up */
	for (i = 0; i < num_routines; i++) {
		index = chosen_routines[i];

		/* initialize the routine */
		routine_object = routine_array + index;
		pthread_mutex_init(&(routine_object->mutex), NULL);
		if (index == DELIMITER)
			snprintf(routine_object->output, sizeof(routine_object->output), ";");

		/* string onto routine list */
		routine_object->next = routine_array + chosen_routines[i+1];
		
		/* create thread */
		for (j = 0; j < sizeof(possible_routines) / sizeof(*possible_routines); j++) {
			if (chosen_routines[i] == possible_routines[j].routine) {
				pthread_create(&(routine_object->thread), NULL, possible_routines[j].callback, (void *)routine_object);
				break;
			}
		}

		/* set flag for this routine */
		sb_flags_active |= 1<<index;
	}
	/* properly terminate the routine list */
	routine_object->next = NULL;

	/* block until all threads exit */
	for (i = 0; i < num_routines; i++) {
		index = chosen_routines[i];
		if (index == DELIMITER)
			continue;

		if (pthread_join(routine_array[index].thread, &join_ret) != 0)
			fprintf(stderr, "%s thread did not exit cleanly (%s)\n", routine_names[index], (char *)join_ret);
		free(join_ret);
	}

	return 0;
}
