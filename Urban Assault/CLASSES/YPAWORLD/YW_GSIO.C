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
#include "audio/audioengine.h"
#include "audio/cdplay.h"
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

_dispatcher( BOOL, yw_YWM_LOADSETTINGS, struct saveloadsettings_msg *lsm )
{
/*
**  FUNCTION    Lädt die daten für die Shell aus den Userfiles.
**              Das, was früher in den Actionparametern stand, kommt
**              jetzt aus der Struktur. Ansonsten bleibt alles
**              beim alten
**              NEU: Es werden die spezifischen Sachen nur noch gemacht,
**                   wenn auch die zugehörigen Infos geladen werden.
**              NEU: Filenamen werden extern übergeben, weil wir auch andere
**                   File ("settings.tmp") beachten müssen. Das Ersetzen von
**                   Leerzeichen im Filenamen machen wir aber hier...
**
**  INPUT       Name, GSR, Maske
**
**  OUTPUT      TRUE, bei Erfolg
*/

    struct GameShellReq *GSR;
    char  helpstring[ 300 ];

    GSR = lsm->gsr;

    /* -----------------------------------------------------
    ** Lädt Initdaten aus einem File. 
    ** Wenn das Score-Flag gesetzt ist, speichern wir den
    ** derzeitigen Spielstand (mit allen Settings!!!) ab und
    ** setzen eine neue Nummer, nämlich die vom neuen File,
    ** wenn (!!) es einen Scoreblock hatte. Denn andernfalls
    ** muß man einfach weitermachen können
    ** ---------------------------------------------------*/

    if( lsm->mask & DM_SCORE ) {

        if( GSR->FirstScoreLoad ) {

            GSR->FirstScoreLoad = FALSE;
            }
        else {

            char   fn[ 300 ];
            struct saveloadsettings_msg ls;

            GSR->FirstScoreLoad = FALSE; // beim ersten Mal kein Speichern

            /*** Filename basten ***/
            sprintf( fn, "%s/user.txt", GSR->UserName );

            /*** Alles abspeichern ***/
            ls.gsr      = GSR;
            ls.filename = fn;
            ls.username = GSR->UserName;
            ls.mask     = (DM_USER|DM_INPUT|DM_SOUND|DM_SHELL|DM_SCORE|
                           DM_VIDEO|DM_BUILD|DM_BUDDY);
            ls.flags    = 0;
            _methoda( o, YWM_SAVESETTINGS, &ls );
            }
        }


    /*** Score-Found-Flag noch löschen ***/
    GSR->FoundContent = 0;

    /*** Buddy-Array löschen ***/
    if( lsm->mask & DM_BUDDY )
        yw_ClearBuddyArray( GSR->ywd );


    /* ---------------------------------------------------------------
    ** jetzt kann es richtig losgehen. Wir basteln aus levels... und
    ** dem übergebenen lsm->filename einen Filenamen und ersetzen alle
    ** Leerzeichen (die im UserNamen enthalten sein können) durch '_'
    ** -------------------------------------------------------------*/
    sprintf( helpstring,"save:%s\0", lsm->filename );

    if( !yw_ParseInitData( GSR, helpstring, lsm->mask, o ) ) {

        _LogMsg("Error while loading information from %s\n",
                lsm->filename );
        return( FALSE );
        }

    /* ----------------------------------------------------------------
    ** mir ham was geladen --> Shell aktualieren
    ** Normalerweise müßte sie offen sein, wenn nicht, öffnen wir sie,
    ** weil hier die Gadgets geändert werden (weil wir nur hier wissen,
    ** daß sich was getan hat!) 
    ** --------------------------------------------------------------*/
    if( (lsm->flags & SLS_OPENSHELL) && (!(GSR->ShellOpen)) ) {

        if( !_methoda( o, YWM_OPENGAMESHELL, GSR )) {

            _LogMsg("Unable to open GameShell\n");
            return( FALSE );
            }
        }


    /* -------------------------------------------------------------
    ** Wenn wir einen neuen Spielstand suchen sollten und auch einen
    ** gefunden haben, so ist das eine Art neues LogIn, dann ändern
    ** wir die Spielernummer.
    ** -----------------------------------------------------------*/
    if( (lsm->mask & DM_SCORE) &&
        (GSR->FoundContent & DM_SCORE) ) {

        strcpy( GSR->UserName, lsm->username );
        }

    /*** Eingabeereignisse merken ***/
    if( lsm->mask & DM_INPUT )
        yw_NoticeKeys( GSR );

    if( lsm->flags & SLS_OPENSHELL )
        _methoda( o, YWM_REFRESHGAMESHELL, GSR );

    return( TRUE );
}


_dispatcher( BOOL, yw_YWM_SAVESETTINGS, struct saveloadsettings_msg *lsm )
{
/*
**  FUNCTION    Die neue methode zum Saven der Shelldaten. Es wird
**              für das laden und Speichern die gleiche Struktur
**              genommen
**              NEU: weil wir den Usernamen nun als Verzeichnisnamen haben und
**              dieser Leerzeichen enthalten kann, ersetzen wir die hier durch '_'
**
**  INPUT       File- und Username, GSR und die Maske
**
**  OUTPUT      TRUE, wenn ok
*/
    struct GameShellReq *GSR;
    FILE   *ifile;
    char   helpstring[ 300 ];

    GSR = lsm->gsr;

    /* --------------------------------------------------------------
    ** Wir öffnen das File, welches uns übergeben wurde und schreiben 
    ** alle Infos hinein, die uns die Maskeninformation erlaubt
    ** ------------------------------------------------------------*/

    if( (!lsm->filename) || (!lsm->username)) {

        /*** keine Namen übergeben ***/
        _LogMsg("No names for save action\n");
        return( FALSE );
        }

    sprintf( helpstring, "save:%s\0", lsm->filename );

    if( ifile = _FOpen( helpstring, "w") ) {

        /*** Einzelne Routinen aufrufen, die ihre Infos reinschreiben ***/
        UWORD mask = (UWORD) lsm->mask;

        /*** User ***/
        if( mask & DM_USER )
            if( !yw_WriteUserData( ifile, lsm->username, GSR ) ) {

                _LogMsg("Unable to write user data to file\n");
                _FClose( ifile );
                return( FALSE );;
                }

        /*** Input ***/
        if( mask & DM_INPUT )
            if( !yw_WriteInputData( ifile, GSR ) ) {

                _LogMsg("Unable to write input data to file\n");
                _FClose( ifile );
                return( FALSE );
                }

        /*** Sound ***/
        if( mask & DM_SOUND )
            if( !yw_WriteSoundData( ifile, GSR ) ) {

                _LogMsg("Unable to write sound data to file\n");
                _FClose( ifile );
                return( FALSE );
                }

        /*** Video ***/
        if( mask & DM_VIDEO )
            if( !yw_WriteVideoData( ifile, GSR ) ) {

                _LogMsg("Unable to write video data to file\n");
                _FClose( ifile );
                return( FALSE );
                }

        /*** Score ***/
        if( mask & DM_SCORE )
            if( !yw_WriteScoreData( ifile, GSR ) ) {

                _LogMsg("Unable to write score data to file\n");
                _FClose( ifile );
                return( FALSE );
                }

        /*** Score ***/
        if( mask & DM_BUDDY )
            if( !yw_SaveAllBuddies( ifile, GSR->ywd ) ) {

                _LogMsg("Unable to write buddies to file\n");
                _FClose( ifile );
                return( FALSE );
                }

        /*** Shell ***/
        if( mask & DM_SHELL )
            if( !yw_WriteShellData( ifile, GSR ) ) {

                _LogMsg("Unable to write shell data to file\n");
                _FClose( ifile );
                return( FALSE );
                }

        /*** Die Bauinformationen ***/
        if( mask & DM_BUILD )
            if( !yw_SaveBuildInfo( GSR->ywd, ifile ) ) {
                    
                _LogMsg("Unable to write build info to file\n");
                _FClose( ifile );
                return( FALSE );
                }

        _FClose( ifile );
        }


    return( TRUE );
}


/*--------------------------------------------------------------------------
**                  Die Parseroutinen
**------------------------------------------------------------------------*/

BOOL yw_ParseInitData( struct GameShellReq *GSR,  UBYTE *filename,
                       UWORD  mask,               Object *World )
{
/*
**  FUNCTION    Lädt alle wichtigen Blöcke aus einem Initfile. Dabei handelt
**              es sich um Daten über den User (nur am Anfang relevant), Daten
**              für Input, Audio, Video (das klingt ja hier wie Muhldie-Mädija)
**              und Spielstände.
**              Die Maske gibt an, was geladen werden soll. ACHTUNG, target ist 
**              meist die GameShell selbst, muß also nochmal angegeben werden, 
**              manchmal kann das aber auch nur ein String sein oder was ähnliches
**              (userdata z.B.)
**
**              folgende Contexte werden bearbeitet:
**              new_user
**              new_/modifiy_input
**              new_/modifiy_sound
**              new_/modifiy_video
**              new_/modifiy_score
**
**              Nachteil.  für alle nur ein target, hm...vorerst geht das....
**
**  RESULT      TRUE, dann war alles ok
**
**  INPUT       mask gibt an, was gelesen werden soll, entspricht der d_mask
**              target wird prinzipiell an die Struktur gekettet und damit der
**              Funktion als Halde übergeben.
**
**  CHANGED     Sonne und ich zieh bald um!
**              der target-Pointer entscheidet, ob die Sache bearbeitet oder
**              nur auf ein End gewartet werden soll
**
**              Morgen ist Fete und ab heute schleife ich den Weltpointer mit durch
**              NEU: Der Parser akzeptiert nun unbekannte Blöcke, ich übergebe
**              also nur die Parser, zu denen
*/


    ULONG p = 0;
    struct ScriptParser parser[10];
    memset(parser, 0, sizeof(parser));

    if( mask & DM_USER ) {

        parser[p].parse_func = yw_ParseUserData;
        parser[p].store[0]   = (ULONG) World;
        parser[p].store[1]   = (ULONG) GSR->ywd;
        p++;
        }

    if( mask & DM_INPUT ) {

        parser[p].parse_func = yw_ParseInputData;
        parser[p].store[0]   = (ULONG) World;
        parser[p].target     = GSR;
        p++;
        }

    if( mask & DM_VIDEO ) {

        parser[p].parse_func = yw_ParseVideoData;
        parser[p].store[0]   = (ULONG) World;
        parser[p].target     = GSR;
        p++;
        }

    if( mask & DM_SOUND ) {

        parser[p].parse_func = yw_ParseSoundData;
        parser[p].store[0]   = (ULONG) World;
        parser[p].target     = GSR;
        p++;
        }

    if( mask & DM_SCORE ) {

        parser[p].parse_func = yw_ParseLevelStatus;
        parser[p].store[0]   = (ULONG) &(GSR->FoundContent);
        parser[p].target     = GSR->ywd;
        p++;
        }

    if( mask & DM_BUDDY ) {

        parser[p].parse_func = yw_ParseBuddy;
        parser[p].target     = GSR->ywd;
        p++;
        }

    if( mask & DM_SHELL ) {

        parser[p].parse_func = yw_ParseShellData;
        parser[p].store[0]   = (ULONG) World;
        parser[p].target     = GSR;
        p++;
        }

    if( mask & DM_BUILD ) {

        parser[p].parse_func = yw_VhclProtoParser;  // für bauflags, energy, shield...
        parser[p].store[0]   = (ULONG) GSR->ywd;
        p++;

        parser[p].parse_func = yw_WeaponProtoParser;  // für bauflags und anderes
        parser[p].store[0]   = (ULONG) GSR->ywd;
        p++;

        parser[p].parse_func = yw_BuildProtoParser;  // für bauflags
        parser[p].store[1]   = (ULONG) GSR->ywd;
        p++;
        }


    /*** nu los ***/
    return( yw_ParseScript( filename, p, parser, PARSEMODE_SLOPPY ) );
}



ULONG yw_ParseUserData( struct ScriptParser *parser )
{
/* --------------------------------------------------------------------
** Liest die UserDaten, dazu gehört nicht mehr der Name (den ignorieren
** wir, wenn wir ihn finden.
** ------------------------------------------------------------------*/

    struct ypaworld_data *ywd;
    ywd = (struct ypaworld_data *) parser->store[1];


    if( PARSESTAT_READY == parser->status ) {

        /*** Wir sind bereit für etwas neues ***/
        if( stricmp( parser->keyword, "new_user") == 0 ) {

            /*** Das ist etwas für uns ***/
            parser->status = PARSESTAT_RUNNING;
            return( PARSE_ENTERED_CONTEXT );
            }
        else {

            /*** Damit kann ich nichts anfangen ***/
            return( PARSE_UNKNOWN_KEYWORD );
            }
        }
    else {

        /*** Wir bearbeiten das schon. Ist es das Ende? ***/
        if( stricmp( parser->keyword, "end") == 0 ) {

            /*** Das Ende naht! ***/
            parser->status = PARSESTAT_READY;
            return( PARSE_LEFT_CONTEXT );
            }
        else {

            /*** Es sollte also ein  normales keyword sein ***/
            if( (parser->store[0]) && (parser->store[1]) ) {

                if( stricmp( parser->keyword, "username") == 0) {

                    /*** Der Name wird ignoriert ***/
                }else if( stricmp( parser->keyword, "netname") == 0) {

                    /* -------------------------------------------------------
                    ** Der Name fuers Netzwerk. Achtung!!! bereits vorher wird
                    ** auf LobbyStart getestet. Wenn remotestart, dann ist das
                    ** Namensfeld schon ausgefuellt.
                    ** -----------------------------------------------------*/
                    if( !ywd->gsr->remotestart ) {
                        
                    int i = 0;
                    
                        strncpy( ywd->gsr->NPlayerName, parser->data, 60);
                        while( ywd->gsr->NPlayerName[ i ]) {
                            if( ywd->gsr->NPlayerName[ i ] == '_')
                                ywd->gsr->NPlayerName[ i ]  = ' ';
                            i++;
                        }
                    }   
                } else if( stricmp( parser->keyword, "maxroboenergy") == 0) {
                    /*** Die höchste Energie, die ich je hatte ***/
                    ywd->MaxRoboEnergy = strtol( parser->data, NULL, 0 );
                } else if (stricmp( parser->keyword, "maxreloadconst") == 0) {
                    ywd->MaxReloadConst = strtol( parser->data, NULL, 0 );
                } else if( stricmp( parser->keyword, "numbuddies") == 0) {
                    /*** Wieviele darf ich mitnehmen? ***/
                    ywd->Level->MaxNumBuddies = strtol( parser->data, NULL, 0 );
                }else if( stricmp( parser->keyword, "playerstatus") == 0) {

                    char  *d;
                    int    o;

                    if( d = strtok( parser->data, "_ \t" )) {
                      o = strtol( d, NULL, 0 );
                      if( d = strtok( NULL, "_ \t" )) {
                        ywd->GlobalStats[o].Kills = strtol( d, NULL, 0 );
                        if( d = strtok( NULL, "_ \t" )) {
                          ywd->GlobalStats[o].UserKills = strtol( d, NULL, 0 );
                          if( d = strtok( NULL, "_ \t" )) {
                            ywd->GlobalStats[o].Time = strtol( d, NULL, 0 );
                            if( d = strtok( NULL, "_ \t" )) {
                              ywd->GlobalStats[o].SecCons = strtol( d, NULL, 0 );
                              if( d = strtok( NULL, "_ \t" )) {
                                ywd->GlobalStats[o].Score = strtol( d, NULL, 0 );
                                if( d = strtok( NULL, "_ \t" )) {
                                  ywd->GlobalStats[o].Power = strtol( d, NULL, 0 );
                                  if( d = strtok( NULL, "_ \t" )) {
                                    ywd->GlobalStats[o].Techs = strtol( d, NULL, 0 );
                                  }
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                }else if( stricmp( parser->keyword, "jodiefoster") == 0) {

                    char  *d;
                    long   o;

                    if( d = strtok( parser->data, "_ \t" )) {
                      o = strtol( d, NULL, 0 );
                      ywd->Level->RaceTouched[0] = (BOOL) o;
                      if( d = strtok( NULL, "_ \t" )) {
                        o = strtol( d, NULL, 0 );
                        ywd->Level->RaceTouched[1] = (BOOL) o;
                        if( d = strtok( NULL, "_ \t" )) {
                          o = strtol( d, NULL, 0 );
                          ywd->Level->RaceTouched[2] = (BOOL) o;
                          if( d = strtok( NULL, "_ \t" )) {
                            o = strtol( d, NULL, 0 );
                            ywd->Level->RaceTouched[3] = (BOOL) o;
                            if( d = strtok( NULL, "_ \t" )) {
                              o = strtol( d, NULL, 0 );
                              ywd->Level->RaceTouched[4] = (BOOL) o;
                              if( d = strtok( NULL, "_ \t" )) {
                                o = strtol( d, NULL, 0 );
                                ywd->Level->RaceTouched[5] = (BOOL) o;
                                if( d = strtok( NULL, "_ \t" )) {
                                  o = strtol( d, NULL, 0 );
                                  ywd->Level->RaceTouched[6] = (BOOL) o;
                                  if( d = strtok( NULL, "_ \t" )) {
                                    o = strtol( d, NULL, 0 );
                                    ywd->Level->RaceTouched[7] = (BOOL) o;
                                  }
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                } else {
                    /*** Unbekannt ***/
                    return( PARSE_UNKNOWN_KEYWORD );
                };
                return( PARSE_ALL_OK );
            };
        };
    };
    /*** can't happen ***/    
    return(PARSE_UNKNOWN_KEYWORD);
}


ULONG yw_ParseInputData( struct ScriptParser *parser )
{
    Object *input_object;
    char   *nstr, *pos;
    BOOL    all_ok;
    LONG    nbr, ofs;
    struct  GameShellReq *GSR;
    struct  inp_handler_msg sh;
    struct idev_sethotkey_msg sk;
    struct inp_delegate_msg del;



    /*** InputObjekt ranschaffen ***/
    _IE_GetAttrs( IET_Object, &input_object, TAG_DONE );

    if( PARSESTAT_READY == parser->status ) {

        /*** Wir sind bereit für etwas neues ***/
        if( (stricmp( parser->keyword, "new_input")    == 0 ) ||
            (stricmp( parser->keyword, "modify_input") == 0 ) ) {

            /*** Das ist etwas für uns. Bei new Defaultwerte setzen ***/
            if( stricmp( parser->keyword, "new_input") == 0 ) {

                /* ----------------------------------------------------
                ** Vielleicht fülle ich hier noch einmal alles aus, für
                ** den Fall, daß keine Default ("0.usr") Datei gefunden
                ** wurde, doch vorerst reicht es, alles von dieser
                ** einzulesen
                **
                ** Vorerst lösche ich die Hotkeys, damit nur über-
                ** schrieben wird, was genutzt werden soll. Quark, ich
                ** lösche alles!
                ** --------------------------------------------------*/
                int i;

                if( parser->target ) {

                    GSR = (struct GameShellReq *) parser->target;
                    for( i=0; i <= NUM_INPUTEVENTS; i++ ) {

                        GSR->inp[ i ].pos = 0;
                        }
                    }
                }

            parser->status = PARSESTAT_RUNNING;
            return( PARSE_ENTERED_CONTEXT );
            }
        else {

            /*** Damit kann ich nichts anfangen ***/
            return( PARSE_UNKNOWN_KEYWORD );
            }
        }
    else {

        /*** Wir bearbeiten das schon. Ist es das Ende? ***/
        if( stricmp( parser->keyword, "end") == 0 ) {

            /*** Das Ende naht! ***/
            parser->status = PARSESTAT_READY;
            return( PARSE_LEFT_CONTEXT );
            }
        else {

            int  i;
            char dec_string[ 500 ];

            /*** Es sollte also ein  normales keyword sein ***/
            if( parser->target ) {

              GSR = (struct GameShellReq *) parser->target;
              GSR->FoundContent |= DM_INPUT; // weil hier GSR da ist

              /*** Was sagt denn der Qualifiermodus, der zuerst kommen muß! ***/
              if( stricmp( parser->keyword, "qualmode") == 0 ) {

                  /*** Keyword Vorläufig noch drinnen lassen ***/
                  return( PARSE_ALL_OK );
                  }

              if( stricmp( parser->keyword, "joystick") == 0 ) {

                  if( (stricmp( parser->data, "yes") == 0) ||
                      (stricmp( parser->data, "on" ) == 0) )
                      GSR->ywd->Prefs.Flags &= ~YPA_PREFS_JOYDISABLE;
                  else
                      GSR->ywd->Prefs.Flags |=  YPA_PREFS_JOYDISABLE;

                  return( PARSE_ALL_OK );
                  }

              if( stricmp( parser->keyword, "forcefeedback") == 0 ) {

                  if( (stricmp( parser->data, "yes") == 0) ||
                      (stricmp( parser->data, "on" ) == 0) )
                      GSR->ywd->Prefs.Flags &= ~YPA_PREFS_FFDISABLE;
                  else
                      GSR->ywd->Prefs.Flags |=  YPA_PREFS_FFDISABLE;

                  return( PARSE_ALL_OK );
                  }

              /* ---------------------------------------------------
              ** Zuerst ersetzen wir alle "_" durch " ", solange wir
              ** in data keine Leerzeichen haben dürfen
              ** -------------------------------------------------*/
              pos = parser->data;
              while( pos = strpbrk( pos, "_") ) *pos = ' ';

              /* ---------------------------------------------------
              ** Nun ersetzen wir alle KEYBORD_DRIVER_SIGNs durch
              ** KEYBORD_DRIVER, so daß der systemunabhängige String
              ** auf das System zugeschnitten wird.
              ** -------------------------------------------------*/
              i = 0;
              pos = dec_string;
              while( parser->data[ i ] ) {

                  if( strncmp( &(parser->data[ i ]),
                      KEYBORD_DRIVER_SIGN, 1 ) == 0 ) {

                      /*** Ersetzen ***/
                      strcpy( pos, KEYBORD_DRIVER );
                      pos += strlen( KEYBORD_DRIVER );
                      }
                  else {

                      /*** Übernehmen ***/
                      *pos = parser->data[ i ];
                      pos++;
                      }
                  i++;
                  }
              *pos = 0;

              if( strnicmp( parser->keyword, "input.slider[",13) == 0) {

                /*** Ein Slider. Zuerst die Nummer ermitteln ***/
                all_ok = FALSE;
                nstr = strtok( &(parser->keyword[13]), "] \t=\n");
                nbr  = atol( nstr );

                /*** Das Objekt informieren ***/
                sh.id   = dec_string;
                sh.num  = nbr;
                sh.type = ITYPE_SLIDER;
                if( !_methoda( input_object, IM_SETHANDLER, &sh ) ) {

                  /*** Fehler ***/
                  _LogMsg("WARNING: cannot set slider %d with %s\n", nbr, sh.id );
                  }
                else {

                  /*** Für interne Verwaltung aufbereiten ***/
                  ofs = yw_GetInternalNumber( GSI_SLIDER, nbr );

                  if( ofs == -1 ) {

                    _LogMsg("Unknown number in slider-declaration (%d)\n", nbr);
                    return( PARSE_BOGUS_DATA );
                    }

                  GSR->inp[ ofs ].number = nbr;
                  GSR->inp[ ofs ].kind   = GSI_SLIDER;

                  /*** keine Qualifier mehr ***/
                  pos = strtok( dec_string, ":");

                  if( pos ) {

                    if( pos = strtok( NULL, " \t") ) {

                      /* ------------------------------------
                      ** pos ist "negativer" Name
                      ** wir müssen nun den Keycode ermitteln 
                      ** ----------------------------------*/

                      GSR->inp[ ofs ].neg = yw_GetKeycode( pos );
                      if( -1 == GSR->inp[ ofs ].neg ) {

                        /*** Das Wort gibt es nicht ***/
                        _LogMsg("Unknown keyword for slider %s\n",
                                 pos );
                        return( PARSE_BOGUS_DATA );
                        }

                      if( pos = strtok( NULL, ":") ) {

                        if( pos = strtok( NULL, " \t\n") ) {

                          /*** pos zeigt auf "positiven" N. ***/

                          GSR->inp[ ofs ].pos = yw_GetKeycode( pos );
                          if( -1 == GSR->inp[ ofs ].pos ) {

                            _LogMsg("Unknown keyword for slider %s\n",
                                     pos );
                            return( PARSE_BOGUS_DATA );
                            }

                          /*** Hier kommen noch Erweiterungen für Jstk. ***/
                          
                          all_ok = TRUE;
                          }
                        }
                      }
                    }
                  }
                
                if( !all_ok ) {

                    /*** Die Zeile war irgendwie nicht ok ***/
                    _LogMsg("Wrong input expression for slider %d\n", nbr );
                    return( PARSE_BOGUS_DATA );
                    }
                
                  /*** Ende Slider ***/

              } else {

                if( strnicmp( parser->keyword, "input.button[",13) == 0) {
                  
                  /*** Ein Button. Zuerst die Nummer ermitteln ***/
                  all_ok = FALSE;
                  nstr = strtok( &(parser->keyword[13]), "] \t=\n");
                  nbr  = atol( nstr );

                  /*** Das Objekt informieren ***/
                  sh.id   = dec_string;
                  sh.num  = nbr;
                  sh.type = ITYPE_BUTTON;
                  if( !_methoda( input_object, IM_SETHANDLER, &sh ) ) {

                    /*** Fehler ***/
                    _LogMsg("WARNING: cannot set button %d with %s\n", nbr, sh.id );
                    }
                  else {

                    /*** Für interne Verwaltung aufbereiten ***/
                    ofs = yw_GetInternalNumber( GSI_BUTTON, nbr );

                    if( ofs == -1 ) {

                      _LogMsg("Unknown number in button-declaration (%d)\n", nbr);
                      return( PARSE_BOGUS_DATA );
                      }

                    GSR->inp[ ofs ].number = nbr;
                    GSR->inp[ ofs ].kind   = GSI_BUTTON;

                    /*** kein Qualifiermodus mehr ***/
                    pos = strtok( dec_string, ":");

                    if( pos ) {

                      if( pos = strtok( NULL, " \t") ) {

                        /* -------------------------------------
                        ** pos ist "positiver" und einziger Name
                        ** wir müssen nun den Keycode ermitteln 
                        ** -----------------------------------*/

                        GSR->inp[ ofs ].pos = yw_GetKeycode( pos );
                        if( -1 == GSR->inp[ ofs ].pos ) {

                          /*** Das Wort gibt es nicht ***/
                          _LogMsg("Unknown keyword for button %s\n",
                                   pos );
                          return( PARSE_BOGUS_DATA );
                          }

                        /*** Hier kommen noch Erweiterungen für Jstk. ***/
                          
                        all_ok = TRUE;
                        }
                      }
                    }
                  
                if( !all_ok ) {

                    /*** Die Zeile war irgendwie nicht ok ***/
                    _LogMsg("Wrong input expression for button %d\n", nbr );
                    return( PARSE_BOGUS_DATA );
                    }
                    
                /*** Ende Button ***/

              } else {

                if( strnicmp( parser->keyword, "input.hotkey[",13) == 0) {
                    
                    /*** Ein Button. Zuerst die Nummer ermitteln ***/
                    all_ok = FALSE;
                    nstr = strtok( &(parser->keyword[13]), "] \t=\n");
                    nbr  = atol( nstr );

                    /*** Das Objekt informieren ***/
                    sk.id     = dec_string;
                    sk.hotkey = nbr;

                    del.type   = ITYPE_KEYBOARD;
                    del.num    = 0;
                    del.method = IDEVM_SETHOTKEY;
                    del.msg    = &sk;
                    
                    if( !_methoda( input_object, IM_DELEGATE, &del ) ) {

                      /*** Fehler ***/
                      _LogMsg("WARNING: cannot set hotkey %d with %s\n", nbr, sh.id );
                      }
                    else {

                      /*** Für interne Verwaltung aufbereiten ***/
                      ofs = yw_GetInternalNumber( GSI_HOTKEY, nbr );

                      if( ofs == -1 ) {

                        /*** Abbruch ohne Fehler, weil Testphase ***/
                        _LogMsg("Unknown number in hotkey-declaration (%d)\n", nbr);
                        //return( PARSE_BOGUS_DATA );
                        return( PARSE_ALL_OK );
                        }

                      GSR->inp[ ofs ].number = nbr;
                      GSR->inp[ ofs ].kind   = GSI_HOTKEY;

                      /*** keinen Qualifier bei Hotkeys ***/

                      /*** nur der Name ***/
                      if( pos = strtok( dec_string, " \t\n") ) {

                        /* -------------------------------------
                        ** pos ist "positiver" und einziger Name
                        ** wir müssen nun den Keycode ermitteln 
                        ** -----------------------------------*/

                        GSR->inp[ ofs ].pos = yw_GetKeycode( pos );
                        if( -1 == GSR->inp[ ofs ].pos ) {

                            /*** Das Wort gibt es nicht ***/
                            _LogMsg("Unknown keyword for hotkey: %s\n",
                                     pos );
                            return( PARSE_BOGUS_DATA );
                            }

                            /*** Hier kommen noch Erweiterungen für Jstk. ***/
                            
                          all_ok = TRUE;
                          }
                        }
                      
                  if( !all_ok ) {

                      /*** Die Zeile war irgendwie nicht ok ***/
                      _LogMsg("Wrong input expression for hotkey %d\n", nbr );
                      return( PARSE_BOGUS_DATA );
                      }
                  
                  /*** Ende Hotkey ***/

              } else {

                  /*** Sowas kenne ich nicht ***/
                  _LogMsg("Unknown keyword %s in InputExpression\n", parser->keyword);
                  return( PARSE_UNKNOWN_KEYWORD );
                  }
                 }
                }
              }

            return( PARSE_ALL_OK );
            }
        }
}

ULONG yw_ParseVideoData( struct ScriptParser *parser )
{
    struct GameShellReq *GSR;
    Object *World;

    World = (Object *) parser->store[ 0 ];


    if( PARSESTAT_READY == parser->status ) {

        /*** Wir sind bereit für etwas neues ***/
        if( stricmp( parser->keyword, "new_video") == 0 ) {

            /*** Das ist etwas für uns ***/
            parser->status = PARSESTAT_RUNNING;
            return( PARSE_ENTERED_CONTEXT );
            }
        else {

            /*** Damit kann ich nichts anfangen ***/
            return( PARSE_UNKNOWN_KEYWORD );
            }
        }
    else {

        /*** Wir bearbeiten das schon. Ist es das Ende? ***/
        if( stricmp( parser->keyword, "end") == 0 ) {

            /*** Das Ende naht! ***/
            parser->status = PARSESTAT_READY;
            return( PARSE_LEFT_CONTEXT );
            }
        else {

            /*** Es sollte also ein  normales keyword sein ***/
            if( parser->target ) {

                GSR = (struct GameShellReq *) parser->target;
                GSR->FoundContent |= DM_VIDEO; // weil hier GSR da ist
                
                if( stricmp( parser->keyword, "videomode") == 0) {

                    /* --------------------------------------------------
                    ** Speichert jetzt Modus-ID ab bzw. liest sie...
                    ** ------------------------------------------------*/

                    ULONG  menuitem;
                    ULONG  mode;
                    struct video_node *vnode, *found_node = NULL;

                    mode = atol(parser->data );

                    /*** Die Node suchen ***/
                    vnode = (struct video_node *) GSR->videolist.mlh_Head;
                    menuitem = 0;
                    while( vnode->node.mln_Succ ) {

                        if( vnode->modus == mode ) {

                            found_node = vnode;
                            break;
                            }

                        menuitem++;
                        vnode = (struct video_node *) vnode->node.mln_Succ;
                        }

                    /*** Unterstützen wir das überhaupt ***/
                    if( !found_node ) {

                        ULONG modus;

                        _LogMsg("Warning: This machine doesn't support mode %d\n", mode);
                        found_node = (struct video_node *) GSR->videolist.mlh_Head;

                        /*** 640 * 480 suchen, das muesste eine ID ... sein ***/
                        modus = GFX_SHELL_RESOLUTION;
                        vnode = (struct video_node *) GSR->videolist.mlh_Head;
                        menuitem = 0;
                        while( vnode->node.mln_Succ ) {

                            if( vnode->modus == modus ) {

                                found_node = vnode;
                                break;
                                }

                            menuitem++;
                            vnode = (struct video_node *) vnode->node.mln_Succ;
                            }
                        }
                        
                    /*** Setzen ***/
                    GSR->v_actualitem = menuitem;
                    GSR->ywd->GameRes = vnode->modus;
                    GSR->new_modus    = vnode->modus;

                    /*** Nicht mehr setzen ***/

                } else {

                if( stricmp( parser->keyword, "farview") == 0 ) {

                    /*** Weitsicht erwünscht? ***/
                    if( stricmp( parser->data, "yes" ) == 0 ) {

                        GSR->video_flags     |= VF_FARVIEW;
                        yw_SetFarView( World, TRUE );
                        }
                    else {

                        GSR->video_flags     &= ~VF_FARVIEW;
                        yw_SetFarView( World, FALSE );
                        }

                } else {

                if( stricmp( parser->keyword, "filtering") == 0 ) {

                    /*** Weitsicht erwünscht? ***/
                    if( stricmp( parser->data, "yes" ) == 0 ) {

                        GSR->video_flags      |= VF_FILTERING;
                        GSR->ywd->Prefs.Flags |= YPA_PREFS_FILTERING;
                        _set(GSR->ywd->GfxObject,WINDDA_TextureFilter,TRUE);
                        }
                    else {

                        GSR->video_flags      &= ~VF_FILTERING;
                        GSR->ywd->Prefs.Flags &= ~YPA_PREFS_FILTERING;
                        _set(GSR->ywd->GfxObject,WINDDA_TextureFilter,FALSE);
                        }

                } else {

                if( stricmp( parser->keyword, "softmouse") == 0 ) {

                    /*** Weitsicht erwünscht? ***/
                    if( stricmp( parser->data, "yes" ) == 0 ) {

                        GSR->video_flags      |= VF_SOFTMOUSE;
                        GSR->ywd->Prefs.Flags |= YPA_PREFS_SOFTMOUSE;
                        _set( GSR->ywd->GfxObject, WINDDA_CursorMode,
                              WINDD_CURSORMODE_SOFT);
                        }
                    else {

                        GSR->video_flags      &= ~VF_SOFTMOUSE;
                        GSR->ywd->Prefs.Flags &= ~YPA_PREFS_SOFTMOUSE;
                        _set( GSR->ywd->GfxObject, WINDDA_CursorMode,
                              WINDD_CURSORMODE_HW);
                        }

                } else {

                if( stricmp( parser->keyword, "palettefx") == 0 ) {

                    /* -------------------------------------
                    ** Zahl der Paletteneffekte. Keyword aus 
                    ** kompatibilitaetsgruenden drinlassen 
                    ** -----------------------------------*/
                    
                } else {

                if( stricmp( parser->keyword, "heaven") == 0 ) {

                    /*** Himmel erwünscht? ***/
                    if( stricmp( parser->data, "yes" ) == 0 ) {

                        GSR->video_flags     |= VF_HEAVEN;
                        _set( World, YWA_RenderHeaven, TRUE );
                        }
                    else {

                        GSR->video_flags     &= ~VF_HEAVEN;
                        _set( World, YWA_RenderHeaven, FALSE );
                        }

                } else {

                if( stricmp( parser->keyword, "fxnumber") == 0 ) {

                    /*** Wieviele Zerstörungsgrade ***/
                    GSR->destfx = (WORD) atol( parser->data );
                    yw_SetDestFX( GSR );

                } else {

                if( stricmp( parser->keyword, "enemyindicator") == 0 ) {

                    /*** komische Dreiecke? ***/
                    if( stricmp( parser->data, "yes" ) == 0) {
                        GSR->ywd->Prefs.Flags |=  YPA_PREFS_INDICATOR;
                        GSR->enemyindicator    = TRUE;
                        }
                    else {
                        GSR->ywd->Prefs.Flags &= ~YPA_PREFS_INDICATOR;
                        GSR->enemyindicator = FALSE;
                        }

                } else {

                    /*** Unbekannt ***/
                    return( PARSE_UNKNOWN_KEYWORD );
                    } } } } } } } }
                }

            return( PARSE_ALL_OK );
            }
        }
}

ULONG yw_ParseShellData( struct ScriptParser *parser )
{
    struct GameShellReq *GSR;
    Object *World;
    struct ypaworld_data *ywd;


    World = (Object *) parser->store[ 0 ];  // im Nachhinein doppelt gemoppelt...
    
    if( PARSESTAT_READY == parser->status ) {

        /*** Wir sind bereit für etwas neues ***/
        if( stricmp( parser->keyword, "new_shell") == 0 ) {

            /*** Das ist etwas für uns ***/
            parser->status = PARSESTAT_RUNNING;
            return( PARSE_ENTERED_CONTEXT );
            }
        else {

            /*** Damit kann ich nichts anfangen ***/
            return( PARSE_UNKNOWN_KEYWORD );
            }
        }
    else {

        /*** Wir bearbeiten das schon. Ist es das Ende? ***/
        if( stricmp( parser->keyword, "end") == 0 ) {

            /*** Das Ende naht! ***/
            parser->status = PARSESTAT_READY;
            return( PARSE_LEFT_CONTEXT );
            }
        else {

            /*** Es sollte also ein  normales keyword sein ***/
            if( parser->target ) {

                char *p;
                char data[ 300 ];

                GSR = (struct GameShellReq *) parser->target;
                GSR->FoundContent |= DM_SHELL; // weil hier GSR da ist

                ywd = GSR->ywd;
                strncpy( data, parser->data, 299 );

                if( stricmp( parser->keyword, "LANGUAGE" ) == 0 ) {

                    /*** Welche Sprache soll verwendet werden ***/
                    struct localeinfonode *ln, *found = NULL, *english = NULL;

                    ln = (struct localeinfonode *) GSR->localelist.mlh_Head;
                    while( ln->node.mln_Succ ) {

                        if( stricmp(ln->language, parser->data) == 0 )
                            found = ln;

                        if( stricmp(ln->language, "language") == 0 )
                            english = ln;

                        ln = (struct localeinfonode *) ln->node.mln_Succ;
                        }

                    /*** kein "Not-Englisch" mehr ***/
                    if( found )
                        GSR->lsel = found;
                    else
                        GSR->lsel = english;
                    GSR->new_lsel = GSR->lsel;

                    /*** Nun Aktion auslösen ***/
                    if( !_methoda( World, YWM_SETGAMELANGUAGE, GSR )) {

                        _LogMsg("Unable to set new language\n");
                        /*** kein return FALSE!!! es kann ja weitergehen ***/
                        }
                    }
                else {

                if( stricmp( parser->keyword, "SOUND" ) == 0 ) {

                    /*** Not longer supported ***/
                    }
                else {

                if( stricmp( parser->keyword, "VIDEO" ) == 0 ) {

                    /*** Not longer supported ***/
                    }
                else {

                if( stricmp( parser->keyword, "INPUT" ) == 0 ) {

                    /*** not longer supported ***/
                    }
                else {

                if( stricmp( parser->keyword, "DISK" ) == 0 ) {

                    /*** not longer supported ***/
                    }
                else {

                if( stricmp( parser->keyword, "LOCALE" ) == 0 ) {

                    /*** not longer supported ***/
                    }
                else {

                if( stricmp( parser->keyword, "NET" ) == 0 ) {

                    /*** not longer supported ***/
                    }
                else {

                if( stricmp( parser->keyword, "FINDER" ) == 0 ) {

                    if( p = strtok( data, "_ \t") ) {

                        /*** auch Fensterpos, obwohl noch nicht ausgewertet ***/
                        if( p = strtok( NULL, "_ \t") ) {
                          ywd->Prefs.WinFinder.rect.x = (WORD) atol( p );

                          if( p = strtok( NULL, "_ \t") ) {
                            ywd->Prefs.WinFinder.rect.y = (WORD) atol( p );

                            if( p = strtok( NULL, "_ \t") ) {
                              ywd->Prefs.WinFinder.rect.w = (WORD) atol( p );

                              if( p = strtok( NULL, "_ \t") ) {
                                ywd->Prefs.WinFinder.rect.h = (WORD) atol( p );
                                }
                              else return( PARSE_BOGUS_DATA );
                              }
                            else return( PARSE_BOGUS_DATA );
                            }
                          else return( PARSE_BOGUS_DATA );
                          }
                        else return( PARSE_BOGUS_DATA );
                        }
                    }
                else {

                if( stricmp( parser->keyword, "LOG" ) == 0 ) {

                    if( p = strtok( data, "_ \t") ) {

                        /*** auch Fensterpos, obwohl noch nicht ausgewertet ***/
                        if( p = strtok( NULL, "_ \t") ) {
                          ywd->Prefs.WinLog.rect.x = (WORD) atol( p );

                          if( p = strtok( NULL, "_ \t") ) {
                            ywd->Prefs.WinLog.rect.y = (WORD) atol( p );

                            if( p = strtok( NULL, "_ \t") ) {
                              ywd->Prefs.WinLog.rect.w = (WORD) atol( p );

                              if( p = strtok( NULL, "_ \t") ) {
                                ywd->Prefs.WinLog.rect.h = (WORD) atol( p );
                                }
                              else return( PARSE_BOGUS_DATA );
                              }
                            else return( PARSE_BOGUS_DATA );
                            }
                          else return( PARSE_BOGUS_DATA );
                          }
                        else return( PARSE_BOGUS_DATA );
                        }
                    }
                else {

                if( stricmp( parser->keyword, "ENERGY" ) == 0 ) {

                    if( p = strtok( data, "_ \t") ) {

                        /*** auch Fensterpos, obwohl noch nicht ausgewertet ***/
                        if( p = strtok( NULL, "_ \t") ) {
                          ywd->Prefs.WinEnergy.rect.x = (WORD) atol( p );

                          if( p = strtok( NULL, "_ \t") ) {
                            ywd->Prefs.WinEnergy.rect.y = (WORD) atol( p );

                            if( p = strtok( NULL, "_ \t") ) {
                              ywd->Prefs.WinEnergy.rect.w = (WORD) atol( p );

                              if( p = strtok( NULL, "_ \t") ) {
                                ywd->Prefs.WinEnergy.rect.h = (WORD) atol( p );
                                }
                              else return( PARSE_BOGUS_DATA );
                              }
                            else return( PARSE_BOGUS_DATA );
                            }
                          else return( PARSE_BOGUS_DATA );
                          }
                        else return( PARSE_BOGUS_DATA );
                        }
                    }
                else {

                if( stricmp( parser->keyword, "MESSAGE" ) == 0 ) {

                    /* --------------------------------------------------
                    ** Ebenso netzspezifisch. Gelesen wird es immer, denn
                    ** jede version muß es schlucken. Auswertung erfolgt
                    ** aber nur im Netzmodus
                    ** ------------------------------------------------*/
                    #ifdef __NETWORK__
                    if( p = strtok( data, "_ \t") ) {

                        /*** auch Fensterpos, obwohl noch nicht ausgewertet ***/
                        if( p = strtok( NULL, "_ \t") ) {
                          ywd->Prefs.WinMessage.rect.x = (WORD) atol( p );

                          if( p = strtok( NULL, "_ \t") ) {
                            ywd->Prefs.WinMessage.rect.y = (WORD) atol( p );

                            if( p = strtok( NULL, "_ \t") ) {
                              ywd->Prefs.WinMessage.rect.w = (WORD) atol( p );

                              if( p = strtok( NULL, "_ \t") ) {
                                ywd->Prefs.WinMessage.rect.h = (WORD) atol( p );
                                }
                              else return( PARSE_BOGUS_DATA );
                              }
                            else return( PARSE_BOGUS_DATA );
                            }
                          else return( PARSE_BOGUS_DATA );
                          }
                        else return( PARSE_BOGUS_DATA );
                        }
                    #endif
                    }
                else {

                if( stricmp( parser->keyword, "MAP" ) == 0 ) {

                    if( p = strtok( data, "_ \t") ) {

                        /*** auch Fensterpos, obwohl noch nicht ausgewertet ***/
                        if( p = strtok( NULL, "_ \t") ) {
                          ywd->Prefs.WinMap.rect.x = (WORD) atol( p );

                          if( p = strtok( NULL, "_ \t") ) {
                            ywd->Prefs.WinMap.rect.y = (WORD) atol( p );

                            if( p = strtok( NULL, "_ \t") ) {
                              ywd->Prefs.WinMap.rect.w = (WORD) atol( p );

                              if( p = strtok( NULL, "_ \t") ) {
                                ywd->Prefs.WinMap.rect.h = (WORD) atol( p );

                                if( p = strtok( NULL, "_ \t") ) {
                                  ywd->Prefs.MapLayers = (WORD) atol( p );

                                  if( p = strtok( NULL, "_ \t") ) {
                                    ywd->Prefs.MapZoom = (WORD) atol( p );

                                    /* -------------------------------------
                                    ** Obwohl es nicht eindeutig ist,
                                    ** setze ich das "valid-Flag" hier, weil
                                    ** es die letztmögliche Stelle ist.
                                    ** Ansonsten müßte ich jedes Fenster
                                    ** einzeln freischalten
                                    ** -----------------------------------*/
                                    ywd->Prefs.valid = (LONG)TRUE;
                                    }
                                  else return( PARSE_BOGUS_DATA );
                                  }
                                else return( PARSE_BOGUS_DATA );
                                }
                              else return( PARSE_BOGUS_DATA );
                              }
                            else return( PARSE_BOGUS_DATA );
                            }
                          else return( PARSE_BOGUS_DATA );
                          }
                        else return( PARSE_BOGUS_DATA );
                        }
                    }
                else {

                    return( PARSE_UNKNOWN_KEYWORD );
                    } } } } } } } } } } } }
                }

            return( PARSE_ALL_OK );
            }
        }
}

ULONG yw_ParseSoundData( struct ScriptParser *parser )
{

    if( PARSESTAT_READY == parser->status ) {

        /*** Wir sind bereit für etwas neues ***/
        if( stricmp( parser->keyword, "new_sound") == 0 ) {

            /*** Das ist etwas für uns ***/
            parser->status = PARSESTAT_RUNNING;
            return( PARSE_ENTERED_CONTEXT );
            }
        else {

            /*** Damit kann ich nichts anfangen ***/
            return( PARSE_UNKNOWN_KEYWORD );
            }
        }
    else {

        /*** Wir bearbeiten das schon. Ist es das Ende? ***/
        if( stricmp( parser->keyword, "end") == 0 ) {

            /*** Das Ende naht! ***/
            parser->status = PARSESTAT_READY;
            return( PARSE_LEFT_CONTEXT );
            }
        else {

            /*** Es sollte also ein  normales keyword sein ***/
            if( parser->target ) {

                struct GameShellReq *GSR = parser->target;
                GSR->FoundContent |= DM_SOUND; // weil hier GSR da ist

                if( stricmp( parser->keyword, "channels") == 0 ) {

                    /*** Not longer supported ***/
                
                } else {

                if( stricmp( parser->keyword, "volume") == 0 ) {

                    GSR->fxvolume = (WORD) atol( parser->data );
                    _AE_SetAttrs( AET_MasterVolume, (LONG)(GSR->fxvolume), TAG_DONE);
                
                } else {

                if( stricmp( parser->keyword, "cdvolume") == 0 ) {

                    struct snd_cdvolume_msg cdv;
                    GSR->cdvolume = cdv.volume = (WORD) atol( parser->data );
                    _SetCDVolume( &cdv );
                
                } else {

                if( stricmp( parser->keyword, "invertlr") == 0 ) {

                    if( stricmp( parser->data, "yes" ) == 0 ) {
                        GSR->sound_flags     |=  SF_INVERTLR;
                        _AE_SetAttrs( AET_ReverseStereo, TRUE, TAG_DONE );
                        }
                    else {
                        GSR->sound_flags     &= ~SF_INVERTLR;
                        _AE_SetAttrs( AET_ReverseStereo, FALSE, TAG_DONE );
                        }

                } else {

                if( stricmp( parser->keyword, "sound") == 0 ) {

                    if( stricmp( parser->data, "yes" ) == 0 ) {
                        GSR->sound_flags      |=  SF_SOUND;
                        GSR->ywd->Prefs.Flags |= YPA_PREFS_SOUNDENABLE;
                    } else {
                        GSR->sound_flags      &= ~SF_SOUND;
                        GSR->ywd->Prefs.Flags &= ~YPA_PREFS_SOUNDENABLE;
                    }
                
                } else {

                if( stricmp( parser->keyword, "cdsound") == 0 ) {

                    if( stricmp( parser->data, "yes" ) == 0 ) {

                        struct snd_cdcontrol_msg cd;

                        GSR->sound_flags      |=  SF_CDSOUND;
                        GSR->ywd->Prefs.Flags |= YPA_PREFS_CDSOUNDENABLE;

                        cd.command = SND_CD_SWITCH;
                        cd.para    = 1;
                        _ControlCDPlayer( &cd );
                    } else {

                        struct snd_cdcontrol_msg cd;

                        GSR->sound_flags      &= ~SF_CDSOUND;
                        GSR->ywd->Prefs.Flags &= ~YPA_PREFS_CDSOUNDENABLE;

                        cd.command = SND_CD_SWITCH;
                        cd.para    = 0;
                        _ControlCDPlayer( &cd );
                    }
                
                } else {

                    return( PARSE_UNKNOWN_KEYWORD );
                    } } } } } }
                }

            return( PARSE_ALL_OK );
            }
        }
}
      


/* ---------------------------------------------------------------
**              die Saveroutinen
** -------------------------------------------------------------*/

BOOL yw_WriteUserData( FILE *ifile, char *name, struct GameShellReq *GSR)
{
/* --------------------------------------------------------
** Der Name des Users muß in Parameter[2] übergeben werden!
** ------------------------------------------------------*/

    char str[ 300 ];
    int  i;

    /*** Eröffnung ***/
    sprintf( str, "new_user\n\0" );
    _FWrite( str, strlen( str ), 1, ifile );

    for( i = 0; i < MAXNUM_ROBOS; i++ ) {

        sprintf( str, "    playerstatus = %d_%d_%d_%d_%d_%d_%d_%d\n\0", i,
                      GSR->ywd->GlobalStats[ i ].Kills,
                      GSR->ywd->GlobalStats[ i ].UserKills,
                      GSR->ywd->GlobalStats[ i ].Time,
                      GSR->ywd->GlobalStats[ i ].SecCons,
                      GSR->ywd->GlobalStats[ i ].Score,
                      GSR->ywd->GlobalStats[ i ].Power,
                      GSR->ywd->GlobalStats[ i ].Techs );
        _FWrite( str, strlen( str ), 1, ifile );
        }

    /*** maximale Roboenergie ***/
    sprintf( str, "    maxroboenergy = %d\n\0", GSR->ywd->MaxRoboEnergy );
    _FWrite( str, strlen( str ), 1, ifile );
    sprintf( str, "    maxreloadconst = %d\n\0", GSR->ywd->MaxReloadConst );
    _FWrite( str, strlen( str ), 1, ifile );
    
    if( ((UBYTE)(GSR->NPlayerName[0])) > 31 ) {
    
        char name[ 300 ];
        int i = 0;
        
        while( GSR->NPlayerName[ i ]) {
            if( GSR->NPlayerName[ i ] == ' ')
                name[ i ] = '_';
            else
                name[ i ] = GSR->NPlayerName[ i ];
            i++;
            }
        name[ i ] = 0;
                        
        sprintf( str, "    netname       = %s\n\0", name );
        _FWrite( str, strlen( str ), 1, ifile );
        }

    /*** Wieviele Buddies duerfen mit? ***/
    sprintf( str, "    numbuddies    = %d\n\0", GSR->ywd->Level->MaxNumBuddies );
    _FWrite( str, strlen( str ), 1, ifile );

    /*** Die Rassenkontakte ***/
    sprintf( str, "    jodiefoster   = \0" );
    for( i = 0; i < MAXNUM_OWNERS; i++ ) {
        char t[10];
        sprintf( t, "%d\0", GSR->ywd->Level->RaceTouched[ i ] );
        strcat( str, t );
        if( i < (MAXNUM_OWNERS-1) ) strcat( str, "_");
        }
    strcat( str, "      ; the contact flags\n\0");
    _FWrite( str, strlen( str ), 1, ifile );

    /*** Abschluss ***/
    sprintf( str, "end\n\n\0" );
    _FWrite( str, strlen( str ), 1, ifile );
        
    return( TRUE );
}


BOOL yw_WriteShellData( FILE *ifile, struct GameShellReq *GSR )
{
/* --------------------------------------------------------------------------
** schreibt alle Positionen und infos zu den Requestern der shell in das File
** ------------------------------------------------------------------------*/

    char   str[ 300 ];
    struct ypaworld_data *ywd;

    ywd = GSR->ywd;

    /*** Eröffnung ***/
    sprintf( str, "new_shell\n\0" );
    _FWrite( str, strlen( str ), 1, ifile );


    /*** Die Sprache ***/
    if( GSR->lsel ) {

        /*** Nur, wenn auch was eingestellt war ***/
        sprintf( str, "    language = %s\n\0", GSR->lsel->language );
        _FWrite( str, strlen( str ), 1, ifile );
        }

    if( ywd->Prefs.valid ) {

        char o_oder_c[ 50 ];
        strcpy( o_oder_c, "closed" ); // vorerst pauschal für alle

        /*** Der Finder ***/
        sprintf( str, "    finder = %s_%d_%d_%d_%d\n\0", o_oder_c,
                           ywd->Prefs.WinFinder.rect.x, ywd->Prefs.WinFinder.rect.y,
                           ywd->Prefs.WinFinder.rect.w, ywd->Prefs.WinFinder.rect.h );
        _FWrite( str, strlen( str ), 1, ifile );

        /*** das LogWindow (Nyheter) ***/
        sprintf( str, "    log    = %s_%d_%d_%d_%d\n\0", o_oder_c,
                           ywd->Prefs.WinLog.rect.x, ywd->Prefs.WinLog.rect.y,
                           ywd->Prefs.WinLog.rect.w, ywd->Prefs.WinLog.rect.h );
        _FWrite( str, strlen( str ), 1, ifile );

        /*** Das EnergyWindow ***/
        sprintf( str, "    energy = %s_%d_%d_%d_%d\n\0", o_oder_c,
                           ywd->Prefs.WinEnergy.rect.x, ywd->Prefs.WinEnergy.rect.y,
                           ywd->Prefs.WinEnergy.rect.w, ywd->Prefs.WinEnergy.rect.h );
        _FWrite( str, strlen( str ), 1, ifile );

        #ifdef __NETWORK__
        /*** Das MessageWindow ***/
        sprintf( str, "    message = %s_%d_%d_%d_%d\n\0", o_oder_c,
                           ywd->Prefs.WinMessage.rect.x, ywd->Prefs.WinMessage.rect.y,
                           ywd->Prefs.WinMessage.rect.w, ywd->Prefs.WinMessage.rect.h );
        _FWrite( str, strlen( str ), 1, ifile );
        #endif

        /*** Die Map ***/
        sprintf( str, "    map    = %s_%d_%d_%d_%d_%d_%d\n\0", o_oder_c,
                           ywd->Prefs.WinMap.rect.x, ywd->Prefs.WinMap.rect.y,
                           ywd->Prefs.WinMap.rect.w, ywd->Prefs.WinMap.rect.h,
                           ywd->Prefs.MapLayers,     ywd->Prefs.MapZoom );
        _FWrite( str, strlen( str ), 1, ifile );
        }

    /*** Abschluss ***/
    sprintf( str, "end\n\n\0" );
    _FWrite( str, strlen( str ), 1, ifile );
    
    return( TRUE );
}


BOOL yw_WriteInputData( FILE *ifile, struct GameShellReq *GSR )
{
/* ------------------------------------------------------------------------
** Wir schreiben die Daten in das File. Alle Leerzeichen werden durch
** Untersctriche ersetzt. Hoztkeys werden nur dann in das File geschrieben, 
** wenn die deklariert wurden, also der pos-Wert != 0 ist
**
** ACHTUNG, ein return( FALSE) führt meist zum Abbruch des Schreibvorgan-
** ges, der komplexer sein kann.
** ----------------------------------------------------------------------*/

    int   i;
    char  str[ 300 ];


    /*** Eröffnung ***/
    sprintf( str, "new_input\n\0" );
    _FWrite( str, strlen( str ), 1, ifile );

    /*** Joystick ***/
    if( GSR->ywd->Prefs.Flags & YPA_PREFS_JOYDISABLE )
        sprintf( str, "    joystick = no\n\0");
    else
        sprintf( str, "    joystick = yes\n\0");
    _FWrite( str, strlen( str ), 1, ifile );

    /*** ForceFeedback ***/
    if( GSR->ywd->Prefs.Flags & YPA_PREFS_FFDISABLE )
        sprintf( str, "    forcefeedback = no\n\0");
    else
        sprintf( str, "    forcefeedback = yes\n\0");
    _FWrite( str, strlen( str ), 1, ifile );

    for( i = 1; i <= NUM_INPUTEVENTS; i++ ) {

        switch( GSR->inp[ i ].kind ) {

            case GSI_BUTTON:

                /*** Anfang mit Keyword ***/
                sprintf( str, "    input.button[%d] = \0", GSR->inp[ i ].number );

                /*** Der Name des Ereignisses ***/
                strcat( str, KEYBORD_DRIVER_SIGN );
                if( GSR->inp[ i ].pos == 0 ) {

                    /*** Muß ja wegen Ausschalten auch möglich sein ***/
                    strcat( str, "nop");
                    }
                else
                    strcat( str, GlobalKeyTable[ GSR->inp[ i ].pos ].config );

                /*** Kommentar ***/
                strcat(str, "         ; ");
                strcat(str, GSR->inp[ i ].menuname );

                strcat( str, "\n");

                /*** Reinschreiben ***/                
                _FWrite( str, strlen( str ), 1, ifile );

                /*** Hier evtl. noch Joysticksupport ***/
                break;

            case GSI_SLIDER:

                /*** Anfang mit Keyword ***/
                sprintf( str, "    input.slider[%d] = \0", GSR->inp[ i ].number );

                /*** Der Name des Ereignisses ***/

                /*** negativ ***/
                strcat( str, "~#" );
                strcat( str, KEYBORD_DRIVER_SIGN );

                if( GSR->inp[ i ].neg == 0 ) {

                    _LogMsg("Slider(neg) %s is not declared!\n", GSR->inp[ i ].menuname);
                    _LogMsg("Use space-key for it\n");

                    /*** Ersatz ist space ***/
                    strcat( str, "space");
                    }
                else
                    strcat( str, GlobalKeyTable[ GSR->inp[ i ].neg ].config );
                
                /*** positiv ***/
                strcat( str, "_#" );
                strcat( str, KEYBORD_DRIVER_SIGN );

                if( GSR->inp[ i ].pos == 0 ) {

                    _LogMsg("Slider(pos) %s is not declared!\n", GSR->inp[ i ].menuname);
                    _LogMsg("Use space-key for it\n");

                    /*** Ersatz ist space ***/
                    strcat( str, "space");
                    }
                else
                    strcat( str, GlobalKeyTable[ GSR->inp[ i ].pos ].config );

                /*** Kommentar ***/
                strcat(str, "         ; ");
                strcat(str, GSR->inp[ i ].menuname );
                
                strcat( str, "\n");

                /*** Reinschreiben ***/                
                _FWrite( str, strlen( str ), 1, ifile );

                /*** Hier evtl. noch Joysticksupport ***/
                break;

            case GSI_HOTKEY:

                /*** Anfang mit Keyword ***/
                sprintf( str, "    input.hotkey[%d] = \0", GSR->inp[ i ].number );

                if( GSR->inp[ i ].pos != 0 )
                    strcat( str, GlobalKeyTable[ GSR->inp[ i ].pos ].config );
                else
                    strcat( str, "nop" );

                /*** Kommentar ***/
                strcat(str, "         ; ");
                strcat(str, GSR->inp[ i ].menuname );
                
                strcat( str, "\n");

                /*** Reinschreiben ***/                
                _FWrite( str, strlen( str ), 1, ifile );

                break;
            }
        }

    /*** Abschluss ***/
    sprintf( str, "end\n\n\0" );
    _FWrite( str, strlen( str ), 1, ifile );

    return( TRUE );
}


BOOL yw_WriteVideoData( FILE *ifile, struct GameShellReq *GSR )
{
/* ----------------------------------------------
** Die Daten stehen in der GSR, besonders in VSel
** --------------------------------------------*/

    char str[ 300 ];

    /*** Eröffnung ***/
    sprintf( str, "new_video\n\0" );
    _FWrite( str, strlen( str ), 1, ifile );

    /*** Die Video-Modi ***/
    sprintf( str, "    videomode = %d\n\0", GSR->ywd->GameRes );
    _FWrite( str, strlen( str ), 1, ifile );

    /*** Weitsicht ***/
    if( GSR->video_flags & VF_FARVIEW )
        sprintf( str,"    farview = yes\n\0");
    else
        sprintf( str,"    farview = no\n\0");
    _FWrite( str, strlen( str ), 1, ifile );

    /*** Himmel ***/
    if( GSR->video_flags & VF_HEAVEN )
        sprintf( str,"    heaven = yes\n\0");
    else
        sprintf( str,"    heaven = no\n\0");
    _FWrite( str, strlen( str ), 1, ifile );

    /*** Softmouse ***/
    if( GSR->video_flags & VF_SOFTMOUSE )
        sprintf( str,"    softmouse = yes\n\0");
    else
        sprintf( str,"    softmouse = no\n\0");
    _FWrite( str, strlen( str ), 1, ifile );

    /*** Filtering ***/
    if( GSR->video_flags & VF_FILTERING )
        sprintf( str,"    filtering = yes\n\0");
    else
        sprintf( str,"    filtering = no\n\0");
    _FWrite( str, strlen( str ), 1, ifile );

    /*** Indikatoren ***/
    if( GSR->enemyindicator )
        sprintf( str,"    enemyindicator = yes\n\0");
    else
        sprintf( str,"    enemyindicator = no\n\0");
    _FWrite( str, strlen( str ), 1, ifile );

    /*** fx-zahl ***/
    sprintf( str,"    fxnumber = %d\n\0", GSR->destfx );
    _FWrite( str, strlen( str ), 1, ifile );

    /*** Abschluss ***/
    sprintf( str, "end\n\n\0" );
    _FWrite( str, strlen( str ), 1, ifile );
    
    return( TRUE );
}


BOOL yw_WriteSoundData( FILE *ifile, struct GameShellReq *GSR )
{

    char str[ 300 ];

    /*** Eröffnung ***/
    sprintf( str, "new_sound\n\0" );
    _FWrite( str, strlen( str ), 1, ifile );

    /*** Lautstärke für Spiel ***/
    sprintf( str, "    volume = %d\n\0", GSR->fxvolume );
    _FWrite( str, strlen( str ), 1, ifile );

    /*** Lautstärke für CD-Tracks***/
    sprintf( str, "    cdvolume = %d\n\0", GSR->cdvolume );
    _FWrite( str, strlen( str ), 1, ifile );

    /*** Überhaupt Sound? ***/
    if( GSR->sound_flags & SF_SOUND )
        sprintf( str,"    sound = yes\n\0");
    else
        sprintf( str,"    sound = no\n\0");
    _FWrite( str, strlen( str ), 1, ifile );

    /*** Links-rechts-Umkehrung ***/
    if( GSR->sound_flags & SF_INVERTLR )
        sprintf( str,"    invertlr = yes\n\0");
    else
        sprintf( str,"    invertlr = no\n\0");
    _FWrite( str, strlen( str ), 1, ifile );

    /*** CD-Audio ***/
    if( GSR->sound_flags & SF_CDSOUND )
        sprintf( str,"    cdsound = yes\n\0");
    else
        sprintf( str,"    cdsound = no\n\0");
    _FWrite( str, strlen( str ), 1, ifile );

    /*** Abschluss ***/
    sprintf( str, "end\n\n\0" );
    _FWrite( str, strlen( str ), 1, ifile );
    return( TRUE );
}


BOOL yw_WriteScoreData( FILE *ifile, struct GameShellReq *GSR )
{
    /* ------------------------------------------------------
    ** Das hier ist nur die Routine zum Abspeichern eines
    ** Zwischen-level-Spiel-standes. Dazu gehören vorerst die
    ** Statusinformationen und die Buddies (das sind die, die
    ** ziemlich mitgenommen aussehen)
    ** NEU: Buddies jetzt extra!
    ** ----------------------------------------------------*/

    return( yw_SaveAllLevelStatus( ifile, GSR->ywd ) );
}



/* ---------------------------------------------------------------
**              spezielle Hilfsroutinen
** -------------------------------------------------------------*/

LONG yw_GetInternalNumber( ULONG type, ULONG number )
{
/*  -------------------------------------------------------------------------
**  number gibt die Nummer für nucleus an und type sagt, worum es sich
**  handelt. Unsere Aufgabe ist es nun, rauszukriegen, welche interne Nummer,
**  also die , die den Eintrag im Menü und im inp-Feld definiert,
**  dazugehört. Weil das eine mit dem anderen nix zu tun hat, wird
**  es wohl auf ein switch-Statement hinauslaufen
**
**  Bei ner -1 hammer nüscht gfunden
**  -----------------------------------------------------------------------*/

    LONG value = -1;  // Fehler vorinitialisieren

    switch( type ) {

        case GSI_SLIDER:

            switch( number ) {

                case 0: /*** Flug Richtung ***/
                        value = I_FLY_DIR;    break;

                case 1: /*** Flug Höhe ***/
                        value = I_FLY_HEIGHT; break;

                case 2: /*** Flug Speed ***/
                        value = I_FLY_SPEED;  break;

                case 3: /*** Boden Richtung ***/
                        value = I_DRV_DIR;    break;

                case 4: /*** Boden Speed ***/
                        value = I_DRV_SPEED;  break;

                case 5: /*** Gun Height ***/
                        value = I_GUN_HEIGHT; break;
                }
            
            break;

        case GSI_BUTTON:

            switch( number ) {

                case 0: /*** feuern ***/
                        value = I_FIRE;     break;

                case 1: /*** feuern ***/
                        value = I_FIREVIEW; break;

                case 2: /*** feuern ***/
                        value = I_FIREGUN;  break;

                case 3: /*** stoi ***/
                        value = I_STOP;     break;

                case 4: /*** wegpunkt ***/
                        value = I_WAYPOINT; break;
                }

            break;

        case GSI_HOTKEY:

            /* -----------------------------------------------------------
            ** Um das zu verallgemeinern, müßte man die HOTKEY_...-Reihen-
            ** folge und die I_...-Reihemfolge anpassen
            ** ---------------------------------------------------------*/
            switch( number ) {

                case HOTKEY_HUD  - 128:         value = I_HUD;         break;
                case HOTKEY_ORDER  - 128:       value = I_ORDER;       break;
                case HOTKEY_FIGHT  - 128:       value = I_FIGHT;       break;
                case HOTKEY_NEW    - 128:       value = I_NEW;         break;
                case HOTKEY_ADD    - 128:       value = I_ADD;         break;
                case HOTKEY_CONTROL - 128:      value = I_CONTROL;     break;
                case HOTKEY_AUTOPILOT - 128:    value = I_AUTOPILOT;   break;
                case HOTKEY_MAP - 128:          value = I_MAP;         break;
                case HOTKEY_FINDER - 128:       value = I_FINDER;      break;
                case HOTKEY_MAP_LAND - 128:     value = I_LANDSCAPE;   break;
                case HOTKEY_MAP_OWNER - 128:    value = I_OWNER;       break;
                case HOTKEY_MAP_HEIGHT - 128:   value = I_HEIGHT;      break;
                case HOTKEY_MAP_LOCKVWR - 128:  value = I_LOCKVIEWER;  break;
                case HOTKEY_MAP_ZOOMIN - 128:   value = I_ZOOMIN;      break;
                case HOTKEY_MAP_ZOOMOUT - 128:  value = I_ZOOMOUT;     break;
                case HOTKEY_MAP_MINIMIZE - 128: value = I_MAPMINI;     break;
                case HOTKEY_NEXTCMDR - 128:     value = I_NEXTCOM;     break;
                case HOTKEY_TOROBO - 128:       value = I_TOROBO;      break;
                case HOTKEY_NEXTUNIT - 128:     value = I_NEXTMAN;     break;
                case HOTKEY_TOCMDR - 128:       value = I_TOCOMMANDER; break;
                case HOTKEY_QUIT - 128:         value = I_QUIT;        break;
                case HOTKEY_LOGWIN - 128:       value = I_LOGWIN;      break;
                case HOTKEY_NEXTITEM - 128:     value = I_NEXTITEM;    break;
                case HOTKEY_PREVITEM - 128:     value = I_PREVITEM;    break;
                case HOTKEY_CTRL2LM - 128:      value = I_LASTMSG;     break;
                case HOTKEY_PAUSE - 128:        value = I_PAUSE;       break;
                case HOTKEY_TOALL - 128:        value = I_TOALL;       break;
                case HOTKEY_AGGR1 - 128:        value = I_AGGR1;       break;
                case HOTKEY_AGGR2 - 128:        value = I_AGGR2;       break;
                case HOTKEY_AGGR3 - 128:        value = I_AGGR3;       break;
                case HOTKEY_AGGR4 - 128:        value = I_AGGR4;       break;
                case HOTKEY_AGGR5 - 128:        value = I_AGGR5;       break;
                case HOTKEY_HELP - 128:         value = I_HELP;       break;
                case HOTKEY_LASTOCCUPIED - 128:  value = I_LASTOCCUPIED;  break;
                case HOTKEY_MAKECOMMANDER - 128: value = I_MAKECOMMANDER; break;
                }

            break;
        }

    return( value );
}


LONG yw_GetKeycode( char *str )
{
/*  -------------------------------------------------------------------------
**  str zeigt auf den Namen aus dem Config-file. Wir haben alle Config-Namen,
**  die in Frage kommen, gespeichert. Also müssen wir vergleichen. Eine
**  -1 bedeutet, daß wir nix gefunden haben
**  -----------------------------------------------------------------------*/

    LONG i;

    for( i = 0; i < 256; i++ ) {

        if( GlobalKeyTable[ i ].config )
            if( stricmp( GlobalKeyTable[ i ].config, str ) == 0 )
                return( i );
        }

    /*** Hamma nichts gefunden ***/
    return( -1 );
}


void yw_SetDestFX( struct GameShellReq *GSR )
{
    GSR->ywd->NumDestFX = GSR->destfx;
}


void yw_ClearBuddyArray( struct ypaworld_data *ywd )
{
    ywd->Level->NumBuddies = 0;
}
