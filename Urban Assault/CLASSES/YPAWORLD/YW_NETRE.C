/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_listview.c,v $
**  $Revision: 38.8 $
**  $Date: 1996/03/21 20:30:55 $
**  $Locker:  $
**  $Author: floh $
**
**  Routinen für Netzwerkrequester. Hält sich an Floh's
**  WeltklassenRequester-Vereinbarungen
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
#include "nucleus/math.h"
#include "engine/engine.h"
#include "ypa/ypaworldclass.h"
#include "ypa/guilist.h"
#include "ypa/ypagui.h"
#include "ypa/guinetwork.h"
#include "visualstuff/ov_engine.h"
#include "yw_protos.h"
#include "yw_gsprotos.h"
#include "ypa/yparoboclass.h"
#include "ypa/ypamissileclass.h"
#include "ypa/ypagunclass.h"
#include "bitmap/bitmapclass.h"

#ifdef __NETWORK__

#include "network/networkclass.h"
#include "ypa/ypamessages.h"
#include "thirdparty/eulerangles.h"
#include "yw_netprotos.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_input_engine
_extern_use_ov_engine
_extern_use_audio_engine

extern UBYTE  **GlobalLocaleHandle;
extern struct key_info GlobalKeyTable[ 256 ];

struct YPAMessageWin MW;

struct ClickButton MW_Iconify;
struct ClickButton MW_DragBar;
char   MW_ReqString[ 1024 ];
char   MsgName[ 300 ];
ULONG  ToOwner = 0;
LONG   KEYTIME = 0;

BOOL yw_InitMW( struct ypaworld_data *ywd )
{
    struct VFMFont *fnt = ywd->Fonts[ FONTID_DEFAULT ];

    /*** E bissel bar fläx ***/
    MW.Req.flags = REQF_Closed|REQF_HasDragBar|REQF_HasCloseGadget|REQF_HasHelpGadget;

    MW.min_width  = fnt->fchars[ 'a' ].width;
    MW.max_width  = ywd->DspXRes;
    MW.min_height = 2 * ywd->FontH;
    MW.max_height = 2 * ywd->FontH;

    if( ywd->Prefs.valid ) {

        /*** Position + Größe aus Prefs, + Randkorrektur ***/
        struct YPAWinPrefs *p = &(ywd->Prefs.WinMessage);
        struct ClickRect *r = &(MW.Req.req_cbox.rect);
        LONG max_w = ywd->DspXRes;
        LONG max_h = ywd->DspYRes - ywd->UpperTabu - ywd->LowerTabu;

        *r = p->rect;

        /*** MinSize Korrektur ***/
        if (r->w < MW.min_width)  r->w=MW.min_width;
        if (r->h < MW.min_height) r->h=MW.min_height;

        /*** Rand-Korrektur ***/
        if (r->x < 0)               r->x = 0;
        if (r->w > max_w)           r->w = max_w;
        if (r->h > max_h)           r->h = max_h;
        if (r->y < ywd->UpperTabu)  r->y = ywd->UpperTabu;
        if ((r->x + r->w) >= max_w) r->x = max_w - r->w;
        if ((r->y + r->h) >= max_h) r->y = ywd->UpperTabu + (max_h - r->h);
        }
    else {

        /*** Default-Einstellungen ***/
        MW.Req.req_cbox.rect.x = ywd->DspXRes / 5;
        MW.Req.req_cbox.rect.y = 2 * ywd->DspYRes / 3;
        MW.Req.req_cbox.rect.w = ywd->DspXRes/3;
        MW.Req.req_cbox.rect.h = 2*ywd->FontH + 2;
        }

    MW.Req.req_cbox.num_buttons = 2;

    /*** Die ersten beiden Buttons müssen so sein für allgemeine Ausw. ***/
    MW.Req.req_cbox.buttons[MWBTN_ICONIFY] = &MW_Iconify;
    MW.Req.req_cbox.buttons[MWBTN_DRAGBAR] = &MW_DragBar;

    MW_Iconify.rect.x = fnt->fchars[ 'a' ].width;
    MW_Iconify.rect.y = 0;
    MW_Iconify.rect.w = fnt->fchars[ 'a' ].width;
    MW_Iconify.rect.h = ywd->FontH;
    MW_DragBar.rect.x = 0;
    MW_DragBar.rect.y = 0;
    MW_DragBar.rect.w = fnt->fchars[ 'a' ].width;  // muß korrigiert werden
    MW_DragBar.rect.h = ywd->FontH;

    /*** Die Strings für die Textausgabe, keine clips ***/
    MW.Req.req_string  = MW_ReqString;
    MW.Req.req_clips   = NULL;

    MW.cursorpos = 0;
    MW.msg[0]    = 0;

    return( TRUE );
}


void yw_KillMW( struct ypaworld_data *ywd )
{
}


void yw_HandleInputMW( struct ypaworld_data *ywd, struct VFMInput *input )
{
    /* -----------------------------------------------------------------
    ** Neu: Alle Buttons werden gelöscht. Das heißt, während das Fenster
    ** offen ist, geht nichts mehr. Das ist besser so, denn schon Enter
    ** ist meist ein Button. Andernfalls wäre noch die Möglichkeit, nur
    ** die verwendeten Sachen auszufiltern, da müßte man aber erstmal
    ** rauskriegen, welcher Button welchem Key entspricht.
    ** ---------------------------------------------------------------*/
    LONG  num_players;
    struct VFMFont *fnt = ywd->Fonts[ FONTID_DEFAULT ];

    num_players = _methoda( ywd->nwo, NWM_GETNUMPLAYERS, NULL );

    /* -------------------------------------------------------------
    ** Hack: Wenn wir Enter gedrückt haben, geht trotzdem
    ** noch eine Rakete los, weil es nun mal ein Button ist. Deshalb
    ** lösche ich noch ne drittel Sekunde danach alle Buttons...
    ** -----------------------------------------------------------*/
    if( KEYTIME > 0 ) {

        KEYTIME -= input->FrameTime;
        input->HotKey  = 0;
        input->NormKey = 0;
        input->ContKey = 0;
        input->Buttons = 0; // och ne...
        memset( input->Slider, 0, 32 * sizeof( FLOAT ) );
        }

    /*** Hotkeys, welche den Requester öffnen ***/
    if( (MW.Req.flags & REQF_Closed) && (HOTKEY_TOALL == input->HotKey) ) {

        /*** Requester öffnen und Ziel festlegen ***/
        yw_LocStrCpy( MsgName, ypa_GetStr( GlobalLocaleHandle, STR_NET_ALL, "ALL") );

        /*** Nun je nach InputModus ***/
        if( ywd->UseSystemTextInput ) {

            ywd->chat_name   = MsgName;
            ywd->chat_owner  = 0;       // wird nicht mehr benoetigt
            ywd->chat_system = TRUE;
            }
        else {

            /*** Normal. Requester Öffnen ***/
            yw_OpenReq(ywd, (struct YPAReq *) &MW );
            _Remove((struct Node *) &MW );
            _AddHead((struct List *) &(ywd->ReqList), (struct Node *) &MW);
            }

        return;
        }

    /*** Normale tasten für die Message auswerten ***/
    if( !(MW.Req.flags & REQF_Closed) ) {

        /*** Spezielle Tasten ***/
        switch( input->HotKey ) {

            case HOTKEY_QUIT:

                /*** Cancel this action ***/
                yw_CloseReq(ywd, (struct YPAReq *) &MW );
                ToOwner       = 0;
                MW.cursorpos  = 0;
                MW.msg[ 0 ]   = 0;
                input->HotKey = 0;
                return;
                break;
            }

        /*** Eingabe des Textes ***/
        switch( input->NormKey ) {

            case 0:

                /*** War wohl nix ... ***/
                break;

            case KEYCODE_RETURN:

                /* ---------------------------------------------
                ** Wenn denn der Buffer nicht leer ist, schicken
                ** wir eine Message und schließen alles
                ** -------------------------------------------*/
                if( MW.msg[ 0 ] ) {

                    /*** Abschicken ***/
                    yw_SendChatMessage( ywd, MW.msg, MsgName, ToOwner );

                    /*** Schließen ***/
                    yw_CloseReq(ywd, (struct YPAReq *) &MW );
                    ToOwner = 0;

                    /*** Alles löschen, auch Slider wegen Cursortasten ***/
                    input->HotKey   = 0;
                    input->NormKey  = 0;
                    input->ContKey  = 0;
                    input->AsciiKey = 0;
                    input->Buttons  = 0; // och ne...
                    memset( input->Slider, 0, 32 * sizeof( FLOAT ) );

                    /*** Rücksetzen ***/
                    MW.cursorpos = 0;
                    MW.msg[ 0 ] = 0;

                    /*** Raus, weil kein Layout mehr ***/
                    KEYTIME = 300;
                    return;
                    }
                break;

            case KEYCODE_BS:

                /*** Zeichen vor Cursor löschen ***/
                if( MW.cursorpos > 0 ) {

                    char *p = &( MW.msg[ MW.cursorpos - 1] );
                    while( p[1] > 31 ) { p[0] = p[1]; p++; }
                    MW.msg[ MW.cursorpos - 1 ] = 0;
                    MW.cursorpos--;
                    }
                break;

            case KEYCODE_DEL:

                /*** Zeichen am Cursor löschen ***/
                if( MW.cursorpos < strlen( MW.msg ) ) {

                    char *p = &( MW.msg[ MW.cursorpos ] );
                    while( p[1] > 31 ) { p[0] = p[1]; p++; }
                    MW.cursorpos--;
                    }
                break;

            case KEYCODE_CURSOR_LEFT:

                if( MW.cursorpos > 0 ) MW.cursorpos--;
                break;

            case KEYCODE_CURSOR_RIGHT:

                if( MW.cursorpos < strlen( MW.msg ) ) MW.cursorpos++;
                break;
            }

        /* -------------------------------------
        ** Neu: Der Text über ASCII-Key
        ** Nur einsortieren, wenn noch Platz ist 
        ** -----------------------------------*/
        if( (input->AsciiKey > 31) &&
            (strlen( MW.msg ) < (CHATMSG_LEN - 3) ) ) {

            char ascii_in[10], ascii_out[10], buffer[ 300 ];

            /* ---------------------------------------------------------------
            ** Was nicht darstellbar ist, sollte auch nicht als * ausgegeben
            ** werden. Folglich jagen wir es durch Floh's Filterroutine und
            ** ignorieren alles, was als Stern rauskam, aber nicht als solcher
            ** rein ist!
            ** -------------------------------------------------------------*/
            sprintf( ascii_in, "%c\0", input->AsciiKey );
            yw_LocStrCpy( ascii_out, ascii_in );
            if( (ascii_in[0] != '*') && (ascii_out[0] == '*') )
                return;

            /*** Einsortieren ***/
            buffer[0] = 0; // !!
            strncpy( buffer, MW.msg, MW.cursorpos );
            strncpy( &(buffer[ MW.cursorpos ]), ascii_out, 1 );
            strcpy(  &(buffer[ MW.cursorpos + 1 ]), &( MW.msg[ MW.cursorpos]) );
            strcpy( MW.msg, buffer );
            MW.cursorpos++;  // Stringgadget unten neu
            }     

        /*** Tasten schlucken ***/
        input->HotKey   = 0;
        input->NormKey  = 0;
        input->ContKey  = 0;
        input->AsciiKey = 0;
        input->Buttons  = 0;
        memset( input->Slider, 0, 32 * sizeof( FLOAT ) );
        }

    /*** Zeichnen ***/
    if( !(MW.Req.flags & REQF_Closed) ) {

        UBYTE *str;
        WORD xpos, ypos, breite;
        char headline[ 300 ], msg[ 300 ];

        str = MW.Req.req_string;

        /*** Breite ermitteln ***/
        sprintf( headline, "%s %s", ypa_GetStr( GlobalLocaleHandle, STR_NET_MESSAGETO,
                                                "MESSAGE TO"), MsgName );
        
        #ifdef __DBCS__
        breite  = ywd->DspXRes -1;
        #else                                                
        breite  = max( strlen( headline ) + 1, strlen( MW.msg ) ) + 1;
        breite *= fnt->fchars[ 'W' ].width;
        #endif
        
        /*** Korrektur ***/
        if( breite > ywd->DspXRes - 1 ) {

            MW.msg[ MW.cursorpos - 1 ] = 0;
            MW.cursorpos--;
            breite -= fnt->fchars[ 'W' ].width;
            }

        if( breite + MW.Req.req_cbox.rect.x > ywd->DspXRes - 1 ) {
            MW.Req.req_cbox.rect.x -= fnt->fchars[ 'W' ].width;
            if( MW.Req.req_cbox.rect.x < 0 ) MW.Req.req_cbox.rect.x = 0;
            }

        /*** Vorbereitung ***/
        new_font( str, FONTID_DEFAULT );
        xpos = MW.Req.req_cbox.rect.x - (ywd->DspXRes>>1);
        ypos = MW.Req.req_cbox.rect.y - (ywd->DspYRes>>1);
        pos_abs(str, xpos, ypos);

        /*** Erste zeile mit Closegadget und Überschrift ***/
        str = yw_BuildReqTitle( ywd, xpos, ypos, breite, headline, str,
                                0, MW.Req.flags );
        new_line( str );

        /*** Zeile mit Message ***/
        new_font( str, FONTID_DEFAULT );
        put( str, '{' );
        msg[0] = 0;
        strncpy( msg, MW.msg, MW.cursorpos );
        msg[ MW.cursorpos ] = 0;
        strcat(  msg, "_" );                             // CURSORSIGN );
        strcat(  msg, &(MW.msg[ MW.cursorpos ]) );
        
        #ifdef __DBCS__
        freeze_dbcs_pos( str );
        #else
        str = yw_StpCpy( msg, str );
        #endif
        
        lstretch_to( str, breite - fnt->fchars[ '}' ].width);
        put( str, ' ');
        put( str, '}'); 
        
        /*** jetzt erst den DBCS-Text ***/
        #ifdef __DBCS__
        put_dbcs( str, breite - 2 * fnt->fchars[ 'W' ].width, DBCSF_LEFTALIGN, msg );
        #endif
        new_line( str );

        /*** Zeile mit abschluß ***/
        off_vert( str, fnt->height - 1 );
        put( str, 'x');
        lstretch_to( str, breite - fnt->fchars[ 'z' ].width);
        put( str, 'y');
        put( str, 'z');


        /*** Abschluß ***/
        eos( str );

        /*** Dragbarbreite aktualisieren ***/
        MW_DragBar.rect.w = breite - fnt->fchars[ 'a' ].width;
        MW_Iconify.rect.x = breite - fnt->fchars[ 'a' ].width;
        MW.Req.req_cbox.rect.w = breite;
        }


}


void yw_SendChatMessage( struct ypaworld_data *ywd, char *text,
                         char *name, UBYTE owner)
{
    struct sendmessage_msg sm;
    struct ypamessage_text tm;
    WORD   number;

    strcpy( tm.text, text );
    tm.generic.message_id  = YPAM_TEXT;
    tm.generic.owner       = ywd->gsr->NPlayerOwner;

    sm.receiver_id         = NULL;
    sm.receiver_kind       = MSG_ALL;
    sm.data                = &tm;
    sm.data_size           = sizeof( tm );
    sm.guaranteed          = TRUE;
    _methoda( ywd->world, YWM_SENDMESSAGE, &sm );

    /*** So es eine Nummer ist, noch Sample abfeuern ***/
    number = strtol( tm.text, NULL, 0 );
    if( number > 0 )
        yw_LaunchChatSample( ywd, number );
    else {
    
        /*** auch bei mir darstellen ***/
        struct logmsg_msg log;
        char t[ 500 ];
        
        sprintf( t, "%s: %s", ywd->gsr->NPlayerName, text );

        log.pri  = 10;
        log.msg  = t;
        log.bact = NULL;
        log.code = 0;
        _methoda( ywd->world, YWM_LOGMSG, &log );
        }
        
}


BOOL yw_LaunchChatSample( struct ypaworld_data *ywd, WORD number )
{
    /*
    ** Lädt und startet ein OneShot-Sample. Wenn schon eines da war, dann geben
    ** wir das frei. Ein Endaudioframe kann eigentlich immer garantiert werden,
    ** um den Sound zu triggern.
    **
    ** Das Sample ermitteln wir aus der Nummer und der eingestellten Sprache.
    ** Achtung, NetzwerkSamples beginnen mit einer 3!
    */
    char filename[ 255 ], new_path[ 255 ], old_path[ 255 ];

    /*** Filenamen basteln ***/
    if( ywd->gsr->lsel )
        sprintf( filename, "sounds/speech/%s/3%d.wav", ywd->gsr->lsel->language,number );
    else
        sprintf( filename, "sounds/speech/language/3%d.wav", number );

    /*** altes Sample freigeben ***/
    if( ywd->gsr->ChatObject ) {

        /*** Muß ein EndSoundSource sein??? ***/
        //_EndSoundSource( &( ywd->gsr->ChatSound ), 0 );

        _dispose( ywd->gsr->ChatObject );
        ywd->gsr->ChatObject = NULL;
        }

    /*** neues laden ***/
    strcpy( old_path, _GetAssign("rsrc"));
    _SetAssign("rsrc","data:");
    if( ywd->gsr->ChatObject = _new( "wav.class", RSA_Name, filename, TAG_DONE) ) {

        /*** Sample holen ***/
        _get( ywd->gsr->ChatObject, SMPA_Sample, &( ywd->gsr->ChatSound.src[ 0 ].sample));

        /*** Soundstruktur ausfüllen für pos und Richtung ***/
        ywd->gsr->ChatSound.vec.x = 0.0;
        ywd->gsr->ChatSound.vec.y = 0.0;
        ywd->gsr->ChatSound.vec.z = 0.0;
        ywd->gsr->ChatSound.pos.x = 0.0;
        ywd->gsr->ChatSound.pos.y = 0.0;
        ywd->gsr->ChatSound.pos.z = 0.0;
        ywd->gsr->ChatSound.src[ 0 ].volume = 127;
        ywd->gsr->ChatSound.src[ 0 ].pitch  = 0;

        /*** Abfeuern ***/
        _StartSoundSource( &( ywd->gsr->ChatSound), 0 );

        /*** Pfad wieder rücksetzen ***/
        _SetAssign("rsrc",old_path);
        return( TRUE );
        }
    else {
        /*** Pfad wieder rücksetzen ***/
        _SetAssign("rsrc",old_path);
        return( FALSE );
        }
}
#endif


