/*
**  $Source: PRG:VFM/Nucleus2/pc/lists.c,v $
**  $Revision: 38.3 $
**  $Date: 1996/04/26 14:05:39 $
**  $Locker:  $
**  $Author: floh $
**
**  Listen-Funktionen des PC-Nucleus-Kernels.
**  [alles ANSI-kompatibel]
**
**  (C) Copyright 1994 by A.Weissflog
*/
#include <network/winlist.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <direct.h>

char DPLAYLOGNAME[ 300 ];
int  DPLAYLOGCOUNT = 0;


/*-----------------------------------------------------------------*/
void win_NewList(struct WinList *list)
/*
**  FUNCTION
**      Initialisiert Listen-Header.
**
**  INPUTS
**      list    -> Pointer auf Listen-Header.
**
**  RESULTS
**      ---
**
**  CHANGED
**      22-Dec-94   floh    rewritten from scratch
**
*/
{
    list->lh_Head     = (struct Node *) &(list->lh_Tail);
    list->lh_Tail     = (void *)0L;
    list->lh_TailPred = (struct Node *) &(list->lh_Head);
};

/*-----------------------------------------------------------------*/
void win_AddHead(struct WinList *list, struct WinNode *node)
/*
**  FUNCTION
**      Hängt neue Node an Anfang der Liste.
**
**  INPUTS
**      list    -> Pointer auf Liste
**      node    -> Pointer auf Node
**
**  RESULTS
**      ---
**
**  CHANGED
**      22-Dec-94   floh    erzeugt aus <exec/lists.i>-Macros
*/
{
    struct WinNode *old_first;

    old_first     = list->lh_Head;
    list->lh_Head = node;

    node->ln_Succ = old_first;
    node->ln_Pred = (struct WinNode *) list;

    old_first->ln_Pred = node;
};

/*-----------------------------------------------------------------*/
void win_AddTail(struct WinList *list, struct WinNode *node)
/*
**  FUNCTION
**      Hängt neue Node ans Ende einer Liste.
**
**  INPUTS
**      list    -> Pointer auf Listen-Header.
**      node    -> Pointer auf Node.
**
**  RESULTS
**      ---
**
**  CHANGED
**      22-Dec-94   floh    erzeugt aus <exec/lists.i>-Macros
*/
{
    struct WinNode *old_last;

    old_last          = list->lh_TailPred;
    list->lh_TailPred = node;

    node->ln_Succ = (struct WinNode *) &(list->lh_Tail);
    node->ln_Pred = old_last;

    old_last->ln_Succ = node;
};

/*-----------------------------------------------------------------*/
struct WinNode *win_RemHead(struct WinList *list)
/*
**  FUNCTION
**      Entfernt erste Node aus Liste und gibt Pointer darauf
**      zurück, bzw. NULL, wenn Liste leer war.
**
**  INPUTS
**      list    -> Pointer auf Liste
**
**  RESULTS
**      Pointer auf entfernte Node, oder NULL, wenn Liste leer war.
**
**  CHANGED
**      22-Dec-94   floh    erzeugt aus <exec/lists.i>-Macros
*/
{
    struct WinNode *first  = list->lh_Head;
    struct WinNode *second = first->ln_Succ;

    if (!second)
        return( (struct WinNode *)0L);

    list->lh_Head   = second;
    second->ln_Pred = (struct WinNode *) list;

    /*** FIXME ***/
    first->ln_Succ = (struct WinNode *)0L;
    first->ln_Pred = (struct WinNode *)0L;

    return(first);
};

/*-----------------------------------------------------------------*/
struct WinNode *win_RemTail(struct WinList *list)
/*
**  FUNCTION
**      Entfernt letzte Node von Liste, gibt Pointer darauf zurück,
**      bzw. NULL, wenn Liste leer.
**
**  INPUTS
**      list    -> Pointer auf Liste
**
**  RESULTS
**      Pointer auf entfernte Node, oder NULL, falls Liste leer.
**
**  CHANGED
**      22-Dec-94   floh    created aus <exec/lists.i>-Macro
*/
{
    struct WinNode *last    = list->lh_TailPred;
    struct WinNode *prelast = last->ln_Pred;

    if (!prelast) 
        return((struct WinNode *)0L);

    list->lh_TailPred = prelast;
    prelast->ln_Succ  = (struct WinNode *) &(list->lh_Tail);

    /*** FIXME ***/
    last->ln_Succ = (struct WinNode *)0L;
    last->ln_Pred = (struct WinNode *)0L;

    return(last);
};

/*-----------------------------------------------------------------*/
void win_Remove(struct WinList *list, struct WinNode *node)
/*
**  FUNCTION
**      Entfernt Node aus der Liste, in der sie eingebettet ist.
**
**  INPUTS
**      node    -> diese Node isolieren
**
**  RESULTS
**      ---
**
**  CHANGED
**      22-Dec-94   floh    erzeugt aus <exec/lists.i>-Macro
**      Wegen der Threadsicherheit brauche ich die Liste
*/
{
    struct WinNode *succ = node->ln_Succ;
    struct WinNode *pred = node->ln_Pred;

    pred->ln_Succ = succ;
    succ->ln_Pred = pred;

    /*** FIXME ***/
    node->ln_Succ = (struct WinNode *)0L;
    node->ln_Pred = (struct WinNode *)0L;
};

//-------------------- LOG ZEUG ----------------------------------------------

unsigned long wdp_OpenLogScript(void)
{
    /*** ermittelt Name zum Script fuer die folgende Session ***/
    FILE *f;
    char wd[ 300 ], number[ 100 ];
    struct dirent *dir, *entry;

    /* -----------------------------------------------------------
    ** Wenn nicht gewuenscht, dann auch nicht initialisieren, dann
    ** ist der NETLOGCOUNT nicht initialisiert und somit kommen
    ** auch keine Messages durch. GetConfigItem wurde schon in
    ** gsinit.c gemacht.
    ** ---------------------------------------------------------*/
    //if( !yw_ConfigItems[3].data ) return( TRUE );

    /*** Wenn Verzeichnis noch nicht existiert, dann erzeugen ***/
    memset( wd, 0, 300 );
    getcwd( wd, 300 );
    strcat( wd, "\\env\\debug" );
    if( dir = opendir( wd ) ) {
        closedir( dir );
        }
    else {
        if( mkdir( wd ))
            return( 0L );
        }

    /* ---------------------------------------------------------
    ** hoechste derzeitige Nummer fuer net_log.txt suchen. Keine
    ** Nucleus-Routinen, die machen die Filenamen kaputt
    ** -------------------------------------------------------*/
    if( dir = opendir( wd ) ) {
        while( entry = readdir( dir ) ) {

            if( strnicmp( entry->d_name, "dplay_log",9 ) == 0 ) {

                /*** ein Netz-Logfile ***/
                strcpy( number, &(entry->d_name[9]) );
                strtok( number, "." );
                if( DPLAYLOGCOUNT < atol( number ) )
                    DPLAYLOGCOUNT = atol( number );
                }
            }
        closedir( dir );
        }
    DPLAYLOGCOUNT++;

    sprintf( DPLAYLOGNAME, "%s\\dplay_log%d.txt", wd, DPLAYLOGCOUNT );
    if( f = fopen( DPLAYLOGNAME, "w" )) {
        fclose( f );
        return( 1L );
        }
    else
        return( 0L );
}


unsigned long wdp_Log( char* string, ... )
{
    FILE *f;

    /*** Schon initialisiert? ***/
    if( 0 == DPLAYLOGCOUNT ) return( 0L );

    if( f = fopen( DPLAYLOGNAME, "a" )) {

        va_list arglist;

        va_start(arglist, string);
        vfprintf( f, string, arglist );

        fclose( f );

        va_end( arglist);
        return( 1L );
        }
    else
        return( 0L );
}



