#ifndef YPA_GUILOGWIN_H
#define YPA_GUILOGWIN_H
/*
**  $Source: PRG:VFM/Include/ypa/guilogwin.h,v $
**  $Revision: 38.7 $
**  $Date: 1998/01/06 14:23:02 $
**  $Locker:  $
**  $Author: floh $
**
**  Im Log-Window werden die hereinkommenden Botschaften
**  angezeigt, die letzten so-und-so-viel Messages
**  werden aufgehoben und können per Scroll-Bar begutachtet
**  werden.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef YPA_GUILIST_H
#include "ypa/guilist.h"
#endif

/*-----------------------------------------------------------------*/
#define LW_NumChars     (64)    // Anzahl Zeichen pro Zeile
#define LW_NumLines     (64)    // Anzahl Zeilen im Buffer
#define LW_Countdown	(4000)	// jede Zeile wird solang angezeigt

struct LW_Line {
    ULONG sender_id;            // wer hat diese Message abgeschickt?
    LONG time_stamp;            // wann ist das passiert?
    LONG count_down;			// Anzeige-Countdown
    UBYTE line[LW_NumChars];    // Platz für 1 Zeile
};

/*-----------------------------------------------------------------*/
struct YPALogWin {
    struct YPAListReq l;    // eingebettete Listen-Requester-Struktur

    /*** Dynamic Layout ***/
    LONG skip_pixels;       // Skip-Pixels als Platz für Zeit-Angabe

    /*** Zeugs ***/
    LONG last_log;          // Zeilen-Position des letzten LogMsg
    LONG last_pri;          // Priorität der letzten LogMsg

    LONG first;             // Ringbuffer-Index, erste Zeile
    LONG last;              // Ringbuffer-Index, letzte Zeile
    struct LW_Line line_buf[LW_NumLines];   // der Zeilen-Ring-Buffer

    ULONG lm_senderid;      // Last-Message-Sender-ID
    ULONG lm_timestamp;     // Last-Message-Timestamp
    ULONG lm_code;          // Special-Code der letzten Message
    ULONG lm_numlines;      // Anzahl Zeilen der letzten Message

    LONG quick_log_counter; // Countdown für Quicklog
};

/*** Layout-Konstanten ***/
#define LW_ItemFont     (FONTID_DEFAULT)    // default.font

/*-----------------------------------------------------------------*/
#endif

