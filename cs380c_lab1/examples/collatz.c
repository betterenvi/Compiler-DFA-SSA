//test control flow sentence
#include <stdio.h>
#define WriteLine() printf("\n");
#define WriteLong(x) printf(" %lld", (long)x);
#define ReadLong(a) if (fscanf(stdin, "%lld", &a) != 1) a = 0;
#define long long long


const long n = 270;

long i, j, maxi;
long k, max;


void main()
{
  max = 0;
  i = 5;
  while (max < n) {
    k = 0;
    j = i;
    while (j != 4) {
      if ((j % 2) == 1) {
        j = (j+j+j+1) / 2;
        k = k + 2;
      } else {
        j = j / 2;
        k = k + 1;
      }
    }
    if (k > max) {
      max = k;
      maxi = i;
      WriteLong(max+2);
      WriteLong(maxi);
      WriteLine();
    }
    i = i + 1;
  }
  WriteLong(max+2);
  WriteLong(maxi);
  WriteLine();
}


/*
 expected output:
 5 5
 8 6
 16 7
 19 9
 20 18
 23 25
 111 27
 112 54
 115 73
 118 97
 121 129
 124 171
 127 231
 130 313
 143 327
 144 649
 170 703
 178 871
 181 1161
 182 2223
 208 2463
 216 2919
 237 3711
 261 6171
 267 10971
 275 13255
 275 13255
*/
