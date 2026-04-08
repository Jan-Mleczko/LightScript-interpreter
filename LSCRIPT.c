/*INTERPRETATOR LIGHTSCRIPT, OKROJONEGO JAVASCRIPT - Jan Mleczko.
  Modu³ g³ówny
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <assert.h>

#include "DEBUG.h"
#include "ERRHAND.h"
#include "DICT.h"
#include "JSVALUES.h"
#include "LEXER.h"
#include "OPER.h"
#include "BUILTINS.h"
#include "PARSER.h"

void cleanup () {
	DG_TRACE(wejscie do cleanup)
	DG_LOGHEAP
	DG_PAUSE
	op_cleanup ();
	DG_TRACE(ukonczono op_cleanup)
	DG_LOGHEAP
	DG_PAUSE
	jv_cleanup ();
	DG_TRACE(ukonczono jv_cleanup)
	DG_LOGHEAP
	DG_PAUSE
	eh_cleanup ();
	DG_LOGHEAP
	}

#ifndef NDEBUG
void tstDumpJV (struct jv_Value *val, FILE *where) {
	struct jv_String *strHeader;

	switch (val->type) {
	case JV_NUMBER:
		fprintf (where, "Number, %ld", val->content.numContent);
		break;
	case JV_STRING:
		strHeader = val->content.strContent;
		fprintf (where, "String of %u, \"", strHeader->length);
		fwrite (strHeader->chars, 1, strHeader->length, where);
		fputc ('\"', where);
		break;
	case JV_BOOLEAN:
		fprintf (where, "Boolean, %d", val->content.boolContent);
		break;
	case JV_OBJECT:
		fputs ("Object...", where);
		break;
	case JV_UNDEFINED:
		fputs ("Undefined", where);
		break;
		}
	}
void tstDumpLx (struct lx_Lexem *lxm, FILE *where) {
	char knd;

	fprintf (where, "Leksem typu %d", knd = lxm->kind);
	switch (knd) {
	case LX_SYMBOL:
		fprintf (where, ", symbol %s", lxm->details.symName);
		break;
	case LX_LITERAL:
		fprintf (where, ", literal o wartosci ");
		tstDumpJV (&lxm->details.litValue, where);
		}
	fputc ('\n', where);
	}
void tstLogOper () {
	struct op_Operation *wsk;
	void (*funkcja) (struct jv_Value *, struct jv_Value *,
			struct jv_Value *);
	FILE *uchwyt;

	if ((uchwyt = fopen ("E:Oper_99b.LOG", "w")) == NULL) {
		DG_GENERAL(Nie udalo sie utworzyc pliku diagnostycznego!)
		return;
		}
	wsk = op_head;
	while (wsk != NULL) {
		fprintf (uchwyt, "%p ", wsk);
		switch (wsk->operation) {
		case OP_LOAD:
			fprintf (uchwyt, "Load name=%.31s\n",
					wsk->details.name);
			break;
		case OP_IMMEDIATE:
			fputs ("Immediate, ", uchwyt);
			tstDumpJV (&wsk->details.immValue, uchwyt);
			fputc ('\n', uchwyt);
			break;
		case OP_CALCULATE:
			fputs ("Calculate calcFn=", uchwyt);
			funkcja = wsk->details.calcFn;
			if (funkcja == jv_add)
				fputs ("jv_add", uchwyt);
			else if (funkcja == jv_subtract)
				fputs ("jv_subtract", uchwyt);
			else if (funkcja == jv_multiply)
				fputs ("jv_multiply", uchwyt);
			else if (funkcja == jv_divide)
				fputs ("jv_divide", uchwyt);
			else if (funkcja == jv_lessThan)
				fputs ("jv_lessThan", uchwyt);
			else if (funkcja == jv_equalTo)
				fputs ("jv_equalTo", uchwyt);
			else if (funkcja == jv_greaterThan)
				fputs ("jv_greaterThan", uchwyt);
			else
				fputs ("???", uchwyt);
			fputc ('\n', uchwyt);
			break;
		case OP_INDEX:
			fputs ("Index\n", uchwyt);
			break;
		case OP_STORE:
			fputs ("Store\n", uchwyt);
			break;
		case OP_ENTER:
			fprintf (uchwyt, "Enter name=%.31s params=%u\n",
					wsk->details.fnDesc.name,
					wsk->details.fnDesc.params);
			break;
		case OP_EXIT:
			fputs ("Exit\n", uchwyt);
			break;
		case OP_CONDITION:
			fprintf (uchwyt, "Condition branch=%p\n", wsk->branch);
			break;
		case OP_JUMP:
			fprintf (uchwyt, "Jump branch=%p\n", wsk->branch);
			break;
		case OP_LINEMARK:
			fprintf (uchwyt, "LineMark %lu\n",
					wsk->details.lineNumber);
			break;
		case OP_FILEMARK:
			fprintf (uchwyt, "FileMark %.50s\n",
					wsk->details.fileName);
			break;
		case OP_VARIABLE:
			fprintf (uchwyt, "Variable name=%.31s\n",
					wsk->details.name);
			break;
		case OP_FUNCTION:
			fprintf (uchwyt, "Function name=%.31s params=%u"
					" branch=%p\n",
					wsk->details.fnDesc.name,
					wsk->details.fnDesc.params,
					wsk->branch);
			break;
		case OP_NEW:
			fputs ("New\n", uchwyt);
			break;
		case OP_ANCHOR:
			fputs ("Anchor\n", uchwyt);
			break;
		case OP_IGNORE:
			fputs ("Ignore\n", uchwyt);
			break;
		case OP_TAKE:
			fprintf (uchwyt, "Take name=%.31s\n",
					wsk->details.name);
			break;
		default:
			fputs ("!!!!!! UNKNOWN !!!!!!\n", uchwyt);
			break;
			}
		wsk = wsk->next;
		}
	fclose (uchwyt);
	}
#endif  /*Niekoniecznie NDEBUG.*/

void intro () {
	puts ("LIGHTSCRIPT INTERPRETER - Jan Mleczko, Poland, 2026."
	" Version 1.");
	}

void help () {
	intro ();
	puts ("\
Command line syntax:\n\
  LSCRIPT <script filename> [options]\n\
Multiple files are allowed.\n\
Options are:\n\
  /A to always continue after an error if possible.\n\
  /V to never continue after an error.\n\
  /Q for \"quiet\" mode. Needs either /A or /V and implies /V if not \
specified.\n\
  /? for help.\n\
For example:\n\
  LSCRIPT EXAMPLE.LS PART2.LS /A");
	}

#define ALWAYSCONT 1
#define NEVERCONT 2
int main (int argc, char **argv) {
	char *cmdArg, **av, cmdValid, option, continuationMode, files;
	char notJustHelp, justHelp, wasntHelp;
	int ac;

	if (argc < 2) {
		help ();
		return 2;
		}
	DG_SETUP
	eh_init ();
	jv_init ();
	op_init ();
	bu_init ();
	if (atexit (cleanup))
		eh_reportErr ("atexit rejected!", 0);

	continuationMode = files = justHelp = notJustHelp = 0;
	eh_nQuiet = cmdValid = 1;
	ac = argc;
	av = argv;
	assert(ac >= 1);
	while (cmdValid && --ac) {
		wasntHelp = 1;
		if (*(cmdArg = *++av) == '/') {
			while (cmdValid && (option = *++cmdArg)) {
				switch (toupper(option)) {
				case 'A':
					if (continuationMode)
						cmdValid = 0;
					else
						continuationMode = ALWAYSCONT;
					break;
				case 'V':
					if (continuationMode)
						cmdValid = 0;
					else
						continuationMode = NEVERCONT;
					break;
				case 'Q':
					if (eh_nQuiet)
						eh_nQuiet = 0;
					else
						cmdValid = 0;
					break;
				case '?':
					justHelp = 1;
					wasntHelp = 0;
					break;
				default:
					cmdValid = 0;
					}
				}
			}
		else {
			if (files < 2)
				++files;
			}
		if (wasntHelp)
			notJustHelp = 1;
		}
	if (!cmdValid || justHelp && notJustHelp) {
		fputs ("Invalid command line syntax! "
		"No parameters or /? for help.\n", stderr);
		return 1;
		}
	if (justHelp) {
		help ();
		return 2;
		}
	if (eh_nQuiet)
		intro ();

	switch (files) {
	case 0:
		eh_reportErr ("No script files specified!", 0);
	case 1:
		ps_allowFileSkip = 0;
		break;
	default:
		ps_allowFileSkip = 1;
		}

	switch (continuationMode) {
	case ALWAYSCONT:
		eh_everContinue = eh_alwaysContinue = 1;
		break;
	case NEVERCONT:
		eh_everContinue = 0;
		break;
	default:
		if (eh_nQuiet) {
			eh_everContinue = 1;
			eh_alwaysContinue = 0;
			}
		else
			eh_everContinue = 0;
		}

	assert(argc >= 1);
	while (--argc) {
		DG_TRACE(drugie przegladanie parametrow)
		if (*(cmdArg = *++argv) != '/')
			ps_parseFile (cmdArg);
		}
	op_executeAll ();
	exit (0);
	}