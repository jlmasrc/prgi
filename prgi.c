/*
 * prgi: C library for easy and flexible progress indicators with
 * negligible overhead and support for multi-threaded programs.
 *
 * See documentation in prgi.h and examples.
 *
 * Copyright (C) 2023  Joao Luis Meloni Assirati.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include <sys/ioctl.h>

/* thread_local. If not available, use __thread */
#include <threads.h>

/* NAN, isfinite() */
#include <math.h>

#include "prgi.h"

/*
 * For all non-static functions, see also the corresponding
 * documentation at prgi.h.
 */

/*** Definitions **************************************************************/

/* PRGI_* definitions can be redefined during compilation. */

/* Maximum supported terminal width. */
#ifndef PRGI_MAXLINELEN
#define PRGI_MAXLINELEN 256
#endif

/* Maximum length of the progress bar */
#ifndef PRGI_MAXBARLEN
#define PRGI_MAXBARLEN PRGI_MAXLINELEN
#endif

/* Extra space to accomodate ANSI sequences in buffers. The value of 8
   is enough for the sequences "\e[7m" and "\e[0m" used in the
   progress bar with reverse video. If the user provided custom ANSI
   sequences in prgi_printf() and the line to be printed is close to
   PRGI_MAXLINELEN (which is improbable), the line may appear truncated. */
#ifndef PRGI_EXTRASIZE
#define PRGI_EXTRASIZE 9
#endif

/* Shorter names for use in this source file. */
#define MAXLINELEN PRGI_MAXLINELEN
#define MAXBARLEN  PRGI_MAXBARLEN
#define EXTRASIZE  PRGI_EXTRASIZE

/* Buffer sizes take into account PRGI_EXTRASIZE and the string terminator. */

/* Size of line buffers. */
#define LINEBUFSIZE (MAXLINELEN + EXTRASIZE + 1)

/* Size of progress bar buffer */
#define BARBUFSIZE (MAXBARLEN + EXTRASIZE + 1)

/************************************************************** Definitions ***/


/*** Utilities and wrappers ***************************************************/

/* Simple (not general) implementation of round() so we don't have to
   link against libm. Gives good results for x > 0. */
float myround(float x) {
    return (int)(x + (float)0.5);
}

/* Returns a pointer to the next printable (i.e. not belonging to an
   ANSI escape sequence) character after *p (in a string), or to the
   string terminator if there is no more printable chars. For now,
   only CSI sequences are recognized. See
   https://en.wikipedia.org/wiki/ANSI_escape_code */
static char *esc_skip(char const *p) {
    while(*p == '\e') {
        ++p;
        if(*p == '[') {
            ++p;
            while(0x30 <= *p && *p <= 0x3F) ++p; /* Parameter bytes */
            while(0x20 <= *p && *p <= 0x2F) ++p; /* Intermediate bytes */
            if(0x40 <= *p && *p <= 0x7E) ++p; /* Final byte */
        }
    }
    return (char *)p;
}

/* Like strlen(), but only counts printable characters. */
static int esc_strlen(char const *p) {
    int n = 0;
    for(;;) {
        p = esc_skip(p);
        if(*p == '\0') return n;
        ++n;
        ++p;
    }
}

/* Returns the maximal raw length of chars, starting from the
   beginning of s, that contains the first l printable characters of
   s. Examples:
   - esc_rawlen("abc\e[7mfgh", 4) = 8 (8 chars until the 4th printable
     char 'f', including the non-printable sequence "\e[7m")
   - esc_rawlen("abc\e[7mfgh", 3) = 7 (the 3rd char is c, but the
     function picks the maximal length, so the non-printable sequence
     "\e[7m" is also counted) */
static int esc_rawlen(char const *s, int l) {
    char const *p = s;
    int n = 0;

    for(;;) {
        p = esc_skip(p);
        if(n == l || *p == '\0') return p - s;
        ++n;
        ++p;
    }
}

/* This macro calls the vsnprintf() with arguments (buf, size, format)
   followed by all variadic arguments that follow format. It does not
   return a value like vsnprintf(). It simplifies code by hiding va_*
   boilerplate. */
#define auto_vsnprintf(buf, size, format) {         \
        va_list ap;                                 \
        va_start(ap, format);                       \
        vsnprintf(buf, size, format, ap);           \
        va_end(ap);                                 \
    }

/* Same as snprintf(), but returns buf instead of the number of bytes
   written. */
__attribute__ ((format(printf, 3, 4)))
static char *sprn(char *buf, int size, char const *format, ...) {
    auto_vsnprintf(buf, size, format);
    return buf;
}

/* Ovewrite dest with src in center mode. If the length of src is
   greater than the length of dest, overwrite dest from the start
   respecting its size.  */
static void overwrite_centered(char *dest, int size, char *src) {
    int dest_len = strlen(dest), src_len = strlen(src);
    if(dest_len > src_len) {
        memcpy(dest + (dest_len - src_len) / 2, src, src_len);
    } else {
        if(src_len >= size) src_len = size - 1;
        memcpy(dest, src, src_len);
        dest[src_len] = '\0';
    }
}

/* Returns the width of the terminal (number of columns) or -1 if f
   does not refer to a terminal. */
static int termwidth(FILE *f) {
    struct winsize w;
    return ioctl(fileno(f), TIOCGWINSZ, &w) ? -1 : w.ws_col;
}

/* Sets *t with the current monotonic time. */
static void gettime(struct timespec *t) {
    clock_gettime(CLOCK_MONOTONIC_RAW, t);
}

/* Return current time, in seconds, since *start (which should have been
   previously set by gettime()) */
static float timef(struct timespec const *start) {
    struct timespec t;
    gettime(&t);
    return (t.tv_sec - start->tv_sec) + 1E-9f * (t.tv_nsec - start->tv_nsec);
}

/* (*n)/d is assigned to *n and (*n)%d is returned */
static int divmod(int *n, int d) {
    int r = (*n) % d;
    *n /= d;
    return r;
}

/*************************************************** Utilities and wrappers ***/


/*** Structs ******************************************************************/

/* struct prgi_t is defined at prgi.h. See documentation there. */

/*
 * Initialize the configuration members here with default values. The
 * user can change these values. The other members are initialized in
 * prgi_init().
 *
 * As stderr is not a constant, we initialize prgi.output with
 * NULL. This will be changed at the first call of prgi_init() with
 * the default stream.
 */
struct prgi_t prgi = {
    .output = NULL,
    .update = .2,
    .lock_on_update = false,
};

/* Global internal state */
struct global_state {
    /* Starting time. timef() reports time relative to this time */
    struct timespec start;
    /* This mutex locks access to global and prgi. */
    pthread_mutex_t lock;
    /* Total (full) value for the global counter. */
    long int total;
    /* Global counter. When this value reaches global.total, the
       progress is 100% */
    long int count;
    /* Count and time of the last time prgi_update__() returned true. */
    long int last_count;
    float last_time;

    /* This is like prgi.width, but limited to MAXLINELEN (see
       prgi_update__().) */
    int width;

    /* The following variables are used by formatting/printing functions. */
    /* Number of lines printed by prgi_printf(). */
    int printed_lines;
    /* Number of pending expandable items to be printed. */
    int expand_count;
};

/* Initialize statically the mutex so that no
   pthread_mutex_init()/pthread_mutex_destroy() is needed. The other
   fields are initialized at prgi_init()/prgi_init_thread(). */
static struct global_state global = {
    .lock = PTHREAD_MUTEX_INITIALIZER,
};


/*
 * The struct prgi_thread_state_ is defined in prgi.h. Its fields are:
 *
 * - total and count: these have the same meaning of global.total and
 *   global.count, but threadwise. count is incremented in
 *   prgi_update() (see prgi.h)
 *
 * - last_count: value of count at the last time prgi_update__() was called.
 *
 * - mark: this is a mark to be reached by count while it is
 *   incremented in prgi_update(). When this happens, prgi_update__()
 *   is called.
 *
 * - last_time: last time thread the state was updated.
 */

/* This thread-local variable keeps the state of the thread. An
   abbreviation for code within this file is also provided. */
thread_local struct prgi_thread_state_ prgi_thread_;
#define thread prgi_thread_

/****************************************************************** Structs ***/


/*** Locking ******************************************************************/

/*
 * Theads must periodically read or write the shared variables prgi
 * and global. When this happens, these variables must be locked and
 * this is done through the mutex global.lock. The functions below
 * provide such functionality.

 * global.lock is statically initialized so
 * pthread_mutex_lock/pthread_mutex_unlock() never fail (see the
 * manpage of those functions). Therefore, the return status of these
 * functions is ignored.
 *
 * When the program is not linked with -lpthread (or -pthread), stub
 * pthread_mutex_lock/pthread_mutex_unlock() functions from libc are
 * used. These stub functions are no-ops. This is a consistent
 * behavior with a single-threaded program, to which no locking is
 * needed.
 */

static void prgi_lock(void) {
    pthread_mutex_lock(&global.lock);
}

/* Unlike prgi_lock(), prgi_unlock() may be called by the user, so it
   is not static. See prgi.h. */
void prgi_unlock(void) {
    pthread_mutex_unlock(&global.lock);
}

/****************************************************************** Locking ***/


/*** Basic output functions ***************************************************/

/* Returns true if is a terminal and has the minimum size to print the
   trucation indicator ">>>" */
static bool valid_terminal(void) {
    return prgi.output && prgi.width >= 3;
}

void prgi_clear(void) {
    if(!global.printed_lines) return; /* Nothing to do. */

    if(!valid_terminal()) {
        /* The only thing we can do is reset global.printed lines. */
        global.printed_lines = 0;
        return;
    }

    /* Return the carriage and clean the line where the cursor is */
    fputs("\r\e[K", prgi.output);
    --global.printed_lines;

    /* Go up all the previous newlines while clearing them */
    while(global.printed_lines) {
        fputs("\e[A\e[K", prgi.output);
        --global.printed_lines;
    }

    fflush(prgi.output);
}

/* Print s to prgi.output not exceeding the terminal width. If s was
   to exceed terminal width, print ">>>" at the end of the line
   instead of characters from s. If previous lines were printed, since
   the last update, print a newline before printing s. Increase the
   number of printed lines. */
void prgi_puts(char const *s) {
    if(!valid_terminal()) return;

    /* Separate this line from the previous one. */
    if(global.printed_lines) fputc('\n', prgi.output);

    if(esc_strlen(s) <= global.width) {
        fputs(s, prgi.output);
    } else {
        /* Truncate the line and append a truncation indicator
           >>>. Also, print a reset ANSI escape code in case some
           reset was lost in truncation. */
        fprintf(prgi.output, "%.*s>>>\e[0m",
                esc_rawlen(s, global.width - 3), s);
    }

    /* Park the cursor at the end of the line. */
    fprintf(prgi.output, "\e[%dG", prgi.width);
    fflush(prgi.output);
    ++global.printed_lines;
}

/*************************************************** Basic output functions ***/


/*** Initialization ***********************************************************/

/* Initializes thread data. */
void prgi_init_thread(long int total) {
    thread.total = total;
    thread.count = 0;
    thread.last_count = 0;
    thread.mark = 0; /* Display as soon as possible */
    thread.last_time = 0;

    /* Increments global.total with the total work of each thread. */
    prgi_lock();
    global.total += total;
    prgi_unlock();
}

/* Initializes global and prgi data. */
void prgi_init(long int total) {
    if(prgi.output == NULL) prgi.output = stdout; /* Default stream. */

    prgi.progress = 0;
    prgi.elapsed = 0;
    /* While no data is collected by prgi_update__(), these are undefined. */
    prgi.remaining = NAN;
    prgi.rate = NAN;
    prgi.mean_rate = NAN;
    prgi.width = -1;

    gettime(&global.start);
    global.total = 0; /* Will be incremented by each thread. */
    global.count = 0;
    global.last_count = 0;
    global.last_time = 0;
    global.width = -1;
    global.printed_lines = 0;
    global.expand_count = 0;

    /* Initialization the thread data for the "main"
       thread. global.total is also incremented here. */
    prgi_init_thread(total);
}

/*********************************************************** Initialization ***/


/*** prgi_update__() **********************************************************/

/*
 * General working:
 *
 * Each thread has its own thread.xxx variables. The global.xxx and
 * prgi.xxx variables are shared between all threads.
 *
 * prgi_update() increments thread.count each time it is called. If
 * thread.count does not exceed thread.mark, prgi_update() returns
 * false. Otherwise, prgi_update() calls and returns prgi_update__().
 *
 * At prgi_update__():
 *
 * - thread.mark is recalculated so that thread.count takes prgi.update
 *   seconds to exceed it at prgi_update()
 *
 * - global.count is incremented according to thread.count variation.
 *
 * - if enough time has passed since the last time, all prgi.xxx
 *   variables are updated, the lines previously printed are ereased,
 *   the cursor is positioned for the next print and true is
 *   returned. Otherwise, false is returned.
 */

/*
 * This function is called by prgi_update().
 * - Updates thread data.
 * - Updates global.count
 * - If enough time has passed, updates prgi data, prepares the
 *   terminal for printing and returns true. Otherwise, returns false.
 * - Access to global and prgi data is locked to other threads during
 *   function execution.
 */
bool prgi_update__(void) {
    long int thread_delta = thread.count - thread.last_count;
    float now = timef(&global.start);
    float global_dt;
    bool ready;

    prgi_lock();

    thread.mark += (prgi.update * thread_delta) / (now - thread.last_time);
    /* Make the total a mark, so that 100% will eventually be shown. */
    if(thread.count < thread.total && thread.mark > thread.total)
        thread.mark = thread.total;
    thread.last_count = thread.count;
    thread.last_time = now;

    global.count += thread_delta;
    global_dt = now - global.last_time;

    /* Due to thread.mark being an estimate, the time since last
       update may be different from prgi.update. Use a threshold to
       detect enough time to print. */
    ready = global_dt > 0.8 * prgi.update;

    /* All threads are competing to print. Allow a thread to print
       only if enough time has passed since the last print. Besides,
       print on first and last updates. */
    if(!(ready || global.count==thread_delta || thread.count==thread.total)) {
        prgi_unlock();
        return false;
    }

    prgi.progress = (float)global.count / global.total;
    prgi.elapsed = now;
    prgi.mean_rate = global.count / now;
    /* Update prgi.rate only is enough time has passed, as it is
       sensitive to poor statistics. */
    if(ready) prgi.rate = (global.count - global.last_count) / global_dt;
    prgi.remaining = (global.total - global.count) / prgi.rate;
    /* This is the true terminal width. */
    prgi.width = termwidth(prgi.output);
    /* This width is limited to the size of the buffers */
    global.width = prgi.width >= MAXLINELEN + 2 ? MAXLINELEN : (prgi.width - 2);

    global.last_count = global.count;
    global.last_time = now;
    global.expand_count = 0;

    prgi_clear();

    if(!prgi.lock_on_update) prgi_unlock();
    return true;
}

/********************************************************** prgi_update__() ***/


/*
 * Formatter functions: these functions provide formatted output for
 * the variables in the struct prgi.
 *
 * prgi member        Formatter function
 *
 * prgi.progress:     prgi_bar(), prgi_txt() and prgi_percent()
 * prgi.remaining:    prgi_remaining()
 * prgi.elapsed:      prgi_elapsed()
 * prgi.rate:         prgi_rate()
 * prgi.mean_rate:    prgi_mean_rate()
 *
 * All these functions write the formatted output in strings at static
 * buffers and return a pointer to that string.
 */


/*** prgi_bar(), prgi_bartxt() and prgi_percent() *****************************/

/* State variables shared by bar formatter functions. */
struct bar_state {
    /* Characters used to fill the bar. */
    char fill[2];
    /* Text inside the bar */
    char txt[BARBUFSIZE];
    /* True if the bar formatter function was called in expandable
       configuration (length zero) */
    bool expand;
};
struct bar_state bar;

/* Fill the first n chars of buf with bar.fill[0] and the following
   len - n char with bar.fill[1]. Then overwrite the center of this
   string with bar.txt. n and len must be <= MAXBARLEN. */
static void bar_fill(char *buf, int n, int len) {
    memset(buf, bar.fill[0], n);
    memset(buf + n, bar.fill[1], len - n);
    buf[len] = '\0';
    overwrite_centered(buf, BARBUFSIZE, bar.txt);
}

/* Draw a progress bar in the internal static buffer buf[] of length
   len. bar.fill and bar.txt must be already configured. */
static char *bar_doit(int len) {
    static char buf[BARBUFSIZE];
    int n;

    /* Return a zero-length string. This is a intermediary state
       for calculating the available space for an expandable bar. */
    if(len <= 0) {
        bar.expand = true;
        ++global.expand_count;
        return buf[0] = '\0', buf;
    }

    /* Minimum and maximum bar lengths */
    if(len < 10) {
        len = 10;
    } else if(len > MAXBARLEN) {
        len = MAXBARLEN;
    }

    /* Number of "filled" chars in the progress bar. Don't report more
       than 100%.  */
    n = myround(len * prgi.progress);
    if(n > len) n = len;

    /* It is guaranteed here that n <= len <= MAXBARLEN, so bar_fill()
       can be safely called below. */

    if(bar.fill[1] != '\0') {
        /* Pure ASCII bar using bar.fill[0] and bar.fill[1]. */
        bar_fill(buf, n, len);
    } else {
        /* Fill the whole bar with bar.fill[0] and use ANSI escape
           codes to reverse video of the first n chars. */
        static char aux[BARBUFSIZE];
        bar_fill(aux, len, len);
        /* Copy aux to buf with the n first chars inverted. */
        snprintf(buf, BARBUFSIZE, "\e[7m%.*s\e[0m%s", n, aux, aux + n);
    }

    return buf;
}

/* prgi_bar() and prgi_bartxt() configure bar.fill and bar.txt. */

char *prgi_bar(int len, char const *fill) {
    memcpy(bar.fill, fill, 2);
    bar.txt[0] = '\0';
    return bar_doit(len);
}

char *prgi_bartxt(int len, char const *fill, char const *format, ...) {
    memcpy(bar.fill, fill, 2);
    auto_vsnprintf(bar.txt, BARBUFSIZE, format);
    return bar_doit(len);
}

char *prgi_percent(void) {
    static char buf[5];
    return sprn(buf, 5, "%.0f%%", 100 * prgi.progress);
}

/***************************** prgi_bar(), prgi_bartxt() and prgi_percent() ***/


/*** prgi_remaining() and prgi_elapsed() **************************************/

/* Prints the time t (in seconds) in buf as an ISO 8601 time interval
   (ex: 31s, 5m42s, 7h36m etc) and returns buf. */
static char *timehms(char *buf, int size, float t) {
    int n, r;

    /* Show an invalid time (possibly NaN or Inf) as "?" */
    if(!isfinite(t) || t < 0) return sprn(buf, size, "?");

    /* If t > 7 days, show the number of seconds in scientific notation */
    if(t > 7 * 24 * 60 * 60) return sprn(buf, size, "%.2Es", t);

    n = t;

    /* Regular times */

    if(n < 60) return sprn(buf, size, "%ds", n);
    r = divmod(&n, 60);

    if(n < 60) return sprn(buf, size, "%dm%02ds", n, r);
    r = divmod(&n, 60);

    if(n < 24) return sprn(buf, size, "%dh%02dm", n, r);
    r = divmod(&n, 24);

    return sprn(buf, size, "%dd%d02h", n, r);
}

char *prgi_remaining(void) {
    static char buf[32];
    return timehms(buf, 32, prgi.remaining);
}

char *prgi_elapsed(void) {
    static char buf[32];
    return timehms(buf, 32, prgi.elapsed);
}

/************************************** prgi_remaining() and prgi_elapsed() ***/


/*** prgi_rate() and prgi_mean_rate() *****************************************/

/* For 1 <= x <= 999, returns the number of places after decimal point
   so that 3 significant digits will be show. Examples: 1.23, 12.3,
   123. */
static int dec3(double x) {
    return x < 10 ? 2 : x < 100;
}

/* Prints x on buf with SI prefixes instead of powers of 10. It works
   for x > 1. */
static char *sipref(char *buf, int size, double x) {
    static struct {
        double v;
        char *p;
    } pw[] = {
        {1E3,  "K"}, {1E6,  "M"}, {1E9,  "G"}, {1E12, "T"}, {1E15, "P"},
        {1E18, "E"}, {1E21, "Z"}, {1E24, "Y"}, {1E27, "R"}, {1E30, "Q"},
        {0, NULL}
    };

    int i;

    if(!isfinite(x) || x < 0) return sprn(buf, size, "?");

    /* When x < 1000, no prefix */
    if(x < 1000) return sprn(buf, size, "%.*f", dec3(x), x);

    for(i = 0; pw[i+1].p != NULL; ++i) {
        if(x < pw[i+1].v) {
            x /= pw[i].v;
            /* Print number with 3 significant digits. */
            return sprn(buf, size, "%.*f%s", dec3(x), x, pw[i].p);
        }
    }

    /* If x is too big, resort to powers of 10 */
    return sprn(buf, size, "%.2G", x);
}

char *prgi_rate(void) {
    static char buf[32];
    return sipref(buf, 32, prgi.rate);
}

char *prgi_mean_rate(void) {
    static char buf[32];
    return sipref(buf, 32, prgi.mean_rate);
}

/***************************************** prgi_rate() and prgi_mean_rate() ***/


/*** prgi_throbber() **********************************************************/

int prgi_throbber(char const *anim) {
    static int i = 0;
    int c;

    /* Don't display the throbber if 100% */
    if(global.count == global.total) return ' ';

    c = anim[i++];
    if(c == '\0') {
        c = anim[0];
        i = 1;
    }
    return c;
}

/********************************************************** prgi_throbber() ***/


/*** prgi_printf() ************************************************************/

/* If there are pending expandable items (currently, only a progress
   bar with automatic size), calculate the available space for each of
   these items and call the corresponding string-print
   functions. Then, print the line with prgi_fputs(). */
void prgi_printf(char const *format, ...) {
    static char line_buf[LINEBUFSIZE];

    if(!valid_terminal()) return;

    if(global.expand_count) {
        /* There are pending expandable items. Calculate available
           length within the terminal width and rerun the expandable
           item functions. */

        int avail_len, expand_len;

        auto_vsnprintf(line_buf, LINEBUFSIZE, format);

        avail_len = global.width - esc_strlen(line_buf);
        expand_len = avail_len > global.expand_count ?
            avail_len / global.expand_count : 1;

        if(bar.expand) {
            bar_doit(expand_len);
            bar.expand = false;
        }

        global.expand_count = 0;
    }

    auto_vsnprintf(line_buf, LINEBUFSIZE, format);

    prgi_puts(line_buf);
}

/************************************************************ prgi_printf() ***/
