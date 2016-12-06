//test array and while sentence
#include <stdio.h>
#define WriteLine() printf("\n");
#define WriteLong(x) printf(" %lld", (long)x);
#define ReadLong(a) if (fscanf(stdin, "%lld", &a) != 1) a = 0;
#define long long long

const long n = 10;

void main() {

    long i;
    long j;
    long temp;
    long array[n];

    i = 0;
    while (i < n) {
        array[i] = (n - i - 1);
        i = i + 1;
    }

    i = 0;
    while (i < n) {
        WriteLong(array[i]);
        i = i + 1;
    }
    WriteLine();

    i = 0;
    while (i < n) {
        j = 0;
        while (j < i) {
            if (array[j] > array[i]) {
                temp = array[i]; 
                array[i] = array[j];
                array[j] = temp;
            }
            j = j + 1;
        }
        i = i + 1;
    }

    i = 0;
    while (i < n) {
        WriteLong(array[i]);
        i = i + 1;
    }
    WriteLine();
}

/*
 expected output:
*/
