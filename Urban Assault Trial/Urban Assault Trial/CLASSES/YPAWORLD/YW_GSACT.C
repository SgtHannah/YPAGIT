/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_listview.c,v $
**  $Revision: 38.8 $
**  $Date: 1996/03/21 20:30:55 $
**  $Locker:  $
**  $Author: floh $
**
**  Routinen für die GameShell
**
**  (C) Copyright 1995 by Flemming aus Schwarzenberg
*/
#include <exec/types.h>
#include <exec/memory.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include "nucleus/nucleus2.h"
#include "engine/engine.h"
#include "visualstuff/ov_engine.h"
#include "input/inputclass.h"
#include "input/idevclass.h"
#include "ypa/ypaworldclass.h"
#include "ypa/guilist.h"
#include "ypa/ypagui.h"
#include "ypa/ypagameshell.h"
#include "bitmap/winddclass.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_input_engine
_extern_use_ov_engine
_extern_use_audio_engine

extern struct key_info   GlobalKeyTable[ 256 ];
extern struct video_info GlobalVideoTable[ 256 ];
extern UBYTE  **GlobalLocaleHandle;

/*** Brot-O-Typen ***/
#include "yw_protos.h"
#include "yw_gsprotos.h"

extern unsigned long wdd_DoDirect3D;

_dispatcher( BOOL, yw_YWM_SETGAMEINPUT, struct GameShellReq *GSR )
{
/*
**  FUNCTION    
*/

    struct ypaworld_data *ywd;
    char   the_string[ 500 ];
    struct inp_handler_msg sh;
    struct idev_sethotkey_msg sk;
    struct inp_delegate_msg del;
    Object *input_object;

    ywd = INST_DATA( cl, o );
    

    /* -----------------------------------------------------------------
    ** Ein Eingabeereignis soll geändert werden. Dazu basteln wir einen
    ** String für das InputObjekt, welches wir durch diverse Beziehungen
    ** erhalten haben, zusammen.
    ** Wenn wir nicht im Qualifiermodus sind, werden die Qualifiertasten
    ** einfach ignoriert.
    ** ---------------------------------------------------------------*/

    /*** Gibt es die tasten überhaupt? ***/
    if( (GSR->i_actualitem < 1) && (GSR->i_actualitem > NUM_INPUTEVENTS))
        return( FALSE );
    if( !GlobalKeyTable[ GSR->inp[ GSR->i_actualitem ].pos ].config )
        return( FALSE );
    if( GSR->inp[ GSR->i_actualitem ].kind == GSI_SLIDER )
        if( !GlobalKeyTable[ GSR->inp[ GSR->i_actualitem ].neg ].config )
            return( FALSE );
    
    the_string[0]=0;
    
    if( GSR->inp[ GSR->i_actualitem ].kind == GSI_SLIDER ) {

        /*** Da müssen wir 2 Sachen mit SliderOption machen ***/
        strcpy( the_string, "~#" );
        strcat( the_string, KEYBORD_DRIVER );
        strcat( the_string, 
        GlobalKeyTable[ GSR->inp[ GSR->i_actualitem ].neg ].config );
        strcat( the_string, " #" );
        strcat( the_string, KEYBORD_DRIVER );
        strcat( the_string, 
        GlobalKeyTable[ GSR->inp[ GSR->i_actualitem ].pos ].config );
        }
    else {

        /*** Hotkeys sind anscheinend absolut ohne... ***/
        if( GSR->inp[ GSR->i_actualitem ].kind == GSI_BUTTON )
            strcpy( the_string, KEYBORD_DRIVER );
        
        strcat( the_string,
        GlobalKeyTable[ GSR->inp[ GSR->i_actualitem ].pos ].config );
        }

    _IE_GetAttrs( IET_Object, &input_object, TAG_DONE );

    /*** Jetzt geht es an das Objekt ***/
    if( GSR->inp[ GSR->i_actualitem ].kind == GSI_HOTKEY ) {

        /*** Bei Hotkeys delegate ***/
        sk.id     = the_string;
        sk.hotkey = GSR->inp[ GSR->i_actualitem ].number;

        del.type   = ITYPE_KEYBOARD;
        del.num    = 0;
        del.method = IDEVM_SETHOTKEY;
        del.msg    = &sk;
        if (!_methoda(input_object, IM_DELEGATE, &del))
            _LogMsg("input.engine: WARNING: Hotkey[%d] (%s) not accepted.\n",
                    GSR->inp[ GSR->i_actualitem ].number,sk.id);
        }
    else {

        /*** Button und Slider gehen direkt ***/
        sh.num = GSR->inp[ GSR->i_actualitem ].number;
        sh.id  = the_string;
        if( GSR->inp[ GSR->i_actualitem ].kind == GSI_BUTTON ) {
            
            sh.type = ITYPE_BUTTON;
            if( !_methoda( input_object, IM_SETHANDLER, &sh ))
                _LogMsg("input.engine: WARNING: Button[%d] (%s) not accepted.\n",
                         GSR->inp[ GSR->i_actualitem ].number,sh.id);
            }
        else {
            
            sh.type = ITYPE_SLIDER;
            if( !_methoda( input_object, IM_SETHANDLER, &sh ))
                _LogMsg("input.engine: WARNING: Slider[%d] (%s) not accepted.\n",
                         GSR->inp[ GSR->i_actualitem ].number,sh.id);
            }
        }

    /*** merken ***/
    GSR->inp[ GSR->i_actualitem ].bu_neg = GSR->inp[ GSR->i_actualitem ].neg;
    GSR->inp[ GSR->i_actualitem ].bu_pos = GSR->inp[ GSR->i_actualitem ].pos;
    return( TRUE );
}


_dispatcher( BOOL, yw_YWM_SETGAMEVIDEO, struct setgamevideo_msg *sgv)
{
    /* ----------------------------------------------------------
    ** Verändert die Auflösung. Dabei müssen wir auch die Shell
    ** schließen und wieder öffnen, damit sich diese an der neuen
    ** Auflösung orientiert.
    **
    ** Die Daten, die wir benötigen, stehen in der GSR
    ** --------------------------------------------------------*/

    struct ypaworld_data *ywd;
    ULONG  id, mi, xres, yres;
    BOOL   shell_was_open;
    Object *dobj;
    struct GameShellReq *GSR;

    ywd = INST_DATA( cl, o );
    GSR = ywd->gsr;

    /* ----------------------------------------------------------------
    ** Um zu gergleichen, ob der neue nicht zufällig dem alten
    ** Modus entspricht und wir gar nix zu tun haben, holen wir das
    ** DisplayObjekt von der Engine und fragen e nach der eingestellten
    ** DisplayID.
    ** --------------------------------------------------------------*/
    _OVE_GetAttrs( OVET_Object, &dobj, TAG_DONE );
    _get( dobj, DISPA_DisplayID, &id );

    if( (id    == sgv->modus) &&
        (FALSE == sgv->forcesetvideo) )
        return( TRUE );

    if( GSR->ShellOpen ) {

        _methoda( o, YWM_CLOSEGAMESHELL, GSR );
        shell_was_open = TRUE;
        }
    else shell_was_open = FALSE;

    _IE_SetAttrs(  IET_ModeInfo,   NULL, TAG_DONE );
    _OVE_SetAttrs( OVET_ModeInfo,  sgv->modus, TAG_DONE );
    _OVE_GetAttrs( OVET_ModeInfo, &mi, TAG_DONE );
    _IE_SetAttrs(  IET_ModeInfo,   mi, TAG_DONE );

    /* ----------------------------------------------------------
    ** Aus Sicherheitsgründen holen wir uns die Auflösung von der
    ** Engine und nicht aus der Node
    ** --------------------------------------------------------*/

    _OVE_GetAttrs( OVET_XRes, &xres, OVET_YRes, &yres, TAG_DONE );
    ywd->DspXRes = (WORD) xres;
    ywd->DspYRes = (WORD) yres;

    if( shell_was_open ) {

        if( !_methoda( o, YWM_OPENGAMESHELL, GSR )) {

            _LogMsg("Warning: Unable to open GameShell with mode %d\n",
                     sgv->modus);

            /* -------------------------------------------------
            ** also default laden. Das mache ich nicht rekursiv,
            ** weil dann die Openinfo untergeht und es zum Chaos
            ** kommt! Also alles nochmal!
            ** -----------------------------------------------*/
            _IE_SetAttrs(  IET_ModeInfo,   NULL, TAG_DONE );
            _OVE_SetAttrs( OVET_ModeInfo,  GSR->ywd->ShellRes, TAG_DONE );
            _OVE_GetAttrs( OVET_ModeInfo, &mi, TAG_DONE );
            _IE_SetAttrs(  IET_ModeInfo,   mi, TAG_DONE );
            
            _OVE_GetAttrs( OVET_XRes, &xres, OVET_YRes, &yres, TAG_DONE );
            ywd->DspXRes = (WORD) xres;
            ywd->DspYRes = (WORD) yres;
            
            if( !_methoda( ywd->world, YWM_OPENGAMESHELL, GSR ) )
                return( FALSE );
            }
        }

    /*** Etwas Nacharbeit ***/
    _OVE_GetAttrs( OVET_Object, &dobj, TAG_DONE );
    if( GSR->video_flags & VF_SOFTMOUSE )
        _set(dobj,WINDDA_CursorMode,WINDD_CURSORMODE_SOFT);
    else
        _set(dobj,WINDDA_CursorMode,WINDD_CURSORMODE_HW);

    // FLOH: 09-May-98  
    #ifdef __DBCS__
    if (ywd->DspXRes < 512) {
        dbcs_SetFont(ypa_GetStr(ywd->LocHandle,STR_FONTDEFINE_LRES,"Arial,8,400,0"));
    } else {
        dbcs_SetFont(ypa_GetStr(ywd->LocHandle,STR_FONTDEFINE,"MS Sans Serif,12,400,0"));
    };
    #endif
    return( TRUE );
}


_dispatcher( BOOL, yw_YWM_SETGAMELANGUAGE, struct GameShellReq *GSR )
{

    /* ------------------------------------------------------------
    ** Verändert die Sprache. Das hat ebenfalls den Nachteil, zu 
    ** Schließen und zu Öffnen. Wir machen das jetzt so, daß wir 
    ** nur schließen und öffnen, wenn auch offen war, weil jedesmal
    ** ein LoadSet dranhängt.
    **
    ** Die Daten, die wir benötigen, stehen in der GSR
    ** --------------------------------------------------------*/

    struct ypaworld_data *ywd;
    BOOL   shell_was_open;
    struct setlanguage_msg sl;

    ywd = INST_DATA( cl, o );

    /*** Überhaupt ne Sprache eingestellt ? ***/
    if( !GSR->lsel ) {

        _LogMsg("Set Language, but no language selected\n");
        return( FALSE );
        }


    /*** Schließen ***/
    if( GSR->ShellOpen ) {

        _methoda( o, YWM_CLOSEGAMESHELL, GSR );
        shell_was_open = TRUE;
        }
    else 
        shell_was_open = FALSE;

    /*** nun etwas Sprache setzen ***/
    sl.lang = GSR->lsel->language;
    if( !_methoda( o, YWM_SETLANGUAGE, &sl ))
        _LogMsg("Warning: SETLANGUAGE failed\n");

    /*** Wieder öffnen ***/
    if( shell_was_open ) {

        if( !_methoda( o, YWM_OPENGAMESHELL, GSR )) {

            _LogMsg("Unable to open GameShell\n");
            return( FALSE );
            }
        }

    /*** Schien alles ok zu sein ***/
    return( TRUE );
}

