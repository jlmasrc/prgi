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
 * This example program uses the series
 *
 * pi^2/6 = 1/1^2 + 1/2^2 + 1/3^2 + 1/4^2 + ...
 *
 * (also known as the Basel problem) to calculate pi. As the
 * convergence of this series is slow, requiring billions of terms to
 * produce an accurate value, and each term demands fairly low
 * processing (around 7 floating point operations with Kahan's
 * summation method), it is ideal for demonstration of prgi's low
 * overhead.
 *
 * - First, the original program without programs indicators is shown
 *   in main().
 *
 * - Then, the program is modified to use progress indicators.
 *
 * - Finally, the use of progress indicators is explained with
 *   comments.
 *
 * Due to the huge number of terms, Kahan's summation method is used
 * to avoid roundoff errors (see
 * https://en.wikipedia.org/wiki/Kahan_summation_algorithm)
 *
 * See prgi.h for general documentation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "prgi.h"


int main(int argc, char **argv) {
    long int N;
    double s, c;

    /*** Definition of the number of terms in the series ******************/
    /* Default value. */
    N = 4000000000l;
    /* Change the default value if an argument is given in the command line. */
    if(argc > 1) N = atol(argv[1]);
    printf("Summing %ld terms\n", N);
    printf("Run %s <Number of terms> to change the number of terms.\n\n",
           argv[0]);



    /*** Original program: no progress indicators *************************/

    printf("Running without progress indicators, please wait.\n");

    s = 0; /* Sum */
    c = 0; /* Kahan's compensator. */
    for(long int n = 1; n <= N; n++) {
        /* Use Kahan's summation method to sum 1/(n^2) to s. */
        double f = n, x = 1.0 / (f*f), y = x + c, t = s + y;
        c = y - (t - s);
        s = t;
    }

    /* pi = sqrt(6*s) */
    printf("pi = %.14f\n\n", sqrt(6*s));

    /*** Program modified to use prgi, no comments ************************/

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
                        prgi_throbber("|/-\\"),prgi_remaining(), prgi_rate());

        }
    }
    prgi_printf("Elapsed time: %s, Mean speed: %s terms/s",
                prgi_elapsed(), prgi_mean_rate());

    printf("\npi = %.14f\n\n", sqrt(6*s));



    /*** Same as above, with comments *************************************/

    printf("Running with progress indicators (again):\n");

    /* Some customizations, see prgi.h */
    /* Change prgi output to stdout (default is stderr) */
    /* prgi.output = stdout; */
    /* Print progress indicators each 0.1s (default is 0.2s) */
    /* prgi.update = 0.1;    */

    /* The progress indicators are initialized with the number of
       terms to be summed (this is the total work to be done). */
    prgi_init(N);

    s = 0;
    c = 0;
    /* Sum N terms of the series */
    for(long int n = 1; n <= N; n++) {
        /* Sum 1 term */
        double f = n, x = 1.0 / (f*f), y = x + c, t = s + y;
        c = y - (t - s);
        s = t;

        /* prgi_update(1) accounts for the sum of 1 term (work done
           equal to 1). If it returns true, then it is time to print
           the progress bar. */
        if(prgi_update(1)) {
            /* Any user code needed to print the progress indicators
               can be run here. This code will run only each 0.2
               seconds, so the overhead is very low. */
            double pi = sqrt(6*s);

            /* Print a progress line using prgi_printf() and formatter
               functions. See also prgi_demo.c of other progress line
               designs. prgi_printf() works together with
               prgi_update() and takes care of line erasing and cursor
               positioning in the terminal */
            prgi_printf("pi = %.14f [%s] %s %c Remaining: %s, Speed: %s terms/s",
                        pi, prgi_bar(0, "#."), prgi_percent(),
                        prgi_throbber("|/-\\"),prgi_remaining(), prgi_rate());
        }

        /* The overhead of calling prgi_update() should be negligible
           even with a work as lightweight as summing just 1 term as
           above. If you see an overhead, try compiling with
           -DPRGI_UPDATE_MACRO. */
    }

    /* Call prgi_clear() to erase the progress line which will be
       substituted by the final line below. */
    /* prgi_clear() */

    /* Print a final line. Printf could be used here with  */
    prgi_printf("Elapsed time: %s, Mean speed: %s terms/s",
                prgi_elapsed(), prgi_mean_rate());

    /* Prepend a newline after the last prgi_printf(). */
    printf("\npi = %.14f\n\n", sqrt(6*s));


    return 0;
}
