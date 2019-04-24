#define TOP    0
#define BOTTOM 1

static const configs_t configs[] = {
	/* TOP BAR */
	{ LOG,        TOP },
	{ TODO,       TOP },
	{ WEATHER,    TOP },
	{ BACKUP,     TOP },
	{ WIFI,       TOP },
	{ TIME,       TOP },
	
	/*BOTTOM BAR */
	{ NETWORK,    BOTTOM },
	{ DISK,       BOTTOM },
	{ RAM,        BOTTOM },
	{ LOAD,       BOTTOM },
	{ CPU_USAGE,  BOTTOM },
	{ CPU_TEMP,   BOTTOM },
	{ FAN,        BOTTOM },
	{ BATTERY,    BOTTOM },
	{ VOLUME,     BOTTOM },
	{ BRIGHTNESS, BOTTOM },
	{ -1, -1 }
};
