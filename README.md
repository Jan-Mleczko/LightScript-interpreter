## Interpretator JavaScript w wersji okrojonej - tzw. LightScript

Oto mój interpretator dalece uproszczonego JavaScript'u, zachowujący jednakże jego najistotniejsze cechy, czyli dynamiczne typowanie i wspomagane przez interpretator dynamiczne struktury danych. Powstałej w ten sposób odmianie JavaScript nadałem nazwę _LightScript_ i udostępniam również jej dokumentację.

Dokumentacja jest w pliku [LightScript.txt](https://raw.githubusercontent.com/Jan-Mleczko/LightScript-interpreter/refs/heads/main/LightScript.txt).

**Przykład działającego skryptu:**
```js
/* Imienne pozdrowienie użytkownika. */
function Osoba_utworz () {
        var instancja;  instancja = new Object ();
        instancja["znamyImie"] = false;
        return instancja;
        }
function Osoba_zapytajOImie (instancja) {
        var podaneImie;
        write ("Jak masz na imie? ");
        podaneImie = readln ();
        if (String_length (podaneImie) > 0) {
                instancja["imie"] = podaneImie;
                instancja["znamyImie"] = true;
                }
        }
function Osoba_pozdrow (instancja) {
        write ("Witaj");
        if (instancja["znamyImie"]) {
                write (", " + instancja["imie"]);
                }
        writeln ("!");
        }
var ktos;  ktos = Osoba_utworz ();
Osoba_pozdrow (ktos);
Osoba_zapytajOImie (ktos);
Osoba_pozdrow (ktos);
```

Obsługiwane elementy JavaScript:

* instrukcje `var`, `if`, `while`, `function`, `return`, instrukcja wyrażeniowa i (oddzielnie) instrukcja przypisania
* wyrażenia z operatorami `+ - * / < > == [] ()`.
* typy danych Number - liczba całkowita zamiast zmiennoprzecinkowej; Boolean; String - do 65 535 znaków 7-bitowych; Object (mogący pełnić też rolę tablicy); undefined
* literały liczbowe, logiczne, łańcuchowe i undefined
* funkcje (ale bez wskazań na funkcje, `this`, funkcji anonimowych itd. - tylko globalne funkcje wywoływane statycznie) z parametrami, wartością zwracaną, zmiennymi lokalnymi i rekurencją

Zestaw funkcji wbudowanych to `write`, `writeln`, `readln`, `String_length`, `String_charCodeAt`, `String_fromCharCode`. Jest możliwość łączenia programu z wielu plików LightScript, zatem da się tworzyć biblioteki. Działa to tak, że kilka plików interpretowanych jest po kolei, a zmienne i funkcje zdefiniowane w poprzednich zostają.

### Pobierz interpretator

Dla Windows(R) NT: [LSCRIPT.EXE, 26K](https://github.com/Jan-Mleczko/LightScript-interpreter/raw/refs/heads/main/LSCRIPT.EXE)

[Analiza VirusTotal](https://www.virustotal.com/gui/file/44033e1ee07533a4d8618d77875a7e0c35563d7ed464954cb499d78ed87e8b17/detection)

Dla 64-bitowego Windows(R): [LSCRIPT64.EXE, 31K](https://github.com/Jan-Mleczko/LightScript-interpreter/raw/refs/heads/main/LSCRIPT64.EXE)

[Analiza VirusTotal](https://www.virustotal.com/gui/file/49b33d969cf4e2d3bd4bb569ed04d31a5cf242f3a33229343acd3df32f7b6f43/detection)

To jest otwartoźródłowe oprogramowanie, więc jeśli masz inny system, boisz się ew. wirusów albo chcesz coś zmienić, możesz pobrać kod źródłówy i skompilować go we własnym zakresie.


### Kompilacja

Sam interpretator jest napisany w języku programowania C. Składa się z plików źródłowych:

* `LSCRIPT.c` - główny
* `DEBUG.h`
* `ERRHAND.h`
* `DICT.h`
* `JSVALUES.h`
* `LEXER.h`
* `OPER.h`
* `BUILTINS.h`
* `PARSER.h`

Na poziomie kodu źródłowego powinien być przenośny między różnymi komputerami, systemami operacyjnymi i kompilatorami ANSI C. Zawsze trzeba zacząć od pobrania ww. plików z repozytorium do jednego katalogu. Poniżej instrukcja dla wybranych kompilatorów.

#### Kompilacja w IDE Code::Blocks 20.03 pod Windows(R) NT - zalecane dla początkujących
1. Otworzyć plik _LSCRIPT.c_ w Code::Blocks.
2. Wybrać z menu _Settings_ / _Compiler..._.
3. Wybrać zakładkę _Compiler settings_ (jeśli nie jest wybrana domyślnie), a następnie _Other compiler options_.
4. W dużym polu tekstowym wpisać `-DNDEBUG`. Kliknąć przycisk _OK_. Okno otwarte w punkcie 2 zamknie się.
5. Kliknąć ikonkę żółtej zębatki podpisaną "Build". Ikonka zmieni barwę. Poczekać na jej powrót do normalnej barwy.
6. Można przywrócić zmienione w puntach 2-4 ustawienia do poprzedniego stanu.

Otrzymany plik wykonywalny `LSCRIPT.exe` jest gotowy do użytku. Jeśli w katalogu pojawił się jakiś pośredni plik z rozszerzeniem `o`, można go usunąć. Jeśli pominiemy kroki 2-4 skompilujemy interpretator w trybie do uruchomień próbnych i będzie on kłopotliwy w normalnym użytkowaniu.

#### Kompilacja za pomocą TinyCC pod Windows(R) NT
Wejść w wierszu poleceń do katalogu z pobranym kodem C i wydać polecenie
```
tcc -DNDEBUG lscript.c
```
otrzymując plik wykonywalny pn. `lscript.exe` gotowy do użytku. Jeśli opuścimy przełącznik `-DNDEBUG` skompilujemy interpretator w trybie do uruchomień próbnych i będzie on kłopotliwy w normalnym użytkowaniu.

Oczywiście katalog zawierający TCC.EXE musi być wymieniony w PATH albo trzeba podać ścieżkę do TCC.EXE w miejscu `tcc`.

#### Kompilacja za pomocą GCC pod Windows(R) NT
Wejść w wierszu poleceń do katalogu z pobranym kodem C i wydać polecenie
```
gcc -o lscript.exe -DNDEBUG lscript.c
```
otrzymując plik wykonywalny pn. `lscript.exe` gotowy do użytku. Jeśli opuścimy przełącznik `-DNDEBUG` skompilujemy interpretator w trybie do uruchomień próbnych i będzie on kłopotliwy w normalnym użytkowaniu.

Oczywiście katalog zawierający GCC.EXE musi być wymieniony w PATH albo trzeba podać ścieżkę do GCC.EXE w miejscu `gcc`. Np. jeśli mamy zainstalowane Dev-C++ 4.9.9.2, będzie mogła to być ścieżka _C:\dev-cpp\bin\gcc_, gdzie _C_ to nasz dysk systemowy.

#### Kompilacja za pomocą GCC pod Linux'em
Dotyczy np. GCC wbudowanego w Linux Mint. Wejść w wierszu poleceń do katalogu z pobranym kodem C i wydać polecenie
```
gcc -o lscript -DNDEBUG LSCRIPT.c
```
otrzymując plik wykonywalny pn. `lscript` bez rozszerzenia. Uruchamuiając go w Linux'ie (i in. systemach uniksopodobnych) piszemy `./lscript` w miejsce prostego `lscript`. Może byc potrzebne polecenie `chmod +x lscript` jednorazowo po kompilacji interpretatora.

#### Kompilacja w Visual Studio, tzn. za pomocą MSVC

Interpretator LightScript nie skompiluje się w Visual Studio na domyślnych ustawieniach, ale po zmianie ustawień pomyślna kompilacja jest możliwa. Wciąż jest to najmniej wypróbowany i najmniej zalecany sposób kompilacji. Należy:
1. Utworzyć całkowicie nowy projekt - "Aplikację konsoli C++".
2. Otworzy się domyślny główny plik źródłówy projektu. Kliknać prawym przyciskiem na jego zakładce w edytorze i wybrać _Otwórz folder zawierający_.
3. Do katalogu, który się otworzy, skopiować lub przenieść wszystkie pliki kodu źródłowego interpretatora.
4. Zamknąć Visual Studio pozostawiając katalog z punktu 3 otwarty.
5. Zmienić rozszerzenie głównego pliku projektu z `cpp` na `c`. NIE chodzi o główny plik interpretatora LSCRIPT.C, tylko o główny plik z perspektywy Visual Studio czyli ten, który się otworzył po utworzeniu projektu.
6. Otworzyć w Notatniku plik ustawień o nazwie takiej jak nasza nazwa projektu i rozszerzeniu VCXPROJ. Znaleźć w nim nazwę głównego pliku projektu i uaktualnić rozszerzenie z `cpp` na `c`. Nie należy "na ślepo" zamieniać wszystkich wystąpień `.cpp` na `.c` poleceniem _Zamień_ - uszkodzi to plik. Zapisać i zamknąć Notatnik.
7. Otworzyć Visual Studio i projekt.
8. Jeśli nie ma żadnej zakładki w edytorze, wybrać z menu Visual Studio _Plik_ / _Otwórz_ / _Plik..._. Wskazać nasz główny plik projektu z perspektywy Visual Studio - ten, któremu zmieniliśmy rozszerzenie.
9. Usunąć domyślny przykładowy kod - komentarze można zostawić - i zamiast niego napisać:
   ```c
   #define _CRT_SECURE_NO_WARNINGS
   #define NDEBUG
   #include "LSCRIPT.c"
   ```
10. Zapisać plik w edytorze Visual Studio.
11. Wybrać z menu _Projekt_ / _[nazwa projektu] właściwości_, a w oknie które się pojawi _C/C++_ / _Ogólne_.
12. Znaleźć ustawienie _Sprawdzanie SDL_ z pogrubionym napisem _Tak (/sdl)_. Jest to lista rozwijana. Zmienić na _Nie (/sdl-)_.
13. Kliknąć przycisk _OK_. Okno ustawień się zamknie.
14. Kliknąć niewypełnioną ikonkę "play" podpisaną "Uruchom bez debugowania".
15. Jeśli pojawi się okno tekstowe z tekstem pomocy interpretatora LightScript, kompilacja się udała. Gotowy plik wykonywalny jest gdzieś w katalogu projektu, np. w katalogu _C:\Users\\\[twoja nazwa użytkownika w Windows(R) NT]\source\repos\\\[nazwa projektu]\\\[nazwa projektu]\x64\Debug_.

---
### Wywoływanie skompilowanego interpretatora

Mając skompilowany interpretator możemy uruchamiać programy w LightScript w wierszu poleceń:
```
>lscript
LIGHTSCRIPT INTERPRETER - Jan Mleczko, Poland, 2026. Version 1.
Command line syntax:
  LSCRIPT <script filename> [options]
Multiple files are allowed.
Options are:
  /A to always continue after an error if possible.
  /V to never continue after an error.
  /Q for "quiet" mode. Needs either /A or /V and implies /V if not specified.
  /? for help.
For example:
  LSCRIPT EXAMPLE.LS PART2.LS /A

>copy con hello.js
writeln ("Hello, world!");
^Z
Liczba skopiowanych plików:         1.

>lscript hello.js
LIGHTSCRIPT INTERPRETER - Jan Mleczko, Poland, 2026. Version 1.
Hello, world!

>_
```
