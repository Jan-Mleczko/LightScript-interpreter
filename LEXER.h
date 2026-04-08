/*INTERPRETATOR LIGHTSCRIPT, OKROJONEGO JAVASCRIPT - Jan Mleczko.
  Modu³ LX - Lexer - czytanie skryptu jako leksemów.
*/

#ifndef LEXER_H
#define LEXER_H

#define LX_STACK 4
FILE *lx_input;
unsigned char lx_stack[LX_STACK], *lx_topOfStack;
char lx_outsideComment, lx_end;

void lx_fileInit () {
	lx_topOfStack = lx_stack;
	lx_outsideComment = 1;
	eh_line = 1;
	lx_end = 0;
	}

int lx_nextChr () {
	int ch;

	assert(lx_input != NULL);
	assert(lx_topOfStack != NULL);
	assert(lx_topOfStack >= lx_stack);
	assert(lx_topOfStack < lx_stack + LX_STACK);
	if (lx_topOfStack > lx_stack)
		ch = *--lx_topOfStack;
	else {
		if (lx_end)
			return EOF;
		if ((ch = fgetc (lx_input)) == '\n') {
			if (eh_line < ULONG_MAX)
				++eh_line;
			}
		}
	if (ch == EOF)
		lx_end = 1;
	else if (lx_outsideComment && ch >= 128)
		eh_reportErr ("Extended character outside of a comment!", 0);
	return ch;
	}

void lx_pushChr (int ch) {
	assert(lx_topOfStack != NULL);
	assert(lx_topOfStack >= lx_stack);
	assert(lx_topOfStack < lx_stack + LX_STACK);
	if (ch != EOF)
		*lx_topOfStack++ = ch;
	}

int lx_peekChr () {
	int ch;

	lx_pushChr (ch = lx_nextChr ());
	return ch;
	}

void lx_whitespace () {
	int ch;
	char comment, notClosed;
	unsigned long int cmtBeginLine;

	do {
		while ((ch = lx_nextChr ()) == ' ' || ch == '\t'
				|| ch == '\n');
		cmtBeginLine = eh_line;
		if (ch == '/' && lx_peekChr () == '*') {
			comment = notClosed = 1;
			lx_nextChr ();
			lx_outsideComment = 0;
			while ((ch = lx_nextChr ()) != EOF) {
				if (ch == '*' && lx_peekChr () == '/') {
					lx_nextChr ();
					notClosed = 0;
					break;
					}
				}
			if (notClosed) {
				eh_line = cmtBeginLine;
				eh_reportErr ("Comment that was never "
						"closed!", 0);
				}
			lx_outsideComment = 1;
			}
		else
			comment = 0;
		} while (comment);
	lx_pushChr (ch);
	}

#define LX_EOF 0
#define LX_VAR 1
#define LX_IF 2
#define LX_WHILE 3
#define LX_FUNCTION 4
#define LX_RETURN 5
#define LX_SYMBOL 6
#define LX_LITERAL 7
#define LX_COMPOUND 8
#define LX_ECOMPOUND 9
#define LX_ASSIGN 10
#define LX_OBRACKET 11
#define LX_CBRACKET 12
#define LX_INDEX 13
#define LX_EINDEX 14
#define LX_COMMA 15
#define LX_SEMICOLON 16
#define LX_NEW 17
#define LX_PLUS 32
#define LX_MINUS 33
#define LX_TIMES 34
#define LX_DIVIDE 35
#define LX_LESS 36
#define LX_EQUAL 37
#define LX_GREATER 38

#define LX_OPERATORS 32

union lx_Details {
	char symName[32];
	struct jv_Value litValue;
	};
struct lx_Lexem {
	char kind;
	union lx_Details details;
	};

int lx_mayStartSymbol (char ch) {
	return isalpha(ch) || ch == '_';
	}
int lx_mayBeInSymbol (char ch) {
	return lx_mayStartSymbol (ch) || isdigit(ch);
	}

char lx_nextLexem (struct lx_Lexem *dest) {
	int firstCh, ch;
	unsigned int strLength, strCapacity;
	char *nameBegin, *namePtr, *nameLimit, warnLong, *strChars;
	char octal1, octal0, *strExpanded;
	long int numericVal;
	struct jv_String *strHeader;

	assert(dest != NULL);
	lx_whitespace ();
	switch (firstCh = lx_nextChr ()) {
#define LX_SINGLE(C,K) case C:  return dest->kind = K;
	LX_SINGLE(EOF, LX_EOF)
	LX_SINGLE('{', LX_COMPOUND)
	LX_SINGLE('}', LX_ECOMPOUND)
	LX_SINGLE('(', LX_OBRACKET)
	LX_SINGLE(')', LX_CBRACKET)
	LX_SINGLE('[', LX_INDEX)
	LX_SINGLE(']', LX_EINDEX)
	LX_SINGLE(',', LX_COMMA)
	LX_SINGLE(';', LX_SEMICOLON)
	LX_SINGLE('+', LX_PLUS)
	LX_SINGLE('-', LX_MINUS)
	LX_SINGLE('*', LX_TIMES)
	LX_SINGLE('/', LX_DIVIDE)
	LX_SINGLE('<', LX_LESS)
	LX_SINGLE('>', LX_GREATER)
#undef LX_SINGLE
	case '=':
		if (lx_peekChr () == '=') {
			lx_nextChr ();
			return dest->kind = LX_EQUAL;
			}
		else
			return dest->kind = LX_ASSIGN;
		assert(0);
	case '"':
	case '\'':
		strLength = 0;
		if ((strChars = malloc (strCapacity = 16)) == NULL)
			eh_heapFull ();
		eh_establishTemp ();
		eh_registerTemp (strChars);
		warnLong = 1;
		while ((ch = lx_nextChr ()) != firstCh) {
			if (ch == EOF)
				eh_reportErr ("String literal never closed!",
						0);
			if (ch == '\\') {
				switch (ch = lx_nextChr ()) {
				case EOF:
					eh_unexpEnd ();
					break;
				case 'n':
					ch = 10;
					break;
				case 't':
					ch = 9;
					break;
				case '0':
				case '1':
					if ((octal1 = lx_nextChr ()) == EOF)
						eh_unexpEnd ();
					if (octal1 < '0' || octal1 > '7')
						eh_escSeqMalformed ();
					if ((octal0 = lx_nextChr ()) == EOF)
						eh_unexpEnd ();
					if (octal0 < '0' || octal0 > '7')
						eh_escSeqMalformed ();
					ch = (ch & 7) << 6
							| (octal1 & 7) << 3
							| (octal0 & 7);
					break;
				case '"':
				case '\'':
				case '\\':
					break;
				default:
					eh_escSeqMalformed ();
					}
				}
			assert(ch >= 0);
			assert(ch <= 127);
			if (strLength < JV_STRLENGTH) { 
				if (strLength >= strCapacity) {
					if ((strExpanded = realloc (strChars,
							strCapacity += 16
							)) == NULL)
						eh_heapFull ();
					eh_leaveTemp ();
					eh_establishTemp ();
					eh_registerTemp (strChars =
							strExpanded);
					}
				assert(strChars != NULL);
				assert(strLength < strCapacity);
				strChars[strLength++] = ch;
				}
			else if (warnLong) {
				eh_reportErr ("String literal too long!", 1);
				warnLong = 0;
				}
			}
		if ((strHeader = malloc (sizeof (struct jv_String))) == NULL)
			eh_heapFull ();
		strHeader->chars = strChars;
		strHeader->length = strLength;
		strHeader->refcount = 1;
		dest->details.litValue.type = JV_STRING;
		dest->details.litValue.content.strContent = strHeader;
		eh_leaveTemp ();
		return dest->kind = LX_LITERAL;
		}
	if (lx_mayStartSymbol (firstCh)) {
		*(nameBegin = namePtr = dest->details.symName) = firstCh;
		nameLimit = namePtr + 30;
		warnLong = 1;
		while ((ch = lx_peekChr ()) != EOF && lx_mayBeInSymbol (ch)) {
			lx_nextChr ();
			if (namePtr < nameLimit)
				*++namePtr = ch;
			else if (warnLong) {
				eh_reportErr ("Name too long!", 1);
				warnLong = 0;
				}
			}
		assert(namePtr < dest->details.symName + 32);
		*++namePtr = 0;
		if (!(strcmp (nameBegin, "var") && strcmp (nameBegin, "let")))
			return dest->kind = LX_VAR;
#define LX_KEYWORD(W,K) \
		if (!strcmp (nameBegin, W))\
			return dest->kind = K;
		LX_KEYWORD("if", LX_IF)
		LX_KEYWORD("while", LX_WHILE)
		LX_KEYWORD("function", LX_FUNCTION)
		LX_KEYWORD("return", LX_RETURN)
		LX_KEYWORD("new", LX_NEW)
#undef LX_KEYWORD
		if (!strcmp (nameBegin, "true")) {
			jv_setBoolean (&dest->details.litValue, 1);
			return dest->kind = LX_LITERAL;
			}
		if (!strcmp (nameBegin, "false")) {
			jv_setBoolean (&dest->details.litValue, 0);
			return dest->kind = LX_LITERAL;
			}
		if (!strcmp (nameBegin, "undefined")) {
			jv_setUndefined (&dest->details.litValue);
			return dest->kind = LX_LITERAL;
			}
		return dest->kind = LX_SYMBOL;
		}
	if (isdigit(firstCh)) {
		numericVal = firstCh - '0';
		assert(numericVal >= 0);
		assert(numericVal <= 9);
		while ((ch = lx_nextChr ()) != EOF && isdigit(ch)) {
			ch -= '0';
			assert(ch >= 0);
			assert(ch <= 9);
			if ((JV_NMAX - ch) / 10 < numericVal)
				eh_reportErr ("Too big number literal!", 0);
			numericVal = numericVal * 10 + ch;
			}
		lx_pushChr (ch);
		assert(numericVal >= 0);
		jv_setNumber (&dest->details.litValue, numericVal);
		return dest->kind = LX_LITERAL;
		}
	eh_syntax ();
	}

void lx_dropLexem (struct lx_Lexem *lxm) {
	assert(lxm != NULL);
	if (lxm->kind == LX_LITERAL)
		jv_unset (&lxm->details.litValue);
	}

#endif