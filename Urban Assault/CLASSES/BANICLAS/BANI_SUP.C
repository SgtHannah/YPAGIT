/*
**  $Source: PRG:VFM/Classes/_BmpAnimClass/bmpanim_support.c,v $
**  $Revision: 38.7 $
**  $Date: 1996/02/28 23:13:21 $
**  $Locker:  $
**  $Author: floh $
**
**  Support-Funktionen für bmpanim.class
**
**  (C) Copyright 1994 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "types.h"
#include "nucleus/nucleus2.h"
#include "nucleus/nukedos.h"
#include "bitmap/bmpanimclass.h"

/*-------------------------------------------------------------------
**  externe Referenzen
*/
_extern_use_nucleus

extern BOOL IsIFF;

APTR bmpanim_FOpen(UBYTE *name, UBYTE *mode);
LONG bmpanim_FClose(APTR handle);
LONG bmpanim_FRead(APTR b, ULONG bsize, ULONG n, APTR fp);
LONG bmpanim_FWrite(APTR b, ULONG bsize, ULONG n, APTR fp);

/*-------------------------------------------------------------------
**  eigene Globale Variablen
*/
#define MAXNUM_ELEMENTS 256 /* max. Anzahl Bitmaps oder Outlines */

#ifdef AMIGA
__far struct VFMOutline *OutlineLUT[MAXNUM_ELEMENTS];
__far UBYTE **FilenameLUT[MAXNUM_ELEMENTS];
#else
struct VFMOutline *OutlineLUT[MAXNUM_ELEMENTS];   /* globale Outline Look Up Table */
UBYTE **FilenameLUT[MAXNUM_ELEMENTS];   /* globale Filename Look Up Table */
#endif
/*===================================================================
**  Der DICE bietet ein paar geile String-Funktionen, die nicht ANSI
**  sind, glücklicherweise sind beim DICE die Lib-Sources dabei...
*/
#ifndef AMIGA
int strbpl(char **av, int max, char *sary)
{
    int i;

    for (i = 0; i < max; ++i) {
        if (*sary == 0) {
            *av++ = NULL;
            return(i);
        }
        *av++ = sary;
        while (*sary)
            ++sary;
        ++sary;
    }
    return(-1);
}

char *stpcpy(char *d, char *s)
{
    while (*d++ = *s++);
    return(d-1);
}
#endif

/*-----------------------------------------------------------------*/
BOOL createFnObjPool(struct sequence_header *sh,
                     UBYTE **fn_pool_array,
                     UBYTE *loaderclass)
/*
**  FUNCTION
**      Erzeugt aus dem <fn_pool_array> den Filename-Pool
**      und den Object-Pool, initialisiert diese vollständig
**      und hängt sie an den <sequence_header>
**
**  INPUTS
**      sh  -> Sequence-Header, für den der Filenamepool erzeugt
**             werden soll
**      fn_pool_array   -> == BANIMA_FilenamePool
**      loaderclass     -> == BANIMA_LoaderClass
**
**  RESULTS
**      TRUE    -> alles ok, in <sh->filename_pool> steht der
**                 Pointer auf den frisch erzeugten Filenamepool
**      FALSE   -> Not Enough Mem - killBmpanimResource() MUSS AUFGERUFEN WERDEN!
**                 (kann aber auch passieren, ein Bitmap-Object konnte
**                 aus irgendeinem Grund nicht geladen werden).
**
**      Im sequence_header werden ausgefüllt:
**          sh->filename_pool
**          sh->object_pool
**          sh->sizeof_fnpool
**          sh->ne_objpool
**          sh->loaderclass
**
**  CHANGED
**      25-Oct-94   floh    created + debugged
**      21-Jan-95   floh    Nucleus2-Revision
**      12-Jan-96   floh    revised & updated
*/
{
    UBYTE **t_array;
    UBYTE *t_pool;
    UBYTE *t_next_pool;
    struct object_info *o_pool;
    UWORD size, numof;

    /* dupliziere LoaderClass nach SequenceHeader */
    sh->loader_class = _AllocVec(strlen(loaderclass)+1,MEMF_PUBLIC);
    if (sh->loader_class) strcpy(sh->loader_class,loaderclass);
    else                  return(FALSE);

    /* ermittle Größe des Filename-Pools und Anzahl Filenames... */
    size  = 0;
    numof = 0;
    t_array = fn_pool_array;
    while (*t_array)  {
        size += strlen(*t_array++)+1;
        numof++;
    };
    size++; /* für zusätzliche "Abschluß-0" */

    /* trage die gerade ermittelten Werte in sh ein */
    sh->sizeof_fnpool = size;
    sh->ne_objpool    = numof;

    /* allokiere Filename-Pool */
    t_pool = _AllocVec(size,MEMF_PUBLIC);
    if (!t_pool) return(FALSE);
    sh->filename_pool = t_pool;

    /* allokiere Object-Pool */
    o_pool = _AllocVec(numof*sizeof(struct object_info), MEMF_PUBLIC|MEMF_CLEAR);
    if (!o_pool) return(FALSE);
    sh->object_pool = o_pool;

    /* fülle Filename und Object-Pool aus */
    t_array = fn_pool_array;
    while (*t_array) {

        /* kopiere Filename nach Pool, t_next_pool auf nächsten Slot */
        t_next_pool = stpcpy(t_pool, *t_array) +1;

        /* erzeuge Bitmap-Object und fülle ObjectInfo aus */
        /* Bitmap-Object OHNE Outline und mit Direktverweis in FN-Pool! */
        o_pool->bitmap_object = (Object *)
            _new(loaderclass, RSA_Name, t_pool,
                              RSA_DontCopy, TRUE,
                              TAG_DONE);
        if (!(o_pool->bitmap_object)) return(FALSE);

        /* schreibe VFMBitmap des Bitmap-Objects nach ObjectInfo */
        _get(o_pool->bitmap_object, BMA_Bitmap, &(o_pool->bitmap));

        /* und der Filename-Verweis in ObjectInfo... */
        o_pool->pict_filename = t_pool;

        /* update alle Pointer */
        t_array++;
        o_pool++;
        t_pool = t_next_pool;
    };

    /* letzte endgültig abschließende 0 nach Filename-Pool */
    *t_pool = 0;

    /* das war's schon */
    return(TRUE);
}

/*-----------------------------------------------------------------*/
ULONG ne_OldOutline(Pixel2D *outline)
/*
**  FUNCTION
**      Ermittelt Anzahl Pixel2D's in einer gegebenen Outline.
**      Die Größe der Outline in Bytes ergibt sich aus:
**          size = ne_Outline(outline)*sizeof(Pixel2D);
**
**      Das Ende-Element wird mitgezählt!
**
**  INPUTS
**      outline -> Pointer auf eine gültige Outline
**
**  RESULTS
**      Anzahl Pixel2D's in der Outline inklusive Ende-Element
**
**  CHANGED
**      25-Oct-94   floh    created + debugged
**      21-Jan-95   floh    Nucleus2-Revision
**      07-Jul-95   floh    umbenannt nach ne_OldOutline()
**      12-Jan-96   floh    revised & updated
*/
{
    ULONG ne = 1;
    while (outline++->flags >= 0) ne++;
    return(ne);
}

/*-----------------------------------------------------------------*/
ULONG ne_NewOutline(struct VFMOutline *outline)
/*
**  FUNCTION
**      Ermittelt Anzahl Koords in neuer Outline-Definition
**      (FLOATs). Das Ende-Element wird mitgezählt.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      07-Jul-95   floh    created
**      12-Jan-96   floh    revised & updated
*/
{
    ULONG ne = 1;
    while (outline++->x >= 0.0) ne++;
    return(ne);
}

/*-----------------------------------------------------------------*/
BOOL createOutlinePool(struct sequence_header *sh,
                       Pixel2D **ol_pool_array)
/*
**  FUNCTION
**      Erzeugt aus dem <ol_pool_array> den vollständigen Outline-Pool
**      und trägt die Pointer auf die einzelnen Outlines im
**      Pool in die globale Variable OutlineLUT ein.
**
**  INPUTS
**      sh  -> Sequence-Header, für den der Outline-Pool erzeugt werden
**             soll.
**      ol_pool_array == BANIMA_OutlinePool
**
**  RESULTS
**      TRUE    -> alles OK
**      FALSE   -> Not Enough Mem
**                 [killBmpanimResource()] MUSS AUFGERUFEN WERDEN!
**
**      In der globalen Variablen OutlineLUT sind die Pointer
**      auf die Outline IM POOL zu finden:
**          OutlineLUT[0] -> Pointer auf erste Outline
**          OutlineLUT[1] -> Pointer auf zweite Outline etc...
**
**      Wenn alles geklappt hat, sind folgende Felder im
**      Sequence-Header ausgefüllt:
**          sh->outline_pool    der frische Outline-Pool
**          sh->ne_olpool       Gesamt-Anzahl Elemente im Outline-Pool
**
**  CHANGED
**      25-Oct-94   floh    created + debugged
**      21-Jan-95   floh    Nucleus2-Revision
**      07-Jul-95   floh    erzeugt jetzt einen Pool im neuen 
**                          VFMOutline-Format (FLOATs).
**      12-Jan-96   floh    revised & updated
*/
{
    Pixel2D **t_array;
    struct VFMOutline *t_pool;
    UWORD ne, index;

    /* ermittle Gesamtanzahl Elemente in Outline-Pool */
    ne = 0;
    t_array = ol_pool_array;
    while (*t_array) ne += ne_OldOutline(*t_array++);

    /* Gesamtanzahl Elemente nach Sequence-Header */
    sh->ne_olpool = ne;

    /* allokiere VFMOutline-Pool */
    t_pool = (struct VFMOutline *) 
             _AllocVec(ne*sizeof(struct VFMOutline),MEMF_PUBLIC);
    if (!t_pool) return(FALSE);
    sh->outline_pool = t_pool;

    /* fülle Outline-Pool und OutlineLUT aus */
    t_array = ol_pool_array;
    index = 0;
    while (*t_array) {

        /* ermittle Anzahl Elemente in aktueller Outline */
        UWORD act_ne = ne_OldOutline(*t_array);
        UWORD i;

        /* konvertiere Pixel2D-Outline nach VFMOutline--Pool */
        for (i=0; i<(act_ne-1); i++) {
            t_pool[i].x = ((FLOAT)(*t_array)[i].x) / 256.0;
            t_pool[i].y = ((FLOAT)(*t_array)[i].y) / 256.0;
        };
        t_pool[i].x = -1.0;     // Outline begrenzen
        t_pool[i].y = -1.0;

        /* fülle OutlineLUT aus */
        OutlineLUT[index++] = t_pool;

        /* update Pointer and stuff... */
        t_pool += act_ne;
        t_array++;
    };

    /* das war's */
    return(TRUE);
}

/*-----------------------------------------------------------------*/
BOOL createSequence(struct sequence_header *sh,
                    UWORD numframes,
                    struct BmpAnimFrame *init_seq)
/*
**  FUNCTION
**      Erzeugt die eigentliche Animations-Sequence für
**      den übergebenen Sequence-Header, greift dabei
**      auf Parameter zurück, die bereits von
**      <createFnObjPool()> und <createOutlinePool()>
**      ermittelt wurden.
**
**  INPUTS
**      sh  -> Sequence wird für diesen Sequence-Header erzeugt
**      numframes   -> == BANIMA_NumFrames
**      init_seq    -> == BANIMA_Sequence
**
**  RESULTS
**      TRUE    -> alles OK
**      FALSE   -> not enough mem
**                 (killBmpanimResource() MUSS AUFGERUFEN WERDEN!)
**
**      Folgende Felder wurden ausgefüllt:
**          sh->sequence
**          sh->endof_sequence
**          sh->num_frames
**
**  CHANGED
**      25-Oct-94   floh    created
**      21-Jan-95   floh    Nucleus2-Revision
**      12-Jan-96   floh    revised & updated
*/
{
    struct internal_frame *seq;
    UWORD i;

    /* trage NumFrames in Sequence-Header ein */
    sh->num_frames = numframes;

    /* allokiere Speicher für Sequence */
    seq = (struct internal_frame *) 
          _AllocVec(numframes*sizeof(struct internal_frame),MEMF_PUBLIC);
    if (!seq) return(FALSE);
    sh->sequence = seq;

    /* konvertiere BANIMA_Sequence nach Internal Sequence */
    for (i=0; i<numframes; i++) {

        /* hole indexe */
        UWORD fni = init_seq->fnpool_index;
        UWORD oli = init_seq->olpool_index;

        /* fülle Frame aus */
        seq->outline        = OutlineLUT[oli];
        seq->bitmap         = sh->object_pool[fni].bitmap;
        seq->frame_time     = init_seq->frame_time;
        seq->pict_index     = fni;
        seq->outline_index  = oli;

        /* aktualisiere Pointer */
        seq++;
        init_seq++;
    };

    /* trage EndOfSequence-Pointer ein... */
    sh->endof_sequence = seq;

    /* das war's */
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void killBmpanimResource(struct sequence_header *sh)
/*
**  FUNCTION
**      Killt einen Sequence-Header mit allem, was schon
**      initialisiert ist.
**
**  INPUTS
**      sh  -> diesen Sequence-Header killen.
**
**  RESULTS
**      ---
**
**  CHANGED
**      25-Oct-94   floh    created + debugged
**      21-Jan-95   floh    Nucleus2-Revision
**      12-Jan-96   floh    revised & updated
*/
{
    if (sh) {

        /* zuerst die einfachen Speicher-Blöcke */
        if (sh->filename_pool) _FreeVec(sh->filename_pool);
        if (sh->outline_pool)  _FreeVec(sh->outline_pool);
        if (sh->loader_class)  _FreeVec(sh->loader_class);
        if (sh->sequence)      _FreeVec(sh->sequence);

        /* jetzt den Object-Pool mit den Bitmap-Objects */
        if (sh->object_pool) {
            UWORD index;
            UWORD limit = sh->ne_objpool;
            for (index=0; index<limit; index++) {
                Object *bmo = sh->object_pool[index].bitmap_object;
                if (bmo) _dispose(bmo);
            };
            _FreeVec(sh->object_pool);
        };
        /* den Sequence-Header selbst freigeben */
        _FreeVec(sh);
    };

    /* das war's */
}

/*-----------------------------------------------------------------*/
struct sequence_header *createBmpanimResource(
    UBYTE *loaderclass,
    UBYTE **fn_pool_array,
    Pixel2D **ol_pool_array,
    UWORD num_frames,
    struct BmpAnimFrame *sequence)
/*
**  FUNCTION
**      Erzeugt die über die Parameter definierte Resource
**      (struct sequence_header) und gibt den Pointer darauf
**      zurück.
**
**  INPUTS
**      loaderclass     -> == BANIMA_LoaderClass
**      fn_pool_array   -> == BANIMA_FilenamePool
**      ol_pool_array   -> == BANIMA_OutlinePool
**      num_frames      -> == BANIMA_NumFrames
**      sequence        -> == BANIMA_Sequence
**
**  RESULTS
**      Pointer auf fertige <struct sequence_header> oder
**      NULL bei Mißerfolg.
**
**  CHANGED
**      25-Oct-94   floh    created
**      21-Jan-95   floh    Nucleus2-Revision
**      12-Jan-96   floh    revised & updated
*/
{
    /* allokiere einen Sequence-Header... */
    struct sequence_header *sh = (struct sequence_header *)
        _AllocVec(sizeof(struct sequence_header), MEMF_PUBLIC|MEMF_CLEAR);
    if (!sh) return(NULL);

    /* und fülle das Teil aus */
    if (!createFnObjPool(sh, fn_pool_array, loaderclass)) {
        killBmpanimResource(sh); return(NULL);
    };
    if (!createOutlinePool(sh, ol_pool_array)) {
        killBmpanimResource(sh); return(NULL);
    };

    if (!createSequence(sh, num_frames, sequence)) {
        killBmpanimResource(sh); return(NULL);
    };

    /* alles klar... */
    return(sh);
}

/********************************************************************
*** RESOURCE SAVE ROUTINEN                                        ***
********************************************************************/

/*-----------------------------------------------------------------*/
BOOL saveSequence(APTR file, struct sequence_header *sh)
/*
**  FUNCTION
**      Sichert folgende Daten in offenen Filehandle:
**          UWORD sh->num_frames;
**          struct BmpAnimFrame sequence[];
**
**  INPUTS
**      file    -> der mit saveBmpanimResource() geöffnete DOS-File
**      sh  -> Pointer auf vollständige <struct sequence_header>,
**             wie mit createBmpanimResource() erzeugt.
**
**  RESULTS
**      TRUE/FALSE
**
**  CHANGED
**      27-Oct-94   floh    created
**      21-Jan-95   floh    Nucleus2-Revision
**      12-Jan-96   floh    revised & updated
*/
{
    BOOL retval = FALSE;
    UWORD numframes;

    /* schreibe NumFrames */
    numframes = sh->num_frames;
    n2vw(&numframes);
    if (bmpanim_FWrite(&numframes, sizeof(numframes), 1, file) == 1) {

        /* konvertiere und sichere Sequence */
        UWORD f_num;
        struct internal_frame *act_frame = sh->sequence;
        for (f_num=0; f_num < sh->num_frames; f_num++) {

            struct BmpAnimFrame save_frame;
            save_frame.frame_time   = act_frame->frame_time;
            save_frame.fnpool_index = act_frame->pict_index;
            save_frame.olpool_index = act_frame->outline_index;
            n2vl(&save_frame.frame_time);
            n2vw(&save_frame.fnpool_index);
            n2vw(&save_frame.olpool_index);

            if (bmpanim_FWrite(&save_frame, sizeof(save_frame), 1, file) != 1) {
                return(FALSE);
            };
            /* next frame, please */
            act_frame++;
        };
        retval = TRUE;
    };
    return(retval);
}

/*-----------------------------------------------------------------*/
BOOL saveOlPool(APTR file, struct sequence_header *sh)
/*
**  FUNCTION
**      Sichert zuerst die Gesamtanzahl der Pixel2D-Elemente
**      im Outline-Pool (als UWORD), danach den gesamten
**      Outline-Pool in einem speziellen komprimierten
**      Format (siehe <bitmap/bmpanimclass.h>).
**
**  INPUTS
**      file    -> der mit saveBmpanimResource() geöffnete DOS-File
**      sh  -> Pointer auf vollständige <struct sequence_header>,
**             wie mit createBmpanimResource() erzeugt.
**
**  RESULTS
**      TRUE/FALSE
**
**  CHANGED
**      27-Oct-94   floh    created
**      21-Jan-95   floh    Nucleus2-Revision
**      22-Jan-95   floh    debugging...
**      07-Jul-95   floh    neues VFMOutline-Format (FLOATs)
**      12-Jan-96   floh    revised & updated
*/
{
    BOOL retval = FALSE;

    UWORD ne_all;
    struct VFMOutline *end_of_pool;

    /* folgender Pointer zeigt auf Ende des Pools */
    end_of_pool = sh->outline_pool + sh->ne_olpool;

    /* schreibe Anzahl Elemente nach Filehandle */
    ne_all = sh->ne_olpool;
    n2vw(&ne_all);
    if (bmpanim_FWrite(&ne_all, sizeof(ne_all), 1, file) == 1) {

        /* Pointer auf Outline-Pool */
        struct VFMOutline *act_ol = sh->outline_pool;

        /* Test auf Ende des Outline-Pools... */
        while (act_ol != end_of_pool) {

            /* schreibe Anzahl Pixel2D's aktuelle Outline... */
            UWORD ne_actol = ne_NewOutline(act_ol) -1;
            n2vw(&ne_actol);
            if (bmpanim_FWrite(&ne_actol, sizeof(ne_actol), 1, file) == 1) {

                /* schreibe aktuelle Outline */
                while (act_ol->x >= 0.0) {
                    struct ol_atom olxy;
                    olxy.x = (BYTE) (act_ol->x * 256.0);
                    olxy.y = (BYTE) (act_ol->y * 256.0);
                    if (bmpanim_FWrite(&olxy, sizeof(olxy), 1, file) != 1) return(FALSE);
                    act_ol++;
                };
            } else return(FALSE);

            /* act_ol auf Anfang der nächsten Outline */
            act_ol++;
        };
        retval = TRUE;
    };
    return(retval);
}

/*-----------------------------------------------------------------*/
BOOL saveFnPool(APTR file, struct sequence_header *sh)
/*
**  FUNCTION
**      Schreibt folgende Daten in den offenen Filehandle:
**          UWORD sh->sizeof_fnpool;    -> Größe des Filename-Pools
**          UBYTE fn_pool[]             -> der gesamte Filename-Pool
**
**  INPUTS
**      file    -> der mit saveBmpanimResource() geöffnete DOS-File
**      sh  -> Pointer auf vollständige <struct sequence_header>,
**             wie mit createBmpanimResource() erzeugt.
**
**  RESULTS
**      TRUE/FALSE
**
**  CHANGED
**      27-Oct-94   floh    created
**      21-Jan-95   floh    Nucleus2-Revision
**      12-Jan-96   floh    revised & updated
*/
{
    BOOL retval   = FALSE;
    UWORD size;

    /* schreibe Größe des Filename-Pools */
    size = sh->sizeof_fnpool;
    n2vw(&size);
    if (bmpanim_FWrite(&size, sizeof(size), 1, file) == 1) {

        /* schreibe Filename-Pool selbst */
        if (bmpanim_FWrite(sh->filename_pool, sh->sizeof_fnpool, 1, file) == 1) {
            retval = TRUE;
        };
    };
    return(retval);
}

/*-----------------------------------------------------------------*/
BOOL saveLoaderClass(APTR file, struct sequence_header *sh)
/*
**  FUNCTION
**      Sichert folgende Daten in den offenen Filehandle:
**          UWORD (strlen(sh->loader_class)+1 -> Größe des folgenden String
**          UBYTE[] sh->loader_class    -> der LoaderClass-String selbst
**
**  INPUTS
**      file    -> der mit saveBmpanimResource() geöffnete DOS-File
**      sh  -> Pointer auf vollständige <struct sequence_header>,
**             wie mit createBmpanimResource() erzeugt.
**
**  RESULTS
**      TRUE/FALSE
**
**  CHANGED
**      27-Oct-94   floh    created
**      21-Jan-95   floh    Nucleus2-Revision
**      12-Jan-96   floh    revised & updated
*/
{
    BOOL retval = FALSE;

    /* ermittle sizeof(LoaderClass) */
    UWORD size = strlen(sh->loader_class)+1;

    /* schreibe Size-Wert (2 Bytes - Motorola-Endian!!!) */
    n2vw(&size);
    if (bmpanim_FWrite(&size, sizeof(size), 1, file) == 1) {

        /* size wieder ins native Endian-Format */
        v2nw(&size);

        /* schreibe LoaderClass-String */
        if (bmpanim_FWrite(sh->loader_class, size, 1, file) == 1) {
            retval = TRUE;
        };
    };
    return(retval);
}

/*-----------------------------------------------------------------*/
BOOL saveBmpanimResource(struct sequence_header *sh,
                         UBYTE *rsrc_id, struct IFFHandle *iff)
/*
**  FUNCTION
**      Die Anim-Sequenz wird als IFF-File entweder in
**      einen Standalone-File gesichert (<rsrc_id> ist Filename
**      rel. zu mc2resources:rsrcpool/) oder als Teil
**      eines offenen IFF-Streams (in diesem Fall muß iff != NULL
**      sein).
**
**  INPUTS
**      sh  -> Pointer auf vollständige <struct sequence_header>,
**             wie mit createBmpanimResource() erzeugt.
**      rsrc_id -> String mit eindeutiger Resource-ID (== BANIMA_AnimID).
**      iff     -> NULL, falls Standalone-File, sonst gültiger 
**                 IFFHandle-Ptr
**
**  RESULTS
**      TRUE    -> alles OK
**      FALSE   -> Ein Fehler trat auf. Wahrscheinlich ließ sich
**                 der File nicht öffnen (kein rsrcpool-Directory?)
**
**  CHANGED
**      27-Oct-94   floh    created
**      21-Jan-95   floh    Nucleus2-Revision
**      12-Jan-96   floh    revised & updated
*/
{
    APTR file;
    BOOL retval = FALSE;

    if (iff) {

        /*** Signal für bmpanim_FOpen(), das IFFHandle existiert ***/
        IsIFF = TRUE;
        file  = bmpanim_FOpen((UBYTE *)iff, "wb");

    } else {

        /*** Standalone-File ***/
        UBYTE *path_prefix;
        UBYTE full_name[256];

        path_prefix = _GetSysPath(SYSPATH_RESOURCES);
        strcpy(full_name, path_prefix);
        strcat(full_name, DISK_RSRCPOOL);
        strcat(full_name, rsrc_id);

        IsIFF = FALSE;

        file = bmpanim_FOpen(full_name, "wb");
    };

    if (file) {
        if (saveLoaderClass(file,sh)) {
            if (saveFnPool(file,sh)) {
                if (saveOlPool(file,sh)) {
                    if (saveSequence(file,sh)) {
                        retval = TRUE;
                    };
                };
            };
        };
        bmpanim_FClose(file);
    };
    return(retval);
}

/********************************************************************
*** RESOURCE LOADER ROUTINEN                                      ***
********************************************************************/

/*-----------------------------------------------------------------*/
BOOL loadSequence(APTR file, struct sequence_header *sh)
/*
**  FUNCTION
**      Lädt Sequence-Definition aus Resource-File.
**
**  INPUTS
**      file    -> der mit loadBmpanimResource() geöffnete
**                 Resource-File
**      sh      -> der von loadBmpanimResource() allokierte
**                 Sequence-Header.
**
**  RESULTS
**      sh->sequence
**      sh->endof_sequence
**      sh->num_frames          ...sind alle gültig
**
**      TRUE    -> alles ok
**      FALSE   -> Mißerfolg. killBmpanimResource() MUSS AUFGERUFEN WERDEN!!!
**
**  CHANGED
**      27-Oct-94   floh    created
**      21-Jan-95   floh    Nucleus2-Revision
**      12-Jan-96   floh    revised & updated
**
**  SEE ALSO
**      createSequence(), saveSequence()
*/
{
    BOOL retval = FALSE;

    UWORD numframes;

    /* lese NumFrames */
    if (bmpanim_FRead(&numframes, sizeof(numframes), 1, file) == 1) {

        UWORD f_num;
        struct internal_frame *seq;

        /* gerade gelesene NumFrames endian-konvertieren */
        v2nw(&numframes);

        /* allokiere Speicher für Sequence */
        seq = (struct internal_frame *)
              _AllocVec(numframes*sizeof(struct internal_frame),MEMF_PUBLIC);
        if (!seq) return(FALSE);

        /* fülle Sequence-Header aus */
        sh->num_frames     = numframes;
        sh->sequence       = seq;
        sh->endof_sequence = seq + numframes;

        /* lade und konvertiere Sequence-Definition */
        for (f_num=0; f_num < numframes; f_num++) {

            struct BmpAnimFrame load_frame;
            UWORD fni, oli;

            /* lese aktuelle Sequence-Definition ein */
            if (bmpanim_FRead(&load_frame, sizeof(load_frame), 1, file) != 1)
                return(FALSE);

            /* Endian-Konvertierung */
            v2nl(&load_frame.frame_time);
            v2nw(&load_frame.fnpool_index);
            v2nw(&load_frame.olpool_index);

            /* hole Indexe von Outline und Filename */
            fni = load_frame.fnpool_index;
            oli = load_frame.olpool_index;

            /* konvertiere nach Sequence */
            seq->outline       = OutlineLUT[oli];
            seq->bitmap        = sh->object_pool[fni].bitmap;
            seq->frame_time    = load_frame.frame_time;
            seq->pict_index    = fni;
            seq->outline_index = oli;

            /* aktualisiere Pointer */
            seq++;
        };

        /* Outline ist drin, Success! */
        retval = TRUE;
    }; /* if (_FRead(numframes) */

    return(retval);
}

/*-----------------------------------------------------------------*/
BOOL loadOlPool(APTR file, struct sequence_header *sh)
/*
**  FUNCTION
**      Lädt kompletten Outline-Pool aus Resource-File.
**
**  INPUTS
**      file    -> der mit loadBmpanimResource() geöffnete
**                 Resource-File
**      sh      -> der von loadBmpanimResource() allokierte
**                 Sequence-Header.
**
**  RESULTS
**      OutlineLUT[]
**      sh->outline_pool
**      sh->ne_olpool       ...sind gültig
**
**      TRUE    -> alles ok
**      FALSE   -> Mißerfolg. killBmpanimResource() MUSS AUFGERUFEN WERDEN!!!
**
**  CHANGED
**      27-Oct-94   floh    created
**      21-Jan-95   floh    Nucleus2-Revision
**      07-Jul-95   floh    neues VFMOutline-Format (FLOATs)
**      12-Jan-96   floh    revised & updated
**
**  SEE ALSO
**      createOutlinePool(), saveOlPool()
*/
{
    BOOL retval = FALSE;

    UWORD ne_all;

    /* lese Gesamt-Anzahl Pixel2D's */
    if (bmpanim_FRead(&ne_all, sizeof(ne_all), 1, file) == 1) {

        struct VFMOutline *act_ol;

        /* Endian-Konvertierung <ne_all> */
        v2nw(&ne_all);

        /* schreibe Anzahl P2D's nach SeqHeader */
        sh->ne_olpool = ne_all;

        /* allokiere Outline-Pool */
        act_ol = (struct VFMOutline *)
                 _AllocVec(ne_all*sizeof(struct VFMOutline),MEMF_PUBLIC);
        if (act_ol) {

            struct VFMOutline *end_of_pool;
            UWORD act_olnum;

            /* schreibe Outline-Pool-Ptr nach SeqHeader */
            sh->outline_pool = act_ol;

            /* Pointer für Endetest */
            end_of_pool = act_ol + ne_all;

            /* aktuelle Outline-Nummer für OutlineLUT-Index */
            act_olnum = 0;

            /* solange Pool noch nicht voll... */
            while (act_ol != end_of_pool) {

                UWORD ne_act, i;

                /* Outline-Pointer nach OutlineLUT */
                OutlineLUT[act_olnum++] = act_ol;

                /* lese Anzahl Pixel2D's in aktueller Outline */
                if (bmpanim_FRead(&ne_act, sizeof(ne_act), 1, file) != 1) return(FALSE);
                v2nw(&ne_act);

                /* lese Outline selbst ein */
                for (i=0; i<ne_act; i++) {

                    /* lese ein Koordinaten-Paar */
                    struct ol_atom olxy;
                    if (bmpanim_FRead(&olxy, sizeof(olxy), 1, file) != 1) return(FALSE);
                    /* keine Endian-Konvertierung, weil 2x UBYTEs */
                    act_ol->x = ((FLOAT) olxy.x / 256.0);
                    act_ol->y = ((FLOAT) olxy.y / 256.0);
                    act_ol++;
                };
                /* begrenze aktuelle Outline */
                act_ol->x = -1.0;
                act_ol->y = -1.0;
                act_ol++;
            };

            /* Outline ist drin, also Erfolg! */
            retval = TRUE;
        }; /* if (act_ol) */
    }; /* if (FRead(ne_all)) */

    return(retval);
}

/*-----------------------------------------------------------------*/
BOOL loadFnObjPool(APTR file, struct sequence_header *sh)
/*
**  FUNCTION
**      Lädt Filename-Pool aus Resource-File, erzeugt
**      außerdem kompletten Object-Pool (für jeden Filename
**      im Pool das zugehörige Bitmap-Object (instance of
**      sh->loader_class).
**
**  INPUTS
**      file    -> der mit loadBmpanimResource() geöffnete
**                 Resource-File
**      sh      -> der von loadBmpanimResource() allokierte
**                 Sequence-Header.
**
**  RESULTS
**      sh->filename_pool
**      sh->object_pool
**      sh->sizeof_fnpool
**      sh->ne_objpool      ...sind alle gültig
**
**      TRUE    -> alles ok
**      FALSE   -> Mißerfolg. killBmpanimResource() MUSS AUFGERUFEN WERDEN!!!
**
**  CHANGED
**      27-Oct-94   floh    created
**      21-Jan-95   floh    Nucleus2-Revision
**      12-Jan-96   floh    revised & updated
**
**  SEE ALSO
**      saveFnPool(), createFnObjPool()
*/
{
    BOOL retval = FALSE;
    UWORD size;

    /* lese Größe des Filename-Pools in Bytes */
    if (bmpanim_FRead(&size, sizeof(size), 1, file) == 1) {

        UBYTE *fn_pool;

        /* Endian-Konvertierung <size> */
        v2nw(&size);

        sh->sizeof_fnpool = size;

        /* allokiere Filename-Pool */
        fn_pool = _AllocVec(size, MEMF_PUBLIC);
        if (fn_pool) {

            /* Pointer nach SequenceHeader + FnPool einlesen */
            sh->filename_pool = fn_pool;
            if (bmpanim_FRead(fn_pool, size, 1, file) == 1) {

                ULONG num_obj;
                struct object_info *o_pool;

                /* ermittle Anzahl Filenamen in Pool und */
                /* zerpflücke Pool nach Lookup-Table     */
                num_obj=strbpl((char **)FilenameLUT,MAXNUM_ELEMENTS,fn_pool);
                if (num_obj == MAXNUM_ELEMENTS) return(FALSE);

                sh->ne_objpool = num_obj;

                /* allokiere Object-Pool */
                o_pool = (struct object_info *) 
                         _AllocVec(num_obj*sizeof(struct object_info),
                                  MEMF_PUBLIC|MEMF_CLEAR);

                if (o_pool) {

                    UWORD i;

                    /* Pointer nach Sequence-Header */
                    sh->object_pool = o_pool;

                    /* Object-Pool erzeugen */
                    for (i=0; i<num_obj; i++) {

                        /* Picture-Filename aus Lookup-Table */
                        UBYTE *pict_name = (UBYTE *) FilenameLUT[i];

                        /* erzeuge Bitmap-Object */
                        o_pool->bitmap_object = (Object *)
                            _new(sh->loader_class, RSA_Name, pict_name,
                                                   RSA_DontCopy, TRUE,
                                                   TAG_DONE);
                        if (!(o_pool->bitmap_object)) return(FALSE);

                        /* hole VFMBitmap des neuen Objects */
                        _get(o_pool->bitmap_object, BMA_Bitmap, &(o_pool->bitmap));

                        /* trage Pointer auf Pict-Filename (FnPool) ein */
                        o_pool->pict_filename = pict_name;

                        /* und das nächste Element im Pool */
                        o_pool++;
                    };

                    /* SUCCESS!!! */
                    retval = TRUE;
                }; /* if (o_pool) */
            }; /* if (FRead(filename_pool)) */
        }; /* if (fn_pool) */
    }; /* if (FRead(size)) */

    return(retval);
}

/*-----------------------------------------------------------------*/
BOOL loadLoaderClass(APTR file, struct sequence_header *sh)
/*
**  FUNCTION
**      Lädt den LoaderClass-String aus dem Resource-File.
**
**  INPUTS
**      file    -> der mit loadBmpanimResource() geöffnete
**                 Resource-File
**      sh      -> der von loadBmpanimResource() allokierte
**                 Sequence-Header.
**
**  RESULTS
**      sh->loader_class    ist gültig
**
**      TRUE    -> alles ok
**      FALSE   -> Mißerfolg. killBmpanimResource() MUSS AUFGERUFEN WERDEN!!!
**
**  CHANGED
**      27-Oct-94   floh    created
**      21-Jan-95   floh    Nucleus2-Revision
**      12-Jan-96   floh    revised & updated
**
**  SEE ALSO
**      saveLoaderClass(), createFnObjPool()
*/
{
    BOOL retval = FALSE;
    UWORD size;

    /* lese "strlen(LoaderClass)+1" */
    if (bmpanim_FRead(&size, sizeof(size), 1, file) == 1) {

        /* Endian-Konv. <size> */
        v2nw(&size);

        /* allokiere Puffer für LoaderClass-String */
        sh->loader_class = _AllocVec(size, MEMF_PUBLIC);
        if (sh->loader_class) {

            /* lese LoaderClass-String */
            if (bmpanim_FRead(sh->loader_class, size, 1, file) == 1) {
                retval = TRUE;
            };
        };
    };
    return(retval);
}

/*-----------------------------------------------------------------*/
struct sequence_header *loadBmpanimResource(UBYTE *rsrc_id, struct IFFHandle *iff)
/*
**  FUNCTION
**      Erzeugt und lädt einen kompletten Sequence-Header
**      entweder aus einem Standalone-File (altes rohes, oder neues
**      IFF-encapsulated Format), oder aus einem offenen
**      IFF-Stream (iff muß in diesem Fall auf einen gültigen
**      IFF-Stream zeigen).
**
**  INPUTS
**      rsrc_id -> Pointer auf Resource-ID (C-String), entspricht
**                 BANIMA_AnimID
**      iff     -> NULL, wenn Standalone-File, sonst Pointer auf
**                 offenes IFFHandle
**
**  RESULTS
**      Pointer auf komplett initialisierten Sequence-Header
**      oder NULL bei Mißerfolg.
**
**  CHANGED
**      27-Oct-94   floh    created
**      28-Oct-94   floh    debugging...
**      21-Jan-95   floh    Nucleus2-Revision
**      12-Jan-96   floh    revised & updated
*/
{
    APTR file;
    BOOL success = FALSE;
    struct sequence_header *sh = 0;

    if (iff) {

        /*** Signal an bmpanim_FOpen(), daß IFFHandle existiert ***/
        IsIFF = TRUE;
        file  = bmpanim_FOpen((UBYTE *)iff, "rb");

    } else {

        /*** Standalone-File ***/
        UBYTE *path_prefix;
        UBYTE full_name[256];

        path_prefix = _GetSysPath(SYSPATH_RESOURCES);
        strcpy(full_name, path_prefix);
        strcat(full_name, DISK_RSRCPOOL);
        strcat(full_name, rsrc_id);

        IsIFF = FALSE;

        file = bmpanim_FOpen(full_name, "rb");
    };

    if (file) {

        /* allokiere Sequence-Header */
        sh = (struct sequence_header *)
             _AllocVec(sizeof(struct sequence_header),
                       MEMF_PUBLIC|MEMF_CLEAR);
        if (sh) {

            /* lade die einzelnen Resource-Komponenten */
            if (loadLoaderClass(file,sh)) {
                if (loadFnObjPool(file,sh)) {
                    if (loadOlPool(file,sh)) {
                        if (loadSequence(file,sh)) {
                            success = TRUE;
                        };
                    };
                };
            };
            /* bei Mißerfolg... aufräumen */
            if (!success) killBmpanimResource(sh);
        };
        /* schließe Filehandle */
        bmpanim_FClose(file);
    };

    if (success && sh) return(sh);
    else               return(NULL);
}

/*=================================================================*/
