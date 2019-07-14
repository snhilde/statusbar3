#include <pthread.h>
#include <X11/Xlib.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <math.h>

#ifdef BUILD_DISK
  #include <sys/statvfs.h>
#endif

#ifdef BUILD_TODO
  #include <ctype.h>
#endif

#ifdef BUILD_NETWORK
  #include <sys/ioctl.h>
  #include <ifaddrs.h>
  #include <net/if.h>
#endif

#ifdef BUILD_WIFI
  #include <linux/wireless.h>
#endif

#ifdef BUILD_VOLUME
  #include <alsa/asoundlib.h>
#endif

enum sb_routine_e {
	BATTERY = 0,
	CPU_TEMP ,
	CPU_USAGE,
	DISK     ,
	FAN      ,
	LOAD     ,
	NETWORK  ,
	RAM      ,
	TIME     ,
	TODO     ,
	VOLUME   ,
	WEATHER  ,
	WIFI     ,
	DELIMITER,
};

typedef enum _SB_BOOL {
	SB_FALSE = 0,
	SB_TRUE  = 1
} SB_BOOL;

/* Routine object declaration */
typedef struct sb_routine {
	enum sb_routine_e  routine;     /* Number assigned to each routine. This is used to
	                                   access the routine's flags and to match it to various
	                                   checks and calls. */
	const char        *name;        /* Printable name of routine. */
	time_t             interval;    /* How often to call routine in seconds. */
	char               output[256]; /* String of data that each routine will output for
	                                   master status bar string to copy */
	const char        *color;       /* Font color */
	pthread_t          thread;      /* Thread assigned to this routine */
	pthread_mutex_t    mutex;       /* Mutex assigned to this routine. This will be used to
	                                   lock output when reading from or writing to it. */
	void            *(*thread_func)(void *); /* Callback function for thread */
	struct sb_routine *next;        /* Pointer to next routine in list. This is how we are
	                                   going to keep track of the order or routines for
	                                   printing to the status bar. */
	SB_BOOL            print;       /* SB_TRUE (default) means print to statusbar.
									   SB_FALSE means thread has exited and this routine should not be printed. */
} sb_routine_t;

static const char *routine_names[] = {
	"Battery",
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
sb_routine_t routine_array[DELIMITER + 1];

/* This will be the ordered list of routines used for printing to the master string. */
sb_routine_t *routine_list;
