#include <stdio.h>
#include <pthread.h>
#include <X11/Xlib.h>
#include <unistd.h>
#include <string.h>


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
	DELIMITER  = 100,
	ENDOFLIST  = -1 /* terminator value */
};

typedef struct sb_routine {
	enum sb_routine_e    routine;     /* The number assigned to each routine. This is used to
	                                     access the routine's flags and to match it to various
										 checks and calls. */
	char                 output[256]; /*  */
	size_t               length;      /*  */
	pthread_t            thread;      /*  */
	pthread_mutex_t      mutex;       /*  */
	void *(*thread_func)(void *);     /*  */
	struct sb_routine *next;          /*  */
} sb_routine_t;

/* These are all the flags for the routine. Because it is global, it is zero'd out on
 * startup. If a user does not chose a particular routine in the config file, that routine's
 * flag will remain zero. If the routine is chosen, then its bit flag will be set to 1. */
unsigned long sb_flags_active;

/* This array will hold all the routine objects. Because it is global, it is zero'd out on
 * startup. If a user does not chose a particular routine in the config file, that
 * routine's index will remain empty. */
sb_routine_t routine_array[16];
