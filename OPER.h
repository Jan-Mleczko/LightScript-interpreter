/*INTERPRETATOR LIGHTSCRIPT, OKROJONEGO JAVASCRIPT - Jan Mleczko.
  Modu³ OP - Operation - wykonywanie skryptu w przek³adzie na format
  wewnêtrzny.

  op_init - inicjacja.
  op_cleanup - sprz¹tanie.
  op_defineSpecFunc - definiowanie funkcji specjalnej udostêpnianej od strony
                      interpretatora.
  op_appendOp - dopisanie kroku.
  op_executeAll - poœrednie wykonanie ca³ego skryptu.
*/
#ifndef OPER_H
#define OPER_H

struct op_FnDesc {
	char name[32];
	unsigned int params;
	};
union op_OpDetails {
	void (*calcFn) (struct jv_Value *, struct jv_Value *,
			struct jv_Value *);
	struct jv_Value immValue;
	char name[32], fileName[51];
	unsigned long int lineNumber;
	struct op_FnDesc fnDesc;
	};
struct op_Operation {
	char operation;
	union op_OpDetails details;
	struct op_Operation *next, *branch;
	};
#define OP_LOAD 1
#define OP_IMMEDIATE 2
#define OP_CALCULATE 3
#define OP_INDEX 4
#define OP_STORE 5
#define OP_ENTER 6
#define OP_EXIT 7
#define OP_CONDITION 8
#define OP_JUMP 9
#define OP_LINEMARK 10
#define OP_FILEMARK 11
#define OP_VARIABLE 12
#define OP_FUNCTION 13
#define OP_NEW 14
#define OP_ANCHOR 15
#define OP_IGNORE 16
#define OP_TAKE 17
struct op_Operation *op_head, *op_last;

struct op_Function {
	struct op_Operation *entryPoint;
	void (*specialImpl) ();
	unsigned int params;
	};
struct di_Dictionary op_functionsDict;

struct op_Stack {
	struct jv_Value value, *lvalue;
	struct op_Stack *deeper;
	};
struct op_Stack *op_stack;
	
struct op_Scope {
	struct di_Dictionary variables;
	struct op_Operation *exitPoint;
	struct op_Scope *wider;
	};
struct op_Scope op_global, *op_topScope;

void op_init () {
	op_head = op_last = NULL;
	di_init (&op_functionsDict);
	di_init (&op_global.variables);
	op_global.exitPoint = NULL;
	op_global.wider = NULL;
	op_topScope = &op_global;
	op_stack = NULL;
	}

struct op_Operation *op_appendOp () {
	struct op_Operation *oper;

	if ((oper = malloc (sizeof (struct op_Operation))) == NULL)
		eh_heapFull ();
	*(op_head == NULL ? &op_head : &op_last->next) = oper;
	oper->next = NULL;
	return op_last = oper;
	}

void op_defineSpecFunc (char *name, void (*func) (), unsigned int p) {
	struct op_Function *fcnHeader;

	eh_establishTemp ();
	if ((fcnHeader = malloc (sizeof (struct op_Function))) == NULL)
		eh_heapFull ();
	eh_registerTemp (fcnHeader);
	fcnHeader->entryPoint = NULL;
	fcnHeader->specialImpl = func;
	fcnHeader->params = p;
	*di_insert (&op_functionsDict, name, strlen (name)) = fcnHeader;
	eh_leaveTemp ();
	}

void op_dropStackItem (struct op_Stack *item) {
	assert(item != NULL);
	jv_unset (&item->value);
	free (item);
	}

void op_dropScope (struct op_Scope *scope) {
	struct di_Dictionary *locals;
	struct di_Iteration iter;
	void *elem;

	di_open (locals = &scope->variables, &iter);
	while ((elem = di_next (&iter)) != NULL) {
		jv_unset (elem);
		free (elem);
		}
	di_drop (locals);
	}

void op_cleanup () {
	struct op_Operation *oper, *nxOper;
	struct op_Stack *stack, *nxStack;
	struct op_Scope *scope, *nxScope;
	void *elem;
	struct di_Iteration iter;

	stack = op_stack;
	while (stack != NULL) {
		nxStack = stack->deeper;
		op_dropStackItem (stack);
		stack = nxStack;
		}
	DG_TRACE(zwolniono stos)
	DG_LOGHEAP
	/****************************************/
	di_open (&op_functionsDict, &iter);
	while ((elem = di_next (&iter)) != NULL)
		free (elem);
	di_drop (&op_functionsDict);
	DG_TRACE(zwolniono funkcje)
	DG_LOGHEAP
	/****************************************/
	oper = op_head;
	while (oper != NULL) {
		if (oper->operation == OP_IMMEDIATE)
			jv_unset (&oper->details.immValue);
		nxOper = oper->next;
		free (oper);
		oper = nxOper;
		}
	DG_TRACE(zwolniono wartosci natychmiastowe)
	DG_LOGHEAP
	/****************************************/
	scope = op_topScope;
	do {
		assert(scope != NULL);
		op_dropScope (scope);
		nxScope = scope->wider;
		if (scope != &op_global)
			free (scope);
		scope = nxScope;
		} while (scope != NULL);
	DG_TRACE(zwolniono zmienne)
	DG_LOGHEAP
	}

struct jv_Value *op_findVariable (char *varName) {
	unsigned int varNamLen;
	struct op_Scope *search;
	struct jv_Value *found;
	void **link;

	assert(varName != NULL);
	varNamLen = strlen (varName);
	search = op_topScope;
	do {
		assert(search != NULL);
		if ((found = di_find (&search->variables, varName, varNamLen)
				) != NULL)
			return found;
		search = search->wider;
		} while (search != NULL);
	eh_reportErr ("Undeclared variable.", 1);
	link = di_insert (&op_global.variables, varName, varNamLen);
	if ((found = malloc (sizeof (struct jv_Value))) == NULL)
		eh_heapFull ();
	jv_setUndefined (found);
	return *link = found;
	}

struct op_Stack *op_pushToStack () {
	struct op_Stack *new;

	if ((new = malloc (sizeof (struct op_Stack))) == NULL)
		eh_heapFull ();
	new->deeper = op_stack;
	return op_stack = new;
	}

void op_pushUndefined () {
	struct op_Stack *item;

	(item = op_pushToStack ())->lvalue = NULL;
	jv_setUndefined (&item->value);
	}

struct op_Stack *op_popFromStack () {
	struct op_Stack *old;

	assert(op_stack != NULL);
	old = op_stack;
	op_stack = op_stack->deeper;
	return old;
	}

void op_regTmpStackItem (struct op_Stack *item) {
	assert(item != NULL);
	if (item->value.type == JV_STRING)
		jv_regTmpString (item->value.content.strContent);
	eh_registerTemp (item);
	}

void op_executeAll () {
	char *vfName;
	unsigned int vfNamLen, availParams, fnParams;
	struct op_Operation *current, *passedCtrl;
	struct op_Stack *popd1, *popd2, *pushed;
	struct jv_Value *asgnDest, value;
	struct op_Function *func;
	struct op_Scope *entered;
	struct di_Dictionary *locals;

	eh_fileKnown = 0;
	eh_line = 0;
	current = op_head;
	while (current != NULL) {
		switch (current->operation) {
		case OP_LOAD:
			eh_establishTemp ();
			pushed = op_pushToStack ();
			jv_assign (&pushed->value,
					pushed->lvalue = op_findVariable (
					current->details.name));
			current = current->next;
			eh_leaveTemp ();
			break;
		case OP_IMMEDIATE:
			pushed = op_pushToStack ();
			pushed->lvalue = NULL;
			jv_assign (&pushed->value, &current->details.immValue);
			current = current->next;
			break;
		case OP_CALCULATE:
			eh_establishTemp ();
			popd1 = op_popFromStack ();
			op_regTmpStackItem (popd1);
			popd2 = op_popFromStack ();
			op_regTmpStackItem (popd2);
			pushed = op_pushToStack ();
			current->details.calcFn (&pushed->value,
					&popd2->value,
					&popd1->value);
			pushed->lvalue = NULL;
			op_dropStackItem (popd1);
			op_dropStackItem (popd2);
			current = current->next;
			eh_leaveTemp ();
			break;
		case OP_INDEX:
			eh_establishTemp ();
			popd1 = op_popFromStack ();
			op_regTmpStackItem (popd1);
			popd2 = op_popFromStack ();
			op_regTmpStackItem (popd2);
			pushed = op_pushToStack ();
			pushed->lvalue = jv_index (&pushed->value,
					&popd2->value,
					&popd1->value);
			op_dropStackItem (popd1);
			op_dropStackItem (popd2);
			current = current->next;
			eh_leaveTemp ();
			break;
		case OP_STORE:
			eh_establishTemp ();
			popd1 = op_popFromStack ();
			op_regTmpStackItem (popd1);
			popd2 = op_popFromStack ();
			op_regTmpStackItem (popd2);
			if ((asgnDest = popd2->lvalue) == NULL)
				eh_reportErr ("Invalid assignment "
						"destination!", 0);
			jv_unset (asgnDest);
			jv_assign (asgnDest, &popd1->value);
			op_dropStackItem (popd1);
			op_dropStackItem (popd2);
			current = current->next;
			eh_leaveTemp ();
			break;
		case OP_ENTER:
			vfNamLen = strlen (vfName =
					current->details.fnDesc.name);
			if ((func = di_find (&op_functionsDict,
					vfName, vfNamLen)) == NULL)
				eh_reportErr ("Undefined function!", 0);
			fnParams = func->params;
			assert(fnParams <= 8);
			availParams = current->details.fnDesc.params;
			if (availParams < fnParams) {
				eh_reportErr ("Too few parameters for "
						"function.", 1);
				while (availParams < fnParams) {
					(pushed = op_pushToStack ()
							)->lvalue = NULL;
					jv_setUndefined (&pushed->value);
					++availParams;
					}
				}
			else if (availParams > fnParams) {
				eh_reportErr ("Too many parameters for "
						"function.", 1);
				while (availParams > fnParams) {
					op_dropStackItem (op_popFromStack ());
					--availParams;
					}
				}
			if ((passedCtrl = func->entryPoint) == NULL) {
				func->specialImpl ();
				current = current->next;
				}
			else {
				eh_establishTemp ();
				if ((entered = malloc (
						sizeof (struct op_Scope))
						) == NULL)
					eh_heapFull ();
				eh_registerTemp (entered);
				di_init (&entered->variables);
				entered->exitPoint = current->next;
				entered->wider = op_topScope;
				op_topScope = entered;
				current = passedCtrl;
				eh_leaveTemp ();
				}
			break;
		case OP_EXIT:
			assert(op_topScope != &op_global);
			current = op_topScope->exitPoint;
			entered = op_topScope->wider;
			op_dropScope (op_topScope);
			free (op_topScope);
			op_topScope = entered;
			assert(op_topScope != NULL);
			assert(op_stack != NULL);
			op_stack->lvalue = NULL;
			break;
		case OP_CONDITION:
			popd1 = op_popFromStack ();
			jv_toBoolean (&value, &popd1->value);
			current = value.content.boolContent ? current->next
					: current->branch;
			op_dropStackItem (popd1);
			break;
		case OP_JUMP:
			current = current->branch;
			break;
		case OP_LINEMARK:
			eh_line = current->details.lineNumber;
			current = current->next;
			break;
		case OP_FILEMARK:
			strcpy (eh_fileName, current->details.fileName);
			eh_fileKnown = 1;
			current = current->next;
			break;
		case OP_VARIABLE:
			eh_establishTemp ();
			if ((asgnDest = malloc (sizeof (struct jv_Value)))
					== NULL)
				eh_heapFull ();
			eh_registerTemp (asgnDest);
			jv_setUndefined (asgnDest);
			vfNamLen = strlen (vfName = current->details.name);
			if (di_find (locals = &op_topScope->variables, vfName,
					vfNamLen) == NULL)
				*di_insert (locals, vfName, vfNamLen)
						= asgnDest;
			else
				eh_reportErr ("Variable redeclaration.", 1);
			current = current->next;
			eh_leaveTemp ();
			break;
		case OP_FUNCTION:
			eh_establishTemp ();
			if ((func = malloc (sizeof (struct op_Function)))
					== NULL)
				eh_heapFull ();
			eh_registerTemp (func);
			func->entryPoint = current->next;
			func->params = current->details.fnDesc.params;
			vfNamLen = strlen (vfName =
					current->details.fnDesc.name);
			if (di_find (&op_functionsDict, vfName, vfNamLen)
					!= NULL)
				eh_reportErr ("Function already exists!", 0);
			*di_insert (&op_functionsDict, vfName, vfNamLen)
					= func;
			current = current->branch;
			eh_leaveTemp ();
			break;
		case OP_NEW:
			(pushed = op_pushToStack ())->lvalue = NULL;
			jv_setObject (&pushed->value);
			current = current->next;
			break;
		case OP_IGNORE:
			op_dropStackItem (op_popFromStack ());
		case OP_ANCHOR:
			current = current->next;
			break;
		case OP_TAKE:
			eh_establishTemp ();
			vfNamLen = strlen (vfName = current->details.name);
			if (di_find (locals = &op_topScope->variables, vfName,
					vfNamLen) != NULL)
				eh_reportErr ("Two parameters of a function "
						"have identical names!", 0);
			op_regTmpStackItem (popd1 = op_popFromStack ());
			if ((asgnDest = malloc (sizeof (struct jv_Value)))
					== NULL)
				eh_heapFull ();
			eh_registerTemp (asgnDest);
			*di_insert (locals, vfName, vfNamLen) = asgnDest;
			jv_assign (asgnDest, &popd1->value);
			current = current->next;
			op_dropStackItem (popd1);
			eh_leaveTemp ();
			break;
			}
		}
	}

#endif