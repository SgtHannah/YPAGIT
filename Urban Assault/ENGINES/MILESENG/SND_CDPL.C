/*
**  $Source: PRG:VFM/Engines/MilesEngine/snd_cdplay.c,v $
**  $Revision: 38.1 $
**  $Date: 1998/01/06 16:40:01 $
**  $Locker:  $
**  $Author: floh $
**
**  SoundKit, kapselt die DOS-/Win-Version von AIL in ein
**  einheitliches Interface.
**
**  (C) Copyright 1997 by A.Weissflog
*/
#include "thirdparty/mss.h"

#ifndef NULL
#define NULL (0)
#endif

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#include "audio/soundkit.h"
#include "audio/cdplay.h"

extern struct cd_data cd_data1;
#define cd_data cd_data1
HREDBOOK redbook;   // muss auf grund von Typenproblemen extra sein

/*-----------------------------------------------------------------*/
unsigned long audio_InitCDPlayer()
/*
**  FUNCTION
**      Initialisierung des CD-Players. Das Handle merke ich mir intern.
**      Der TrackCount wird 0, das heißt wir spielen nix...
**
**  INPUTS
**
**  RESULTS
**      TRUE, wenn alles geklappt hat.
**
**  CHANGED
*/
{
    memset( &cd_data, 0, sizeof( cd_data ) );

    if( redbook = AIL_redbook_open( 0 ) ) {

        /*** erfolgreich ***/

        /*** grundsaetzlich erstmal einschalten ***/
        cd_data.on = TRUE;

        return( TRUE );
        }
    else {

        /*** Fehler ***/
        return( FALSE );
        }
}
void audio_FreeCDPlayer()
/*
**  FUNCTION
**      CD-Player wieder aufräumen und ausschalten
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
*/
{
    if( redbook ) {

        /*** Vorsichtshalber stoppen ***/
        if( cd_data.on )
            AIL_redbook_stop( redbook );

        /*** Cleanup ***/
        AIL_redbook_close( redbook );

        redbook = 0;
        }
}
unsigned long audio_ControlCDPlayer( struct snd_cdcontrol_msg *cdmsg)
/*
**  FUNCTION
**      Setzt den Playermodus.
**      Achtung! Tracknummern gehen von 2 .. Ende! 1 ist Spiel-Track!
**
**  INPUTS
**
**  RESULTS
**      TRUE, wenn die Sache geklappt hat.
**
**  CHANGED
*/
{

    if( redbook ) {

        if( cd_data.on ) {

            switch( cdmsg->command ) {

                case SND_CD_PLAY:

                    /*** schaltet auf "Spielen", analog wie in Trigger ***/
                    AIL_redbook_track_info( redbook, cd_data.track, &(cd_data.starts), &(cd_data.ends) );
                    AIL_redbook_play( redbook, cd_data.starts+1, cd_data.ends );
                    cd_data.begins = cd_data.timestamp;
                    break;

                case SND_CD_STOP:

                    /*** Stoptaste ***/
                    AIL_redbook_stop( redbook );
                    cd_data.track = 0;       // das heißt, wir spielen nichts
                    break;

                case SND_CD_PAUSE:

                    /*** Anhalten ***/
                    if( REDBOOK_PAUSED != AIL_redbook_status( redbook ) )
                        AIL_redbook_pause( redbook );
                    break;

                case SND_CD_RESUME:

                    /*** Weitermachen nach pause ***/
                    if( REDBOOK_PAUSED == AIL_redbook_status( redbook ) )
                        AIL_redbook_resume( redbook );
                    break;

                case SND_CD_NEXT:

                    /*** Nächster Titel, so dies möglich ist ***/
                    if( cd_data.track < AIL_redbook_tracks( redbook ) ) {
                        cd_data.track++;
                        if( REDBOOK_PLAYING ==  AIL_redbook_status( redbook ) ) {
                            AIL_redbook_track_info( redbook, cd_data.track, &(cd_data.starts), &(cd_data.ends) );
                            AIL_redbook_play( redbook, cd_data.starts+1, cd_data.ends );
                            cd_data.begins = cd_data.timestamp;
                            }
                        }
                    break;

                case SND_CD_PREV:

                    /*** Vorheriger Titel ***/
                    if( cd_data.track > 2 ) {
                        cd_data.track--;
                        if( REDBOOK_PLAYING ==  AIL_redbook_status( redbook ) ) {
                            AIL_redbook_track_info( redbook, cd_data.track, &(cd_data.starts), &(cd_data.ends) );
                            AIL_redbook_play( redbook, cd_data.starts+1, cd_data.ends );
                            cd_data.begins = cd_data.timestamp;
                            }
                        }
                    break;

                case SND_CD_SETTITLE:

                    /*** Titel direkt setzen ***/
                    if( (cdmsg->para <= AIL_redbook_tracks( redbook )) &&
                        (cdmsg->para >= 2) ) {

                        cd_data.track  = cdmsg->para;

                        /*** Hier werden auch Verzoegerungen gesetzt ***/
                        cd_data.min_delay = cdmsg->min_delay;
                        cd_data.max_delay = cdmsg->max_delay;
                        if( REDBOOK_PLAYING ==  AIL_redbook_status( redbook ) ) {
                            AIL_redbook_track_info( redbook, cd_data.track, &(cd_data.starts), &(cd_data.ends) );
                            AIL_redbook_play( redbook, cd_data.starts+1, cd_data.ends );
                            cd_data.begins = cd_data.timestamp;
                            }
                        }
                    break;

                case SND_CD_SWITCH:

                    /*** Evtl. ausschalten ***/
                    if( 0 == cdmsg->para ) {

                        /*** kein Stop! (sonst stop nach laden) ***/
                        cd_data.track = 0;       // das heißt, wir spielen nichts
                        cd_data.on    = FALSE;
                        }
                    break;
                }
            }
        else {

            /*** hoechstens wieder einschalten ***/
            if( (SND_CD_SWITCH == cdmsg->command) &&
                (0             != cdmsg->para) )
                cd_data.on = TRUE;
            }

        return( TRUE );
        }
    else return( FALSE );   // denn sonst klappt gar nix...
}
unsigned long audio_TriggerCDPlayer( struct snd_cdtrigger_msg *cdmsg )
/*
**  FUNCTION
**      Übernimmt alle periodisch zu erledigenden Aufgaben wie erneutes
**      Starten, Faden usw.
**      In starts und ends stehen Start- und Endposition des Titels
**      in msec.
**
**  INPUTS
**
**  RESULTS
**      TRUE, wenn die Sache geklappt hat.
**
**  CHANGED
**      Neu: Zwischen  den Titeln kann nun eine zufaellig zwischen
**           min_delay und max_delay liegende Zeit sein.
**      Alles in eine struktur
*/
{
    if( redbook && cd_data.on ) {

        if( cd_data.wait ) {

            /*** ich warte schon. Zu lange? Dann wieder spielen ***/
            if( (cd_data.timestamp - cd_data.waitbegins) >= cd_data.delay ) {

                cd_data.wait = FALSE;

                AIL_redbook_track_info( redbook, cd_data.track, &(cd_data.starts), &(cd_data.ends) );
                AIL_redbook_play( redbook, cd_data.starts+1, cd_data.ends );
                cd_data.begins = cd_data.timestamp;
                }
            }
        else {

            /*** Zur Zeit Spielen ***/
            if( (cd_data.timestamp - cd_data.begins) > (cd_data.ends - cd_data.starts) ) {

                /*** "Abgelaufenes Lied erneut starten? ***/
                if( cdmsg->playitagainsam && cd_data.track ) {

                    /*** zeit auswuerfeln ***/
                    cd_data.wait       = TRUE;
                    cd_data.waitbegins = cd_data.timestamp;
                    cd_data.delay      = cd_data.min_delay +
                       (cd_data.max_delay - cd_data.min_delay)*
                       (cd_data.timestamp % 30) / 30;
                    }
                }
            }
        }

    return( 1L );
}
unsigned long audio_SetCDVolume( struct snd_cdvolume_msg *cdv )
{
/*
**  FUNCTION    Reguliert die Lautstärke für die CD
**
**  INPUT       die Lautstärke
**
**  RESULTS     TRUE bei Erfolg
**
**  CHANGED
*/

    if( redbook && cd_data.on ) {

        unsigned long rvolume;

        rvolume = AIL_redbook_set_volume( redbook, cdv->volume );

        if( rvolume == cdv->volume )
            return( 1L );
        else
            return( 0L );
        }

    return( 0L );
}

