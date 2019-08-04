#include "statusbar.h"
#include "config.h"

#define SBLENGTH 10240

#define SB_START_TIMER \
		clock_gettime(CLOCK_MONOTONIC_RAW, &start_tp);

#define SB_STOP_TIMER \
		clock_gettime(CLOCK_MONOTONIC_RAW, &finish_tp);

#define SB_SLEEP \
		elapsed_usec = (finish_tp.tv_sec - start_tp.tv_sec) + ((finish_tp.tv_nsec - start_tp.tv_nsec) / 1000); \
		if (elapsed_usec < routine->interval) { \
			if (usleep(routine->interval - elapsed_usec) != 0) { \
				sb_print_error(routine, "Error sleeping"); \
			} \
		}

#define SB_TIMER_VARS \
	struct timespec start_tp  = {0}; \
	struct timespec finish_tp = {0}; \
	long            elapsed_usec;

/* --- HELPER FUNCTIONS --- */
static float sb_calc_magnitude(long number, char *unit)
{
	/* This will calculate how many commas the number would have add set the appropriate unit. */
	int  i;
	static const char symbols[] = "BKMGTP";

	if (number < 1000) {
		*unit = 'B';
		return number * 1.0;
	}

	for (i=0; number / powl(10, 3*i) > 999; i++);

	*unit = symbols[i];
	return (number / powl(10, 3*(i-1))) / 1000.0;
}

static void sb_debug(const char *name, const char *message, ...)
{
#ifdef DEBUG
	static struct timespec tp;
	static struct tm       tm;
	static const char     *time_format = "%H:%M:%S";

	va_list args;
	char    input[256];
	char    timestamp[32];

	va_start(args, message);

	vsnprintf(input, sizeof(input), message, args);

	pthread_mutex_lock(&debug_mutex);
	memset(&tm, 0, sizeof(tm));
	clock_gettime(CLOCK_REALTIME, &tp);
	localtime_r(&(tp.tv_sec), &tm);
	strftime(timestamp, sizeof(timestamp), time_format, &tm);

	printf("%s.%ld: %s: %s\n", timestamp, tp.tv_nsec, name, input);
	pthread_mutex_unlock(&debug_mutex);

	va_end(args);
#else
	(void)name;
	(void)message;
#endif
}

static SB_BOOL sb_isrgb(const char *color)
{
	/* Colors must be in this form: #RRGGBB, where RGB are hexadecimal digits. */
	int i;

	if (strlen(color) != 7)
		return SB_FALSE;

	if (color[0] != '#')
		return SB_FALSE;

	for (i=1; i<7; i++) {
		if (!isxdigit(color[i]))
			return SB_FALSE;
	}

	return SB_TRUE;
}

static void sb_leak_check(const char *name)
{
#ifdef DEBUG_LEAKS
	/* If a leak is found, this will halt the program.
 	 * It must be run in every non-terminating loop. */
	if (__lsan_do_recoverable_leak_check() != 0) {
		sb_debug(name, "leak found");
		exit(EXIT_FAILURE);
	}
	sb_debug(name, "no leaks found");
#else
	(void)name;
#endif
}

static long sb_normalize_perc(long num)
{
	/* This will keep percentages within the range of 0-100. */
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

static void sb_print_error(sb_routine_t *routine, const char *format, ...)
{
	va_list args;
	char    input[256] = {0};

	if (routine == NULL)
		return;

	routine->color = routine->colors.error;
	pthread_mutex_lock(&(routine->mutex));
	snprintf(routine->output, sizeof(routine->output), "%s: Error", routine->name);
	pthread_mutex_unlock(&(routine->mutex));

	va_start(args, format);

	vsnprintf(input, sizeof(input)-1, format, args);
	fprintf(stderr, "%s: %s\n", routine->name, input);

	va_end(args);
}

static SB_BOOL sb_read_file(char buf[], size_t size, const char *base, const char *file, sb_routine_t *routine)
{
	/* This will construct a path by concatentating base and file (or just using base if no file is passed in),
 	 * open the file at the path, and handle its reading and closing, with error handling along the way. */
	char  path[512];
	FILE *fd;

	sb_debug(__func__, "Reading %s%s", base, file?file:"");
	memset(buf, 0, size);

	snprintf(path, sizeof(path), "%s%s", base, file?file:"");
	fd = fopen(path, "r");
	if (fd == NULL) {
		sb_print_error(routine, "Failed to open %s", path);
		return SB_FALSE;
	}

	if (fgets(buf, size, fd) == NULL) {
		sb_print_error(routine, "Failed to read %s", path);
		fclose(fd);
		return SB_FALSE;
	}

	if (fclose(fd) != 0) {
		sb_print_error(routine, "Failed to close %s", path);
		return SB_FALSE;
	}

	return SB_TRUE;
}

static SB_BOOL sb_get_path(char buf[], size_t size, const char *base, const char *file, const char *match, sb_routine_t *routine)
{
	/* This will open the directory at base and search through every subdirectory until a file with
	 * contents equal to match are found. The path to that file is then written to buf. */
	DIR           *dir;
	struct dirent *dirent;
	char           path[512];
	char           contents[512];

	sb_debug(__func__, "Looking in %s for %s=%s", base, file, match);
	memset(buf, 0, size);

	dir = opendir(base);
	if (dir == NULL) {
		sb_print_error(routine, "Failed to open %s", base);
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

	sb_print_error(routine, "Failed to find file %s", NULL);
	closedir(dir);
	return SB_FALSE;
}


/* --- BATTERY ROUTINE --- */
static void *sb_battery_routine(void *thunk)
{
	sb_routine_t *routine = thunk;

#ifdef BUILD_BATTERY
	SB_TIMER_VARS;
	static const char *base = "/sys/class/power_supply";
	static const char *file = "type";
	char               path[512];
	char               buf[512];
	long               max;
	long               now;
	long               perc;

	if (!sb_get_path(path, sizeof(path), base, file, "Battery", routine)) {
		routine->run = SB_FALSE;
	} else if (!sb_read_file(buf, sizeof(buf), path, "charge_full", routine)) {
		routine->run = SB_FALSE;
	} else {
		max = atol(buf);
		if (max <= 0) {
			sb_print_error(routine, "Failed to read max level");
			routine->run = SB_FALSE;
		}
	}
	if (routine->run)
		sb_debug(__func__, "init: found %s", path);
	sb_leak_check(__func__);

	while (routine->run) {
		SB_START_TIMER;

		if (!sb_read_file(buf, sizeof(buf), path, "charge_now", routine))
			break;

		now = atol(buf);
		if (now < 0) {
			sb_print_error(routine, "Failed to read current level");
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

		sb_leak_check(__func__);
		SB_STOP_TIMER;
		SB_SLEEP;
	}
#else
	sb_print_error(routine, "routine was selected but not built during compilation. Check config.log");
#endif

	routine->run = SB_FALSE;
	sb_leak_check(__func__);
	return NULL;
}


/* --- CPU TEMP ROUTINE --- */
#ifdef BUILD_CPU_TEMP
static SB_BOOL sb_cpu_temp_get_filename(char path[], char filename[], size_t size, sb_routine_t *routine)
{
	/* This will open the directory at path and look at every file until it finds one named
	 * temp*_input, where * is a number from 0 to 9. It will then save that filename to filename. */
	DIR           *dir;
	struct dirent *dirent;

	sb_debug(__func__, "init: looking for file at %s", path);

	dir = opendir(path);
	if (dir == NULL) {
		sb_print_error(routine, "Failed to open %s", path);
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

	sb_print_error(routine, "Failed to find temperature monitor");
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
		routine->run = SB_FALSE;
	} else if (!sb_cpu_temp_get_filename(path, filename, sizeof(filename), routine)) {
		routine->run = SB_FALSE;
	} else {
		sb_debug(__func__, "init: found %s%s", path, filename);
	}
	sb_leak_check(__func__);

	while (routine->run) {
		SB_START_TIMER;

		if (!sb_read_file(contents, sizeof(contents), path, filename, routine))
			break;

		now = atol(contents);
		if (now < 0) {
			sb_print_error(routine, "Failed to read temperature");
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

		sb_leak_check(__func__);
		SB_STOP_TIMER;
		SB_SLEEP;
	}
#else
	sb_print_error(routine, "routine was selected but not built during compilation. Check config.log");
#endif

	routine->run = SB_FALSE;
	sb_leak_check(__func__);
	return NULL;
}


/* --- CPU USAGE ROUTINE --- */
#ifdef BUILD_CPU_USAGE
static SB_BOOL sb_cpu_usage_get_ratio(int *ratio)
{
	/* Calculate the thread-to-processor ratio. */
	int procs;  /* number of processors */
	int online; /* number online */

	procs  = get_nprocs_conf();
	online = get_nprocs();

	if (procs < 1 || online < 1)
		return SB_FALSE;

	*ratio = online / procs;

	return SB_TRUE;
}
#endif

static void *sb_cpu_usage_routine(void *thunk)
{
	sb_routine_t      *routine = thunk;

#ifdef BUILD_CPU_USAGE
	SB_TIMER_VARS;
	int                ratio;
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

	sb_debug(__func__, "init: get thread-to-processor ratio");
	if (!sb_cpu_usage_get_ratio(&ratio)) {
		sb_print_error(routine, "Failed to determine ratio");
		routine->run = SB_FALSE;
	}
	sb_debug(__func__, "init: thread-to-processor ratio: %d", ratio);
	sb_leak_check(__func__);

	memset(&old, 0, sizeof(old));
	memset(&new, 0, sizeof(new));
	while (routine->run) {
		SB_START_TIMER;

		if (!sb_read_file(contents, sizeof(contents), path, NULL, routine))
			break;
		if (sscanf(contents, "cpu %lu %lu %lu %lu", &new.user, &new.nice, &new.system, &new.idle) != 4 ) {
			sb_print_error(routine, "Failed to read %s", path);
			break;
		}

		used  = (new.user-old.user) + (new.nice-old.nice) + (new.system-old.system);
		total = (new.user-old.user) + (new.nice-old.nice) + (new.system-old.system) + (new.idle-old.idle);
		perc  = ((used * 100) / total) / ratio;
		perc  = sb_normalize_perc(perc);
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

		sb_leak_check(__func__);
		SB_STOP_TIMER;
		SB_SLEEP;
	}
#else
	sb_print_error(routine, "routine was selected but not built during compilation. Check config.log");
#endif

	routine->run = SB_FALSE;
	sb_leak_check(__func__);
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

	while (routine->run) {
		SB_START_TIMER;

		/* In this routine, we're going to lock the mutex for the entire operation so we
		 * can safely add to the routine's output for the entire loop. */
		pthread_mutex_lock(&(routine->mutex));
		*routine->output = '\0';

		color_level     = 1;
		routine->color  = routine->colors.normal; /* start at normal */
		num_filesystems = sizeof(filesystems) / sizeof(*filesystems);
		sb_debug(__func__, "reading %zu filesystems", num_filesystems);
		for (i=0; i<num_filesystems; i++) {
			if (statvfs(filesystems[i].path, &stats) != 0) {
				sb_print_error(routine, "Failed to get stats for %s", filesystems[i].path);
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

		sb_leak_check(__func__);
		SB_STOP_TIMER;
		SB_SLEEP;
	}
#else
	sb_print_error(routine, "routine was selected but not built during compilation. Check config.log");
#endif

	routine->run = SB_FALSE;
	sb_leak_check(__func__);
	return NULL;
}


/* --- FAN ROUTINE --- */
#ifdef BUILD_FAN
static SB_BOOL sb_fan_get_path(char path[], size_t size, sb_routine_t *routine)
{
	/* This will open the directory at base and look through every subdirectory
	 * for another directory named "device". If that is present, it will search
	 * through that directory until it finds a file named "fan*_output", where * is
	 * a digit from 0 to 9. */
	static const char *base = "/sys/class/hwmon";
	DIR               *dir;
	struct dirent     *dirent;
	DIR               *device;
	struct dirent     *devent;

	sb_debug(__func__, "init: looking in %s for device", base);

	dir = opendir(base);
	if (dir == NULL) {
		sb_print_error(routine, "Failed to open %s", base);
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

	sb_print_error(routine, "Failed to find a fan");
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
		routine->run = SB_FALSE;
	} else if (!sb_read_file(contents, sizeof(contents), path, "_max", routine)) {
		sb_print_error(routine, "%s routine: Failed to read %s_max", path);
		routine->run = SB_FALSE;
	} else {
		strncat(path, "_output", sizeof(path)-strlen(path)-1);
		sb_debug(__func__, "init: found %s", path);
		max = atol(contents);
		if (max < 0)
			routine->run = SB_FALSE;
	}
	sb_leak_check(__func__);

	while (routine->run) {
		SB_START_TIMER;

		if (!sb_read_file(contents, sizeof(contents), path, NULL, routine))
			break;

		now = atol(contents);
		if (now < 0) {
			sb_print_error(routine, "Failed to read current fan speed");
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

		sb_leak_check(__func__);
		SB_STOP_TIMER;
		SB_SLEEP;
	}
#else
	sb_print_error(routine, "routine was selected but not built during compilation. Check config.log");
#endif

	routine->run = SB_FALSE;
	sb_leak_check(__func__);
	return NULL;
}


/* --- LOAD ROUTINE --- */
static void *sb_load_routine(void *thunk)
{
	sb_routine_t *routine = thunk;

#ifdef BUILD_LOAD
	SB_TIMER_VARS;
	double loads[3];

	while (routine->run) {
		SB_START_TIMER;

		if (getloadavg(loads, 3) != 3) {
			sb_print_error(routine, "Failed to read loads");
			break;
		}

		if (loads[0] >= 2 || loads[1] >= 2 || loads[2] >= 2) {
			routine->color = routine->colors.error;
		} else if (loads[0] >= 1 || loads[1] >= 1 || loads[2] >= 1) {
			routine->color = routine->colors.warning;
		} else {
			routine->color = routine->colors.normal;
		}

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output), "%.2lf, %.2lf, %.2lf", loads[0], loads[1], loads[2]);
		pthread_mutex_unlock(&(routine->mutex));

		sb_leak_check(__func__);
		SB_STOP_TIMER;
		SB_SLEEP;
	}
#else
	sb_print_error(routine, "routine was selected but not built during compilation. Check config.log");
#endif

	routine->run = SB_FALSE;
	sb_leak_check(__func__);
	return NULL;
}


/* --- NETWORK ROUTINE --- */
#ifdef BUILD_NETWORK
struct sb_network_t {
	char path[IFNAMSIZ+64];
	long old_bytes; /* bytes from the last run */
	long new_bytes; /* bytes from the current run */
	long reduced;   /* bytes reduced to the thousands */
	char unit;
};

static SB_BOOL sb_network_get_paths(struct sb_network_t *rx_file, struct sb_network_t *tx_file, sb_routine_t *routine)
{
	/* This will get all the network interfaces and look for one that is running
 	 * and not a loopback. */
	int             sock;
	struct ifreq    ifr;
	struct ifaddrs *ifaddrs = NULL;
	struct ifaddrs *ifap;

	sb_debug(__func__, "init: finding interface path");

	/* open socket and return file descriptor for it */
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		sb_print_error(routine, "Failed to open socket file descriptor");
		return SB_FALSE;
	}
	sb_debug(__func__, "init: opened socket");

	/* get all network interfaces */
	if (getifaddrs(&ifaddrs) < 0 || ifaddrs == NULL) {
		sb_print_error(routine, "Failed to find interface addresses");
		close(sock);
		return SB_FALSE;
	}
	ifap = ifaddrs;
	sb_debug(__func__, "init: got list of interfaces");

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
		sb_print_error(routine, "No wireless interfaces found");
		return SB_FALSE;
	}

	return SB_TRUE;
}
#endif

static void *sb_network_routine(void *thunk)
{
	/* This routine is going to read two files, rx_bytes and tx_bytes, for the current
 	 * up-and-running network interface. It will compare the number of bytes between
	 * loops to get the current network throughput. */
	sb_routine_t *routine = thunk;

#ifdef BUILD_NETWORK
	SB_TIMER_VARS;
	struct sb_network_t files[2] = {0};
	SB_BOOL             error;
	int                 i;
	char                contents[128];
	int                 color_level;

	if (!sb_network_get_paths(&files[0], &files[1], routine)) {
		routine->run = SB_FALSE;
	} else {
		sb_debug(__func__, "init: found %s for receiving", files[0].path);
		sb_debug(__func__, "init: found %s for sending", files[1].path);
	}
	sb_leak_check(__func__);

	while (routine->run) {
		SB_START_TIMER;

		color_level    = 1;
		routine->color = routine->colors.normal;
		error = SB_FALSE;
		for (i=0; i<2 && !error; i++) {
			files[i].old_bytes = files[i].new_bytes;
			if (!sb_read_file(contents, sizeof(contents), files[i].path, NULL, routine)) {
				error = SB_TRUE;
			} else if (sscanf(contents, "%ld", &files[i].new_bytes) != 1) {
				sb_print_error(routine, "Failed to read %s", files[i].path);
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
		snprintf(routine->output, sizeof(routine->output), "%3ld%c down/%3ld%c up",
				files[0].reduced, files[0].unit, files[1].reduced, files[1].unit);
		pthread_mutex_unlock(&(routine->mutex));

		sb_leak_check(__func__);
		SB_STOP_TIMER;
		SB_SLEEP;
	}
#else
	sb_print_error(routine, "routine was selected but not built during compilation. Check config.log");
#endif

	routine->run = SB_FALSE;
	sb_leak_check(__func__);
	return NULL;
}


/* --- RAM ROUTINE --- */
static void *sb_ram_routine(void *thunk)
{
	sb_routine_t *routine = thunk;

#ifdef BUILD_RAM
	SB_TIMER_VARS
	long  page_size;
	long  avail_l;
	float avail_f;
	char  avail_unit;
	long  total_l;
	float total_f;
	char  total_unit;
	long  perc;

	page_size = sysconf(_SC_PAGESIZE);

	/* calculate available and total bytes */
	avail_l = sysconf(_SC_AVPHYS_PAGES) * page_size;
	total_l = sysconf(_SC_PHYS_PAGES)   * page_size;
	if (avail_l < 1 || total_l < 1) {
		sb_print_error(routine, "Failed to get memory amounts");
		routine->run = SB_FALSE;
	} else {
		total_f = sb_calc_magnitude(total_l, &total_unit);
		sb_debug(__func__, "init: calculated total bytes free");
	}
	sb_leak_check(__func__);

	while (routine->run) {
		SB_START_TIMER;

		/* get available memory */
		avail_l = sysconf(_SC_AVPHYS_PAGES) * page_size;
		if (avail_l < 1) {
			sb_print_error(routine, "Failed to get available bytes");
			break;
		}

		perc  = sb_normalize_perc((avail_l*100)/total_l);
		if (perc < 75) {
			routine->color = routine->colors.normal;
		} else if (perc < 90) {
			routine->color = routine->colors.warning;
		} else {
			routine->color = routine->colors.error;
		}

		avail_f = sb_calc_magnitude(avail_l, &avail_unit);

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output), "%.1f%c free/%.1f%c",
				avail_f, avail_unit, total_f, total_unit);
		pthread_mutex_unlock(&(routine->mutex));

		sb_leak_check(__func__);
		SB_STOP_TIMER;
		SB_SLEEP;
	}
#else
	sb_print_error(routine, "routine was selected but not built during compilation. Check config.log");
#endif

	routine->run = SB_FALSE;
	sb_leak_check(__func__);
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
	}

	return 0;
}
#endif

static void *sb_todo_routine(void *thunk)
{
	/* We're going to read in the first two lines of the user's personal TODO list
 	 * and print them based on a few rules:
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

	snprintf(path, sizeof(path), "%s/%s", getenv("HOME"), todo_path);
	sb_debug(__func__, "init: using %s", path);
	sb_leak_check(__func__);

	routine->color = routine->colors.normal;
	while (routine->run) {
		SB_START_TIMER;

		fd = fopen(path, "r");
		if (fd == NULL) {
			sb_print_error(routine, "Failed to open %s", path);
			break;
		} else if (fgets(line[0].line, sizeof(line[0].line), fd) == NULL) {
			line[0].isempty = SB_TRUE;
		} else if (fgets(line[1].line, sizeof(line[1].line), fd) == NULL) {
			line[1].isempty = SB_TRUE;
		} else if (fclose(fd) != 0) {
			sb_print_error(routine, "Failed to close %s", path);
			fclose(fd);
			break;
		}

		if (line[0].isempty || line[1].isempty) {
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
				line[i].line[strlen(line[i].line)-1] = '\0'; /* don't print newline character */
				line[i].ptr += sb_todo_count_blanks(line[i].line, &line[i].isempty);
			}
		}

		pthread_mutex_lock(&(routine->mutex));
		if (line[0].isempty && line[1].isempty) {
			snprintf(routine->output, sizeof(routine->output), "Finished");
		} else {
			snprintf(routine->output, sizeof(routine->output), "%s%s%s",
					line[0].ptr, separator, line[1].ptr);
		}
		pthread_mutex_unlock(&(routine->mutex));

		sb_leak_check(__func__);
		SB_STOP_TIMER;
		SB_SLEEP;
	}
#else
	(void)todo_path;
	sb_print_error(routine, "routine was selected but not built during compilation. Check config.log");
#endif

	routine->run = SB_FALSE;
	sb_leak_check(__func__);
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
		sb_print_error(routine, "Failed to open mixer");
	} else if (snd_mixer_attach(*mixer, card) < 0) {
		sb_print_error(routine, "Failed to attach mixer");
	} else if (snd_mixer_selem_register(*mixer, NULL, NULL) < 0) {
		sb_print_error(routine, "Failed to register mixer");
	} else if (snd_mixer_load(*mixer) < 0) {
		sb_print_error(routine, "Failed to load mixer");

	/* get id */
	} else if (snd_mixer_selem_id_malloc(&snd_id) != 0) {
		sb_print_error(routine, "Failed to allocate snd_id");

	/* get element */
	} else {
		sb_debug(__func__, "init: opened mixer");
		sb_debug(__func__, "init: opened id");

		snd_mixer_selem_id_set_index(snd_id, index);
		snd_mixer_selem_id_set_name(snd_id, name);
		*snd_elem = snd_mixer_find_selem(*mixer, snd_id);
		snd_mixer_selem_id_free(snd_id);
		if (*snd_elem == NULL) {
			sb_print_error(routine, "Failed to find element");
		} else if (snd_mixer_selem_has_playback_volume(*snd_elem) == 0) {
			sb_print_error(routine, "Element does not have playback volume");
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

	sb_debug(__func__, "init: open element");
	if (!sb_volume_get_snd_elem(&mixer, &snd_elem, routine)) {
		routine->run = SB_FALSE;
	} else if (snd_mixer_selem_get_playback_dB_range(snd_elem, &min, &max) != 0) {
		sb_print_error(routine, "Failed to get decibels range");
		routine->run = SB_FALSE;
	} else {
		sb_debug(__func__, "init: opened element");
	}
	sb_leak_check(__func__);

	while (routine->run) {
		SB_START_TIMER;

		if (snd_mixer_handle_events(mixer) < 0) {
			sb_print_error(routine, "Failed to clear mixer");
			break;
		} else if (snd_mixer_selem_get_playback_switch(snd_elem, SND_MIXER_SCHN_MONO, &mute) != 0) {
			sb_print_error(routine, "Failed to get mute state");
			break;
		} else if (mute == 0) {
			sb_debug(__func__, "sound is muted");
			pthread_mutex_lock(&(routine->mutex));
			snprintf(routine->output, sizeof(routine->output), "mute");
			pthread_mutex_unlock(&(routine->mutex));
		} else if (snd_mixer_selem_get_playback_dB(snd_elem, SND_MIXER_SCHN_MONO, &decibels) != 0) {
			sb_print_error(routine, "Failed to get decibels");
			break;
		} else {
			sb_debug(__func__, "current decibels: %ld", decibels);
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

		sb_leak_check(__func__);
		SB_STOP_TIMER;
		SB_SLEEP;
	}

	if (mixer != NULL) {
		snd_mixer_close(mixer);
		snd_mixer_free(mixer);
	}
	if (snd_elem != NULL)
		snd_mixer_elem_free(snd_elem);
#else
	sb_print_error(routine, "routine was selected but not built during compilation. Check config.log");
#endif

	routine->run = SB_FALSE;
	sb_leak_check(__func__);
	return NULL;
}


/* --- WEATHER ROUTINE --- */
#ifdef BUILD_WEATHER
struct sb_weather_t {
	CURL              *curl;
	struct curl_slist *headers;
	char               url[128];  /* Temporary URL during init, temperature URL during loop. */
	char               url2[128]; /* Empty during init, daily forecast URL during loop. */
	char              *response;
	size_t             len;
};

static size_t sb_weather_curl_cb(char *buffer, size_t size, size_t num, void *thunk)
{
	struct sb_weather_t *info = thunk;
	size_t               buffer_len;

	buffer_len     = size*num;
	info->response = realloc(info->response, info->len + buffer_len + 1);

	memcpy(info->response+info->len, buffer, buffer_len);

	info->len                 += buffer_len;
	info->response[info->len]  = '\0';

	return buffer_len;
}

static void sb_weather_clear_response(struct sb_weather_t *info)
{
	free(info->response);

	info->response = NULL;
	info->len      = 0;
	sb_leak_check(__func__);
}

static SB_BOOL sb_weather_perform_curl(struct sb_weather_t *info, const char *data, sb_routine_t *routine)
{
	CURLcode  ret;
	long      code;
	char     *type; /* this will get free'd during curl_easy_cleanup() */

	ret = curl_easy_perform(info->curl);
	if (ret != CURLE_OK) {
		sb_print_error(routine, "Failed to get %s: %s", data, curl_easy_strerror(ret));
		return SB_FALSE;
	}

	curl_easy_getinfo(info->curl, CURLINFO_RESPONSE_CODE, &code);
	if (code != 200) {
		sb_print_error(routine, "curl returned %ld for %s", code, data);
		return SB_FALSE;
	}

	/* Check for content type equal to JSON or GeoJSON. */
    curl_easy_getinfo(info->curl, CURLINFO_CONTENT_TYPE, &type);
	if (strcasecmp(type, "application/json") != 0 && strcasecmp(type, "application/geo+json") != 0) {
		sb_print_error(routine, "Mismatch content type (%s) for %s", type, data);
		return SB_FALSE;
	}

	return SB_TRUE;
}

static SB_BOOL sb_weather_get_forecast(struct sb_weather_t *info, int *low, int *high, sb_routine_t *routine)
{
	cJSON *json;
	cJSON *tmp;
	cJSON *array;
	int    i;

	/* Set daily forecast URL. */
	curl_easy_setopt(info->curl, CURLOPT_URL, info->url2);

	sb_debug(__func__, "get forecast");
	if (!sb_weather_perform_curl(info, "daily forecast", routine))
		return SB_FALSE;

	json = cJSON_Parse(info->response);
	if (json == NULL) {
		sb_print_error(routine, "Failed to parse forecast response");
		cJSON_Delete(json);
		return SB_FALSE;
	}

	tmp = cJSON_GetObjectItem(json, "properties");
	if (tmp == NULL) {
		sb_print_error(routine, "Failed to find forecast \"properties\" node");
		cJSON_Delete(json);
		return SB_FALSE;
	}

	array = cJSON_GetObjectItem(tmp, "periods");
	if (array == NULL) {
		sb_print_error(routine, "Failed to find forecast \"periods\" array node");
		cJSON_Delete(json);
		return SB_FALSE;
	}

	/* We want to skip past the nodes for Today/This Afternoon and Tonight and grab the next two after that. */
	i   = 0;
	tmp = cJSON_GetArrayItem(array, 0);
	tmp = cJSON_GetObjectItem(tmp, "name");
	if (strcmp(tmp->valuestring, "Overnight") == 0) {
		i = 3;
	} else if (strcmp(tmp->valuestring, "Today") == 0 || strcmp(tmp->valuestring, "This Afternoon") == 0) {
		i = 2;
	} else if (strcmp(tmp->valuestring, "Tonight") == 0) {
		i = 1;
	} else {
		sb_print_error(routine, "Error in forecast array");
		cJSON_Delete(json);
		return SB_FALSE;
	}

	tmp   = cJSON_GetArrayItem(array, i);
	tmp   = cJSON_GetObjectItem(tmp, "temperature");
	*high = tmp->valueint;

	i++;
	tmp  = cJSON_GetArrayItem(array, i);
	tmp  = cJSON_GetObjectItem(tmp, "temperature");
	*low = tmp->valueint;

	cJSON_Delete(json);
	sb_weather_clear_response(info);
	return SB_TRUE;
}

static SB_BOOL sb_weather_get_temperature(struct sb_weather_t *info, int *temp, sb_routine_t *routine)
{
	cJSON *json;
	cJSON *tmp;

	/* Set hourly temperature URL. */
	curl_easy_setopt(info->curl, CURLOPT_URL, info->url);

	sb_debug(__func__, "get current temperature");
	if (!sb_weather_perform_curl(info, "temperature", routine))
		return SB_FALSE;

	json = cJSON_Parse(info->response);
	if (json == NULL) {
		sb_print_error(routine, "Failed to parse temperature response");
		cJSON_Delete(json);
		return SB_FALSE;
	}

	tmp = cJSON_GetObjectItem(json, "properties");
	if (tmp == NULL) {
		sb_print_error(routine, "Failed to find temperature \"properties\" node");
		cJSON_Delete(json);
		return SB_FALSE;
	}

	tmp = cJSON_GetObjectItem(tmp, "periods");
	if (tmp == NULL) {
		sb_print_error(routine, "Failed to find temperature \"periods\" array node");
		cJSON_Delete(json);
		return SB_FALSE;
	}

	tmp = cJSON_GetArrayItem(tmp, 0);
	tmp = cJSON_GetObjectItem(tmp, "temperature");
	if (tmp == NULL) {
		sb_print_error(routine, "Failed to find \"temperature\" array node");
		cJSON_Delete(json);
		return SB_FALSE;
	}
	*temp = tmp->valueint;

	cJSON_Delete(json);
	sb_weather_clear_response(info);
	return SB_TRUE;
}

static SB_BOOL sb_weather_get_temperature_url(struct sb_weather_t *info, sb_routine_t *routine)
{
	cJSON *json;
	cJSON *props;
	cJSON *url;

	sb_debug(__func__, "init: getting temperature URL info");
	if (!sb_weather_perform_curl(info, "forecast url", routine))
		return SB_FALSE;

	json = cJSON_Parse(info->response);
	if (json == NULL) {
		sb_print_error(routine, "Failed to parse properties response");
		cJSON_Delete(json);
		return SB_FALSE;
	}
	sb_debug(__func__, "init: read JSON");

	props = cJSON_GetObjectItem(json, "properties");
	if (props == NULL) {
		sb_print_error(routine, "Failed to find \"properties\" node");
		cJSON_Delete(json);
		return SB_FALSE;
	}
	sb_debug(__func__, "init: found \"properties\" node");

	url = cJSON_GetObjectItem(props, "forecast");
	if (url == NULL) {
		sb_print_error(routine, "Failed to find \"forecast\" node");
		cJSON_Delete(json);
		return SB_FALSE;
	}
	sb_debug(__func__, "init: found \"forecast\" node");

	/* Store daily forecast URL. */
	strncpy(info->url2, url->valuestring, sizeof(info->url2)-1);
	sb_debug(__func__, "init: stored daily forecast URL");

	/* Store hourly temperature URL. */
	strncpy(info->url, info->url2, sizeof(info->url)-1);
	strncat(info->url, "/hourly", sizeof(info->url)-strlen(info->url)-1);
	sb_debug(__func__, "stored hourly temperature URL");

	cJSON_Delete(json);
	sb_weather_clear_response(info);
	return SB_TRUE;
}

static SB_BOOL sb_weather_get_coordinates(struct sb_weather_t *info, sb_routine_t *routine)
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

	sb_debug(__func__, "init: getting coordinates info");
	if (!sb_weather_perform_curl(info, "coordinates", routine))
		return SB_FALSE;

	json = cJSON_Parse(info->response);
	if (json == NULL) {
		sb_print_error(routine, "Failed to parse zip code response");
		cJSON_Delete(json);
		return SB_FALSE;
	}
	sb_debug(__func__, "init: opened JSON");

	/* Check that we don't have an error status code. */
	tmp = cJSON_GetObjectItem(json, "status");
	if (tmp->valueint != 1) {
		sb_print_error(routine, "Response returned code %d", tmp->valueint);
		cJSON_Delete(json);
		return SB_FALSE;
	}

	tmp = cJSON_GetObjectItem(json, "output");
	tmp = cJSON_GetArrayItem(tmp, 0);
	if (tmp == NULL) {
		sb_print_error(routine, "Failed to find \"output\" node");
		cJSON_Delete(json);
		return SB_FALSE;
	} else {
		sb_debug(__func__, "init: found \"output\" node");
	}

	num  = cJSON_GetObjectItem(tmp, "latitude");
	lat = atof(num->valuestring);
	sb_debug(__func__, "init: using latitude %f", lat);

	num  = cJSON_GetObjectItem(tmp, "longitude");
	lon = atof(num->valuestring);
	sb_debug(__func__, "init: using longitude %f", lon);

	/* Write coordinates into next url, which is for getting the zone and identifiers of the area. */
	snprintf(info->url, sizeof(info->url)-1, "https://api.weather.gov/points/%.4f,%.4f", lat, lon);
	curl_easy_setopt(info->curl, CURLOPT_URL, info->url);
	sb_debug(__func__, "init: prepared next URL");

	cJSON_Delete(json);
	sb_weather_clear_response(info);
	return SB_TRUE;
}

static SB_BOOL sb_weather_init_curl(struct sb_weather_t *info, char errbuf[], sb_routine_t *routine)
{
	sb_debug(__func__, "init: intializing libcurl object");
	memset(info, 0, sizeof(*info));

	info->curl = curl_easy_init();
	if (info->curl == NULL) {
		sb_print_error(routine, "Failed to initialize curl handle");
		return SB_FALSE;
	}

	info->headers = curl_slist_append(NULL, "accept: application/json");
	curl_easy_setopt(info->curl, CURLOPT_HTTPHEADER, info->headers);
	sb_debug(__func__, "init: set header");

	snprintf(info->url, sizeof(info->url)-1, "https://api.promaptools.com/service/us/zip-lat-lng/get/?zip=%s&key=17o8dysaCDrgv1c", zip_code);
	curl_easy_setopt(info->curl, CURLOPT_URL, info->url);
	sb_debug(__func__, "init: set first URL");

	curl_easy_setopt(info->curl, CURLOPT_ERRORBUFFER, errbuf);
	sb_debug(__func__, "init: set error buffer");

	curl_easy_setopt(info->curl, CURLOPT_USERAGENT, "curl/7.9.7+");
	sb_debug(__func__, "init: set user agent");

	curl_easy_setopt(info->curl, CURLOPT_WRITEFUNCTION, sb_weather_curl_cb);
	sb_debug(__func__, "init: set write callback function");

	curl_easy_setopt(info->curl, CURLOPT_WRITEDATA, info);
	sb_debug(__func__, "init: set write callback data");

#ifdef DEBUG
	curl_easy_setopt(info->curl, CURLOPT_VERBOSE, 1L);
	/* libcurl directs verbose output to stderr, so we'll
 	 * redirect it to stdout here. */
	curl_easy_setopt(info->curl, CURLOPT_STDERR, stdout);
	sb_debug(__func__, "init: set verbose mode");
#endif

	return SB_TRUE;
}
#endif

static void *sb_weather_routine(void *thunk)
{
	sb_routine_t *routine = thunk;

#ifdef BUILD_WEATHER
	SB_TIMER_VARS;
	struct sb_weather_t info;
	char                errbuf[CURL_ERROR_SIZE] = {0};
	int                 temp;
	int                 low;
	int                 high;

	if (!sb_weather_init_curl(&info, errbuf, routine)) {
		routine->run = SB_FALSE;
	} else if (!sb_weather_get_coordinates(&info, routine)) {
		routine->run = SB_FALSE;
	} else if (!sb_weather_get_temperature_url(&info, routine)) {
		routine->run = SB_FALSE;
	} else {
		sb_debug(__func__, "init: successful");
	}
	sb_leak_check(__func__);

	routine->color = routine->colors.normal;
	while (routine->run) {
		SB_START_TIMER;

		if (!sb_weather_get_temperature(&info, &temp, routine))
			break;
		if (!sb_weather_get_forecast(&info, &low, &high, routine))
			break;

		pthread_mutex_lock(&(routine->mutex));
		snprintf(routine->output, sizeof(routine->output), "weather: %d °F (%d/%d)", temp, low, high);
		pthread_mutex_unlock(&(routine->mutex));

		sb_leak_check(__func__);
		SB_STOP_TIMER;
		SB_SLEEP;
	}

	if (strlen(errbuf) > 0)
		sb_print_error(routine, "cURL error: %s", errbuf);

	if (info.response != NULL)
		free(info.response);
	if (info.headers != NULL)
		curl_slist_free_all(info.headers);
	curl_easy_cleanup(info.curl);
#else
	sb_print_error(routine, "routine was selected but not built during compilation. Check config.log");
#endif

	routine->run = SB_FALSE;
	sb_leak_check(__func__);
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

	sb_debug(__func__, "initializing wifi");

	/* open socket and return file descriptor for it */
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		sb_print_error(routine, "Failed to open socket file descriptor");
		return SB_FALSE;
	}
	sb_debug(__func__, "opened socket");

	/* get all network interfaces */
	if (getifaddrs(&ifaddrs) < 0 || ifaddrs == NULL) {
		sb_print_error(routine, "Failed to find interface addresses");
		close(sock);
		return SB_FALSE;
	}
	ifap = ifaddrs;
	sb_debug(__func__, "got list of interfaces");

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
	sb_print_error(routine, "No wireless interfaces found");
	freeifaddrs(ifaddrs);
	close(sock);
	return SB_FALSE;
}
#endif

static void *sb_wifi_routine(void *thunk)
{
	/* First, we are going to loop through all network interfaces, checking for an SSID.
	 * When we find one, we'll break out of the loop and use that interface as
	 * the wireless network. We'll run this again if the wireless connection ever goes
	 * down until we find another suitable connection. Until then, we'll print "Wifi Down". */
	sb_routine_t *routine = thunk;

#ifdef BUILD_WIFI
	SB_TIMER_VARS;
	struct iwreq iwr;
	char         essid[IW_ESSID_MAX_SIZE + 1];
	int          sock;
	SB_BOOL      found = SB_FALSE;

	while (routine->run) {
		SB_START_TIMER;

		memset(essid, 0, sizeof(essid));
		if (!found && !sb_wifi_init(&iwr, essid, sizeof(essid), routine))
			break;

		sock = socket(AF_INET, SOCK_DGRAM, 0);
		if (sock < 0) {
			sb_print_error(routine, "Failed to open socket file descriptor");
			break;
		}

		if (ioctl(sock, SIOCGIWESSID, &iwr) < 0) {
			found = SB_FALSE;
			routine->color = routine->colors.warning;
			sb_debug(__func__, "Wifi is not connected");

			pthread_mutex_lock(&(routine->mutex));
			snprintf(routine->output, sizeof(routine->output), "Not Connected");
			pthread_mutex_unlock(&(routine->mutex));
		} else {
			found = SB_TRUE;
			if (strlen(essid) == 0) {
				sb_debug(__func__, "Wifi is connected but down");
				snprintf(essid, sizeof(essid)-1, "Wifi Down");
				routine->color = routine->colors.error;
			} else {
				sb_debug(__func__, "Wifi is operating on network %s", essid);
				routine->color = routine->colors.normal;
			}

			pthread_mutex_lock(&(routine->mutex));
			snprintf(routine->output, sizeof(routine->output), "%s", essid);
			pthread_mutex_unlock(&(routine->mutex));
		}
		close(sock);
		sb_debug(__func__, "Closed socket");

		sb_leak_check(__func__);
		SB_STOP_TIMER;
		SB_SLEEP;
	}
#else
	sb_print_error(routine, "routine was selected but not built during compilation. Check config.log");
#endif

	routine->run = SB_FALSE;
	sb_leak_check(__func__);
	return NULL;
}


/* --- PRINT LOOP --- */
static void sb_copy_output(char *full_output, sb_routine_t *routine)
{
	strcat(full_output, "[");

	/* Print opening status2d color code. */
	if (color_text) {
		strcat(full_output, "^c");
		strcat(full_output, routine->color);
		strcat(full_output, "^");
	}

	strcat(full_output, routine->output);
	sb_debug(__func__, "%zu bytes: %s", strlen(routine->output), routine->output);

	/* Print status2d terminator code. */
	if (color_text)
		strcat(full_output, "^d^");

	strcat(full_output, "] ");
}

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
	sb_debug(__func__, "starting...");
	/* Here, we are not using the SB_START_TIMER and SB_STOP_TIMER macros,
 	 * because we need to use CLOCK_REALTIME to get the actual system time. */
	SB_TIMER_VARS
	Display      *dpy;
	Window        root;
	sb_routine_t *routine;
	char          full_output[SBLENGTH];
	SB_BOOL       blink = SB_TRUE;
	size_t        len;

	dpy  = XOpenDisplay(NULL);
	root = RootWindow(dpy, DefaultScreen(dpy));

	while (1) {
		sb_debug(__func__, "starting print loop");
		clock_gettime(CLOCK_REALTIME, &start_tp); /* START TIMER */

		memset(full_output, 0, SBLENGTH);
		for (routine = routine_list; routine != NULL; routine = routine->next) {
			if (routine->routine == DELIMITER) {
				sb_debug(__func__, "adding delimiter");
				strcat(full_output, ";");
				continue;
			} else if (routine->routine == TIME) {
				sb_debug(__func__, "printing time");
				if (blink)
					blink = SB_FALSE;
				else
					blink = SB_TRUE;
				sb_print_get_time(routine->output, sizeof(routine->output), &start_tp, blink);
			}

			pthread_mutex_lock(&(routine->mutex));

			len = strlen(routine->output);
			if (len == 0) {
				sb_debug(__func__, "empty, skipping");
				pthread_mutex_unlock(&(routine->mutex));
				continue;
			} else if (strlen(full_output)+len+1 > SBLENGTH+(color_text?10:0)) {
				fprintf(stderr, "Print: %s: exceeded max output length\n", routine->name);
				pthread_mutex_unlock(&(routine->mutex));
				break;
			}

			sb_copy_output(full_output, routine);

			pthread_mutex_unlock(&(routine->mutex));
		}

		sb_debug(__func__, "Send output to statusbar");
		XStoreName(dpy, root, full_output);
		XSync(dpy, False);

		sb_leak_check(__func__);
		clock_gettime(CLOCK_REALTIME, &finish_tp); /* STOP TIMER */
		elapsed_usec = (finish_tp.tv_sec - start_tp.tv_sec) +
				((finish_tp.tv_nsec - start_tp.tv_nsec) / 1000);

		if (elapsed_usec < 1000000) {
			if (usleep(1000000 - elapsed_usec) != 0) {
				fprintf(stderr, "Print routine: Error sleeping\n");
			}
		}
	}
#ifdef BUILD_WEATHER
	sb_debug(__func__, "clean up global libcurl object");
	curl_global_cleanup(); /* Same lack of thread-safety as curl_global_init(). */
#endif

	fprintf(stderr, "Closing print loop, exiting program...\n");
	sb_leak_check(__func__);
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
static SB_BOOL parse_config(void)
{
	static const char *path = "src/options.conf";
	FILE              *fd;
	char               buf[512];

	sb_debug(__func__, "Parsing config");

	fd = fopen(path, "r");
	if (fd == NULL) {
		fprintf(stderr, "Config: Failed to open %s", path);
		return SB_FALSE;
	}
	sb_debug(__func__, "opened %s", path);

	sb_debug(__func__, "reading config lines");
	while (fgets(buf, sizeof(buf), fd) != NULL) {
		/* Ignore comment lines. */
		if (buf[0] == '#' || buf[0] == '\n')
			continue;
	}

	return SB_TRUE;
}

int main(int argc, char *argv[])
{
	size_t             num_routines;
	int                i;
	enum sb_routine_e  index;
	enum sb_routine_e  next;
	sb_routine_t      *routine_object;

#ifdef DEBUG
	/* Create debug mutex so we can print debug statements. */
	pthread_mutex_init(&debug_mutex, NULL);
#else
	(void)debug_mutex;
#endif

	sb_debug(__func__, "running statusbar with debug output enabled");

	num_routines = sizeof(chosen_routines) / sizeof(*chosen_routines);
	sb_debug(__func__, "%zu routines chosen", num_routines);
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
		if (index == DELIMITER) {
			sb_debug(__func__, "don't initialize delimiter");
			continue;
		} else if (index == WEATHER) {
#ifdef BUILD_WEATHER
			/* From the libcurl docs, about curl_global_init():
 			 * "You must not call it when any other thread in the program (i.e. a
			 * thread sharing the same memory) is running. This doesn't just mean
			 * no other thread that is using libcurl. Because curl_global_init calls
			 * functions of other libraries that are similarly thread unsafe, it could
			 * conflict with any other thread that uses these other libraries."
			 */
			sb_debug(__func__, "Checking weather arguments");

			if (strlen(zip_code) != 5 || strspn(zip_code, "0123456789") != 5) {
				fprintf(stderr, "Weather routine: Zip Code must be 5 digits\n");
				continue;
			}
			sb_debug("Weather", "zip code is good");

			if (chosen_routines[i].seconds < 30) {
				fprintf(stderr, "Weather routine: Interval time must be at least 30 seconds\n");
				continue;
			}
			sb_debug("Weather", "interval is good");

			sb_debug("Weather", "starting libcurl global init");
			if (curl_global_init(CURL_GLOBAL_SSL) != 0) {
				fprintf(stderr, "Weather routine: Failed to initialize global libcurl\n");
				continue;
			}
			sb_debug("Weather", "libcurl global init is good");
#endif
		}

		if (
			/* Check that all 3 colors are 7 characters long and hexadecimal. */
			color_text == SB_TRUE &&
			(
				!sb_isrgb(chosen_routines[i].color_normal)  ||
				!sb_isrgb(chosen_routines[i].color_warning) ||
				!sb_isrgb(chosen_routines[i].color_error)
			)
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
			routine_object->run            = SB_TRUE;

			sb_debug(__func__, "Initializing %s:", routine_object->name);
			sb_debug(routine_object->name, "Interval: %ld sec", routine_object->interval / 1000000);
			sb_debug(routine_object->name, "Normal color: %s", routine_object->colors.normal);
			sb_debug(routine_object->name, "Warning color: %s", routine_object->colors.warning);
			sb_debug(routine_object->name, "Error color: %s", routine_object->colors.error);

			/* create thread */
			pthread_mutex_init(&(routine_object->mutex), NULL);
			pthread_create(&(routine_object->thread), NULL, routine_object->thread_func, (void *)routine_object);
			sb_debug(routine_object->name, "Thread created");
		}
	}
	sb_leak_check(__func__);

	/* print loop */
	sb_print();

	return EXIT_SUCCESS;
}
