#include <stdio.h>
#include <pthread.h>
#include <X11/Xlib.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

/* for wifi routine */
#include <sys/ioctl.h>
#include <linux/wireless.h>
#include <ifaddrs.h>

enum sb_routine_e {
	BACKUP     = 0,
	BATTERY    = 1,
	BRIGHTNESS = 2,
	CPU_TEMP   = 3,
	CPU_USAGE  = 4,
	DISK       = 5,
	FAN        = 6,
	LOAD       = 7,
	LOG        = 8,
	NETWORK    = 9,
	RAM        = 10,
	TIME       = 11,
	TODO       = 12,
	VOLUME     = 13,
	WEATHER    = 14,
	WIFI       = 15,
	DELIMITER  = 16
};

static const char *routine_names[] = {
	"Backup",
	"Battery",
	"Brightness",
	"CPU Temp",
	"CPU Usage",
	"Disk",
	"Fan",
	"Load",
	"Log",
	"Network",
	"RAM",
	"Time",
	"TODO",
	"Volume",
	"Weather",
	"Wifi"
};

/* Routine object declaration */
typedef struct sb_routine {
	enum sb_routine_e  routine;     /* Number assigned to each routine. This is used to
	                                   access the routine's flags and to match it to various
	                                   checks and calls. */
	time_t             interval;    /* How often to call routine in seconds. */
	char               output[256]; /* String of data that each routine will output for
	                                   master status bar string to copy */
	size_t             length;      /* Length of output */
	pthread_t          thread;      /* Thread assigned to this routine */
	pthread_mutex_t    mutex;       /* Mutex assigned to this routine. This will be used to
	                                   lock output when reading from or writing to it. */
	void            *(*thread_func)(void *); /* Callback function for thread */
	struct sb_routine *next;        /* Pointer to next routine in list. This is how we are
	                                   going to keep track of the order or routines for
	                                   printing to the status bar. */
} sb_routine_t;

/* This array will hold all the routine objects. Because it is global, it is zero'd out on
 * startup. If a user does not chose a particular routine in the config file, that
 * routine's index will remain empty. */
sb_routine_t routine_array[sizeof(routine_names) / sizeof(*routine_names) + 1];

/* These are all the flags for the routine. Because it is global, it is zero'd out on
 * startup. If a user does not chose a particular routine in the config file, that routine's
 * flag will remain zero. If the routine is chosen, then its bit flag will be set to 1. */
unsigned long sb_flags_active;

/* This will be the ordered list of routines used for printing to the master string. */
sb_routine_t *routine_list;
