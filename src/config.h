/* Which routines to run (and display) and how often (in seconds) they should update. */
static const struct {
	enum sb_routine_e  routine;
	time_t             seconds;
	const char        *color_normal;
	const char        *color_warning;
	const char        *color_error;
} chosen_routines[] = {
	/* TOP BAR */       /* normal     warning    error */
	{ TODO     , 5      , "#FFFFFF", "#BB4F2E", "#A1273E" },
	{ WEATHER  , 60 * 30, "#FFFFFF", "#BB4F2E", "#A1273E" },
	{ WIFI     , 5      , "#FFFFFF", "#BB4F2E", "#A1273E" },
	{ TIME     , 1      , "#FFFFFF", "#BB4F2E", "#A1273E" },

	/* DELIMITER BETWEEN BARS */
	{ DELIMITER, 0      , NULL     , NULL     , NULL      },

	/*BOTTOM BAR */
	{ NETWORK  , 1      , "#FFFFFF", "#BB4F2E", "#A1273E" },
	{ DISK     , 5      , "#FFFFFF", "#BB4F2E", "#A1273E" },
	{ RAM      , 5      , "#FFFFFF", "#BB4F2E", "#A1273E" },
	{ LOAD     , 1      , "#FFFFFF", "#BB4F2E", "#A1273E" },
	{ CPU_USAGE, 1      , "#FFFFFF", "#BB4F2E", "#A1273E" },
	{ CPU_TEMP , 1      , "#FFFFFF", "#BB4F2E", "#A1273E" },
	{ FAN      , 1      , "#FFFFFF", "#BB4F2E", "#A1273E" },
	{ BATTERY  , 30     , "#FFFFFF", "#BB4F2E", "#A1273E" },
	{ VOLUME   , 1      , "#FFFFFF", "#BB4F2E", "#A1273E" },
};
SB_BOOL color_text = SB_TRUE; /* SB_FALSE to not color text, or if status2d patch is not installed */

/* Which mounted filesystems to display for the disk routine. */
static const struct {
	const char *path;
	const char *display_name;
} filesystems[] = {
	{ "/"    , "root" },
	{ "/home", "home" },
};

/* Format of the clock (see strftime(3) for conversion specifications). */
const char *time_format = "%b %d - %I:%M";

/* Zip Code for weather data. */
const char *zip_code = "90210";