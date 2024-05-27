//#ifndef __ZENTIMER_H__
//#define __ZENTIMER_H__



//#include <stdio.h>
#ifdef WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif
#ifdef HAVE_STDINT_H
#include <stdint.h>
#elif HAVE_INTTYPES_H
#include <inttypes.h>
#else
typedef unsigned char uint8_t;
typedef unsigned long int uint32_t;
typedef unsigned long long uint64_t;
#endif

//#ifdef __cplusplus
//extern "C" {
//#pragma }
//#endif /* __cplusplus */

#define ZTIME_USEC_PER_SEC 1000000

/* ztime_t represents usec */
typedef uint64_t ztime_t;

#ifdef WIN32
static uint64_t ztimer_freq = 0;
#endif

static void
ztime (ztime_t *ztimep)
{

#ifdef WIN32
    QueryPerformanceCounter ((LARGE_INTEGER *) ztimep);
	//printf("Creating ZenTimer\n");
	
#else
    struct timeval tv;

    gettimeofday (&tv, NULL);

    *ztimep = ((uint64_t) tv.tv_sec * ZTIME_USEC_PER_SEC) + tv.tv_usec;

#endif
}

enum {
    ZTIMER_INACTIVE = 0,
    ZTIMER_ACTIVE   = (1 << 0),
    ZTIMER_PAUSED   = (1 << 1),
};

typedef struct {
    ztime_t start;
    ztime_t stop;
    int state;
} ztimer_t;

#define ZTIMER_INITIALIZER { 0, 0, 0 }

/* default timer */
static ztimer_t __ztimer = ZTIMER_INITIALIZER;

static void
ZenTimerStart (ztimer_t *ztimer)
{
    ztimer = ztimer ? ztimer : &__ztimer;

    ztimer->state = ZTIMER_ACTIVE;
    ztime (&ztimer->start);
    printf("ZenTimer starting\n");
}

static void
ZenTimerStop (ztimer_t *ztimer)
{
    ztimer = ztimer ? ztimer : &__ztimer;

    ztime (&ztimer->stop);
    ztimer->state = ZTIMER_INACTIVE;
}

static void
ZenTimerStop2 (ztimer_t *ztimer)
{
    ztimer = ztimer ? ztimer : &__ztimer;

    //ztime (&ztimer->stop);
	ztimer->stop=0; /* if you dont do this the timers will be stopped but ZenTimerElapsed will report stale value */
	ztimer->start=0; /* if you dont do this the timers will be stopped but ZenTimerElapsed will report stale value */
    ztimer->state = ZTIMER_INACTIVE;
}



static void
ZenTimerPause (ztimer_t *ztimer)
{
    ztimer = ztimer ? ztimer : &__ztimer;

    ztime (&ztimer->stop);
    ztimer->state |= ZTIMER_PAUSED;
}

static void
ZenTimerResume (ztimer_t *ztimer)
{
    ztime_t now, delta;

    ztimer = ztimer ? ztimer : &__ztimer;

    /* unpause */
    ztimer->state &= ~ZTIMER_PAUSED;

    ztime (&now);

    /* calculate time since paused */
    delta = now - ztimer->stop;

    /* adjust start time to account for time elapsed since paused */
    ztimer->start += delta;
}

static double
ZenTimerElapsed (ztimer_t *ztimer, uint64_t *usec)
{
#ifdef WIN32
    static uint64_t freq = 0;
    ztime_t delta, stop;

    if (freq == 0)
        QueryPerformanceFrequency ((LARGE_INTEGER *) &freq);
#else
#define freq ZTIME_USEC_PER_SEC
    ztime_t delta, stop;
#endif

    ztimer = ztimer ? ztimer : &__ztimer;

    if (ztimer->state != ZTIMER_ACTIVE)
        stop = ztimer->stop;
    else
        ztime (&stop);

    delta = stop - ztimer->start;

    if (usec != NULL)
        *usec = (uint64_t) (delta * ((double) ZTIME_USEC_PER_SEC / (double) freq));

    return (double) delta / (double) freq;
}

static void
ZenTimerReport (ztimer_t *ztimer, const char *oper)
{
    fprintf (stderr, "ZenTimer: %s took %.6f seconds\n", oper, ZenTimerElapsed (ztimer, NULL));
}


//#define ZenTimerStart(ztimerp)
//#define ZenTimerStop(ztimerp)
//#define ZenTimerPause(ztimerp)
//#define ZenTimerResume(ztimerp)
//#define ZenTimerElapsed(ztimerp, usec)
//#define ZenTimerReport(ztimerp, oper)



//#endif /* __ZENTIMER_H__ */
