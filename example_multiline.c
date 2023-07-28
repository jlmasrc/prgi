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
 * This example is a version of example_tutorial.c with multiline
 * progress report.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "prgi.h"

int main(int argc, char **argv) {
    long int N;
    double s, c;

    N = 4000000000l;
    if(argc > 1) N = atol(argv[1]);
    printf("Summing %ld terms\n", N);
    printf("Run %s <Number of terms> to change the number of terms.\n\n",
           argv[0]);


    prgi_init(N);

    s = 0;
    c = 0;
    for(long int n = 1; n <= N; n++) {
        double f = n, x = 1.0 / (f*f), y = x + c, t = s + y;
        c = y - (t - s);
        s = t;

        if(prgi_update(1)) {
            double pi = sqrt(6*s);

            prgi_printf("s = %.14f, pi = %.14f", s, pi);
            prgi_printf("[%s] %s %c", prgi_bar(0, "#."),
                        prgi_percent(), prgi_throbber("|/-\\"));
            prgi_printf("Remaining: %s, Speed: %s terms/s",
                        prgi_remaining(), prgi_rate());
        }
    }
    prgi_printf("Elapsed time: %s, Mean speed: %s terms/s",
                prgi_elapsed(), prgi_mean_rate());

    printf("\ns = %.14f, pi = %.14f\n\n", s, sqrt(6*s));

    return 0;
}
