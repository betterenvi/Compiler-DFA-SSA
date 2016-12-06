#ifndef _CSubScan_H_
#define _CSubScan_H_

enum {CSStimes, CSSdiv, CSSmod, CSSplus, CSSminus, CSSeql, CSSneq, CSSlss,
      CSSleq, CSSgtr, CSSgeq, CSSperiod, CSScomma, CSSrparen, CSSrbrak,
      CSSrbrace, CSSlparen, CSSlbrak, CSSlbrace, CSSbecomes, CSSnumber,
      CSSident, CSSsemicolon, CSSelse, CSSif, CSSwhile, CSSstruct, CSSconst,
      CSSvoid, CSSeof};

#define CSSidlen 16
typedef char CSSIdent[CSSidlen];
extern unsigned long long CSSval;
extern CSSIdent CSSid;

extern void CSSError(char *);
extern int CSSGet(void);
extern void CSSInit(char *);

#endif /* _CSubScan_H_ */
