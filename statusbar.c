#include "statusbar.h"
#include "config.h"


static void *backup_callback(void *thunk)
{
	
	return NULL;
}

static void *battery_callback(void *thunk)
{
	
	return NULL;
}

static void *brightness_callback(void *thunk)
{
	
	return NULL;
}

static void *cpu_temp_callback(void *thunk)
{
	
	return NULL;
}

static void *cpu_usage_callback(void *thunk)
{
	
	return NULL;
}

static void *disk_callback(void *thunk)
{
	
	return NULL;
}

static void *fan_callback(void *thunk)
{
	
	return NULL;
}

static void *load_callback(void *thunk)
{
	
	return NULL;
}

static void *log_callback(void *thunk)
{
	
	return NULL;
}

static void *network_callback(void *thunk)
{
	
	return NULL;
}

static void *ram_callback(void *thunk)
{
	
	return NULL;
}

static void *time_callback(void *thunk)
{
	
	return NULL;
}

static void *todo_callback(void *thunk)
{
	
	return NULL;
}

static void *volume_callback(void *thunk)
{
	
	return NULL;
}

static void *weather_callback(void *thunk)
{
	
	return NULL;
}

static void *wifi_callback(void *thunk)
{
	
	return NULL;
}

int main(int argc, char *argv[])
{
	int i;
	int index;
	
	for (i = 0; configs[i].process > 0; i++) {
		index                = configs[i].process;
		sb_flags_active     |= 1<<index;
		proc_arr[index].bar  = configs[i].bar;
	}
	
	return 0;
}
