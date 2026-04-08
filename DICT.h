/*INTERPRETATOR LIGHTSCRIPT, OKROJONEGO JAVASCRIPT - Jan Mleczko.
  Modu³ DI - Dictionary - "s³ownik", struktura danych odwzorowuj¹ca ci¹gi
  znaków na wskaŸniki do ró¿nych zastosowañ.

  Ci¹gi-klucze sk³adaj¹ siê ze znaków 0-127. WskaŸniki odpowiadaj¹ce kluczom
  nie s¹ NULL. NULL oznacza brak wpisu.

  di_Dictionary - struktura s³ownika.
  di_init - inicjacja s³ownika.
  di_drop - sprz¹tanie po s³owniku.
  di_find - szukanie wpisu.
  di_insert - tworzenie wpisu.
  di_open - rozpoczêcie przegl¹dania elementów.
  di_next - kolejny element przy przegl¹daniu.
  di_Iteration - struktura stanu przegl¹dania elementów.
*/
#ifndef DICT_H
#define DICT_H

struct di_Entry {
	char *key;
	unsigned int keySize;
	void *value;
	struct di_Entry *next;
	};
	
struct di_Dictionary {
	struct di_Entry *categories[128];
	};

struct di_Iteration {
	struct di_Dictionary *dict;
	struct di_Entry *nextEntry;
	unsigned int nextCategory;
	};

void di_init (struct di_Dictionary *dict) {
	struct di_Entry **cptr, **cbegin;
	
	assert(dict != NULL);
	cptr = (cbegin = dict->categories) + 128;
	while (cptr > cbegin)
		*--cptr = NULL;
	}

void di_drop (struct di_Dictionary *dict) {
	struct di_Entry **cptr, **cbegin, *curr, *next;

	assert(dict != NULL);
	cptr = (cbegin = dict->categories) + 128;
	while (cptr > cbegin) {
		curr = *--cptr;
		while (curr != NULL) {
			next = curr->next;
			free (curr->key);
			free (curr);
			curr = next;
			}
		}
	}

int di_hash (char *key, unsigned int size) {
	unsigned int sum, cumulated;
	char *kptr;
	
	assert(key != NULL);
	kptr = key + size;
	sum = cumulated = 0;
	while (kptr > key)
		cumulated += sum += *--kptr;
	assert((int) ((sum ^ cumulated) & 127) >= 0);
	assert((int) ((sum ^ cumulated) & 127) <= 127);
	return (sum ^ cumulated) & 127;
	}
	
void *di_find (struct di_Dictionary *dict, char *rqKey,
		unsigned int rqKeySize) {
	struct di_Entry *ent;
	
	assert(dict != NULL);
	assert(rqKey != NULL);
	ent = dict->categories[di_hash (rqKey, rqKeySize)];
	while (ent != NULL) {
		assert(ent->key != NULL);
		if (ent->keySize == rqKeySize
		&& !memcmp (ent->key, rqKey, rqKeySize))
			return ent->value;
		ent = ent->next;
		}
	return NULL;
	}

void **di_insert (struct di_Dictionary *dict, char *newKey,
		unsigned int newKeySize) {
	struct di_Entry *newEnt, **link;
	char *keyCopy;

	assert(dict != NULL);
	assert(newKey != NULL);
	if ((newEnt = malloc (sizeof (struct di_Entry))) == NULL)
		eh_heapFull ();
	if ((keyCopy = malloc (newKeySize)) == NULL) {
		free (newEnt);
		eh_heapFull ();
		}
	memcpy (keyCopy, newKey, newKeySize);
	newEnt->key = keyCopy;
	newEnt->keySize = newKeySize;
	newEnt->next = *(link = dict->categories +
			di_hash (newKey, newKeySize));
	*link = newEnt;
	return &newEnt->value;
	}

void di_open (struct di_Dictionary *dict, struct di_Iteration *iter) {
	assert(dict != NULL);
	assert(iter != NULL);
	iter->dict = dict;
	iter->nextCategory = 0;
	iter->nextEntry = NULL;
	}

void *di_next (struct di_Iteration *iter) {
	struct di_Entry *entPtr;

	assert(iter != NULL);
	assert(iter->dict != NULL);
	assert(iter->nextCategory <= 128);
	entPtr = iter->nextEntry;
	while (entPtr == NULL) {
		if (iter->nextCategory >= 128)
			return NULL;
		entPtr = iter->dict->categories[iter->nextCategory++];
		}
	iter->nextEntry = entPtr->next;
	return entPtr->value;
	}

#endif