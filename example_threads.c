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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>

#include "prgi.h"

/*
 * Multi-threaded version of example_tutorial.c.
 */


/* Global variables used by threads. */
/* Number of terms of the sum */
long int N;
/* Results of sums in each thread. */
double s1, s2, s3, s4;


/* This functions sums the terms for n0 <= n <= n1. It will be called
   by thread functions f1(), f2(), f3() and f4() below. */
double sum(long int n0, long int n1) {
    double s, c;

    /* Thread-wise initialization with the number of terms the thread
       will sum. */
    prgi_init_thread(n1 - n0 + 1);

    s = 0;
    c = 0;
    for(long int n = n0; n <= n1; n++) {
        double f = n, x = 1.0 / (f*f), y = x + c, t = s + y;
        c = y - (t - s);
        s = t;

        /* Account for 1 summed term in the thread */
        if(prgi_update(1)) {
            prgi_printf("%s %c [%s] Remaining: %s, Speed: %s terms/s)",
                        prgi_percent(), prgi_throbber("|/-\\"),
                        prgi_bar(0, "#."), prgi_remaining(), prgi_rate());
        }
    }

    return s;
}

void *f1(void *data) {
    s1 = sum(1, N/4);
    return NULL;
}

void *f2(void *data) {
    s2 = sum(N/4 + 1, 2*(N/4));
    return NULL;
}

void *f3(void *data) {
    s3 = sum(2*(N/4)+ 1, 3*(N/4));
    return NULL;
}

void *f4(void *data) {
    s4 = sum(3*(N/4) + 1, N);
    return NULL;
}

int main(int argc, char **argv) {
    pthread_t t1, t2, t3, t4;
    double s;

    N = 4000000000l;
    if(argc > 1) N = atol(argv[1]);
    printf("Summing %ld terms\n", N);
    printf("Run %s <Number of terms> to change the number of terms.\n\n",
           argv[0]);

    printf("Multi-threaded run (4 threads)\n");

    /* The main thread will not sum terms, so call prgi_init() with
       argument 0. */
    prgi_init(0);

    /* Spawn all threads. */
    if(pthread_create(&t1, NULL, f1, NULL) ||
       pthread_create(&t2, NULL, f2, NULL) ||
       pthread_create(&t3, NULL, f3, NULL) ||
       pthread_create(&t4, NULL, f4, NULL)    ) {
        printf("Thread creation failed\n");
        exit(1);
    }

    /* Wait for the completion of all threads */
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    pthread_join(t4, NULL);

    /* The final line is printed in the main thread after all threads
       finisehd */
    prgi_printf("Elapsed: %s, Mean speed: %s terms/s",
                prgi_elapsed(), prgi_mean_rate());

    /* Collect the results */
    s = s1 + s2 + s3 + s4;
    printf("\npi = %.14f\n\n", sqrt(6*s));

    return 0;
}
