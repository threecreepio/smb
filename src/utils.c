#include <time.h>

#ifdef __WIN32__
#include <windows.h>
#define TIME_UTC 1

static inline int
rte_timespec_get(struct timespec *now, int base)
{
	/* 100ns ticks from 1601-01-01 to 1970-01-01 */
	static const uint64_t EPOCH = 116444736000000000ULL;
	static const uint64_t TICKS_PER_SEC = 10000000;
	static const uint64_t NS_PER_TICK = 100;

	FILETIME ft;
	uint64_t ticks;

	if (base != TIME_UTC)
		return 0;

	GetSystemTimePreciseAsFileTime(&ft);
	ticks = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
	ticks -= EPOCH;
	now->tv_sec = ticks / TICKS_PER_SEC;
	now->tv_nsec = (ticks - now->tv_sec * TICKS_PER_SEC) * NS_PER_TICK;
	return base;
}

#define timespec_get(ts, base) rte_timespec_get(ts, base)
#endif

static inline double get_millis(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    double sec = ts.tv_sec * 1000.0;
    double ns = ts.tv_nsec / 1000000.0;
    return sec + ns;
}
