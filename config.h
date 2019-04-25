#define TOP    0
#define BOTTOM 1

static const enum sb_routine_e chosen_routines[] = {
	/* TOP BAR */
	LOG,
	TODO,
	WEATHER,
	BACKUP,
	WIFI,
	TIME,

	/* DELIMITER BETWEEN BARS */
	DELIMITER,
	
	/*BOTTOM BAR */
	NETWORK,
	DISK,
	RAM,
	LOAD,
	CPU_USAGE,
	CPU_TEMP,
	FAN,
	BATTERY,
	VOLUME,
	BRIGHTNESS,

	/* this must always be the list item in this list */
	ENDOFLIST
};
