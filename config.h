/* Which routines to run (and display) and how often (in seconds) they should update. */
static const struct {
	enum sb_routine_e  routine;
	time_t             seconds;
	const char        *color;
} chosen_routines[] = {
	/* TOP BAR */
	{ TODO     , 5      , "#FFFFFF" },
	{ WEATHER  , 60 * 30, "#FFFFFF" },
	{ WIFI     , 5      , "#FFFFFF" },
	{ TIME     , 1      , "#FFFFFF" },

	/* DELIMITER BETWEEN BARS */
	{ DELIMITER, 0      , NULL      },

	/*BOTTOM BAR */
	{ NETWORK  , 1      , "#FFFFFF" },
	{ DISK     , 5      , "#FFFFFF" },
	{ RAM      , 1      , "#FFFFFF" },
	{ LOAD     , 1      , "#FFFFFF" },
	{ CPU_USAGE, 1      , "#FFFFFF" },
	{ CPU_TEMP , 1      , "#FFFFFF" },
	{ FAN      , 1      , "#FFFFFF" },
	{ BATTERY  , 30     , "#FFFFFF" },
	{ VOLUME   , 1      , "#FFFFFF" },
};
SB_BOOL color_text = SB_TRUE; /* SB_FALSE to not color text, or if status2d patch is not installed */

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
