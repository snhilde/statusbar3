#include <stdio.h>
#include <pthread.h>
#include <X11/Xlib.h>
#include <unistd.h>
#include <string.h>


enum sb_process_e {
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
	ENDOFLIST  = -1 /* terminator for configs array */
};

typedef struct configs_t {
	enum sb_process_e process;
	int               bar;
}configs_t;

typedef struct sb_process_t {
	enum sb_process_e    process;
	unsigned int         bar;
	char                 output[256];
	size_t               length;
	pthread_t            thread;
	pthread_mutex_t      mutex;
	void *(*thread_func)(void *);
	struct sb_process_t *next;
} sb_process_t;

unsigned long sb_flags_active = 0;

sb_process_t proc_arr[16];
