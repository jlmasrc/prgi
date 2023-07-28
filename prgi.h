/*
 * prgi: header file and library documentation.
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


#ifndef PRGI_H
#define PRGI_H

#include <threads.h>
#include <stdbool.h>


/*** Private symbols **********************************************************/

/* Don't use them. They are visible because the inline function
   prgi_update() needs them. See documentation in prgi.c. */

struct prgi_thread_state_ {
    long int total, count, last_count, mark;
    float last_time;
};
extern thread_local struct prgi_thread_state_ prgi_thread_;

bool prgi_update__(void);

/********************************************************** Private symbols ***/

/* prgi_t: struct for the prgi global variable.
 */
struct prgi_t {
    /* Configuration fiels. */
    FILE *output;
    float update;
    bool lock_on_update;

    /* Status fields. */
    int width;
    float progress;
    float elapsed;
    float remaining;
    float rate;
    float mean_rate;
};
extern struct prgi_t prgi;

/*
 * Configuration fields. If needed, change these variables before
 * calling prgi_init().
 *
 * prgi.output: Stream prgi_printf() and prgi_puts() will print to.
 * Default value is stderr.
 *
 * prgi.update: How frequently (in seconds) prgi_update() will update
 * status and return true.
 * Default value is 0.2.
 *
 * prgi.lock_on_update: If set as true, this option will lock access
 * to prgi by other threads and prgi_unlock() will have to be called
 * after prgi_update() returns true and all printing is done. This is
 * only necessary in multithreaded programs if printing of progress
 * may be slow. It should never be necessary.  Default value: false
 */


/* Initializes the status variables in the main thread. Takes as
 * argument the total work to be done by the main thread (usually zero
 * in a multi-threaded program).
 */
void prgi_init(long int total);


/* Initializes the per-thread status variables in the worker
 * threads. Takes as argument the total work to be done by the thread.
 * prgi_init() must be called in the main thread before the worker
 * threads are spawned.
 */
void prgi_init_thread(long int total);


/* prgi_update() increments the internal work counter by 'inc'. This
 * increment is the amount of work done since the last time
 * prgi_update() was called.
 *
 * Returns true each prgi.update seconds (approximately). When it
 * returns true, the status fields in the struct prgi are ready to be
 * directly read or printed in convenient form by the formatters.
 *
 * This is a very lightweith function that can be called billions of
 * times per second without causing significant overhead to the
 * program's task. It will return true only each prgi.update seconds,
 * so the code executed when this happens will also not be significant
 * should prgi.update be sufficiently large (the default value meet
 * this requirement for ordinary progress display).
 *
 * Normally, prgi_update() is an inline function. If PRGI_UPDATE_MACRO
 * is defined, it will be a preprocessor macro.
 */
#ifndef PRGI_UPDATE_MACRO

static inline __attribute__ ((always_inline)) bool prgi_update(long int inc) {
    return (prgi_thread_.count += inc) >= prgi_thread_.mark && prgi_update__();
}

#else

#define prgi_update(inc)                                                \
    ((prgi_thread_.count += (inc)) >= prgi_thread_.mark && prgi_update__())

#endif


/*
 * Formatting functions.
 *
 * Use the following formatting functions to get the various progress
 * indicators as strings in human readable form after prgi_update()
 * returns true (prgi_update() updates the status variables). Optionally,
 * the raw values of the indicators can also be used.
 *
 * All these functions write the formatted output in strings at static
 * buffers and return a pointer to that string.
 */

/* Returns a string with the percentage progress with '%' sign.
 * Raw value: prgi.progress (taking values from 0 (0%) to 1 (100%).
 */
char *prgi_percent(void);

/* Returns a string with the estimated remaining time im the format HMS.
   Raw value: prgi.remaining (taking values in seconds).
*/
char *prgi_remaining(void);

/* Returns a string with the elapsed time im the format HMS.
   Raw value: prgi.elapsed (taking values in seconds).
*/
char *prgi_elapsed(void);

/* Returns a string with the instantaneous work rate (how much work
 * per second) with SI suffixes (K, M, G, T etc). This value can be
 * used as a speed or performance indicator.
 * Raw value: prgi.rate
 */
char *prgi_rate(void);

/* Returns a string with the mean work rate (how much work per second)
 * with SI suffixes (K, M, G, T etc). This value can be used as a mean
 * speed or performance indicator, mainly after task completion.
 * Raw value: prgi.mean_rate
 */
char *prgi_mean_rate(void);

/* Returns a string with a progress bar with length len. If len = 0,
 * then the length is automatic, taking all the available space in the
 * terminal width.
 *
 * The string fill must have 1 or 2 chars. If it has two chars, then
 * the first char will be used to fill the completed part of the
 * progress bar and the second will be used to fill the incompleted
 * part. Example: if fill is "#.", then the bar will look like
 * ###############.................
 *
 * If fill has only one char (for example, " "), then the whole
 * progress bar will be filled with it and the completed part will be
 * printed in reverse video Using ANSI escape sequences. This is the
 * preferred way for prgi_bartxt() below.
 *
 * Raw value: prgi.progress.
 */
char *prgi_bar(int len, char const *fill);

/* This function extends prgi_bartxt() by placing a centered text
 * inside the progress bar. It has a printf-like syntax.
 * Example: show the percentage progress inside the progress bar with
 * progress in reverse video
 * prgi_bartxt(0, " ", "%s", prgi_percent())
 */
__attribute__ ((format(printf, 3, 4)))
char *prgi_bartxt(int len, char const *fill, char const *format, ...);

/*
 * Sequentially returns characters form anim each time it is
 * called. It is used to display "throbbers" that show the work is
 * being done. This function does not translate any prgi
 * field. Examples of anim strings:
 *
 * - Spinning wheel: "|/-\\"
 * - Throbber: ".oOo"
 * - Exploding ballon: ".oO*"
 * - dqpb: "dqpb"
 * - Flip: "__-`'-__"
 */
int prgi_throbber(char const *anim);


/*
 * Prints a progress indicator line with a printf-like syntax. This
 * function should be called when prgi_update() returns true. It
 * automatically takes care, in conjunction with prgi_update(), of
 * line counting, clipping if necessary, and line erasing. It also
 * allows automatic progress bar sizes.
 *
 * Multiple calls to prgi_printf() are allowed to produce multiple
 * line progress indicators, one for each line. No newlines are
 * necessary, as each extra call to prgi_printf() will print a newline
 * to separate the line from the previous one.
 *
 * Example usage:
 * if(prgi_update(1)) {
 *     prgi_printf("%s [%s] Remaining time: %s", prgi_percent(),
 *                 prgi_bar(0, "#."), prgi_remaining());
 * }
 */
__attribute__ ((format(printf, 1, 2)))
void prgi_printf(char const *format, ...);

/* Clears all lines previously printed by prgi_printf() or
   prgi_puts(). It is not needed after prgi_update(), as it also
   clears the lines before new lines are printed. It is used to print
   a last line after task completion that substitutes the 100%
   progress indicators, usually containing only elapsed time and mean
   rate. */
void prgi_clear(void);

/*
 * This is a lower level print function that does not allow formatting
 * like prgi_printf(). Printed lines are automatically separated by
 * newlines and lines too big are truncated and appended with ">>>".
 */
void prgi_puts(char const *s);


/* prgi.width reports the terminal width. It is updated when
 * prgi_update() returns true. This can be useful for custom printing
 * functions.
 */


/* Unlocks access to prgi by other threads. This is used only in
 * multithreaded programs if the option prgi.lock_on_update is set as
 * true.
 */
void prgi_unlock(void);

#endif /* PRGI_H */
