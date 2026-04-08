/*INTERPRETATOR LIGHTSCRIPT, OKROJONEGO JAVASCRIPT - Jan Mleczko.
  Modu³ BU - Built-in Functions - funkcje wbudowane.
*/
#ifndef BUILTINS_H
#define BUILTINS_H

void bu_write () {
	struct jv_Value *asString, casted;
	struct op_Stack *strToWrite;
	struct jv_String *strHeader;
	char *outpPtr, *outpEnd, outpChr;

	eh_establishTemp ();
	op_regTmpStackItem (strToWrite = op_popFromStack ());
	if ((asString = &strToWrite->value)->type != JV_STRING) {
		jv_toString (&casted, asString);
		asString = &casted;
		}
	outpPtr = (strHeader = asString->content.strContent)->chars;
	outpEnd = outpPtr + strHeader->length;
	while (outpPtr < outpEnd) {
		if ((outpChr = *outpPtr++) == 10)
			putchar ('\n');
		else
			putchar (outpChr);
		}
	if (asString == &casted)
		jv_unset (&casted);
	op_dropStackItem (strToWrite);
	eh_leaveTemp ();
	op_pushUndefined ();
	}

void bu_writeln () {
	bu_write ();
	putchar ('\n');
	}

void bu_readln () {
	struct jv_String *strHeader;
	struct op_Stack *pushed;
	char *strChars, *expanded;
	unsigned int strLength, strCapacity;
	int ch;

	if ((strChars = malloc (strCapacity = 64)) == NULL)
		eh_heapFull ();
	strLength = 0;
	while ((ch = getchar ()) != '\n' && ch != EOF) {
		if (strLength < JV_STRLENGTH) {
			if (strLength >= strCapacity) {
				if ((expanded = realloc (strChars,
						strCapacity += 16)) == NULL) {
					free (strChars);
					eh_heapFull ();
					}
				strChars = expanded;
				}
			assert(strLength < strCapacity);
			if (ch & 128)
				ch = '?';
			assert(ch >= 0);
			assert(ch <= 127);
			strChars[strLength++] = ch;
			}
		}
	if ((strHeader = malloc (sizeof (struct jv_String))) == NULL) {
		free (strChars);
		eh_heapFull ();
		}
	strHeader->chars = strChars;
	strHeader->length = strLength;
	strHeader->refcount = 1;
	eh_establishTemp ();
	jv_regTmpString (strHeader);
	(pushed = op_pushToStack ())->lvalue = NULL;
	pushed->value.type = JV_STRING;
	pushed->value.content.strContent = strHeader;
	eh_leaveTemp ();
	}

void bu_strLength () {
	struct op_Stack *popd, *pushed;
	struct jv_Value casted;

	assert(JV_STRLENGTH <= JV_NMAX);
	eh_establishTemp ();
	op_regTmpStackItem (popd = op_popFromStack ());
	jv_toString (&casted, &popd->value);
	jv_regTmpString (casted.content.strContent);
	(pushed = op_pushToStack ())->lvalue = NULL;
	jv_setNumber (&pushed->value, casted.content.strContent->length);
	jv_unset (&casted);
	op_dropStackItem (popd);
	eh_leaveTemp ();
	}

void bu_codeAt () {
	struct op_Stack *popd1, *popd2, *pushedRes;
	struct jv_Value strJV, idxJV;
	unsigned int length;

	eh_establishTemp ();
	op_regTmpStackItem (popd1 = op_popFromStack ());
	op_regTmpStackItem (popd2 = op_popFromStack ());
	jv_toString (&strJV, &popd2->value);
	jv_regTmpString (strJV.content.strContent);
	jv_toNumber (&idxJV, &popd1->value);
	length = strJV.content.strContent->length;
	assert(length >= 0);
	assert(length <= JV_STRLENGTH);
	(pushedRes = op_pushToStack ())->lvalue = NULL;
	jv_setNumber (&pushedRes->value,
		idxJV.content.numContent >= 0
				&& idxJV.content.numContent < length
		? strJV.content.strContent->chars[idxJV.content.numContent]
		: 0
		);
	jv_unset (&strJV);
	jv_unset (&idxJV);
	op_dropStackItem (popd1);
	op_dropStackItem (popd2);
	eh_leaveTemp ();
	}

void bu_fromCode () {
	struct op_Stack *popd, *pushedRes;
	struct jv_String *strHeader;
	struct jv_Value casted;
	long int chCode;
	char *strActual;

	eh_establishTemp ();
	op_regTmpStackItem (popd = op_popFromStack ());
	jv_toNumber (&casted, &popd->value);
	if ((strHeader = malloc (sizeof (struct jv_String))) == NULL)
		eh_heapFull ();
	eh_registerTemp (strHeader);
	strHeader->refcount = 1;
	chCode = casted.content.numContent;
	if (chCode >= 0 && chCode <= 127) {
		if ((strActual = malloc (1)) == NULL)
			eh_heapFull ();
		eh_registerTemp (strActual);
		*(strHeader->chars = strActual) = chCode;
		strHeader->length = 1;
		}
	else {
		strHeader->chars = NULL;
		strHeader->length = 0;
		}
	(pushedRes = op_pushToStack ())->lvalue = NULL;
	pushedRes->value.type = JV_STRING;
	pushedRes->value.content.strContent = strHeader;
	jv_unset (&casted);
	eh_leaveTemp ();
	}

void bu_init () {
	struct jv_Value *verObj, *verLV, verKey, previous;

	eh_establishTemp ();
	if ((verObj = malloc (sizeof (struct jv_Value))) == NULL)
		eh_heapFull ();
	eh_registerTemp (verObj);
	jv_setObject (verObj);
	jv_setString (&verKey, "LIGHTSCRIPT_VER");
	jv_regTmpString (verKey.content.strContent);
	jv_unset (verLV = jv_index (&previous, verObj, &verKey));
	jv_unset (&previous);
	jv_setNumber (verLV, 1);
	*di_insert (&op_global.variables, "window", 6) = verObj;
	jv_unset (&verKey);
	eh_leaveTemp ();
	op_defineSpecFunc ("write", bu_write, 1);
	op_defineSpecFunc ("piszTekst", bu_write, 1);
	op_defineSpecFunc ("writeln", bu_writeln, 1);
	op_defineSpecFunc ("piszWiersz", bu_writeln, 1);
	op_defineSpecFunc ("readln", bu_readln, 0);
	op_defineSpecFunc ("czytajWiersz", bu_readln, 0);
	op_defineSpecFunc ("String_length", bu_strLength, 1);
	op_defineSpecFunc ("String_charCodeAt", bu_codeAt, 2);
	op_defineSpecFunc ("String_fromCharCode", bu_fromCode, 1);
	}

#endif