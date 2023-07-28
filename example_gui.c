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
 * In this example, we use prgi_update() with the Zenity dialog
 * instead of prgi_printf() to show progress indicators.
 *
 * As with prgi_printf(), prgi_update() allows sending messages to
 * Zenity only each 0.2s, thus enabling almost zero-overhead progress
 * indication.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "prgi.h"

int main(int argc, char **argv) {
    long int N;
    double s, c;
    FILE *gui;

    N = 4000000000l;
    if(argc > 1) N = atol(argv[1]);
    printf("Summing %ld terms\n", N);
    printf("Run %s <Number of terms> to change the number of terms.\n\n",
           argv[0]);

    printf("GUI progress bar using zenity.\n");

    /* Run Zenity. The stream gui, connected to zenity's stdin, will
       be used to send messages to it. */
    gui = popen("zenity --title='example_gui' --width=500 --progress", "w");
    if(!gui) {
        printf("Could not run zenity.");
        exit(1);
    }

    prgi_init(N);

    s = 0;
    c = 0;
    for(long int n = 1; n <= N; n++) {
        double f = n, x = 1.0 / (f*f), y = x + c, t = s + y;
        c = y - (t - s);
        s = t;

        if(prgi_update(1)) {
            /* Zenity needs a numeric value for the progress bar, so
               we use the raw value prgi.progress. */
            fprintf(gui, "%.0f\n", 100 * prgi.progress);
            /* The formating functions can be used normally with
               printf() to send messages to Zenity. */
            fprintf(gui, "# Remaining: %s,  Speed: %s terms/s,  pi = %.14f\n",
                    prgi_remaining(), prgi_rate(), sqrt(6*s));
            fflush(gui);
        }
    }

    fprintf(gui, "# Done.  Elapsed: %s,  Mean speed: %s terms/s,  pi = %.14f\n",
            prgi_elapsed(), prgi_mean_rate(), sqrt(6*s));
    fflush(gui);

    pclose(gui);

    return 0;
}
