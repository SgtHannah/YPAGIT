/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_render.c,v $
**  $Revision: 38.27 $
**  $Date: 1998/01/06 16:26:35 $
**  $Locker: floh $
**  $Author: floh $
**
**  Render-Modul für ypaworld.class. Enthält Low-Level-Routinen
**  für Frame-Erzeugung.
**
**  (C) Copyright 1995 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <math.h>

#include <stdlib.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "engine/engine.h"
#include "ypa/ypabactclass.h"
#include "ypa/ypaworldclass.h"

/*** nur für Debugging ***/
#include "input/ie.h"
#include "polygon/polygonflags.h"

#include "yw_protos.h"

#ifdef __WINDOWS__
extern ULONG wdd_DoDirect3D;
#endif

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_ov_engine
_extern_use_tform_engine
_extern_use_input_engine

/*---------------------------------------------------------------------*/
/*** HACK: Pointer auf Speicherbereiche der base.class, siehe OM_NEW ***/

#ifdef AMIGA
extern __far struct pubstack_entry *yw_PubStack;
extern __far                 UBYTE *yw_ArgStack;
extern __far                 UBYTE *yw_EOArgStack;
#else
extern struct pubstack_entry *yw_PubStack;
extern                 UBYTE *yw_ArgStack;
extern                 UBYTE *yw_EOArgStack;
#endif

/*-------------------------------------------------------------------
**  Ein Array von privaten Status-Strukturen, die Informationen
**  über die Sektoren um den aktuellen Viewer herum enthalten.
**  Das Array ist groß genug, alle Informationen für den
**  "Himmels-Darstellungs-Bereich" zu halten, welcher größer
**  als der "Sektor-Darstellungs-Bereich" ist. Der Sektor-
**  Darstellungs-Bereich ist als "Unterbereich" im Array enthalten.
*/
struct sector_status {
    struct Cell *sector;
    BOOL ok;
    BOOL visible;
    FLOAT x,y,z;
    FLOAT avrg_y;
};

struct sector_status StatusArray[10][10] = { NULL };

/*===================================================================
**  Compare-Hook für qsort() auf Publish-Stack.
*/
int __CDECL yw_cmp(struct pubstack_entry *first,
                   struct pubstack_entry *second)
{
    return(first->depth - second->depth);
}

/*-----------------------------------------------------------------*/
void yw_RenderFrame(Object *o, 
                    struct ypaworld_data *ywd,
                    struct trigger_msg *msg,
                    BOOL render_gui)
/*
**  FUNCTION
**      Rendert einen vollständigen Frame. Sonst nichts.
**      Das beinhaltet:
**
**          (1) Stelle die umliegenden Sektoren dar, indem aus der
**              Sector-Map die Sektor-Typen gelesen werden, das
**              entsprechende base.class Object aus dem TypeArray
**              gepickt wird, dieses positioniert und ein BSM_PUBLISH
**              darauf angewendet wird.
**          (2) Für jeden in (1) gezeichneten Sektor werden die
**              Slurp-ADEs berechnet und gezeichnet.
**          (3) Für jeden in (1) gezeichneten Sektor werden die
**              in den jeweiligen Sektoren befindlichen Bakterien
**              mit einem BSM_PUBLISH bedacht.
**
**          (4) Der aus (1),(2) und (3) resultierende ADE-Stack
**              (Publish- und Args-Stack) wird sortiert und
**              "gerendert".
**              Siehe base.class/BSM_TRIGGER.
**
**  INPUTS
**      o          -> Pointer auf Welt-Object
**      ywd        -> Pointer auf dessen LID
**      msg        -> komplette Trigger-Msg
**      render_gui -> TRUE: yw_RenderVizor() und yw_RenderRequesters()
**                    wird aufgerufen, sonst nicht
**
**  RESULTS
**      ---
**
**  CHANGED
**      09-May-95   floh    created
**      13-May-95   floh    Am Anfang des Frames wird zuerst die
**                          TForm des aktuellen Viewers globalisiert.
**                          (siehe base.class/BSM_TRIGGER)
**      06-Jun-95   floh    Änderungen in LID
**      11-Jun-95   floh    In den gezeichneten Sektoren wird auf alle
**                          Bakterien ein YBM_TR_VISUAL angewendet.
**      26-Jun-95   floh    neu: Slurp-Handling
**      11-Jul-95   floh    neu: yw_RenderHeaven()
**      11-Aug-95   floh    um _ToggleDisplays() herum jetzt _BeginRefresh()
**                          & _EndRefresh() (Mauszeiger-Handling in
**                          Input.Engine).
**      13-Aug-95   floh    yw_RenderRequesters() wird aufgerufen
**      14-Aug-95   floh    Die Debug-Items ywd->PolysAll und ywd->PolysDrawn
**                          werden ausgefüllt.
**      22-Sep-95   floh    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAARGH!
**                          Ich hatte vergessen, die owner_id in der
**                          basepublish_msg zu initialisieren. Das
**                          hatte SEHR mysteriöse Auswirkungen, vor
**                          allem bei den Partikel-Systemen. 4 Tage
**                          und 4 Jahre meiner Lebenserwartung dahin ... ;-)
**      30-Sep-95   floh    + _FlushGfx() [für Span-Clipper-Pipeline]
**                          + yw_RenderRequesters() verlagert, damit
**                            sie nicht überschrieben werden.
**      24-Oct-95   floh    yw_RenderRequesters() nochmal verlagert :-)
**      13-Dec-95   floh    yw_RenderRequesters() wird jetzt aufgerufen,
**                          nachdem das Bild fertig ist
**      06-Apr-96   floh    ruft jetzt yw_RenderVisor() auf
**      22-Jun-96   floh    + WarpIn-Effekt
**      24-Jun-96   floh    + WarpOut-Effekt, nach Beenden der WarpOut-
**                            Sequenz wird das LevelFinished-Flag
**                            gesetzt.
**      23-Jul-96   floh    + Robo-Sektor-Grenzen-Passierungs-Fix.
**                            (Robo hat sich weggeschaltet, wenn
**                            Sektorgrenzen passiert wurden -> wegen neuem
**                            Extra-Viewer-Modell)
**      19-Aug-96   floh    + Render-Aufrufe bissel umgebaut, damit
**                            "asynchrone" Devices paralleler arbeiten
**                            können.
**      01-Sep-96   floh    - yw_RenderVisor() (wird jetzt beim
**                            HUD-Rendern mit erledigt)
**      01-Jun-97   floh    + MinZ/MaxZ-Werte werden ausgefüllt
**                          + verändertes "Diamanten-förmiges" Render-Schema,
**      12-Feb-98   floh    + yw_RenderSuperItems()
*/
{
    /*** ohne Viewer gehts nicht ***/
    if (ywd->Viewer) {

        struct basepublish_msg bpub_msg;
        LONG i,j;
        LONG x, y;
        ULONG act_elm, num_elm;
        tform *vwr;
        struct drawtext_args dt_args;
        ULONG vis,vis_sectors;

        /*** Viewer-TForm globalisieren ***/
        if (vwr = _GetViewer()) _TFormToGlobal(vwr);

        /** die basepublish_msg für BSM_PUBLISH ausfüllen **/
        bpub_msg.frame_time  = msg->frame_time;
        bpub_msg.global_time = msg->global_time;
        bpub_msg.pubstack    = yw_PubStack;
        bpub_msg.argstack    = (struct argstack_entry *) yw_ArgStack;
        bpub_msg.eoargstack  = (struct argstack_entry *) yw_EOArgStack;
        bpub_msg.ade_count   = 0;
        bpub_msg.maxnum_ade  = 2000; // 2100 + Sicherheits-Zone
        bpub_msg.owner_id    = 1;   // != 0, der 22-Sep-95-Bug.
        bpub_msg.min_z       = 17.0;
        #ifdef __WINDOWS__
            if (wdd_DoDirect3D) {
                /*** bei Direct3D Frontplane weiter rein, weil keine ***/
                /*** Genauigkeits-Probleme                           ***/
                bpub_msg.min_z = 1.0;
            };
        #endif
        if (ywd->VisSectors == 5) {
            /*** NearView: Backplane auf 3*Subsektor + 0.5*Subsektor ***/
            bpub_msg.max_z = 1500.0;    // 1*Sektorsize + SlurpSize/2
        }else {
            /*** Farview ***/
            bpub_msg.max_z = 3500.0;    // was 2700: 2*Subsektor + Slurpsize
        };

        /*** <vis> immer abgerundet! ***/
        vis_sectors = ywd->VisSectors-1;
        vis = vis_sectors>>1;

        /*** für jeden potentiell sichtbaren Sektor Status initialisieren ***/
        for (j=0;j<vis_sectors;j++) {
            for (i=0;i<vis_sectors;i++) {
                struct sector_status *status = &(StatusArray[i][j]);
                status->ok      = FALSE;
                status->visible = FALSE;
            };
        };

        /*** Rendering selbst stärker (auf Diamant-Form) einschränken! ***/
        for (i=0; i<=vis; i++) {
            LONG x0 = -i;
            LONG x1 = i;
            LONG y0 = -(vis-i);
            LONG y1 = vis-i;
            LONG x,y;
            for (x=x0; x<=x1; x++) {
                struct sector_status *status = &(StatusArray[x+vis][y0+vis]);
                LONG abs_x = ywd->Viewer->SectX + x;
                LONG abs_y = ywd->Viewer->SectY + y0;                
                yw_RenderBeeBox(ywd,abs_x,abs_y,status,&bpub_msg);
                if (status->ok) {
                    yw_RenderSector(ywd,status,&bpub_msg);
                };
            };
            if (y0 != y1) {
                for (x=x0; x<=x1; x++) {
                    struct sector_status *status = &(StatusArray[x+vis][y1+vis]);
                    LONG abs_x = ywd->Viewer->SectX + x;
                    LONG abs_y = ywd->Viewer->SectY + y1;                
                    yw_RenderBeeBox(ywd,abs_x,abs_y,status,&bpub_msg);
                    if (status->ok) {
                        yw_RenderSector(ywd,status,&bpub_msg);
                    };
                };
            };
        };

        /*** Slurp-Stripes berechnen und publishen ***/
        yw_RenderSlurps(ywd, &bpub_msg);

        /*** Superitems rendern, so welche aktiv sind ***/
        yw_RenderSuperItems(ywd,&bpub_msg);

        /*** Himmel rendern ***/
        if (ywd->RenderHeaven) yw_RenderHeaven(ywd, &bpub_msg);

        /*** ...und endlich die eigentliche Darstellung ***/
        msg->ade_count = bpub_msg.ade_count;

        /* Anzahl Elemente im PubStack */
        num_elm         = (bpub_msg.pubstack - yw_PubStack);
        ywd->PolysAll   = bpub_msg.ade_count;
        ywd->PolysDrawn = num_elm;

        /* Depthsort auf Publish-Stack */
        if (num_elm > 1) {
            /*** unter Direct3D keine Tiefensortierung ***/
            #ifdef __WINDOWS__
                if (!wdd_DoDirect3D) {
                    qsort(yw_PubStack, num_elm, sizeof(struct pubstack_entry), yw_cmp);
                };
            #else
                qsort(yw_PubStack, num_elm, sizeof(struct pubstack_entry), yw_cmp);
            #endif
        };

        /*** Frame zeichnen ***/
        _methoda(ywd->GfxObject,RASTM_Begin3D,NULL);
        for (act_elm=0; act_elm<num_elm; act_elm++) {

            /* Pointer auf Args-Block */
            void *drawargs = yw_PubStack[act_elm].args + 1;

            /* Pointer auf DrawFunc() */
            #ifdef AMIGA
                void __asm (*drawfunc) (__a0 void *);
                drawfunc = (void __asm (*) (__a0 void *))
                           yw_PubStack[act_elm].args->draw_func;
            #else
                void (*drawfunc) (void *);
                drawfunc = yw_PubStack[act_elm].args->draw_func;
            #endif

            /* DrawFunc() ausführen */
            drawfunc(drawargs);
        };
        _methoda(ywd->GfxObject,RASTM_End3D,NULL);
        if (render_gui) {
            _methoda(ywd->GfxObject,RASTM_Begin2D,NULL);
            yw_RenderRequesters(ywd);
            _methoda(ywd->GfxObject,RASTM_End2D,NULL);
        };
    }; /* if (Viewer) */

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yw_RenderBeeBox(struct ypaworld_data *ywd,
                     LONG x, LONG y,
                     struct sector_status *status,
                     struct basepublish_msg *bpub_msg)
/*
**  FUNCTION
**      Rendert das "BeeBox-Object", das außerdem die Darstellung
**      des Himmels mitrealisiert. Nebenbei wird die entsprechende
**      Status-Struktur im Status-Array ausgefüllt.
**
**  INPUTS
**      ywd      -> Ptr auf LID des Welt-Objects
**      x,y      -> Sektor-Position in CellArea (Sektor-Map)
**      status   -> Pointer auf auszufüllende Status-Strukt.
**                  (siehe StatusArray am Anfang dieses Files)
**      bpub_msg -> Fürs Publishing zu verwendende basepublish_msg
**
**  RESULTS
**      status->sector  -> Pointer auf Sektor in Sektor-Map
**      status->ok      -> Sektor-Bereich innerhalb Cell-Area?
**      status->visible -> Sektor-Bereich sichtbar?
**      status->[x,y,z] -> geometrischer Mittelpunkt des Sektors
**      status->avrg_y  -> Durchschnitts-Höhe für Cross-Slurps...
**
**  CHANGED
**      10-Jul-95   floh    created
**      11-Jul-95   floh    BeeBox-Object jetzt wieder "unsichtbar", der
**                          Himmel wird jetzt separat gezeichnet.
**      19-Sep-95   floh    x und y wurden als ULONG übergeben,
**                          ein Gültigkeits-Test kleiner 0 war in
**                          diesem Fall sinnlos
*/
{
    status->ok      = FALSE;
    status->visible = FALSE;

    /*** Sektor-Position gültig? ***/
    if ((x>=0) && (x<ywd->MapSizeX) && (y>=0) && (y<ywd->MapSizeY)) {

        struct Cell *sector;
        struct flt_vector_msg pos;

        /*** Sektor-Adresse ermitteln ***/
        sector = &(ywd->CellArea[y * ywd->MapSizeX + x]);
        status->ok      = TRUE;
        status->sector  = sector;
        status->avrg_y  = sector->AvrgHeight;

        /*** Bounding-Box-Baseobject positionieren ***/
        pos.mask = VEC_X|VEC_Y|VEC_Z;
        pos.x    = (FLOAT) (x * SECTOR_SIZE + SECTOR_SIZE/2);
        pos.z    = (FLOAT) -(y * SECTOR_SIZE + SECTOR_SIZE/2);
        pos.y    = sector->Height;
        status->x = pos.x;
        status->y = pos.y;
        status->z = pos.z;
        _methoda(ywd->BeeBox, BSM_POSITION, &pos);

        /* BeeBox-Object publishen */
        if (_methoda(ywd->BeeBox, BSM_PUBLISH, bpub_msg)) {
            status->visible = TRUE;
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_RenderSector(struct ypaworld_data *ywd,
                     struct sector_status *status,
                     struct basepublish_msg *bpub_msg)
/*
**  FUNCTION
**      Rendert einen kompletten Sektor, mit allen Bakterien,
**      die so drin rumschwirren.
**      *** WICHTIG ***
**      Die Routine darf nur aufgerufen werden, wenn der Sektor
**      auch tatsächlich dargestellt werden kann/sollte
**      (status->visible = TRUE). Dieses Feld wird von
**      yw_RenderBeeBox() initialisiert.
**
**  INPUTS
**      ywd      -> Ptr auf LID des Welt-Objects
**      status   -> Pointer auf ausgefüllte Status-Strukt.
**                  (siehe yw_RenderBeeBox)
**      bpub_msg -> Fürs Publishing zu verwendende basepublish_msg
**
**  RESULTS
**      status ist ausgefüllt.
**
**  CHANGED
**      28-Jun-95   floh    created
**      14-Jul-95   floh    Aargh, Tippfehler beim Bakterien-Publishing...
**      24-Jul-95   floh    Das richtige Visualisierungs-Lego wird
**                          jetzt mit Hilfe der SubSektor-Energie dynamisch
**                          ermittelt.
**      16-Aug-95   floh    Subsektoren werden jetzt nur gepublished,
**                          wenn sie weniger als (ywd->NormVisLimit + 150.0)
**                          vom Viewer entfernt sind!
**      01-Feb-96   floh    jetzt mit visuellem "Bauen"
**      10-Feb-96   floh    Falls gerade bauend, wird immer der "vollste"
**                          Lego genommen (damit keine Ruinen gebaut werden).
**      23-Jul-96   floh    -> muß jetzt auch aufgerufen werden, wenn
**                             <status->visible == FALSE>, in dem Fall
**                             wird zwar nicht die Landschaft gerendert,
**                             aber dafür bei den Bakterien der Robo
**                             (damit er sich nicht an Sektor-Grenzen
**      12-Apr-97   floh    + BP_Array nicht mehr global
**      03-May-98   floh    + norm_dist Optimierung ist rausgeflogen,
**                            um den verbleibenden Plop-Effekt zu eliminieren 
*/
{
    struct MinList *ls;
    struct MinNode *nd;

    if (status->visible) {

        /*** Sektor-Internes-Zeug ist sichtbar ***/
        struct flt_vector_msg pos;
        LONG ix,iy,lim,correct;

        BOOL do_build = FALSE;
        struct flt_vector_msg scale;

        struct Cell *sec = status->sector;

        /*** Sektor-Positions-Msg initialisieren ***/
        pos.mask = (VEC_X|VEC_Y|VEC_Z);
        pos.x    = status->x;
        pos.y    = status->y;
        pos.z    = status->z;

        /*** Sektor im Aufbau? ***/
        if (WTYPE_JobLocked == sec->WType) {

            struct BuildJob   *bj = &(ywd->BuildJobs[sec->WIndex]);
            struct BuildProto *bp = &(ywd->BP_Array[bj->bp]);

            do_build   = TRUE;
            scale.mask = VEC_Y;
            scale.y    = ((FLOAT)bj->age) / ((FLOAT)bj->duration);

            /*** Sektor auf neues Aussehen patchen ***/
            sec->Type  = bp->SecType;
            sec->SType = ywd->Sectors[sec->Type].SecType;
        };

        /*** Sektor-Oberfläche darstellen ***/
        if (sec->SType == SECTYPE_COMPACT) {
            lim=1; correct=0;
        } else {
            lim=3; correct=-1;
        };
        for (iy=0; iy<lim; iy++) {
            for (ix=0; ix<lim; ix++) {

                /*** Subsektor positionieren ***/
                pos.x = status->x + ((ix+correct) * SLURP_WIDTH);
                pos.z = status->z + ((iy+correct) * SLURP_WIDTH);
                if (do_build) {
                    /*** für's Bauen immer den "vollen" Lego nehmen ***/
                    UWORD lego_num = ywd->Sectors[sec->Type].SSD[ix][iy]->limit_lego[DLINDEX_FULL];
                    Object *lego   = ywd->Legos[lego_num].lego;
                    _set(lego, BSA_Static, FALSE);
                    _methoda(lego, BSM_SCALE, &scale);
                    _methoda(lego, BSM_POSITION, &pos);
                    _methoda(lego, BSM_PUBLISH, bpub_msg);
                    _set(lego, BSA_Static, TRUE);
                } else {
                    UWORD lego_num = GET_LEGONUM(ywd,sec,ix,iy);
                    Object *lego   = ywd->Legos[lego_num].lego;
                    _methoda(lego, BSM_POSITION, &pos);
                    _methoda(lego, BSM_PUBLISH, bpub_msg);
                };
            };
        };
    };

    /*** alle Bakterien im Sektor publishen (Robos wegen Größe immer!) ***/
    ls = (struct MinList *) &(status->sector->BactList);
    for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ)
    {
        struct Bacterium *b = (struct Bacterium *) nd;
        if ((status->visible) || (b->BactClassID==BCLID_YPAROBO)) {
            Object *bo = b->BactObject;
            _methoda(bo, YBM_TR_VISUAL, bpub_msg);
        };
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yw_RenderHeaven(struct ypaworld_data *ywd,
                     struct basepublish_msg *bpub_msg)
/*
**  FUNCTION
**      Malt den Himmel :-)
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      11-Jul-95   floh    created
**      16-Jul-95   floh    Test mit Himmel, der sich mitbewegt
**      27-Aug-95   floh    jetzt ohne Multiplizieren.
**      26-Jan-96   floh    testet jetzt, ob Heaven überhaupt vorhanden
**                          ist
**      01-Jun-97   floh    + min_z/max_z Beachtung
*/
{
    if (ywd->Heaven) {
        struct flt_vector_msg pos;
        FLOAT old_max_z = bpub_msg->max_z;
        pos.x = ywd->Viewer->pos.x;
        pos.y = ywd->Viewer->pos.y + ywd->HeavenHeight;
        pos.z = ywd->Viewer->pos.z;
        pos.mask = (VEC_X|VEC_Y|VEC_Z);
        _methoda(ywd->Heaven, BSM_POSITION, &pos);
        bpub_msg->max_z = 32000.0;  // sehr groß
        _methoda(ywd->Heaven, BSM_PUBLISH, bpub_msg);
        bpub_msg->max_z = old_max_z;
    };
}

/*-----------------------------------------------------------------*/
Object *yw_GetVSlurp(struct ypaworld_data *ywd,
                     struct sector_status *s1,
                     struct sector_status *s2,
                     FLOAT n_height,
                     FLOAT s_height)
/*
**  FUNCTION
**      Ermittelt passendes vertikales base.class Slurp-Object
**      zwischen zwei Sektoren, gleicht es an die Sektor-Höhen
**      an und gibt einen Pointer darauf zurück.
**
**  INPUTS
**      ywd      -> Ptr auf LID des Welt-Objects
**      s1       -> gültige <struct sector_status> des westlichen
**                  Sektors (ausgefüllt innerhalb yw_Render())
**      s2       -> wie s1 für östlichen Sektor
**      n_height -> "Durchschnitts-Höhe" des nördlichen Dreiecks
**      s_height -> "Durchschnitts-Höhe" des südlichen Dreiecks
**
**  RESULTS
**      <Object *>
**          Pointer auf Slurp-Object (instance of base.class),
**          fertig zum publishen per BSM_PUBLISH, 
**          oder NULL, wenn nicht geslurpt werden muß/kann.
**
**  CHANGED
**      26-Jun-95   floh    created
*/
{
    if ((s1->ok == TRUE) && (s2->ok == TRUE)) {
        if ((s1->visible == TRUE) || (s2->visible == TRUE)) {

            /*** richtiges Slurp-Object ermitteln ***/
            ULONG i;
            UBYTE st1 = s1->sector->Type;
            UBYTE st2 = s2->sector->Type;

            UBYTE gt1 = ywd->Sectors[st1].GroundType;
            UBYTE gt2 = ywd->Sectors[st2].GroundType;

            Object *slurp_o             = ywd->VSlurp[gt1][gt2].o;
            struct Skeleton *slurp_sklt = ywd->VSlurp[gt1][gt2].sklt;

            struct flt_vector_msg pos_msg;
            fp3d *pool;

            /*** Slurp-Object positionieren ***/
            pos_msg.mask = VEC_X|VEC_Z;
            pos_msg.x    = s2->x;
            pos_msg.z    = s2->z;
            _methoda(slurp_o, BSM_POSITION, &pos_msg);

            /*** Skeleton-Pool-Points angleichen ***/
            pool = slurp_sklt->Pool;
            for (i=0; i<4; i++)  pool[i].y = s1->y;
            for (i=4; i<8; i++)  pool[i].y = s2->y;
            pool[8].y = s_height;
            pool[9].y = n_height;

            /*** Slurp-Stripe jetzt fertig zum publishen... ***/
            return(slurp_o);
        };
    };

    /*** dont slurp ***/
    return(NULL);
}

/*-----------------------------------------------------------------*/
Object *yw_GetHSlurp(struct ypaworld_data *ywd,
                     struct sector_status *s1,
                     struct sector_status *s2,
                     FLOAT w_height,
                     FLOAT e_height)
/*
**  FUNCTION
**      Ermittelt passendes horizontales base.class Slurp-Object
**      zwischen zwei Sektoren, gleicht es an die Sektor-Höhen
**      an und gibt einen Pointer darauf zurück.
**
**  INPUTS
**      ywd      -> Ptr auf LID des Welt-Objects
**      s1       -> gültige <struct sector_status> des nördlichen
**                  Sektors (ausgefüllt innerhalb yw_Render())
**      s2       -> wie s1 für südlichen Sektor
**      w_height -> "Durchschnitts-Höhe" des westlichen Dreiecks
**      e_height -> "Durchschnitts-Höhe" des östlichen Dreiecks
**
**  RESULTS
**      <Object *>
**          Pointer auf Slurp-Object (instance of base.class),
**          fertig zum publishen per BSM_PUBLISH, 
**          oder NULL, wenn nicht geslurpt werden muß/kann.
**
**  CHANGED
**      26-Jun-95   floh    created
*/
{
    if ((s1->ok == TRUE) && (s2->ok == TRUE)) {
        if ((s1->visible == TRUE) || (s2->visible == TRUE)) {

            /*** richtiges Slurp-Object ermitteln ***/
            ULONG i;
            UBYTE st1 = s1->sector->Type;
            UBYTE st2 = s2->sector->Type;

            UBYTE gt1 = ywd->Sectors[st1].GroundType;
            UBYTE gt2 = ywd->Sectors[st2].GroundType;

            Object *slurp_o             = ywd->HSlurp[gt1][gt2].o;
            struct Skeleton *slurp_sklt = ywd->HSlurp[gt1][gt2].sklt;

            struct flt_vector_msg pos_msg;
            fp3d *pool;

            /*** Slurp-Object positionieren ***/
            pos_msg.mask = VEC_X|VEC_Z;
            pos_msg.x    = s2->x;
            pos_msg.z    = s2->z;
            _methoda(slurp_o, BSM_POSITION, &pos_msg);

            /*** Skeleton-Pool-Points angleichen ***/
            pool = slurp_sklt->Pool;
            for (i=0; i<4; i++)  pool[i].y = s1->y;
            for (i=4; i<8; i++)  pool[i].y = s2->y;
            pool[8].y = e_height;
            pool[9].y = w_height;

            /*** Slurp-Stripe jetzt fertig zum publishen... ***/
            return(slurp_o);
        };
    };

    /*** dont slurp ***/
    return(NULL);
}

/*-----------------------------------------------------------------*/
void yw_RenderSlurps(struct ypaworld_data *ywd,
                     struct basepublish_msg *bpub_msg)
/*
**  FUNCTION
**      Zeichnet Slurp-Objects für den gesamten Darstellungs-
**      bereich.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      26-Jun-95   floh    created
**      10-Jul-95   floh    StatusArray[][] enthält jetzt sowohl
**                          die Daten für die Himmels-Bereiche, als
**                          auch die Daten für die "tatsächlichen"
**                          Sektoren... es muß deshalb mit einem
**                          Offset ins StatusArray[][] gearbeitet werden.
**      11-Jul-95   floh    gestrige Änderungen wieder rückgängig gemacht
*/
{
    ULONG x,y;
    Object *so;
    struct sector_status *s1, *s2;
    FLOAT y1, y2;

    /*** vertikale Slurps ***/
    for (y=0; y<(ywd->VisSectors); y++) {
        for (x=0; x<(ywd->VisSectors-1); x++) {

            s1 = &(StatusArray[x][y]);
            s2 = &(StatusArray[x+1][y]);
            y1 = StatusArray[x+1][y].avrg_y;
            y2 = StatusArray[x+1][y+1].avrg_y;

            so = yw_GetVSlurp(ywd, s1, s2, y1, y2);
            if (so) _methoda(so, BSM_PUBLISH, bpub_msg);
        };
    };

    /*** horizontale Slurps ***/
    for (y=0; y<(ywd->VisSectors-1); y++) {
        for (x=0; x<(ywd->VisSectors); x++) {

            s1 = &(StatusArray[x][y]);
            s2 = &(StatusArray[x][y+1]),
            y1 = StatusArray[x][y+1].avrg_y;
            y2 = StatusArray[x+1][y+1].avrg_y;

            so = yw_GetHSlurp(ywd, s1, s2, y1, y2);
            if (so) _methoda(so, BSM_PUBLISH, bpub_msg);
        };
    };

    /*** Ende ***/
}

