#include "statusbar.h"
#include "config.h"

#define SBLENGTH 10240

typedef enum _SB_BOOL {
	SB_FALSE = 0,
	SB_TRUE  = 1
} SB_BOOL;

static void *sb_print_to_sb(void *thunk)
{
	int             *run = thunk;
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

	while (*run) {
		clock_gettime(CLOCK_MONOTONIC_RAW, &start_tp);

		offset = 0;
		routine = routine_list;
		while (routine != NULL) {
			if (routine->skip == 1) {
				routine = routine->next;
				continue;
			}
				
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

		clock_gettime(CLOCK_MONOTONIC_RAW, &finish_tp);
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
		clock_gettime(CLOCK_MONOTONIC_RAW, &start_tp);

		/* TODO: run routine */

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output)-1, "backup: TODO");
		pthread_mutex_unlock(&(routine->mutex));

		clock_gettime(CLOCK_MONOTONIC_RAW, &finish_tp);
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000);
		if (usleep((routine->interval * 1000000) - elapsed_usec) != 0) {
			fprintf(stderr, "Backup routine: Error sleeping\n");
		}
	}
	
	routine->skip = 1;
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
		clock_gettime(CLOCK_MONOTONIC_RAW, &start_tp);

		/* TODO: run routine */

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output)-1, "battey: TODO");
		pthread_mutex_unlock(&(routine->mutex));

		clock_gettime(CLOCK_MONOTONIC_RAW, &finish_tp);
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000);
		if (usleep((routine->interval * 1000000) - elapsed_usec) != 0) {
			fprintf(stderr, "Battery routine: Error sleeping\n");
		}
	}
	
	routine->skip = 1;
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
		clock_gettime(CLOCK_MONOTONIC_RAW, &start_tp);

		/* TODO: run routine */

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output)-1, "brightness: TODO");
		pthread_mutex_unlock(&(routine->mutex));

		clock_gettime(CLOCK_MONOTONIC_RAW, &finish_tp);
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000);
		if (usleep((routine->interval * 1000000) - elapsed_usec) != 0) {
			fprintf(stderr, "Brightness routine: Error sleeping\n");
		}
	}

	routine->skip = 1;
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
		clock_gettime(CLOCK_MONOTONIC_RAW, &start_tp);

		/* TODO: run routine */

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output)-1, "cpu temp: TODO");
		pthread_mutex_unlock(&(routine->mutex));

		clock_gettime(CLOCK_MONOTONIC_RAW, &finish_tp);
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000);
		if (usleep((routine->interval * 1000000) - elapsed_usec) != 0) {
			fprintf(stderr, "CPU Temp routine: Error sleeping\n");
		}
	}
	
	routine->skip = 1;
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
		clock_gettime(CLOCK_MONOTONIC_RAW, &start_tp);

		/* TODO: run routine */

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output)-1, "cpu usage: TODO");
		pthread_mutex_unlock(&(routine->mutex));

		clock_gettime(CLOCK_MONOTONIC_RAW, &finish_tp);
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000);
		if (usleep((routine->interval * 1000000) - elapsed_usec) != 0) {
			fprintf(stderr, "CPU Usage routine: Error sleeping\n");
		}
	}
	
	routine->skip = 1;
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
		clock_gettime(CLOCK_MONOTONIC_RAW, &start_tp);

		/* TODO: run routine */

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output)-1, "disk: TODO");
		pthread_mutex_unlock(&(routine->mutex));

		clock_gettime(CLOCK_MONOTONIC_RAW, &finish_tp);
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000);
		if (usleep((routine->interval * 1000000) - elapsed_usec) != 0) {
			fprintf(stderr, "Disk routine: Error sleeping\n");
		}
	}
	
	routine->skip = 1;
	return NULL;
}


/* --- FAN ROUTINE --- */
struct sb_fan {
	char  path[512];
	long  min;
	long  max;
	FILE *fd;
};

static SB_BOOL sb_open_fans(struct sb_fan *fans, int fan_count)
{
	int     i;
	SB_BOOL ret = SB_TRUE;

	for (i=0; i<fan_count; i++) {
		fans[i].fd = fopen(fans[i].path, "r");
		if (fans[i].fd == NULL) {
			fprintf(stderr, "Fan routine: Error opening %s\n", fans[i].path);
			ret = SB_FALSE;
			/* close all open file descriptors */
			for (--i; i>=0; i--) {
				fclose(fans[i].fd);
			}
			break;
		}
	}
	
	return ret;
}

static int sb_read_fan_speeds(char *fan, char *condition)
{
	char  path[512];
	FILE *fd;
	char  buf[64];

	snprintf(path, sizeof(path)-1, "%s%s", fan, condition);
	fd = fopen(path, "r");
	if (fd == NULL) {
		fprintf(stderr, "Fan routine: Failed to open %s\n", path);
		return -1;
	}

	if (fgets(buf, sizeof(buf)-1, fd) == NULL) {
		fprintf(stderr, "Fan routine: Failed to read %s\n", path);
		fclose(fd);
		return -1;
	}
	fclose(fd);

	return atol(buf);
}

static SB_BOOL sb_find_fans(struct sb_fan *fans, int *count)
{
	static const char *base      = "/sys/class/hwmon";
	DIR               *dir;
	struct dirent     *dirent;
	char               path[512] = {0};
	DIR               *device;

	*count = 0;

	dir = opendir(base);
	if (dir == NULL) {
		fprintf(stderr, "Fan routine: Could not open directory /sys/class/hwmon\n");
		return SB_FALSE;
	}

	for (dirent=readdir(dir); dirent!=NULL; dirent=readdir(dir)) {
		snprintf(path, sizeof(path)-1, "%s/%s/device", base, dirent->d_name);
		device = opendir(path);
		if (device != NULL) {
			for (dirent=readdir(device); dirent!=NULL; dirent=readdir(device)) {
				if (!strncmp(dirent->d_name, "fan", 3) && !strncmp(dirent->d_name+4, "_output", 7)) {
					snprintf(fans[*count].path, sizeof(fans[*count].path)-1, "%s/%.4s", path, dirent->d_name);
					fans[*count].min = sb_read_fan_speeds(fans[*count].path, "_min");
					fans[*count].max = sb_read_fan_speeds(fans[*count].path, "_max");
					if (fans[*count].min < 0 || fans[*count].max < 0)
						break;
					strncat(fans[*count].path, "_output", sizeof(fans[*count].path)-strlen(fans[*count].path));
					(*count)++;
				}
			}
			closedir(device);
			if (*count > 0) {
				break;
			}
		}
	}
	
	closedir(dir);
	if (count == 0) {
		fprintf(stderr, "Fan routine: No fans found\n");
		return SB_FALSE;
	}
	return SB_TRUE;
}

static void *sb_fan_routine(void *thunk)
{
	sb_routine_t    *routine = thunk;
	struct timespec  start_tp;
	struct timespec  finish_tp;;
	long             elapsed_usec;

	struct sb_fan    fans[64];
	int              count   = 0;
	int              i;
	SB_BOOL          error;
	char             buf[64] = {0};
	long             speed;
	long             percent;
	long             average;

	memset(&start_tp, 0, sizeof(start_tp));
	memset(&finish_tp, 0, sizeof(finish_tp));

	memset(fans, 0, sizeof(fans));
	if (!sb_find_fans(fans, &count))
		return NULL;
	if (!sb_open_fans(fans, count))
		return NULL;

	while(1) {
		clock_gettime(CLOCK_MONOTONIC_RAW, &start_tp);

		error   = SB_FALSE;
		average = 0;
		for (i=0; i<count && !error; i++) {
			if (lseek(fileno(fans[i].fd), 0L, SEEK_SET) < 0) {
				fprintf(stderr, "Fan routine: Error resetting file offset\n");
				error = SB_TRUE;
			} else if (fgets(buf, sizeof(buf), fans[i].fd) == NULL) {
				fprintf(stderr, "Fan routine: Error reading %s\n", fans[i].path);
				error = SB_TRUE;
			}
			speed = atol(buf);
			if (speed < 0) {
				fprintf(stderr, "Fan routine: Failed to parse %s\n", fans[i].path);
				error = SB_TRUE;
			}
			percent += ((speed - fans[i].min) * 100) / (fans[i].max - fans[i].min);
			if (percent < 0)
				percent = 0;
			if (percent > 100)
				percent = 100;
			average += percent;
		}
		if (error)
			break;

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output)-1, "fan speed: %ld%%", average / count);
		pthread_mutex_unlock(&(routine->mutex));

		clock_gettime(CLOCK_MONOTONIC_RAW, &finish_tp);
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000);
		if (usleep((routine->interval * 1000000) - elapsed_usec) != 0) {
			fprintf(stderr, "Fan routine: Error sleeping\n");
		}
	}

	/* close open file descriptors */
	for (i=0; i<count; i++)
		fclose(fans[i].fd);
	
	routine->skip = 1;
	return NULL;
}


/* --- LOAD ROUTINE --- */
static void *sb_load_routine(void *thunk)
{
	sb_routine_t    *routine = thunk;
	struct timespec  start_tp;
	struct timespec  finish_tp;;
	long             elapsed_usec;

	FILE            *fd;
	char             buf[64] = {0};
	double           av[3];

	memset(&start_tp, 0, sizeof(start_tp));
	memset(&finish_tp, 0, sizeof(finish_tp));

	fd = fopen("/proc/loadavg", "r");
	if (fd == NULL) {
		fprintf(stderr, "Load routine: Error opening loadavg\n");
		return NULL;
	}

	while(1) {
		clock_gettime(CLOCK_MONOTONIC_RAW, &start_tp);

		if (lseek(fileno(fd), 0L, SEEK_SET) < 0) {
			fprintf(stderr, "Load routine: Error resetting file offset\n");
			break;
		} else if (fgets(buf, sizeof(buf), fd) == NULL) {
			fprintf(stderr, "Load routine: Error reading loadavg file\n");
			break;
		} else if (sscanf(buf, "%lf %lf %lf", &av[0], &av[1], &av[2]) < 3) {
			fprintf(stderr, "Load routine: Error scanning buffer\n");
			break;
		}

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output)-1, "load: %.2f, %.2f, %.2f", av[0], av[1], av[2]);
		pthread_mutex_unlock(&(routine->mutex));

		clock_gettime(CLOCK_MONOTONIC_RAW, &finish_tp);
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000);
		if (usleep((routine->interval * 1000000) - elapsed_usec) != 0) {
			fprintf(stderr, "Load routine: Error sleeping\n");
		}
	}
	
	if (fd != NULL)
		fclose(fd);
	routine->skip = 1;
	return NULL;
}


/* --- NETWORK ROUTINE --- */
static SB_BOOL sb_open_files(FILE **rxfd, char *rx_path, FILE **txfd, char *tx_path)
{
	*rxfd = fopen(rx_path, "r");
	if (*rxfd < 0) {
		fprintf(stderr, "Network routine: Error opening rx file: %s\n", rx_path);
		return SB_FALSE;
	}

	*txfd = fopen(tx_path, "r");
	if (*txfd < 0) {
		fprintf(stderr, "Network routine: Error opening tx file: %s\n", tx_path);
		fclose(*rxfd);
		return SB_FALSE;
	}

	return SB_TRUE;
}

static SB_BOOL sb_get_paths(char *rx_path, size_t rx_path_size, char *tx_path, size_t tx_path_size)
{
	int             fd;
	struct ifreq    ifr;
	struct ifaddrs *ifaddrs = NULL;
	struct ifaddrs *ifap;

	/* open socket and return file descriptor for it */
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		fprintf(stderr, "Network routine: Error opening socket file descriptor\n");
		return SB_FALSE;
	}

	/* get all network interfaces */
	if (getifaddrs(&ifaddrs) < 0 || ifaddrs == NULL) {
		fprintf(stderr, "Network routine: Error finding interface addresses\n");
		close(fd);
		return SB_FALSE;
	}
	ifap = ifaddrs;

	/* go through each interface until we find an active one */
	while (ifap != NULL) {
		strncpy(ifr.ifr_name, ifap->ifa_name, IFNAMSIZ);
		if (ioctl(fd, SIOCGIFFLAGS, &ifr) >= 0) {
			if (ifr.ifr_flags & IFF_RUNNING && !(ifr.ifr_flags & IFF_LOOPBACK)) {
				snprintf(rx_path, rx_path_size, "/sys/class/net/%s/statistics/rx_bytes", ifap->ifa_name);
				snprintf(tx_path, tx_path_size, "/sys/class/net/%s/statistics/tx_bytes", ifap->ifa_name);
				break;
			}
		}
		ifap = ifap->ifa_next;
	}

	close(fd);
	freeifaddrs(ifaddrs);

	if (ifap == NULL) {
		fprintf(stderr, "Network routine: Could not find wireless interface\n");
		return SB_FALSE;
	}

	return SB_TRUE;
}

static void *sb_network_routine(void *thunk)
{
	sb_routine_t    *routine = thunk;
	struct timespec  start_tp;
	struct timespec  finish_tp;;
	long             elapsed_usec;

	int              i;
	SB_BOOL          error;
	int              prefix;
	char            *unit    = "KMGTP";
	struct {
		FILE *fd;
		char  path[IFNAMSIZ+64];
		char  buf[64];
		long  old_bytes;
		long  new_bytes;
		long  diff;
		int   prefix;
	} files[2]               = {0};

	if (!sb_get_paths(files[0].path, sizeof(files[0].path), files[1].path, sizeof(files[1].path)))
		return NULL;
	if (!sb_open_files(&files[0].fd, files[0].path, &files[1].fd, files[1].path))
		return NULL;

	memset(&start_tp, 0, sizeof(start_tp));
	memset(&finish_tp, 0, sizeof(finish_tp));

	while(1) {
		clock_gettime(CLOCK_MONOTONIC_RAW, &start_tp);

		error = SB_FALSE;
		for (i=0; i<2 && !error; i++) {
			if (lseek(fileno(files[i].fd), 0L, SEEK_SET) < 0) {
				fprintf(stderr, "Network routine: Error resetting file offset\n");
				error = SB_TRUE;
			} else if (fgets(files[i].buf, sizeof(files[i].buf), files[i].fd) == NULL) {
				fprintf(stderr, "Network routine: Error reading network file\n");
				error = SB_TRUE;
			} else {
				files[i].old_bytes = files[i].new_bytes;
				files[i].new_bytes = atol(files[i].buf);
				files[i].diff      = files[i].new_bytes - files[i].old_bytes;
				for (prefix=0; (files[i].diff >> (10*(prefix+2))) > 0; prefix++);
				files[i].prefix    = prefix;
			}
		}
		if (error)
			break;

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output)-1, "Down: %.1f %c Up: %.1f %c",
				(files[0].diff >> (10*files[0].prefix)) / 1024.0, unit[files[0].prefix],
				(files[1].diff >> (10*files[1].prefix)) / 1024.0, unit[files[1].prefix]);
		pthread_mutex_unlock(&(routine->mutex));

		clock_gettime(CLOCK_MONOTONIC_RAW, &finish_tp);
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000);
		if (usleep((routine->interval * 1000000) - elapsed_usec) != 0) {
			fprintf(stderr, "Network routine: Error sleeping\n");
		}
	}
	
	if (files[0].fd != NULL)
		fclose(files[0].fd);
	if (files[1].fd != NULL)
		fclose(files[1].fd);
	routine->skip = 1;
	return NULL;
}


/* --- RAM ROUTINE --- */
static void *sb_ram_routine(void *thunk)
{
	sb_routine_t    *routine = thunk;
	struct timespec  start_tp;
	struct timespec  finish_tp;;
	long             elapsed_usec;

	long             page_size;
	long             total_pages;
	long             total_bytes;
	int              i;
	float            total_bytes_f;
	char             unit[] = "KMGTP";
	long             available_bytes;

	memset(&start_tp, 0, sizeof(start_tp));
	memset(&finish_tp, 0, sizeof(finish_tp));

	page_size   = sysconf(_SC_PAGESIZE);
	total_pages = sysconf(_SC_PHYS_PAGES);
	if (page_size < 0 || total_pages < 0) {
		fprintf(stderr, "Ram routine: Error getting page info\n");
		return NULL;
	}

	/* calculate unit of memory */
	total_bytes = total_pages * page_size;
	for (i=0; (total_bytes >> (10*(i+2))) > 0; i++);

	/* get total bytes as a decimal */
	total_bytes_f = ((total_pages*page_size) >> (10*i)) / 1024.0;

	while(1) {
		clock_gettime(CLOCK_MONOTONIC_RAW, &start_tp);

		/* get available memory */
		available_bytes = sysconf(_SC_AVPHYS_PAGES) * page_size;
		if (available_bytes < 0) {
			fprintf(stderr, "Ram routine: Error getting available bytes\n");
			break;
		}

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output)-1, "Free: %.1f %c / %.1f %c",
				(available_bytes >> (10*i)) / 1024.0, unit[i], total_bytes_f, unit[i]);
		pthread_mutex_unlock(&(routine->mutex));

		clock_gettime(CLOCK_MONOTONIC_RAW, &finish_tp);
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000);
		if (usleep((routine->interval * 1000000) - elapsed_usec) != 0) {
			fprintf(stderr, "Ram routine: Error sleeping\n");
		}
	}
	
	routine->skip = 1;
	return NULL;
}


/* --- TIME ROUTINE --- */
static void *sb_time_routine(void *thunk)
{
	sb_routine_t    *routine = thunk;
	struct timespec  start_tp;
	struct timespec  finish_tp;;
	long             elapsed_usec;

	struct tm        tm;

	memset(&start_tp, 0, sizeof(start_tp));
	memset(&finish_tp, 0, sizeof(finish_tp));

	while(1) {
		clock_gettime(CLOCK_REALTIME, &start_tp);

		/* convert time in seconds since epoch to local time */
		memset(&tm, 0, sizeof(tm));
		localtime_r(&start_tp.tv_sec, &tm);

		pthread_mutex_lock(&(routine->mutex));
		strftime(routine->output, sizeof(routine->output)-1, time_format, &tm);
		pthread_mutex_unlock(&(routine->mutex));

		clock_gettime(CLOCK_MONOTONIC_RAW, &finish_tp);
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000);

		if (usleep((routine->interval * 1000000) - elapsed_usec) != 0) {
			fprintf(stderr, "Time routine: Error sleeping\n");
		}
	}
	
	routine->skip = 1;
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
		clock_gettime(CLOCK_MONOTONIC_RAW, &start_tp);

		/* TODO: run routine */

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output)-1, "todo: TODO");
		pthread_mutex_unlock(&(routine->mutex));

		clock_gettime(CLOCK_MONOTONIC_RAW, &finish_tp);
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000);
		if (usleep((routine->interval * 1000000) - elapsed_usec) != 0) {
			fprintf(stderr, "TODO routine: Error sleeping\n");
		}
	}
	
	routine->skip = 1;
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
		clock_gettime(CLOCK_MONOTONIC_RAW, &start_tp);

		/* TODO: run routine */

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output)-1, "volume: TODO");
		pthread_mutex_unlock(&(routine->mutex));

		clock_gettime(CLOCK_MONOTONIC_RAW, &finish_tp);
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000);
		if (usleep((routine->interval * 1000000) - elapsed_usec) != 0) {
			fprintf(stderr, "Volume routine: Error sleeping\n");
		}
	}
	
	routine->skip = 1;
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
		clock_gettime(CLOCK_MONOTONIC_RAW, &start_tp);

		/* TODO: run routine */

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output)-1, "weather: TODO");
		pthread_mutex_unlock(&(routine->mutex));

		clock_gettime(CLOCK_MONOTONIC_RAW, &finish_tp);
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000);
		if (usleep((routine->interval * 1000000) - elapsed_usec) != 0) {
			fprintf(stderr, "Weather routine: Error sleeping\n");
		}
	}
	
	routine->skip = 1;
	return NULL;
}


/* --- WIFI ROUTINE --- */
static SB_BOOL sb_init_wifi(int *fd, struct iwreq *iwr, char *essid, size_t max_len)
{
	struct ifaddrs *ifaddrs = NULL;
	struct ifaddrs *ifap;

	memset(essid, 0, max_len);
	iwr->u.essid.pointer = (caddr_t *)essid;
	iwr->u.data.length   = max_len;
	iwr->u.data.flags    = 0;

	/* open socket and return file descriptor for it */
	*fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (*fd < 0) {
		fprintf(stderr, "Wifi routine: Error opening socket file descriptor\n");
		return SB_FALSE;
	}

	/* get all network interfaces */
	if (getifaddrs(&ifaddrs) < 0 || ifaddrs == NULL) {
		fprintf(stderr, "Wifi routine: Error finding interface addresses\n");
		return SB_FALSE;
	}
	ifap = ifaddrs;

	/* go through each interface until one returns an ssid */
	while (ifap != NULL) {
		strncpy(iwr->ifr_ifrn.ifrn_name, ifap->ifa_name, IFNAMSIZ);
		if (ioctl(*fd, SIOCGIWESSID, iwr) >= 0) {
			freeifaddrs(ifaddrs);
			return SB_TRUE;
		}
		ifap = ifap->ifa_next;
	}

	fprintf(stderr, "Wifi routine: Could not find wireless interface\n");
	freeifaddrs(ifaddrs);
	return SB_FALSE;
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

	if (!sb_init_wifi(&fd, &iwr, essid, sizeof(essid))) {
		close(fd);
		return NULL;
	}

	while(1) {
		clock_gettime(CLOCK_MONOTONIC_RAW, &start_tp);

		memset(essid, 0, sizeof(essid));
		if (ioctl(fd, SIOCGIWESSID, &iwr) < 0) {
			fprintf(stderr, "Wifi routine: Error getting SSID\n");
			break;
		}

		pthread_mutex_lock(&(routine->mutex));
		strncpy(routine->output, essid, sizeof(routine->output)-1);
		pthread_mutex_unlock(&(routine->mutex));

		clock_gettime(CLOCK_MONOTONIC_RAW, &finish_tp);
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000);
		if (usleep((routine->interval * 1000000) - elapsed_usec) != 0) {
			fprintf(stderr, "Wifi routine: Error sleeping\n");
		}
	}

	close(fd);
	routine->skip = 1;
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
	int                run      = 1;
	void              *join_ret = NULL;

	num_routines = sizeof(chosen_routines) / sizeof(*chosen_routines);
	if (num_routines < 1) {
		fprintf(stderr, "No routines chosen, exiting...\n");
		return 1;
	}

	routine_list = routine_array + chosen_routines[0].routine;

	/* step through each routine chosen in config.h and set it up */
	for (i=0; i<num_routines; i++) {
		index          = chosen_routines[i].routine;
		routine_object = routine_array + index;

		/* set flag for this routine */
		sb_flags_active |= 1<<index;

		/* string onto routine list */
		routine_object->next = routine_array + chosen_routines[i+1].routine;

		/* initialize the routine */
		routine_object->routine = chosen_routines[i].routine;
		if (index == DELIMITER) {
			snprintf(routine_object->output, sizeof(routine_object->output)-1, ";");
			routine_object->length = 1;
			continue;
		}
		routine_object->thread_func = possible_routines[index].callback;
		routine_object->interval    = chosen_routines[i].seconds;

		/* create thread */
		pthread_mutex_init(&(routine_object->mutex), NULL);
		pthread_create(&(routine_object->thread), NULL, routine_object->thread_func, (void *)routine_object);
	}
	/* properly terminate the routine list */
	routine_object->next = NULL;

	pthread_create(&print_thread, NULL, sb_print_to_sb, (void *)&run);

	/* block until all threads exit */
	for (i=0; i<num_routines; i++) {
		index          = chosen_routines[i].routine;
		routine_object = routine_array + index;
		if (index == DELIMITER)
			continue;

		if (pthread_join(routine_object->thread, &join_ret) != 0)
			fprintf(stderr, "%s thread did not exit cleanly (%s)\n", routine_names[index], (char *)join_ret);
		if (pthread_mutex_destroy(&(routine_object->mutex)) != 0)
			fprintf(stderr, "%s: error destroying mutex\n", routine_names[index]);
		routine_object->skip = 1; /* make sure routine is skipped */
		free(join_ret);
	}
	/* kill print thread */
	run = 0;
	if (pthread_join(print_thread, &join_ret) != 0)
		fprintf(stderr, "print thread did not exit cleanly (%s)\n", (char *)join_ret);
	free(join_ret);

	return 0;
}
