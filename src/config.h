/* Which routines to run (and display), how often (in seconds) they should update,
 * and which colors to use for them.
 *
 * There are 5 comma-delimited values for each routine:
 * 1. The routine (possible values are in enum sb_routine_e in src/statusbar.h)
 * 2. How often in seconds to run each routine. For example, a value 5 means to
 *    run every 5 seconds.
 * 3. The color in RGB form to use for normal operating conditions.
 * 4. The color in RGB form to use for warning operating conditions.
 * 5. The color in RGB form to use for error operating conditions.
 *
 * The specifications of the 3 operating conditions for each routine can be
 * found in the README under "Getting Started -> Colors".

 * As explained in the README under "Recommendations", if you are using the
 * dualstatus patch for dwm, you can split the output between the two bars
 * using the DELIMITER routine. All routines before DELIMITER will appear on
 * the first bar, and all routines after on the second bar. If you are not
 * using this patch, you should remove the DELIMITER routine below. */
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

/* With the status2d patch, you can color the output of each
 * routine using the color codes above.
 * SB_TRUE  = colored output based on color codes above
 * SB_FALSE = default color based on dwm settings */
static SB_BOOL color_text = SB_TRUE;

/* The mounted filesystems to display for the DISK routine.
 * The first value is the absolute path to the partition's mount point.
 * The second value is the display name to use for it. */
static const struct {
	const char *path;
	const char *display_name;
} filesystems[] = {
	{ "/"    , "root" },
	{ "/home", "home" },
};

/* The conversion specification for the TIME routine.
 * See strftime(3) for more options. */
static const char *time_format = "%b %d - %I:%M";

/* For the TODO routine, the path to the user's TODO list.
 * Note: This path is relative to the home directory of the
 * user running the program. */
static const char *todo_path = ".TODO";

/* For the WEATHER routine, the zip code to use for
 * getting weather data. */
static const char *zip_code = "90210";
