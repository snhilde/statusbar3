#include <stdio.h>
#include <pthread.h>
#include <X11/Xlib.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

/* for disk routine */
#include <sys/statvfs.h>

/* for fan routine */
#include <dirent.h>

/* for wifi routine */
#include <sys/ioctl.h>
#include <linux/wireless.h>
#include <ifaddrs.h>


enum sb_routine_e {
	BACKUP = 0,
	BATTERY   ,
	BRIGHTNESS,
	CPU_TEMP  ,
	CPU_USAGE ,
	DISK      ,
	FAN       ,
	LOAD      ,
	NETWORK   ,
	RAM       ,
	TIME      ,
	TODO      ,
	VOLUME    ,
	WEATHER   ,
	WIFI      ,
	DELIMITER ,
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
	SB_BOOL            skip;        /* SB_FALSE (default) means don't skip
									   SB_TRUE means thread has exited and this routine should not be printed. */
} sb_routine_t;

static const char *routine_names[] = {
	"Backup",
	"Battery",
	"Brightness",
	"CPU Temp",
	"CPU Usage",
	"Disk",
	"Fan",
	"Load",
	"Network",
	"RAM",
	"Time",
	"TODO",
	"Volume",
	"Weather",
	"Wifi"
};

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
