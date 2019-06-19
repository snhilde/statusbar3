#include "statusbar.h"
#include "config.h"

#define SBLENGTH 10240

#define SB_TIMER_VARS \
	struct timespec  start_tp  = {0};   \
	struct timespec  finish_tp = {0};   \
	long             elapsed_usec;

#define SB_START_TIMER \
	clock_gettime(CLOCK_MONOTONIC_RAW, &start_tp);

#define SB_STOP_TIMER \
		clock_gettime(CLOCK_MONOTONIC_RAW, &finish_tp);

#define SB_SLEEP \
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000); \
		if (usleep((routine->interval * 1000000) - elapsed_usec) != 0) { \
			fprintf(stderr, "%s routine: Error sleeping\n", routine_names[routine->routine]); \
		}

/* --- HELPER FUNCTIONS --- */
static float sb_calc_magnitude(long number, char *prefix)
{
	int   i;
	long  num     = number;
	char *symbols = "BKMGTP";

	if (number < 1024) {
		*prefix = 'B';
		return number * 1.0;
	}

	/* This will calculate (roughly) how many commas the number would have by
	 * shifting it 3 places to the right (or, more precisely, dividing by 1024)
	 * until the number is consumed. We are going to start at 1 because 0 would
	 * just be skipped (nothing would happen). We are adding 1 to every bitshift
	 * operation because we need to save a thousandth place for later dividing
	 * into a floating-point number. If we didn't add the one 1, the number
	 * would lose its decimal places and precision. */
	for (i=1; (num >> (10*(i+1))) > 0; i++);

	*prefix = symbols[i];
	return (number >> (10*(i-1))) / 1024.0;
}


/* --- PRINTING THREAD --- */
static void *sb_print_to_sb(void *thunk)
{
	SB_TIMER_VARS
	int          *run = thunk;
	Display      *dpy;
	Window        root;

	char          full_output[SBLENGTH];
	size_t        offset;
	sb_routine_t *routine;
	size_t        len;

	dpy  = XOpenDisplay(NULL);
	root = RootWindow(dpy, DefaultScreen(dpy));

	memset(&start_tp, 0, sizeof(start_tp));
	memset(&finish_tp, 0, sizeof(finish_tp));

	while (*run) {
		SB_START_TIMER;

		offset  = 0;
		routine = routine_list;
		while (routine != NULL) {
			if (routine->skip == SB_TRUE) {
				routine = routine->next;
				continue;
			}

			pthread_mutex_lock(&(routine->mutex));
			len = strlen(routine->output);
			if (offset+len > SBLENGTH-1) {
				fprintf(stderr, "Print: Exceeded max output length\n");
				break;
			}

			memcpy(full_output+offset, routine->output, len);
			offset += len;

			pthread_mutex_unlock(&(routine->mutex));
			routine = routine->next;
		}
		full_output[offset] = '\0';

		if (!XStoreName(dpy, root, full_output)) {
			fprintf(stderr, "Print: Failed to set root name\n");
			break;
		}
		if (!XFlush(dpy)) {
			fprintf(stderr, "Print: Failed to flush output buffer\n");
			break;
		}

		SB_STOP_TIMER;
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
	SB_TIMER_VARS;
	sb_routine_t *routine = thunk;

	routine->skip = SB_FALSE;
	while(1) {
		SB_START_TIMER;

		/* TODO: run routine */

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output)-1, "backup: TODO");
		pthread_mutex_unlock(&(routine->mutex));

		SB_STOP_TIMER;
		SB_SLEEP;
	}

	routine->skip = SB_TRUE;
	return NULL;
}


/* --- BATTERY ROUTINE --- */
struct sb_bat_t {
	char path[512];
	long max;
	long now;
	int  status; /* -1 = discharging, 0 = full, 1 = charging */
};

static long sb_bat_read_max(const char *bat, const char *file)
{
	char  path[512];
	FILE *fd;
	char  buf[64];

	snprintf(path, sizeof(path)-1, "%s%s", bat, file);
	fd = fopen(path, "r");
	if (fd == NULL) {
		fprintf(stderr, "Battery routine: Failed to open %s\n", path);
		return -1;
	}

	if (fgets(buf, sizeof(buf)-1, fd) == NULL) {
		fprintf(stderr, "Battery routine: Failed to read %s\n", path);
		fclose(fd);
		return -1;
	}
	fclose(fd);

	return atol(buf);
}

static SB_BOOL sb_bat_find_bat(struct sb_bat_t *bat)
{
	/* Here, we're going to check each device in /sys/class/power_supply. When we find
 	 * a battery, we'll break out of the loop and record some general data.
	 */
	static const char *base      = "/sys/class/power_supply";
	DIR               *dir;
	struct dirent     *dirent;
	char               path[512];
	char               buf[512];
	FILE              *fd;
	SB_BOOL            found_bat = SB_FALSE;

	dir = opendir(base);
	if (dir == NULL) {
		fprintf(stderr, "Battery routine: Failed to open %s\n", base);
		return SB_FALSE;
	}

	/* step through each device, looking for file "type" with value "battery" */
	for (dirent=readdir(dir); dirent!=NULL && !found_bat; dirent=readdir(dir)) {
		if (!strcmp(dirent->d_name, ".") || !strcmp(dirent->d_name, ".."))
			continue;

		snprintf(path, sizeof(path)-1, "%s/%s/type", base, dirent->d_name);
		fd = fopen(path, "r");
		if (fd == NULL) {
			fprintf(stderr, "Battery routine: Failed to open %s", path);
		} else if (fgets(buf, sizeof(buf)-1, fd) == NULL) {
			fprintf(stderr, "Battery routine: Failed to read %s\n", path);
		} else if (!strncasecmp(buf, "Battery", 7)) {
			snprintf(bat->path, sizeof(bat->path)-1, "%s/%s/", base, dirent->d_name);
			found_bat = SB_TRUE;
		}
		if (fd != NULL)
			fclose(fd);
	}
	closedir(dir);

	if (found_bat) {
		bat->max = sb_bat_read_max(bat->path, "charge_full");
		if (bat->max < 0)
			return SB_FALSE;
		strncat(bat->path, "charge_now", sizeof(bat->path)-strlen(bat->path)-1);
	} else {
		fprintf(stderr, "Battery routine: Failed to find battery");
		return SB_FALSE;
	}

	return SB_TRUE;
}

static void *sb_battery_routine(void *thunk)
{
	SB_TIMER_VARS;
	sb_routine_t    *routine = thunk;
	struct sb_bat_t  bat;
	FILE            *fd;

	memset(&bat, 0, sizeof(bat));
	if (!sb_bat_find_bat(&bat))
		return NULL;

	routine->skip = SB_FALSE;
	while(1) {
		SB_START_TIMER;

		fd = fopen(bat.path, "r");
		if (fd == NULL) {
			fprintf(stderr, "Battery routine: Failed to open %s\n", bat.path);
		} else if (fscanf(fd, "%ld", &bat.now) < 1) {
			fprintf(stderr, "Battery routine: Failed to read %s\n", bat.path);
		} else if (fclose(fd) != 0) {
			fprintf(stderr, "Battery routine: Failed to close %s\n", bat.path);
		}

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output)-1, "bat: %ld%%", (bat.now * 100) / bat.max);
		pthread_mutex_unlock(&(routine->mutex));

		SB_STOP_TIMER;
		SB_SLEEP;
	}

	if (fd != NULL)
		fclose(fd);
	routine->skip = SB_TRUE;
	return NULL;
}


/* --- CPU TEMP ROUTINE --- */
static void *sb_cpu_temp_routine(void *thunk)
{
	SB_TIMER_VARS;
	sb_routine_t *routine = thunk;

	routine->skip = SB_FALSE;
	while(1) {
		SB_START_TIMER;

		/* TODO: run routine */

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output)-1, "cpu temp: TODO");
		pthread_mutex_unlock(&(routine->mutex));

		SB_STOP_TIMER;
		SB_SLEEP;
	}

	routine->skip = SB_TRUE;
	return NULL;
}


/* --- CPU USAGE ROUTINE --- */
static void *sb_cpu_usage_routine(void *thunk)
{
	/* TODO: why is sysconf(_SC_NPROCESSORS_ONLN) or num cores not necessary here? */
	SB_TIMER_VARS;
	sb_routine_t      *routine = thunk;
	FILE              *fd;
	static const char *path    = "/proc/stat";
	unsigned long      used;
	unsigned long      total;
	struct {
		unsigned long user;
		unsigned long nice;
		unsigned long system;
		unsigned long idle;
	} old, new;

	routine->skip = SB_FALSE;
	while(1) {
		SB_START_TIMER;

		fd = fopen(path, "r");
		if (fd == NULL) {
			fprintf(stderr, "CPU Usage routine: Failed to open %s\n", path);
			break;
		} else if (fscanf(fd, "cpu %lu %lu %lu %lu", &new.user, &new.nice, &new.system, &new.idle) < 4) {
			fprintf(stderr, "CPU Usage routine: Failed to read %s\n", path);
			break;
		} else if (fclose(fd) != 0) {
			fprintf(stderr, "CPU Usage routine: Failed to close %s\n", path);
			break;
		}

		used  = (new.user-old.user) + (new.nice-old.nice) + (new.system-old.system);
		total = (new.user-old.user) + (new.nice-old.nice) + (new.system-old.system) + (new.idle-old.idle);

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output)-1, "CPU usage: %lu%%", (used*100)/total);
		pthread_mutex_unlock(&(routine->mutex));

		old.user   = new.user;
		old.nice   = new.nice;
		old.system = new.system;
		old.idle   = new.idle;

		SB_STOP_TIMER;
		SB_SLEEP;
	}

	if (fd != NULL)
		fclose(fd);
	routine->skip = SB_TRUE;
	return NULL;
}


/* --- DISK ROUTINE --- */
static void *sb_disk_routine(void *thunk)
{
	SB_TIMER_VARS;
	sb_routine_t   *routine = thunk;
	size_t          num_filesystems;
	int             i;
	struct statvfs  stats;
	SB_BOOL         error   = SB_FALSE;
	float           avail;
	char            avail_prefix;
	float           total;
	char            total_prefix;
	char            output[512];

	routine->skip = SB_FALSE;
	while(1) {
		SB_START_TIMER;

		/* In this routine, we're going to lock the mutex for the entire operation so we
		 * can safely add to the routine's output for the entire loop. */
		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output)-1, "Disk: ");
		num_filesystems = sizeof(filesystems) / sizeof(*filesystems);
		for (i=0; i<num_filesystems; i++) {
			if (statvfs(filesystems[i].path, &stats) != 0) {
				fprintf(stderr, "Disk routine: Failed to get stats for %s", filesystems[i].path);
				error = SB_TRUE;
				break;
			}
			avail = sb_calc_magnitude(stats.f_bfree *stats.f_bsize, &avail_prefix);
			total = sb_calc_magnitude(stats.f_blocks*stats.f_bsize, &total_prefix);
			snprintf(output, sizeof(output)-1, "%s: %.1f%c/%.1f%c",
					filesystems[i].display_name, avail, avail_prefix, total, total_prefix);
			strncat(routine->output, output, sizeof(routine->output)-strlen(routine->output)-1);

			if (i+1 < num_filesystems)
				strncat(routine->output, ", ", sizeof(routine->output)-strlen(routine->output)-1);
		}
		pthread_mutex_unlock(&(routine->mutex));

		if (error)
			break;

		SB_STOP_TIMER;
		SB_SLEEP;
	}

	routine->skip = SB_TRUE;
	return NULL;
}


/* --- FAN ROUTINE --- */
struct sb_fan_t {
	char  path[512];
	long  min;
	long  max;
};

static long sb_read_fan_speeds(const char *fan, const char *file)
{
	char  path[512];
	FILE *fd;
	char  buf[64];

	snprintf(path, sizeof(path)-1, "%s%s", fan, file);
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

static SB_BOOL sb_find_fans(struct sb_fan_t *fans, int *count)
{
	/* We don't know which hardware monitor we want, so w're going to peek into each
	 * device listed in /sys/class/hwmon. If there's a device folder, we'll scan that
	 * for every fan with the name fan#_output, where # is a number between 0 and 9. For
	 * every fan that we find, we'll save the path to the fan#_output and read the min
	 * and max values from fan#_min and fan#_max.
	 * */
	static const char *base      = "/sys/class/hwmon";
	DIR               *dir;
	struct dirent     *dirent;
	char               path[512] = {0};
	DIR               *device;

	*count = 0;

	dir = opendir(base);
	if (dir == NULL) {
		fprintf(stderr, "Fan routine: Failed to open %s\n", base);
		return SB_FALSE;
	}

	/* step through each file/directory in the base and try to open a subdirectory called device */
	for (dirent=readdir(dir); dirent!=NULL; dirent=readdir(dir)) {
		snprintf(path, sizeof(path)-1, "%s/%s/device", base, dirent->d_name);
		device = opendir(path);
		if (device != NULL) {
			/* step through each file in base/hwmon#/device and find any fans */
			for (dirent=readdir(device); dirent!=NULL; dirent=readdir(device)) {
				if (!strncmp(dirent->d_name, "fan", 3) && !strncmp(dirent->d_name+4, "_output", 7)) {
					snprintf(fans[*count].path, sizeof(fans[*count].path)-1, "%s/%.4s", path, dirent->d_name);
					fans[*count].min = sb_read_fan_speeds(fans[*count].path, "_min");
					fans[*count].max = sb_read_fan_speeds(fans[*count].path, "_max");
					if (fans[*count].min < 0 || fans[*count].max < 0)
						break;
					strncat(fans[*count].path, "_output", sizeof(fans[*count].path)-strlen(fans[*count].path-1));
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
	if (*count == 0) {
		fprintf(stderr, "Fan routine: No fans found\n");
		return SB_FALSE;
	}
	return SB_TRUE;
}

static void *sb_fan_routine(void *thunk)
{
	SB_TIMER_VARS;
	sb_routine_t    *routine = thunk;
	struct sb_fan_t  fans[64];
	int              count   = 0;
	int              i;
	FILE            *fd;
	SB_BOOL          error;
	long             speed;
	long             percent;
	long             average;

	memset(fans, 0, sizeof(fans));
	if (!sb_find_fans(fans, &count))
		return NULL;

	routine->skip = SB_FALSE;
	while(1) {
		SB_START_TIMER;

		error   = SB_FALSE;
		average = 0;
		/* go through each fan#_output, get the value, and determine the percentage from
		 * its max and min */
		for (i=0; i<count && !error; i++) {
			fd = fopen(fans[i].path, "r");
			if (fd == NULL) {
				fprintf(stderr, "Fan routine: Failed to open %s\n", fans[i].path);
				error = SB_TRUE;
			} else if (fscanf(fd, "%ld", &speed) < 1) {
				fprintf(stderr, "Fan routine: Failed to read %s\n", fans[i].path);
				error = SB_TRUE;
			} else if (fclose(fd) != 0) {
				fprintf(stderr, "Fan routine: Failed to close %s\n", fans[i].path);
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

		SB_STOP_TIMER;
		SB_SLEEP;
	}

	if (fd != NULL)
		fclose(fd);
	routine->skip = SB_TRUE;
	return NULL;
}


/* --- LOAD ROUTINE --- */
static void *sb_load_routine(void *thunk)
{
	SB_TIMER_VARS;
	sb_routine_t      *routine = thunk;
	FILE              *fd;
	static const char *path    = "/proc/loadavg";
	double             av[3];

	routine->skip = SB_FALSE;
	while(1) {
		SB_START_TIMER;

		fd = fopen(path, "r");
		if (fd == NULL) {
			fprintf(stderr, "Load routine: Failed to open %s\n", path);
			break;
		} else if (fscanf(fd, "%lf %lf %lf", &av[0], &av[1], &av[2]) < 3) {
			fprintf(stderr, "Load routine: Failed to read %s\n", path);
			break;
		} else if (fclose(fd) != 0) {
			fprintf(stderr, "Load routine: Failed to close %s\n", path);
			break;
		}

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output)-1, "load: %.2f, %.2f, %.2f", av[0], av[1], av[2]);
		pthread_mutex_unlock(&(routine->mutex));

		SB_STOP_TIMER;
		SB_SLEEP;
	}

	if (fd != NULL)
		fclose(fd);
	routine->skip = SB_TRUE;
	return NULL;
}


/* --- NETWORK ROUTINE --- */
struct sb_file_t {
	char   path[IFNAMSIZ+64];
	long   old_bytes;
	long   new_bytes;
	float  reduced;
	char   prefix;
};

static SB_BOOL sb_get_paths(struct sb_file_t *rx_file, struct sb_file_t *tx_file)
{
	int             fd;
	struct ifreq    ifr;
	struct ifaddrs *ifaddrs = NULL;
	struct ifaddrs *ifap;

	/* open socket and return file descriptor for it */
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		fprintf(stderr, "Network routine: Failed to open socket file descriptor\n");
		return SB_FALSE;
	}

	/* get all network interfaces */
	if (getifaddrs(&ifaddrs) < 0 || ifaddrs == NULL) {
		fprintf(stderr, "Network routine: Failed to find interface addresses\n");
		close(fd);
		return SB_FALSE;
	}
	ifap = ifaddrs;

	/* go through each interface until we find an active one */
	while (ifap != NULL) {
		strncpy(ifr.ifr_name, ifap->ifa_name, IFNAMSIZ);
		if (ioctl(fd, SIOCGIFFLAGS, &ifr) >= 0) {
			if (ifr.ifr_flags & IFF_RUNNING && !(ifr.ifr_flags & IFF_LOOPBACK)) {
				snprintf(rx_file->path, sizeof(rx_file->path)-1,
						"/sys/class/net/%s/statistics/rx_bytes", ifap->ifa_name);
				snprintf(tx_file->path, sizeof(tx_file->path)-1,
						"/sys/class/net/%s/statistics/tx_bytes", ifap->ifa_name);
				break;
			}
		}
		ifap = ifap->ifa_next;
	}

	close(fd);
	freeifaddrs(ifaddrs);

	if (ifap == NULL) {
		fprintf(stderr, "Network routine: No wireless interfaces found\n");
		return SB_FALSE;
	}

	return SB_TRUE;
}

static void *sb_network_routine(void *thunk)
{
	SB_TIMER_VARS;
	sb_routine_t     *routine  = thunk;
	int               i;
	SB_BOOL           error;
	struct sb_file_t  files[2] = {0};
	FILE             *fd;
	long              diff;

	if (!sb_get_paths(&files[0], &files[1]))
		return NULL;

	routine->skip = SB_FALSE;
	while(1) {
		SB_START_TIMER;

		error = SB_FALSE;
		for (i=0; i<2 && !error; i++) {
			files[i].old_bytes = files[i].new_bytes;
			fd                 = fopen(files[i].path, "r");
			if (fd == NULL) {
				fprintf(stderr, "Network routine: Failed to open %s\n", files[i].path);
				error = SB_TRUE;
			} else if (fscanf(fd, "%ld", &files[i].new_bytes) < 1) {
				fprintf(stderr, "Network routine: Failed to read %s\n", files[i].path);
				error = SB_TRUE;
			} else if (fclose(fd) != 0) {
				fprintf(stderr, "Network routine: Failed to close %s\n", files[i].path);
				error = SB_TRUE;
			} else {
				diff             = files[i].new_bytes - files[i].old_bytes;
				files[i].reduced = sb_calc_magnitude(diff, &files[i].prefix);
			}
		}
		if (error)
			break;

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output)-1, "Down: %.1f %c Up: %.1f %c",
				files[0].reduced, files[0].prefix, files[1].reduced, files[1].prefix);
		pthread_mutex_unlock(&(routine->mutex));

		SB_STOP_TIMER;
		SB_SLEEP;
	}

	if (fd != NULL)
		fclose(fd);
	routine->skip = SB_TRUE;
	return NULL;
}


/* --- RAM ROUTINE --- */
static void *sb_ram_routine(void *thunk)
{
	SB_TIMER_VARS
	sb_routine_t *routine = thunk;
	long          page_size;
	long          total_pages;
	float         total_bytes_f;
	char          total_bytes_prefix;
	long          avail_bytes;
	float         avail_bytes_f;
	char          avail_bytes_prefix;

	page_size   = sysconf(_SC_PAGESIZE);
	total_pages = sysconf(_SC_PHYS_PAGES);
	if (page_size < 0 || total_pages < 0) {
		fprintf(stderr, "Ram routine: Failed to get page info\n");
		return NULL;
	}

	/* get total bytes as a decimal in human-readable format */
	total_bytes_f = sb_calc_magnitude(total_pages*page_size, &total_bytes_prefix);

	routine->skip = SB_FALSE;
	while(1) {
		SB_START_TIMER;

		/* get available memory */
		avail_bytes = sysconf(_SC_AVPHYS_PAGES) * page_size;
		if (avail_bytes < 0) {
			fprintf(stderr, "Ram routine: Failed to get available bytes\n");
			break;
		}
		avail_bytes_f = sb_calc_magnitude(avail_bytes, &avail_bytes_prefix);

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output)-1, "Free: %.1f %c / %.1f %c",
				avail_bytes_f, avail_bytes_prefix, total_bytes_f, total_bytes_prefix);
		pthread_mutex_unlock(&(routine->mutex));

		SB_STOP_TIMER;
		SB_SLEEP;
	}

	routine->skip = SB_TRUE;
	return NULL;
}


/* --- TIME ROUTINE --- */
static void *sb_time_routine(void *thunk)
{
	/* For this routine, we are not using the SB_START_TIMER and SB_STOP_TIMER
	 * macros because we need to use CLOCK_REALTIME to get the actual system time.
	 */
	SB_TIMER_VARS;
	sb_routine_t *routine = thunk;
	struct tm     tm;

	routine->skip = SB_FALSE;
	while(1) {
		clock_gettime(CLOCK_REALTIME, &start_tp);

		/* convert time from seconds since epoch to local time */
		memset(&tm, 0, sizeof(tm));
		localtime_r(&start_tp.tv_sec, &tm);

		pthread_mutex_lock(&(routine->mutex));
		strftime(routine->output, sizeof(routine->output)-1, time_format, &tm);
		pthread_mutex_unlock(&(routine->mutex));

		clock_gettime(CLOCK_REALTIME, &finish_tp);
		elapsed_usec = ((finish_tp.tv_sec - start_tp.tv_sec) * 1000000) + (labs(start_tp.tv_nsec - finish_tp.tv_nsec) / 1000);

		if (usleep((routine->interval * 1000000) - elapsed_usec) != 0) {
			fprintf(stderr, "Time routine: Error sleeping\n");
		}
	}

	routine->skip = SB_TRUE;
	return NULL;
}


/* --- TODO ROUTINE --- */
static void *sb_todo_routine(void *thunk)
{
	/* We're going to read in the first two lines of the user's personal TODO
 	 * list and print them based on a few rules:
	 * 1. If the file is empty, print "Finished".
	 * 2. If the second line is empty, only print the first line.
	 * 3. If the second line is indented over from the first, then it is a child
	 *    task of the first. Print "line1 -> line2".
	 * 4. If the second line is not indented, both tasks are equal. Print "line1 | line2".
	 */
	SB_TIMER_VARS;
	sb_routine_t *routine    = thunk;
	FILE         *fd;
	char          path[512]  = {0};
	char          line1[512] = {0};
	char          line2[512] = {0};
	const char   *const_line;
	SB_BOOL       empty;
	const char   *separator;

	snprintf(path, sizeof(path), "%s/.TODO", getenv("HOME"));

	routine->skip = SB_FALSE;
	while(1) {
		SB_START_TIMER;

		/* reset pointer to beginning of line */
		const_line = line2;

		fd = fopen(path, "r");
		if (fd == NULL) {
			fprintf(stderr, "Todo routine: Failed to open %s\n", path);
			break;
		} else if (fgets(line1, sizeof(line1), fd) == NULL) {
			empty = SB_TRUE;
		} else if (fgets(line2, sizeof(line2), fd) == NULL) {
			/* if this line is empty, do nothing */
		} else if (fclose(fd) != 0) {
			fprintf(stderr, "Todo routine: Failed to close %s\n", path);
			break;
		}

		switch (*line2) {
			case '\t':
			case ' ' :
				break;
			case '\n':
				break;
			default:
		}

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output)-1, "todo: TODO");
		pthread_mutex_unlock(&(routine->mutex));

		SB_STOP_TIMER;
		SB_SLEEP;
	}

	if (fd != NULL)
		fclose(fd);
	routine->skip = SB_TRUE;
	return NULL;
}


/* --- VOLUME ROUTINE --- */
static void *sb_volume_routine(void *thunk)
{
	SB_TIMER_VARS;
	sb_routine_t *routine = thunk;

	routine->skip = SB_FALSE;
	while(1) {
		SB_START_TIMER;

		/* TODO: run routine */

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output)-1, "volume: TODO");
		pthread_mutex_unlock(&(routine->mutex));

		SB_STOP_TIMER;
		SB_SLEEP;
	}

	routine->skip = SB_TRUE;
	return NULL;
}


/* --- WEATHER ROUTINE --- */
static void *sb_weather_routine(void *thunk)
{
	SB_TIMER_VARS;
	sb_routine_t *routine = thunk;

	routine->skip = SB_FALSE;
	while(1) {
		SB_START_TIMER;

		/* TODO: run routine */

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output)-1, "weather: TODO");
		pthread_mutex_unlock(&(routine->mutex));

		SB_STOP_TIMER;
		SB_SLEEP;
	}

	routine->skip = SB_TRUE;
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
		fprintf(stderr, "Wifi routine: Failed to open socket file descriptor\n");
		return SB_FALSE;
	}

	/* get all network interfaces */
	if (getifaddrs(&ifaddrs) < 0 || ifaddrs == NULL) {
		fprintf(stderr, "Wifi routine: Failed to find interface addresses\n");
		return SB_FALSE;
	}
	ifap = ifaddrs;

	/* go through each interface until one returns an ssid */
	while (ifap != NULL) {
		strncpy(iwr->ifr_ifrn.ifrn_name, ifap->ifa_name, IFNAMSIZ);
		if (ioctl(*fd, SIOCGIWESSID, iwr) >= 0) {
			/* we found a match */
			freeifaddrs(ifaddrs);
			return SB_TRUE;
		}
		/* no match found, try the next interface */
		ifap = ifap->ifa_next;
	}

	/* if we reached here, then we didn't find anything */
	fprintf(stderr, "Wifi routine: No wireless interfaces found\n");
	freeifaddrs(ifaddrs);
	return SB_FALSE;
}

static void *sb_wifi_routine(void *thunk)
{
	/* First, we are going to loop through all network interfaces, checking for an SSID.
	 * When we first find one, we'll break out of the loop and use that interface as
	 * the wireless network.
	 *
	 * TODO:
	 * - if init fails, try again later
	 * - handle break and reattach at a later time
	 */
	SB_TIMER_VARS;
	sb_routine_t *routine = thunk;
	int           fd;
	struct iwreq  iwr;
	char          essid[IW_ESSID_MAX_SIZE + 1];

	if (!sb_init_wifi(&fd, &iwr, essid, sizeof(essid))) {
		close(fd);
		return NULL;
	}

	routine->skip = SB_FALSE;
	while(1) {
		SB_START_TIMER;

		memset(essid, 0, sizeof(essid));
		if (ioctl(fd, SIOCGIWESSID, &iwr) < 0) {
			fprintf(stderr, "Wifi routine: Failed to get SSID\n");
			break;
		}

		pthread_mutex_lock(&(routine->mutex));
		strncpy(routine->output, essid, sizeof(routine->output)-1);
		pthread_mutex_unlock(&(routine->mutex));

		SB_STOP_TIMER;
		SB_SLEEP;
	}

	close(fd);
	routine->skip = SB_TRUE;
	return NULL;
}


static const struct thread_routines_t {
	enum sb_routine_e routine;
	void *(*callback)(void *thunk);
} possible_routines[] = {
	{ BACKUP,     sb_backup_routine     },
	{ BATTERY,    sb_battery_routine    },
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
		routine_object->skip        = SB_TRUE; /* this will be set to SB_FALSE when routine initializes successfully */

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
			fprintf(stderr, "%s: Thread did not exit cleanly (%s)\n", routine_names[index], (char *)join_ret);
		if (pthread_mutex_destroy(&(routine_object->mutex)) != 0)
			fprintf(stderr, "%s: Failed to destroy mutex\n", routine_names[index]);
		routine_object->skip = SB_TRUE; /* make sure routine is skipped */
		free(join_ret);
	}
	/* kill print thread */
	run = 0;
	if (pthread_join(print_thread, &join_ret) != 0)
		fprintf(stderr, "print thread did not exit cleanly (%s)\n", (char *)join_ret);
	free(join_ret);

	return 0;
}
