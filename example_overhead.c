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
 * This example is a version of example_tutorial to measure prgi overhead.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "prgi.h"

void run_prgi(long int N) {
    double s, c;

    printf("Running with progress indicators:\n");

    prgi_init(N);

    s = 0;
    c = 0;
    for(long int n = 1; n <= N; n++) {
        double f = n, x = 1.0 / (f*f), y = x + c, t = s + y;
        c = y - (t - s);
        s = t;

        if(prgi_update(1)) {
            double pi = sqrt(6*s);

            prgi_printf("pi = %.14f [%s] %s %c Remaining: %s, Speed: %s terms/s",
                        pi, prgi_bar(0, "#."), prgi_percent(),
                        prgi_throbber("|/-\\"), prgi_remaining(), prgi_rate());
        }
    }

    /* Use the raw value prgi.elapsed to get more precision. */
    prgi_printf("Elapsed time: %fs, Mean speed: %s terms/s",
                prgi.elapsed, prgi_mean_rate());

    printf("\npi = %.14f\n\n", sqrt(6*s));
}

void run_plain(long int N) {
    double s, c;

    printf("Showing progress indicators only at the end, please wait.\n");

    prgi_init(N);

    s = 0;
    c = 0;
    for(long int n = 1; n <= N; n++) {
        double f = n, x = 1.0 / (f*f), y = x + c, t = s + y;
        c = y - (t - s);
        s = t;
    }

    /* Do only one update at task completion. This will make the
       elapsed time and the mean rate available. */
    prgi_update(N);

    /* Use the raw value prgi.elapsed to get more precision. */
    prgi_printf("Elapsed time: %fs, Mean speed: %s terms/s",
                prgi.elapsed, prgi_mean_rate());

    printf("\npi = %.14f\n\n", sqrt(6*s));
}


int main(int argc, char **argv) {
    long int N;
    float t_prgi, t_plain;

    N = 4000000000l;
    if(argc > 1) N = atol(argv[1]);
    printf("Summing %ld terms\n", N);
    printf("Run %s <Number of terms> to change the number of terms.\n\n",
           argv[0]);

    run_prgi(N);
    t_prgi = prgi.elapsed; /* Save the elapsed time. */

    run_plain(N);
    t_plain = prgi.elapsed;

    printf("PRGI overhead = %.0f%%\n", 100 * (t_prgi - t_plain) / t_plain);

    return 0;
}
