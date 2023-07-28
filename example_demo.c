/*
 * prgi: example program.
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

/*
 * Demo for various progress indicator styles and configurations.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "prgi.h"

int main(int argc, char **argv) {
    long int k;
    long int N;

    N = 5000000000l;
    if(argc > 1) N = atol(argv[1]);
    printf("Using N = %ld\n", N);
    printf("Run '%s <N>' to change N.\n", argv[0]);
    printf("Larger values increase completion times.\n\n");


    printf("Example 1: ASCII expandable progress bar.\n");
    prgi_init(N);
    for(k = 0; k < N; k++) {
        if(prgi_update(1)) {
            prgi_printf("%s %c [%s] Remaining: %s, Speed: %s counts/s",
                        prgi_percent(), prgi_throbber("|/-\\"),
                        prgi_bar(0, "#."), prgi_remaining(), prgi_rate());
        }
    }
    prgi_printf("Elapsed: %s, Mean speed: %s counts/s",
                 prgi_elapsed(), prgi_mean_rate());
    sleep(2);
    printf("\n\n");


    printf("Example 2: fixed length progress bar, different throbber.\n");
    prgi_init(N);
    for(k = 0; k < N; k++) {
        if(prgi_update(1)) {
            prgi_printf("%s %c [%s] Remaining: %s, Speed: %s counts/s",
                        prgi_percent(), prgi_throbber(".oOo"),
                        prgi_bar(20, "#."), prgi_remaining(), prgi_rate());
        }
    }
    prgi_printf("Elapsed: %s, Mean speed: %s counts/s",
                prgi_elapsed(), prgi_mean_rate());
    sleep(2);
    printf("\n\n");


    printf("Example 3: percentage progress and throbber embedded in the prgress bar.\n");
    prgi_init(N);
    for(k = 0; k < N; k++) {
        if(prgi_update(1)) {
            prgi_printf("[%s] Remaining: %s, Speed: %s counts/s",
                        prgi_bartxt(0, "#.", " %s %c ",
                                    prgi_percent(), prgi_throbber("|/-\\")),
                        prgi_remaining(), prgi_rate());
        }
    }
    prgi_printf("Elapsed: %s, Mean speed: %s counts/s",
                prgi_elapsed(), prgi_mean_rate());
    sleep(2);
    printf("\n\n");


    printf("Example 4: reverse video progress bar, different delimiter.\n");
    prgi_init(N);
    for(k = 0; k < N; k++) {
        if(prgi_update(1)) {
            prgi_printf("%s %c |%s| Remaining: %s, Speed: %s counts/s",
                        prgi_percent(), prgi_throbber("|/-\\"),
                        prgi_bar(0, " "), prgi_remaining(), prgi_rate());
        }
    }
    prgi_printf("Elapsed: %s, Mean speed: %s counts/s",
                prgi_elapsed(), prgi_mean_rate());
    sleep(2);
    printf("\n\n");


    printf("Example 5: different filling.\n");
    prgi_init(N);
    for(k = 0; k < N; k++) {
        if(prgi_update(1)) {
            prgi_printf("%s %c |%s| Remaining: %s, Speed: %s counts/s",
                        prgi_percent(), prgi_throbber("|/-\\"),
                        prgi_bar(0, ":"), prgi_remaining(), prgi_rate());
        }
    }
    prgi_printf("Elapsed: %s, Mean speed: %s counts/s",
                prgi_elapsed(), prgi_mean_rate());
    sleep(2);
    printf("\n\n");


    printf("Example 6: Percentage progress and throbber embedded in the progress bar.\n");
    prgi_init(N);
    for(k = 0; k < N; k++) {
        if(prgi_update(1)) {
            prgi_printf("|%s| Remaining: %s, Speed: %s counts/s",
                        prgi_bartxt(0, ":", " %s %c ",
                                    prgi_percent(), prgi_throbber("|/-\\")),
                        prgi_remaining(), prgi_rate());
        }
    }
    prgi_printf("Elapsed: %s, Mean speed: %s counts/s",
                prgi_elapsed(), prgi_mean_rate());
    sleep(2);
    printf("\n\n");


    printf("Example 7: Fixed length, no delimiter.\n");
    prgi_init(N);
    for(k = 0; k < N; k++) {
        if(prgi_update(1)) {
            prgi_printf("%s Remaining: %s, Speed: %s counts/s",
                        prgi_bartxt(30, ":", " %s %c ",
                                    prgi_percent(), prgi_throbber("|/-\\")),
                        prgi_remaining(), prgi_rate());
        }
    }
    prgi_printf("Elapsed: %s, Mean speed: %s counts/s",
                prgi_elapsed(), prgi_mean_rate());
    sleep(2);
    printf("\n\n");


    printf("Example 8: substitute the progress line after completion.\n");
    prgi_init(N);
    for(k = 0; k < N; k++) {
        if(prgi_update(1)) {
            prgi_printf("|%s| Remaining: %s, Speed: %s counts/s",
                        prgi_bartxt(0, ":", " %s %c ",
                                    prgi_percent(), prgi_throbber("|/-\\")),
                        prgi_remaining(), prgi_rate());
        }
    }
    prgi_clear();
    prgi_printf("All done. Elapsed: %s, Mean speed: %s counts/s",
                prgi_elapsed(), prgi_mean_rate());
    sleep(2);
    printf("\n\n");


    printf("Example 9: multiline progress indicators.\n");
    prgi_init(N);
    for(k = 0; k < N; k++) {
        if(prgi_update(1)) {
            prgi_printf("|%s|",
                        prgi_bartxt(0, ":", " %s %c ",
                                    prgi_percent(), prgi_throbber("|/-\\")));
            prgi_printf("Remaining: %s, Speed: %s counts/s",
                        prgi_remaining(), prgi_rate());
            prgi_printf("Value of the counter: %ld", k);
        }
    }
    prgi_printf("Elapsed: %s, Mean speed: %s counts/s",
                prgi_elapsed(), prgi_mean_rate());
    sleep(2);
    printf("\n\n");


    printf("Example 10: substitute all the lines after completion.\n");
    prgi_init(N);
    for(k = 0; k < N; k++) {
        if(prgi_update(1)) {
            prgi_printf("|%s|",
                        prgi_bartxt(0, ":", " %s %c ",
                                    prgi_percent(), prgi_throbber("|/-\\")));
            prgi_printf("Remaining: %s, Speed: %s counts/s",
                        prgi_remaining(), prgi_rate());
            prgi_printf("Value of the counter: %ld", k);
        }
    }
    prgi_clear();
    prgi_printf("Elapsed: %s, Mean speed: %s counts/s",
                prgi_elapsed(), prgi_mean_rate());
    sleep(2);
    printf("\n\n");

    return 0;
}
