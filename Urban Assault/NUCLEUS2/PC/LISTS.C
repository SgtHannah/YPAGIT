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
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/nodes.h>

#include <string.h>

/*** FIXME ***/
#include "nucleus/nucleus2.h"
_extern_use_nucleus

/*-----------------------------------------------------------------*/
void nc_NewList(struct List *list)
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
    list->lh_Tail     = NULL;
    list->lh_TailPred = (struct Node *) &(list->lh_Head);
};

/*-----------------------------------------------------------------*/
void nc_AddHead(struct List *list, struct Node *node)
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
    register struct Node *old_first;

    old_first     = list->lh_Head;
    list->lh_Head = node;

    /*** FIXME ***/
    if ((node->ln_Succ != NULL) || (node->ln_Pred != NULL)) {
        _LogMsg("_AddHead(): Node added twice!\n");
    };

    node->ln_Succ = old_first;
    node->ln_Pred = (struct Node *) list;

    old_first->ln_Pred = node;
};

/*-----------------------------------------------------------------*/
void nc_AddTail(struct List *list, struct Node *node)
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
    register struct Node *old_last;

    old_last          = list->lh_TailPred;
    list->lh_TailPred = node;

    /*** FIXME ***/
    if ((node->ln_Succ != NULL) || (node->ln_Pred != NULL)) {
        _LogMsg("_AddTail(): Node added twice!\n");
    };

    node->ln_Succ = (struct Node *) &(list->lh_Tail);
    node->ln_Pred = old_last;

    old_last->ln_Succ = node;
};

/*-----------------------------------------------------------------*/
struct Node *nc_RemHead(struct List *list)
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
    register struct Node *first  = list->lh_Head;
    register struct Node *second = first->ln_Succ;
    if (!second) return(NULL);

    list->lh_Head   = second;
    second->ln_Pred = (struct Node *) list;

    /*** FIXME ***/
    first->ln_Succ = NULL;
    first->ln_Pred = NULL;

    return(first);
};

/*-----------------------------------------------------------------*/
struct Node *nc_RemTail(struct List *list)
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
    register struct Node *last    = list->lh_TailPred;
    register struct Node *prelast = last->ln_Pred;
    if (!prelast) return(NULL);

    list->lh_TailPred = prelast;
    prelast->ln_Succ  = (struct Node *) &(list->lh_Tail);

    /*** FIXME ***/
    last->ln_Succ = NULL;
    last->ln_Pred = NULL;

    return(last);
};

/*-----------------------------------------------------------------*/
void nc_Remove(struct Node *node)
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
*/
{
    register struct Node *succ = node->ln_Succ;
    register struct Node *pred = node->ln_Pred;

    /*** FIXME ***/
    if ((succ == NULL) || (pred == NULL)) {
        _LogMsg("_Remove(): Node not in list!\n");
    };

    pred->ln_Succ = succ;
    succ->ln_Pred = pred;

    /*** FIXME ***/
    node->ln_Succ = NULL;
    node->ln_Pred = NULL;
};

/*-----------------------------------------------------------------*/
struct Node *nc_FindName(struct List *start, STRPTR name)
/*
**  FUNCTION
**      Scannt Liste nach Namen (node->ln_Name), returniert
**      Pointer auf Node, falls erfolgreich, NULL, falls
**      Name nicht in Liste. Die Suche ist <case-sensitiv>!
**
**  INPUTS
**      start   -> Pointer auf Liste, bzw. Node mitten in der
**                 Liste (in dem Fall fließt diese Node nicht mit in
**                 den Vergleich ein).
**
**  RESULTS
**      Pointer auf Node oder NULL, falls Suche erfolglos.
**
**  CHANGED
**      22-Dec-94   floh    created from scratch.
**      02-Mar-95   floh    arbeitet jetzt case-insensitiv.
*/
{
    struct Node *nd;
    for (nd=start->lh_Head; nd->ln_Succ; nd=nd->ln_Succ) {
        if (stricmp(name,nd->ln_Name) == 0) return(nd);
    };
    return(NULL);
};

