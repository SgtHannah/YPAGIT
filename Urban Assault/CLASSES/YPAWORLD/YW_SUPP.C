/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_supp.c,v $
**  $Revision: 38.18 $
**  $Date: 1998/01/06 16:29:26 $
**  $Locker: floh $
**  $Author: floh $
**
**  Bacterium- und Sector- und Misc-Methoden für ypaworld.class.
**
**  (C) Copyright 1995 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/nodes.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define NO_MATHFUNCTIONS    (1)

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "engine/engine.h"
#include "visualstuff/ov_engine.h"
#include "input/input.h"
#include "audio/cdplay.h"
#include "bitmap/ilbmclass.h"
#include "ypa/ypaworldclass.h"
#include "ypa/ypabactclass.h"   /* wegen <struct newmaster_msg> */
#include "bitmap/winddclass.h"

#include "yw_protos.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_ov_engine
_extern_use_audio_engine

#ifdef __WINDOWS__
extern unsigned long wdd_DoDirect3D;
#endif

/*** HACK: globale VFMInput Struktur ***/
extern struct VFMInput Ip;

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, yw_YWM_GETSECTORINFO, struct getsectorinfo_msg *msg)
/*
**  FUNCTION
**      In der Msg wird die 3D-Position des Objects im Welt-Koordinaten-
**      System übergeben. YWM_GETSECTORINFO errechnet daraus
**      folgende Sachen:
**
**          - x,y Koordinate in Sektor-Map
**          - Pointer auf <struct Cell> in Sektor-Map
**          - relative x,z Position innerhalb des Sektors
**            (ohne die Höhe!)
**
**      Diese Daten werden in die Message zurückgeschrieben.
**
**  INPUTS
**
**  RESULTS
**      Falls die übergebene Position ungültig ist (außerhalb
**      der Sektor-Map), wird eine 0L zurückgegeben, sonst
**      eine 1L.
**
**  CHANGED
**      03-May-95   floh    created
**      11-Jun-95   floh    erheblich erweitert
**      12-Jun-95   floh    Sektor-Koordinaten wurden falsch berechnet
**      13-Jun-95   floh    relative Koords wurden falsch berechent (Damn!)
**      03-May-97   floh    der 1-Sektor-Weltrand wird als ungültig
**                          zurückgegeben.
**                          + und wieder rückgängig gemacht, bringt nix!
*/
{
    LONG secx,secy;

    struct ypaworld_data *ywd = INST_DATA(cl,o);

    /*** Sektor-Koordinaten aus Absolut-Position ermitteln ***/
    secx = msg->sec_x = ((LONG)(msg->abspos_x))/((WORD)SECTOR_SIZE);
    secy = msg->sec_y = ((LONG)(-msg->abspos_z))/((WORD)SECTOR_SIZE);

    /* Relativ-Position in Sektor */
    msg->relpos_x = msg->abspos_x - (secx * SECTOR_SIZE) - SECTOR_SIZE/2;
    msg->relpos_z = msg->abspos_z + (secy * SECTOR_SIZE) + SECTOR_SIZE/2;

    /*** Pointer auf Sektor aus Koordinaten ermitteln ***/
    if ((secx>=0)&&(secy>=0)&&(secx<(ywd->MapSizeX))&&(secy<(ywd->MapSizeY))) {
        msg->sector = &(ywd->CellArea[secy * ywd->MapSizeX + secx]);
        return(1L);
    } else {
        _LogMsg("YWM_GETSECTORINFO ausserhalb!!!\n");
        msg->sector = NULL;
        return(0L);
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yw_YWM_SETVIEWER, struct Bacterium *bact)
/*
**  FUNCTION
**      Signalisiert dem Welt-Object einen neuen Viewer.
**
**  INPUTS
**      bact    -> Pointer auf eine gültige Bacterium-Struktur.
**
**  RESULTS
**
**  CHANGED
**      09-May-95   floh    created
*/
{
    struct ypaworld_data *ywd;

    ywd = INST_DATA(cl,o);
    ywd->Viewer = bact;
    _set(o,YWA_UserVehicle,bact->BactObject);
}

/*-----------------------------------------------------------------*/
_dispatcher(struct Bacterium *, yw_YWM_GETVIEWER, void *ignore)
/*
**  FUNCTION
**      Gibt Pointer auf aktuellen Viewer zurück. Achtung,
**      *KEIN* Object-Pointer, sondern <struct Bacterium *>!!!
**
**  INPUTS
**      ---
**
**  RESULTS
**      <struct Bacterium *> des aktuellen Viewers.
**
**  CHANGED
**      07-Aug-95   floh    created
*/
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);
    return(ywd->Viewer);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yw_YWM_GETHEIGHT, struct getheight_msg *msg)
/*
**  FUNCTION
**      Bietet Low-Level-Support für Kollisions-Check gegen
**      'Umwelt' (sprich Sektor-Map). Übergeben wird eine
**      Sektor-Koordinate, sowie eine Position innerhalb
**      des Sektors (nämlich die relative x/z-Position).
**      Die Methode ermittelt die 'Boden-Höhe' an dieser
**      Position (fehlendes Y, Ausrichtung beachten), und
**      den Polygon, der 'getroffen' wird. Die Rückgabe-Werte
**      werden in die Msg zurückgeschrieben.
**      Siehe <struct getheight_msg> Definition.
**
**  INPUTS
**      msg->sector -> Pointer auf Sektor (z.B. per YWM_GETSECTORINFO)
**      msg->pos.x  -> zu testende x und z 3D-Koordinate
**      msg->pos.z
**
**  RESULTS
**      msg->pos.y  -> Ermittelte Y-Koordinate (nur gültig, wenn
**                     (msg->poly != -1)
**      msg->poly   -> Nummer des getroffenen Polygons, -1, falls kein
**                     Treffer
**      msg->sklt   -> Pointer auf Kollisions-Skeleton-Struktur für diesen
**                     Sektor
**
**  CHANGED
**      23-May-95   floh    created
**      28-Jun-95   floh    temporär unschädlich gepatcht...
*/
{
    struct ypaworld_data *ywd;

    ywd = INST_DATA(cl,o);

    msg->poly  = -1;
    msg->pos.y = (FLOAT) 0.0;
    msg->sklt  = NULL;
}

/*-----------------------------------------------------------------*/
_dispatcher(Object *, yw_YWM_GETVISPROTO, ULONG *gvp_msg)
/*
**  FUNCTION
**      Akzeptiert einen Visual-Prototype-Code (0..255),
**      returniert den entsprechenden Object-Pointer, oder
**      NULL, falls kein VisProto für diesen Code geladen wurde.
**
**  INPUTS
**      gvp_msg -> ULONG-Pointer, im ULONG steht der gesuchte Code.
**
**  RESULTS
**      Ein Object-Pointer (instance of base.class), oder NULL.
**
**  EXAMPLE
**      Object = _method(world, YWM_GETVISPROTO, PROTO_HUBI);
**
**  CHANGED
**      06-Jun-95   floh    created
**      23-Sep-95   floh    veränderte VisProto-Array-Struktur
*/
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);
    return(ywd->VisProtos[*gvp_msg].o);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yw_YWM_ADDCOMMANDER, Object *commander)
/*
**  FUNCTION
**      Hängt ein neues Bakterien-Object an die Commander-Liste
**      des Welt-Objects. Das Welt-Object wendet innerhalb
**      YWM_ADDUNIT ein YBA_NEWMASTER mit einem <Object *> == 1L
**      (als Zeichen dafür, daß es sich um das Welt-Object
**      handelt) und einen Pointer auf den CommanderList-Header.
**      In jedem Frame schickt das Welt-Object ein YBA_TR_LOGIC
**      an die Commanders, bei einem Dispose des Welt-Objects
**      werden die Commanders vom Welt-Object disposed.
**
**      Bitte beachten, daß die Methode nur mit _methoda()
**      benutzt werden kann!
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      11-Jun-95   floh    created
*/
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);
    struct newmaster_msg nw_msg;

    /*** eine 'Sonderfall-Msg' bauen und YBM_NEWMASTER invoken ***/
    nw_msg.master      = (void * )1L;
    nw_msg.master_bact = NULL;
    nw_msg.slave_list  = &(ywd->CmdList);
    _methoda(commander, YBM_NEWMASTER, &nw_msg);
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, yw_YWM_ISVISIBLE, struct Bacterium *b)
/*
**  FUNCTION
**      siehe "ypa/ypaworldclass.h"
**
**  CHANGED
**      06-Oct-95   floh    created
**      01-Jun-97   floh    + grenzt den Sichtbereich um einen
**                            weiteren Sektor ein (wegen dem Diamantform-
**                            Sektor-Render-Fix)
**      11-Feb-98   floh    + an Diamant-Form angepaßt
*/
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);
    ULONG retval = FALSE;
    if (ywd->Viewer) {
        LONG dx = ywd->Viewer->SectX - b->SectX;
        LONG dy = ywd->Viewer->SectY - b->SectY;
        dx = (dx < 0) ? -dx:dx;
        dy = (dy < 0) ? -dy:dy;
        if ((dx+dy) <= ((ywd->VisSectors-1)>>1)) return(TRUE);
    };
    return(retval);
}

/*-----------------------------------------------------------------*/
void yw_ScreenShot(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Erzeugt einen Screenshot.
**
**  CHANGED
**      07-Dec-96   floh    created
**      18-Jun-97   floh    + Screenshots jetzt per DISPM_ScreenShot
**      31-Mar-98   floh    + snaps jetzt in env/snaps
*/
{
    struct disp_screenshot_msg dssm;
    Object *gfxo;
    UBYTE filename[256];
    sprintf(filename, "env/snaps/f_%04d", ywd->snap_num++);
    _OVE_GetAttrs(OVET_Object,&gfxo,TAG_DONE);
    dssm.filename = filename;
    _methoda(gfxo,DISPM_ScreenShot,&dssm);
}

/*-----------------------------------------------------------------*/
void yw_DoDigiStuff(struct ypaworld_data *ywd, struct VFMInput *ip)
/*
**  FUNCTION
**      Realisiert Frame-Snapshot, Sequence-Digitizer und
**      Sequence-Recorder-Zeug.
**
**  CHANGED
**      03-Apr-96   floh    created
**      07-Jul-96   floh    erweitert um Sequenz-Recorder.
**      19-Jan-97   floh    + Oh Gott... hatte ich doch beim
**                            Screenshot-Einbauen in die Shell
**                            (oder in den Player) die entsprechenden
**                            Zeilen hier raus-ge-cuttet!!! (anstatt
**                            ins Clipboard zu kopieren...)
**      18-Jun-97   floh    + Screenshots jetzt per DISPM_ScreenShot
*/
{
    /*** Snapshot? ***/
    if (ip->NormKey == KEYCODE_NUM_MUL) yw_ScreenShot(ywd);

    /*** Sequenz-Digitizing? ***/
    if (ywd->seq_digger) {

        struct disp_screenshot_msg dssm;
        Object *gfxo;
        UBYTE filename[256];

        /*** auschalten? ***/
        if (ip->NormKey == KEYCODE_NUM_DIV) ywd->seq_digger = FALSE;
        sprintf(filename, "env/snaps/s%d_%04d", ywd->seq_num, ywd->frame_num++);
        _OVE_GetAttrs(OVET_Object,&gfxo,TAG_DONE);
        dssm.filename = filename;
        _methoda(gfxo,DISPM_ScreenShot,&dssm);

    } else {
        /*** einschalten? ***/
        if (ip->NormKey == KEYCODE_NUM_DIV) {
            ywd->seq_digger = TRUE;
            ywd->seq_num++;
            ywd->frame_num = 0;
        };
    };

    /*** Sequenz-Recording? ***/
    if (ywd->out_seq->active) {
        /*** ausschalten? ***/
        if (ip->NormKey == KEYCODE_NUM_MINUS) yw_RCEndScene(ywd);
    } else {
        /*** einschalten? ***/
        if (ip->NormKey == KEYCODE_NUM_MINUS) yw_RCNewScene(ywd);
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yw_YWM_VISOR, struct visor_msg *msg)
/*
**  CHANGED
**      06-Apr-96   floh    created
*/
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);
    memcpy(&(ywd->visor), msg, sizeof(struct visor_msg));
}

/*-----------------------------------------------------------------*/
void yw_FadeOut(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Simple(!) Paletten-Fade-Funktion. Funktioniert
**      nur, wenn das Display-Device auf den Vertikal-
**      Retrace wartet.
**
**  CHANGED
**      07-Dec-96   floh    created
**      11-Dec-96   floh    zeitlich kürzer
**      04-Mar-98   floh    + wird nur noch unter DirectDraw
**                            ausgeführt
**      17-May-98   floh    + disabled...
*/
{
//    if (!wdd_DoDirect3D) {
//        Object *gfxo;
//        ULONG i;
//        ULONG num_steps = 16;
//        struct snd_cdcontrol_msg cd;
//
//        _OVE_GetAttrs(OVET_Object,&gfxo,TAG_DONE);
//        for (i=0; i<num_steps; i++) {
//
//            struct disp_mixpal_msg dsm;
//            ULONG slot[1];
//            ULONG weight[1];
//
//            dsm.num    = 1;
//            dsm.slot   = slot;
//            dsm.weight = weight;
//            slot[0]    = 0;
//            weight[0]  = 256 - ((i*256)/num_steps);
//           _methoda(gfxo,DISPM_MixPalette,&dsm);
//            delay(20);
//        };
//    };
}

/*-----------------------------------------------------------------*/
void yw_FadeIn(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Gegenstück zu yw_FadeOut() of course.
**
**  CHANGED
**      13-Dec-96   floh    created
**      04-Mar-98   floh    + wird nur noch unter DirectDraw
**                            ausgeführt
**      17-May-98   floh    + ...disabled
*/
{
//    if (!wdd_DoDirect3D) {
//        Object *gfxo;
//        ULONG i;
//        ULONG num_steps = 16;
//
//        _OVE_GetAttrs(OVET_Object,&gfxo,TAG_DONE);
//        for (i=0; i<num_steps; i++) {
//
//            struct disp_mixpal_msg dsm;
//            ULONG slot[1];
//            ULONG weight[1];
//
//            dsm.num    = 1;
//            dsm.slot   = slot;
//            dsm.weight = weight;
//            slot[0]    = 0;
//            weight[0]  = (i*256)/num_steps;
//            _methoda(gfxo,DISPM_MixPalette,&dsm);
//            delay(20);
//        };
//    };
}

/*-----------------------------------------------------------------*/
void yw_BlackOut(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Setzt Palette sofort auf schwarz.
**
**  CHANGED
**      13-Dec-96   floh    created
**      04-Mar-98   floh    + wird nur noch unter DirectDraw
**                            ausgeführt
**      17-May-98   floh    + disabled...
*/
{
//    if (!wdd_DoDirect3D) {
//       Object *gfxo;
//        struct disp_mixpal_msg dsm;
//        ULONG slot[1];
//        ULONG weight[1];
//
//        _OVE_GetAttrs(OVET_Object,&gfxo,TAG_DONE);
//        dsm.num    = 1;
//        dsm.slot   = slot;
//        dsm.weight = weight;
//        slot[0]    = 0;
//        weight[0]  = 0;
//        _methoda(gfxo,DISPM_MixPalette,&dsm);
//    };
}

/*-----------------------------------------------------------------*/
void yw_ShowDiskAccess(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Wird vor einer längeren Lade-Periode aufgerufen und
**      zeigt irgendeine sinnvolle Sache während der Ladezeit
**      an. Das Bild bleibt bis zum nächsten Frame-Aufbau
**      erhalten (das Bitmap-Object wird sofort wieder gekillt).
**
**  CHANGED
**      31-May-97   floh    created
**      17-Sep-97   floh    + CD-Player-Support
*/
{
    UBYTE *filename;
    Object *pic;
    UBYTE old_path[128];
    struct snd_cdcontrol_msg cd;

    /*** AudioTrack für Loading ***/
    cd.command = SND_CD_STOP;
    _ControlCDPlayer(&cd);
    if( ywd->gsr && ywd->gsr->loadingtrack ) {
        cd.command   = SND_CD_SETTITLE;
        cd.para      = ywd->gsr->loadingtrack;
        cd.min_delay = ywd->gsr->loading_min_delay;
        cd.max_delay = ywd->gsr->loading_max_delay;
        _ControlCDPlayer(&cd);
        cd.command = SND_CD_PLAY;
        _ControlCDPlayer(&cd);
    };

    /*** wähle Mipmap-Stufe des Disk-Access-Bild ***/
    if (ywd->DspXRes <= 360)      filename="disk320.ilbm";
    else if (ywd->DspXRes <= 600) filename="disk512.ilbm";
    else                          filename="disk640.ilbm";


    strcpy(old_path,_GetAssign("rsrc"));
    _SetAssign("rsrc","data:mc2res");
    pic = _new("ilbm.class", RSA_Name, filename,
                             BMA_Texture,      TRUE,
                             BMA_TxtBlittable, TRUE,
                             TAG_DONE);
    _SetAssign("rsrc",old_path);
    if (pic) {

        Object *gfxo;
        struct disp_pointer_msg dpm;
        struct rast_blit blt;

        /*** Mauspointer + Audio-Volume holen ***/
        dpm.pointer = ywd->MousePtrBmp[YW_MOUSE_DISK];
        dpm.type    = DISP_PTRTYPE_DISK;
        _get(pic, BMA_Bitmap, &(blt.src));
        blt.from.xmin = blt.to.xmin = -1.0;
        blt.from.ymin = blt.to.ymin = -1.0;
        blt.from.xmax = blt.to.xmax = +1.0;
        blt.from.ymax = blt.to.ymax = +1.0;

        _OVE_GetAttrs(OVET_Object, &gfxo, TAG_DONE);
        if (gfxo) {
            _methoda(gfxo, DISPM_Begin, NULL);
            _methoda(gfxo, DISPM_SetPointer, &dpm);
            _methoda(gfxo, RASTM_Begin2D, NULL);
            _methoda(gfxo, RASTM_Blit, &blt);
            _methoda(gfxo, RASTM_End2D, NULL);
            _methoda(gfxo, DISPM_End, NULL);
        };

        /*** und das Bitmap-Objekt wieder killen ***/
        _dispose(pic);
    };
}

/*-----------------------------------------------------------------*/
void yw_SaveBmpAsAscii(struct ypaworld_data *ywd,
                       Object *bmpo,
                       UBYTE *prefix,
                       APTR fp)
/*
**  FUNCTION
**      Saved ein Bitmap-Object in ein offenes Filehandle als
**      ASCII.
**
**  CHANGED
**      23-Jun-97   floh    created
*/
{
    ULONG x,y;
    struct VFMBitmap *bmp;
    UBYTE *body;

    _get(bmpo,BMA_Bitmap,&bmp);

    /*** schreibe Breite und Höhe ***/
    if (prefix) fprintf(fp,prefix);
    fprintf(fp,"%d %d\n",bmp->Width,bmp->Height);
    body = bmp->Data;

    /*** schreibe Pixel-Data ***/
    for (y=0; y<bmp->Height; y++) {
        if (prefix) fprintf(fp,prefix);
        for (x=0; x<bmp->Width; x++) {
            UBYTE p = *body++;
            fprintf(fp,"%02x ",p);
        };
        fprintf(fp,"\n");
    };

    /*** dat war's ***/
}

/*-----------------------------------------------------------------*/
Object *yw_CreateBmpFromAscii(struct ypaworld_data *ywd,
                              UBYTE *name,
                              APTR fp)
/*
**  FUNCTION
**      Erzeugt ein Bitmap-Objekt, der Inhalt wird aus einem
**      ASCII-Stream gelesen.
**      "name" ist beliebig, muß aber unique sein!
**
**  CHANGED
**      23-Jun-97   floh    created
**      08-Jul-97   floh    es wird jetzt ein "echtes" ilbm.class
**                          Objekt zurückgegeben, damit das Gamesave
**                          erstmal wieder klappt
*/
{
    ULONG w,h;
    UBYTE line[1024];
    UBYTE *ptr;
    Object *bmpo;

    /*** lese Höhe und Breite ***/
    _FGetS(line,sizeof(line),fp);
    ptr = strtok(line," \n");
    w   = strtol(ptr,NULL,0);
    ptr = strtok(NULL," \n");
    h   = strtol(ptr,NULL,0);

    /*** erzeuge Bitmap-Objekt ***/
    bmpo = _new("bitmap.class",
                RSA_Name,   name,
                BMA_Width,  w,
                BMA_Height, h,
                BMA_HasColorMap, TRUE,
                TAG_DONE);
    if (bmpo) {
        struct VFMBitmap *bmp;
        Object *clone_bmpo;
        UBYTE *body;
        ULONG x,y;
        _get(bmpo,BMA_Bitmap,&bmp);
        body = bmp->Data;
        for (y=0; y<bmp->Height; y++) {

            UBYTE *first_hit;

            /*** neue Zeile lesen ***/
            _FGetS(line,sizeof(line),fp);
            first_hit = line;

            /*** Zahlen parsen... ***/
            for (x=0; x<bmp->Width; x++) {
                ptr = strtok(first_hit," \n");
                if (first_hit) first_hit=NULL;
                /*** immer eine 2-stellige Hex-Zahl ***/
                *body++ = (UBYTE) strtol(ptr,NULL,16);
            };
        };
        /*** erzeuge einen ILBM-Clone dieses Objects ***/
        clone_bmpo = _new("ilbm.class",
                          RSA_Name,        name,
                          ILBMA_SaveILBM,  TRUE,
                          TAG_DONE);
        _dispose(bmpo);
        bmpo = clone_bmpo;
    };
    return(bmpo);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yw_YWM_NOTIFYWEAPONHIT, struct yw_notifyweaponhit_msg *msg)
/*
**  CHANGED
**      01-Aug-97   floh    created
*/
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);
    ywd->WeaponHitTimeStamp = ywd->TimeStamp;
    ywd->WeaponHitPower     = msg->power;
}

/*-----------------------------------------------------------------*/
Object *yw_CopyBmpObject(Object *src, UBYTE *copy_name)
/*
**  FUNCTION
**      Erzeugt eine Kopie des übergebenen Bitmap-Objects.
**
**  CHANGED
**      01-Sep-97   floh    created
*/
{
    struct VFMBitmap *src_bmp;
    struct VFMBitmap *tar_bmp;
    Object *tar = NULL;

    _get(src,BMA_Bitmap,&src_bmp);
    tar = _new("bitmap.class",
               RSA_Name,   copy_name,
               BMA_Width,  src_bmp->Width,
               BMA_Height, src_bmp->Height,
               TAG_DONE);
    if (tar) {
        _get(tar,BMA_Bitmap,&tar_bmp);
        memcpy(tar_bmp->Data, src_bmp->Data, src_bmp->Width*src_bmp->Height);
    };
    return(tar);
}

/*-----------------------------------------------------------------*/
void yw_BackToRoboNotify(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Muß aufgerufen werden, wenn der User sich in
**      seinen Robo zurücksetzt. Macht u.a. die
**      Abhandlung der "Welcome Back User" VoiceFeedback
**      Message.
**
**  CHANGED
**      26-Sep-97   floh    created
*/
{
    LONG dt = ywd->TimeStamp - ywd->UserInRoboTimeStamp;
    if (dt > 35000) {
        /*** Welcome Back Message absetzen ***/
        struct logmsg_msg lm;
        lm.bact = ywd->URBact;
        lm.pri  = 10;
        lm.msg  = NULL;
        lm.code = LOGMSG_HOST_WELCOMEBACK;
        _methoda(ywd->world,YWM_LOGMSG,&lm);
    };
    ywd->UserInRoboTimeStamp = ywd->TimeStamp;
}

/*-----------------------------------------------------------------*/
ULONG yw_HandleGamePaused(struct ypaworld_data *ywd, struct trigger_msg *msg)
/*
**  FUNCTION
**      Aktiviert, deaktiviert und behandelt den Game Paused
**      Status.
**
**  RESULTS
**      TRUE    -> Spiel im Game Paused Status, BSM_TRIGGER sofort verlassen!
**      FALSE   -> Spiel nicht im GamePaused Modus, alles OK
**
**  CHANGED
**      28-Oct-97   floh    created
**      24-Nov-97   floh    Game Paused Meldung DBCS Enabled
*/
{
    if (!ywd->playing_network) {
        if (ywd->GamePaused) {
            /*** gerade im GamePaused Modus ***/
            if (msg->input->NormKey) {

                /*** PausenMode im nächsten Frame verlassen ***/
                ywd->GamePaused  = FALSE;
                msg->global_time = ywd->GamePausedTimeStamp;

            } else {

                /*** Pausenmode abhandeln ***/
                struct flt_triple vwr_pos;
                struct flt_triple vwr_vec;
                struct flt_m3x3   vwr_mx;

                _methoda(ywd->GfxObject,DISPM_Begin,NULL);
                _set(ywd->GfxObject,RASTA_BGPen,0);
                _methoda(ywd->GfxObject,RASTM_Clear,NULL);

                /*** Listener weit weg positionieren, damit er nix hört ***/
                vwr_pos.x=ywd->ViewerPos.x;
                vwr_pos.y=ywd->ViewerPos.y + 50000.0;
                vwr_pos.z=ywd->ViewerPos.z;
                vwr_vec.x=0.0;
                vwr_vec.y=0.0;
                vwr_vec.z=0.0;
                vwr_mx.m11=1.0; vwr_mx.m12=0.0; vwr_mx.m13=0.0;
                vwr_mx.m21=0.0; vwr_mx.m22=1.0; vwr_mx.m23=0.0;
                vwr_mx.m31=0.0; vwr_mx.m32=0.0; vwr_mx.m33=1.0;
                _StartAudioFrame(1,&vwr_pos,&vwr_vec,&vwr_mx);

                /*** blinkende Game Paused Meldung ***/
                if ((msg->global_time / 500) & 1) {
                    struct rast_text rt;
                    UBYTE str_buf[256];
                    UBYTE *str = str_buf;
                    WORD ypos = -(ywd->FontH>>1);
                    UBYTE *text = ypa_GetStr(ywd->LocHandle,STR_PAUSED,"*** GAME PAUSED, HIT KEY TO CONTINUE ***");

                    new_font(str,FONTID_TRACY);
                    xpos_brel(str,0);
                    ypos_abs(str,ypos);
                    str = yw_TextCenteredSkippedItem(ywd->Fonts[FONTID_TRACY],
                                                     str, text, ywd->DspXRes);
                    eos(str);
                    rt.string = str_buf;
                    rt.clips  = NULL;
                    _methoda(ywd->GfxObject,RASTM_Begin2D,NULL);
                    _methoda(ywd->GfxObject,RASTM_Text,&rt);
                    _methoda(ywd->GfxObject,RASTM_End2D,NULL);
                };
                _EndAudioFrame();
                _methoda(ywd->GfxObject,DISPM_End,NULL);
            };
            return(TRUE);

        } else {
            if (HOTKEY_PAUSE == msg->input->HotKey) {
                ywd->GamePaused  = TRUE;
                ywd->GamePausedTimeStamp = msg->global_time;
            };
        };
    };
    return(FALSE);
}

/*-----------------------------------------------------------------**
**                                                                 **
**  COLOR HANDLING                                                 **
**                                                                 **
**-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
void yw_InitColors(struct ypaworld_data *ywd) 
/*
**  CHANGED
**      09-Dec-97   floh    created
*/
{
    ULONG i;
    for (i=0; i<YPACOLOR_NUM; i++) {
        ULONG r,g,b;
        ywd->Colors[i].r = r = 255;
        ywd->Colors[i].g = g = 255;
        ywd->Colors[i].b = b = 0;
        ywd->Colors[i].i = 10;
        ywd->Colors[i].rgb  = ((r<<16) & 0xff0000) | ((g<<8) & 0x00ff00) | ((b<<0) & 0x0000ff);
    };
}

/*-----------------------------------------------------------------*/
void yw_SetColor(struct ypaworld_data *ywd, ULONG id, 
                 ULONG r, ULONG g, ULONG b, ULONG i)
/*
**  CHANGED
**      09-Dec-97   floh    created
*/
{
    LONG r2,g2,b2;
    ywd->Colors[id].r = r;
    ywd->Colors[id].g = g;
    ywd->Colors[id].b = b;
    ywd->Colors[id].i = i;
    ywd->Colors[id].rgb  = ((r<<16) & 0xff0000) | ((g<<8) & 0x00ff00) | ((b<<0) & 0x0000ff);
}

/*-----------------------------------------------------------------*/
BOOL yw_ParseColorDef(struct ypaworld_data *ywd, ULONG id, UBYTE *def)
/*
**  FUNCTION
**      Parst einen Farbdefinitions-String "r_g_b_i" und setzt
**      den entsprechenden Eintrag in ywd->Colors.
**
**  CHANGED
**      09-Dec-97   floh    created
*/
{
    UBYTE *r_str = strtok(def, "_");
    UBYTE *g_str = strtok(NULL,"_");
    UBYTE *b_str = strtok(NULL,"_");
    UBYTE *i_str = strtok(NULL,"_");
    if (r_str && g_str && b_str && i_str) {
        ULONG r,g,b,i;
        r = atoi(r_str);
        g = atoi(g_str);
        b = atoi(b_str);
        i = atoi(i_str);
        yw_SetColor(ywd,id,r,g,b,i);
        return(TRUE);
    };
    return(FALSE);
};

/*-----------------------------------------------------------------*/
ULONG yw_ParseColors(struct ScriptParser *p)  
/*
**  FUNCTION
**      Parst den <begin_colors>/<end> Block im world.ini
**
**  CHANGED
**      09-Dec-97   floh    created
**      15-Dec-97   floh    HUD-Factors-Keyword
**      16-Dec-97   floh    + alte Keywords raus, neue Keywords rein.
*/
{
    UBYTE *kw   = p->keyword;
    UBYTE *data = p->data;

    if (PARSESTAT_READY == p->status) {

        /*** momentan ausserhalb eines Contexts ***/
        if (stricmp(kw,"begin_colors")==0) {
            p->status = PARSESTAT_RUNNING;
            return(PARSE_ENTERED_CONTEXT);
        } else return(PARSE_UNKNOWN_KEYWORD);

    } else {
        struct ypaworld_data *ywd = (struct ypaworld_data *) p->store[0];
        ULONG r = TRUE;

        /*** momentan innerhalb eines Context ***/
        if (stricmp(kw,"end")==0){
            p->status = PARSESTAT_READY;
            return(PARSE_LEFT_CONTEXT);

        /*** die Rassen ***/
        }else if (stricmp(kw,"owner_0")==0) r=yw_ParseColorDef(ywd,YPACOLOR_OWNER_0,data);
        else if (stricmp(kw,"owner_1")==0)  r=yw_ParseColorDef(ywd,YPACOLOR_OWNER_1,data);
        else if (stricmp(kw,"owner_2")==0)  r=yw_ParseColorDef(ywd,YPACOLOR_OWNER_2,data);
        else if (stricmp(kw,"owner_3")==0)  r=yw_ParseColorDef(ywd,YPACOLOR_OWNER_3,data);
        else if (stricmp(kw,"owner_4")==0)  r=yw_ParseColorDef(ywd,YPACOLOR_OWNER_4,data);
        else if (stricmp(kw,"owner_5")==0)  r=yw_ParseColorDef(ywd,YPACOLOR_OWNER_5,data);
        else if (stricmp(kw,"owner_6")==0)  r=yw_ParseColorDef(ywd,YPACOLOR_OWNER_6,data);
        else if (stricmp(kw,"owner_7")==0)  r=yw_ParseColorDef(ywd,YPACOLOR_OWNER_7,data);
        
        /*** Map ***/
        else if (stricmp(kw,"map_direction")==0)  r=yw_ParseColorDef(ywd,YPACOLOR_MAP_DIRECTION,data);
        else if (stricmp(kw,"map_primtarget")==0) r=yw_ParseColorDef(ywd,YPACOLOR_MAP_PRIMTARGET,data);
        else if (stricmp(kw,"map_sectarget")==0)  r=yw_ParseColorDef(ywd,YPACOLOR_MAP_SECTARGET,data);
        else if (stricmp(kw,"map_commander")==0)  r=yw_ParseColorDef(ywd,YPACOLOR_MAP_COMMANDER,data);
        else if (stricmp(kw,"map_dragbox")==0)    r=yw_ParseColorDef(ywd,YPACOLOR_MAP_DRAGBOX,data);
        else if (stricmp(kw,"map_viewer")==0)     r=yw_ParseColorDef(ywd,YPACOLOR_MAP_VIEWER,data);

        /*** HUD ***/
        else if (stricmp(kw,"hud_weapon")==0)               r=yw_ParseColorDef(ywd,YPACOLOR_HUD_WEAPON_0,data);
        else if (stricmp(kw,"hud_weapon_1")==0)             r=yw_ParseColorDef(ywd,YPACOLOR_HUD_WEAPON_1,data);
        else if (stricmp(kw,"hud_compass_commandvec")==0)   r=yw_ParseColorDef(ywd,YPACOLOR_HUD_COMPASS_COMMANDVEC_0,data);
        else if (stricmp(kw,"hud_compass_commandvec_1")==0) r=yw_ParseColorDef(ywd,YPACOLOR_HUD_COMPASS_COMMANDVEC_1,data);
        else if (stricmp(kw,"hud_compass_primtarget")==0)   r=yw_ParseColorDef(ywd,YPACOLOR_HUD_COMPASS_PRIMTARGET_0,data);
        else if (stricmp(kw,"hud_compass_primtarget_1")==0) r=yw_ParseColorDef(ywd,YPACOLOR_HUD_COMPASS_PRIMTARGET_1,data);
        else if (stricmp(kw,"hud_compass_locktarget")==0)   r=yw_ParseColorDef(ywd,YPACOLOR_HUD_COMPASS_LOCKTARGET_0,data);
        else if (stricmp(kw,"hud_compass_locktarget_1")==0) r=yw_ParseColorDef(ywd,YPACOLOR_HUD_COMPASS_LOCKTARGET_1,data);
        else if (stricmp(kw,"hud_compass_compass")==0)      r=yw_ParseColorDef(ywd,YPACOLOR_HUD_COMPASS_COMPASS_0,data);
        else if (stricmp(kw,"hud_compass_compass_1")==0)    r=yw_ParseColorDef(ywd,YPACOLOR_HUD_COMPASS_COMPASS_1,data);
        else if (stricmp(kw,"hud_vehicle")==0)              r=yw_ParseColorDef(ywd,YPACOLOR_HUD_VEHICLE_0,data);
        else if (stricmp(kw,"hud_vehicle_1")==0)            r=yw_ParseColorDef(ywd,YPACOLOR_HUD_VEHICLE_1,data);
        else if (stricmp(kw,"hud_visor_mg")==0)             r=yw_ParseColorDef(ywd,YPACOLOR_HUD_VISOR_MG_0,data);
        else if (stricmp(kw,"hud_visor_mg_1")==0)           r=yw_ParseColorDef(ywd,YPACOLOR_HUD_VISOR_MG_1,data);
        else if (stricmp(kw,"hud_visor_locked")==0)         r=yw_ParseColorDef(ywd,YPACOLOR_HUD_VISOR_LOCKED_0,data);
        else if (stricmp(kw,"hud_visor_locked_1")==0)       r=yw_ParseColorDef(ywd,YPACOLOR_HUD_VISOR_LOCKED_1,data);
        else if (stricmp(kw,"hud_visor_autonom")==0)        r=yw_ParseColorDef(ywd,YPACOLOR_HUD_VISOR_AUTONOM_0,data);
        else if (stricmp(kw,"hud_visor_autonom_1")==0)      r=yw_ParseColorDef(ywd,YPACOLOR_HUD_VISOR_AUTONOM_1,data);

        /*** Missionbriefing ***/
        else if (stricmp(kw,"brief_lines")==0)              r=yw_ParseColorDef(ywd,YPACOLOR_BRIEF_LINES,data);

        /*** Text ***/
        else if (stricmp(kw,"text_default")==0)             r=yw_ParseColorDef(ywd,YPACOLOR_TEXT_DEFAULT,data);
        else if (stricmp(kw,"text_list")==0)                r=yw_ParseColorDef(ywd,YPACOLOR_TEXT_LIST,data);
        else if (stricmp(kw,"text_list_sel")==0)            r=yw_ParseColorDef(ywd,YPACOLOR_TEXT_LIST_SEL,data);
        else if (stricmp(kw,"text_tooltip")==0)             r=yw_ParseColorDef(ywd,YPACOLOR_TEXT_TOOLTIP,data);
        else if (stricmp(kw,"text_message")==0)             r=yw_ParseColorDef(ywd,YPACOLOR_TEXT_MESSAGE,data);
        else if (stricmp(kw,"text_hud")==0)                 r=yw_ParseColorDef(ywd,YPACOLOR_TEXT_HUD,data);
        else if (stricmp(kw,"text_briefing")==0)            r=yw_ParseColorDef(ywd,YPACOLOR_TEXT_BRIEFING,data);
        else if (stricmp(kw,"text_debriefing")==0)          r=yw_ParseColorDef(ywd,YPACOLOR_TEXT_DEBRIEFING,data);
        else if (stricmp(kw,"text_button")==0)              r=yw_ParseColorDef(ywd,YPACOLOR_TEXT_BUTTON,data);

         /*** UNKNOWN KEYWORD ***/
        else return(PARSE_UNKNOWN_KEYWORD);

        /*** all-ok ***/
        return(PARSE_ALL_OK);
    };

    /*** can't happen ***/
    return(PARSE_UNKNOWN_KEYWORD);
}

/*-----------------------------------------------------------------*/
ULONG yw_GetColor(struct ypaworld_data *ywd, ULONG id)
/*
**  FUNCTION
**      Returniert den RGB Wert, bzw. 8 Bit Index der Farb-ID.    
**
**  CHANGED
**      09-Dec-97   floh    created
*/
{
    #ifdef __WINDOWS__
        if (wdd_DoDirect3D) return(ywd->Colors[id].rgb);
        else                return(ywd->Colors[id].i);
    #else
        return(ywd->Colors[id].i);
    #endif
}

/*-----------------------------------------------------------------*/
void yw_SetMovie(struct ypaworld_data *ywd, ULONG id, UBYTE *name)
/*
**  FUNCTION
**      Trägt eine Movie-Definition in die MovieData Struktur
**      ein.
**
**  CHANGED
**      25-Jan-98   floh    created
**      01-Apr-98   floh    + macht keinen eigenen Filenamensfilter mehr
*/
{
    ywd->MovieData.Valid[id] = TRUE;
    strcpy(&(ywd->MovieData.Name[id][0]),name);
}

/*-----------------------------------------------------------------*/
ULONG yw_ParseMovieData(struct ScriptParser *p)
/*
**  FUNCTION
**      Parst Definitionen für Movies.
**
**  CHANGED
**      25-Jan-98   floh    created
*/
{
    UBYTE *kw   = p->keyword;
    UBYTE *data = p->data;

    if (PARSESTAT_READY == p->status) {

        /*** momentan ausserhalb eines Contexts ***/
        if (stricmp(kw,"begin_movies")==0) {
            struct ypaworld_data *ywd = (struct ypaworld_data *) p->store[0];
            memset(&(ywd->MovieData),0,sizeof(ywd->MovieData));
            p->status = PARSESTAT_RUNNING;
            return(PARSE_ENTERED_CONTEXT);
        } else return(PARSE_UNKNOWN_KEYWORD);

    } else {

        struct ypaworld_data *ywd = (struct ypaworld_data *) p->store[0];

        /*** momentan innerhalb eines Context ***/
        if (stricmp(kw,"end")==0){
            p->status = PARSESTAT_READY;
            return(PARSE_LEFT_CONTEXT);

        } else if (stricmp(kw,"game_intro")==0)
            yw_SetMovie(ywd,MOV_GAME_INTRO,data);
        else if (stricmp(kw,"win_extro")==0)
            yw_SetMovie(ywd,MOV_LOSE_EXTRO,data);
        else if (stricmp(kw,"lose_extro")==0)
            yw_SetMovie(ywd,MOV_WIN_EXTRO,data);
        else if (stricmp(kw,"user_intro")==0)
            yw_SetMovie(ywd,MOV_LOSE_EXTRO,data);
        else if (stricmp(kw,"kyt_intro")==0)
            yw_SetMovie(ywd,MOV_KYT_INTRO,data);
        else if (stricmp(kw,"taer_intro")==0)
            yw_SetMovie(ywd,MOV_TAER_INTRO,data);
        else if (stricmp(kw,"myk_intro")==0)
            yw_SetMovie(ywd,MOV_MYK_INTRO,data);
        else if (stricmp(kw,"sulg_intro")==0)
            yw_SetMovie(ywd,MOV_SULG_INTRO,data);
        else if (stricmp(kw,"black_intro")==0)
            yw_SetMovie(ywd,MOV_BLACK_INTRO,data);

        /*** UNKNOWN KEYWORD ***/
        else return(PARSE_UNKNOWN_KEYWORD);

        /*** all-ok ***/
        return(PARSE_ALL_OK);
    };

    /*** can't happen ***/
    return(PARSE_UNKNOWN_KEYWORD);
}

/*-----------------------------------------------------------------*/
ULONG yw_WorldMiscParser(struct ScriptParser *p)
/*
**  CHANGED
**      04-Apr-98   floh    created
*/
{
    UBYTE *kw   = p->keyword;
    UBYTE *data = p->data;

    if (PARSESTAT_READY == p->status) {

        /*** momentan ausserhalb eines Contexts ***/
        if (stricmp(kw,"begin_misc")==0) {
            p->status = PARSESTAT_RUNNING;
            return(PARSE_ENTERED_CONTEXT);
        } else return(PARSE_UNKNOWN_KEYWORD);

    } else {

        struct ypaworld_data *ywd = (struct ypaworld_data *) p->store[0];

        /*** momentan innerhalb eines Context ***/
        if (stricmp(kw,"end")==0){
            p->status = PARSESTAT_READY;
            return(PARSE_LEFT_CONTEXT);

        }else if (stricmp(kw,"one_game_res")==0){
            if ((stricmp(data,"yes")==0) ||
                (stricmp(data,"true")==0) ||
                (stricmp(data,"on")==0))
            {
                ywd->OneDisplayRes = TRUE;
            }else{
                ywd->OneDisplayRes = FALSE;
            };
        }else if (stricmp(kw,"shell_default_res")==0){
            UBYTE *w_str = strtok(data,"_");
            UBYTE *h_str = strtok(NULL," \t");
            if (w_str && h_str) {
                ULONG id = 0;
                ULONG w  = strtol(w_str,NULL,0);
                ULONG h  = strtol(h_str,NULL,0);
                ywd->ShellRes = ((w<<12)|(h));
            };
        }else if (stricmp(kw,"game_default_res")==0){
            UBYTE *w_str = strtok(data,"_");
            UBYTE *h_str = strtok(NULL," \t");
            if (w_str && h_str) {
                ULONG id = 0;
                ULONG w  = strtol(w_str,NULL,0);
                ULONG h  = strtol(h_str,NULL,0);
                ywd->GameRes = ((w<<12)|(h));
            };

        /*** UNKNOWN KEYWORD ***/
        }else return(PARSE_UNKNOWN_KEYWORD);

        /*** all-ok ***/
        return(PARSE_ALL_OK);
    };

    /*** can't happen ***/
    return(PARSE_UNKNOWN_KEYWORD);
}

/*-----------------------------------------------------------------*/
void yw_PlayMovie(struct ypaworld_data *ywd, UBYTE *fname)
/*
**  CHANGED
**      26-Jan-98   floh    created
**      01-Apr-98   floh    Filenames werden jetzt gemangelt
*/
{
    UBYTE buf[256];
    struct windd_playmovie wpm;
    _ManglePath(fname,buf,sizeof(buf));
    wpm.fname = buf;
    _methoda(ywd->GfxObject,WINDDM_PlayMovie,&wpm);
    _GetInput(&Ip);
    Ip.NormKey = 0;
    Ip.ContKey = 0;
    Ip.HotKey  = 0;
};

/*-----------------------------------------------------------------*/
void yw_PlayIntroMovie(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Falls definiert, wird Init-Movie gestartet. Sollte
**      normalerweise aus YWM_INITGAMESHELL aufgerufen werden.
**
**  CHANGED
**      25-Jan-98   floh    created
*/
{
    if (ywd->MovieData.Valid[MOV_GAME_INTRO]) {
        _OVE_GetAttrs(OVET_Object, &(ywd->GfxObject), TAG_DONE);
        yw_PlayMovie(ywd,&(ywd->MovieData.Name[MOV_GAME_INTRO][0]));
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yw_YWM_ONLINEHELP, struct yw_onlinehelp_msg *msg)
/*
**  CHANGED
**      05-Mar-98   floh    created
*/
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);
    Object *gfx;
    _OVE_GetAttrs(OVET_Object,&gfx,TAG_DONE);
    if (gfx && msg->url && (!ywd->playing_network)) {
        UBYTE buf[256];
        _ManglePath(msg->url,buf,sizeof(buf));
        yw_LaunchOnlineHelp(buf);
    };
}

/*-----------------------------------------------------------------*/
void yw_ClipFloatRect(struct rast_rect *r)
/*
**  FUNCTION
**      Clippt die Koordinaten in <r> gegen
**      [-1.0,+1.0].
**
**  CHANGED
**      16-Mar-98   floh    created
*/
{
    if (r->xmin < -1.0)      r->xmin = -1.0;
    else if (r->xmin > +1.0) r->xmin = +1.0;
    if (r->ymin < -1.0)      r->ymin = -1.0;
    else if (r->ymin > +1.0) r->ymin = +1.0;
    if (r->xmax < -1.0)      r->xmax = -1.0;
    else if (r->xmax > +1.0) r->xmax = +1.0;
    if (r->ymax < -1.0)      r->ymax = -1.0;
    else if (r->ymax > +1.0) r->ymax = +1.0;
}

/*-----------------------------------------------------------------*/
ULONG yw_AssignParser(struct ScriptParser *p)
/*
**  FUNCTION
**      Parst einen <begin_assign> Block.
**
**  CHANGED
**      01-Apr-98   floh    created
*/
{
    UBYTE *kw   = p->keyword;
    UBYTE *data = p->data;
    if (PARSESTAT_READY == p->status) {
        /*** momentan außerhalb eines Kontext ***/
        if (stricmp(kw, "begin_assign")==0) {
            p->status = PARSESTAT_RUNNING;
            return(PARSE_ENTERED_CONTEXT);
        } else return(PARSE_UNKNOWN_KEYWORD);
    } else {
        /*** momentan innerhalb eines Kontext ***/
        if (stricmp(kw,"end")==0) {
            p->status = PARSESTAT_READY;
            return(PARSE_LEFT_CONTEXT);
        } else if (kw && data) {
            /*** alle anderen kw/data Paare werden als Assigns behandelt ***/
            _SetAssign(kw,data);
            _LogMsg("parsing assign.txt: set assign %s to %s\n",kw,data);
        } else return(PARSE_BOGUS_DATA);
        return(PARSE_ALL_OK);
    };

    /*** can't happen ***/
    return(PARSE_UNKNOWN_KEYWORD);
}

/*-----------------------------------------------------------------*/
BOOL yw_ParseAssignScript(UBYTE *script)
/*
**  FUNCTION
**      Parst ein Script mit einem oder mehreren <begin_assign>
**      Blöcken.
**
**  CHANGED
**      01-Apr-98   floh    created
*/
{
    struct ScriptParser parsers[1];
    memset(parsers,0,sizeof(parsers));
    parsers[0].parse_func = yw_AssignParser;
    return(yw_ParseScript(script,1,parsers,0));
}

/*-----------------------------------------------------------------*/
void yw_ParseAssignRegistryKeys(void)
/*
**  FUNCTION
**      Ueberschreibt die von yw_ParseAssignScript() erzeugten 
**      Assigns falls in der Registry welche definiert sind.
**
**  CHANGED
**      28-Apr-98   floh    created
*/
{
    UBYTE buf[1024];
    ULONG i;
    for (i=0; i<6; i++) {
        UBYTE *name;
        switch(i) {
            case 0:  name="data";   break;
            case 1:  name="save";   break;
            case 2:  name="help";   break;
            case 3:  name="mov";    break;
            case 4:  name="levels"; break;
            case 5:  name="mbpix";  break;
            default: name="nop";   break;
        };
        if (yw_ReadRegistryKeyString(name,buf,sizeof(buf))) {
            _SetAssign(name,buf);
            _LogMsg("parsing registry: set assign %s to %s\n",name,buf);
        };
    };
}
        
    
    
    
