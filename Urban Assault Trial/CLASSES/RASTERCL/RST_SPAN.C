/*
**  $Source: PRG:VFM/Classes/_RasterClass/rst_span.c,v $
**  $Revision: 38.4 $
**  $Date: 1998/01/06 14:57:44 $
**  $Locker:  $
**  $Author: floh $
**
**  Span-Clipper für raster.class.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>

#include <string.h>

#include "nucleus/nucleus2.h"
#include "bitmap/rasterclass.h"

_extern_use_nucleus

#define SF_RIGHT    (1<<0)
#define SF_LEFT     (1<<1)
#define SF_HIDDEN   (1<<2)
#define SF_CLIPPED  (1<<3)      // private
#define SF_MERGE    (1<<4)      // private

void rst_seAddSpan(UBYTE *, struct rast_scanline *, struct raster_data *);

/*-------------------------------------------------------------------
**  Die folgenden Routinen sind lokal, damit sie vom Compiler
**  inlined werden können.
*/
void rst_addTail(struct List *list, struct Node *node)
{
    register struct Node *old_last;

    old_last          = list->lh_TailPred;
    list->lh_TailPred = node;

    node->ln_Succ = (struct Node *) &(list->lh_Tail);
    node->ln_Pred = old_last;

    old_last->ln_Succ = node;
}

struct Node *rst_remHead(struct List *list)
{
    register struct Node *first  = list->lh_Head;
    register struct Node *second = first->ln_Succ;
    if (!second) return(NULL);

    list->lh_Head   = second;
    second->ln_Pred = (struct Node *) list;
    return(first);
}

void rst_remove(struct Node *node)
{
    register struct Node *succ = node->ln_Succ;
    register struct Node *pred = node->ln_Pred;
    pred->ln_Succ = succ;
    succ->ln_Pred = pred;
}

/*-----------------------------------------------------------------*/
BOOL rst_seInitSpangine(struct raster_data *rd, ULONG num_spans, ULONG num_lines)
/*
**  FUNCTION
**      Initialisiert die Span Engine.
**
**  INPUTS
**      rd          - Ptr auf LID des raster.class Objects
**      num_spans   - gewünschte Anzahl Span-Elemente im Pool
**      num_lines   - Anzahl Pixel-Zeilen im Display
**
**  RESULTS
**      TRUE    -> alles OK
**      FALSE   -> out of mem
**
**  CHANGED
**      10-Nov-95   floh    created
**      12-Nov-95   floh    + se_SpansUsed
**                          + VERDAMMT! So einfach wie ich mir das
**                            mit den transparenten Spans gedacht
**                            habe, ist es doch nicht!!! Ich muß jetzt
**                            doch einen Stack für transparente
**                            Spans aufheben, die erst gezeichnet werden,
**                            wenn das Bild fertig ist... FUCK!
**      13-Nov-95   floh    + Profiling
**      05-Jun-96   floh    + übernommen in raster.class
*/
{
    BOOL retval = FALSE;

    /*** Platz für MinSpans allokieren ***/
    rd->span_pool = (struct rast_minspan *)
        _AllocVec(num_spans * sizeof(struct rast_minspan),MEMF_PUBLIC|MEMF_CLEAR);
    if (rd->span_pool) {

        /*** Zeilen-Listen-Header allokieren ***/
        rd->header_pool = (struct MinList *)
            _AllocVec(num_lines * sizeof(struct MinList), MEMF_PUBLIC);
        if (rd->header_pool) {

            ULONG i;

            /*** HeaderPool initialisieren ***/
            for (i=0; i<num_lines; i++) {
                _NewList((struct List *) &(rd->header_pool[i]));
            };

            /*** FreeList initialisieren ***/
            _NewList((struct List *) &(rd->free_list));

            /*** alle MinSpans in die FreeList einklinken ***/
            for (i=0; i<num_spans; i++) {
                rst_addTail((struct List *) &(rd->free_list),
                            (struct Node *) &(rd->span_pool[i]));
            };

            /*** GlassStack initialisieren (num_lines*5) ***/
            rd->glass_stack = (struct rast_scanline *)
                _AllocVec((num_lines*5) * sizeof(struct rast_scanline),MEMF_PUBLIC);
            if (rd->glass_stack) {
                rd->glass_stack_ptr    = rd->glass_stack;
                rd->end_of_glass_stack = rd->glass_stack + num_lines*5;
                retval = TRUE;
            };
        };
    };
    return(retval);
}

/*-----------------------------------------------------------------*/
void rst_seKillSpangine(struct raster_data *rd)
/*
**  CHANGED
**      10-Nov-95   floh    created
**      12-Nov-95   floh    + se_GlassStack
**      04-Jun-96   floh    + übernommen nach raster.class
*/
{
    if (rd->header_pool) _FreeVec(rd->header_pool);
    if (rd->span_pool)   _FreeVec(rd->span_pool);
    if (rd->glass_stack) _FreeVec(rd->glass_stack);
}

/*-----------------------------------------------------------------*/
void rst_seNewFrame(struct raster_data *rd, ULONG num_lines)
/*
**  FUNCTION
**      Initialisiert Spangine für einen neuen Frame.
**      Namely: Alle MinSpans, die noch in den Listen des
**      HeaderPools sind, werden von dort removed und
**      in die FreeList zurückgelinkt.
**
**  INPUTS
**      num_lines   - Anzahl Zeilen im Display
**
**  CHANGED
**      10-Nov-95   floh    created
**      12-Nov-95   floh    + se_SpansUsed
**      05-Jun-96   floh    + übernommen nach raster.class
*/
{
    ULONG i;

    for (i=0; i<num_lines; i++) {
        struct Node *nd;
        while (nd = rst_remHead((struct List *) &(rd->header_pool[i]))) {
            rst_addTail((struct List *) &rd->free_list, nd);
        };
    };
    rd->glass_stack_ptr = rd->glass_stack;
}

/*-----------------------------------------------------------------*/
void rst_sePushOnGlassStack(struct raster_data *rd, struct rast_scanline *span)
/*
**  FUNCTION
**      Pusht die übergebene SpanElm Struktur in den
**      GlassStack.
**
**  INPUTS
**      span -> Pointer auf Span-Element
**
**  CHANGED
**      12-Nov-95   floh    created
**      13-Nov-95   floh    + online Debugging
**      05-Jun-96   floh    + übernommen nach raster.class
*/
{
    if (rd->glass_stack_ptr < rd->end_of_glass_stack) {
        memcpy(rd->glass_stack_ptr++, span, sizeof(struct rast_scanline));
    };
}

/*-----------------------------------------------------------------*/
ULONG rst_seClipSpan(UBYTE *addr,
                     struct raster_data *rd,
                     struct rast_scanline *clipme, 
                     struct rast_minspan *withme)
/*
**  FUNCTION
**      Clippt das Span-Element <clipme> gegen den
**      MinSpan <withme>. Zurückgegeben werden
**      Flags, die die Relation zwischen <clipme> und
**      <withme> beschreiben.
**
**      Bei Bedarf wird <clipme> gesplittet und rekursiv
**      weitergeclippt.
**
**  INPUTS
**      addr    - Pointer auf Zeilenanfang im Display!
**      rd      - Pointer auf LID des raster.class Objects
**      clipme  - voll ausgefülltes Span-Element
**      withme  - existierender MinSpan, gegen den geclippt wird
**
**  RESULTS
**      Folgende Flags werden kombiniert zurückgeliefert:
**
**          SF_RIGHT   - clipme liegt voll rechts von withme
**          SF_LEFT    - clipme liegt voll links von withme
**          SF_HIDDEN  - clipme ist vollständig von withme verdeckt
**          SF_CLIPPED - clipme wurde geclippt (SF_RIGHT|SF_LEFT gibt
**                       die Seite an, die geclippt wurde)
**
**  CHANGED
**      11-Nov-95   floh    created
**      12-Nov-95   floh    + neues Flag: SF_MERGE
**                          + Y-Koordinate jetzt in <struct SpanElm>
**                          + debugging (prompt vergessen, beim Splitten
**                            die Y-Koordinate mitzunehmen!
**      05-Jun-96   floh    + übernommen nach raster.class
*/
{
    WORD cstart = clipme->x0;
    WORD cend   = cstart + clipme->dx;

    WORD wstart = withme->x0;
    WORD wend   = wstart + withme->dx;

    if (cstart == wend)     return(SF_MERGE|SF_RIGHT);
    else if (cstart > wend) return(SF_RIGHT);

    if (wstart == cend) return(SF_MERGE|SF_LEFT);
    else if (wstart > cend) return(SF_LEFT);

    if (cstart >= wstart) {

        if (cend <= wend) return(SF_HIDDEN);    // voll von <withme> verdeckt
        else {

            /*** Left clippen ***/
            LONG pixdist = (LONG) (wend - cstart);

            clipme->x0 += (WORD) pixdist;
            clipme->dx -= (WORD) pixdist;
            clipme->b0 += (LONG) (pixdist * clipme->db);
            clipme->z0 += (LONG) (pixdist * clipme->dz);
            clipme->u0 += (LONG) (pixdist * clipme->du);
            clipme->v0 += (LONG) (pixdist * clipme->dv);

            return(SF_MERGE|SF_RIGHT|SF_CLIPPED);
        };

    } else {

        WORD dx_store = clipme->dx;
        clipme->dx    = wstart - cstart;

        if (cend > wend) {

            /*** SPLITTEN! (also einen neuen Span erzeugen) ***/
            struct rast_scanline span;
            LONG pixdist;

            pixdist = (LONG) (wend - cstart);

            span.draw_span = clipme->draw_span;
            span.flags     = clipme->flags;
            span.y         = clipme->y;
            span.x0        = clipme->x0 + (WORD)pixdist;
            span.dx        = dx_store   - (WORD)pixdist;
            span.b0        = clipme->b0 + (LONG)(pixdist*clipme->db);
            span.z0        = clipme->z0 + (LONG)(pixdist*clipme->dz);
            span.u0        = clipme->u0 + (LONG)(pixdist*clipme->du);
            span.v0        = clipme->v0 + (LONG)(pixdist*clipme->dv);
            span.db        = clipme->db;
            span.dz        = clipme->dz;
            span.du        = clipme->du;
            span.dv        = clipme->dv;
            span.map       = clipme->map;

            /*** neuen Span rekursiv abhandeln ***/
            rst_seAddSpan(addr,&span,rd);
        };
        return(SF_MERGE|SF_LEFT|SF_CLIPPED);
    };

    /*** can't happen ***/
}

/*-----------------------------------------------------------------*/
void rst_seInsertBefore(struct raster_data *rd,
                        struct rast_scanline *span,
                        struct rast_minspan  *succ,
                        ULONG succ_clip, ULONG pred_clip)
/*
**  FUNCTION
**      Hängt einen neuen Span in die interne Span-Clip-Liste.
**      Der neue Span wird *vor* <succ> eingeklinkt, falls
**      möglich werden Spans miteinander gemerget.
**
**  INPUTS
**      span      - Pointer auf geclipptes Span-Element
**      succ      - Pointer auf MinSpan, vor dem <span> eingeordnet
**                  wird
**      succ_clip - ClipCode (von se_ClipSpan()) von <span> gegen <succ>
**      pred_clip - ClipCode von <span> gegen den Vorgänger von <succ>
**                  (NULL, wenn act 1.Element in Liste).
**
**  RESULTS
**      ---
**
**  CHANGED
**      11-Nov-95   floh    created
**      12-Nov-95   floh    + testet jetzt auf SF_MERGE statt SF_CLIPPED
**                          + Beim Mergen mit dem Vorgänger hatte ich
**                            den Vorgänger in den Nachfolger gemerged,
**                            und den Vorgänger removed. Das war genau
**                            die falsche Reihenfolge, weil die Split-
**                            Rekursion sich ja von rechts nach links
**                            bewegt :-/ (???)
**                          + se_SpansUsed
**      13-Nov-95   floh    + Profiling
**                          - se_SpansUsed
**      05-Jun-96   floh    + übernommen nach raster.class
**
**  NOTE
**      Beim Mergen bleibt die Node <act> garantiert erhalten
**      (das ist wichtig für das Zusammenspiel von se_ClipSpan()
**      und se_AddSpan()).
*/
{
    if (succ_clip & SF_MERGE) {

        /*** <span> mit <succ> mergen ***/
        succ->x0  = span->x0;
        succ->dx += span->dx;

        if (pred_clip & SF_MERGE) {
            /*** <succ> in <pred> reinmergen ***/
            struct rast_minspan *pred;
            pred = (struct rast_minspan *) succ->nd.mln_Pred;
            pred->dx += succ->dx;

            rst_remove((struct Node *)succ);
            rst_addTail((struct List *)&(rd->free_list),(struct Node *)succ);
        };

    } else if (pred_clip & SF_MERGE) {

        /*** <span> mit Vorgänger von <succ> mergen ***/
        struct rast_minspan *pred;
        pred = (struct rast_minspan *) succ->nd.mln_Pred;
        pred->dx += span->dx;

    } else {

        /*** neuen MinSpan erzeugen und einklinken ***/
        struct rast_minspan *newspan;
        newspan = (struct rast_minspan *) 
                  rst_remHead((struct List *) &(rd->free_list));
        if (newspan) {

            newspan->x0 = span->x0;
            newspan->dx = span->dx;

            /*** vor <succ> einklinken ***/
            newspan->nd.mln_Succ = (struct MinNode *) succ;
            newspan->nd.mln_Pred = succ->nd.mln_Pred;
            succ->nd.mln_Pred->mln_Succ = (struct MinNode *) newspan;
            succ->nd.mln_Pred           = (struct MinNode *) newspan;
        };
    };
}

/*-----------------------------------------------------------------*/
void rst_seInsertAfter(struct raster_data *rd,
                       struct rast_scanline *span,
                       struct rast_minspan *pred,
                       ULONG pred_clip)
/*
**  FUNCTION
**      Hängt einen neuen Span in die interne Span-Clip-Liste.
**      Der neue Span wird *nach* <pred> eingeklinkt, falls
**      möglich werden Spans miteinander gemerget.
**
**  INPUTS
**      span      - Pointer auf geclipptes Span-Element
**      pred      - Pointer auf MinSpan, hinter dem <span> eingeordnet
**                  wird, wenn Liste leer ist, zeigt pred auf den
**                  Listen-Header.
**      pred_clip - ClipCode von <span> gegen <pred>
**                  (NULL, wenn keine Elemente in Liste)
**
**  CHANGED
**      11-Nov-95   floh    created
**      12-Nov-95   floh    testet jetzt auf SF_MERGE statt SF_CLIPPED
**      12-Nov-95   floh    + se_SpansUsed
**      13-Nov-95   floh    + Profiling
**                          - se_SpansUsed
**      05-Jun-96   floh    + übernommen nach raster.class
**
**  NOTE
**      Die Routine arbeitet nur korrekt, wenn <pred> das letzte
**      Element in der Liste ist, ansonsten wird nicht korrekt
**      gemergt. Das ist aber OK so, solange se_AddSpan() das
**      beachtet.
*/
{
    if (pred_clip & SF_MERGE) {

        /*** mit Vorgänger mergen ***/
        pred->dx += span->dx;

    } else {

        /*** sonst neuen MinSpan holen und einklinken ***/
        struct rast_minspan *newspan;
        newspan = (struct rast_minspan *) 
                  rst_remHead((struct List *) &(rd->free_list));
        if (newspan) {

            newspan->x0 = span->x0;
            newspan->dx = span->dx;

            newspan->nd.mln_Succ        = pred->nd.mln_Succ;
            newspan->nd.mln_Pred        = (struct MinNode *) pred;
            pred->nd.mln_Succ->mln_Pred = (struct MinNode *) newspan;
            pred->nd.mln_Succ           = (struct MinNode *) newspan;
        };
    };
}

/*-----------------------------------------------------------------*/
void rst_seRender(UBYTE *line_addr,
                  struct rast_scanline *rs,
                  struct raster_data *rd)
/*
**  FUNCTION
**      Ruft den Spandrawer des übergebenen Spans auf.
**
**  INPUTS
**      line_addr - Startadresse der Y-Zeile(!!!!) im Display
**      rs        - der Span itself (geclippt!)
**      rd        - LID des raster.class Objects
**
**  CHANGED
**      11-Nov-95   floh    created
**      12-Nov-95   floh    y-Koord jetzt in <struct SpanElm>
**      13-Nov-95   floh    + Profiling
**      31-Mar-96   floh    + LNF Spans (Linear, Remap-Tracy)
**      05-Jun-96   floh    + übernommen nach raster.class und
**                            entsprechend angepaßt
**
*/
{
    rs->draw_span(line_addr,rs,rd);
}

/*-----------------------------------------------------------------*/
void rst_seAddSpan(UBYTE *addr,
                   struct rast_scanline *span,
                   struct raster_data *rd)
/*
**  FUNCTION
**      Übergibt der Spangine einen neuen Span zur
**      Bearbeitung. Der Span wird gegen bereits
**      existierende Spans geclippt, sollte er
**      dann noch sichtbar sein, wird er sofort
**      gezeichnet.
**
**      Wenn der Span 'solid' ist (wird anhand des
**      <flags> Element ermittelt, werden spätere
**      Spans gegen diesen geclippt, ist er
**      transparent, kommt er nicht in die interne
**      Clip-Liste.
**
**  INPUTS
**      addr    - Pointer auf Zeilenanfang im Display
**      span    - der Span itself
**      rd      - LID des raster.class Objects
**
**
**  RESULTS
**      TRUE    -> Span war zumindest teilweise sichtbar
**                 und wurde gezeichnet
**      FALSE   -> Span war unsichtbar
**
**  CHANGED
**      11-Nov-95   floh    created
**      12-Nov-95   floh    Y-Koordinate jetzt Teil von <struct SpanElm>
**      13-Nov-95   floh    + Profiling
**      05-Jun-96   floh    + übernommen nach raster.class
*/
{
    struct MinNode *nd = (struct MinNode *) &(rd->header_pool[span->y]);
    ULONG clip;
    ULONG prev_clip = 0;

    while (nd->mln_Succ->mln_Succ) {

        nd   = nd->mln_Succ;
        clip = rst_seClipSpan(addr,rd,span,(struct rast_minspan *)nd);
        if (clip & SF_HIDDEN) return;
        else if (clip & SF_LEFT) {
            if (span->flags & (RPF_ZeroTracy|RPF_LUMTracy)) {
                rst_sePushOnGlassStack(rd,span);
            } else {
                rst_seInsertBefore(rd,span,(struct rast_minspan *)nd,clip,prev_clip);
                rst_seRender(addr,span,rd);
            };
            return;
        };
        prev_clip = clip;
    };

    /*** Span ist letztes Element in Clip-Liste ***/
    if (span->flags & (RPF_ZeroTracy|RPF_LUMTracy)) {
        rst_sePushOnGlassStack(rd,span);
    } else {
        rst_seInsertAfter(rd,span,(struct rast_minspan *)nd,prev_clip);
        rst_seRender(addr,span,rd);
    };
}

/*-----------------------------------------------------------------*/
void rst_seFlushSpangine(struct raster_data *rd)
/*
**  FUNCTION
**      Macht den GlassStack leer, es werden also alle darin
**      befindlichen Spans per rst_seRender() gezeichnet.
**
**  CHANGED
**      12-Nov-95   floh    created
**      13-Nov-95   floh    + Profiling
**      05-Jun-96   floh    + übernommen nach raster.class
**      12-Feb-97   floh    + Pitch Handling
*/
{
    if (rd->glass_stack_ptr > rd->glass_stack) {
        while (--rd->glass_stack_ptr >= rd->glass_stack) {
            struct rast_scanline *rs = rd->glass_stack_ptr;
            UBYTE *addr = ((UBYTE *)rd->r->Data) + (rd->r->BytesPerRow*rs->y);
            rst_seRender(addr,rs,rd);
        };
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(void, rst_RASTM_Begin3D, void *nil)
/*
**  CHANGED
**      26-Mar-97   floh    created
*/
{ }

/*-----------------------------------------------------------------*/
_dispatcher(void, rst_RASTM_End3D, void *nil)
/*
**  CHANGED
**      07-Jun-96   floh    created
*/
{
    struct raster_data *rd = INST_DATA(cl,o);
    rst_seFlushSpangine(rd);
    rst_seNewFrame(rd, rd->r->Height);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, rst_RASTM_Begin2D, void *nil)
/*
**  CHANGED
**      26-Mar-97   floh    created
*/
{ }

/*-----------------------------------------------------------------*/
_dispatcher(void, rst_RASTM_End2D, void *nil)
/*
**  CHANGED
**      26-Mar-97   floh    created
*/
{ }

