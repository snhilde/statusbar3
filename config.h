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
};

/* 1 to display keyboard brightness with screen brightness */
#define SHOW_KBD 1
