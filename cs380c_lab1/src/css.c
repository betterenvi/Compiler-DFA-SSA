#include <stdlib.h>
#include <stdio.h>
#include <string.h>


enum {CSStimes, CSSdiv, CSSmod, CSSplus, CSSminus, CSSeql, CSSneq, CSSlss,
      CSSleq, CSSgtr, CSSgeq, CSSperiod, CSScomma, CSSrparen, CSSrbrak,
      CSSrbrace, CSSlparen, CSSlbrak, CSSlbrace, CSSbecomes, CSSnumber,
      CSSident, CSSsemicolon, CSSelse, CSSif, CSSwhile, CSSstruct, CSSconst,
      CSSvoid, CSSeof};

#define CSSidlen 16
typedef char CSSIdent[CSSidlen];
unsigned long long CSSval;
CSSIdent CSSid;


static int ch;
static int line;
static FILE *f;


void CSSError(char *msg)
{
  printf(" line %d error %s\n", line, msg);
  exit(-1);
}


static void Identifier(void)
{
  register int i;

  i = 0;
  while ((('a' <= ch) && (ch <= 'z')) || (('A' <= ch) && (ch <= 'Z')) || (('0' <= ch) && (ch <= '9')) || (ch == '_')) {
    if (i < CSSidlen) CSSid[i] = ch;
    i++;
    ch = getc(f);
  }
  if (i >= CSSidlen) CSSError("identifier too long");
  CSSid[i] = 0;
}


static void Number(void)
{
  CSSval = 0;
  while (('0' <= ch) && (ch <= '9') && ((0x8000000000000000ULL + '0' - ch) / 10 >= CSSval)) {
    CSSval = CSSval * 10 + ch - '0';
    ch = getc(f);
  }
  if (('0' <= ch) && (ch <= '9')) CSSError("number too large");
}


static void Comment(void)
{
  ch = getc(f);
  do {
    while ((ch != EOF) && (ch != '*')) {
      if (ch == '\n') line++;
      ch = getc(f);
    }
    ch = getc(f);
  } while ((ch != EOF) && (ch != '/'));
  ch = getc(f);
}


static void CommentLine(void)
{
  do {
    ch = getc(f);
  } while ((ch != EOF) && (ch != '\n'));
}


int CSSGet(void)
{
  register int sym;

  while ((ch != EOF) && (ch <= ' ')) {
    if (ch == '\n') line++;
    ch = getc(f);
  }
  switch (ch) {
    case EOF: sym = CSSeof; break;
    case '+': sym = CSSplus; ch = getc(f); break;
    case '-': sym = CSSminus; ch = getc(f); break;
    case '*': sym = CSStimes; ch = getc(f); break;
    case '%': sym = CSSmod; ch = getc(f); break;
    case '/':
      sym = CSSdiv;
      ch = getc(f);
      if (ch == '/') {
        CommentLine();
        sym = CSSGet();
      } else if (ch == '*') {
        Comment();
        sym = CSSGet();
      }
      break;
    case '=':
      sym = CSSbecomes;
      ch = getc(f);
      if (ch == '=') {
        sym = CSSeql;
        ch = getc(f);
      }
      break;
    case '#': CommentLine(); sym = CSSGet(); break;
    case '.': sym = CSSperiod; ch = getc(f); break;
    case ',': sym = CSScomma; ch = getc(f); break;
    case ';': sym = CSSsemicolon; ch = getc(f); break;
    case '!':
      ch = getc(f);
      if (ch != '=') CSSError("illegal symbol encountered");
      sym = CSSneq;
      ch = getc(f);
      break;
    case '(': sym = CSSlparen; ch = getc(f); break;
    case '[': sym = CSSlbrak; ch = getc(f); break;
    case '{': sym = CSSlbrace; ch = getc(f); break;
    case ')': sym = CSSrparen; ch = getc(f); break;
    case ']': sym = CSSrbrak; ch = getc(f); break;
    case '}': sym = CSSrbrace; ch = getc(f); break;
    case '<':
      sym = CSSlss;
      ch = getc(f);
      if (ch == '=') {
        sym = CSSleq;
        ch = getc(f);
      }
      break;
    case '>':
      sym = CSSgtr;
      ch = getc(f);
      if (ch == '=') {
        sym = CSSgeq;
        ch = getc(f);
      }
      break;

    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      sym = CSSnumber;
      Number();
      break;

    case 'c': sym = CSSident; Identifier(); if (strcmp(CSSid, "const") == 0) sym = CSSconst; break;
    case 'e': sym = CSSident; Identifier(); if (strcmp(CSSid, "else") == 0) sym = CSSelse; break;
    case 'i': sym = CSSident; Identifier(); if (strcmp(CSSid, "if") == 0) sym = CSSif; break;
    case 's': sym = CSSident; Identifier(); if (strcmp(CSSid, "struct") == 0) sym = CSSstruct; break;
    case 'v': sym = CSSident; Identifier(); if (strcmp(CSSid, "void") == 0) sym = CSSvoid; break;
    case 'w': sym = CSSident; Identifier(); if (strcmp(CSSid, "while") == 0) sym = CSSwhile; break;

    case 'a': case 'b': case 'd': case 'f': case 'g': case 'h': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 't': case 'u': case 'x': case 'y': case 'z': case '_':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z':
      sym = CSSident;
      Identifier();
      break;
    default: CSSError("illegal symbol encountered");
  }
  return sym;
}


void CSSInit(char *filename)
{
  line = 0;
  f = fopen(filename, "r+t");
  if (f == NULL) CSSError("could not open file");
  line = 1;
  ch = getc(f);
}
