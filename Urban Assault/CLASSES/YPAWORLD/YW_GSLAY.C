/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_listview.c,v $
**  $Revision: 38.8 $
**  $Date: 1996/03/21 20:30:55 $
**  $Locker:  $
**  $Author: floh $
**
**  Routinen für die GameShell
**
**  (C) Copyright 1995 by A.Flemming
*/
#include <exec/types.h>
#include <exec/memory.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "nucleus/nucleus2.h"
#include "engine/engine.h"
#include "ypa/ypaworldclass.h"
#include "ypa/guilist.h"
#include "ypa/ypagui.h"
#include "ypa/ypagameshell.h"
#include "visualstuff/ov_engine.h"
#include "network/networkclass.h"
#include "bitmap/winddclass.h"
#include "nucleus/math.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_input_engine
_extern_use_ov_engine
_extern_use_audio_engine

extern struct key_info   GlobalKeyTable[ 256 ];
extern struct video_info GlobalVideoTable[ 256 ];
extern UBYTE  **GlobalLocaleHandle;

/*** externe Designdaten ***/
extern WORD ReqDeltaX;
extern WORD ReqDeltaY;
extern WORD IReqWidth;
extern WORD SReqWidth;
extern WORD VReqWidth;
extern WORD LReqWidth;
extern WORD DReqWidth;
extern WORD NReqWidth;
extern WORD NListWidth;
extern WORD IListWidth;

#define GET_X_COORD(x) (FLOAT_TO_INT((((FLOAT)x)/640.0)*((FLOAT)ywd->DspXRes)))
#define GET_Y_COORD(y) (FLOAT_TO_INT((((FLOAT)y)/480.0)*((FLOAT)ywd->DspYRes)))

/*** Prototypen, nix als Prototypen... ***/
#include "yw_protos.h"
#include "yw_gsprotos.h"
#include "yw_netprotos.h"

#ifdef __WINDOWS__
extern ULONG wdd_DoDirect3D;
#endif

/// "Layout der GameShell"
void yw_LayoutGameShell( struct ypaworld_data *ywd, struct GameShellReq *GSR )
{
/*
**  FUNCTION    zeichnet die GameShell
**
**  CHANGED     created
**              mit reihenfolge
*/

    switch( GSR->shell_mode ) {

        case SHELLMODE_DISK:

            _methoda( GSR->bdisk,   BTM_PUBLISH, NULL );
            yw_DrawDiskMenu( ywd, GSR );
            break;

        case SHELLMODE_ABOUT:

            _methoda( GSR->babout,  BTM_PUBLISH, NULL );
            break;

        case SHELLMODE_INPUT:

            _methoda( GSR->binput,  BTM_PUBLISH, NULL );
            yw_DrawInputMenu( ywd, GSR );
            break;

        case SHELLMODE_SETTINGS:

            _methoda( GSR->bvideo,  BTM_PUBLISH, NULL );
            if( !(GSR->vmenu.Req.flags & REQF_Closed) )
                yw_DrawVideoMenu( ywd, GSR );
            if( !(GSR->d3dmenu.Req.flags & REQF_Closed) )
                yw_DrawD3DMenu( ywd, GSR );
            break;

        case SHELLMODE_LOCALE:

            _methoda( GSR->blocale, BTM_PUBLISH, NULL );
            yw_DrawLocaleMenu( ywd, GSR );
            break;

        case SHELLMODE_NETWORK:

            _methoda( GSR->bnet, BTM_PUBLISH, NULL );
            if(GSR->n_selmode != NM_PLAYER)
                yw_DrawNetMenu( ywd, GSR );
            break;

        case SHELLMODE_TITLE:

            _methoda( GSR->Titel, BTM_PUBLISH, NULL );
            break;

        case SHELLMODE_PLAY:
        case SHELLMODE_TUTORIAL:

            _methoda( GSR->UBalken, BTM_PUBLISH, NULL );
            break;

        default:

            break;
        }
        
    yw_RenderConfirmRequester( GSR );
        
}
///


/// "yw_DrawInputMenu"
void yw_DrawInputMenu(struct ypaworld_data *ywd, struct GameShellReq *GSR)
/*
**  FUNCTION
**      Layoutet InputMenu komplett.
**
**  CHANGED
**      16-Apr-96   8100 000C geklaut vom Floh, am 110. Geburtstag von
**                  Ernst Thälmann
**      18-Jun-96   kann Flohs Routinen wegen der Tabs nicht nehmen, massive
**                  Erweiterungen
*/
{
    ULONG  i;
    struct drawtext_args dt;
    BYTE   *str = GSR->imenu.Itemblock, zsp[ 200 ];
    struct VFMFont *fnt;

    /*** Die Position des Requesters und damit des LV kann sich ändern ***/
    yw_ListSetRect(ywd, &(GSR->imenu), -2, -2);

    /*** Den Font für Menüs ermitteln ***/
    fnt = _GetFont( STAT_MENUUP_FONT );

    /*** ModeMenu-Parameter modifizieren ***/
    // GSR->IMenu.Selected     = 0;

    /*** das letztendliche Layout ***/
    str = yw_LVItemsPreLayout(ywd, &(GSR->imenu), str, STAT_MENUUP_FONT, "uvw");

    /*** alle Items zeichnen, Nummern beginnen bei 1 ***/
    for (i=1; i<=GSR->imenu.ShownEntries; i++) {

        /*** finde zugehörigen String ***/
        if( GSR->inp[ i + GSR->imenu.FirstShown ].menuname ) {

           
            struct ypa_ColumnItem col[2];
            UBYTE fnt_id;
            UBYTE space_chr;
            UBYTE prefix_chr;
            UBYTE postfix_chr;
            ULONG pos, width;

            pos = GSR->imenu.FirstShown + i;

            /*** Spaltenaufauen ***/
            memset(col,0,sizeof(col));
            if( pos == GSR->i_actualitem ) {
                fnt_id      = STAT_MENUDOWN_FONT;
                space_chr   = 'c';
                prefix_chr  = 'b';
                postfix_chr = 'd';
            } else {
                fnt_id = STAT_MENUUP_FONT;
                space_chr   = 'f';
                prefix_chr  = 'f';
                postfix_chr = 'f';
            };

            /*** Spalten initialisieren ***/
            width = GSR->imenu.ActEntryWidth - 2 * GSR->ywd->EdgeW;

            /*** Rundungsfehler-Hack.... ***/
            width++;

            /* ------------------------------------------------------
            ** Taste(n) erstmal zwischenspeichern. Up/Down(Left/Right
            ** sind localised
            ** ----------------------------------------------------*/
            if( GSR->inp[ pos ].kind == GSI_SLIDER ) {

                char *p, *n;

                /*** Testen, ob der  Eintrag definiert wurde ***/
                if( GSR->inp[ pos ].neg == 0 ) {

                    /*** Gibts nicht ***/
                    n = NICHTSTRING;
                    }
                else {

                    n = GlobalKeyTable[ GSR->inp[ pos ].neg ].name;
                    }

                /*** Definiert? ***/
                if( GSR->inp[ pos ].pos == 0 ) {

                    /*** Gibts nicht ***/
                    p = NICHTSTRING;
                    }
                else {

                    p = GlobalKeyTable[ GSR->inp[ pos ].pos ].name;
                    }

                /*** ist was zur Zeit in der Aenderung? ***/
                if( GSR->inp[ pos ].done & IF_FIRST )
                    p = ypa_GetStr( GlobalLocaleHandle, STR_IGADGET_TOBEDEFINED, "?");
                if( GSR->inp[ pos ].done & IF_SECOND )
                    n = ypa_GetStr( GlobalLocaleHandle, STR_IGADGET_TOBEDEFINED, "?");

                sprintf( zsp, "%s/%s\0", n, p );
                }
            else {

                char *p;

                /*** Gibts den überhaupt ***/
                if( GSR->inp[ pos ].pos == 0 ) {

                    /*** Is nich ***/
                    p = NICHTSTRING;
                    }
                else {

                    p = GlobalKeyTable[ GSR->inp[ pos ].pos ].name;
                    }

                /*** ist was zur Zeit in der Aenderung? ***/
                if( GSR->inp[ pos ].done & IF_FIRST )
                    p = ypa_GetStr( GlobalLocaleHandle, STR_IGADGET_TOBEDEFINED, "?");

                sprintf( zsp, "%s\0", p);
                }
    
            col[0].string      = GSR->inp[ pos ].menuname;
            col[0].width       = 0.68 * width;
            col[0].font_id     = fnt_id;
            col[0].space_chr   = space_chr;
            col[0].prefix_chr  = prefix_chr;
            col[0].postfix_chr = 0;
            col[0].flags       = YPACOLF_ALIGNLEFT | YPACOLF_DOPREFIX | YPACOLF_TEXT;

            col[1].string      = zsp;
            col[1].width       = (WORD)(width - 0.68 * width);
            col[1].font_id     = fnt_id;
            col[1].space_chr   = space_chr;
            col[1].prefix_chr  = 0;
            col[1].postfix_chr = postfix_chr;
            col[1].flags       = YPACOLF_ALIGNLEFT | YPACOLF_DOPOSTFIX | YPACOLF_TEXT;

            new_font(str,STAT_MENUUP_FONT);
            put(str,'{');   // linker Menü-Rand
            
            if( pos == GSR->i_actualitem ) {
                dbcs_color(str,yw_Red(GSR->ywd,YPACOLOR_TEXT_LIST_SEL),
                               yw_Green(GSR->ywd,YPACOLOR_TEXT_LIST_SEL),
                               yw_Blue(GSR->ywd,YPACOLOR_TEXT_LIST_SEL));
                }
            else {
                dbcs_color(str,yw_Red(GSR->ywd,YPACOLOR_TEXT_LIST),
                               yw_Green(GSR->ywd,YPACOLOR_TEXT_LIST),
                               yw_Blue(GSR->ywd,YPACOLOR_TEXT_LIST));
                }

            str = yw_BuildColumnItem(ywd,str,2,col);
            new_font(str,STAT_MENUUP_FONT);
            put(str,'}');   // rechter Menü-Rand
            new_line(str);

            }
        }

    /*** zeichne unteren Rand ***/
    str = yw_LVItemsPostLayout(ywd, &(GSR->imenu), str, STAT_MENUUP_FONT, "xyz");

    /*** EOS (Edge of Sanity??? ) ***/
    *str++ = 0x0;
    *str++ = 0x0;

    /*** Zeichnen ***/
    dt.string = GSR->imenu.Req.req_string;
    dt.clips  = GSR->imenu.Req.req_clips;
    _DrawText(&dt);

}
///


/// "yw_DrawVideoMenu"
void yw_DrawVideoMenu(struct ypaworld_data *ywd, struct GameShellReq *GSR)
/*
**  FUNCTION
**      Layoutet InputMenu komplett.
**
**  CHANGED
**      16-Apr-96   8100 000C geklaut vom Floh, am 110. Geburtstag von
**                  Ernst Thälmann
*/
{
    struct drawtext_args dt;
    BYTE   *str = GSR->vmenu.Itemblock;
    LONG   item_count;
    struct video_node *vnode;

    /*** Die Position des Requesters und damit des LV kann sich ändern ***/
    yw_ListSetRect(ywd, &(GSR->vmenu), -2, -2);

    /*** das letztendliche Layout ***/
    str = yw_LVItemsPreLayout(ywd, &(GSR->vmenu), str, STAT_MENUUP_FONT, "uvw");

    vnode      = (struct video_node *) GSR->videolist.mlh_Head;
    item_count = 0;
    while( vnode->node.mln_Succ ) {

        UBYTE *mode_str;

        mode_str = vnode->name;

        /*** Noch im darzustellenden Bereich? ***/
        if( (item_count <  (GSR->vmenu.ShownEntries + GSR->vmenu.FirstShown)) &&
            (item_count >= GSR->vmenu.FirstShown) ) {

            /*** Sonderfall Selected Menuitem ***/
            if( GSR->vmenu.Selected == item_count ) {
                str = yw_MenuLayoutSelItem(ywd, &(GSR->vmenu), str, mode_str, 0);
                }
            else {
                str = yw_MenuLayoutItem(ywd, &(GSR->vmenu), str, mode_str, 0);
                }
            }
        vnode = (struct video_node *) vnode->node.mln_Succ;
        item_count++;
        }

    /*** Wieviel haben wir? ***/
    GSR->vmenu.ShownEntries = min( item_count , NUM_VIDEO_SHOWN );

    /*** zeichne unteren Rand ***/
    str = yw_LVItemsPostLayout(ywd, &(GSR->vmenu), str, STAT_MENUUP_FONT, "xyz");

    /*** EOS (Edge of Sanity??? ) ***/
    *str++ = 0x0;
    *str++ = 0x0;

    /*** Zeichnen ***/
    dt.string = GSR->vmenu.Req.req_string;
    dt.clips  = GSR->vmenu.Req.req_clips;
    _DrawText(&dt);
}
///


/// "yw_DrawD3DMenu"
void yw_DrawD3DMenu(struct ypaworld_data *ywd, struct GameShellReq *GSR)
/*
**  FUNCTION
**      Layoutet das D3D-Menue
**
**  CHANGED
**      16-Apr-96   8100 000C geklaut vom Floh, am 110. Geburtstag von
**                  Ernst Thälmann
*/
{
    struct drawtext_args dt;
    BYTE   *str = GSR->d3dmenu.Itemblock;
    LONG   item_count;
    struct windd_device wdm;

    /*** Die Position des Requesters und damit des LV kann sich ändern ***/
    yw_ListSetRect(ywd, &(GSR->d3dmenu), -2, -2);

    /*** das letztendliche Layout ***/
    str = yw_LVItemsPreLayout(ywd, &(GSR->d3dmenu), str, STAT_MENUUP_FONT, "uvw");

    item_count = 0;
    wdm.name   = NULL;
    wdm.guid   = NULL;
    wdm.flags  = 0;

    do {

        UBYTE *mode_str;

        _methoda( ywd->GfxObject, WINDDM_QueryDevice, &wdm );

        if( wdm.name ) {

            mode_str = wdm.name;

            /*** Noch im darzustellenden Bereich? ***/
            if( (item_count < (GSR->d3dmenu.ShownEntries + GSR->d3dmenu.FirstShown)) &&
                (item_count >= GSR->d3dmenu.FirstShown) ) {

                /*** Sonderfall Selected Menuitem ***/
                if( item_count == GSR->d3dmenu.Selected ) {
                    str = yw_MenuLayoutSelItem(ywd, &(GSR->d3dmenu), str, mode_str, 0);
                    }
                else {
                    str = yw_MenuLayoutItem(ywd, &(GSR->d3dmenu), str, mode_str, 0);
                    }
                }

            item_count++;
            }

        } while( wdm.name );

    /*** Wieviel haben wir? ***/
    GSR->d3dmenu.ShownEntries = min( item_count , NUM_D3D_SHOWN );
    GSR->d3dmenu.NumEntries   = item_count;

    /*** zeichne unteren Rand ***/
    str = yw_LVItemsPostLayout(ywd, &(GSR->d3dmenu), str, STAT_MENUUP_FONT, "xyz");

    /*** EOS (Edge of Sanity??? ) ***/
    *str++ = 0x0;
    *str++ = 0x0;

    /*** Zeichnen ***/
    dt.string = GSR->d3dmenu.Req.req_string;
    dt.clips  = GSR->d3dmenu.Req.req_clips;
    _DrawText(&dt);
}
///


/// "yw_DrawDiskMenu"
void yw_DrawDiskMenu(struct ypaworld_data *ywd, struct GameShellReq *GSR)
/*
**  FUNCTION
**      Layoutet InputMenu komplett.
**
**  CHANGED
**      28-Apr-96   8100 000C geklaut vom Floh, früh, kurz nach Miternacht
*/
{
    ULONG  i, j;
    struct drawtext_args dt;
    struct fileinfonode *nd;
    BYTE   *mode_str, *str = GSR->dmenu.Itemblock;

    /*** Die Position des Requesters und damit des LV kann sich ändern ***/
    yw_ListSetRect(ywd, &(GSR->dmenu), -2, -2);

    /*** das letztendliche Layout ***/
    str = yw_LVItemsPreLayout(ywd, &(GSR->dmenu), str, STAT_MENUUP_FONT, "uvw");

    /*** alle Items zeichnen, Nummern beginnen bei 1 ***/
    for( nd = (struct fileinfonode *) GSR->flist.mlh_Head, i= 0;
         nd->node.mln_Succ;
         nd = (struct fileinfonode *) nd->node.mln_Succ, i++ ) {
        
        /*** Noch im darzustellenden Bereich? ***/
        if( (i >= GSR->dmenu.FirstShown) &&
            (i < (GSR->dmenu.ShownEntries + GSR->dmenu.FirstShown) ) ) {

            struct ypa_ColumnItem col[2];
            UBYTE fnt_id;
            UBYTE space_chr;
            UBYTE prefix_chr;
            UBYTE postfix_chr;
            ULONG s, m, h, width, liw;
            char  time[ 20 ];

            /*** Spaltenaufauen ***/
            memset(col,0,sizeof(col));
            if( (i+1) == GSR->d_actualitem ) {
                fnt_id      = STAT_MENUDOWN_FONT;
                space_chr   = 'c';
                prefix_chr  = 'b';
                postfix_chr = 'd';
            } else {
                fnt_id = STAT_MENUUP_FONT;
                space_chr   = 'f';
                prefix_chr  = 'f';
                postfix_chr = 'f';
            };

            /*** Spalten initialisieren ***/
            width = GSR->dmenu.ActEntryWidth - 2 * GSR->ywd->EdgeW;

            s  = nd->global_time / 1000;
            h  = s / 3600;
            s -= 3600 * h;
            m  = s / 60;
            s -= m * 60;

            sprintf( time, "%02d:%02d:%02d", h, m, s );

            col[0].string      = nd->username;
            col[0].width       = (WORD)(0.75 * width);
            col[0].font_id     = fnt_id;
            col[0].space_chr   = space_chr;
            col[0].prefix_chr  = prefix_chr;
            col[0].postfix_chr = 0;
            col[0].flags       = YPACOLF_DOPREFIX | YPACOLF_ALIGNLEFT | YPACOLF_TEXT;

            col[1].string      = time;
            col[1].width       = width - col[0].width;
            col[1].font_id     = fnt_id;
            col[1].space_chr   = space_chr;
            col[1].prefix_chr  = 0;
            col[1].postfix_chr = postfix_chr;
            col[1].flags       = YPACOLF_ALIGNLEFT | YPACOLF_DOPOSTFIX | YPACOLF_TEXT;

            new_font(str,STAT_MENUUP_FONT);
            put(str,'{');   // linker Menü-Rand

            if( stricmp( nd->username, GSR->UserName) == 0 ) {
            
                /*** Der aktuell geladene bekommt immer ne andere Farbe ***/
                dbcs_color(str,yw_Red(GSR->ywd,YPACOLOR_OWNER_2),
                               yw_Green(GSR->ywd,YPACOLOR_OWNER_2),
                               yw_Blue(GSR->ywd,YPACOLOR_OWNER_2));
                }
            else {


                if( (i+1) == GSR->d_actualitem ) {
                    dbcs_color(str,yw_Red(ywd,YPACOLOR_TEXT_LIST_SEL),yw_Green(ywd,YPACOLOR_TEXT_LIST_SEL),
                                   yw_Blue(ywd,YPACOLOR_TEXT_LIST_SEL));
                    }
                else {
                    dbcs_color(str,yw_Red(ywd,YPACOLOR_TEXT_LIST),yw_Green(ywd,YPACOLOR_TEXT_LIST),
                                   yw_Blue(ywd,YPACOLOR_TEXT_LIST));
                    }
                }

            str = yw_BuildColumnItem(ywd,str,2,col);
            new_font(str,STAT_MENUUP_FONT);
            put(str,'}');   // rechter Menü-Rand
            new_line(str);
            }
        }


    /*** Korrektur, wenn weniger ***/
    if( i <= NUM_DISK_SHOWN ) {

        GSR->dmenu.ShownEntries = i;

        /*** Noch etwas auffüllen ***/
        j        = NUM_DISK_SHOWN - i;
        mode_str = " ";

        while( j-- )
            str = yw_MenuLayoutItem(ywd, &(GSR->dmenu), str, mode_str, 0);
        }

    GSR->dmenu.NumEntries = i;

    /*** zeichne unteren Rand ***/
    str = yw_LVItemsPostLayout(ywd, &(GSR->dmenu), str, STAT_MENUUP_FONT, "xyz");

    /*** EOS (Edge of Sanity??? ) ***/
    *str++ = 0x0;
    *str++ = 0x0;

    /*** Zeichnen ***/
    dt.string = GSR->dmenu.Req.req_string;
    dt.clips  = GSR->dmenu.Req.req_clips;
    _DrawText(&dt);
}
///


/// "yw_DrawLocaleMenu"
void yw_DrawLocaleMenu(struct ypaworld_data *ywd, struct GameShellReq *GSR)
/*
**  FUNCTION
**      Layoutet InputMenu komplett.
**
**  CHANGED
**      28-Apr-96   8100 000C geklaut vom Floh, früh, kurz nach Miternacht
*/
{
    ULONG  i;
    struct drawtext_args dt;
    struct localeinfonode *nd;
    BYTE   *str = GSR->lmenu.Itemblock;

    /*** Die Position des Requesters und damit des LV kann sich ändern ***/
    yw_ListSetRect(ywd, &(GSR->lmenu), -2, -2);

    /*** das letztendliche Layout ***/
    str = yw_LVItemsPreLayout(ywd, &(GSR->lmenu), str, STAT_MENUUP_FONT, "uvw");

    /*** alle Items zeichnen, Nummern beginnen bei 1 ***/
    for( nd = (struct localeinfonode *) GSR->localelist.mlh_Head, i= 0;
         nd->node.mln_Succ;
         nd = (struct localeinfonode *) nd->node.mln_Succ, i++ ) {

        UBYTE *mode_str;

        mode_str = nd->language;

        /*** Noch im darzustellenden Bereich? ***/
        if( (i >= GSR->lmenu.FirstShown) &&
            (i < (GSR->lmenu.ShownEntries + GSR->lmenu.FirstShown) ) ) {

            /* ---------------------------------------------------------------
            ** Sonderfall Selected Menuitem. Achtung, new_lsel ist ausgewaehl-
            ** tes, lsel wird dann erst uebernommen.
            ** -------------------------------------------------------------*/
            if( i == GSR->lmenu.Selected ) {
                str = yw_MenuLayoutSelItem(ywd, &(GSR->lmenu), str, mode_str, 0);
            } else {
                str = yw_MenuLayoutItem(ywd, &(GSR->lmenu), str, mode_str, 0);
            }
        }
    };

    /*** Korrektur, wenn weniger ***/
    if( i <= NUM_LOCALE_SHOWN )
        GSR->lmenu.ShownEntries = i;
    else
        GSR->lmenu.ShownEntries = NUM_LOCALE_SHOWN;
    GSR->lmenu.NumEntries = i;

    /*** Auffüllen, wenn NUM_NET_SHOWN < ShownEntries ***/
    if( GSR->lmenu.ShownEntries < NUM_LOCALE_SHOWN )
        str = yw_AddEmptyLinesToMenu( GSR->ywd, &(GSR->lmenu), str,
                                      NUM_LOCALE_SHOWN - GSR->lmenu.ShownEntries);

    /*** zeichne unteren Rand ***/
    str = yw_LVItemsPostLayout(ywd, &(GSR->lmenu), str, STAT_MENUUP_FONT, "xyz");

    /*** EOS (Edge of Sanity??? ) ***/
    *str++ = 0x0;
    *str++ = 0x0;

    /*** Zeichnen ***/
    dt.string = GSR->lmenu.Req.req_string;
    dt.clips  = GSR->lmenu.Req.req_clips;
    _DrawText(&dt);
}
///


#ifdef __NETWORK__
/// "yw_DrawNetMenu"
void yw_DrawNetMenu(struct ypaworld_data *ywd, struct GameShellReq *GSR)
/*
**  FUNCTION
**      Layoutet NetMenu komplett je nach Status.
**
**  CHANGED
**      28-Apr-96   8100 000C geklaut vom Floh, früh, kurz nach Miternacht
*/
{
    ULONG  i, count, merke;
    struct drawtext_args dt;
    BYTE   *str = GSR->nmenu.Itemblock;
    struct getprovidername_msg gpn;
    struct getsessionname_msg gsn;
    struct setstring_msg ss;
    struct LevelNode *level;
    char block[ 300 ],block_size[ 300 ], block_plys[ 300 ];
    char block_msg[ 300 ], block_name[ 300 ], block_slow[10];
    struct VFMFont *fnt;

    /*** Den Font für Menüs ermitteln ***/
    fnt = _GetFont( STAT_MENUUP_FONT );

    /*** Die Position des Requesters und damit des LV kann sich ändern ***/
    yw_ListSetRect(ywd, &(GSR->nmenu), -2, -2);

    /*** das letztendliche Layout ***/
    str = yw_LVItemsPreLayout(ywd, &(GSR->nmenu), str, STAT_MENUUP_FONT, "uvw");

    /*** Vorbereitungen für eventuelles Setzen ***/
    ss.number       = GSID_NETSTRING;
    ss.pressed_text = GSR->N_Name;

    count = -1;

    /*** alle Items zeichnen, Nummern beginnen bei 1 ***/
    for( i= 0;; i++ ) {

        BOOL   raus = TRUE;

        memset( block,      0, 300);
        memset( block_size, 0, 300);
        memset( block_plys, 0, 300);
        memset( block_name, 0, 300);
        memset( block_msg,  0, 300);

        switch( GSR->n_selmode ) {

            case NM_PROVIDER:

                gpn.number = i;
                if( _methoda( GSR->ywd->nwo, NWM_GETPROVIDERNAME, &gpn ) ) {
                    yw_LocStrCpy( block, gpn.name );
                    raus        = FALSE;

                    if( (-1 == GSR->NSel) && (0 == i) ) {

                        /*** keiner ausgewählt aber einer da --> setzen ***/
                        yw_LocStrCpy( GSR->N_Name, gpn.name );
                        ss.unpressed_text = GSR->N_Name;
                        _methoda( GSR->bnet, BTM_SETSTRING, &ss );
                        GSR->NSel = 0;
                        }
                    }
                else {
                    raus = TRUE;
                    strcpy( block, "---");
                    }

                break;

            case NM_SESSIONS:

                gsn.number = i;
                if( _methoda( GSR->ywd->nwo, NWM_GETSESSIONNAME, &gsn ) ) {

                    int k = 0, j = 0, n;
                    char tzs[] = SEPHOSTFROMSESSION;

                    /*** Level-Name rausfiltern (Space ist Trennzeichen) ***/
                    while( (gsn.name[ k ] != tzs[0]) &&
                           (gsn.name[ k ] >  31    ) )
                        block[ j++ ] = gsn.name[ k++ ];
                    block[ j ] = 0;

                    /* -------------------------------------------------
                    ** aus Levelnummer den Namen ermitteln. Diesen bitte
                    ** auch lokalisieren.
                    ** -----------------------------------------------*/
                    n = atoi( block );
                    strcpy( block, ypa_GetStr( GlobalLocaleHandle,
                                         STR_NAME_LEVELS + n,
                                         GSR->ywd->LevelNet->Levels[ n ].title) );

                    /*** Host-Name rausfiltern. Da kommt noch ein versionsidentifier ***/
                    j = 0;
                    k++;
                    while( (gsn.name[ k ] >  31) &&
                           (gsn.name[ k ] != tzs[0]) )
                        block_name[ j++ ] = gsn.name[ k++ ];
                    block_name[ j ] = 0;
                    raus        = FALSE;

                    if( (-1 == GSR->NSel) && (0 == i) ) {

                        /*** keiner ausgewählt aber einer da --> setzen ***/
                        char session_name[ 300 ];
                        strcpy( session_name, gsn.name );
                        strtok( session_name, SEPHOSTFROMSESSION );
                        n = atoi( session_name );
                        strcpy( GSR->N_Name, GSR->ywd->LevelNet->Levels[ n ].title );

                        ss.unpressed_text = GSR->N_Name;
                        _methoda( GSR->bnet, BTM_SETSTRING, &ss );
                        GSR->NSel = 0;
                        }
                    }
                else {
                    raus = TRUE;
                    strcpy( block, "---");
                    }

                break;

            case NM_LEVEL:

                /*** Da ist alles anders. Wir helfen uns mit hacks ***/
                if( i < GSR->num_netlevels ) {

                    level = &(GSR->ywd->LevelNet->Levels[ GSR->netlevel[ i ].number ]);
                    
                    /*** Schon zuviele Spieler da? ***/
                    if( level->num_players < _methoda( GSR->ywd->nwo, NWM_GETNUMPLAYERS, NULL) )
                        continue;

                    yw_LocStrCpy( block, GSR->netlevel[ i ].name );
                    count++;
                    raus = FALSE;
                    sprintf( block_size, "%d X %d\0", level->size_x, level->size_y );
                    
                    strcpy( block_slow, " " );
                    
                    /*** Welche Rassen sind im Spiel? ***/
                    if( i != GSR->NSel ) {
                        if( level->races & 2 )  strcat( block_plys, "P" );
                        if( level->races & 64 ) strcat( block_plys, "R" );
                        if( level->races & 8 )  strcat( block_plys, "T" );
                        if( level->races & 16 ) strcat( block_plys, "V" );
                        
                        if( level->slow_conn )
                            strcpy( block_slow, "X" );    
                        }
                    else {   
                        if( level->races & 2 )  strcat( block_plys, "1" );
                        if( level->races & 64 ) strcat( block_plys, "2" );
                        if( level->races & 8 )  strcat( block_plys, "3" );
                        if( level->races & 16 ) strcat( block_plys, "4" );
                        
                        if( level->slow_conn )
                            strcpy( block_slow, "Y" );    
                        }
                        
                    if( (-1 == GSR->NSel) && (0 == count) ) {

                        /*** keiner ausgewählt aber einer da --> setzen ***/
                        yw_StrUpper( GSR->N_Name, GSR->netlevel[ i ].name );
                        ss.unpressed_text = GSR->N_Name;
                        _methoda( GSR->bnet, BTM_SETSTRING, &ss );

                        /*** Levelnummer ***/
                        GSR->NLevelName   = GSR->netlevel[ i ].name;
                        GSR->NLevelOffset = GSR->netlevel[ i ].number;
                        GSR->NSel = 0;
                        }
                    }
                else
                    raus = TRUE;
                break;

            case NM_PLAYER:
            case NM_MESSAGE:

                if( i <  GSR->act_messageline ) {

                    yw_LocStrCpy( block, GSR->messagebuffer[ i ] );
                    raus = FALSE;
                    }
                else
                    raus = TRUE;

                break;
            }

        if( raus ) break;

        /*** Level-Hack ***/
        if( GSR->n_selmode == NM_LEVEL ) { merke = i; i = count; count = merke; }

        /*** Noch im darzustellenden Bereich? ***/
        if( (i >= GSR->nmenu.FirstShown) &&
            (i < (GSR->nmenu.ShownEntries + GSR->nmenu.FirstShown) ) ) {

            struct ypa_ColumnItem col[6];
            UBYTE  fnt_id;
            UBYTE  space_chr;
            UBYTE  prefix_chr;
            UBYTE  postfix_chr;
            ULONG  width, num_items;

            /* ------------------------------------------
            ** Spaltenaufbauen. Im Player und Messagemode
            ** gibt es keine selectierten Zeilen 
            ** ----------------------------------------*/
            memset(col,0,sizeof(col));
            if( (i == GSR->NSel) &&
                (NM_MESSAGE != GSR->n_selmode) &&
                (NM_PLAYER  != GSR->n_selmode)  ) {
                fnt_id      = STAT_MENUDOWN_FONT;
                space_chr   = 'c';
                prefix_chr  = 'b';
                postfix_chr = 'd';
            } else {
                fnt_id      = STAT_MENUUP_FONT;
                space_chr   = 'f';
                prefix_chr  = 'f';
                postfix_chr = 'f';
            };
 
            width      = (LONG)( GSR->nmenu.ActEntryWidth - 2 * GSR->ywd->EdgeW );

            switch( GSR->n_selmode ) {

                case NM_PROVIDER:
            
                    col[0].string      = block;
                    col[0].width       = (WORD)(width);
                    col[0].font_id     = fnt_id;
                    col[0].space_chr   = space_chr;
                    col[0].prefix_chr  = prefix_chr;
                    col[0].postfix_chr = postfix_chr;
                    col[0].flags       = YPACOLF_ALIGNLEFT | YPACOLF_DOPOSTFIX | YPACOLF_DOPREFIX | YPACOLF_TEXT;
                    
                    num_items = 1;

                break;

                case NM_SESSIONS:
                    
                    col[0].string      = block_name;
                    col[0].width       = (WORD)(0.5 * width);
                    col[0].font_id     = fnt_id;
                    col[0].space_chr   = space_chr;
                    col[0].prefix_chr  = prefix_chr;
                    col[0].postfix_chr = postfix_chr;
                    col[0].flags       = YPACOLF_ALIGNLEFT | YPACOLF_DOPREFIX | YPACOLF_TEXT;

                    col[1].string      = " ";
                    col[1].width       = 10;
                    col[1].font_id     = fnt_id;
                    col[1].space_chr   = space_chr;
                    col[1].prefix_chr  = prefix_chr;
                    col[1].postfix_chr = postfix_chr;
                    col[1].flags       = YPACOLF_ALIGNLEFT | YPACOLF_TEXT;

                    col[2].string      = block;
                    col[2].width       = (WORD)(width - 0.5 * width -10);
                    col[2].font_id     = fnt_id;
                    col[2].space_chr   = space_chr;
                    col[2].prefix_chr  = prefix_chr;
                    col[2].postfix_chr = postfix_chr;
                    col[2].flags       = YPACOLF_ALIGNLEFT | YPACOLF_DOPOSTFIX | YPACOLF_TEXT;
                    
                    num_items = 3;

                break;

                case NM_LEVEL:

                    fnt = _GetFont( FONTID_GADGET );
                    
                    col[0].string      = block;
                    col[0].width       = (WORD)(0.6 * width);
                    col[0].font_id     = fnt_id;
                    col[0].space_chr   = space_chr;
                    col[0].prefix_chr  = prefix_chr;
                    col[0].postfix_chr = postfix_chr;
                    col[0].flags       = YPACOLF_ALIGNLEFT | YPACOLF_DOPREFIX | YPACOLF_TEXT;

                    col[1].string      = block_size;
                    col[1].width       = (WORD)(0.15 * width);
                    col[1].font_id     = fnt_id;
                    col[1].space_chr   = space_chr;
                    col[1].prefix_chr  = prefix_chr;
                    col[1].postfix_chr = postfix_chr;
                    col[1].flags       = YPACOLF_ALIGNLEFT | YPACOLF_TEXT;
                    
                    col[2].string      = block_plys;
                    //col[2].width       = (WORD)(fnt->fchars['P'].width * strlen( block_plys) + 6);
                    col[2].width       = (WORD)(fnt->fchars['P'].width * 4 + 6);
                    if( GSR->NSel == i )
                        col[2].font_id = FONTID_MENUDOWN;
                    else
                        col[2].font_id = FONTID_GADGET;
                    col[2].space_chr   = space_chr;
                    col[2].prefix_chr  = prefix_chr;
                    col[2].postfix_chr = postfix_chr;
                    col[2].flags       = YPACOLF_ALIGNLEFT;
                    
                    col[3].string      = block_slow;
                    col[3].width       = (WORD)(fnt->fchars['P'].width );
                    col[3].font_id     = FONTID_GADGET;
                    col[3].space_chr   = space_chr;
                    col[3].prefix_chr  = prefix_chr;
                    col[3].postfix_chr = postfix_chr;
                    col[3].flags       = YPACOLF_ALIGNLEFT;
                    
                    col[4].string      = " ";
                    col[4].width       = (WORD)(width  - col[0].width -
                                          col[1].width - col[2].width -
                                          col[3].width);
                    col[4].font_id     = fnt_id;
                    col[4].space_chr   = space_chr;
                    col[4].prefix_chr  = prefix_chr;
                    col[4].postfix_chr = postfix_chr;
                    col[4].flags       = YPACOLF_ALIGNLEFT |  YPACOLF_DOPOSTFIX;
                  
                    num_items = 5;

                break;

                case NM_PLAYER:
                case NM_MESSAGE:

                    /*** Nur Eintraege aus dem Messagebuffer darstellen. ***/
                    col[0].string      = block;
                    col[0].width       = width;
                    col[0].font_id     = fnt_id;
                    col[0].space_chr   = space_chr;
                    col[0].prefix_chr  = prefix_chr;
                    col[0].postfix_chr = postfix_chr;
                    col[0].flags       = YPACOLF_ALIGNLEFT | YPACOLF_DOPOSTFIX |
                                         YPACOLF_TEXT | YPACOLF_DOPREFIX ;
                    num_items = 1;

                break;
                }
                
            /*** Jetzt darstellen ***/
            new_font(str,STAT_MENUUP_FONT);
            put(str,'{');   // linker Menü-Rand
            #ifdef __DBCS__
            if( (i == GSR->NSel) &&
                (NM_MESSAGE != GSR->n_selmode) &&
                (NM_PLAYER  != GSR->n_selmode) ) {
                dbcs_color(str,yw_Red(ywd,YPACOLOR_TEXT_LIST_SEL),yw_Green(ywd,YPACOLOR_TEXT_LIST_SEL),
                               yw_Blue(ywd,YPACOLOR_TEXT_LIST_SEL));
                }
            else {
                dbcs_color(str,yw_Red(ywd,YPACOLOR_TEXT_LIST),yw_Green(ywd,YPACOLOR_TEXT_LIST),
                               yw_Blue(ywd,YPACOLOR_TEXT_LIST));
                }
            #endif
            str = yw_BuildColumnItem(ywd,str,num_items,col);
            new_font(str,STAT_MENUUP_FONT);
            put(str,'}');   // rechter Menü-Rand
            new_line(str);
            }

        /*** Level-Hack ***/
        if( GSR->n_selmode == NM_LEVEL ) { merke = i; i = count; count = merke; }
        };

    /*** Level-Hack ***/
    if( GSR->n_selmode == NM_LEVEL ) i = count + 1; // weil nach erstem count schon 1 is

    /*** Korrektur, wenn weniger ***/
    if( i <= GSR->nmenu.MaxShown )
        GSR->nmenu.ShownEntries = i;
    else
        GSR->nmenu.ShownEntries = GSR->nmenu.MaxShown;

    /*** Auffüllen, wenn NUM_NET_SHOWN < ShownEntries ***/
    if( GSR->nmenu.ShownEntries < GSR->nmenu.MaxShown )
        str = yw_AddEmptyLinesToMenu( GSR->ywd, &(GSR->nmenu), str,
                                      GSR->nmenu.MaxShown - GSR->nmenu.ShownEntries);

    GSR->nmenu.NumEntries = i;

    /*** zeichne unteren Rand ***/
    str = yw_LVItemsPostLayout(ywd, &(GSR->nmenu), str, STAT_MENUUP_FONT, "xyz");

    /*** EOS (Edge of Sanity??? ) ***/
    *str++ = 0x0;
    *str++ = 0x0;

    /*** Zeichnen ***/
    dt.string = GSR->nmenu.Req.req_string;
    dt.clips  = GSR->nmenu.Req.req_clips;
    _DrawText(&dt);
}
///
#endif


char *yw_AddEmptyLinesToMenu( struct ypaworld_data *ywd, struct YPAListReq *Menu,
                             char *str, int number )
{
    while( number-- )
        str = yw_MenuLayoutItem( ywd, Menu, str, " ", 0);

    return( str );
}


/// "2 ToUpper-Routinen"
void yw_StrUpper( char *ziel, char *quelle )
{
    int i = 0;
    while( quelle[ i ] ) {

        ziel[ i ] = toupper( quelle[ i ] );
        i++;
        }

    ziel[ i ] = 0;
}

void yw_StrUpper2( char *ziel, char *quelle )
{
    /*** Korrigiert auch gleichzeitig ***/
    int i = 0;
    while( quelle[ i ] ) {

        ziel[ i ] = toupper( quelle[ i ] );
        if( (ziel[ i ] < 32) || (ziel[ i ] > 90) ) ziel[ i ] = '*';
        i++;
        }

    ziel[ i ] = 0;
}
///


/// "yw_PosSelected"
void yw_PosSelected( struct YPAListReq *req, WORD actual )
{
    /* ---------------------------------------------------------------------
    ** Positioniert das selektiert Item so, daß es möglichst oben steht und
    ** berechnet dazu FirstShown in Abhängigkeit von actual, NumEntries und
    ** ShownEntries neu. Bei actual zählen wir ab 0, einige Requester müssen
    ** da aufpassen, weil die ab 1 zählen
    ** -------------------------------------------------------------------*/

    /*
    ** Wenn "danach" noch ShownEntries nach actual frei sind, dann First-
    ** shown = actual. Das machen wir aber nur, wenn es das selektierte
    ** derzeit nicht sichtbar ist!
    */
    if( (actual <  req->FirstShown) ||
        (actual > (req->FirstShown + req->MaxShown - 1) ) ) {

        if( (req->NumEntries - actual) > req->MaxShown )
            req->FirstShown = actual;
        else
            req->FirstShown = req->NumEntries - req->MaxShown;
        }
    else {

        /*** Es kann trotzdem noch sein, daß der Sichtbereich sich verändert hat ***/
        if( (req->FirstShown + req->MaxShown) > req->NumEntries )
            req->FirstShown = req->NumEntries - req->MaxShown;

        if( req->FirstShown < 0 )   req->FirstShown = 0;
        }
}
///


/// "yw_GSMessage"
void yw_GSMessage( struct GameShellReq *GSR, char *message )
{
/* ----------------------------------------------------------------
** Gibt im unteren Balken eine Message aus und erzeugt (später) ein 
** Error-geräusch
** --------------------------------------------------------------*/

    struct setstring_msg ss;

    ss.number         = GSID_UNTENINFO;
    ss.unpressed_text = message;
    ss.pressed_text   = NULL;
    _methoda( GSR->Titel, BTM_SETSTRING, &ss );
}
///


/// "yw_ActualizeInputRequester"
void yw_ActualizeInputRequester( struct GameShellReq *GSR )
{

    struct setbuttonstate_msg sbs;

    if( GSR->ywd->Prefs.Flags & YPA_PREFS_JOYDISABLE )
        sbs.state = SBS_UNPRESSED;
    else
        sbs.state = SBS_PRESSED;
    sbs.who = GSID_JOYSTICK;
    _methoda( GSR->binput, BTM_SETSTATE, &sbs );

    if( GSR->altjoystick )
        sbs.state = SBS_PRESSED;
    else
        sbs.state = SBS_UNPRESSED;
    sbs.who = GSID_ALTJOYSTICK;
    _methoda( GSR->binput, BTM_SETSTATE, &sbs );

    if( GSR->ywd->Prefs.Flags & YPA_PREFS_FFDISABLE )
        sbs.state = SBS_UNPRESSED;
    else
        sbs.state = SBS_PRESSED;
    sbs.who = GSID_FORCEFEEDBACK;
    _methoda( GSR->binput, BTM_SETSTATE, &sbs );

    /*** jetzt kommen nur noch I_Act-abhängige Sachen.... ***/
    if( !GSR->i_actualitem ) return;

    yw_PosSelected( &(GSR->imenu), GSR->i_actualitem - 1);
}
///


/// "yw_GameShellToolTips"
void yw_GameShellToolTips( struct GameShellReq *GSR, ULONG who )
{
    /*** Riesiges switch-Statement für zu erklärende Gadgets ***/
    switch( who ) {

        /*** Der Titelscreen ***/
        case GSID_GAME:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_PLAY );
            break;

        case GSID_DISK:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_OCDISK );
            break;

        case GSID_INPUT:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_OCINPUT );
            break;

        case GSID_VIDEO:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_OCVIDEO );
            break;

        case GSID_LOCALE:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_OCLOCALE );
            break;

        case GSID_SOUND:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_OCSOUND );
            break;

        case GSID_NET:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_OCNET );
            break;

        case GSID_QUIT:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_QUITGAME );
            break;

        case GSID_HELP:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_HELP );
            break;

        /*** Spiel-Statusbalken ***/
        case GSID_PL_QUIT:

            if( FALSE==yw_MBActive(GSR->ywd) )
                yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_PL_TOTITLE );
            else {
                if(GSR->ywd->Level->Status == LEVELSTAT_DEBRIEFING)
                    yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_EXITDEBRIEFING );            
                else
                    yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_PL_TOPLAY );
                }
            break;

        case GSID_PL_SETBACK:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_PL_SETBACK );
            break;

        case GSID_PL_FASTFORWARD:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_PL_FASTFORWARD );
            break;

        case GSID_PL_GAME:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_PL_GAME );
            break;

        case GSID_PL_LOAD:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_PL_LOAD );
            break;
 
        case GSID_PL_GOTOLOADSAVE:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_GOTOLOADSAVE );
            break;
            

        /*** Der Netzwerkrequester ***/
        case GSID_NETCANCEL:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_NETCANCEL );
            break;

        case GSID_NETHELP:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_HELP );
            break;

        case GSID_NETSEND:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_SENDMESSAGE );
            break;

        case GSID_NETOK:

            switch( GSR->n_selmode ) {

                case NM_PROVIDER:

                    yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_OKPROVIDER );
                    break;

                case NM_PLAYER:

                    yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_OKPLAYER );
                    break;

                case NM_SESSIONS:

                    yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_OKSESSION );
                    break;

                case NM_LEVEL:

                    yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_OKLEVEL );
                    break;

                case NM_MESSAGE:

                    yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_STARTLEVEL );
                    break;
                }
            break;

        case GSID_NETBACK:

            switch( GSR->n_selmode ) {

                case NM_PLAYER:

                    yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_BACKTOPROVIDER );
                    break;

                case NM_SESSIONS:

                    yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_BACKTOPLAYER );
                    break;

                case NM_LEVEL:

                    yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_BACKTOSESSION );
                    break;

                case NM_MESSAGE:

                    yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_BACKTOSESSION );
                    break;
                }
            break;

        case GSID_NETNEW:

            switch( GSR->n_selmode ) {

                case NM_SESSIONS:

                    yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_NEWLEVEL );
                    break;

                case NM_MESSAGE:

                    yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_SEND );
                    break;
                }
            break;

        case GSID_USERRACE:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_USERRACE );
            break;

        case GSID_KYTERNESER:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_KYTERNESER );
            break;

        case GSID_MYKONIER:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_MYKONIER );
            break;

        case GSID_TAERKASTEN:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_TAERKASTEN );
            break;

        case GSID_NETREADY:

            if( GSR->ReadyToStart )
                yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_NOTREADY );
            else
                yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_READY );
            break;

        /*** Der Diskrequester ***/
        case GSID_LOAD:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_LOAD );
            break;

        case GSID_NEWGAME:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_NEWGAME );
            break;

        case GSID_DELETE:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_DELETE );
            break;

        case GSID_SAVE:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_SAVE );
            break;

        case GSID_DISKHELP:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_HELP );
            break;

        case GSID_DISKOK:

            switch( GSR->D_InputMode ) {

                case DIM_LOAD:

                    yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_OKLOAD );
                    break;

                case DIM_SAVE:

                    yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_OKSAVE );
                    break;

                case DIM_CREATE:

                    yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_OKCREATE );
                    break;

                case DIM_KILL:

                    yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_OKDELETE );
                    break;
                }
            break;

        case GSID_DISKCANCEL:

            switch( GSR->D_InputMode ) {

                case DIM_NONE:

                    yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_DISKCANCEL );
                    break;

                case DIM_LOAD:

                    yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_CANCELLOAD );
                    break;

                case DIM_SAVE:

                    yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_CANCELSAVE );
                    break;

                case DIM_CREATE:

                    yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_CANCELCREATE );
                    break;

                case DIM_KILL:

                    yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_CANCELDELETE );
                    break;
                }
            break;

        /*** Der Settingsrequester ***/
        case GSID_SETTINGSHELP:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_HELP );
            break;

        case GSID_SETTINGSCANCEL:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_SETTINGSCANCEL );
            break;

        case GSID_SETTINGSOK:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_SETTINGSOK );
            break;

        case GSID_VMENU_BUTTON:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_RESOLUTION );
            break;

        case GSID_3DMENU_BUTTON:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_SETTINGS3D );
            break;

        case GSID_16BITTEXTURE:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_16BITTEXTURE );
            break;

        case GSID_DRAWPRIMITIVE:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_DRAWPRIMITIVE );
            break;

        case GSID_SOUND_LR:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_SOUNDLR );
            break;

        case GSID_FARVIEW:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_FARVIEW );
            break;

        case GSID_HEAVEN:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_HEAVEN );
            break;

        case GSID_CDSOUND:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_CDSOUND );
            break;

       case GSID_SOFTMOUSE:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_SOFTMOUSE );
            break;

        case GSID_ENEMYINDICATOR:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_ENEMYINDICATOR );
            break;

        case GSID_FXVOLUMESLIDER:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_FXVOLUME );
            break;

        case GSID_CDVOLUMESLIDER:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_CDVOLUME );
            break;

        case GSID_FXSLIDER:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_FXSLIDER );
            break;

        /*** Der Inputrequester ***/
        case GSID_INPUTOK:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_INPUTOK );
            break;

        case GSID_INPUTHELP:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_HELP );
            break;

        case GSID_INPUTCANCEL:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_INPUTCANCEL );
            break;

        case GSID_SWITCHOFF:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_SWITCHOFF );
            break;

        case GSID_INPUTRESET:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_INPUTRESET );
            break;

        case GSID_JOYSTICK:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_JOYSTICK );
            break;

        case GSID_FORCEFEEDBACK:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_FORCEFEEDBACK );
            break;

        case GSID_ALTJOYSTICK:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_ALTJOYSTICK );
            break;

        /*** Der Lokalerequester ***/
        case GSID_LOCALEOK:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_LOCALEOK );
            break;

        case GSID_LOCALECANCEL:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_LOCALECANCEL );
            break;

        case GSID_LOCALEHELP:

            yw_Tooltip( GSR->ywd, TOOLTIP_SHELL_HELP );
            break;
        }
}
///

       
/// "yw_PlayShellSamples"
void yw_PlayShellSamples( struct GameShellReq *GSR )
{

}
///


/// "Sortieren der Netzwerklevel"
int yw_SortHook( void *nlevel1, void *nlevel2 )
{
    struct netlevel *nl1 = nlevel1;
    struct netlevel *nl2 = nlevel2;

    #ifdef __DBCS__
    return( stricmp( nl1->name, nl2->name ) );    // ???
    #else
    return( stricmp( nl1->name, nl2->name ) );
    #endif
}


void yw_SortNetworkLevels( struct GameShellReq *GSR )
{
    /* -----------------------------------------------------------------
    ** Wir sortieren die Level fuer das netzwerkspiel. Zuerst schreiben
    ** wir sie mit Lokalisierung in ein array, anschliessend wird dieses
    ** Array vorerst unter Missachtung von DBCS sortiert.
    ** Weil wir zum Zeichnen der Levelnamen aber nicht einfach das neue
    ** Namensarray durchgehen koennen (wegen der Levelinformationen),
    ** muss ich dem Namenseintrag die Levelnummer mitgeben, damit die
    ** Levelnode beim Zeichnen gefunden werden kann.
    **
    ** Jetzt muesste man maps mit Sortiermethode haben...
    ** ---------------------------------------------------------------*/
    WORD offset, level;

    /*** Lokalisieren und Levelnummern eintragen ***/
    for( level = 0, offset = 0; level < MAXNUM_LEVELS; level++ ) {

        /*** Netzlevel? ***/
        if( GSR->ywd->LevelNet->Levels[ level ].status != LNSTAT_NETWORK )
            continue;

        /*** registrieren ***/
        GSR->netlevel[ offset ].number = level;
        GSR->netlevel[ offset ].name   = ypa_GetStr( GlobalLocaleHandle,
                                         STR_NAME_LEVELS + level,
                                         GSR->ywd->LevelNet->Levels[ level ].title);

        offset++;
        }

    GSR->num_netlevels = offset;

    /*** Sortieren ***/
    qsort( GSR->netlevel, GSR->num_netlevels, sizeof(struct netlevel),
           yw_SortHook );
}
///


/// "network info ins logfile"
void yw_PrintNetworkInfoStart( struct GameShellReq *GSR )
{
    struct getplayerdata_msg gpd;
    struct getprovider_msg gp;

    if( !yw_OpenNetScript() ) {

        _LogMsg("Unable to open Network log script\n");
        return;
        }

    yw_NetLog("-------------- Start YPA network session ------------------\n\n");

    _methoda( GSR->ywd->nwo, NWM_GETPROVIDER, &gp );
    if( gp.name )
        yw_NetLog("Provider: %s\n", gp.name );
    else
        yw_NetLog("!!! Unknown provider\n");
    yw_NetLog("for this povider I send a dplay-msg after %d ms\n", GSR->flush_time_normal);

    yw_NetLog("Following players startet with the game:\n");
    gpd.askmode = GPD_ASKNUMBER;
    gpd.number  = 0;
    while( _methoda( GSR->ywd->nwo, NWM_GETPLAYERDATA, &gpd ) ) {

        UBYTE foundrace;

        if( gpd.flags & NWFP_OWNPLAYER)
            foundrace = GSR->SelRace;
        else
            foundrace = GSR->player2[ gpd.number ].race;
        switch( foundrace ) {

            case FREERACE_USER:
                yw_NetLog("    %s and plays Resistance\n", gpd.name);
                        break;
            case FREERACE_MYKONIER:
                yw_NetLog("    %s and plays Mykonier\n", gpd.name);
                        break;
            case FREERACE_TAERKASTEN:
                yw_NetLog("    %s and plays Taerkasten\n", gpd.name);
                        break;
            case FREERACE_KYTERNESER:
                yw_NetLog("    %s and plays Ghorkov\n", gpd.name);
                        break;
            default:
                yw_NetLog("    %s and plays an unknown race\n", gpd.name);
                        break;
            }
        gpd.number++;
        }

    if( GSR->is_host )
        yw_NetLog("\nThe local player is %s and is HOST\n", GSR->NPlayerName);
    else
        yw_NetLog("\nThe local player is %s and is CLIENT\n", GSR->NPlayerName);

    yw_NetLog("They play level %d, this is %s\n", GSR->NLevelOffset,
             ypa_GetStr( GlobalLocaleHandle, STR_NAME_LEVELS + GSR->NLevelOffset,
             GSR->ywd->LevelNet->Levels[ GSR->NLevelOffset ].title));
    yw_NetLog("the session started at timeindex %d\n", GSR->ywd->TimeStamp/1000);
    yw_NetLog("\n\n--------------------------------------------------------\n");
}

void yw_PrintNetworkInfoEnd( struct GameShellReq *GSR )
{
    struct diagnosis_msg dm;
    _methoda( GSR->ywd->nwo, NWM_DIAGNOSIS, &dm );

    yw_NetLog("---------------- YPA Network Statistics -------------------\n\n");
    yw_NetLog("Sending:\n");
    yw_NetLog("   bytes per second: %d\n", GSR->transfer_sbps_avr );
    yw_NetLog("   bps minimum:      %d\n", GSR->transfer_sbps_min );
    yw_NetLog("   bps maximum:      %d\n", GSR->transfer_sbps_max );
    yw_NetLog("   packet size:      %d\n", GSR->transfer_pckt_avr );
    yw_NetLog("   packet minimum:   %d\n", GSR->transfer_pckt_min );
    yw_NetLog("   packet maximum:   %d\n", GSR->transfer_pckt_max );
    yw_NetLog("receiving:\n");
    yw_NetLog("   bytes per second: %d\n", GSR->transfer_rbps_avr );
    yw_NetLog("   bps minimum:      %d\n", GSR->transfer_rbps_min );
    yw_NetLog("   bps maximum:      %d\n", GSR->transfer_rbps_max );
    yw_NetLog("statistical sr-thread info\n");
    yw_NetLog("   max. in send list %d\n", dm.max_send ); 
    yw_NetLog("   max. in recv list %d\n", dm.max_recv ); 
    yw_NetLog("\nthe session ended at timeindex %d\n", GSR->ywd->TimeStamp/1000);
    yw_NetLog("-----------------------------------------------------------\n");
}
///

void yw_MessageBox( struct ypaworld_data *ywd, char *title, char *text )
{
    /*** Messagebox mit Screen umschalten ***/
    _methoda( ywd->GfxObject, WINDDM_EnableGDI, NULL);

    yw_WinMessageBox( title, text );

    _methoda( ywd->GfxObject, WINDDM_DisableGDI, NULL);
}

void yw_RenderConfirmRequester( struct GameShellReq *GSR )
{
    struct ypaworld_data *ywd = GSR->ywd;
    
    if( CONFIRM_NONE == GSR->confirm_modus )
        return;
                
    /*** irgendwie Hintergrund malen ***/
    yw_PaintRect( GSR, GET_X_COORD(150), GET_Y_COORD(200), GET_X_COORD(340), GET_Y_COORD(80) );
            
    _methoda( GSR->confirm, BTM_PUBLISH, NULL );
}


void yw_PaintRect( struct GameShellReq *GSR, WORD x, WORD y, WORD w, WORD h )
{
/*
**  zeichnet ein Rechteck als Hintergrund fuer Messages.
*/

    WORD   value, count = 0;
    struct drawtext_args dt;
    BOOL   first;
    struct VFMFont *sfont;
    char   buffer[300], *str;

    sfont = _GetFont( FONTID_DEFAULT );
    
    str = buffer;
    new_font( str, FONTID_DEFAULT); 

    xpos_abs( str, (x - GSR->ywd->DspXRes/2));
    ypos_abs( str, (y - GSR->ywd->DspYRes/2)); 

    first      = TRUE;

    while( h > sfont->height ) {  // Fonthöhe immer gleich...

        /*** Eröffnung der Zeile ***/
        if( first ) {
            put( str, 'u');    // links oben
            }
        else {
            put( str, '{');      // rand
            }

        /*** ZwischenStücke ***/
        lstretch_to( str, w );
        if( first ) {
            put( str, 'v');        // rand oben
            }
        else {
            put( str, '{');        // frei
            }

        if( first ) {
            put( str, 'w');   // oben rechts
            }
        else {
            put( str, '}');     // rand
            }
            
        /*** new line ***/
        new_line( str );  
              
        first = FALSE;
        h -= sfont->height;
        }

    /*** Nun noch Rand der Rest-Höhe h ***/
    //off_vert( str, sfont->height - h);
    off_vert( str, sfont->height - 1);
    put( str, 'x');          // unten links
    
    lstretch_to( str, w );
    put( str, 'y');
    
    put( str, 'z');

    eos( str );
    
    /*** Zeichnen ***/
    dt.string = buffer;
    dt.clips  = NULL;
    _DrawText( &dt );
}
