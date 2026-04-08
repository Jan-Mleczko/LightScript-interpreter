/*INTERPRETATOR LIGHTSCRIPT, OKROJONEGO JAVASCRIPT - Jan Mleczko.
  Modu³ DG - Debugging - definicje pomagaj¹ce w uruchamianiu, tylko na czas
  uruchomieñ próbnych i szukania b³êdów
*/

#ifndef DEBUG_H
#define DEBUG_H

#ifdef NDEBUG

#define DG_SETUP
#define DG_TRACE(MSG)
#define DG_GENERAL(MSG)
#define DG_PAUSE
#define DG_LOGHEAP

#else
/*Nie NDEBUG.*/

#define DG_GENTRC 1
#define DG_DETTRC 2
#define DG_ALLTRC 3
#define DG_HEAP 4
#define DG_PAUSES 8
#define DG_VHEAP 16
char dg_flags;
long int dg_balanceHeap;
FILE *dg_consoleIn;

#ifdef __unix__
#define DG_DIRECT "/dev/tty"
#else
#define DG_DIRECT "CON"
#endif

void dg_setup () {
	int answer;
	char notFinished;

	dg_balanceHeap = 0;
	if ((dg_consoleIn = fopen (DG_DIRECT, "r")) == NULL)
		dg_consoleIn = stdin;
	fputs ("******************************\n"
	"*** Enter debugging settings:\n"
	"***   T - trace messages\n"
	"***   G - general trace messages\n"
	"***   H - heap interaction tracking\n"
	"***   V - visible heap usage trace\n"
	"***   P - debugging pauses\n"
	"***   . - end of settings entry\n"
	"******************************\n", stderr);
	dg_flags = 0;
	notFinished = 1;
	do {
		fputc ('?', stderr);
		while ((answer = fgetc (dg_consoleIn)) != '\n') {
			switch (answer) {
			case 'T':
				dg_flags |= DG_ALLTRC;
				fputs ("*** All trace messages enabled.\n",
						stderr);
				break;
			case 'G':
				dg_flags |= DG_GENTRC;
				fputs ("*** General trace messages enabled.\n",
						stderr);
				break;
			case 'V':
				dg_flags |= DG_VHEAP;
				fputs ("*** Visible heap interaction tracking "
						"enabled.\n", stderr);
			case 'H':
				dg_flags |= DG_HEAP;
				fputs ("*** Heap interaction tracking "
						"enabled.\n", stderr);
				break;
			case 'P':
				dg_flags |= DG_PAUSES;
				fputs ("*** Debugging pauses enabled.\n",
						stderr);
				break;
			case '.':
				notFinished = 0;
				fputs ("***** Done with debugging settings.\n",
						stderr);
				break;
				}
			}
		} while (notFinished);
	}

void dg_message (char requiredFlag, char const *message) {
	if (dg_flags & requiredFlag)
		fprintf (stderr, "*** %s\n", message);
	}

void dg_pause () {
	if (dg_flags & DG_PAUSES) {
		fputs ("*** PAUSED...", stderr);
		fflush (dg_consoleIn);
		while (fgetc (dg_consoleIn) != '\n');
		}
	}

void *dg_mallocWrapper (unsigned long int size) {
	void *result;
	
	result = malloc (size);
	if (dg_flags & DG_HEAP) {
		if (result == NULL) {
			if (dg_flags & DG_VHEAP)
			  fprintf (stderr,
			  "*** FAILED TO ALLOCATE %lu BYTES.\n", size);
			}
		else {
			++dg_balanceHeap;
			if (dg_flags & DG_VHEAP) {
			  fprintf (stderr,
			  "*** SUCCEEDED TO ALLOCATE %lu BYTES."
			  " %ld heap areas occupied.\n",
			  size, dg_balanceHeap);
			  }
			}
		}
	return result;
	}

void dg_freeWrapper (void *area) {
	free (area);
	if (area != NULL && dg_flags & DG_HEAP) {
		--dg_balanceHeap;
		if (dg_flags & DG_VHEAP)
		  fprintf (stderr,
		  "*** DISPOSED ONE HEAP AREA."
		  " %ld heap areas occupied.\n", dg_balanceHeap);
		}
	}

void dg_logHeap () {
	if (dg_flags & DG_HEAP)
		fprintf (stderr,
		"*** %ld HEAP AREAS ARE OCCUPIED NOW.\n", dg_balanceHeap);
	else
		fputs ("*** HEAP TRACKING IS DISABLED!\n", stderr);
	}

#define DG_SETUP dg_setup ();
#define DG_TRACE(MSG) dg_message (DG_DETTRC, #MSG);
#define DG_GENERAL(MSG) dg_message (DG_GENTRC, #MSG);
#define DG_PAUSE dg_pause ();
#define DG_LOGHEAP dg_logHeap ();
#define malloc dg_mallocWrapper
#define free dg_freeWrapper
#define calloc abort
#endif
/*Niekoniecznie NDEBUG.*/

#endif