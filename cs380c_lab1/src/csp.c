#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "css.h"
#include "csg.h"


static int sym;
static int instruct;
static int tos;
static CSGNode globscope;


// This function searches for an object named id in the root scope.  If
// found, a pointer to the object is returned.  Otherwise, NULL is returned.
static CSGNode FindObj(CSGNode *root, CSSIdent *id)
{
  register int maxlev;
  register CSGNode curr;
  register CSGNode obj;

  maxlev = -1;
  curr = *root;
  obj = NULL;
  while (curr != NULL) {
    while ((curr != NULL) && ((strcmp(curr->name, *id) != 0) || (curr->lev <= maxlev))) {
      curr = curr->next;
    }
    if (curr != NULL) {
      obj = curr;
      maxlev = curr->lev;
      curr = curr->next;
    }
  }
  if (obj != NULL) {
    if (((obj->class == CSGVar) || (obj->class == CSGFld)) && ((obj->lev != 0) && (obj->lev != CSGcurlev))) {
      CSSError("object cannot be accessed");
    }
  }
  return obj;
}


// This function adds a new object at the end of the object list pointed to
// by root and returns a pointer to the new node.
static CSGNode AddToList(CSGNode *root, CSSIdent *id)
{
  register CSGNode curr;

  curr = NULL;
  if (*root == NULL) {  // first object
    curr = malloc(sizeof(CSGNodeDesc));
    assert(curr != NULL);
    *root = curr;
    if (curr == NULL) CSSError("out of memory");
    curr->class = -1;
    curr->lev = CSGcurlev;
    curr->next = NULL;
    curr->dsc = NULL;
    curr->type = NULL;
    strcpy(curr->name, *id);
    curr->val = 0;
  } else {  // linked list is not empty, add to the end of the list
    curr = *root;
    while (((curr->lev != CSGcurlev) || (strcmp(curr->name, *id) != 0)) && (curr->next != NULL)) {
      curr = curr->next;
    }
    if ((strcmp(curr->name, *id) == 0) && (curr->lev == CSGcurlev)) {
      CSSError("duplicate identifier");
    } else {
      curr->next = malloc(sizeof(CSGNodeDesc));
      assert(curr->next != NULL);
      curr = curr->next;
      if (curr == NULL) CSSError("out of memory");
      curr->class = -1;
      curr->lev = CSGcurlev;
      curr->next = NULL;
      curr->dsc = NULL;
      curr->type = NULL;
      strcpy(curr->name, *id);
      curr->val = 0;
    }
  }
  return curr;
}


// This function initializes the fields of an object.
static void InitObj(CSGNode obj, signed char class, CSGNode dsc, CSGType type, long long val)
{
  obj->class = class;
  obj->next = NULL;
  obj->dsc = dsc;
  obj->type = type;
  obj->val = val;
}


// Similar to InitObj(), but also initalizes the ENTRY POINT of a procedure.
static void InitProcObj(CSGNode obj, signed char class, CSGNode dsc, CSGType type, CSGNode entrypt)
{
  obj->class = class;
  obj->next = NULL;
  obj->dsc = dsc;
  obj->type = type;
  obj->true = entrypt;
}


/*************************************************************************/


static void Expression(CSGNode *x);
static void DesignatorM(CSGNode *x);


static void Factor(CSGNode *x)
{
  register CSGNode obj;

  switch (sym) {
    case CSSident:
      obj = FindObj(&globscope, &CSSid);
      if (obj == NULL) CSSError("unknown identifier");
      CSGMakeNodeDesc(x, obj);
      sym = CSSGet();  // consume ident before calling Designator
      DesignatorM(x);
      break;
    case CSSnumber:
      CSGMakeConstNodeDesc(x, CSGlongType, CSSval);
      sym = CSSGet();
      break;
    case CSSlparen:
      sym = CSSGet();
      Expression(x);
      if (sym != CSSrparen) CSSError("')' expected");
      sym = CSSGet();
      break;
    default: CSSError("factor expected"); break;
  }
}


static void Term(CSGNode *x)
{
  register int op;
  CSGNode y;

  Factor(x);
  while ((sym == CSStimes) || (sym == CSSdiv) || (sym == CSSmod)) {
    op = sym; 
    sym = CSSGet();
    y = malloc(sizeof(CSGNodeDesc));
    assert(y != NULL);
    Factor(&y);
    CSGOp2(op, x, y);
  }
}


static void SimpleExpression(CSGNode *x)
{
  register int op;
  CSGNode y;

  if ((sym == CSSplus) || (sym == CSSminus)) {
    op = sym; 
    sym = CSSGet();
    Term(x);
    CSGOp1(op, x);
  } else {
    Term(x);
  }
  while ((sym == CSSplus) || (sym == CSSminus)) {
    op = sym; 
    sym = CSSGet();
    y = malloc(sizeof(CSGNodeDesc));
    assert(y != NULL);
    Term(&y);
    CSGOp2(op, x, y);
  }
}


static void EqualityExpr(CSGNode *x)
{
  register int op;
  CSGNode y;

  SimpleExpression(x);
  if ((sym == CSSlss) || (sym == CSSleq) || (sym == CSSgtr) || (sym == CSSgeq)) {
    y = malloc(sizeof(CSGNodeDesc));
    assert(y != NULL);
    op = sym; 
    sym = CSSGet();
    SimpleExpression(&y);
    CSGRelation(op, x, y);
  }
}


static void Expression(CSGNode *x)
{
  register int op;
  CSGNode y;

  EqualityExpr(x);
  if ((sym == CSSeql) || (sym == CSSneq)) {
    op = sym; 
    sym = CSSGet();
    y = malloc(sizeof(CSGNodeDesc));
    assert(y != NULL);
    EqualityExpr(&y);
    CSGRelation(op, x, y);
  }
}


static void ConstExpression(CSGNode *expr)
{
  Expression(expr);
  if ((*expr)->class != CSGConst) CSSError("constant expression expected");
}


/*************************************************************************/


static void VariableDeclaration(CSGNode *root);


static void FieldList(CSGType type)
{
  register CSGNode curr;

  VariableDeclaration(&(type->fields));
  while (sym != CSSrbrace) {
    VariableDeclaration(&(type->fields));
  }
  curr = type->fields;
  if (curr == NULL) CSSError("empty structs are not allowed");
  while (curr != NULL) {
    curr->class = CSGFld;
    curr->val = type->size;
    type->size += curr->type->size;
    if (type->size > 0x7fffffff) CSSError("struct too large");
    curr = curr->next;
  }
}


static void StructType(CSGType *type)
{
  register CSGNode obj;
  register int oldinstruct;
  CSSIdent id;

  assert(sym == CSSstruct);
  sym = CSSGet();
  if (sym != CSSident) CSSError("identifier expected");
  strcpy(id, CSSid);
  sym = CSSGet();
  if (sym != CSSlbrace) {
    obj = FindObj(&globscope, &id);
    if (obj == NULL) CSSError("unknown struct type");
    if ((obj->class != CSGTyp) || (obj->type->form != CSGStruct)) CSSError("struct type expected");
    *type = obj->type;
  } else {
    sym = CSSGet();
    *type = malloc(sizeof(CSGTypeDesc));
    if ((*type) == NULL) CSSError("out of memory");
    (*type)->form = CSGStruct;
    (*type)->fields = NULL;
    (*type)->size = 0;
    oldinstruct = instruct;
    instruct = 1;
    FieldList(*type);
    instruct = oldinstruct;
    if (sym != CSSrbrace) CSSError("'}' expected");
    sym = CSSGet();
    obj = AddToList(&globscope, &id);
    InitObj(obj, CSGTyp, NULL, *type, (*type)->size);
  }
}


static void Type(CSGType *type)
{
  register CSGNode obj;

  if (sym == CSSstruct) {
    StructType(type);
  } else {
    if (sym != CSSident) CSSError("identifier expected");
    obj = FindObj(&globscope, &CSSid);
    sym = CSSGet();
    if (obj == NULL) CSSError("unknown type");
    if (obj->class != CSGTyp) CSSError("type expected");
    *type = obj->type;
  }
}


static void RecurseArray(CSGType *type)
{
  register CSGType typ;
  CSGNode expr;

  expr = malloc(sizeof(CSGNodeDesc));
  assert(expr != NULL);
  assert(sym == CSSlbrak);
  sym = CSSGet();
  ConstExpression(&expr);
  if (expr->type != CSGlongType) CSSError("constant long expression required");
  if (sym != CSSrbrak) CSSError("']' expected");
  sym = CSSGet();
  if (sym == CSSlbrak) {
    RecurseArray(type);
  }
  typ = malloc(sizeof(CSGTypeDesc));
  if (typ == NULL) CSSError("out of memory");
  typ->form = CSGArray;
  typ->len = expr->val;
  typ->base = *type;
  if (0x7fffffff / typ->len < typ->base->size) {
    CSSError("array size too large");
  }
  typ->size = typ->len * typ->base->size;
  *type = typ;
}


static void IdentArray(CSGNode *root, CSGType type)
{
  register CSGNode obj;

  if (sym != CSSident) CSSError("identifier expected");
  obj = AddToList(root, &CSSid);
  sym = CSSGet();
  if (sym == CSSlbrak) {
    RecurseArray(&type);
  }
  if (instruct == 0) tos -= type->size;
  InitObj(obj, CSGVar, NULL, type, tos);
}


static void IdentList(CSGNode *root, CSGType type)
{
  IdentArray(root, type);
  while (sym == CSScomma) {
    sym = CSSGet();
    IdentArray(root, type);
  }
}


static void VariableDeclaration(CSGNode *root)
{
  CSGType type;

  Type(&type);
  IdentList(root, type);
  if (sym != CSSsemicolon) CSSError("';' expected");
  sym = CSSGet();
}


static void ConstantDeclaration(CSGNode *root)
{
  register CSGNode obj;
  CSGType type;
  CSGNode expr;
  CSSIdent id;

  expr = malloc(sizeof(CSGNodeDesc));
  assert(expr != NULL);
  assert(sym == CSSconst);
  sym = CSSGet();
  Type(&type);
  if (type != CSGlongType) CSSError("only long supported");
  if (sym != CSSident) CSSError("identifier expected");
  strcpy(id, CSSid);
  sym = CSSGet();
  if (sym != CSSbecomes) CSSError("'=' expected");
  sym = CSSGet();
  ConstExpression(&expr);
  if (expr->type != CSGlongType) CSSError("constant long expression required");
  obj = AddToList(root, &id);
  InitObj(obj, CSGConst, NULL, type, expr->val);
  if (sym != CSSsemicolon) CSSError("';' expected");
  sym = CSSGet();
}


/*************************************************************************/


static void DesignatorM(CSGNode *x)
{
  register CSGNode obj;
  CSGNode y;

  // CSSident already consumed
  while ((sym == CSSperiod) || (sym == CSSlbrak)) {
    if (sym == CSSperiod) {
      sym = CSSGet();
      if ((*x)->type->form != CSGStruct) CSSError("struct type expected");
      if (sym != CSSident) CSSError("field identifier expected");
      obj = FindObj(&(*x)->type->fields, &CSSid);
      sym = CSSGet();
      if (obj == NULL) CSSError("unknown identifier");
      CSGField(x, obj);
    } else {
      sym = CSSGet();
      if ((*x)->type->form != CSGArray) CSSError("array type expected");
      y = malloc(sizeof(CSGNodeDesc));
      assert(y != NULL);
      Expression(&y);
      CSGIndex(x, y);
      if (sym != CSSrbrak) CSSError("']' expected");
      sym = CSSGet();
    }
  }
}


static void AssignmentM(CSGNode *x)
{
  CSGNode y;

  assert(x != NULL);
  assert(*x != NULL);
  // CSSident already consumed
  y = malloc(sizeof(CSGNodeDesc));
  assert(y != NULL);
  DesignatorM(x);
  if (sym != CSSbecomes) CSSError("'=' expected");
  sym = CSSGet();
  Expression(&y);
  CSGStore(*x, y);
  if (sym != CSSsemicolon) CSSError("';' expected");
  sym = CSSGet();
}


static void ExpList(CSGNode proc)
{
  register CSGNode curr;
  CSGNode x;

  x = malloc(sizeof(CSGNodeDesc));
  assert(x != NULL);
  curr = proc->dsc;
  Expression(&x);
  if ((curr == NULL) || (curr->dsc != proc)) CSSError("too many parameters");
  if (x->type != curr->type) CSSError("incorrect type");
  CSGParameter(&x, curr->type, curr->class);
  curr = curr->next;
  while (sym == CSScomma) {
    x = malloc(sizeof(CSGNodeDesc));
    assert(x != NULL);
    sym = CSSGet();
    Expression(&x);
    if ((curr == NULL) || (curr->dsc != proc)) CSSError("too many parameters");
    if (x->type != curr->type) CSSError("incorrect type");
    CSGParameter(&x, curr->type, curr->class);
    curr = curr->next;
  }
  if ((curr != NULL) && (curr->dsc == proc)) CSSError("too few parameters");
}


static void ProcedureCallM(CSGNode obj, CSGNode *x)
{
  CSGNode y;

  // CSSident already consumed
  CSGMakeNodeDesc(x, obj);
  if (sym != CSSlparen) CSSError("'(' expected");
  sym = CSSGet();
  if ((*x)->class == CSGSProc) {
    y = malloc(sizeof(CSGNodeDesc));
    assert(y != NULL);
    if ((*x)->val == 1) {
      if (sym != CSSident) CSSError("identifier expected");
      obj = FindObj(&globscope, &CSSid);
      if (obj == NULL) CSSError("unknown identifier");
      CSGMakeNodeDesc(&y, obj);
      sym = CSSGet();  // consume ident before calling Designator
      DesignatorM(&y);
    } else if ((*x)->val == 2) {
      Expression(&y);
    }
    CSGIOCall(*x, y);
  } else {
    assert((*x)->type == NULL);
    if (sym != CSSrparen) {
      ExpList(obj);
    } else {
      if ((obj->dsc != NULL) && (obj->dsc->dsc == obj)) CSSError("too few parameters");
    }
    CSGCall(*x);
  }
  if (sym != CSSrparen) CSSError("')' expected");
  sym = CSSGet();
  if (sym != CSSsemicolon) CSSError("';' expected");
  sym = CSSGet();
}


static void StatementSequence(void);


// This function parses if statements - helpful for CFG creation.
static void IfStatement(void)
{
  CSGNode label;
  CSGNode x;

  x = malloc(sizeof(CSGNodeDesc));
  assert(x != NULL);
  assert(sym == CSSif);
  sym = CSSGet();
  CSGInitLabel(&label);
  if (sym != CSSlparen) CSSError("'(' expected");
  sym = CSSGet();
  Expression(&x);
  CSGTestBool(&x);
  CSGFixLink(x->false);
  if (sym != CSSrparen) CSSError("')' expected");
  sym = CSSGet();
  if (sym != CSSlbrace) CSSError("'{' expected");
  sym = CSSGet();
  StatementSequence();
  if (sym != CSSrbrace) CSSError("'}' expected");
  sym = CSSGet();
  if (sym == CSSelse) {
    sym = CSSGet();
    CSGFJump(&label);
    CSGFixLink(x->true);
    if (sym != CSSlbrace) CSSError("'{' expected");
    sym = CSSGet();
    StatementSequence();
    if (sym != CSSrbrace) CSSError("'}' expected");
    sym = CSSGet();
  } else {
    CSGFixLink(x->true);
  }
  CSGFixLink(label);
}


// This function parses while statements - helpful for CFG creation.
static void WhileStatement(void)
{
  CSGNode label;
  CSGNode x;

  x = malloc(sizeof(CSGNodeDesc));
  assert(x != NULL);
  assert(sym == CSSwhile);
  sym = CSSGet();
  if (sym != CSSlparen) CSSError("'(' expected");
  sym = CSSGet();
  CSGSetLabel(&label);
  Expression(&x);
  CSGTestBool(&x);
  CSGFixLink(x->false);
  if (sym != CSSrparen) CSSError("')' expected");
  sym = CSSGet();
  if (sym != CSSlbrace) CSSError("'{' expected");
  sym = CSSGet();
  StatementSequence();
  if (sym != CSSrbrace) CSSError("'}' expected");
  sym = CSSGet();
  CSGBJump(label);
  CSGFixLink(x->true);
}


static void Statement(void)
{
  register CSGNode obj;
  CSGNode x;

  switch (sym) {
    case CSSif: IfStatement(); break;
    case CSSwhile: WhileStatement(); break;
    case CSSident:
      obj = FindObj(&globscope, &CSSid);
      if (obj == NULL) CSSError("unknown identifier");
      sym = CSSGet();
      x = malloc(sizeof(CSGNodeDesc));
      assert(x != NULL);
      if (sym == CSSlparen) {
        ProcedureCallM(obj, &x);
      } else {
        CSGMakeNodeDesc(&x, obj);
        AssignmentM(&x);
      }
      break;
    case CSSsemicolon: break;  /* empty statement */
    default: CSSError("unknown statement");
  }
}


static void StatementSequence(void)
{
  while (sym != CSSrbrace) {
    Statement();
  }
}


/*************************************************************************/


static void FPSection(CSGNode *root, int *paddr)
{
  register CSGNode obj;
  CSGType type;

  Type(&type);
  if (type != CSGlongType) CSSError("only basic type formal parameters allowed");
  if (sym != CSSident) CSSError("identifier expected");
  obj = AddToList(root, &CSSid);
  sym = CSSGet();
  if (sym == CSSlbrak) CSSError("no array parameters allowed");
  InitObj(obj, CSGVar, *root, type, 0);
  *paddr += type->size; 
}


static void FormalParameters(CSGNode *root)
{
  register CSGNode curr;
  int paddr;

  paddr = 16;
  FPSection(root, &paddr);
  while (sym == CSScomma) {
    sym = CSSGet();
    FPSection(root, &paddr);
  }
  curr = (*root)->next;
  while (curr != NULL) {
    paddr -= curr->type->size;
    curr->val = paddr;
    curr = curr->next;
  }
}


static void ProcedureHeading(CSGNode *proc)
{
  CSSIdent name;

  if (sym != CSSident) CSSError("function name expected");
  strcpy(name, CSSid);
  *proc = AddToList(&globscope, &name);
  InitProcObj(*proc, CSGProc, NULL, NULL, CSGpc);
  CSGAdjustLevel(1);
  sym = CSSGet();
  if (sym != CSSlparen) CSSError("'(' expected");
  sym = CSSGet();
  if (sym != CSSrparen) {
    FormalParameters(proc);
  }
  if (sym != CSSrparen) CSSError("')' expected");
  sym = CSSGet();
  if (strcmp(name, "main") == 0) CSGEntryPoint();
}


static void ProcedureBody(CSGNode *proc)
{
  register int returnsize;
  register CSGNode curr;

  tos = 0;
  while ((sym == CSSconst) || (sym == CSSstruct) || ((sym == CSSident) && (strcmp(CSSid, "long") == 0))) {
    if (sym == CSSconst) {
      ConstantDeclaration(proc);
    } else {
      VariableDeclaration(proc);
    }
  }
  assert((*proc)->dsc == NULL);
  (*proc)->dsc = (*proc)->next;
  if (-tos > 32768) CSSError("maximum stack frame size of 32kB exceeded");
  CSGEnter(-tos);
  returnsize = 0;
  curr = (*proc)->dsc;
  while ((curr != NULL) && (curr->dsc == *proc)) {
    returnsize += 8;
    curr = curr->next;
  }
  StatementSequence();
  CSGReturn(returnsize);
  CSGAdjustLevel(-1);
}


static void ProcedureDeclaration(void)
{
  CSGNode proc;

  assert(sym == CSSvoid);
  sym = CSSGet();
  ProcedureHeading(&proc);
  if (sym != CSSlbrace) CSSError("'{' expected");
  sym = CSSGet();
  ProcedureBody(&proc);
  if (sym != CSSrbrace) CSSError("'}' expected");
  sym = CSSGet();
  proc->next = NULL;  // cut off rest of list
}


static void Program(void)
{
  CSGOpen();
  tos = 32768;
  instruct = 0;
  while ((sym != CSSvoid) && (sym != CSSeof)) {
    if (sym == CSSconst) {
      ConstantDeclaration(&globscope);
    } else {
      VariableDeclaration(&globscope);
    }
  }
  CSGStart(32768 - tos);
  if (sym != CSSvoid) CSSError("procedure expected");
  while (sym == CSSvoid) {
    ProcedureDeclaration();
  }
  if (sym != CSSeof) CSSError("unrecognized characters at end of file");
}


/*************************************************************************/


static void InsertObj(CSGNode *root, signed char class, CSGType type, CSSIdent name, long long val)
{
  register CSGNode curr;

  if (*root == NULL) {
    *root = malloc(sizeof(CSGNodeDesc));
    if (*root == NULL) CSSError("out of memory");
    curr = *root;
  } else {
    curr = *root;
    if (strcmp(curr->name, name) == 0) CSSError("duplicate symbol");
    while (curr->next != NULL) {
      curr = curr->next;
      if (strcmp(curr->name, name) == 0) CSSError("duplicate symbol");
    }
    curr->next = malloc(sizeof(CSGNodeDesc));
    assert(curr->next != NULL);
    curr = curr->next;
    if (curr == NULL) CSSError("out of memory");
  }
  curr->next = NULL;
  curr->class = class;
  curr->type = type;
  strcpy(curr->name, name);
  curr->val = val;
  curr->dsc = NULL;
  curr->lev = 0;
}


static void Compile(char *filename)
{
  fprintf(stderr, "compiling %s\n", filename);

  globscope = NULL;
  InsertObj(&globscope, CSGTyp, CSGlongType, "long", 8);
  InsertObj(&globscope, CSGSProc, NULL, "ReadLong", 1);
  InsertObj(&globscope, CSGSProc, NULL, "WriteLong", 2);
  InsertObj(&globscope, CSGSProc, NULL, "WriteLine", 3);

  CSSInit(filename);
  sym = CSSGet();
  Program();
}


/*************************************************************************/


int main(int argc, char *argv[])
{
  CSGInit();
  if (argc >= 2) {
    Compile(argv[1]);
  } else {
    Compile("test.c");
  }
  CSGDecode();

  return 0;
}
