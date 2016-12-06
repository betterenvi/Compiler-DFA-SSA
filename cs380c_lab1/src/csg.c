#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "css.h"
#include "csg.h"

CSGType CSGlongType, CSGboolType;
char CSGcurlev;
CSGNode CSGpc;

enum {ineg, iadd, isub, imul, idiv, imod, iparam, ienter, ileave, iend, iload,
      istore, imove, icmpeq, icmplt, icmple, iblbs, iblbc, icall, ibr, iret,
      iread, iwrite, iwrl, inop, ientrypc};

static CSGNode code, entrypc, FP, GP;


/*****************************************************************************/


// This function adds a new instruction at the end of the instruction list
// and sets the operation to op and the operands to x and y.
static CSGNode PutOpNodeNode(int op, CSGNode x, CSGNode y)
{
  CSGNode i;

  CSGpc->nxt = malloc(sizeof(CSGNodeDesc));
  assert(CSGpc->nxt != NULL);
  i = CSGpc;
  CSGpc = CSGpc->nxt;
  CSGpc->class = CSGInst;
  CSGpc->op = inop;
  CSGpc->prv = i;
  CSGpc->nxt = NULL;

  assert(i != NULL);
  i->class = CSGInst;
  i->op = op;
  i->x = x;
  i->y = y;
  i->type = CSGlongType;
  i->lev = 0;

  return i;
}


static CSGNode PutOpNode(int op, CSGNode x)
{
  return PutOpNodeNode(op, x, NULL);
}


static CSGNode PutOp(int op)
{
  return PutOpNodeNode(op, NULL, NULL);
}


// This function SETS the member fields of a CSGNode object.
void CSGMakeConstNodeDesc(CSGNode *x, CSGType typ, long long val)
{
  (*x)->class = CSGConst;
  (*x)->type = typ;
  (*x)->val = val;
  (*x)->lev = CSGcurlev;
}


// This function make a COPY of a CSGNode object
void CSGMakeNodeDesc(CSGNode *x, CSGNode y)
{
  register int i;

  (*x)->class = y->class;
  (*x)->type = y->type;
  (*x)->val = y->val;
  (*x)->lev = y->lev;
  (*x)->true = y->true;
  for (i = 0; i < CSSidlen; i++) (*x)->name[i] = y->name[i];

  // Make the 'original' field of the copy to point back to original Node object
  if (y->original == NULL) {
    (*x)->original = y;
  } else {
    (*x)->original = y->original;
  }
}


/*****************************************************************************/


static void Load(CSGNode *x)
{
  if ((*x)->class == CSGAddr) {
    *x = PutOpNode(iload, *x);
  } else if (((*x)->class == CSGVar) && ((*x)->lev == 0)) {
    *x = PutOpNodeNode(iadd, *x, GP);
    *x = PutOpNode(iload, *x);
  }
}


void CSGOp2(int op, CSGNode *x, CSGNode y);


void CSGField(CSGNode *x, CSGNode y)  /* x = x.y */
{
  if ((*x)->class == CSGVar) {
    if ((*x)->lev == 0) *x = PutOpNodeNode(iadd, *x, GP); else *x = PutOpNodeNode(iadd, *x, FP);
  }
  *x = PutOpNodeNode(iadd, *x, y);
  (*x)->type = y->type;
  (*x)->class = CSGAddr;
}


void CSGIndex(CSGNode *x, CSGNode y)  /* x = x[y] */
{
  CSGNode z;

  if (y->class == CSGConst) {
    if ((y->val < 0) || ((*x)->type->len <= y->val)) CSSError("index out of bounds");
  }
  z = malloc(sizeof(CSGNodeDesc));
  assert(z != NULL);

  CSGMakeConstNodeDesc(&z, CSGlongType, (*x)->type->base->size);
  CSGOp2(CSStimes, &y, z);
  z = *x;

  if ((*x)->class != CSGAddr) {
    if ((*x)->class != CSGInst) {
      if ((*x)->lev > 0) {
        *x = PutOpNodeNode(iadd, *x, FP);
      } else {
        *x = PutOpNodeNode(iadd, *x, GP);
      }
    }
  }
  *x = PutOpNodeNode(iadd, *x, y);
  (*x)->type = z->type->base;
  (*x)->class = CSGAddr;
}


/*****************************************************************************/


// The folloing five functions deal with control transfer.  Mostly, they
// remember where the control transferring instructions are so that their
// targets can be set later if the targets have not yet been compiled.


void CSGInitLabel(CSGNode *lbl)
{
  *lbl = NULL;
}


void CSGSetLabel(CSGNode *lbl)
{
  *lbl = CSGpc;
}


// This function sets the target of a forward jump, call, or branch once
// the target is being compiled.
void CSGFixLink(CSGNode lbl)
{
  if (lbl != NULL) {
    if ((lbl->op == icall) || (lbl->op == ibr)) {
      lbl->x = CSGpc;
    } else {
      lbl->y = CSGpc;
    }
  }
}


void CSGBJump(CSGNode lbl)
{
  PutOpNode(ibr, lbl);
}


void CSGFJump(CSGNode *lbl)
{
  PutOpNode(ibr, *lbl);
  *lbl = CSGpc->prv;
}


/*****************************************************************************/


static void TestInt(CSGNode x)
{
  if (x->type->form != CSGInteger) CSSError("type integer expected");
}


void CSGTestBool(CSGNode *x)
{
  if ((*x)->type->form != CSGBoolean) CSSError("type boolean expected");
  Load(x);
}


/*****************************************************************************/


void CSGOp1(int op, CSGNode *x)  /* x = op x */
{
  Load(x);
  if (op == CSSplus) {
    TestInt(*x);
  } else if (op == CSSminus) {
    TestInt(*x);
    *x = PutOpNode(ineg, *x);
  }
}


void CSGOp2(int op, CSGNode *x, CSGNode y)  /* x = x op y */
{
  assert(x != NULL);
  assert(*x != NULL);
  assert(y != NULL);
  if ((*x)->type != y->type) CSSError("incompatible types");
  Load(x);
  Load(&y);
  switch (op) {
    case CSSplus: *x = PutOpNodeNode(iadd, *x, y); break;
    case CSSminus: *x = PutOpNodeNode(isub, *x, y); break;
    case CSStimes: *x = PutOpNodeNode(imul, *x, y); break;
    case CSSdiv: *x = PutOpNodeNode(idiv, *x, y); break;
    case CSSmod: *x = PutOpNodeNode(imod, *x, y); break;
  }
}


void CSGRelation(int op, CSGNode *x, CSGNode y)
{
  CSGNode t;

  TestInt(*x);
  TestInt(y);
  Load(x);
  Load(&y);
  switch (op) {
    case CSSeql: t = PutOpNodeNode(icmpeq, *x, y); *x = PutOpNode(iblbc, t); break;
    case CSSneq: t = PutOpNodeNode(icmpeq, *x, y); *x = PutOpNode(iblbs, t); break;
    case CSSlss: t = PutOpNodeNode(icmplt, *x, y); *x = PutOpNode(iblbc, t); break;
    case CSSgtr: t = PutOpNodeNode(icmple, *x, y); *x = PutOpNode(iblbs, t); break;
    case CSSleq: t = PutOpNodeNode(icmple, *x, y); *x = PutOpNode(iblbc, t); break;
    case CSSgeq: t = PutOpNodeNode(icmplt, *x, y); *x = PutOpNode(iblbs, t); break;
  }
  (*x)->type = CSGboolType;
  (*x)->false = NULL;
  (*x)->true = CSGpc->prv;
}


/*****************************************************************************/


void CSGStore(CSGNode x, CSGNode y)  /* x = y */
{
  assert(x != NULL);
  assert(y != NULL);
  Load(&y);
  if (x->type->form != y->type->form) CSSError("incompatible assignment");
  if (x->type->form != CSGInteger) CSSError("only basic type assignments supported");
  if ((x->class == CSGInst) || (x->class == CSGAddr)) {
    PutOpNodeNode(istore, y, x);
  } else if ((x->class == CSGVar) && (x->lev == 0)) {
    x = PutOpNodeNode(iadd, x, GP);
    PutOpNodeNode(istore, y, x);
  } else {
    PutOpNodeNode(imove, y, x);
  }
}


void CSGAdjustLevel(int n)
{
  CSGcurlev += n;
}


void CSGParameter(CSGNode *x, CSGType ftyp, signed char class)
{
  if (ftyp != CSGlongType) CSSError("integer type expected");
  Load(x);
  *x = PutOpNode(iparam, *x);
}


/*****************************************************************************/


void CSGCall(CSGNode x)
{
  PutOpNode(icall, x);
}


void CSGIOCall(CSGNode x, CSGNode y)
{
  CSGNode z;

  if (x->val < 3) TestInt(y);
  if (x->val == 1) {
    z = PutOp(iread);
    CSGStore(y, z);
  } else if (x->val == 2) {
    Load(&y);
    PutOpNode(iwrite, y);
  } else {
    PutOp(iwrl);
  }
}


void CSGEntryPoint(void)
{
  if (entrypc != NULL) CSSError("multiple program entry points");
  entrypc = CSGpc;
  PutOp(ientrypc);
}


void CSGEnter(int size)
{
  /* size: The size of local variables */
  CSGNode x = malloc(sizeof(CSGNodeDesc));
  assert(x != NULL);
  CSGMakeConstNodeDesc(&x, CSGlongType, size);
  PutOpNode(ienter, x);
}


void CSGReturn(int size)
{
  /* The size of formal parameters, shows how much to unwind the stack */
  // PutOp(ileave); Not using ileave in our implementation
  CSGNode x = malloc(sizeof(CSGNodeDesc));
  assert(x != NULL);
  CSGMakeConstNodeDesc(&x, CSGlongType, size);
  PutOpNode(iret, x);
}


void CSGStart(int size)
{
  // this function may be used later
}


void CSGOpen(void)
{
  CSGcurlev = 0;
  CSGpc = malloc(sizeof(CSGNodeDesc));
  assert(CSGpc != NULL);
  CSGpc->class = CSGInst;
  CSGpc->op = inop;
  CSGpc->prv = code;
  CSGpc->nxt = NULL;
  code->nxt = CSGpc;
}


void CSGClose(void)
{
  /* This function is used specifically to end the main function in
   * Martin's version. We treat main() as just another function. So,
   * control should not reach here. */
  assert(0);
  PutOp(ileave);
  PutOp(iend);
}


static void PrintBrakNode(CSGNode x)
{
  assert(x != NULL);
  if (x->class != CSGInst) {
    printf("unknown brak class");
  } else {
    printf(" [%d]", x->line);
  }
}


// This function Prints the CSGNode information based on the Node type
static void PrintNode(CSGNode x)
{
  assert(x != NULL);
  if (x == GP) {
    printf(" GP");
  } else if (x == FP) {
    printf(" FP");
  } else {
    switch (x->class) {
      case CSGVar:
          if ((x->type == CSGlongType) && (x->lev == 1))
              printf(" %s#%lld", x->name, x->val);
          else
              printf(" %s_base#%d", x->name, x->val);
          break;
      case CSGConst: printf(" %lld", x->val); break;
      case CSGFld: printf(" %s_offset#%d", x->name, x->val); break;
      case CSGInst: case CSGAddr: printf(" (%d)", x->line); break;
      case CSGProc: printf(" [%d]", x->true->line); break;
      default: printf("unknown class");
    }
  }
}


void CSGDecode(void)
{
  register CSGNode i;
  register int cnt;

  // assign line numbers
  cnt = 1;
  i = code;
  while (i != NULL) {
    i->line = cnt;
    cnt++;
    i = i->nxt;
  }

  i = code;
  while (i != NULL) {
    printf("    instr %d: ", i->line);
    switch (i->op) {
      case iadd: printf("add"); PrintNode(i->x); PrintNode(i->y); break;
      case isub: printf("sub"); PrintNode(i->x); PrintNode(i->y); break;
      case imul: printf("mul"); PrintNode(i->x); PrintNode(i->y); break;
      case idiv: printf("div"); PrintNode(i->x); PrintNode(i->y); break;
      case imod: printf("mod"); PrintNode(i->x); PrintNode(i->y); break;
      case ineg: printf("neg"); PrintNode(i->x); break;

      case iparam: printf("param"); PrintNode(i->x); break;
      case ienter: printf("enter"); PrintNode(i->x); break;
      case ientrypc: printf("entrypc"); break;
      case ileave: printf("leave"); break;
      case iret: printf("ret"); PrintNode(i->x); break;
      case iend: printf("end"); break;

      case icall: printf("call"); PrintNode(i->x); break;
      case ibr: printf("br"); PrintBrakNode(i->x); break;

      case iblbc: printf("blbc"); PrintNode(i->x); PrintBrakNode(i->y); break;
      case iblbs: printf("blbs"); PrintNode(i->x); PrintBrakNode(i->y); break;

      case icmpeq: printf("cmpeq"); PrintNode(i->x); PrintNode(i->y); break;
      case icmple: printf("cmple"); PrintNode(i->x); PrintNode(i->y); break;
      case icmplt: printf("cmplt"); PrintNode(i->x); PrintNode(i->y); break;

      case iread: printf("read"); break;
      case iwrite: printf("write"); PrintNode(i->x); break;
      case iwrl: printf("wrl"); break;

      case iload: printf("load"); PrintNode(i->x); break;
      case istore: printf("store"); PrintNode(i->x); PrintNode(i->y); break;
      case imove: printf("move"); PrintNode(i->x); PrintNode(i->y); break;

      case inop: printf("nop"); break;
      default: printf("unknown instruction");
    }
    printf("\n");
    i = i->nxt;
  }
}


void CSGInit(void)
{
  entrypc = NULL;
  code = malloc(sizeof(CSGNodeDesc));
  assert(code != NULL);
  code->class = CSGInst;
  code->op = inop;
  code->prv = NULL;
  code->nxt = NULL;

  CSGlongType = malloc(sizeof(CSGTypeDesc));
  assert(CSGlongType != NULL);
  CSGlongType->form = CSGInteger;
  CSGlongType->size = 8;
  CSGboolType = malloc(sizeof(CSGTypeDesc));
  assert(CSGboolType != NULL);
  CSGboolType->form = CSGBoolean;
  CSGboolType->size = 8;

  GP = malloc(sizeof(CSGNodeDesc));
  assert(GP != NULL);
  CSGlongType->form = CSGInteger;
  CSGlongType->size = 8;
  FP = malloc(sizeof(CSGNodeDesc));
  assert(FP != NULL);
  CSGlongType->form = CSGInteger;
  CSGlongType->size = 8;
}
