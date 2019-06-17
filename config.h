/* Which routines to run (and display) and how often (in seconds) they should update. */
static const struct {
	enum sb_routine_e routine;
	time_t            seconds;
} chosen_routines[] = {
	/* TOP BAR */
	{ TODO      , 5       },
	{ WEATHER   , 60 * 30 },
	{ BACKUP    , 5       },
	{ WIFI      , 5       },
	{ TIME      , 1       },

	/* DELIMITER BETWEEN BARS */
	{ DELIMITER , 0       },
	
	/*BOTTOM BAR */
	{ NETWORK   , 1       },
	{ DISK      , 5       },
	{ RAM       , 1       },
	{ LOAD      , 1       },
	{ CPU_USAGE , 1       },
	{ CPU_TEMP  , 1       },
	{ FAN       , 1       },
	{ BATTERY   , 30      },
	{ VOLUME    , 1       },
	{ BRIGHTNESS, 1       },
};

/* Format of the clock (see strftime(3) for conversion specifications). */
const char *time_format = "%b %d - %I:%M";

/* Which mounted filesystems to display for the disk routine. */
static const struct {
	const char *path;
	const char *display_name;
} filesystems[] = {
	{ "/"    , "root" },
	{ "/home", "home" },
};

/* Use 1 to display keyboard brightness with screen brightness or 0 to not. */
#define SHOW_KBD 1
