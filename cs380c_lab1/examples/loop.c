//test control flow hierarchy
#include <stdio.h>
#define WriteLine() printf("\n");
#define WriteLong(x) printf(" %lld", (long)x);
#define ReadLong(a) if (fscanf(stdin, "%lld", &a) != 1) a = 0;
#define long long long



void main()
{
  long a, b, c, d, e, f;
  long n, x;

  n = 13;
  x = 0;


  a = 0;
  while(a < n){
    b = 0;
    x = 0;
    while(b < n){
      c = 0;
      while(c <n ){
        d = 0;
        while(d < n){
          e = 0;
          while(e<n){
            f = 0;
            while(f < n){
              x=x+1;
              f= f +1;
            }
            e=e+1;

          }
          d = d +1;
        }

        c = c+1;
      }
      b=b+1;
    }
    a=a+1;
  }
  WriteLong(x);
  WriteLine();
}


/*
 expected output:
 371293
*/
