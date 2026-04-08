/*INTERPRETATOR LIGHTSCRIPT, OKROJONEGO JAVASCRIPT - Jan Mleczko.
  Modu³ JV - JavaScript Values - wszystkie dzia³ania na "wartoœciach
  javascriptowych"
*/
#ifndef JSVALUES_H
#define JSVALUES_H

#define JV_NUMBER 1  /*typy wartoœci LightScript*/
#define JV_STRING 2
#define JV_BOOLEAN 3
#define JV_OBJECT 4
#define JV_UNDEFINED 5

#define JV_NMAX 2147483647  /*zakres typu Number*/
#define JV_NMIN (-JV_NMAX)
#define JV_STRLENGTH 65535  /*dopuszczalna d³ugoœæ ³añcucha typu String*/

struct jv_String {
	char *chars;
	unsigned short int length;
	unsigned long int refcount;
	};

struct jv_Object {
	struct di_Dictionary entries;
	unsigned long int refcount;
	struct jv_Object **linkInChain, *nextInChain, *nextToUnlink;
	};

union jv_Content {
	long int numContent;
	char boolContent;
	struct jv_String *strContent;
	struct jv_Object *objContent;
	};

struct jv_Value {
	char type;
	union jv_Content content;
	};

struct jv_Object *jv_chainRoot;

void jv_init () {
	jv_chainRoot = NULL;
	}

void jv_unlinkSimple (struct jv_Value *val) {
	struct jv_String *s;

	assert(val != NULL);
	if (val->type == JV_STRING) {
		s = val->content.strContent;
		assert(s != NULL);
		assert(s->refcount >= 1);
		if (!--s->refcount) {
			free (s->chars);
			free (s);
			}
		}
	}	

void jv_cleanup () {
	struct jv_Object *current, *next;
	struct di_Dictionary *fieldDict;
	struct di_Iteration fieldItr;
	struct jv_Value *objField;

	DG_TRACE(wejscie do jv_cleanup)
#ifndef NDEBUG
	printf ("jv_chainRoot=%p\n", jv_chainRoot);
#endif
	current = jv_chainRoot;
	while (current != NULL) {
		next = current->nextInChain;
		di_open (fieldDict = &current->entries, &fieldItr);
		while ((objField = di_next (&fieldItr)) != NULL) {
			jv_unlinkSimple (objField);
			free (objField);
			}
		di_drop (fieldDict);
		free (current);
		current = next;
		}
	}

void jv_enchainObject (struct jv_Object *obj) {
	assert(obj != NULL);
	obj->nextInChain = jv_chainRoot;
	obj->linkInChain = &jv_chainRoot;
	if (jv_chainRoot != NULL)
		jv_chainRoot->linkInChain = &obj->nextInChain;
	jv_chainRoot = obj;
	}

void jv_dechainObject (struct jv_Object *obj) {
	assert(obj != NULL);
	if (obj->nextInChain != NULL)
		obj->nextInChain->linkInChain = obj->linkInChain;
	*obj->linkInChain = obj->nextInChain;
	}

void jv_setUndefined (struct jv_Value *val) {
	assert(val != NULL);
	val->type = JV_UNDEFINED;
	}

void jv_setNumber (struct jv_Value *val, long int number) {
	assert(val != NULL);
	assert(number >= JV_NMIN);
	assert(number <= JV_NMAX);
	val->type = JV_NUMBER;
	val->content.numContent = number;
	}

void jv_setBoolean (struct jv_Value *val, int p) {
	assert(val != NULL);
	val->type = JV_BOOLEAN;
	val->content.boolContent = p ? 1 : 0;
	}

void jv_setString (struct jv_Value *val, char *string) {
	unsigned int strLength;
	char *sChars;
	struct jv_String *sBlock;

	assert(val != NULL);
	assert(string != NULL);
	if ((strLength = strlen (string)) > JV_STRLENGTH)
		strLength = JV_STRLENGTH;
	if ((sBlock = malloc (sizeof (struct jv_String))) == NULL)
		eh_heapFull ();
	if (strLength) {
		if ((sChars = malloc (strLength)) == NULL) {
			free (sBlock);
			eh_heapFull ();
			}
		memcpy (sChars, string, strLength);
		}
	else
		sChars = NULL;
	sBlock->chars = sChars;
	sBlock->length = strLength;
	sBlock->refcount = 1;
	val->type = JV_STRING;
	val->content.strContent = sBlock;
	}

void jv_regTmpString (struct jv_String *str) {
	char *chs;

	assert(str != NULL);
	eh_uniqueTemp (str);
	if ((chs = str->chars) != NULL)
		eh_uniqueTemp (chs);
	}

void jv_setObject (struct jv_Value *val) {
	struct jv_Object *obj;

	if ((obj = malloc (sizeof (struct jv_Object))) == NULL)
		eh_heapFull ();
	di_init (&obj->entries);
	obj->refcount = 1;
	jv_enchainObject (obj);
	val->type = JV_OBJECT;
	val->content.objContent = obj;
	}

void jv_unlinkObject (struct jv_Object *firstToUnlink) {
	struct jv_Object *current, *anothObj;
	struct di_Iteration iter;
	struct jv_Value *another;

	DG_TRACE(wejscie do jv_unlinkObject)
	assert(firstToUnlink != NULL);
	assert(firstToUnlink->refcount >= 1);
	if (--firstToUnlink->refcount)
		return;
	firstToUnlink->nextToUnlink = NULL;
	do {
		current = firstToUnlink;
		firstToUnlink = firstToUnlink->nextToUnlink;
		assert(current != NULL);
		assert(current->refcount == 0);
		di_open (&current->entries, &iter);
		while ((another = di_next (&iter)) != NULL) {
			if (another->type == JV_OBJECT) {
				anothObj = another->content.objContent;
				assert(anothObj != NULL);
				assert(anothObj->refcount >= 1);
				if (!--anothObj->refcount) {
					anothObj->nextToUnlink = firstToUnlink;
					firstToUnlink = anothObj;
					DG_TRACE(dodano obiekt do kolejki)
					}
				}
			else
				jv_unlinkSimple (another);
			free (another);
			}
		jv_dechainObject (current);
		di_drop (&current->entries);
		free (current);
		} while (firstToUnlink != NULL);
	DG_TRACE(wyjscie z jv_unlinkObject)
	}

void jv_unset (struct jv_Value *val) {
	assert(val != NULL);
	if (val->type == JV_OBJECT)
		jv_unlinkObject (val->content.objContent);
	else
		jv_unlinkSimple (val);
	}

void jv_assign (struct jv_Value *dest, struct jv_Value *src) {
	unsigned long int *refcntField;

	assert(dest != NULL);
	assert(src != NULL);
	assert(dest != src);
	switch (src->type) {
	case JV_STRING:
		assert(src->content.strContent != NULL);
		if (*(
				refcntField =
				&src->content.strContent->refcount
				) >= ULONG_MAX)
			eh_reportErr ("Too many references to a string!", 0);
		++*refcntField;
		break;
	case JV_OBJECT:
		assert(src->content.objContent != NULL);
		if (*(
				refcntField =
				&src->content.objContent->refcount
				) >= ULONG_MAX)
			eh_reportErr ("Too many references to an object!", 0);
		++*refcntField;
#ifndef NDEBUG
		printf ("** przypisanie obiektu %p, refcount %lu.\n",
				src->content.objContent, *refcntField);
#endif
		}
	*dest = *src;
	}

void jv_toNumber (struct jv_Value *dest, struct jv_Value *src) {
	char digit, *pdigit, *end;
	struct jv_String *strHeader;
	long int result;

	assert(dest != NULL);
	assert(src != NULL);
	assert(dest != src);
	dest->type = JV_NUMBER;
	switch (src->type) {
	case JV_NUMBER:
		dest->content = src->content;
		break;
	case JV_BOOLEAN:
assert(src->content.boolContent == 0 || src->content.boolContent == 1);
		dest->content.numContent = src->content.boolContent ? 1L : 0L;
		break;
	case JV_STRING:
		strHeader = src->content.strContent;
		assert(strHeader != NULL);
		assert(strHeader->refcount > 0);
		pdigit = strHeader->chars;
		end = pdigit + strHeader->length;
		result = 0;
		assert(pdigit != NULL);
		while (pdigit < end) {
			digit = *pdigit++;
			if (isdigit(digit)) {
				digit -= '0';
				assert(digit >= 0);
				assert(digit <= 9);
				if (result > (JV_NMAX - digit) / 10) {
					result = JV_NMAX;
					break;
					}
				result = result * 10 + digit;
				}
			else
				break;
			}
		dest->content.numContent = result;
		break;
	case JV_OBJECT:
		assert(src->content.objContent != NULL);
		assert(src->content.objContent->refcount > 0);
		dest->content.numContent = 1L;
		break;
	case JV_UNDEFINED:
		dest->content.numContent = 0L;
		}
	assert(dest->content.numContent >= JV_NMIN);
	assert(dest->content.numContent <= JV_NMAX);
	}

void jv_toBoolean (struct jv_Value *dest, struct jv_Value *src) {
	assert(dest != NULL);
	assert(src != NULL);
	assert(dest != src);
	dest->type = JV_BOOLEAN;
	switch (src->type) {
	case JV_NUMBER:
		assert(src->content.numContent >= JV_NMIN);
		assert(src->content.numContent <= JV_NMAX);
		dest->content.boolContent = src->content.numContent != 0;
		break;
	case JV_BOOLEAN:
		dest->content = src->content;
		break;
	case JV_STRING:
		assert(src->content.strContent != NULL);
		dest->content.boolContent
				= src->content.strContent->length > 0;
		break;
	case JV_OBJECT:
		dest->content.boolContent = 1;
		break;
	case JV_UNDEFINED:
		dest->content.boolContent = 0;
		}
assert(dest->content.boolContent == 0 || dest->content.boolContent == 1);
	}

void jv_toString (struct jv_Value *dest, struct jv_Value *src) {
	char decimal[64];

	assert(dest != NULL);
	assert(src != NULL);
	assert(dest != src);
	switch (src->type) {
	case JV_NUMBER:
		assert(src->content.numContent >= JV_NMIN);
		assert(src->content.numContent <= JV_NMAX);
		sprintf (decimal, "%ld", src->content.numContent);
		jv_setString (dest, decimal);
		break;
	case JV_BOOLEAN:
assert(src->content.boolContent == 0 || src->content.boolContent == 1);
		jv_setString (dest, src->content.boolContent
				? "true" : "false");
		break;
	case JV_STRING:
		jv_assign (dest, src);
		break;
	case JV_OBJECT:
		jv_setString (dest, "[object Object]");
		break;
	case JV_UNDEFINED:
		jv_setString (dest, "undefined");
		}
	assert(dest->type == JV_STRING);
	assert(dest->content.strContent != NULL);
	assert(dest->content.strContent->refcount > 0);
	}

void jv_add (struct jv_Value *dest, struct jv_Value *left,
		struct jv_Value *right) {
	char typeLeft, typeRight;
	unsigned short int ltLength, rtLength, fitting, resLength;
	long int valLeft, valRight;
	struct jv_Value castedLeft, castedRight;
	struct jv_String *resHeader;
	char *resChars;

	assert(dest != NULL);
	assert(left != NULL);
	assert(right != NULL);
	assert(dest != left);
	assert(dest != right);
	typeLeft = left->type;
	typeRight = right->type;
	if ((typeLeft == JV_NUMBER || typeLeft == JV_BOOLEAN)
	&& (typeRight == JV_NUMBER || typeRight == JV_BOOLEAN)) {
		jv_toNumber (&castedLeft, left);
		jv_toNumber (&castedRight, right);
		valLeft = castedLeft.content.numContent;
		valRight = castedRight.content.numContent;
		dest->type = JV_NUMBER;
		if (valLeft > 0 && valRight > 0
		&& JV_NMAX - valRight < valLeft) {
			eh_overflow ();
			dest->content.numContent = JV_NMAX;
			}
		else if (valLeft < 0 && valRight < 0
		&& JV_NMIN - valRight > valLeft) {
			eh_overflow ();
			dest->content.numContent = JV_NMIN;
			}
		else
			dest->content.numContent = valLeft + valRight;
		assert(dest->content.numContent >= JV_NMIN);
		assert(dest->content.numContent <= JV_NMAX);
		}
	else {
		eh_establishTemp ();
		jv_toString (&castedLeft, left);
		jv_regTmpString (castedLeft.content.strContent);
		jv_toString (&castedRight, right);
		jv_regTmpString (castedRight.content.strContent);
		ltLength = castedLeft.content.strContent->length;
		rtLength = castedRight.content.strContent->length;
		assert(ltLength <= JV_STRLENGTH);
		assert(rtLength <= JV_STRLENGTH);
		if ((fitting = JV_STRLENGTH - ltLength) < rtLength) {
			eh_reportErr ("String concatenation overflow!", 1);
			rtLength = fitting;
			}
		if ((resHeader = malloc (sizeof (struct jv_String))) == NULL)
			eh_heapFull ();
		eh_registerTemp (resHeader);
		if ((resChars = malloc (
				resLength = ltLength + rtLength
				)) == NULL)
			eh_heapFull ();
		resHeader->chars = resChars;
		resHeader->length = resLength;
		resHeader->refcount = 1;
		assert(resLength <= JV_STRLENGTH);
		assert(ltLength + rtLength == resLength);
		memcpy (resChars, castedLeft.content.strContent->chars,
				ltLength);
		memcpy (resChars + ltLength,
				castedRight.content.strContent->chars,
				rtLength);
		dest->type = JV_STRING;
		dest->content.strContent = resHeader;
		eh_leaveTemp ();
		}
	jv_unset (&castedLeft);
	jv_unset (&castedRight);
	}

void jv_subtract (struct jv_Value *dest, struct jv_Value *left,
		struct jv_Value *right) {
	struct jv_Value castedLeft, castedRight;
	long int valLeft, valRight;

	assert(dest != NULL);
	assert(left != NULL);
	assert(right != NULL);
	assert(dest != left);
	assert(dest != right);
	jv_toNumber (&castedLeft, left);
	valLeft = castedLeft.content.numContent;
	jv_toNumber (&castedRight, right);
	valRight = castedRight.content.numContent;
	dest->type = JV_NUMBER;
	if (valLeft < 0 && valRight > 0 && JV_NMIN + valRight > valLeft) {
		eh_overflow ();
		dest->content.numContent = JV_NMIN;
		}
	else if (valLeft > 0 && valRight < 0 && JV_NMAX + valRight < valLeft) {
		eh_overflow ();
		dest->content.numContent = JV_NMAX;
		}
	else
		dest->content.numContent = valLeft - valRight;
	assert(dest->content.numContent >= JV_NMIN);
	assert(dest->content.numContent <= JV_NMAX);
	jv_unset (&castedLeft);
	jv_unset (&castedRight);
	}

void jv_multiply (struct jv_Value *dest, struct jv_Value *left,
		struct jv_Value *right) {
	struct jv_Value castedLeft, castedRight;
	long int valLeft, valRight;

	assert(dest != NULL);
	assert(left != NULL);
	assert(right != NULL);
	assert(dest != left);
	assert(dest != right);
	jv_toNumber (&castedLeft, left);
	valLeft = castedLeft.content.numContent;
	jv_toNumber (&castedRight, right);
	valRight = castedRight.content.numContent;
	assert(valLeft >= JV_NMIN);
	assert(valLeft <= JV_NMAX);
	assert(valRight >= JV_NMIN);
	assert(valRight <= JV_NMAX);
	dest->type = JV_NUMBER;
	if (valRight && JV_NMAX / labs (valRight) < labs (valLeft)) {
		eh_overflow ();
		dest->content.numContent =
				valLeft > 0 == valRight > 0
				? JV_NMAX : JV_NMIN;
		}
	else
		dest->content.numContent = valLeft * valRight;
	assert(dest->content.numContent >= JV_NMIN);
	assert(dest->content.numContent <= JV_NMAX);
	jv_unset (&castedRight);
	jv_unset (&castedLeft);
	}

void jv_divide (struct jv_Value *dest, struct jv_Value *left,
		struct jv_Value *right) {
	struct jv_Value castedLeft, castedRight;
	long int divisor;

	assert(dest != NULL);
	assert(left != NULL);
	assert(right != NULL);
	assert(dest != left);
	assert(dest != right);
	jv_toNumber (&castedLeft, left);
	jv_toNumber (&castedRight, right);
	if (!(divisor = castedRight.content.numContent))
		eh_reportErr ("Division by zero!", 0);
	assert(castedLeft.content.numContent >= JV_NMIN);
	assert(castedLeft.content.numContent <= JV_NMAX);
	assert(divisor >= JV_NMIN);
	assert(divisor <= JV_NMAX);
	assert(divisor != 0);
	dest->type = JV_NUMBER;
	dest->content.numContent = castedLeft.content.numContent / divisor;
	assert(dest->content.numContent >= JV_NMIN);
	assert(dest->content.numContent <= JV_NMAX);
	jv_unset (&castedLeft);
	jv_unset (&castedRight);
	}

struct jv_Value *jv_index (struct jv_Value *nonLvalDest, struct jv_Value *left,
		struct jv_Value *right) {
	char *objKey, *strChars;
	unsigned int objKSize, pastIndex;
	struct jv_Value castedLeft, castedRight, *place;
	struct di_Dictionary *entDict;
	struct jv_String *strHeader;
	long int numericIndex;

	assert(left != NULL);
	assert(right != NULL);
	assert(nonLvalDest != NULL);
	assert(nonLvalDest != left);
	assert(nonLvalDest != right);
	eh_establishTemp ();
	if (left->type == JV_OBJECT) {
		jv_toString (&castedRight, right);
		jv_regTmpString (castedRight.content.strContent);
		objKey = castedRight.content.strContent->chars;
		objKSize = castedRight.content.strContent->length;
		if ((place = di_find (
				entDict = &left->content.objContent->entries,
				objKey, objKSize)) == NULL) {
			eh_establishTemp ();
			if ((place = malloc (sizeof (struct jv_Value)))
					== NULL)
				eh_heapFull ();
			eh_registerTemp (place);
			*di_insert (entDict, objKey, objKSize) = place;
			eh_leaveTemp ();
			jv_setUndefined (place);
			}
		jv_assign (nonLvalDest, place);
		}
	else {
		jv_toString (&castedLeft, left);
		jv_regTmpString (castedLeft.content.strContent);
		jv_toNumber (&castedRight, right);
		numericIndex = castedRight.content.numContent;
		pastIndex = castedLeft.content.strContent->length;
		if (numericIndex < 0 || numericIndex >= pastIndex)
			jv_setUndefined (nonLvalDest);
		else {
			if ((strChars = malloc (1)) == NULL)
				eh_heapFull ();
			eh_registerTemp (strChars);
			*strChars = castedLeft.content.strContent
					->chars[numericIndex];
			if ((strHeader = malloc (sizeof (struct jv_String)))
					== NULL)
				eh_heapFull ();
			strHeader->chars = strChars;
			strHeader->length = 1;
			strHeader->refcount = 1;
			nonLvalDest->type = JV_STRING;
			nonLvalDest->content.strContent = strHeader;
			}
		place = NULL;
		jv_unset (&castedLeft);
		}
	eh_leaveTemp ();
	jv_unset (&castedRight);
	return place;
	}

#define JV_LESS 1
#define JV_EQUAL 2
#define JV_GREATER 4
#define JV_UNCOMPARABLE 0

int jv_compare (struct jv_Value *left, struct jv_Value *right) {
	struct jv_Value casted1, casted2;
	struct jv_Value *leftPrepared, *rightPrepared, *exchange;
	int res;

	assert(left != NULL);
	assert(right != NULL);
	if (left->type > right->type) {
		exchange = left;
		left = right;
		right = exchange;
		}
	else
		exchange = NULL;
	switch (left->type) {
	case JV_NUMBER:
		switch (right->type) {
		case JV_NUMBER:
			leftPrepared = left;
			rightPrepared = right;
			break;
		case JV_STRING:
			jv_toNumber (&casted2, right);
			leftPrepared = left;
			rightPrepared = &casted2;
			break;
		case JV_BOOLEAN:
			jv_toNumber (&casted2, right);
			leftPrepared = left;
			rightPrepared = &casted2;
			break;
		case JV_OBJECT:
		case JV_UNDEFINED:
			return JV_UNCOMPARABLE;
			}
		break;
	case JV_STRING:
		assert(right->type != JV_NUMBER);
		switch (right->type) {
		case JV_STRING:
			leftPrepared = left;
			rightPrepared = right;
			break;
		case JV_BOOLEAN:
			jv_toNumber (&casted2, left);
			jv_toBoolean (&casted1, &casted2);
			jv_unset (&casted2);
			leftPrepared = &casted1;
			rightPrepared = right;
			break;
		case JV_OBJECT:
			jv_toString (&casted2, right);
			leftPrepared = left;
			rightPrepared = &casted2;
			break;
		case JV_UNDEFINED:
			return JV_UNCOMPARABLE;
			}
		break;
	case JV_BOOLEAN:
		assert(right->type != JV_NUMBER);
		assert(right->type != JV_STRING);
		if (right->type != JV_BOOLEAN)
			return JV_UNCOMPARABLE;
		leftPrepared = left;
		rightPrepared = right;
		break;
	case JV_OBJECT:
		assert(right->type==JV_OBJECT || right->type==JV_UNDEFINED);
		if (right->type != JV_OBJECT)
			return JV_UNCOMPARABLE;
		leftPrepared = left;
		rightPrepared = right;
		break;
	default:
		assert(left->type == JV_UNDEFINED);
		assert(right->type == JV_UNDEFINED);
		leftPrepared = left;
		rightPrepared = right;
		break;
		}
	assert(leftPrepared == left || leftPrepared == &casted1);
	assert(rightPrepared == right || rightPrepared == &casted2);
	assert(leftPrepared->type == rightPrepared->type);
	if (exchange != NULL) {
		exchange = leftPrepared;
		leftPrepared = rightPrepared;
		rightPrepared = exchange;
		}
	switch (rightPrepared->type) {
	case JV_NUMBER:  {
		long int nValLeft, nValRight;

		if ((nValLeft = leftPrepared->content.numContent)
		< (nValRight = rightPrepared->content.numContent))
			res = JV_LESS;
		else if (nValLeft > nValRight)
			res = JV_GREATER;
		else
			res = JV_EQUAL;
		}  break;
	case JV_STRING: {
		struct jv_String *shLeft, *shRight;
		unsigned short int commonLength, leftLength, rightLength;

		shLeft = leftPrepared->content.strContent;
		shRight = rightPrepared->content.strContent;
		assert(shLeft != NULL);
		assert(shRight != NULL);
		commonLength = leftLength = shLeft->length;
		if ((rightLength = shRight->length) < commonLength)
			commonLength = rightLength;
		assert(leftLength == shLeft->length);
		assert(rightLength == shRight->length);
		assert(commonLength <= shLeft->length);
		assert(commonLength <= shRight->length);
		if (commonLength) {
			assert(shLeft->chars != NULL);
			assert(shRight->chars != NULL);
			res = memcmp (shLeft->chars, shRight->chars,
					commonLength);
			if (res < 0)
				res = JV_LESS;
			else if (res > 0)
				res = JV_GREATER;
			else {
				if (leftLength < rightLength)
					res = JV_LESS;
				else if (leftLength > rightLength)
					res = JV_GREATER;
				else
					res = JV_EQUAL;
				}
			}
		else {
			assert(commonLength == 0);
			assert(shLeft->length == 0 || shRight->length == 0);
			if (leftLength)
				res = JV_GREATER;
			else if (rightLength)
				res = JV_LESS;
			else
				res = JV_EQUAL;
			}
		}  break;
	case JV_BOOLEAN:  {
		char bValLeft, bValRight;

		bValLeft = leftPrepared->content.boolContent;
		bValRight = rightPrepared->content.boolContent;
		assert(bValLeft == 0 || bValLeft == 1);
		assert(bValRight == 0 || bValRight == 1);
		if (bValLeft < bValRight)
			res = JV_LESS;
		else if (bValLeft > bValRight)
			res = JV_GREATER;
		else
			res = JV_EQUAL;
		}  break;
	case JV_OBJECT:
		res =
				leftPrepared->content.objContent
				== rightPrepared->content.objContent
				? JV_EQUAL : JV_UNCOMPARABLE;
		break;
	case JV_UNDEFINED:
		res = JV_EQUAL;
		break;
		}
	if (leftPrepared == &casted1 || rightPrepared == &casted1)
		jv_unset (&casted1);
	if (leftPrepared == &casted2 || rightPrepared == &casted2)
		jv_unset (&casted2);
assert(res==JV_LESS||res==JV_GREATER||res==JV_EQUAL||res==JV_UNCOMPARABLE);
	return res;
	}

void jv_lessThan (struct jv_Value *dest, struct jv_Value *left,
		struct jv_Value *right) {
	assert(dest != NULL);
	assert(left != NULL);
	assert(right != NULL);
	assert(dest != left);
	assert(dest != right);
	jv_setBoolean (dest, jv_compare (left, right) == JV_LESS);
	}

void jv_equalTo (struct jv_Value *dest, struct jv_Value *left,
		struct jv_Value *right) {
	assert(dest != NULL);
	assert(left != NULL);
	assert(right != NULL);
	assert(dest != left);
	assert(dest != right);
	jv_setBoolean (dest, jv_compare (left, right) == JV_EQUAL);
	}

void jv_greaterThan (struct jv_Value *dest, struct jv_Value *left,
		struct jv_Value *right) {
	assert(dest != NULL);
	assert(left != NULL);
	assert(right != NULL);
	assert(dest != left);
	assert(dest != right);
	jv_setBoolean (dest, jv_compare (left, right) == JV_GREATER);
	}

#endif
