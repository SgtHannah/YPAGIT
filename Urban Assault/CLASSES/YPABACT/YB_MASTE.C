/*
**  $Source: PRG:MovingCubesII/Classes/_YPABactClass/yb_masterslave.c,v $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:23:36 $
**  $Locker:  $
**  $Author: floh $
**
**  Realisierung der logischen Geschwader-Struktur.
**
**  (C) Copyright 1995 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/lists.h>

#include "nucleus/nucleus2.h"
#include "ypa/ypabactclass.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus

/*-----------------------------------------------------------------*/
_dispatcher(void, yb_YBM_ADDSLAVE, Object *slave)
/*
**  FUNCTION
**      Hängt ein neues Bakterien-Object in die Slave-Liste.
**      Der neue Master (auf den die Methode angewendet wurde),
**      baut eine <struct newmaster_msg> zusammen und wendet
**      sie auf den neuen Slave an. Dieser klinkt sich dann
**      selbst ein (funktioniert wie BSM_ADDCHILD bei base.class).
**
**      Bitte beachten, daß die Methode nur mit _methoda()
**      invoked werden kann!
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      11-Jun-95   floh    created
*/
{
    struct ypabact_data *ybd = INST_DATA(cl,o);
    struct newmaster_msg slave_msg;

    slave_msg.master = o;       /* das bin ich */
    slave_msg.master_bact = &(ybd->bact);
    slave_msg.slave_list  = &(ybd->bact.slave_list);

    /*** das ist easy... ***/
    _methoda(slave, YBM_NEWMASTER, &slave_msg);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yb_YBM_NEWMASTER, struct newmaster_msg *msg)
/*
**  FUNCTION
**      Diese Methode wird normalerweise nicht direkt auf das
**      Object angewendet. Sie wird von einem neuen Master
**      auf seinen neuen Slave angewendet mit Informationen,
**      wie und wo er sich in die slave_list des Masters
**      einklinken soll.
**      Das Objekt entfernt sich selbständig aus seiner alten
**      Liste.
**
**      *** SONDERFALL ***
**      Diese Methode wird auch vom Welt-Object auf Commander-Objects
**      angewendet, in diesem Fall sieht die newmaster_msg so aus:
**
**      msg.master = 1L         -> also KEIN gültiger Object-Pointer!
**      msg.master_bact = NULL  -> weil Welt-Object kein Bakterien-Object
**      msg.slave_list          -> Listenheader im Welt-Object
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      11-Jun-95   floh    created
**                          (1) Ooops, böser Bug in _AddTail()-Args
*/
{
    struct ypabact_data *ybd = INST_DATA(cl,o);

    /*** bin ich bereits in eine slave_list eingebunden (das ***/
    /*** ist immer der Fall, außer 'innerhalb' OM_NEW)       ***/
    if (ybd->bact.master != NULL) {

        /*** aus alter slave_list entfernen ***/
        _Remove((struct Node *) &(ybd->bact.slave_node.nd));
    };

    /*** in neue Liste einklinken und ein paar Daten übernehmen... ***/
    _AddHead((struct List *) msg->slave_list,
             (struct Node *) &(ybd->bact.slave_node));

    ybd->bact.master = msg->master;           /* kann 1L sein */
    ybd->bact.master_bact = msg->master_bact; /* kann NULL sein */

    /*** Ende ***/
}









