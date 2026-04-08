/*INTERPRETATOR LIGHTSCRIPT, OKROJONEGO JAVASCRIPT - Jan Mleczko.
  Modu³ EH - Error Handling - zg³aszanie i obs³uga b³êdów

  Przy zg³aszaniu b³êdów i awarii czasami zasadne jest podanie pliku, w którym
  jest b³¹d i numeru wiersza, ale nie zawsze. W komunikatach stosowana jest
  zredagowana postaæ nazwy pliku zamieniona na same wielkie litery oraz z
  wyciêtym œrodkiem œcie¿ki, gdy jest ona d³uga.

  eh_fileKnown to znacznik, czy znany jest "bie¿¹cy plik". Jeœli prawdziwy, to
  w tablicy eh_fileName jest zredagowana nazwa pliku.
  Dodatkowo, jeœli eh_line jest niezerowe, to przechowuje numer wiersza od 1.
*/
#ifndef ERRHAND_H
#define ERRHAND_H

#define EH_TEMPSIZE 15

char eh_everContinue, eh_alwaysContinue, eh_nQuiet;
char eh_fileKnown, eh_fileName[51];
unsigned long int eh_line;
void *eh_tempAreas[EH_TEMPSIZE], **eh_tempNew;

void eh_init () {
	eh_fileKnown = eh_alwaysContinue = 0;
	eh_tempNew = eh_tempAreas;
	}

void eh_cleanup () {
	void **ptr;

	assert(eh_tempNew >= eh_tempAreas);
	assert(eh_tempNew - eh_tempAreas <= EH_TEMPSIZE);
	ptr = eh_tempNew;
	while (ptr > eh_tempAreas)
		free (*--ptr);
	}

void eh_changeFile (char const *path) {
	/*Redagowanie nazwy pliku na potrzeby zg³oszenia b³êdu i jej
	zapamiêtanie niezale¿nie od pochodzenia.*/
	unsigned int pathLen;
	char ch, *iter;

	if ((pathLen = strlen (path)) <= 50)
		/*Ca³a œcie¿ka jest odpowiednio krótka.*/
		strcpy (eh_fileName, path);
	else {  /*Œcie¿ka jest d³uga.
		Zast¹pienie œrodkowej czêœci wielokropkiem.*/
		memcpy (eh_fileName, path, 22);
		memcpy (eh_fileName + 22, "(...)", 5);
		strcpy (eh_fileName + 27, path + pathLen - 23);
		}
	iter = eh_fileName;  /*Na du¿e litery.*/
	while (ch = *iter)
		*iter++ = toupper(ch);
	eh_fileKnown = 1;  /*Teraz plik jest znany.*/
	}

void eh_reportErr (char const *message, char allowContinue) {
	/*Zg³oszenie sytuacji wyj¹tkowej z komunikatem message i zakoñczenie
	programu interpretatora. Dla ma³o powa¿nych okolicznoœci mo¿na pozwoliæ
	u¿ytkownikowi na niezatrzymywanie interpretatora. Parametr
	allowContinue jest wtedy prawdziwy i funkcja zadaje pytanie, czy
	przerwaæ bieg interpretatora.

	Odbywa siê zakoñczenie funkcj¹ standardow¹ exit(). Aby zapewniæ
	wykonanie koniecznych czynnoœci koñcowych trzeba skorzystaæ
	z atexit().*/
	int ansCh1, ansCh2;

	assert(eh_nQuiet || eh_alwaysContinue || !eh_everContinue);
	if (eh_nQuiet) {
		printf ("Error");
		if (eh_fileKnown) {
			printf (" in ");
			if (eh_line)
				printf ("line %lu of ", eh_line);
			printf ("%s", eh_fileName);
			}
		printf (":\n  %s\n", message);
		}
	if (eh_everContinue && allowContinue) {
		if (eh_alwaysContinue)
			return;
		do {
			printf ("Continue? [Y,N,A,?] ");
			ansCh1 = ansCh2 = getchar ();
			while (ansCh2 != '\n' && ansCh2 != EOF)
				ansCh2 = getchar ();
			switch (toupper(ansCh1)) {
			case 'A':
				eh_alwaysContinue = 1;
			case 'Y':
				return;
			case 'N':
				allowContinue = 0;
				break;
			case '?':
				puts ("  \"Y\" as \"yes\".\n"
				"  \"N\" as \"no\".\n"
				"  \"A\" to always continue from now.\n"
				"  \"?\" for this help message.");
				break;
			default:
				puts ("Unexpected answer!");
				}
			} while (allowContinue);
		}
	exit (1);
	}

void eh_heapFull () {
	eh_reportErr ("Out of heap memory!", 0);
	}

void eh_overflow () {
	eh_reportErr ("Numeric overflow!", 1);
	}

void eh_escSeqMalformed () {
	eh_reportErr ("Malformed escape sequence in a string literal!", 0);
	}

void eh_syntax () {
	eh_reportErr ("Syntax error!", 0);
	}

void eh_unexpEnd () {
	eh_reportErr ("Unexpected end of file!", 0);
	}

void eh_exprSyntax () {
	eh_reportErr ("Expression syntax error!", 0);
	}

void eh_exprComplex () {
	eh_reportErr ("Expression too complex!", 0);
	}

void eh_tooParam () {
	eh_reportErr ("More than 8 function parameters!", 0);
	}

void eh_registerTemp (void *area) {
	assert(eh_tempNew >= eh_tempAreas);
	assert(eh_tempNew - eh_tempAreas <= EH_TEMPSIZE);
	if (area != NULL)
		*eh_tempNew++ = area;
	}

void eh_uniqueTemp (void *area) {
	void **searchPtr;

	searchPtr = eh_tempNew;
	assert(searchPtr >= eh_tempAreas);
	assert(searchPtr - eh_tempAreas <= EH_TEMPSIZE);
	while (searchPtr > eh_tempAreas) {
		if (*--searchPtr == area)
			return;
		}
	eh_registerTemp (area);
	}
	
void eh_establishTemp () {
	assert(eh_tempNew >= eh_tempAreas);
	assert(eh_tempNew - eh_tempAreas <= EH_TEMPSIZE);
	*eh_tempNew++ = NULL;
	}

void eh_leaveTemp () {
	assert(eh_tempNew > eh_tempAreas);
	assert(eh_tempNew - eh_tempAreas <= EH_TEMPSIZE);
	assert(*eh_tempAreas == NULL);
	while (*--eh_tempNew != NULL);
	}

#endif
