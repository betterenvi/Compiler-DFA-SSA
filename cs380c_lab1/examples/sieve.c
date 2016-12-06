//test the array
#include <stdio.h>
#define WriteLine() printf("\n");
#define WriteLong(x) printf(" %lld", (long)x);
#define ReadLong(a) if (fscanf(stdin, "%lld", &a) != 1) a = 0;
#define long long long

/*
 * Sieve of Eratosthenes
 * Method for finding out prime numbers.
 */

const long n = 1000;

void main() {
    long i;
    long j;
    long is_prime[n];

    /* Mark all numbers as prime, initially */
    is_prime[0] = 0;
    is_prime[1] = 0;
    i = 2;
    while (i < n) {
        is_prime[i] = 1;
        i = i + 1;
    }

    i = 2;
    while (i < n) {
        if (is_prime[i] != 0) {
            j = 2;
            while ((i * j) < n) {
                is_prime[i * j] = 0;
                j = j + 1;
            }
        }
        i = i + 1;
    }

    /* Write out all the prime numbers */
    i = 2;
    while (i < n) {
        if (is_prime[i] != 0) {
            WriteLong(i);
        }
        i = i + 1;
    }
    WriteLine();
}
