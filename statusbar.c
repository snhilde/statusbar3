#include "statusbar.h"
#include "config.h"

#define SBLENGTH 10240

static void *sb_print_to_sb(void *thunk)
{
	Display         *dpy;
	Window           root;
	struct timespec  start_tp;
	struct timespec  finish_tp;;
	long             elapsed_usec;
	char             full_output[SBLENGTH];
	size_t           offset;;
	sb_routine_t    *routine;
	size_t           len;

	dpy = XOpenDisplay(NULL);
	root = RootWindow(dpy, DefaultScreen(dpy));

	memset(&start_tp, 0, sizeof(start_tp));
	memset(&finish_tp, 0, sizeof(finish_tp));

	while (1) {
		clock_gettime(CLOCK_MONOTONIC, &start_tp);

		offset = 0;
		routine = routine_list;
		while (routine != NULL) {
			pthread_mutex_lock(&(routine->mutex));
			len = strlen(routine->output);
			if (offset + len > SBLENGTH - 1) {
				fprintf(stderr, "Exceeded max output length\n");
				break;
			}

			memcpy(full_output + offset, routine->output, len);
			offset += len;

			pthread_mutex_unlock(&(routine->mutex));
			routine = routine->next;
		}
		full_output[offset] = '\0';

		if (!XStoreName(dpy, root, full_output)) {
			fprintf(stderr, "Failed to set root name\n");
			break;
		}
		if (!XFlush(dpy)) {
			fprintf(stderr, "Failed to flush output buffer\n");
			break;
		}

		clock_gettime(CLOCK_MONOTONIC, &finish_tp);
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000);
		if (usleep(1000000 - elapsed_usec) != 0) {
			fprintf(stderr, "Print routine: Error sleeping\n");
		}
	}

	/* TODO: exit program here */
	return NULL;
}


/* --- BACKUP ROUTINE --- */
static void *sb_backup_routine(void *thunk)
{
	sb_routine_t    *routine = thunk;
	struct timespec  start_tp;
	struct timespec  finish_tp;;
	long             elapsed_usec;

	memset(&start_tp, 0, sizeof(start_tp));
	memset(&finish_tp, 0, sizeof(finish_tp));

	while(1) {
		clock_gettime(CLOCK_MONOTONIC, &start_tp);

		/* TODO: run routine */

		clock_gettime(CLOCK_MONOTONIC, &finish_tp);
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000);
		if (usleep((routine->interval * 1000000) - elapsed_usec) != 0) {
			fprintf(stderr, "Backup routine: Error sleeping\n");
		}
	}
	
	return NULL;
}


/* --- BATTERY ROUTINE --- */
static void *sb_battery_routine(void *thunk)
{
	sb_routine_t    *routine = thunk;
	struct timespec  start_tp;
	struct timespec  finish_tp;;
	long             elapsed_usec;

	memset(&start_tp, 0, sizeof(start_tp));
	memset(&finish_tp, 0, sizeof(finish_tp));

	while(1) {
		clock_gettime(CLOCK_MONOTONIC, &start_tp);

		/* TODO: run routine */

		clock_gettime(CLOCK_MONOTONIC, &finish_tp);
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000);
		if (usleep((routine->interval * 1000000) - elapsed_usec) != 0) {
			fprintf(stderr, "Battery routine: Error sleeping\n");
		}
	}
	
	return NULL;
}


/* --- BRIGHTNESS ROUTINE --- */
static void *sb_brightness_routine(void *thunk)
{
	sb_routine_t    *routine = thunk;
	struct timespec  start_tp;
	struct timespec  finish_tp;;
	long             elapsed_usec;

	memset(&start_tp, 0, sizeof(start_tp));
	memset(&finish_tp, 0, sizeof(finish_tp));

	while(1) {
		clock_gettime(CLOCK_MONOTONIC, &start_tp);

		/* TODO: run routine */

		clock_gettime(CLOCK_MONOTONIC, &finish_tp);
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000);
		if (usleep((routine->interval * 1000000) - elapsed_usec) != 0) {
			fprintf(stderr, "Brightness routine: Error sleeping\n");
		}
	}

	return NULL;
}


/* --- CPU TEMP ROUTINE --- */
static void *sb_cpu_temp_routine(void *thunk)
{
	sb_routine_t    *routine = thunk;
	struct timespec  start_tp;
	struct timespec  finish_tp;;
	long             elapsed_usec;

	memset(&start_tp, 0, sizeof(start_tp));
	memset(&finish_tp, 0, sizeof(finish_tp));

	while(1) {
		clock_gettime(CLOCK_MONOTONIC, &start_tp);

		/* TODO: run routine */

		clock_gettime(CLOCK_MONOTONIC, &finish_tp);
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000);
		if (usleep((routine->interval * 1000000) - elapsed_usec) != 0) {
			fprintf(stderr, "CPU Temp routine: Error sleeping\n");
		}
	}
	
	return NULL;
}


/* --- CPU USAGE ROUTINE --- */
static void *sb_cpu_usage_routine(void *thunk)
{
	sb_routine_t    *routine = thunk;
	struct timespec  start_tp;
	struct timespec  finish_tp;;
	long             elapsed_usec;

	memset(&start_tp, 0, sizeof(start_tp));
	memset(&finish_tp, 0, sizeof(finish_tp));

	while(1) {
		clock_gettime(CLOCK_MONOTONIC, &start_tp);

		/* TODO: run routine */

		clock_gettime(CLOCK_MONOTONIC, &finish_tp);
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000);
		if (usleep((routine->interval * 1000000) - elapsed_usec) != 0) {
			fprintf(stderr, "CPU Usage routine: Error sleeping\n");
		}
	}
	
	return NULL;
}


/* --- DISK ROUTINE --- */
static void *sb_disk_routine(void *thunk)
{
	sb_routine_t    *routine = thunk;
	struct timespec  start_tp;
	struct timespec  finish_tp;;
	long             elapsed_usec;

	memset(&start_tp, 0, sizeof(start_tp));
	memset(&finish_tp, 0, sizeof(finish_tp));

	while(1) {
		clock_gettime(CLOCK_MONOTONIC, &start_tp);

		/* TODO: run routine */

		clock_gettime(CLOCK_MONOTONIC, &finish_tp);
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000);
		if (usleep((routine->interval * 1000000) - elapsed_usec) != 0) {
			fprintf(stderr, "Disk routine: Error sleeping\n");
		}
	}
	
	return NULL;
}


/* --- FAN ROUTINE --- */
static void *sb_fan_routine(void *thunk)
{
	sb_routine_t    *routine = thunk;
	struct timespec  start_tp;
	struct timespec  finish_tp;;
	long             elapsed_usec;

	memset(&start_tp, 0, sizeof(start_tp));
	memset(&finish_tp, 0, sizeof(finish_tp));

	while(1) {
		clock_gettime(CLOCK_MONOTONIC, &start_tp);

		/* TODO: run routine */

		clock_gettime(CLOCK_MONOTONIC, &finish_tp);
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000);
		if (usleep((routine->interval * 1000000) - elapsed_usec) != 0) {
			fprintf(stderr, "Fan routine: Error sleeping\n");
		}
	}
	
	return NULL;
}


/* --- LOAD ROUTINE --- */
static void *sb_load_routine(void *thunk)
{
	sb_routine_t    *routine = thunk;
	struct timespec  start_tp;
	struct timespec  finish_tp;;
	long             elapsed_usec;

	memset(&start_tp, 0, sizeof(start_tp));
	memset(&finish_tp, 0, sizeof(finish_tp));

	while(1) {
		clock_gettime(CLOCK_MONOTONIC, &start_tp);

		/* TODO: run routine */

		clock_gettime(CLOCK_MONOTONIC, &finish_tp);
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000);
		if (usleep((routine->interval * 1000000) - elapsed_usec) != 0) {
			fprintf(stderr, "Load routine: Error sleeping\n");
		}
	}
	
	return NULL;
}


/* --- LOG ROUTINE --- */
static void *sb_log_routine(void *thunk)
{
	sb_routine_t    *routine = thunk;
	struct timespec  start_tp;
	struct timespec  finish_tp;;
	long             elapsed_usec;

	memset(&start_tp, 0, sizeof(start_tp));
	memset(&finish_tp, 0, sizeof(finish_tp));

	while(1) {
		clock_gettime(CLOCK_MONOTONIC, &start_tp);

		/* TODO: run routine */

		clock_gettime(CLOCK_MONOTONIC, &finish_tp);
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000);
		if (usleep((routine->interval * 1000000) - elapsed_usec) != 0) {
			fprintf(stderr, "Log routine: Error sleeping\n");
		}
	}
	
	return NULL;
}


/* --- NETWORK ROUTINE --- */
static void *sb_network_routine(void *thunk)
{
	sb_routine_t    *routine = thunk;
	struct timespec  start_tp;
	struct timespec  finish_tp;;
	long             elapsed_usec;

	memset(&start_tp, 0, sizeof(start_tp));
	memset(&finish_tp, 0, sizeof(finish_tp));

	while(1) {
		clock_gettime(CLOCK_MONOTONIC, &start_tp);

		/* TODO: run routine */

		clock_gettime(CLOCK_MONOTONIC, &finish_tp);
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000);
		if (usleep((routine->interval * 1000000) - elapsed_usec) != 0) {
			fprintf(stderr, "Network routine: Error sleeping\n");
		}
	}
	
	return NULL;
}


/* --- RAM ROUTINE --- */
static void *sb_ram_routine(void *thunk)
{
	sb_routine_t    *routine = thunk;
	struct timespec  start_tp;
	struct timespec  finish_tp;;
	long             elapsed_usec;

	memset(&start_tp, 0, sizeof(start_tp));
	memset(&finish_tp, 0, sizeof(finish_tp));

	while(1) {
		clock_gettime(CLOCK_MONOTONIC, &start_tp);

		/* TODO: run routine */

		clock_gettime(CLOCK_MONOTONIC, &finish_tp);
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000);
		if (usleep((routine->interval * 1000000) - elapsed_usec) != 0) {
			fprintf(stderr, "Ram routine: Error sleeping\n");
		}
	}
	
	return NULL;
}


/* --- TIME ROUTINE --- */
static void *sb_time_routine(void *thunk)
{
	sb_routine_t    *routine = thunk;
	struct timespec  start_tp;
	struct timespec  finish_tp;;
	long             elapsed_usec;

	memset(&start_tp, 0, sizeof(start_tp));
	memset(&finish_tp, 0, sizeof(finish_tp));

	while(1) {
		clock_gettime(CLOCK_MONOTONIC, &start_tp);

		/* TODO: run routine */

		clock_gettime(CLOCK_MONOTONIC, &finish_tp);
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000);

		if (usleep((routine->interval * 1000000) - elapsed_usec) != 0) {
			fprintf(stderr, "Time routine: Error sleeping\n");
		}
	}
	
	return NULL;
}


/* --- TODO ROUTINE --- */
static void *sb_todo_routine(void *thunk)
{
	sb_routine_t    *routine = thunk;
	struct timespec  start_tp;
	struct timespec  finish_tp;;
	long             elapsed_usec;

	memset(&start_tp, 0, sizeof(start_tp));
	memset(&finish_tp, 0, sizeof(finish_tp));

	while(1) {
		clock_gettime(CLOCK_MONOTONIC, &start_tp);

		/* TODO: run routine */

		clock_gettime(CLOCK_MONOTONIC, &finish_tp);
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000);
		if (usleep((routine->interval * 1000000) - elapsed_usec) != 0) {
			fprintf(stderr, "TODO routine: Error sleeping\n");
		}
	}
	
	return NULL;
}


/* --- VOLUME ROUTINE --- */
static void *sb_volume_routine(void *thunk)
{
	sb_routine_t    *routine = thunk;
	struct timespec  start_tp;
	struct timespec  finish_tp;;
	long             elapsed_usec;

	memset(&start_tp, 0, sizeof(start_tp));
	memset(&finish_tp, 0, sizeof(finish_tp));

	while(1) {
		clock_gettime(CLOCK_MONOTONIC, &start_tp);

		/* TODO: run routine */

		clock_gettime(CLOCK_MONOTONIC, &finish_tp);
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000);
		if (usleep((routine->interval * 1000000) - elapsed_usec) != 0) {
			fprintf(stderr, "Volume routine: Error sleeping\n");
		}
	}
	
	return NULL;
}


/* --- WEATHER ROUTINE --- */
static void *sb_weather_routine(void *thunk)
{
	sb_routine_t    *routine = thunk;
	struct timespec  start_tp;
	struct timespec  finish_tp;;
	long             elapsed_usec;

	memset(&start_tp, 0, sizeof(start_tp));
	memset(&finish_tp, 0, sizeof(finish_tp));

	while(1) {
		clock_gettime(CLOCK_MONOTONIC, &start_tp);

		/* TODO: run routine */

		clock_gettime(CLOCK_MONOTONIC, &finish_tp);
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000);
		if (usleep((routine->interval * 1000000) - elapsed_usec) != 0) {
			fprintf(stderr, "Weather routine: Error sleeping\n");
		}
	}
	
	return NULL;
}


/* --- WIFI ROUTINE --- */
static int sb_init_wifi(int *fd, struct iwreq *iwr, char *essid)
{
	struct ifaddrs *ifaddrs = NULL;
	struct ifaddrs *ifap;

	memset(essid, 0, IW_ESSID_MAX_SIZE + 1);
	iwr->u.essid.pointer = (caddr_t *)essid;
	iwr->u.data.length   = IW_ESSID_MAX_SIZE + 1;
	iwr->u.data.flags    = 0;

	/* open socket and return file descriptor for it */
	*fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (*fd < 0) {
		fprintf(stderr, "Wifi routine: Error opening socket file descriptor\n");
		return -1;
	}

	/* get all network interfaces */
	if (getifaddrs(&ifaddrs) < 0 || ifaddrs == NULL) {
		fprintf(stderr, "Wifi routine: Error finding interface addresses\n");
		return -1;
	}
	ifap = ifaddrs;

	/* go through each interface until one returns an ssid */
	while (ifap != NULL) {
		strncpy(iwr->ifr_ifrn.ifrn_name, ifap->ifa_name, IFNAMSIZ);
		if (ioctl(*fd, SIOCGIWESSID, iwr) >= 0) {
			freeifaddrs(ifaddrs);
			return 1;
		}
		ifap = ifap->ifa_next;
	}

	fprintf(stderr, "Wifi routine: Could not find wireless interface\n");
	freeifaddrs(ifaddrs);
	return -1;
}

static void *sb_wifi_routine(void *thunk)
{
	/* TODO:
	 * if sb_init_wifi() fails, try again after interval sleep
	 * handle break and reattach at a later time
	 */
	sb_routine_t    *routine = thunk;
	struct timespec  start_tp;
	struct timespec  finish_tp;;
	long             elapsed_usec;
	int              fd;
	struct iwreq     iwr;
	char             essid[IW_ESSID_MAX_SIZE + 1];

	memset(&start_tp, 0, sizeof(start_tp));
	memset(&finish_tp, 0, sizeof(finish_tp));

	if (sb_init_wifi(&fd, &iwr, essid) < 0)
		return NULL;

	while(1) {
		clock_gettime(CLOCK_MONOTONIC, &start_tp);

		memset(essid, 0, sizeof(essid));
		if (ioctl(fd, SIOCGIWESSID, &iwr) < 0) {
			fprintf(stderr, "Wifi routine: Error getting SSID\n");
			break;
		}

		pthread_mutex_lock(&(routine->mutex));
		strncpy(routine->output, essid, sizeof(essid)-1);
		pthread_mutex_unlock(&(routine->mutex));

		clock_gettime(CLOCK_MONOTONIC, &finish_tp);
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000);
		if (usleep((routine->interval * 1000000) - elapsed_usec) != 0) {
			fprintf(stderr, "Wifi routine: Error sleeping\n");
		}
	}

	close(fd);
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
	sb_routine_t      *routine_object;
	pthread_t          print_thread;
	void              *join_ret = NULL;

	num_routines = sizeof(chosen_routines) / sizeof(*chosen_routines);
	if (num_routines < 1) {
		fprintf(stderr, "No routines chosen, exiting...\n");
		return 1;
	}

	routine_list = routine_array + chosen_routines[0].routine;

	/* step through each routine chosen in config.h and set it up */
	for (i = 0; i < num_routines; i++) {
		index = chosen_routines[i].routine;
		routine_object = routine_array + index;

		/* set flag for this routine */
		sb_flags_active |= 1<<index;

		/* string onto routine list */
		routine_object->next = routine_array + chosen_routines[i+1].routine;

		/* initialize the routine */
		routine_object->routine = chosen_routines[i].routine;
		if (index == DELIMITER) {
			snprintf(routine_object->output, sizeof(routine_object->output), ";");
			routine_object->length = 1;
			continue;
		}
		routine_object->thread_func = possible_routines[index].callback;
		routine_object->interval = chosen_routines[i].seconds;

		/* create thread */
		pthread_mutex_init(&(routine_object->mutex), NULL);
		pthread_create(&(routine_object->thread), NULL, routine_object->thread_func, (void *)routine_object);
	}
	/* properly terminate the routine list */
	routine_object->next = NULL;

	pthread_create(&print_thread, NULL, sb_print_to_sb, NULL);
	pthread_join(print_thread, NULL); /* TODO: remove when threads start going */

	/* block until all threads exit */
	for (i = 0; i < num_routines; i++) {
		index = chosen_routines[i].routine;
		if (index == DELIMITER)
			continue;

		if (pthread_join(routine_array[index].thread, &join_ret) != 0)
			fprintf(stderr, "%s thread did not exit cleanly (%s)\n", routine_names[index], (char *)join_ret);
		free(join_ret);
	}

	return 0;
}
