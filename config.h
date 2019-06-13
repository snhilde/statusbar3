static const struct {
	enum sb_routine_e routine;
	time_t            seconds;
} chosen_routines[] = {
	/* TOP BAR */
	{ LOG       , 5       },
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

/* format of the clock (see strftime(3) for conversion specifications) */
const char *time_format = "%b %d - %I:%M";

/* 1 to display keyboard brightness with screen brightness */
#define SHOW_KBD 1
