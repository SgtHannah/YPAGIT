/*

**  $Source: PRG:VFM/Engines/InputEngine/ie_main.c,v $

**  $Revision: 38.7 $

**  $Date: 1996/11/26 01:44:14 $

**  $Locker:  $

**  $Author: floh $

**

**  Systemunabhängige Input-Engine als Kompatiblitäts-

**  Hülle für die neue input.class.

**

**  (C) Copyright 1996 by A.Weissflog

*/

#include <exec/types.h>

#include <utility/tagitem.h>



#include <stdlib.h>

#include <string.h>



#include "nucleus/nucleus2.h"

#include "engine/engine.h"

#include "modules.h"

#include "input/ie.h"

#include "input/inputclass.h"

#include "input/iwimpclass.h"

#include "input/idevclass.h"

#include "input/itimerclass.h"



/*-------------------------------------------------------------------

**  Prototypes

*/

BOOL ie_Open(ULONG,...);

void ie_Close(void);

void ie_SetAttrs(ULONG,...);

void ie_GetAttrs(ULONG,...);



/*-------------------------------------------------------------------

**  Global Entry Table der Input-Engine

*/

struct GET_Engine ie_IE_GET = {

    ie_Open,

    ie_Close,

    ie_SetAttrs,

    ie_GetAttrs,

};



/* Einsprung-Tabelle für Dynamic Linking */

#ifdef DYNAMIC_LINKING

struct ie_GET_Specific ie_GET_SPEC = {

    ie_GetInput,

    ie_EnableGUI,

    ie_DisableGUI,

    ie_AddClickBox,

    ie_RemClickBox,

    ie_BeginRefresh,

    ie_EndRefresh,

};

#endif



/*-----------------------------------------------------------------*/



_use_nucleus



Object *InputObject = NULL;     // das zentrale Input-Collector-Object



/*** Config-Handling ***/

UBYTE TimerName[CONFIG_MAX_STRING_LEN];

UBYTE WimpName[CONFIG_MAX_STRING_LEN];

UBYTE KeyboardName[CONFIG_MAX_STRING_LEN];

UBYTE ButtonName[32][CONFIG_MAX_STRING_LEN];

UBYTE SliderName[32][CONFIG_MAX_STRING_LEN];

UBYTE HotkeyName[IDEV_NUMHOTKEYS][CONFIG_MAX_STRING_LEN];



struct ConfigItem ie_ConfigItems[] = {



    /*** Misc ***/

    { IECONF_Debug,    CONFIG_BOOL, FALSE },

    { IECONF_Timer,    CONFIG_STRING, (ULONG)TimerName },

    { IECONF_Wimp,     CONFIG_STRING, (ULONG)WimpName },

    { IECONF_Keyboard, CONFIG_STRING, (ULONG)KeyboardName },



    /*** Buttons ***/

    { IECONF_Button0,  CONFIG_ROL, (ULONG)ButtonName[0] },

    { IECONF_Button1,  CONFIG_ROL, (ULONG)ButtonName[1] },

    { IECONF_Button2,  CONFIG_ROL, (ULONG)ButtonName[2] },

    { IECONF_Button3,  CONFIG_ROL, (ULONG)ButtonName[3] },

    { IECONF_Button4,  CONFIG_ROL, (ULONG)ButtonName[4] },

    { IECONF_Button5,  CONFIG_ROL, (ULONG)ButtonName[5] },

    { IECONF_Button6,  CONFIG_ROL, (ULONG)ButtonName[6] },

    { IECONF_Button7,  CONFIG_ROL, (ULONG)ButtonName[7] },

    { IECONF_Button8,  CONFIG_ROL, (ULONG)ButtonName[8] },

    { IECONF_Button9,  CONFIG_ROL, (ULONG)ButtonName[9] },

    { IECONF_Button10, CONFIG_ROL, (ULONG)ButtonName[10] },

    { IECONF_Button11, CONFIG_ROL, (ULONG)ButtonName[11] },

    { IECONF_Button12, CONFIG_ROL, (ULONG)ButtonName[12] },

    { IECONF_Button13, CONFIG_ROL, (ULONG)ButtonName[13] },

    { IECONF_Button14, CONFIG_ROL, (ULONG)ButtonName[14] },

    { IECONF_Button15, CONFIG_ROL, (ULONG)ButtonName[15] },

    { IECONF_Button16, CONFIG_ROL, (ULONG)ButtonName[16] },

    { IECONF_Button17, CONFIG_ROL, (ULONG)ButtonName[17] },

    { IECONF_Button18, CONFIG_ROL, (ULONG)ButtonName[18] },

    { IECONF_Button19, CONFIG_ROL, (ULONG)ButtonName[19] },

    { IECONF_Button20, CONFIG_ROL, (ULONG)ButtonName[20] },

    { IECONF_Button21, CONFIG_ROL, (ULONG)ButtonName[21] },

    { IECONF_Button22, CONFIG_ROL, (ULONG)ButtonName[22] },

    { IECONF_Button23, CONFIG_ROL, (ULONG)ButtonName[23] },

    { IECONF_Button24, CONFIG_ROL, (ULONG)ButtonName[24] },

    { IECONF_Button25, CONFIG_ROL, (ULONG)ButtonName[25] },

    { IECONF_Button26, CONFIG_ROL, (ULONG)ButtonName[26] },

    { IECONF_Button27, CONFIG_ROL, (ULONG)ButtonName[27] },

    { IECONF_Button28, CONFIG_ROL, (ULONG)ButtonName[28] },

    { IECONF_Button29, CONFIG_ROL, (ULONG)ButtonName[29] },

    { IECONF_Button30, CONFIG_ROL, (ULONG)ButtonName[30] },

    { IECONF_Button31, CONFIG_ROL, (ULONG)ButtonName[31] },



    /*** Sliders ***/

    { IECONF_Slider0,  CONFIG_ROL, (ULONG)SliderName[0] },

    { IECONF_Slider1,  CONFIG_ROL, (ULONG)SliderName[1] },

    { IECONF_Slider2,  CONFIG_ROL, (ULONG)SliderName[2] },

    { IECONF_Slider3,  CONFIG_ROL, (ULONG)SliderName[3] },

    { IECONF_Slider4,  CONFIG_ROL, (ULONG)SliderName[4] },

    { IECONF_Slider5,  CONFIG_ROL, (ULONG)SliderName[5] },

    { IECONF_Slider6,  CONFIG_ROL, (ULONG)SliderName[6] },

    { IECONF_Slider7,  CONFIG_ROL, (ULONG)SliderName[7] },

    { IECONF_Slider8,  CONFIG_ROL, (ULONG)SliderName[8] },

    { IECONF_Slider9,  CONFIG_ROL, (ULONG)SliderName[9] },

    { IECONF_Slider10, CONFIG_ROL, (ULONG)SliderName[10] },

    { IECONF_Slider11, CONFIG_ROL, (ULONG)SliderName[11] },

    { IECONF_Slider12, CONFIG_ROL, (ULONG)SliderName[12] },

    { IECONF_Slider13, CONFIG_ROL, (ULONG)SliderName[13] },

    { IECONF_Slider14, CONFIG_ROL, (ULONG)SliderName[14] },

    { IECONF_Slider15, CONFIG_ROL, (ULONG)SliderName[15] },

    { IECONF_Slider16, CONFIG_ROL, (ULONG)SliderName[16] },

    { IECONF_Slider17, CONFIG_ROL, (ULONG)SliderName[17] },

    { IECONF_Slider18, CONFIG_ROL, (ULONG)SliderName[18] },

    { IECONF_Slider19, CONFIG_ROL, (ULONG)SliderName[19] },

    { IECONF_Slider20, CONFIG_ROL, (ULONG)SliderName[20] },

    { IECONF_Slider21, CONFIG_ROL, (ULONG)SliderName[21] },

    { IECONF_Slider22, CONFIG_ROL, (ULONG)SliderName[22] },

    { IECONF_Slider23, CONFIG_ROL, (ULONG)SliderName[23] },

    { IECONF_Slider24, CONFIG_ROL, (ULONG)SliderName[24] },

    { IECONF_Slider25, CONFIG_ROL, (ULONG)SliderName[25] },

    { IECONF_Slider26, CONFIG_ROL, (ULONG)SliderName[26] },

    { IECONF_Slider27, CONFIG_ROL, (ULONG)SliderName[27] },

    { IECONF_Slider28, CONFIG_ROL, (ULONG)SliderName[28] },

    { IECONF_Slider29, CONFIG_ROL, (ULONG)SliderName[29] },

    { IECONF_Slider30, CONFIG_ROL, (ULONG)SliderName[30] },

    { IECONF_Slider31, CONFIG_ROL, (ULONG)SliderName[31] },



    /*** Hotkeys ***/

    { "input.hotkey[0]",  CONFIG_STRING, (ULONG)HotkeyName[0] },

    { "input.hotkey[1]",  CONFIG_STRING, (ULONG)HotkeyName[1] },

    { "input.hotkey[2]",  CONFIG_STRING, (ULONG)HotkeyName[2] },

    { "input.hotkey[3]",  CONFIG_STRING, (ULONG)HotkeyName[3] },

    { "input.hotkey[4]",  CONFIG_STRING, (ULONG)HotkeyName[4] },

    { "input.hotkey[5]",  CONFIG_STRING, (ULONG)HotkeyName[5] },

    { "input.hotkey[6]",  CONFIG_STRING, (ULONG)HotkeyName[6] },

    { "input.hotkey[7]",  CONFIG_STRING, (ULONG)HotkeyName[7] },

    { "input.hotkey[8]",  CONFIG_STRING, (ULONG)HotkeyName[8] },

    { "input.hotkey[9]",  CONFIG_STRING, (ULONG)HotkeyName[9] },

    { "input.hotkey[10]", CONFIG_STRING, (ULONG)HotkeyName[10] },

    { "input.hotkey[11]", CONFIG_STRING, (ULONG)HotkeyName[11] },

    { "input.hotkey[12]", CONFIG_STRING, (ULONG)HotkeyName[12] },

    { "input.hotkey[13]", CONFIG_STRING, (ULONG)HotkeyName[13] },

    { "input.hotkey[14]", CONFIG_STRING, (ULONG)HotkeyName[14] },

    { "input.hotkey[15]", CONFIG_STRING, (ULONG)HotkeyName[15] },

    { "input.hotkey[16]", CONFIG_STRING, (ULONG)HotkeyName[16] },

    { "input.hotkey[17]", CONFIG_STRING, (ULONG)HotkeyName[17] },

    { "input.hotkey[18]", CONFIG_STRING, (ULONG)HotkeyName[18] },

    { "input.hotkey[19]", CONFIG_STRING, (ULONG)HotkeyName[19] },

    { "input.hotkey[20]", CONFIG_STRING, (ULONG)HotkeyName[20] },

    { "input.hotkey[21]", CONFIG_STRING, (ULONG)HotkeyName[21] },

    { "input.hotkey[22]", CONFIG_STRING, (ULONG)HotkeyName[22] },

    { "input.hotkey[23]", CONFIG_STRING, (ULONG)HotkeyName[23] },

    { "input.hotkey[24]", CONFIG_STRING, (ULONG)HotkeyName[24] },

    { "input.hotkey[25]", CONFIG_STRING, (ULONG)HotkeyName[25] },

    { "input.hotkey[26]", CONFIG_STRING, (ULONG)HotkeyName[26] },

    { "input.hotkey[27]", CONFIG_STRING, (ULONG)HotkeyName[27] },

    { "input.hotkey[28]", CONFIG_STRING, (ULONG)HotkeyName[28] },

    { "input.hotkey[29]", CONFIG_STRING, (ULONG)HotkeyName[29] },

    { "input.hotkey[30]", CONFIG_STRING, (ULONG)HotkeyName[30] },

    { "input.hotkey[31]", CONFIG_STRING, (ULONG)HotkeyName[31] },

    { "input.hotkey[32]", CONFIG_STRING, (ULONG)HotkeyName[32] },

    { "input.hotkey[33]", CONFIG_STRING, (ULONG)HotkeyName[33] },

    { "input.hotkey[34]", CONFIG_STRING, (ULONG)HotkeyName[34] },

    { "input.hotkey[35]", CONFIG_STRING, (ULONG)HotkeyName[35] },

    { "input.hotkey[36]", CONFIG_STRING, (ULONG)HotkeyName[36] },

    { "input.hotkey[37]", CONFIG_STRING, (ULONG)HotkeyName[37] },

    { "input.hotkey[38]", CONFIG_STRING, (ULONG)HotkeyName[38] },

    { "input.hotkey[39]", CONFIG_STRING, (ULONG)HotkeyName[39] },

    { "input.hotkey[40]", CONFIG_STRING, (ULONG)HotkeyName[40] },

    { "input.hotkey[41]", CONFIG_STRING, (ULONG)HotkeyName[41] },

    { "input.hotkey[42]", CONFIG_STRING, (ULONG)HotkeyName[42] },

    { "input.hotkey[43]", CONFIG_STRING, (ULONG)HotkeyName[43] },

    { "input.hotkey[44]", CONFIG_STRING, (ULONG)HotkeyName[44] },

    { "input.hotkey[45]", CONFIG_STRING, (ULONG)HotkeyName[45] },

    { "input.hotkey[46]", CONFIG_STRING, (ULONG)HotkeyName[46] },

    { "input.hotkey[47]", CONFIG_STRING, (ULONG)HotkeyName[47] },

};





#define IE_NUM_CONFIG_ITEMS (32 + 32 + 48 + 4)



/*-------------------------------------------------------------------

**  *** CODE SEGMENT HEADER ***

*/

#ifdef DYNAMIC_LINKING

#ifdef AMIGA

__geta4 struct GET_Engine *start(void)

{

    return(&ie_IE_GET);

}

#endif

#endif



#ifdef STATIC_LINKING

struct GET_Engine *ie_entry(void)

{

    return(&ie_IE_GET);

}



struct SegInfo ie_engine_seg = {

    { NULL, NULL,

      0, 0,

      "MC2engines:input.engine",

    },

    ie_entry,

};

#endif



/*-----------------------------------------------------------------*/

#ifdef DYNAMIC_LINKING

struct GET_Nucleus *local_GetNucleus(struct TagItem *tagList)

{

    register ULONG act_tag;



    while ((act_tag = tagList->ti_Tag) != MID_NUCLEUS) {

        switch (act_tag) {

            case TAG_DONE:  return(NULL); break;

            case TAG_MORE:  tagList = (struct TagItem *) tagList->ti_Data; break;

            case TAG_SKIP:  tagList += tagList->ti_Data; break;

            default:        tagList++;

        };

    };

    return((struct GET_Nucleus *)tagList->ti_Data);

}

#endif



/*-----------------------------------------------------------------*/

#ifdef AMIGA

__geta4 BOOL ie_Open(ULONG id,...)

#else

BOOL ie_Open(ULONG id,...)

#endif

/*

**  CHANGED

**      26-Feb-96   floh    created

**      07-Mar-96   floh    Button- und Slider-Config-Items werden

**                          jetzt per CONFIG_ROL (Rest Of Line) gelesen,

**                          weil diese Dinger jetzt komplexe

**                          Input Expressions darstellen

**      23-Mar-96   floh    + Hotkey-Support

*/

{

    BOOL retval = FALSE;



    #ifdef DYNAMIC_LINKING

    if (!(NUC_GET = local_GetNucleus((struct TagItem *)&id))) return(FALSE);

    #endif



    memset(TimerName,0,sizeof(TimerName));

    memset(WimpName,0,sizeof(WimpName));

    memset(KeyboardName,0,sizeof(KeyboardName));

    memset(ButtonName,0,sizeof(ButtonName));

    memset(SliderName,0,sizeof(SliderName));

    memset(HotkeyName,0,sizeof(HotkeyName));



    /*** Konfiguration lesen ***/

    _GetConfigItems(NULL, ie_ConfigItems, IE_NUM_CONFIG_ITEMS);



    /*** das zentrale Input-Object erzeugen ***/

    InputObject = _new("input.class", TAG_DONE);



    if (InputObject) {



        struct inp_handler_msg ihnd;

        ULONG i;



        /*** Timer Object erzeugen ***/

        if (*((UBYTE *)ie_ConfigItems[1].data)) {

            ihnd.type = ITYPE_TIMER;

            ihnd.num  = 0;

            ihnd.id   = (UBYTE *) ie_ConfigItems[1].data;

            if (!_methoda(InputObject, IM_SETHANDLER, &ihnd)) {

                _LogMsg("input.engine: WARNING: Timer object creation failed.\n");

            };

        } else {

            _LogMsg("input.engine: WARNING: no Timer driver defined in prefs file.\n");

        };



        /*** WIMP Object erzeugen ***/

        if (*((UBYTE *)ie_ConfigItems[2].data)) {

            ihnd.type = ITYPE_WIMP;

            ihnd.num  = 0;

            ihnd.id   = (UBYTE *) ie_ConfigItems[2].data;

            if (!_methoda(InputObject, IM_SETHANDLER, &ihnd)) {

                _LogMsg("input.engine: WARNING: Wimp object creation failed.\n");

            };

        } else {

            _LogMsg("input.engine: WARNING: no Wimp driver defined in prefs file.\n");

        };



        /*** Keyboard Object erzeugen ***/

        if (*((UBYTE *)ie_ConfigItems[3].data)) {

            ihnd.type = ITYPE_KEYBOARD;

            ihnd.num  = 0;

            ihnd.id   = (UBYTE *) ie_ConfigItems[3].data;

            if (!_methoda(InputObject, IM_SETHANDLER, &ihnd)) {

                _LogMsg("input.engine: WARNING: Keyboard object creation failed.\n");

            };

        } else {

            _LogMsg("input.engine: WARNING: no Keyboard driver defined in prefs file.\n");

        };



        /*** Button Objects erzeugen ***/

        for (i=0; i<32; i++) {



            ULONG cnf_index = i + 4;



            if (*((UBYTE *)ie_ConfigItems[cnf_index].data)) {

                ihnd.type = ITYPE_BUTTON;

                ihnd.num  = i;

                ihnd.id   = (UBYTE *) ie_ConfigItems[cnf_index].data;

                if (!_methoda(InputObject, IM_SETHANDLER, &ihnd)) {

                    _LogMsg("input.engine: WARNING: Button[%d] object creation failed.\n",i);

                };

            };

        };



        /*** Slider Objects erzeugen ***/

        for (i=0; i<32; i++) {



            ULONG cnf_index = i + 32 + 4;



            if (*((UBYTE *)ie_ConfigItems[cnf_index].data)) {

                ihnd.type = ITYPE_SLIDER;

                ihnd.num  = i;

                ihnd.id   = (UBYTE *) ie_ConfigItems[cnf_index].data;

                if (!_methoda(InputObject, IM_SETHANDLER, &ihnd)) {

                    _LogMsg("input.engine: WARNING: Slider[%d] object creation failed.\n",i);

                };

            };

        };



        /*** Hotkeys initialisieren ***/

        for (i=0; i<IDEV_NUMHOTKEYS; i++) {



            ULONG cnf_index = i + 32 + 32 + 4;



            if (*((UBYTE *)ie_ConfigItems[cnf_index].data)) {



                struct inp_delegate_msg del_msg;

                struct idev_sethotkey_msg sh_msg;



                sh_msg.id     = (UBYTE *) ie_ConfigItems[cnf_index].data;

                sh_msg.hotkey = i;



                del_msg.type   = ITYPE_KEYBOARD;

                del_msg.num    = 0;

                del_msg.method = IDEVM_SETHOTKEY;

                del_msg.msg    = &sh_msg;

                if (!_methoda(InputObject, IM_DELEGATE, &del_msg)) {

                    _LogMsg("input.engine: WARNING: Hotkey[%d] (%s) not accepted.\n",i,sh_msg.id);

                };

            };

        };



        /*** Initialisierung abgeschlossen ***/

        retval = TRUE;



    } else {

        _LogMsg("input.engine: ERROR: could not create input.class object.\n");

    };



    /*** Ende ***/

    return(retval);

}



/*-----------------------------------------------------------------*/

#ifdef AMIGA

__geta4 void ie_Close(void)

#else

void ie_Close(void)

#endif

/*

**  CHANGED

**      26-Feb-96   floh    created

*/

{

    if (InputObject) {

        _dispose(InputObject);

        InputObject = NULL;

    };

}



/*-----------------------------------------------------------------*/

#ifdef AMIGA

__geta4 void ie_SetAttrs(ULONG tags,...)

#else

void ie_SetAttrs(ULONG tags,...)

#endif

/*

**  CHANGED

**      26-Feb-96   floh    created

**      25-Nov-96   floh    IET_Pointer obsolete

*/

{

    struct TagItem *tlist = (struct TagItem *) &tags;

    struct TagItem *ti;



    /*** IET_ModeInfo -> IWIMPA_Environment ***/

    ti = _FindTagItem(IET_ModeInfo, tlist);

    if (ti) {



        /*** ein OM_SET an das WIMP-Object konstruieren ***/

        struct TagItem attrs[2];

        struct inp_delegate_msg dlg;



        attrs[0].ti_Tag  = IWIMPA_Environment;

        attrs[0].ti_Data = ti->ti_Data;

        attrs[1].ti_Tag  = TAG_DONE;



        dlg.type   = ITYPE_WIMP;

        dlg.num    = 0;

        dlg.method = OM_SET;

        dlg.msg    = &attrs;

        _methoda(InputObject, IM_DELEGATE, &dlg);

    };



    /*** Ende ***/

}



/*-----------------------------------------------------------------*/

#ifdef AMIGA

__geta4 void ie_GetAttrs(ULONG tags,...)

#else

void ie_GetAttrs(ULONG tags,...)

#endif

/*

**  CHANGED

**      26-Feb-96   floh    created

**      27-May-96   floh    + IET_Object (G)

*/

{

    ULONG *value;

    struct TagItem *tlist = (struct TagItem *) &tags;



    #ifdef DYNAMIC_LINKING

    value = (ULONG *) _GetTagData(IET_GET_SPEC, NULL, tlist);

    if (value) *value = (ULONG) &ie_GET_SPEC;

    #endif



    value = (ULONG *) _GetTagData(IET_Object, NULL, tlist);

    if (value) *value = (ULONG) InputObject;

}



/*-----------------------------------------------------------------*/

#ifdef AMIGA

__geta4 __asm void ie_GetInput(__a0 struct VFMInput *input)

#else

void ie_GetInput(struct VFMInput *input)

#endif

/*

**  CHANGED

**      26-Feb-96   floh    created

*/

{

    _methoda(InputObject, IM_GETINPUT, input);

}



/*-----------------------------------------------------------------*/

#ifdef AMIGA

__geta4 __asm void ie_AddClickBox(__a0 struct ClickBox *cbox, __d0 ULONG flg)

#else

void ie_AddClickBox(struct ClickBox *cbox, ULONG flg)

#endif

/*

**  CHANGED

**      26-Feb-96   floh    created

*/

{

    struct iwimp_clickbox_msg cb_msg;

    struct inp_delegate_msg dlg;



    cb_msg.cb = cbox;

    if (flg == IE_CBX_ADDHEAD) cb_msg.flags = IWIMPF_AddHead;

    else                       cb_msg.flags = 0;



    dlg.type   = ITYPE_WIMP;

    dlg.num    = 0;

    dlg.method = IWIMPM_ADDCLICKBOX;

    dlg.msg    = &cb_msg;

    _methoda(InputObject, IM_DELEGATE, &dlg);

}



/*-----------------------------------------------------------------*/

#ifdef AMIGA

__geta4 __asm void ie_RemClickBox(__a0 struct ClickBox *cbox)

#else

void ie_RemClickBox(struct ClickBox *cbox)

#endif

/*

**  CHANGED

**      26-Feb-96   floh    created

*/

{

    struct iwimp_clickbox_msg cb_msg;

    struct inp_delegate_msg dlg;



    cb_msg.cb = cbox;

    cb_msg.flags = 0;



    dlg.type   = ITYPE_WIMP;

    dlg.num    = 0;

    dlg.method = IWIMPM_REMCLICKBOX;

    dlg.msg    = &cb_msg;

    _methoda(InputObject, IM_DELEGATE, &dlg);

}



/*-----------------------------------------------------------------*/

#ifdef AMIGA

__geta4 __asm void ie_EnableGUI(void)

#else

void ie_EnableGUI(void)

#endif

/*

**  CHANGED

**      26-Feb-96   fllh    created

*/

{

    /*** hat keinerlei Funktion mehr ***/

}



/*-----------------------------------------------------------------*/

#ifdef AMIGA

__geta4 __asm void ie_DisableGUI(void)

#else

void ie_DisableGUI(void)

#endif

/*

**  CHANGED

**      26-Feb-96   floh    created

*/

{

    /*** hat keinerlei Funktion mehr ***/

}



/*-----------------------------------------------------------------*/

#ifdef AMIGA

__geta4 __asm void ie_BeginRefresh(void)

#else

void ie_BeginRefresh(void)

#endif

/*

**  CHANGED

**      26-Feb-96   floh    created

**      25-Nov-96   floh    + obsolete

*/

{ }



/*-----------------------------------------------------------------*/

#ifdef AMIGA

__geta4 __asm void ie_EndRefresh(void)

#else

void ie_EndRefresh(void)

#endif

/*

**  CHANGED

**      26-Feb-96   floh    created

**      25-Nov-96   floh    obsolete

*/

{ }



