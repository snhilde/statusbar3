#include "statusbar.h"
#include "config.h"

#define SBLENGTH 10240

#define SB_PRINT_ERROR(msg, arg) \
		{ const char *_arg = arg; \
		  fprintf(stderr, "%s routine: " msg " %s\n", routine->name, _arg); }

#define SB_START_TIMER \
		clock_gettime(CLOCK_MONOTONIC_RAW, &start_tp);

#define SB_STOP_TIMER \
		clock_gettime(CLOCK_MONOTONIC_RAW, &finish_tp);

#define SB_SLEEP \
		elapsed_usec = (finish_tp.tv_sec - start_tp.tv_sec) + ((finish_tp.tv_nsec - start_tp.tv_nsec) / 1000); \
		if (elapsed_usec < routine->interval) { \
			if (usleep(routine->interval - elapsed_usec) != 0) { \
				SB_PRINT_ERROR("Error sleeping", NULL); \
			} \
		}

#define SB_TIMER_VARS \
	struct timespec  start_tp  = {0}; \
	struct timespec  finish_tp = {0}; \
	long             elapsed_usec;

/* --- HELPER FUNCTIONS --- */
static float sb_calc_magnitude(long number, char *unit)
{
	/* This will calculate how many commas the number would have. */
	int  i;
	const char symbols[] = "BKMGTP";

	if (number < 1000) {
		*unit = 'B';
		return number * 1.0;
	}

	for (i=0; number / powl(10, 3*i) > 999; i++);

	*unit = symbols[i];
	return (number / powl(10, 3*(i-1))) / 1000.0;
}

static long sb_normalize_perc(long num)
{
	if (num > 100)
		return 100;

	if (num < 0)
		return 0;

	return num;
}

static void *sb_null_cb(void *thunk)
{
	/* empty function to satisfy TIME not having a proper thread */
	(void)thunk;
	return NULL;
}

static SB_BOOL sb_read_file(char buf[], size_t size, const char *base, const char *file, sb_routine_t *routine)
{
	char  path[512];
	FILE *fd;

	memset(buf, 0, size);

	snprintf(path, sizeof(path), "%s%s", base, file?file:"");
	fd = fopen(path, "r");
	if (fd == NULL) {
		SB_PRINT_ERROR("Failed to open", path);
		return SB_FALSE;
	}

	if (fgets(buf, size, fd) == NULL) {
		SB_PRINT_ERROR("Failed to read", path);
		fclose(fd);
		return SB_FALSE;
	}

	if (fclose(fd) != 0) {
		SB_PRINT_ERROR("Failed to close", path);
		return SB_FALSE;
	}

	return SB_TRUE;
}

static SB_BOOL sb_get_path(char buf[], size_t size, const char *base, const char *file, const char *match, sb_routine_t *routine)
{
	DIR           *dir;
	struct dirent *dirent;
	char           path[512];
	char           contents[512];

	memset(buf, 0, size);

	dir = opendir(base);
	if (dir == NULL) {
		SB_PRINT_ERROR("Failed to open", base);
		return SB_FALSE;
	}

	while ((dirent=readdir(dir))) {
		if (strncasecmp(dirent->d_name, ".", 1) == 0 || strncasecmp(dirent->d_name, "..", 2) == 0)
			continue;

		snprintf(path, sizeof(path), "%s/%s/", base, dirent->d_name);
		if (!sb_read_file(contents, sizeof(contents), path, file, routine))
			continue;

		if (strncasecmp(contents, match, strlen(match)) == 0) {
			snprintf(buf, size, "%s/%s/", base, dirent->d_name);
			closedir(dir);
			return SB_TRUE;
		}
	}

	SB_PRINT_ERROR("Failed to find file", NULL);
	closedir(dir);
	return SB_FALSE;
}


/* --- BATTERY ROUTINE --- */
static void *sb_battery_routine(void *thunk)
{
	sb_routine_t *routine = thunk;

#ifdef BUILD_BATTERY
	SB_TIMER_VARS;
	char path[512];
	char buf[512];
	long max;
	long now;
	long perc;

	if (!sb_get_path(path, sizeof(path), "/sys/class/power_supply", "type", "Battery", routine)) {
		routine->print = SB_FALSE;
	} else if (!sb_read_file(buf, sizeof(buf), path, "charge_full", routine)) {
		routine->print = SB_FALSE;
	} else {
		max = atol(buf);
		if (max <= 0) {
			SB_PRINT_ERROR("Failed to read max level", NULL);
			routine->print = SB_FALSE;
		}
	}

	while (routine->print) {
		SB_START_TIMER;

		if (!sb_read_file(buf, sizeof(buf), path, "charge_now", routine))
			break;

		now = atol(buf);
		if (now < 0) {
			SB_PRINT_ERROR("Failed to read current level", NULL);
			break;
		}

		perc = sb_normalize_perc((now*100)/max);
		if (perc > 25) {
			routine->color = routine->colors.normal;
		} else if (perc > 10) {
			routine->color = routine->colors.warning;
		} else {
			routine->color = routine->colors.error;
		}

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output), "%ld%% BAT", perc);
		pthread_mutex_unlock(&(routine->mutex));

		SB_STOP_TIMER;
		SB_SLEEP;
	}
#endif

	if (pthread_mutex_destroy(&(routine->mutex)) != 0)
		SB_PRINT_ERROR("Failed to destroy mutex", NULL);
	routine->print = SB_FALSE;
	return NULL;
}


/* --- CPU TEMP ROUTINE --- */
#ifdef BUILD_CPU_TEMP
static SB_BOOL sb_cpu_temp_get_filename(char path[], char filename[], size_t size, sb_routine_t *routine)
{
	DIR           *dir;
	struct dirent *dirent;

	dir = opendir(path);
	if (dir == NULL) {
		SB_PRINT_ERROR("Failed to open", path);
		return SB_FALSE;
	}

	/* Find a temperature monitor. */
	while ((dirent=readdir(dir))) {
		if (strncasecmp(dirent->d_name, ".", 1) == 0 || strncasecmp(dirent->d_name, "..", 2) == 0)
			continue;

		if (strncasecmp(dirent->d_name, "temp", 4) == 0 && strncasecmp(dirent->d_name+5, "_input", 6) == 0) {
			/* We found a match. */
			strncpy(filename, dirent->d_name, size-1);
			closedir(dir);
			return SB_TRUE;
		}
	}

	SB_PRINT_ERROR("Failed to find temperature monitor", NULL);
	closedir(dir);
	return SB_FALSE;
}
#endif

static void *sb_cpu_temp_routine(void *thunk)
{
	sb_routine_t *routine = thunk;

#ifdef BUILD_CPU_TEMP
	SB_TIMER_VARS;
	char path[512];
	char filename[128];
	char contents[128];
	long now;

	if (!sb_get_path(path, sizeof(path), "/sys/class/hwmon", "name", "coretemp", routine)) {
		routine->print = SB_FALSE;
	} else if (!sb_cpu_temp_get_filename(path, filename, sizeof(filename), routine)) {
		routine->print = SB_FALSE;
	}

	while (routine->print) {
		SB_START_TIMER;

		if (!sb_read_file(contents, sizeof(contents), path, filename, routine))
			break;

		now = atol(contents);
		if (now < 0) {
			SB_PRINT_ERROR("Failed to read temperature", NULL);
			break;
		}

		now /= 1000; /* convert to celsius */
		if (now < 75) {
			routine->color = routine->colors.normal;
		} else if (now < 100) {
			routine->color = routine->colors.warning;
		} else {
			routine->color = routine->colors.error;
		}

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output), "%ld °C", now);
		pthread_mutex_unlock(&(routine->mutex));

		SB_STOP_TIMER;
		SB_SLEEP;
	}
#endif

	if (pthread_mutex_destroy(&(routine->mutex)) != 0)
		SB_PRINT_ERROR("Failed to destroy mutex", NULL);
	routine->print = SB_FALSE;
	return NULL;
}


/* --- CPU USAGE ROUTINE --- */
static void *sb_cpu_usage_routine(void *thunk)
{
	/* TODO: why is sysconf(_SC_NPROCESSORS_ONLN) or num cores not necessary here? */
	sb_routine_t      *routine = thunk;

#ifdef BUILD_CPU_USAGE
	SB_TIMER_VARS;
	static const char *path = "/proc/stat";
	char               contents[128];
	unsigned long      used;
	unsigned long      total;
	long               perc;
	struct {
		unsigned long user;
		unsigned long nice;
		unsigned long system;
		unsigned long idle;
	} old, new;

	memset(&old, 0, sizeof(old));
	memset(&new, 0, sizeof(new));
	while (routine->print) {
		SB_START_TIMER;

		if (!sb_read_file(contents, sizeof(contents), path, NULL, routine))
			break;
		if (sscanf(contents, "cpu %lu %lu %lu %lu", &new.user, &new.nice, &new.system, &new.idle) != 4 ) {
			SB_PRINT_ERROR("Failed to read", path);
			break;
		}

		used  = (new.user-old.user) + (new.nice-old.nice) + (new.system-old.system);
		total = (new.user-old.user) + (new.nice-old.nice) + (new.system-old.system) + (new.idle-old.idle);
		perc  = sb_normalize_perc((used*100)/total);
		if (perc < 75) {
			routine->color = routine->colors.normal;
		} else if (perc < 90) {
			routine->color = routine->colors.warning;
		} else {
			routine->color = routine->colors.error;
		}

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output), "%2lu%% CPU", perc);
		pthread_mutex_unlock(&(routine->mutex));

		old.user   = new.user;
		old.nice   = new.nice;
		old.system = new.system;
		old.idle   = new.idle;

		SB_STOP_TIMER;
		SB_SLEEP;
	}
#endif

	if (pthread_mutex_destroy(&(routine->mutex)) != 0)
		SB_PRINT_ERROR("Failed to destroy mutex", NULL);
	routine->print = SB_FALSE;
	return NULL;
}


/* --- DISK ROUTINE --- */
static void *sb_disk_routine(void *thunk)
{
	sb_routine_t *routine = thunk;

#ifdef BUILD_DISK
	SB_TIMER_VARS;
	size_t         num_filesystems;
	int            i;
	struct statvfs stats;
	long           avail;
	char           avail_unit;
	long           total;
	char           total_unit;
	long           perc;
	char           output[512];
	int            color_level;

	while (routine->print) {
		SB_START_TIMER;

		/* In this routine, we're going to lock the mutex for the entire operation so we
		 * can safely add to the routine's output for the entire loop. */
		pthread_mutex_lock(&(routine->mutex));
		*routine->output = '\0';

		color_level     = 1;
		routine->color  = routine->colors.normal; /* start at normal */
		num_filesystems = sizeof(filesystems) / sizeof(*filesystems);
		for (i=0; i<num_filesystems; i++) {
			if (statvfs(filesystems[i].path, &stats) != 0) {
				SB_PRINT_ERROR("Failed to get stats for", filesystems[i].path)
				break;
			}
			avail = (long)sb_calc_magnitude(stats.f_bfree *stats.f_bsize, &avail_unit);
			total = (long)sb_calc_magnitude(stats.f_blocks*stats.f_bsize, &total_unit);
			/* chose highest warning for any filesystem */
			perc = sb_normalize_perc((avail*100)/total);
			if (perc >= 90) {
				color_level    = 3;
				routine->color = routine->colors.error;
			} else if (perc >= 75 && color_level < 3) {
				color_level    = 2;
				routine->color = routine->colors.warning;
			}

			snprintf(output, sizeof(output), "%s: %ld%c/%ld%c",
					filesystems[i].display_name, avail, avail_unit, total, total_unit);
			strncat(routine->output, output, sizeof(routine->output)-strlen(routine->output)-1);

			if (i+1 < num_filesystems)
				strncat(routine->output, ", ", sizeof(routine->output)-strlen(routine->output)-1);
		}
		pthread_mutex_unlock(&(routine->mutex));

		SB_STOP_TIMER;
		SB_SLEEP;
	}
#endif

	if (pthread_mutex_destroy(&(routine->mutex)) != 0)
		SB_PRINT_ERROR("Failed to destroy mutex", NULL);
	routine->print = SB_FALSE;
	return NULL;
}


/* --- FAN ROUTINE --- */
#ifdef BUILD_FAN
static SB_BOOL sb_fan_get_path(char path[], size_t size, sb_routine_t *routine)
{
	static const char *base = "/sys/class/hwmon";
	DIR               *dir;
	struct dirent     *dirent;
	DIR               *device;
	struct dirent     *devent;

	dir = opendir(base);
	if (dir == NULL) {
		SB_PRINT_ERROR("Failed to open", base);
		return SB_FALSE;
	}

	/* step through each file/directory in the base and try to open a subdirectory called device */
	while ((dirent=readdir(dir))) {
		if (strncasecmp(dirent->d_name, ".", 1) == 0 || strncasecmp(dirent->d_name, "..", 2) == 0)
			continue;

		snprintf(path, size, "%s/%s/device", base, dirent->d_name);
		device = opendir(path);
		if (device != NULL) {
			/* step through each file in base/hwmon#/device and find any fans */
			while ((devent=readdir(device))) {
				if (strncasecmp(devent->d_name, ".", 1) == 0 || strncasecmp(devent->d_name, "..", 2) == 0)
					continue;

				if (strncasecmp(devent->d_name, "fan", 3) == 0 && strncasecmp(devent->d_name+4, "_output", 7) == 0) {
					/* We found a fan. */
					snprintf(path, size, "%s/%s/device/%.4s", base, dirent->d_name, devent->d_name);
					closedir(device);
					closedir(dir);
					return SB_TRUE;
				}
			}

			closedir(device);
		}
	}

	SB_PRINT_ERROR("Failed to find a fan", NULL);
	closedir(dir);
	return SB_FALSE;
}
#endif

static void *sb_fan_routine(void *thunk)
{
	sb_routine_t *routine = thunk;

#ifdef BUILD_FAN
	SB_TIMER_VARS;
	char path[512];
	char contents[128];
	long max;
	long now;
	long perc;

	if (!sb_fan_get_path(path, sizeof(path), routine)) {
		routine->print = SB_FALSE;
	} else if (!sb_read_file(contents, sizeof(contents), path, "_max", routine)) {
		fprintf(stderr, "%s routine: Failed to read %s_max\n", routine->name, path);
		routine->print = SB_FALSE;
	} else {
		strncat(path, "_output", sizeof(path)-strlen(path)-1);
		max = atol(contents);
		if (max < 0)
			routine->print = SB_FALSE;
	}

	while (routine->print) {
		SB_START_TIMER;

		if (!sb_read_file(contents, sizeof(contents), path, NULL, routine))
			break;

		now = atol(contents);
		if (now < 0) {
			SB_PRINT_ERROR("Failed to read current fan speed", NULL);
			break;
		}
		perc = sb_normalize_perc((now*100)/max);
		if (perc < 75) {
			routine->color = routine->colors.normal;
		} else if (perc < 90) {
			routine->color = routine->colors.warning;
		} else {
			routine->color = routine->colors.error;
		}

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output), "%ld RPM", now);
		pthread_mutex_unlock(&(routine->mutex));

		SB_STOP_TIMER;
		SB_SLEEP;
	}

#endif

	if (pthread_mutex_destroy(&(routine->mutex)) != 0)
		SB_PRINT_ERROR("Failed to destroy mutex", NULL);
	routine->print = SB_FALSE;
	return NULL;
}


/* --- LOAD ROUTINE --- */
static void *sb_load_routine(void *thunk)
{
	sb_routine_t *routine = thunk;

#ifdef BUILD_LOAD
	SB_TIMER_VARS;
	static const char *path = "/proc/loadavg";
	char               contents[128];
	double             av[3];

	while (routine->print) {
		SB_START_TIMER;

		if (!sb_read_file(contents, sizeof(contents), path, NULL, routine))
			break;
		if (sscanf(contents, "%lf %lf %lf", &av[0], &av[1], &av[2]) != 3) {
			SB_PRINT_ERROR("Failed to read", path);
			break;
		}

		if (av[0] > 2 || av[1] > 2 || av[2] > 2) {
			routine->color = routine->colors.error;
		} else if (av[0] > 1 || av[1] > 1 || av[2] > 1) {
			routine->color = routine->colors.warning;
		} else {
			routine->color = routine->colors.normal;
		}

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output), "%.2f, %.2f, %.2f", av[0], av[1], av[2]);
		pthread_mutex_unlock(&(routine->mutex));

		SB_STOP_TIMER;
		SB_SLEEP;
	}
#endif

	if (pthread_mutex_destroy(&(routine->mutex)) != 0)
		SB_PRINT_ERROR("Failed to destroy mutex", NULL);
	routine->print = SB_FALSE;
	return NULL;
}


/* --- NETWORK ROUTINE --- */
#ifdef BUILD_NETWORK
struct sb_file_t {
	char path[IFNAMSIZ+64];
	long old_bytes;
	long new_bytes;
	long reduced;
	char unit;
};

static SB_BOOL sb_network_get_paths(struct sb_file_t *rx_file, struct sb_file_t *tx_file, sb_routine_t *routine)
{
	int             sock;
	struct ifreq    ifr;
	struct ifaddrs *ifaddrs = NULL;
	struct ifaddrs *ifap;

	/* open socket and return file descriptor for it */
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		SB_PRINT_ERROR("Failed to open socket file descriptor", NULL);
		return SB_FALSE;
	}

	/* get all network interfaces */
	if (getifaddrs(&ifaddrs) < 0 || ifaddrs == NULL) {
		SB_PRINT_ERROR("Failed to find interface addresses", NULL);
		close(sock);
		return SB_FALSE;
	}
	ifap = ifaddrs;

	/* go through each interface until we find an active one */
	while (ifap != NULL) {
		strncpy(ifr.ifr_name, ifap->ifa_name, IFNAMSIZ);
		if (ioctl(sock, SIOCGIFFLAGS, &ifr) >= 0) {
			if (ifr.ifr_flags & IFF_RUNNING && !(ifr.ifr_flags & IFF_LOOPBACK)) {
				snprintf(rx_file->path, sizeof(rx_file->path),
						"/sys/class/net/%s/statistics/rx_bytes", ifap->ifa_name);
				snprintf(tx_file->path, sizeof(tx_file->path),
						"/sys/class/net/%s/statistics/tx_bytes", ifap->ifa_name);
				break;
			}
		}
		ifap = ifap->ifa_next;
	}

	close(sock);
	freeifaddrs(ifaddrs);

	if (ifap == NULL) {
		SB_PRINT_ERROR("No wireless interfaces found", NULL);
		return SB_FALSE;
	}

	return SB_TRUE;
}
#endif

static void *sb_network_routine(void *thunk)
{
	sb_routine_t *routine = thunk;

#ifdef BUILD_NETWORK
	SB_TIMER_VARS;
	struct sb_file_t files[2] = {0};
	SB_BOOL          error;
	int              i;
	char             contents[128];
	int              color_level;

	if (!sb_network_get_paths(&files[0], &files[1], routine))
		routine->print = SB_FALSE;

	while (routine->print) {
		SB_START_TIMER;

		color_level    = 1;
		routine->color = routine->colors.normal;
		error = SB_FALSE;
		for (i=0; i<2 && !error; i++) {
			files[i].old_bytes = files[i].new_bytes;
			if (!sb_read_file(contents, sizeof(contents), files[i].path, NULL, routine)) {
				error = SB_TRUE;
			} else if (sscanf(contents, "%ld", &files[i].new_bytes) != 1) {
				SB_PRINT_ERROR("Failed to read", files[i].path);
				error = SB_TRUE;
			} else {
				files[i].reduced = (long)sb_calc_magnitude(files[i].new_bytes - files[i].old_bytes, &files[i].unit);
				if (files[i].unit == 'B' || files[i].unit == 'K') {
				} else if (files[i].unit == 'M' && color_level < 3) {
					color_level    = 2;
					routine->color = routine->colors.warning;
				} else {
					color_level    = 3;
					routine->color = routine->colors.error;
				}
			}
		}
		if (error)
			break;

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output), "%3ld%c up/%3ld%c down",
				files[0].reduced, files[0].unit, files[1].reduced, files[1].unit);
		pthread_mutex_unlock(&(routine->mutex));

		SB_STOP_TIMER;
		SB_SLEEP;
	}
#endif

	if (pthread_mutex_destroy(&(routine->mutex)) != 0)
		SB_PRINT_ERROR("Failed to destroy mutex", NULL);
	routine->print = SB_FALSE;
	return NULL;
}


/* --- RAM ROUTINE --- */
static void *sb_ram_routine(void *thunk)
{
	sb_routine_t *routine = thunk;

#ifdef BUILD_RAM
	SB_TIMER_VARS
	long page_size;
	long total;
	char total_unit;
	long avail;
	char avail_unit;
	long perc;

	page_size = sysconf(_SC_PAGESIZE);

	/* calculate available and total bytes */
	avail = sysconf(_SC_AVPHYS_PAGES) * page_size;
	total = sysconf(_SC_PHYS_PAGES)   * page_size;
	if (avail < 1 || total < 1) {
		fprintf(stderr, "%s routine: Failed to get memory amounts\n", routine->name);
		routine->print = SB_FALSE;
	} else {
		/* calculate units now so we have something to print on first loop */
		sb_calc_magnitude(avail, &avail_unit);
		sb_calc_magnitude(total, &total_unit);
	}

	while (routine->print) {
		SB_START_TIMER;

		/* get available memory */
		avail = sysconf(_SC_AVPHYS_PAGES) * page_size;
		if (avail < 1) {
			fprintf(stderr, "%s routine: Failed to get available bytes\n", routine->name);
			break;
		}

		perc  = sb_normalize_perc((avail*100)/total);
		if (perc < 75) {
			routine->color = routine->colors.normal;
		} else if (perc < 90) {
			routine->color = routine->colors.warning;
		} else {
			routine->color = routine->colors.error;
		}

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output), "%.1f%c free/%.1f%c",
				sb_calc_magnitude(avail, &avail_unit), avail_unit,
				sb_calc_magnitude(total, &total_unit), total_unit);
		pthread_mutex_unlock(&(routine->mutex));

		SB_STOP_TIMER;
		SB_SLEEP;
	}
#endif

	if (pthread_mutex_destroy(&(routine->mutex)) != 0)
		SB_PRINT_ERROR("Failed to destroy mutex", NULL);
	routine->print = SB_FALSE;
	return NULL;
}


/* --- TODO ROUTINE --- */
#ifdef BUILD_TODO
static int sb_todo_count_blanks(const char *line, SB_BOOL *isempty)
{
	int i = 0;

	*isempty = SB_FALSE;

	switch (*line) {
		case '\t':
		case ' ' :
			while (line[i] && isblank(line[i]) != 0) {
				i++;
			}
			if (!isgraph(line[i]))
				*isempty = SB_TRUE;
			return i;
		case '\n':
			*isempty = SB_TRUE;
		default:
			return 0;
	}

	return 0;
}
#endif

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
	sb_routine_t *routine = thunk;

#ifdef BUILD_TODO
	SB_TIMER_VARS;
	FILE       *fd;
	char        path[512]  = {0};
	const char *separator;
	int         i;
	struct {
		char        line[512];
		const char *ptr;
		SB_BOOL     isempty;
	} line[2] = {0};

	snprintf(path, sizeof(path), "%s/.TODO", getenv("HOME"));

	routine->color = routine->colors.normal;
	while (routine->print) {
		SB_START_TIMER;

		fd = fopen(path, "r");
		if (fd == NULL) {
			SB_PRINT_ERROR("Failed to open", path);
			break;
		} else if (fgets(line[0].line, sizeof(line[0].line), fd) == NULL) {
			line[0].isempty = SB_TRUE;
		} else if (fgets(line[1].line, sizeof(line[1].line), fd) == NULL) {
			line[1].isempty = SB_TRUE;
		} else if (fclose(fd) != 0) {
			SB_PRINT_ERROR("Failed to close", path);
			fclose(fd);
			break;
		}

		if (line[0].isempty) {
			separator = "";
		} else if (isblank(*line[1].line)) {
			separator = " -> ";
		} else {
			separator = " | ";
		}

		for (i=0; i<2; i++) {
			/* reset pointer to beginning of line */
			line[i].ptr = line[i].line;

			/* advance line pointer until it hits the first non-blank character */
			if (!line[i].isempty) {
				line[i].line[strlen(line[i].line)-1] = '\0';
				line[i].ptr += sb_todo_count_blanks(line[i].line, &line[i].isempty);
			}
		}

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output), "%s%s%s",
				line[0].ptr, separator, line[1].ptr);
		pthread_mutex_unlock(&(routine->mutex));

		SB_STOP_TIMER;
		SB_SLEEP;
	}
#endif

	if (pthread_mutex_destroy(&(routine->mutex)) != 0)
		SB_PRINT_ERROR("Failed to destroy mutex", NULL);
	routine->print = SB_FALSE;
	return NULL;
}


/* --- VOLUME ROUTINE --- */
#ifdef BUILD_VOLUME
static SB_BOOL sb_volume_get_snd_elem(snd_mixer_t **mixer, snd_mixer_elem_t **snd_elem, sb_routine_t *routine)
{
	static const char    *card   = "hw:0";
	snd_mixer_selem_id_t *snd_id = NULL;
	static const int      index  = 0;
	static const char    *name   = "Master";

	*snd_elem = NULL;
	*mixer    = NULL;

	/* open and load mixer */
	if (snd_mixer_open(mixer, 0) < 0) {
		SB_PRINT_ERROR("Failed to open mixer", NULL);
	} else if (snd_mixer_attach(*mixer, card) < 0) {
		SB_PRINT_ERROR("Failed to attach mixer", NULL);
	} else if (snd_mixer_selem_register(*mixer, NULL, NULL) < 0) {
		SB_PRINT_ERROR("Failed to register mixer", NULL);
	} else if (snd_mixer_load(*mixer) < 0) {
		SB_PRINT_ERROR("Failed to load mixer", NULL);

	/* get id */
	} else if (snd_mixer_selem_id_malloc(&snd_id) != 0) {
		SB_PRINT_ERROR("Failed to allocate snd_id", NULL);

	/* get element */
	} else {
		snd_mixer_selem_id_set_index(snd_id, index);
		snd_mixer_selem_id_set_name(snd_id, name);
		*snd_elem = snd_mixer_find_selem(*mixer, snd_id);
		snd_mixer_selem_id_free(snd_id);
		if (*snd_elem == NULL) {
			SB_PRINT_ERROR("Failed to find element", NULL);
		} else if (snd_mixer_selem_has_playback_volume(*snd_elem) == 0) {
			SB_PRINT_ERROR("Element does not have playback volume", NULL);
		} else {
			return SB_TRUE;
		}
	}

	return SB_FALSE;
}
#endif

static void *sb_volume_routine(void *thunk)
{
	sb_routine_t *routine = thunk;

#ifdef BUILD_VOLUME
	SB_TIMER_VARS;
	snd_mixer_t      *mixer;
	snd_mixer_elem_t *snd_elem;
	long              min;
	long              max;
	int               mute = 0;
	long              decibels;
	long              perc;

	if (!sb_volume_get_snd_elem(&mixer, &snd_elem, routine)) {
		routine->print = SB_FALSE;
	} else if (snd_mixer_selem_get_playback_dB_range(snd_elem, &min, &max) != 0) {
		SB_PRINT_ERROR("Failed to get decibels range", NULL);
		routine->print = SB_FALSE;
	}

	while (routine->print) {
		SB_START_TIMER;

		if (snd_mixer_handle_events(mixer) < 0) {
			SB_PRINT_ERROR("Failed to clear mixer", NULL);
			break;
		} else if (snd_mixer_selem_get_playback_switch(snd_elem, SND_MIXER_SCHN_MONO, &mute) != 0) {
			SB_PRINT_ERROR("Failed to get mute state", NULL);
			break;
		} else if (mute == 0) {
			pthread_mutex_lock(&(routine->mutex));
			snprintf(routine->output, sizeof(routine->output), "mute");
			pthread_mutex_unlock(&(routine->mutex));
		} else if (snd_mixer_selem_get_playback_dB(snd_elem, SND_MIXER_SCHN_MONO, &decibels) != 0) {
			SB_PRINT_ERROR("Failed to get decibels", NULL);
			break;
		} else {
			perc = sb_normalize_perc((decibels-min)*100/(max-min));
			perc = rint((float)perc / 10) * 10; /* round to nearest ten */
			if (perc < 80) {
				routine->color = routine->colors.normal;
			} else if (perc < 100) {
				routine->color = routine->colors.warning;
			} else {
				routine->color = routine->colors.error;
			}
			pthread_mutex_lock(&(routine->mutex));
			snprintf(routine->output, sizeof(routine->output), "Vol %ld%%", perc);
			pthread_mutex_unlock(&(routine->mutex));
		}

		SB_STOP_TIMER;
		SB_SLEEP;
	}

	if (mixer != NULL) {
		snd_mixer_close(mixer);
		snd_mixer_free(mixer);
	}
	if (snd_elem != NULL)
		snd_mixer_elem_free(snd_elem);
#endif

	if (pthread_mutex_destroy(&(routine->mutex)) != 0)
		SB_PRINT_ERROR("Failed to destroy mutex", NULL);
	routine->print = SB_FALSE;
	return NULL;
}


/* --- WEATHER ROUTINE --- */
static int sb_weather_global_init(void)
{
#ifdef BUILD_WEATHER
	return curl_global_init(CURL_GLOBAL_SSL);
#else
	/* Triggered if user selects WEATHER as routine to run in config.h but
 	 * doesn't have library to build routine. */
	fprintf(stderr, "Weather routine: Not building weather routine\n");
	return 0;
#endif
}

#ifdef BUILD_WEATHER
static size_t sb_weather_curl_cb(char *buffer, size_t size, size_t num, void *thunk)
{
	char   **data = thunk;
	size_t   len;
	size_t   newlen;

	len    = strlen(*data);
	newlen = (size * num) + len + 1;
	*data  = realloc(*data, newlen);

	memcpy(*data+len, buffer, size*num);
	(*data)[newlen] = '\0';

	return size * num;
}

static SB_BOOL sb_weather_read_properties(CURL *curl, const char *response, char url_daily[], size_t daily_size, char url_hourly[], size_t hourly_size, sb_routine_t *routine)
{
	cJSON *json;
	cJSON *props;
	cJSON *url;

	memset(url_daily, 0, daily_size);
	memset(url_hourly, 0, hourly_size);

	json = cJSON_Parse(response);
	if (json == NULL) {
		fprintf(stderr, "%s routine: Failed to parse properties response\n", routine->name);
		cJSON_Delete(json);
		return SB_FALSE;
	}

	props = cJSON_GetObjectItem(json, "properties");
	if (props == NULL) {
		fprintf(stderr, "%s routine: Failed to find \"properties\" node\n", routine->name);
		cJSON_Delete(json);
		return SB_FALSE;
	}

	url = cJSON_GetObjectItem(props, "forecast");
	if (url == NULL) {
		fprintf(stderr, "%s routine: Failed to find \"forecast\" node\n", routine->name);
		cJSON_Delete(json);
		return SB_FALSE;
	}
	strncpy(url_daily, url->valuestring, daily_size-1);

	url = cJSON_GetObjectItem(props, "forecastHourly");
	if (url == NULL) {
		fprintf(stderr, "%s routine: Failed to find \"forecastHourly\" node\n", routine->name);
		cJSON_Delete(json);
		return SB_FALSE;
	}
	strncpy(url_hourly, url->valuestring, hourly_size-1);

	cJSON_Delete(json);
	return SB_TRUE;
}

static SB_BOOL sb_weather_read_coordinates(CURL *curl, const char *response, char url[], size_t size, sb_routine_t *routine)
{
 	/* A successful response will look something like this:
	 * {"status":1,"output":[{"zip":"90210","latitude":"34.103131","longitude":"-118.416253"}]}
	 * An unsuccessful response will look something like this:
	 * {"status":-3,"msg":"No results found"}
	 */
	cJSON *json;
	cJSON *tmp;
	cJSON *num;
	float  lat;
	float  lon;

	json = cJSON_Parse(response);
	if (json == NULL) {
		fprintf(stderr, "%s routine: Failed to parse zip code response\n", routine->name);
		cJSON_Delete(json);
		return SB_FALSE;
	}

	/* Check that we don't have an error status code. */
	tmp = cJSON_GetObjectItem(json, "status");
	if (tmp->valueint != 1) {
		fprintf(stderr, "%s routine: response returned code %d\n", routine->name, tmp->valueint);
		cJSON_Delete(json);
		return SB_FALSE;
	}

	tmp = cJSON_GetObjectItem(json, "output");
	tmp = cJSON_GetArrayItem(tmp, 0);

	num  = cJSON_GetObjectItem(tmp, "latitude");
	lat = atof(num->valuestring);

	num  = cJSON_GetObjectItem(tmp, "longitude");
	lon = atof(num->valuestring);

	/* Write next URL, which is for getting the zone and identifiers of the area. */
	snprintf(url, size-1, "https://api.weather.gov/points/%.4f,%.4f", lat, lon);
	curl_easy_setopt(curl, CURLOPT_URL, url);

	cJSON_Delete(json);
	return SB_TRUE;
}

static SB_BOOL sb_weather_perform_curl(CURL *curl, char **response, const char *data, sb_routine_t *routine)
{
	CURLcode  ret;
	long      code;
	char     *type; /* this will get free'd during curl_easy_cleanup() */

	if (*response != NULL)
		free(*response);
	*response = calloc(1, sizeof(**response));

	ret = curl_easy_perform(curl);
	if (ret != CURLE_OK) {
		fprintf(stderr, "%s routine: Failed to get %s: %s\n", routine->name, data, curl_easy_strerror(ret));
		return SB_FALSE;
	}

	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
	if (code != 200) {
		fprintf(stderr, "%s routine: curl returned %ld for %s\n", routine->name, code, data);
		return SB_FALSE;
	}

	/* Check for content type equal to JSON or GeoJSON. */
    curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &type);
	if (strcasecmp(type, "application/json") != 0 && strcasecmp(type, "application/geo+json") != 0) {
		fprintf(stderr, "%s routine: Mismatch content type (%s) for %s\n", routine->name, type, data);
		return SB_FALSE;
	}

	return SB_TRUE;
}

static SB_BOOL sb_weather_init_curl(CURL **curl, char errbuf[], struct curl_slist **headers, char url[], size_t size, char **response, sb_routine_t *routine)
{
	*curl = curl_easy_init();
	if (*curl == NULL) {
		fprintf(stderr, "%s routine: Failed to initialize curl handle\n", routine->name);
		return SB_FALSE;
	}

	snprintf(url, size-1, "https://api.promaptools.com/service/us/zip-lat-lng/get/?zip=%s&key=17o8dysaCDrgv1c", zip_code);
	curl_easy_setopt(*curl, CURLOPT_URL, url);

	*headers = curl_slist_append(NULL, "accept: application/json");

	curl_easy_setopt(*curl, CURLOPT_HTTPHEADER, *headers);
	curl_easy_setopt(*curl, CURLOPT_USERAGENT, "curl/7.58.0");
	curl_easy_setopt(*curl, CURLOPT_ERRORBUFFER, errbuf);
	curl_easy_setopt(*curl, CURLOPT_WRITEFUNCTION, sb_weather_curl_cb);
	curl_easy_setopt(*curl, CURLOPT_WRITEDATA, response);
	curl_easy_setopt(*curl, CURLOPT_SSL_VERIFYPEER, 0);

	return SB_TRUE;
}
#endif

static void *sb_weather_routine(void *thunk)
{
	sb_routine_t *routine = thunk;

#ifdef BUILD_WEATHER
	SB_TIMER_VARS;
	CURL              *curl = NULL;
	char               errbuf[CURL_ERROR_SIZE] = {0};
	struct curl_slist *headers;
	char               url[128];
	char               url_hourly[128];
	char              *response = NULL;

	if (!sb_weather_init_curl(&curl, errbuf, &headers, url, sizeof(url), &response, routine)) {
		routine->print = SB_FALSE;
	} else if (!sb_weather_perform_curl(curl, &response, "coordinates", routine)) {
		routine->print = SB_FALSE;
	} else if (!sb_weather_read_coordinates(curl, response, url, sizeof(url), routine)) {
		routine->print = SB_FALSE;
	} else if (!sb_weather_perform_curl(curl, &response, "properties", routine)) {
		routine->print = SB_FALSE;
	} else if (!sb_weather_read_properties(curl, response, url, sizeof(url), url_hourly, sizeof(url_hourly), routine)) {
		routine->print = SB_FALSE;
	}

	if (strlen(errbuf) > 0)
		fprintf(stderr, "%s: curl error: %s\n", routine->name, errbuf);

	routine->color = routine->colors.normal;
	while (routine->print) {
		SB_START_TIMER;

		if (!sb_weather_perform_curl(curl, &response, "forecast", routine))
			break;

		/* TODO: run routine */

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output), "weather: TODO");
		pthread_mutex_unlock(&(routine->mutex));

		SB_STOP_TIMER;
		SB_SLEEP;
	}
	free(response);
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
#endif

	if (pthread_mutex_destroy(&(routine->mutex)) != 0)
		SB_PRINT_ERROR("Failed to destroy mutex", NULL);
	routine->print = SB_FALSE;
	return NULL;
}


/* --- WIFI ROUTINE --- */
#ifdef BUILD_WIFI
static SB_BOOL sb_wifi_init(struct iwreq *iwr, char *essid, size_t max_len, sb_routine_t *routine)
{
	int             sock;
	struct ifaddrs *ifaddrs = NULL;
	struct ifaddrs *ifap;

	memset(essid, 0, max_len);
	iwr->u.essid.pointer = (caddr_t *)essid;
	iwr->u.data.length   = max_len;
	iwr->u.data.flags    = 0;

	/* open socket and return file descriptor for it */
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		SB_PRINT_ERROR("Failed to open socket file descriptor", NULL);
		return SB_FALSE;
	}

	/* get all network interfaces */
	if (getifaddrs(&ifaddrs) < 0 || ifaddrs == NULL) {
		SB_PRINT_ERROR("Failed to find interface addresses", NULL);
		close(sock);
		return SB_FALSE;
	}
	ifap = ifaddrs;

	/* go through each interface until one returns an ssid */
	while (ifap != NULL) {
		strncpy(iwr->ifr_ifrn.ifrn_name, ifap->ifa_name, IFNAMSIZ);
		if (ioctl(sock, SIOCGIWESSID, iwr) >= 0) {
			/* we found a match */
			freeifaddrs(ifaddrs);
			close(sock);
			return SB_TRUE;
		}
		/* no match found, try the next interface */
		ifap = ifap->ifa_next;
	}

	/* if we reached here, then we didn't find anything */
	SB_PRINT_ERROR("No wireless interfaces found", NULL);
	freeifaddrs(ifaddrs);
	close(sock);
	return SB_FALSE;
}
#endif

static void *sb_wifi_routine(void *thunk)
{
	/* First, we are going to loop through all network interfaces, checking for an SSID.
	 * When we first find one, we'll break out of the loop and use that interface as
	 * the wireless network. */
	sb_routine_t *routine = thunk;

#ifdef BUILD_WIFI
	SB_TIMER_VARS;
	struct iwreq iwr;
	char         essid[IW_ESSID_MAX_SIZE + 1];
	int          sock;

	if (!sb_wifi_init(&iwr, essid, sizeof(essid), routine))
		routine->print = SB_FALSE;

	routine->color = routine->colors.normal;
	while (routine->print) {
		SB_START_TIMER;

		memset(essid, 0, sizeof(essid));

		sock = socket(AF_INET, SOCK_DGRAM, 0);
		if (sock < 0) {
			SB_PRINT_ERROR("Failed to open socket file descriptor", NULL);
			break;
		}

		if (ioctl(sock, SIOCGIWESSID, &iwr) < 0) {
			routine->color = routine->colors.warning;
			pthread_mutex_lock(&(routine->mutex));
			snprintf(routine->output, sizeof(routine->output), "Not Connected");
			pthread_mutex_unlock(&(routine->mutex));
		} else {
			pthread_mutex_lock(&(routine->mutex));
			snprintf(routine->output, sizeof(routine->output), "%s", essid);
			pthread_mutex_unlock(&(routine->mutex));
		}
		close(sock);

		SB_STOP_TIMER;
		SB_SLEEP;
	}
#endif

	if (pthread_mutex_destroy(&(routine->mutex)) != 0)
		SB_PRINT_ERROR("Failed to destroy mutex", NULL);
	routine->print = SB_FALSE;
	return NULL;
}


/* --- PRINT LOOP --- */
static void sb_print_get_time(char buf[], size_t size, struct timespec *start_tp, SB_BOOL blink)
{
	struct tm tm;

	/* convert time from seconds since epoch to local time */
	memset(&tm, 0, sizeof(tm));
	localtime_r(&(start_tp->tv_sec), &tm);

	strftime(buf, size-1, time_format, &tm);

	if (blink)
		*strchr(buf, ':') = ' ';
}

static void sb_print(void)
{
	/* Here, we are not using the SB_START_TIMER and SB_STOP_TIMER macros,
 	 * because we need to use CLOCK_REALTIME to get the actual system time. */
	SB_TIMER_VARS
	Display      *dpy;
	Window        root;
	size_t        offset;
	sb_routine_t *routine;
	char          full_output[SBLENGTH];
	SB_BOOL       blink    = SB_TRUE;
	size_t        len;
	long          interval = 1000000;

	dpy  = XOpenDisplay(NULL);
	root = RootWindow(dpy, DefaultScreen(dpy));

	while (1) {
		clock_gettime(CLOCK_REALTIME, &start_tp); /* START TIMER */

		offset  = 0;
		for (routine = routine_list; routine != NULL; routine = routine->next) {
			if (routine->print == SB_FALSE) {
				continue;
			} else if (routine->routine == DELIMITER) {
				memcpy(full_output+offset, ";", 1);
				offset += 1;
				continue;
			} else if (routine->routine == TIME) {
				if (blink)
					blink = SB_FALSE;
				else
					blink = SB_TRUE;
				sb_print_get_time(routine->output, sizeof(routine->output), &start_tp, blink);
			}

			pthread_mutex_lock(&(routine->mutex));
			len = strlen(routine->output);
			if (len == 0) {
				pthread_mutex_unlock(&(routine->mutex));
				continue;
			} else if (offset+len > SBLENGTH - 1 + (color_text?10:0)) {
				fprintf(stderr, "Print: Exceeded max output length\n");
				pthread_mutex_unlock(&(routine->mutex));
				break;
			}

			memcpy(full_output+offset, "[", 1);
			offset += 1;

			/* Print opening status2d color code. */
			if (color_text) {
				memcpy(full_output+offset, "^c", 2);
				offset += 2;
				memcpy(full_output+offset, routine->color, 7);
				offset += 7;
				memcpy(full_output+offset, "^", 1);
				offset += 1;
			}

			memcpy(full_output+offset, routine->output, len);
			offset += len;

			/* Print status2d terminator code. */
			if (color_text) {
				memcpy(full_output+offset, "^d^", 3);
				offset += 3;
			}

			memcpy(full_output+offset, "] ", 2);
			offset += 2;

			pthread_mutex_unlock(&(routine->mutex));
		}
		full_output[offset] = '\0';

		XStoreName(dpy, root, full_output);
		XSync(dpy, False);

		clock_gettime(CLOCK_REALTIME, &finish_tp); /* STOP TIMER */
		elapsed_usec = (finish_tp.tv_sec - start_tp.tv_sec) +
				((finish_tp.tv_nsec - start_tp.tv_nsec) / 1000);

		if (elapsed_usec < interval) {
			if (usleep(interval - elapsed_usec) != 0) {
				fprintf(stderr, "Print routine: Error sleeping\n");
			}
		}
	}
#ifdef BUILD_WEATHER
	curl_global_cleanup(); /* Same lack of thread-safety as curl_global_init(). */
#endif

	fprintf(stderr, "Closing print loop, exiting program...\n");
	exit(EXIT_FAILURE);
}


static const struct thread_routines_t {
	enum sb_routine_e routine;
	void *(*callback)(void *thunk);
} possible_routines[] = {
	{ BATTERY,    sb_battery_routine    },
	{ CPU_TEMP,   sb_cpu_temp_routine   },
	{ CPU_USAGE,  sb_cpu_usage_routine  },
	{ DISK,       sb_disk_routine       },
	{ FAN,        sb_fan_routine        },
	{ LOAD,       sb_load_routine       },
	{ NETWORK,    sb_network_routine    },
	{ RAM,        sb_ram_routine        },
	{ TIME,       sb_null_cb            },
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
	enum sb_routine_e  next;
	sb_routine_t      *routine_object;

	num_routines = sizeof(chosen_routines) / sizeof(*chosen_routines);
	if (num_routines < 1) {
		fprintf(stderr, "No routines chosen, exiting...\n");
		return 1;
	}

	/* mark head of routine list */
	routine_list = &(routine_array[chosen_routines[0].routine]);

	/* step through each routine chosen in config.h and set it up */
	for (i=0; i<num_routines; i++) {
		index          = chosen_routines[i].routine;
		routine_object = &(routine_array[index]);

		/* string onto routine list */
		if (i+1 < num_routines) {
			next = chosen_routines[i+1].routine;
			routine_object->next = &(routine_array[next]);
		} else {
			routine_object->next = NULL;
		}

		/* initialize the routine */
		routine_object->routine = index;
		if (index == WEATHER) {
			/* From the libcurl docs, about curl_global_init():
 			 * "You must not call it when any other thread in the program (i.e. a
			 * thread sharing the same memory) is running. This doesn't just mean
			 * no other thread that is using libcurl. Because curl_global_init calls
			 * functions of other libraries that are similarly thread unsafe, it could
			 * conflict with any other thread that uses these other libraries."
			 */
			if (strlen(zip_code) != 5 || strspn(zip_code, "0123456789") != 5) {
				fprintf(stderr, "Weather routine: Zip Code must be 5 digits\n");
				continue;
			}

			if (chosen_routines[i].seconds < 30) {
				fprintf(stderr, "Weather routine: Interval time must be at least 30 seconds\n");
				continue;
			}

			if (sb_weather_global_init() != 0) {
				fprintf(stderr, "Weather routine: Failed to initialize global libcurl\n");
				continue;
			}
		} else if (index == DELIMITER) {
			snprintf(routine_object->output, sizeof(routine_object->output), ";");
			routine_object->print = SB_TRUE;
			continue;
		}

		if (
			strlen(chosen_routines[i].color_normal)  != 7 ||
			strlen(chosen_routines[i].color_warning) != 7 ||
			strlen(chosen_routines[i].color_error)   != 7
		) {
			fprintf(stderr, "%s: color must be RGB hex (\"#RRGGBB\")", routine_names[index]);
		} else {
			routine_object->thread_func    = possible_routines[index].callback;
			routine_object->interval       = chosen_routines[i].seconds * 1000000;
			routine_object->colors.normal  = chosen_routines[i].color_normal;
			routine_object->colors.warning = chosen_routines[i].color_warning;
			routine_object->colors.error   = chosen_routines[i].color_error;
			routine_object->color          = routine_object->colors.normal;
			routine_object->name           = routine_names[index];
			routine_object->print          = SB_TRUE;

			/* create thread */
			pthread_mutex_init(&(routine_object->mutex), NULL);
			pthread_create(&(routine_object->thread), NULL, routine_object->thread_func, (void *)routine_object);
		}
	}

	/* print loop */
	sb_print();

	return EXIT_SUCCESS;
}
