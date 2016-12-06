//test array and struct
#include <stdio.h>
#define WriteLine() printf("\n");
#define WriteLong(x) printf(" %lld", (long)x);
#define ReadLong(a) if (fscanf(stdin, "%lld", &a) != 1) a = 0;
#define long long long


struct A {
  long x, y;
   struct B {
     long q, r, s;
   } z;
} a, b[3];


void main()
{
  long i;
  struct A c;
  struct B qq;

  c.z.r = 987654321;

  a.x = 1;
  a.y = 2;

  c.x = 9;
  c.y = 0;

  b[0].x = 3;
  b[0].y = 4;

  b[a.x].x = 5;
  b[a.x].y = 6;

  b[b[a.x-1].x-1].x = 7;
  b[b[a.y-2].x-1].y = 8;

  WriteLong(a.x);
  WriteLong(a.y);
  WriteLine();

  i = 0;
  while (i < 3) {
    WriteLong(b[i].x);
    WriteLong(b[i].y);
    WriteLine();
    i = i + 1;
  }

  WriteLong(c.x);
  WriteLong(c.y);
  WriteLine();

  WriteLong(c.z.r);
  WriteLine();
}


/*
 expected output:
 1 2
 3 4
 5 6
 7 8
 9 0
 987654321
*/
