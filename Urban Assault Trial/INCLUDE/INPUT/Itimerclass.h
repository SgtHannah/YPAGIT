#ifndef INPUT_ITIMERCLASS_H
#define INPUT_ITIMERCLASS_H
/*
**  $Source: PRG:VFM/Include/input/itimerclass.h,v $
**  $Revision: 38.2 $
**  $Date: 1996/02/19 19:44:49 $
**  $Locker:  $
**  $Author: floh $
**
**  Die itimer.class ist Superklasse aller Timesource-Treiber-
**  Klassen.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef NUCLEUS_NUCLEUS2_H
#include "nucleus/nucleus2.h"
#endif

#ifndef NUCLEUS_NUCLEUSCLASS_H
#include "nucleus/nucleusclass.h"
#endif  

/*-------------------------------------------------------------------
**  NAME
**      itimer.class -- Superklasse aller Timesource-Treiber-Klassen.
**                      Subklasse der nucleus.class.
**
**  FUNCTION
**      Dient als Superklasse aller Klassen, die devicespezifische
**      Timing-Information bereitstellen. Die Zeitbasis ist
**      1/1000 sec, allerdings kann die reale Genauigkeit des
**      konkreten Timer-Objekts erheblich abweichen.
**      Ein Timer-Objekt stellt eine Methode bereit, mit
**      denen die Zeit zwischen zwei Aufrufen der Methode
**      festgestellt werden kann, ist aber auch in der
**      Lage, eine beliebige Menge Timer-Jobs zu verwalten,
**      das sind Routinen, die in Zeitintervallen aufgerufen
**      werden und meistens im Kontext des System-Timer-Interrupts
**      laufen.
**
**  METHODS
**
**      ITIM_ELAPSEDTIME
**          Msg:    void
**          Ret:    ULONG (Zeitdifferenz seit letztem Aufruf)
**
**          Liefert die Zeitdifferent seit dem letzten Aufruf
**          der Methode in Millisekunden zurück, aber mindestens
**          1 Millisekunde. Ist dies der 1.Aufruf seit der Geburt
**          des Objekts, ist das Ergebnis 1.
**
**      ITIM_ADDTIMERJOB
**          Msg:    static struct itimer_job_msg;
**          Ret:    ULONG success
**
**          Installiert einen neuen Timer-Job. Definiert
**          wird einen Zeitintervall, eine Zeitdauer und
**          3 Pointer auf Routinen für init(), trigger()
**          und cleanup() (jeder davon darf auch NULL sein).
**          Der init() Pointer wird noch innerhalb
**          ITIM_TIMEDJOB aufgerufen, der trigger()
**          Pointer jeweils nach Ablauf des Zeitintervalls,
**          der cleanup() Pointer bei der Terminierung des
**          Jobs. Alle 3 Routinen müssen aus einem externen
**          Kontext aufrufbar sein (z.B. aus einem Timer-
**          Interrupt).
**          Zurückgegeben wird 1 (TRUE), wenn der Job akzeptiert wurde,
**          oder 0 (FALSE), wenn er abgelehnt wurde.
**          Es ist wichtig, daß die übergebene Msg über die
**          Lebensdauer des Jobs erhalten bleibt!!! Es ist
**          also keine gute Idee, die Msg in eine Stack-Variable
**          zu packen, oder _method() zum Aufruf zu verwenden!
**          Es muß sichergestellt werden, daß die Job-Routinen
**          über die Laufzeit des Jobs in einem gültigen
**          Status bleiben. Die Laufzeit eines Jobs kann
**          vorzeitig per ITIM_REMTIMERJOB abgebrochen werden.
**
**      ITIM_REMTIMERJOB
**          Msg:    static struct itimer_job_msg
**          Ret:    void
**
**          Der entsprechende Timerjob wird sofort terminiert,
**          es wird garantiert, daß die Job-Routinen nach
**          Rückkehr der Methode nicht aufgerufen werden.
**          Der cleanup() Vektor des Jobs wird korrekt aufgerufen.
**          Nach Rückkehr der Methode kann die <itimer_job_msg>
**          Struktur gekillt werden.
*/

/*-----------------------------------------------------------------*/
#define ITIMER_CLASSID  "itimer.class"
/*-----------------------------------------------------------------*/
#define ITIM_BASE       (OM_BASE+METHOD_DISTANCE)

#define ITIM_ELAPSEDTIME    (ITIM_BASE)
#define ITIM_ADDTIMERJOB    (ITIM_BASE+2)
#define ITIM_REMTIMERJOB    (ITIM_BASE+3)

/*-------------------------------------------------------------------
**  Message-Strukturen
*/
struct itimer_job_msg {
    ULONG interval;         // Timer-Interval in millisec
    ULONG duration;         // Zeitdauer in millisec, 0 für unendlich
    void (*init) (void);
    void (*trigger) (void);
    void (*cleanup) (void);
};

/*-----------------------------------------------------------------*/
#endif

