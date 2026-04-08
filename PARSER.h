/*INTERPRETATOR LIGHTSCRIPT, OKROJONEGO JAVASCRIPT - Jan Mleczko.
  Modu³ PS - Parser - zamiana kodu czytanego przez modu³ LX na postaæ dla
  modu³u OP.
*/
#ifndef PARSER_H
#define PARSER_H

#define PS_CMPDSTACK 32
#define PS_IF 1
#define PS_WHILE 2
#define PS_FUNCDEF 3
struct ps_Compound {
	char type;
	struct op_Operation *begin, *earlier;
	} ps_cmpdStack[PS_CMPDSTACK], *ps_topCmpd;

struct ps_Compound *ps_pushCompound () {
	if (ps_topCmpd >= ps_cmpdStack + PS_CMPDSTACK)
		eh_reportErr ("Too deep statement nesting!", 0);
	return ps_topCmpd++;
	}

struct ps_Compound *ps_popCompound () {
	if (ps_topCmpd <= ps_cmpdStack)
		eh_syntax ();
	return --ps_topCmpd;
	}

#define PS_NORMBRACKET 1
#define PS_IDXBRACKET 2
#define PS_CALLBRACKET 3
struct ps_Bracket {
	char type;
	struct op_FnDesc callDesc;
	};
#define PS_BRACSTACK 31
#define PS_OPERSTACK 35

unsigned long int ps_lastRemindedLine;

void ps_remindLine () {
	struct op_Operation *oper;

	assert(eh_line);
	(oper = op_appendOp ())->operation = OP_LINEMARK;
	oper->details.lineNumber = ps_lastRemindedLine = eh_line;
	}

void ps_remindFile () {
	struct op_Operation *oper;

	assert(eh_fileKnown);
	(oper = op_appendOp ())->operation = OP_FILEMARK;
	strcpy (oper->details.fileName, eh_fileName);
	}

struct lx_Lexem ps_lexem;

unsigned char ps_otorStk[PS_OPERSTACK], ps_precStk[PS_OPERSTACK];
int ps_oStkTop;
void ps_releaseOperators (unsigned char minPreced) {
	struct op_Operation *oper;
	while (ps_oStkTop >= 0 && ps_precStk[ps_oStkTop] >= minPreced) {
		(oper = op_appendOp ())->operation = OP_CALCULATE;
		switch (ps_otorStk[ps_oStkTop]) {
		case LX_PLUS:
			oper->details.calcFn = jv_add;
			break;
		case LX_MINUS:
			oper->details.calcFn = jv_subtract;
			break;
		case LX_TIMES:
			oper->details.calcFn = jv_multiply;
			break;
		case LX_DIVIDE:
			oper->details.calcFn = jv_divide;
			break;
		case LX_LESS:
			oper->details.calcFn = jv_lessThan;
			break;
		case LX_EQUAL:
			oper->details.calcFn = jv_equalTo;
			break;
		case LX_GREATER:
			oper->details.calcFn = jv_greaterThan;
#ifndef NDEBUG
			break;
		default:
			assert(0);
#endif
			}
		--ps_oStkTop;
		}
	}

int ps_expression () {
	struct op_Operation *oper;
	struct ps_Bracket brackets[PS_BRACSTACK], *newBracket, *offBracket;
	unsigned char basePreced, precedence;
	char vfName[32];
	char needValue, metExpr, nNowOpened, nPrevOpened;

	needValue = nPrevOpened = 1;
	metExpr = 0;
	newBracket = brackets;
	offBracket = brackets + PS_BRACSTACK;
	ps_oStkTop = -1;
	basePreced = 0;
	do {
		nNowOpened = 1;
		if (ps_lexem.kind & LX_OPERATORS) {
			if (needValue)
				eh_exprSyntax ();
			precedence = basePreced;
			switch (ps_lexem.kind) {
			case LX_LESS:
			case LX_EQUAL:
			case LX_GREATER:
				++precedence;
				break;
			case LX_PLUS:
			case LX_MINUS:
				precedence += 2;
				break;
			case LX_TIMES:
			case LX_DIVIDE:
				precedence += 3;
#ifndef NDEBUG
				break;
			default:
				assert(0);
#endif
				}
			ps_releaseOperators (precedence);
			if (++ps_oStkTop >= PS_OPERSTACK)
				eh_exprComplex ();
			ps_otorStk[ps_oStkTop] = ps_lexem.kind;
			ps_precStk[ps_oStkTop] = precedence;
			needValue = 1;
			lx_nextLexem (&ps_lexem);
			assert(metExpr == 1);
			continue;
			}
		switch (ps_lexem.kind) {
		case LX_LITERAL:
			if (!needValue) {
				jv_unset (&ps_lexem.details.litValue);
				eh_exprSyntax ();
				}
			(oper = op_appendOp ())->operation = OP_IMMEDIATE;
			oper->details.immValue = ps_lexem.details.litValue;
			lx_nextLexem (&ps_lexem);
			needValue = 0;
			break;
		case LX_SYMBOL:
			if (!needValue)
				eh_exprSyntax ();
			strcpy (vfName, ps_lexem.details.symName);
			if (lx_nextLexem (&ps_lexem) == LX_OBRACKET) {
				if (newBracket >= offBracket
						|| basePreced >= 248)
					eh_exprComplex ();
				newBracket->type = PS_CALLBRACKET;
				strcpy (newBracket->callDesc.name, vfName);
				newBracket++->callDesc.params = 0;
				basePreced += 8;
				assert(needValue);
				lx_nextLexem (&ps_lexem);
				nNowOpened = 0;
				}
			else {
				(oper = op_appendOp ())->operation = OP_LOAD;
				strcpy (oper->details.name, vfName);
				needValue = 0;
				}
			break;
		case LX_OBRACKET:
			if (!needValue)
				eh_exprSyntax ();
			if (basePreced >= 248 || newBracket >= offBracket)
				eh_exprComplex ();
			basePreced += 8;
			newBracket++->type = PS_NORMBRACKET;
			lx_nextLexem (&ps_lexem);
			assert(needValue == 1);
			break;
		case LX_CBRACKET:
			ps_releaseOperators (basePreced);
			if (newBracket <= brackets) {
				assert(basePreced == 0);
				assert(ps_oStkTop == -1);
				return metExpr;
				}
			else {
				assert(basePreced >= 8);
				basePreced -= 8;
				switch ((--newBracket)->type) {
				case PS_NORMBRACKET:
					if (needValue)
						eh_exprSyntax ();
					break;
				case PS_CALLBRACKET:
					if (needValue) {
						if (nPrevOpened)
							eh_exprSyntax ();
						}
					else {
						if (++newBracket->callDesc
								.params > 8)
							eh_tooParam ();
						}
					(oper = op_appendOp ())->operation
							= OP_ENTER;
					oper->details.fnDesc
							= newBracket->callDesc;
					ps_remindFile ();
					ps_remindLine ();
					break;
				default:
					eh_exprSyntax ();
					}
				needValue = 0;
				lx_nextLexem (&ps_lexem);
				}
			break;
		case LX_COMMA:
			if (needValue || newBracket <= brackets
			|| (--newBracket)->type != PS_CALLBRACKET
			|| newBracket->callDesc.params >= 8)
				eh_exprSyntax ();
			ps_releaseOperators (basePreced);
			++newBracket++->callDesc.params;
			needValue = 1;
			lx_nextLexem (&ps_lexem);
			break;
		case LX_NEW:
			if (!needValue || lx_nextLexem (&ps_lexem) != LX_SYMBOL
			|| strcmp (ps_lexem.details.symName, "Object")
			|| lx_nextLexem (&ps_lexem) != LX_OBRACKET
			|| lx_nextLexem (&ps_lexem) != LX_CBRACKET) {
				lx_dropLexem (&ps_lexem);
				eh_exprSyntax ();
				}
			op_appendOp ()->operation = OP_NEW;
			needValue = 0;
			lx_nextLexem (&ps_lexem);
			break;
		case LX_INDEX:
			if (needValue)
				eh_exprSyntax ();
			if (basePreced >= 248 || newBracket >= offBracket)
				eh_exprComplex ();
			basePreced += 8;
			newBracket++->type = PS_IDXBRACKET;
			needValue = 1;
			lx_nextLexem (&ps_lexem);
			break;
		case LX_EINDEX:
			if (needValue || newBracket <= brackets
			|| (--newBracket)->type != PS_IDXBRACKET)
				eh_exprSyntax ();
			ps_releaseOperators (basePreced);
			basePreced -= 8;
			op_appendOp ()->operation = OP_INDEX;
			lx_nextLexem (&ps_lexem);
			assert(needValue == 0);
			break;
		default:
			if (basePreced || newBracket > brackets
					|| needValue && metExpr)
				eh_exprSyntax ();
			ps_releaseOperators (0);
			assert(ps_oStkTop == -1);
			return metExpr;
			}
		metExpr = 1;
		nPrevOpened = nNowOpened;
		} while (1);
	}

void ps_ifWhileCommon () {
	if (lx_nextLexem (&ps_lexem) != LX_OBRACKET) {
		lx_dropLexem (&ps_lexem);
		eh_syntax ();
		}
	lx_nextLexem (&ps_lexem);
	if (!ps_expression ())
		eh_syntax ();
	if (ps_lexem.kind != LX_CBRACKET
	|| lx_nextLexem (&ps_lexem) != LX_COMPOUND) {
		lx_dropLexem (&ps_lexem);
		eh_syntax ();
		}
	lx_nextLexem (&ps_lexem);
	}

char ps_allowFileSkip;

void ps_parseFile (char const *fnam) {
	struct op_Operation *oper;
	struct ps_Compound *cmpd;
	char parNames[256], moreParams, inFunction;
	unsigned int funcParams;

	eh_changeFile (fnam);
	eh_line = 0;
	if ((lx_input = fopen (fnam, "r")) == NULL)
		eh_reportErr ("No such file!", ps_allowFileSkip);
	ps_topCmpd = ps_cmpdStack;
	inFunction = 0;
	lx_fileInit ();
	lx_nextLexem (&ps_lexem);
	ps_remindFile ();
	ps_remindLine ();
	while (ps_lexem.kind) {
		if (eh_line != ps_lastRemindedLine)
			ps_remindLine ();
		if (ps_expression ()) {
			switch (ps_lexem.kind) {
			case LX_SEMICOLON:
				op_appendOp ()->operation = OP_IGNORE;
				lx_nextLexem (&ps_lexem);
				break;
			case LX_ASSIGN:
				lx_nextLexem (&ps_lexem);
				if (!ps_expression ())
					eh_syntax ();
				op_appendOp ()->operation = OP_STORE;
				if (ps_lexem.kind != LX_SEMICOLON)
					eh_syntax ();
				lx_nextLexem (&ps_lexem);
				break;
			default:
				eh_syntax ();
				}
			continue;
			}
		switch (ps_lexem.kind) {
		case LX_VAR:
			if (lx_nextLexem (&ps_lexem) != LX_SYMBOL) {
				lx_dropLexem (&ps_lexem);
				eh_syntax ();
				}
			(oper = op_appendOp ())->operation = OP_VARIABLE;
			strcpy (oper->details.name, ps_lexem.details.symName);
			if (lx_nextLexem (&ps_lexem) != LX_SEMICOLON) {
				lx_dropLexem (&ps_lexem);
				eh_syntax ();
				}
			lx_nextLexem (&ps_lexem);
			break;
		case LX_FUNCTION:
			if (inFunction)
				eh_reportErr ("Function definition inside "
						"a function!", 0);
			(oper = op_appendOp ())->operation = OP_FUNCTION;
			if (lx_nextLexem (&ps_lexem) != LX_SYMBOL) {
				lx_dropLexem (&ps_lexem);
				eh_syntax ();
				}
			strcpy (oper->details.fnDesc.name,
					ps_lexem.details.symName);
			if (lx_nextLexem (&ps_lexem) != LX_OBRACKET) {
				lx_dropLexem (&ps_lexem);
				eh_syntax ();
				}
			funcParams = 0;
			if (lx_nextLexem (&ps_lexem) == LX_SYMBOL) {
				moreParams = 1;
				do {
					if (funcParams >= 8)
						eh_tooParam ();
					strcpy (parNames + (funcParams++ << 5),
						ps_lexem.details.symName);
					if (lx_nextLexem (&ps_lexem)
							== LX_COMMA) {
						if (lx_nextLexem (&ps_lexem)
								!= LX_SYMBOL) {
							lx_dropLexem
							  (&ps_lexem);
							eh_syntax ();
							}
						}
					else
						moreParams = 0;
					} while (moreParams);
				}
			if (ps_lexem.kind != LX_CBRACKET) {
				lx_dropLexem (&ps_lexem);
				eh_syntax ();
				}
			if (lx_nextLexem (&ps_lexem) != LX_COMPOUND) {
				lx_dropLexem (&ps_lexem);
				eh_syntax ();
				}
			oper->details.fnDesc.params = funcParams;
			(cmpd = ps_pushCompound ())->type = PS_FUNCDEF;
			cmpd->begin = oper;
			ps_remindFile ();
			ps_remindLine ();
			inFunction = 1;
			while (funcParams--) {
				(oper = op_appendOp ())->operation = OP_TAKE;
				strcpy (oper->details.name,
						parNames + (funcParams << 5));
				}
			lx_nextLexem (&ps_lexem);
			break;
		case LX_IF:
			ps_ifWhileCommon ();
			(oper = op_appendOp ())->operation = OP_CONDITION;
			(cmpd = ps_pushCompound ())->type = PS_IF;
			cmpd->begin = oper;
			break;
		case LX_WHILE:
			(oper = op_appendOp ())->operation = OP_ANCHOR;
			ps_remindFile ();
			ps_remindLine ();
			ps_ifWhileCommon ();
			(cmpd = ps_pushCompound ())->type = PS_WHILE;
			cmpd->earlier = oper;
			(oper = op_appendOp ())->operation = OP_CONDITION;
			cmpd->begin = oper;
			break;
		case LX_ECOMPOUND:
			switch ((cmpd = ps_popCompound ())->type) {
			case PS_WHILE:
				(oper = op_appendOp ())->operation = OP_JUMP;
				oper->branch = cmpd->earlier;
				break;
			case PS_FUNCDEF:
				assert(inFunction == 1);
				(oper = op_appendOp ())->operation
						= OP_IMMEDIATE;
				jv_setUndefined (&oper->details.immValue);
				op_appendOp ()->operation = OP_EXIT;
				inFunction = 0;
				}
			(oper = op_appendOp ())->operation = OP_ANCHOR;
			cmpd->begin->branch = oper;
			ps_remindLine ();
			lx_nextLexem (&ps_lexem);
			break;
		case LX_RETURN:
			if (!inFunction)
				eh_reportErr ("Return statement outside of a "
						" function!", 0);
			lx_nextLexem (&ps_lexem);
			if (!ps_expression ()
					|| ps_lexem.kind != LX_SEMICOLON) {
				lx_dropLexem (&ps_lexem);
				eh_syntax ();
				}
			op_appendOp ()->operation = OP_EXIT;
			lx_nextLexem (&ps_lexem);
			break;
		default:
			lx_dropLexem (&ps_lexem);
			eh_syntax ();
			}
		}
	fclose (lx_input);
	}

#endif