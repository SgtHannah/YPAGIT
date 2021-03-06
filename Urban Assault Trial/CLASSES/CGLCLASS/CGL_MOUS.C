/*
**  $Source: PRG:VFM/Classes/_CglClass/cgl_mouse.c,v $
**  $Revision: 38.1 $
**  $Date: 1996/12/02 21:15:14 $
**  $Locker:  $
**  $Author: floh $
**
**  Mouse-Callback-Modul f�r cgl.class.
**  Compiliere mit ss!=ds!!!
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bitmap/cglclass.h"

/*-----------------------------------------------------------------*/
void cgl_MCallBack(struct cgl_data *cd, LONG x, LONG y)
/*
**  FUNCTION
**      Callback-Routine f�r "Mouse-Moved-Event". Wird
**      von der Maustreiber-Klasse aufgerufen.
**
**  CHANGED
**      28-Nov-96   floh    created
*/
{
    cd->ptr_x = x;
    cd->ptr_y = y;
    if (cd->ptr) cglSetCursorPos(cd->ptr_x,cd->ptr_y);
}
