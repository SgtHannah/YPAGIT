/***************************************************************************
**
** MUI - MagicUserInterface
** (c) 1993 by Stefan Stuntz
**
** Main Header File
**
****************************************************************************
** Class Tree
****************************************************************************
**
** rootclass               (BOOPSI's base class)
** +--Notify               (implements notification mechanism)
**    +--Family            (handles multiple children)
**    !  +--Menustrip      (describes a complete menu strip)
**    !  +--Menu           (describes a single menu)
**    !  \--Menuitem       (describes a single menu item)
**    +--Application       (main class for all applications)
**    +--Window            (handles intuition window related topics)
**    +--Area              (base class for all GUI elements)
**       +--Rectangle      (creates (empty) rectangles)
**       +--Image          (creates images)
**       +--Text           (creates some text)
**       +--String         (creates a string gadget)
**       +--Prop           (creates a proportional gadget)
**       +--Gauge          (creates a fule gauge)
**       +--Scale          (creates a percentage scale)
**       +--Boopsi         (interface to BOOPSI gadgets)
**       +--Colorfield     (creates a field with changeable color)
**       +--List           (creates a line-oriented list)
**       !  +--Floattext   (special list with floating text)
**       !  +--Volumelist  (special list with volumes)
**       !  +--Scrmodelist (special list with screen modes)
**       !  \--Dirlist     (special list with files)
**       +--Group          (groups other GUI elements)
**          +--Register    (handles page groups with titles)
**          +--Virtgroup   (handles virtual groups)
**          +--Scrollgroup (handles virtual groups with scrollers)
**          +--Scrollbar   (creates a scrollbar)
**          +--Listview    (creates a listview)
**          +--Radio       (creates radio buttons)
**          +--Cycle       (creates cycle gadgets)
**          +--Slider      (creates slider gadgets)
**          +--Coloradjust (creates some RGB sliders)
**          +--Palette     (creates a complete palette gadget)
**          +--Colorpanel  (creates a panel of colors)
**          +--Popstring   (base class for popups)
**             +--Popobject(popup a MUI object in a window)
**             \--Popasl   (popup an asl requester)
**
****************************************************************************
** General Header File Information
****************************************************************************
**
** All macro and structure definitions follow these rules:
**
** Name                       Meaning
**
** MUIC_<class>               Name of a class
** MUIM_<class>_<method>      Method
** MUIP_<class>_<method>      Methods parameter structure
** MUIV_<class>_<method>_<x>  Special method value
** MUIA_<class>_<attrib>      Attribute
** MUIV_<class>_<attrib>_<x>  Special attribute value
** MUIE_<error>               Error return code from MUI_Error()
** MUII_<name>                Standard MUI image
** MUIX_<code>                Control codes for text strings
** MUIO_<name>                Object type for MUI_MakeObject()
**
** MUIA_... attribute definitions are followed by a comment
** consisting of the three possible letters I, S and G.
** I: it's possible to specify this attribute at object creation time.
** S: it's possible to change this attribute with SetAttrs().
** G: it's possible to get this attribute with GetAttr().
**
** Items marked with "Custom Class" are for use in custom classes only!
*/


#ifndef LIBRARIES_MUI_H
#define LIBRARIES_MUI_H

#ifndef EXEC_TYPES_H
#include "exec/types.h"
#endif

#ifndef INTUITION_CLASSES_H
#include "intuition/classes.h"
#endif

#ifndef INTUITION_SCREENS_H
#include "intuition/screens.h"
#endif

#ifndef CLIB_INTUITION_PROTOS_H
#include "clib/intuition_protos.h"
#endif


/***************************************************************************
** Library specification
***************************************************************************/

#define MUIMASTER_NAME "muimaster.library"
#define MUIMASTER_VMIN 8

/*
** !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
** Warning, the macros in this header file work only with muimaster.library
** V8 and above. If you recompile your programs, be sure to open
** muimaster.library with MUIMASTER_VMIN as version number.
** !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*/

#define MUI_OBSOLETE /* include obsolete identifiers */



/***************************************************************************
** Object Types for MUI_MakeObject()
***************************************************************************/

#define MUIO_Label        1   /* STRPTR label, ULONG flags */
#define MUIO_Button       2   /* STRPTR label */
#define MUIO_Checkmark    3   /* STRPTR label */
#define MUIO_Cycle        4   /* STRPTR label, STRPTR *entries */
#define MUIO_Radio        5   /* STRPTR label, STRPTR *entries */
#define MUIO_Slider       6   /* STRPTR label, LONG min, LONG max */
#define MUIO_String       7   /* STRPTR label, LONG maxlen */
#define MUIO_PopButton    8   /* STRPTR imagespec */
#define MUIO_HSpace       9   /* LONG space   */
#define MUIO_VSpace      10   /* LONG space   */
#define MUIO_HBar        11   /* LONG space   */
#define MUIO_VBar        12   /* LONG space   */
#define MUIO_MenustripNM 13   /* struct NewMenu *nm, ULONG flags */
#define MUIO_Menuitem    14   /* STRPTR label, STRPTR shortcut, ULONG flags, ULONG data  */
#define MUIO_BarTitle    15   /* STRPTR label */

#define MUIO_Label_SingleFrame   (1<< 8)
#define MUIO_Label_DoubleFrame   (1<< 9)
#define MUIO_Label_LeftAligned   (1<<10)
#define MUIO_Label_Centered      (1<<11)


/***************************************************************************
** ARexx Interface
***************************************************************************/

struct MUI_Command
{
	char        *mc_Name;
	char        *mc_Template;
	LONG         mc_Parameters;
	struct Hook *mc_Hook;
	LONG         mc_Reserved[5];
};

#define MC_TEMPLATE_ID ((STRPTR)~0)

#define MUI_RXERR_BADDEFINITION  -1
#define MUI_RXERR_OUTOFMEMORY    -2
#define MUI_RXERR_UNKNOWNCOMMAND -3
#define MUI_RXERR_BADSYNTAX      -4


/***************************************************************************
** Return values for MUI_Error()
***************************************************************************/

#define MUIE_OK                  0
#define MUIE_OutOfMemory         1
#define MUIE_OutOfGfxMemory      2
#define MUIE_InvalidWindowObject 3
#define MUIE_MissingLibrary      4
#define MUIE_NoARexx             5
#define MUIE_SingleTask          6



/***************************************************************************
** Standard MUI Images
***************************************************************************/

#define MUII_WindowBack     0   /* These images are configured   */
#define MUII_RequesterBack  1   /* with the preferences program. */
#define MUII_ButtonBack     2
#define MUII_ListBack       3
#define MUII_TextBack       4
#define MUII_PropBack       5
#define MUII_PopupBack      6
#define MUII_SelectedBack   7
#define MUII_ListCursor     8
#define MUII_ListSelect     9
#define MUII_ListSelCur    10
#define MUII_ArrowUp       11
#define MUII_ArrowDown     12
#define MUII_ArrowLeft     13
#define MUII_ArrowRight    14
#define MUII_CheckMark     15
#define MUII_RadioButton   16
#define MUII_Cycle         17
#define MUII_PopUp         18
#define MUII_PopFile       19
#define MUII_PopDrawer     20
#define MUII_PropKnob      21
#define MUII_Drawer        22
#define MUII_HardDisk      23
#define MUII_Disk          24
#define MUII_Chip          25
#define MUII_Volume        26
#define MUII_PopUpBack     27
#define MUII_Network       28
#define MUII_Assign        29
#define MUII_TapePlay      30
#define MUII_TapePlayBack  31
#define MUII_TapePause     32
#define MUII_TapeStop      33
#define MUII_TapeRecord    34
#define MUII_GroupBack     35
#define MUII_SliderBack    36
#define MUII_SliderKnob    37
#define MUII_TapeUp        38
#define MUII_TapeDown      39
#define MUII_Count         40

#define MUII_BACKGROUND    128    /* These are direct color    */
#define MUII_SHADOW        129    /* combinations and are not  */
#define MUII_SHINE         130    /* affected by users prefs.  */
#define MUII_FILL          131
#define MUII_SHADOWBACK    132    /* Generally, you should     */
#define MUII_SHADOWFILL    133    /* avoid using them. Better  */
#define MUII_SHADOWSHINE   134    /* use one of the customized */
#define MUII_FILLBACK      135    /* images above.             */
#define MUII_FILLSHINE     136
#define MUII_SHINEBACK     137
#define MUII_FILLBACK2     138
#define MUII_HSHINEBACK    139
#define MUII_HSHADOWBACK   140
#define MUII_HSHINESHINE   141
#define MUII_HSHADOWSHADOW 142
#define MUII_LASTPAT       142
/*#define MUII_N1HSHINE      143*/



/***************************************************************************
** Special values for some methods
***************************************************************************/

#define MUIV_TriggerValue    0x49893131
#define MUIV_NotTriggerValue 0x49893133
#define MUIV_EveryTime       0x49893131

#define MUIV_Application_Save_ENV     ((STRPTR) 0)
#define MUIV_Application_Save_ENVARC  ((STRPTR)~0)
#define MUIV_Application_Load_ENV     ((STRPTR) 0)
#define MUIV_Application_Load_ENVARC  ((STRPTR)~0)

#define MUIV_Application_ReturnID_Quit -1

#define MUIV_List_Insert_Top             0
#define MUIV_List_Insert_Active         -1
#define MUIV_List_Insert_Sorted         -2
#define MUIV_List_Insert_Bottom         -3

#define MUIV_List_Remove_First           0
#define MUIV_List_Remove_Active         -1
#define MUIV_List_Remove_Last           -2
#define MUIV_List_Remove_Selected       -3

#define MUIV_List_Select_Off             0
#define MUIV_List_Select_On              1
#define MUIV_List_Select_Toggle          2
#define MUIV_List_Select_Ask             3

#define MUIV_List_GetEntry_Active       -1
#define MUIV_List_Select_Active         -1
#define MUIV_List_Select_All            -2

#define MUIV_List_Redraw_Active         -1
#define MUIV_List_Redraw_All            -2

#define MUIV_List_Move_Top               0
#define MUIV_List_Move_Active           -1
#define MUIV_List_Move_Bottom           -2
#define MUIV_List_Move_Next             -3 /* only valid for second parameter */
#define MUIV_List_Move_Previous         -4 /* only valid for second parameter */

#define MUIV_List_Exchange_Top           0
#define MUIV_List_Exchange_Active       -1
#define MUIV_List_Exchange_Bottom       -2
#define MUIV_List_Exchange_Next         -3 /* only valid for second parameter */
#define MUIV_List_Exchange_Previous     -4 /* only valid for second parameter */

#define MUIV_List_Jump_Top               0
#define MUIV_List_Jump_Active           -1
#define MUIV_List_Jump_Bottom           -2

#define MUIV_Colorpanel_GetColor_Active -1
#define MUIV_Colorpanel_SetColor_Active -1

#define MUIV_List_NextSelected_Start    -1
#define MUIV_List_NextSelected_End      -1


/***************************************************************************
** Control codes for text strings
***************************************************************************/

#define MUIX_R "\033r"    /* right justified */
#define MUIX_C "\033c"    /* centered        */
#define MUIX_L "\033l"    /* left justified  */

#define MUIX_N "\033n"    /* normal     */
#define MUIX_B "\033b"    /* bold       */
#define MUIX_I "\033i"    /* italic     */
#define MUIX_U "\033u"    /* underlined */

#define MUIX_PT "\0332"   /* text pen           */
#define MUIX_PH "\0338"   /* highlight text pen */



/***************************************************************************
** Parameter structures for some classes
***************************************************************************/

struct MUI_Palette_Entry
{
	LONG  mpe_ID;
	ULONG mpe_Red;
	ULONG mpe_Green;
	ULONG mpe_Blue;
	LONG  mpe_Group;
};

#define MUIV_Palette_Entry_End -1


struct MUI_Scrmodelist_Entry
{
	char *sme_Name;
	ULONG sme_ModeID;
};



/***************************************************************************
**
** Macro Section
** -------------
**
** To make GUI creation more easy and understandable, you can use the
** macros below. If you dont want, just define MUI_NOSHORTCUTS to disable
** them.
**
** These macros are available to C programmers only.
**
***************************************************************************/

#ifndef MUI_NOSHORTCUTS



/***************************************************************************
**
** Object Generation
** -----------------
**
** The xxxObject (and xChilds) macros generate new instances of MUI classes.
** Every xxxObject can be followed by tagitems specifying initial create
** time attributes for the new object and must be terminated with the
** End macro:
**
** obj = StringObject,
**          MUIA_String_Contents, "foo",
**          MUIA_String_MaxLen  , 40,
**          End;
**
** With the Child, SubWindow and WindowContents shortcuts you can
** construct a complete GUI within one command:
**
** app = ApplicationObject,
**
**          ...
**
**          SubWindow, WindowObject,
**             WindowContents, VGroup,
**                Child, String("foo",40),
**                Child, String("bar",50),
**                Child, HGroup,
**                   Child, CheckMark(TRUE),
**                   Child, CheckMark(FALSE),
**                   End,
**                End,
**             End,
**
**          SubWindow, WindowObject,
**             WindowContents, HGroup,
**                Child, ...,
**                Child, ...,
**                End,
**             End,
**
**          ...
**
**          End;
**
***************************************************************************/

#define MenustripObject   MUI_NewObject(MUIC_Menustrip
#define MenuObject        MUI_NewObject(MUIC_Menu
#define MenuObjectT(name) MUI_NewObject(MUIC_Menu,MUIA_Menu_Title,name
#define MenuitemObject    MUI_NewObject(MUIC_Menuitem
#define WindowObject      MUI_NewObject(MUIC_Window
#define ImageObject       MUI_NewObject(MUIC_Image
#define BitmapObject      MUI_NewObject(MUIC_Bitmap
#define BodychunkObject   MUI_NewObject(MUIC_Bodychunk
#define NotifyObject      MUI_NewObject(MUIC_Notify
#define ApplicationObject MUI_NewObject(MUIC_Application
#define TextObject        MUI_NewObject(MUIC_Text
#define RectangleObject   MUI_NewObject(MUIC_Rectangle
#define ListObject        MUI_NewObject(MUIC_List
#define PropObject        MUI_NewObject(MUIC_Prop
#define StringObject      MUI_NewObject(MUIC_String
#define ScrollbarObject   MUI_NewObject(MUIC_Scrollbar
#define ListviewObject    MUI_NewObject(MUIC_Listview
#define RadioObject       MUI_NewObject(MUIC_Radio
#define VolumelistObject  MUI_NewObject(MUIC_Volumelist
#define FloattextObject   MUI_NewObject(MUIC_Floattext
#define DirlistObject     MUI_NewObject(MUIC_Dirlist
#define SliderObject      MUI_NewObject(MUIC_Slider
#define CycleObject       MUI_NewObject(MUIC_Cycle
#define GaugeObject       MUI_NewObject(MUIC_Gauge
#define ScaleObject       MUI_NewObject(MUIC_Scale
#define BoopsiObject      MUI_NewObject(MUIC_Boopsi
#define ColorfieldObject  MUI_NewObject(MUIC_Colorfield
#define ColorpanelObject  MUI_NewObject(MUIC_Colorpanel
#define ColoradjustObject MUI_NewObject(MUIC_Coloradjust
#define PaletteObject     MUI_NewObject(MUIC_Palette
#define GroupObject       MUI_NewObject(MUIC_Group
#define RegisterObject    MUI_NewObject(MUIC_Register
#define VirtgroupObject   MUI_NewObject(MUIC_Virtgroup
#define ScrollgroupObject MUI_NewObject(MUIC_Scrollgroup
#define PopstringObject   MUI_NewObject(MUIC_Popstring
#define PopobjectObject   MUI_NewObject(MUIC_Popobject
#define PoplistObject     MUI_NewObject(MUIC_Poplist
#define PopaslObject      MUI_NewObject(MUIC_Popasl
#define ScrmodelistObject MUI_NewObject(MUIC_Scrmodelist
#define VGroup            MUI_NewObject(MUIC_Group
#define HGroup            MUI_NewObject(MUIC_Group,MUIA_Group_Horiz,TRUE
#define ColGroup(cols)    MUI_NewObject(MUIC_Group,MUIA_Group_Columns,(cols)
#define RowGroup(rows)    MUI_NewObject(MUIC_Group,MUIA_Group_Rows   ,(rows)
#define PageGroup         MUI_NewObject(MUIC_Group,MUIA_Group_PageMode,TRUE
#define VGroupV           MUI_NewObject(MUIC_Virtgroup
#define HGroupV           MUI_NewObject(MUIC_Virtgroup,MUIA_Group_Horiz,TRUE
#define ColGroupV(cols)   MUI_NewObject(MUIC_Virtgroup,MUIA_Group_Columns,(cols)
#define RowGroupV(rows)   MUI_NewObject(MUIC_Virtgroup,MUIA_Group_Rows   ,(rows)
#define PageGroupV        MUI_NewObject(MUIC_Virtgroup,MUIA_Group_PageMode,TRUE
#define RegisterGroup(t)  MUI_NewObject(MUIC_Register,MUIA_Register_Titles,(t)
#define End               TAG_DONE)

#define Child             MUIA_Group_Child
#define SubWindow         MUIA_Application_Window
#define WindowContents    MUIA_Window_RootObject



/***************************************************************************
**
** Frame Types
** -----------
**
** These macros may be used to specify one of MUI's different frame types.
** Note that every macro consists of one { ti_Tag, ti_Data } pair.
**
** GroupFrameT() is a special kind of frame that contains a centered
** title text.
**
** HGroup, GroupFrameT("Horiz Groups"),
**    Child, RectangleObject, TextFrame  , End,
**    Child, RectangleObject, StringFrame, End,
**    Child, RectangleObject, ButtonFrame, End,
**    Child, RectangleObject, ListFrame  , End,
**    End,
**
***************************************************************************/

#define NoFrame          MUIA_Frame, MUIV_Frame_None
#define ButtonFrame      MUIA_Frame, MUIV_Frame_Button
#define ImageButtonFrame MUIA_Frame, MUIV_Frame_ImageButton
#define TextFrame        MUIA_Frame, MUIV_Frame_Text
#define StringFrame      MUIA_Frame, MUIV_Frame_String
#define ReadListFrame    MUIA_Frame, MUIV_Frame_ReadList
#define InputListFrame   MUIA_Frame, MUIV_Frame_InputList
#define PropFrame        MUIA_Frame, MUIV_Frame_Prop
#define SliderFrame      MUIA_Frame, MUIV_Frame_Slider
#define GaugeFrame       MUIA_Frame, MUIV_Frame_Gauge
#define VirtualFrame     MUIA_Frame, MUIV_Frame_Virtual
#define GroupFrame       MUIA_Frame, MUIV_Frame_Group
#define GroupFrameT(s)   MUIA_Frame, MUIV_Frame_Group, MUIA_FrameTitle, s



/***************************************************************************
**
** Spacing Macros
** --------------
**
***************************************************************************/

#define HVSpace           MUI_NewObject(MUIC_Rectangle,TAG_DONE)
#define HSpace(x)         MUI_MakeObject(MUIO_HSpace,x)
#define VSpace(x)         MUI_MakeObject(MUIO_VSpace,x)
#define HCenter(obj)      (HGroup, GroupSpacing(0), Child, HSpace(0), Child, (obj), Child, HSpace(0), End)
#define VCenter(obj)      (VGroup, GroupSpacing(0), Child, VSpace(0), Child, (obj), Child, VSpace(0), End)
#define InnerSpacing(h,v) MUIA_InnerLeft,(h),MUIA_InnerRight,(h),MUIA_InnerTop,(v),MUIA_InnerBottom,(v)
#define GroupSpacing(x)   MUIA_Group_Spacing,x



#ifdef MUI_OBSOLETE

/***************************************************************************
**
** String-Object
** -------------
**
** The following macro creates a simple string gadget.
**
***************************************************************************/

#define String(contents,maxlen)\
	StringObject,\
		StringFrame,\
		MUIA_String_MaxLen  , maxlen,\
		MUIA_String_Contents, contents,\
		End

#define KeyString(contents,maxlen,controlchar)\
	StringObject,\
		StringFrame,\
		MUIA_ControlChar    , controlchar,\
		MUIA_String_MaxLen  , maxlen,\
		MUIA_String_Contents, contents,\
		End

#endif



#ifdef MUI_OBSOLETE

/***************************************************************************
**
** CheckMark-Object
** ----------------
**
** The following macro creates a checkmark gadget.
**
***************************************************************************/

#define CheckMark(selected)\
	ImageObject,\
		ImageButtonFrame,\
		MUIA_InputMode        , MUIV_InputMode_Toggle,\
		MUIA_Image_Spec       , MUII_CheckMark,\
		MUIA_Image_FreeVert   , TRUE,\
		MUIA_Selected         , selected,\
		MUIA_Background       , MUII_ButtonBack,\
		MUIA_ShowSelState     , FALSE,\
		End

#define KeyCheckMark(selected,control)\
	ImageObject,\
		ImageButtonFrame,\
		MUIA_InputMode        , MUIV_InputMode_Toggle,\
		MUIA_Image_Spec       , MUII_CheckMark,\
		MUIA_Image_FreeVert   , TRUE,\
		MUIA_Selected         , selected,\
		MUIA_Background       , MUII_ButtonBack,\
		MUIA_ShowSelState     , FALSE,\
		MUIA_ControlChar      , control,\
		End

#endif


/***************************************************************************
**
** Button-Objects
** --------------
**
** Note: Use small letters for KeyButtons, e.g.
**       KeyButton("Cancel",'c')  and not  KeyButton("Cancel",'C') !!
**
***************************************************************************/

#define SimpleButton(label) MUI_MakeObject(MUIO_Button,label)

#ifdef MUI_OBSOLETE

#define KeyButton(name,key)\
	TextObject,\
		ButtonFrame,\
		MUIA_Text_Contents, name,\
		MUIA_Text_PreParse, "\33c",\
		MUIA_Text_HiChar  , key,\
		MUIA_ControlChar  , key,\
		MUIA_InputMode    , MUIV_InputMode_RelVerify,\
		MUIA_Background   , MUII_ButtonBack,\
		End

#endif


#ifdef MUI_OBSOLETE

/***************************************************************************
**
** Cycle-Object
** ------------
**
***************************************************************************/

#define Cycle(entries)        CycleObject, MUIA_Cycle_Entries, entries, End
#define KeyCycle(entries,key) CycleObject, MUIA_Cycle_Entries, entries, MUIA_ControlChar, key, End



/***************************************************************************
**
** Radio-Object
** ------------
**
***************************************************************************/

#define Radio(name,array)\
	RadioObject,\
		GroupFrameT(name),\
		MUIA_Radio_Entries,array,\
		End

#define KeyRadio(name,array,key)\
	RadioObject,\
		GroupFrameT(name),\
		MUIA_Radio_Entries,array,\
		MUIA_ControlChar, key,\
		End



/***************************************************************************
**
** Slider-Object
** -------------
**
***************************************************************************/


#define Slider(min,max,level)\
	SliderObject,\
		MUIA_Slider_Min  , min,\
		MUIA_Slider_Max  , max,\
		MUIA_Slider_Level, level,\
		End

#define KeySlider(min,max,level,key)\
	SliderObject,\
		MUIA_Slider_Min  , min,\
		MUIA_Slider_Max  , max,\
		MUIA_Slider_Level, level,\
		MUIA_ControlChar , key,\
		End

#endif



/***************************************************************************
**
** Button to be used for popup objects
**
***************************************************************************/

#define PopButton(img) MUI_MakeObject(MUIO_PopButton,img)



/***************************************************************************
**
** Labeling Objects
** ----------------
**
** Labeling objects, e.g. a group of string gadgets,
**
**   Small: |foo   |
**  Normal: |bar   |
**     Big: |foobar|
**    Huge: |barfoo|
**
** is done using a 2 column group:
**
** ColGroup(2),
** 	Child, Label2("Small:" ),
**    Child, StringObject, End,
** 	Child, Label2("Normal:"),
**    Child, StringObject, End,
** 	Child, Label2("Big:"   ),
**    Child, StringObject, End,
** 	Child, Label2("Huge:"  ),
**    Child, StringObject, End,
**    End,
**
** Note that we have three versions of the label macro, depending on
** the frame type of the right hand object:
**
** Label1(): For use with standard frames (e.g. checkmarks).
** Label2(): For use with double high frames (e.g. string gadgets).
** Label() : For use with objects without a frame.
**
** These macros ensure that your label will look fine even if the
** user of your application configured some strange spacing values.
** If you want to use your own labeling, you'll have to pay attention
** on this topic yourself.
**
***************************************************************************/

#define Label(label)   MUI_MakeObject(MUIO_Label,label,0)
#define Label1(label)  MUI_MakeObject(MUIO_Label,label,MUIO_Label_SingleFrame)
#define Label2(label)  MUI_MakeObject(MUIO_Label,label,MUIO_Label_DoubleFrame)
#define LLabel(label)  MUI_MakeObject(MUIO_Label,label,MUIO_Label_LeftAligned)
#define LLabel1(label) MUI_MakeObject(MUIO_Label,label,MUIO_Label_LeftAligned|MUIO_Label_SingleFrame)
#define LLabel2(label) MUI_MakeObject(MUIO_Label,label,MUIO_Label_LeftAligned|MUIO_Label_DoubleFrame)

#define KeyLabel(label,key)   MUI_MakeObject(MUIO_Label,label,key)
#define KeyLabel1(label,key)  MUI_MakeObject(MUIO_Label,label,MUIO_Label_SingleFrame|(key))
#define KeyLabel2(label,key)  MUI_MakeObject(MUIO_Label,label,MUIO_Label_DoubleFrame|(key))
#define KeyLLabel(label,key)  MUI_MakeObject(MUIO_Label,label,MUIO_Label_LeftAligned|(key))
#define KeyLLabel1(label,key) MUI_MakeObject(MUIO_Label,label,MUIO_Label_LeftAligned|MUIO_Label_SingleFrame|(key))
#define KeyLLabel2(label,key) MUI_MakeObject(MUIO_Label,label,MUIO_Label_LeftAligned|MUIO_Label_DoubleFrame|(key))



/***************************************************************************
**
** Controlling Objects
** -------------------
**
** set() and get() are two short stubs for BOOPSI GetAttr() and SetAttrs()
** calls:
**
** {
**    char *x;
**
**    set(obj,MUIA_String_Contents,"foobar");
**    get(obj,MUIA_String_Contents,&x);
**
**    printf("gadget contains '%s'\n",x);
** }
**
** nnset() sets an attribute without triggering a possible notification.
**
***************************************************************************/

#define get(obj,attr,store) GetAttr(attr,obj,(ULONG *)store)
#define set(obj,attr,value) SetAttrs(obj,attr,value,TAG_DONE)
#define nnset(obj,attr,value) SetAttrs(obj,MUIA_NoNotify,TRUE,attr,value,TAG_DONE)

#define setmutex(obj,n)     set(obj,MUIA_Radio_Active,n)
#define setcycle(obj,n)     set(obj,MUIA_Cycle_Active,n)
#define setstring(obj,s)    set(obj,MUIA_String_Contents,s)
#define setcheckmark(obj,b) set(obj,MUIA_Selected,b)
#define setslider(obj,l)    set(obj,MUIA_Slider_Level,l)

#endif /* MUI_NOSHORTCUTS */


/***************************************************************************
**
** For Boopsi Image Implementors Only:
**
** If MUI is using a boopsi image object, it will send a special method
** immediately after object creation. This method has a parameter structure
** where the boopsi can fill in its minimum and maximum size and learn if
** its used in a horizontal or vertical context.
**
** The boopsi image must use the method id (MUIM_BoopsiQuery) as return
** value. That's how MUI sees that the method is implemented.
**
** Note: MUI does not depend on this method. If the boopsi image doesn't
**       implement it, minimum size will be 0 and maximum size unlimited.
**
***************************************************************************/

#define MUIM_BoopsiQuery 0x80427157 /* this is send to the boopsi and */
                                    /* must be used as return value   */

struct MUI_BoopsiQuery              /* parameter structure */
{
	ULONG mbq_MethodID;              /* always MUIM_BoopsiQuery */

	struct Screen *mbq_Screen;       /* obsolete, use mbq_RenderInfo */
	ULONG mbq_Flags;                 /* read only, see below */

	LONG mbq_MinWidth ;              /* write only, fill in min width  */
	LONG mbq_MinHeight;              /* write only, fill in min height */
	LONG mbq_MaxWidth ;              /* write only, fill in max width  */
	LONG mbq_MaxHeight;              /* write only, fill in max height */
	LONG mbq_DefWidth ;              /* write only, fill in def width  */
	LONG mbq_DefHeight;              /* write only, fill in def height */

	struct MUI_RenderInfo *mbq_RenderInfo;  /* read only, display context */

	/* may grow in future ... */
};

#define MUIP_BoopsiQuery MUI_BoopsiQuery  /* old structure name */

#define MBQF_HORIZ (1<<0)           /* object used in a horizontal */
                                    /* context (else vertical)     */

#define MBQ_MUI_MAXMAX (10000)          /* use this for unlimited MaxWidth/Height */


/****************************************************************************/
/** Notify                                                                 **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Notify[];
#else
#define MUIC_Notify "Notify.mui"
#endif

/* Methods */

#define MUIM_CallHook                  0x8042b96b /* V4 */
#define MUIM_FindUData                 0x8042c196 /* V8 */
#define MUIM_GetUData                  0x8042ed0c /* V8 */
#define MUIM_KillNotify                0x8042d240 /* V4 */
#define MUIM_MultiSet                  0x8042d356 /* V7 */
#define MUIM_NoNotifySet               0x8042216f /* V9 */
#define MUIM_Notify                    0x8042c9cb /* V4 */
#define MUIM_Set                       0x8042549a /* V4 */
#define MUIM_SetAsString               0x80422590 /* V4 */
#define MUIM_SetUData                  0x8042c920 /* V8 */
#define MUIM_WriteLong                 0x80428d86 /* V6 */
#define MUIM_WriteString               0x80424bf4 /* V6 */
struct  MUIP_CallHook                  { ULONG id; struct Hook *Hook; ULONG param1; /* ... */ };
struct  MUIP_FindUData                 { ULONG id; ULONG udata; };
struct  MUIP_GetUData                  { ULONG id; ULONG udata; ULONG attr; ULONG *storage; };
struct  MUIP_KillNotify                { ULONG id; ULONG TrigAttr; };
struct  MUIP_MultiSet                  { ULONG id; ULONG attr; ULONG val; APTR obj; /* ... */ };
struct  MUIP_NoNotifySet               { ULONG id; ULONG attr; char *format; ULONG val; /* ... */ };
struct  MUIP_Notify                    { ULONG id; ULONG TrigAttr; ULONG TrigVal; APTR DestObj; ULONG FollowParams; /* ... */ };
struct  MUIP_Set                       { ULONG id; ULONG attr; ULONG val; };
struct  MUIP_SetAsString               { ULONG id; ULONG attr; char *format; ULONG val; /* ... */ };
struct  MUIP_SetUData                  { ULONG id; ULONG udata; ULONG attr; ULONG val; };
struct  MUIP_WriteLong                 { ULONG id; ULONG val; ULONG *memory; };
struct  MUIP_WriteString               { ULONG id; char *str; char *memory; };

/* Attributes */

#define MUIA_AppMessage                 0x80421955 /* V5 ..g struct AppMessage * */
#ifdef MUI_OBSOLETE
#define MUIA_HelpFile                   0x80423a6e /* V4 isg STRPTR            */
#endif /* MUI_OBSOLETE */
#define MUIA_HelpLine                   0x8042a825 /* V4 isg LONG              */
#define MUIA_HelpNode                   0x80420b85 /* V4 isg STRPTR            */
#define MUIA_NoNotify                   0x804237f9 /* V7 .s. BOOL              */
#define MUIA_Revision                   0x80427eaa /* V4 ..g LONG              */
#define MUIA_UserData                   0x80420313 /* V4 isg ULONG             */
#define MUIA_Version                    0x80422301 /* V4 ..g LONG              */



/****************************************************************************/
/** Family                                                                 **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Family[];
#else
#define MUIC_Family "Family.mui"
#endif

/* Methods */

#define MUIM_Family_AddHead            0x8042e200 /* V8 */
#define MUIM_Family_AddTail            0x8042d752 /* V8 */
#define MUIM_Family_Insert             0x80424d34 /* V8 */
#define MUIM_Family_Remove             0x8042f8a9 /* V8 */
#define MUIM_Family_Sort               0x80421c49 /* V8 */
#define MUIM_Family_Transfer           0x8042c14a /* V8 */
struct  MUIP_Family_AddHead            { ULONG id; Object *obj; };
struct  MUIP_Family_AddTail            { ULONG id; Object *obj; };
struct  MUIP_Family_Insert             { ULONG id; Object *obj; Object *pred; };
struct  MUIP_Family_Remove             { ULONG id; Object *obj; };
struct  MUIP_Family_Sort               { ULONG id; Object *obj[1]; };
struct  MUIP_Family_Transfer           { ULONG id; Object *family; };

/* Attributes */

#define MUIA_Family_Child               0x8042c696 /* V8 i.. Object *          */



/****************************************************************************/
/** Menustrip                                                              **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Menustrip[];
#else
#define MUIC_Menustrip "Menustrip.mui"
#endif

/* Methods */


/* Attributes */

#define MUIA_Menustrip_Enabled          0x8042815b /* V8 isg BOOL              */



/****************************************************************************/
/** Menu                                                                   **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Menu[];
#else
#define MUIC_Menu "Menu.mui"
#endif

/* Methods */


/* Attributes */

#define MUIA_Menu_Enabled               0x8042ed48 /* V8 isg BOOL              */
#define MUIA_Menu_Title                 0x8042a0e3 /* V8 isg STRPTR            */



/****************************************************************************/
/** Menuitem                                                               **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Menuitem[];
#else
#define MUIC_Menuitem "Menuitem.mui"
#endif

/* Methods */


/* Attributes */

#define MUIA_Menuitem_Checked           0x8042562a /* V8 isg BOOL              */
#define MUIA_Menuitem_Checkit           0x80425ace /* V8 isg BOOL              */
#define MUIA_Menuitem_Enabled           0x8042ae0f /* V8 isg BOOL              */
#define MUIA_Menuitem_Exclude           0x80420bc6 /* V8 isg LONG              */
#define MUIA_Menuitem_Shortcut          0x80422030 /* V8 isg char              */
#define MUIA_Menuitem_Title             0x804218be /* V8 isg STRPTR            */
#define MUIA_Menuitem_Toggle            0x80424d5c /* V8 isg BOOL              */
#define MUIA_Menuitem_Trigger           0x80426f32 /* V8 ..g struct MenuItem * */



/****************************************************************************/
/** Application                                                            **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Application[];
#else
#define MUIC_Application "Application.mui"
#endif

/* Methods */

#ifdef MUI_OBSOLETE
#define MUIM_Application_GetMenuCheck  0x8042c0a7 /* V4 */
#endif /* MUI_OBSOLETE */
#ifdef MUI_OBSOLETE
#define MUIM_Application_GetMenuState  0x8042a58f /* V4 */
#endif /* MUI_OBSOLETE */
#define MUIM_Application_Input         0x8042d0f5 /* V4 */
#define MUIM_Application_InputBuffered 0x80427e59 /* V4 */
#define MUIM_Application_Load          0x8042f90d /* V4 */
#define MUIM_Application_PushMethod    0x80429ef8 /* V4 */
#define MUIM_Application_ReturnID      0x804276ef /* V4 */
#define MUIM_Application_Save          0x804227ef /* V4 */
#ifdef MUI_OBSOLETE
#define MUIM_Application_SetMenuCheck  0x8042a707 /* V4 */
#endif /* MUI_OBSOLETE */
#ifdef MUI_OBSOLETE
#define MUIM_Application_SetMenuState  0x80428bef /* V4 */
#endif /* MUI_OBSOLETE */
#define MUIM_Application_ShowHelp      0x80426479 /* V4 */
struct  MUIP_Application_GetMenuCheck  { ULONG id; ULONG MenuID; };
struct  MUIP_Application_GetMenuState  { ULONG id; ULONG MenuID; };
struct  MUIP_Application_Input         { ULONG id; LONGBITS *signal; };
struct  MUIP_Application_Load          { ULONG id; STRPTR name; };
struct  MUIP_Application_PushMethod    { ULONG id; Object *dest; LONG count; /* ... */ };
struct  MUIP_Application_ReturnID      { ULONG id; ULONG retid; };
struct  MUIP_Application_Save          { ULONG id; STRPTR name; };
struct  MUIP_Application_SetMenuCheck  { ULONG id; ULONG MenuID; LONG stat; };
struct  MUIP_Application_SetMenuState  { ULONG id; ULONG MenuID; LONG stat; };
struct  MUIP_Application_ShowHelp      { ULONG id; Object *window; char *name; char *node; LONG line; };

/* Attributes */

#define MUIA_Application_Active         0x804260ab /* V4 isg BOOL              */
#define MUIA_Application_Author         0x80424842 /* V4 i.g STRPTR            */
#define MUIA_Application_Base           0x8042e07a /* V4 i.g STRPTR            */
#define MUIA_Application_Broker         0x8042dbce /* V4 ..g Broker *          */
#define MUIA_Application_BrokerHook     0x80428f4b /* V4 isg struct Hook *     */
#define MUIA_Application_BrokerPort     0x8042e0ad /* V6 ..g struct MsgPort *  */
#define MUIA_Application_BrokerPri      0x8042c8d0 /* V6 i.g LONG              */
#define MUIA_Application_Commands       0x80428648 /* V4 isg struct MUI_Command * */
#define MUIA_Application_Copyright      0x8042ef4d /* V4 i.g STRPTR            */
#define MUIA_Application_Description    0x80421fc6 /* V4 i.g STRPTR            */
#define MUIA_Application_DiskObject     0x804235cb /* V4 isg struct DiskObject * */
#define MUIA_Application_DoubleStart    0x80423bc6 /* V4 ..g BOOL              */
#define MUIA_Application_DropObject     0x80421266 /* V5 is. Object *          */
#define MUIA_Application_ForceQuit      0x804257df /* V8 ..g BOOL              */
#define MUIA_Application_HelpFile       0x804293f4 /* V8 isg STRPTR            */
#define MUIA_Application_Iconified      0x8042a07f /* V4 .sg BOOL              */
#ifdef MUI_OBSOLETE
#define MUIA_Application_Menu           0x80420e1f /* V4 i.g struct NewMenu *  */
#endif /* MUI_OBSOLETE */
#define MUIA_Application_MenuAction     0x80428961 /* V4 ..g ULONG             */
#define MUIA_Application_MenuHelp       0x8042540b /* V4 ..g ULONG             */
#define MUIA_Application_Menustrip      0x804252d9 /* V8 i.. Object *          */
#define MUIA_Application_RexxHook       0x80427c42 /* V7 isg struct Hook *     */
#define MUIA_Application_RexxMsg        0x8042fd88 /* V4 ..g struct RxMsg *    */
#define MUIA_Application_RexxString     0x8042d711 /* V4 .s. STRPTR            */
#define MUIA_Application_SingleTask     0x8042a2c8 /* V4 i.. BOOL              */
#define MUIA_Application_Sleep          0x80425711 /* V4 .s. BOOL              */
#define MUIA_Application_Title          0x804281b8 /* V4 i.g STRPTR            */
#define MUIA_Application_Version        0x8042b33f /* V4 i.g STRPTR            */
#define MUIA_Application_Window         0x8042bfe0 /* V4 i.. Object *          */



/****************************************************************************/
/** Window                                                                 **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Window[];
#else
#define MUIC_Window "Window.mui"
#endif

/* Methods */

#ifdef MUI_OBSOLETE
#define MUIM_Window_GetMenuCheck       0x80420414 /* V4 */
#endif /* MUI_OBSOLETE */
#ifdef MUI_OBSOLETE
#define MUIM_Window_GetMenuState       0x80420d2f /* V4 */
#endif /* MUI_OBSOLETE */
#define MUIM_Window_ScreenToBack       0x8042913d /* V4 */
#define MUIM_Window_ScreenToFront      0x804227a4 /* V4 */
#define MUIM_Window_SetCycleChain      0x80426510 /* V4 */
#ifdef MUI_OBSOLETE
#define MUIM_Window_SetMenuCheck       0x80422243 /* V4 */
#endif /* MUI_OBSOLETE */
#ifdef MUI_OBSOLETE
#define MUIM_Window_SetMenuState       0x80422b5e /* V4 */
#endif /* MUI_OBSOLETE */
#define MUIM_Window_ToBack             0x8042152e /* V4 */
#define MUIM_Window_ToFront            0x8042554f /* V4 */
struct  MUIP_Window_GetMenuCheck       { ULONG id; ULONG MenuID; };
struct  MUIP_Window_GetMenuState       { ULONG id; ULONG MenuID; };
struct  MUIP_Window_SetCycleChain      { ULONG id; Object *obj[1]; };
struct  MUIP_Window_SetMenuCheck       { ULONG id; ULONG MenuID; LONG stat; };
struct  MUIP_Window_SetMenuState       { ULONG id; ULONG MenuID; LONG stat; };

/* Attributes */

#define MUIA_Window_Activate            0x80428d2f /* V4 isg BOOL              */
#define MUIA_Window_ActiveObject        0x80427925 /* V4 .sg Object *          */
#define MUIA_Window_AltHeight           0x8042cce3 /* V4 i.g LONG              */
#define MUIA_Window_AltLeftEdge         0x80422d65 /* V4 i.g LONG              */
#define MUIA_Window_AltTopEdge          0x8042e99b /* V4 i.g LONG              */
#define MUIA_Window_AltWidth            0x804260f4 /* V4 i.g LONG              */
#define MUIA_Window_AppWindow           0x804280cf /* V5 i.. BOOL              */
#define MUIA_Window_Backdrop            0x8042c0bb /* V4 i.. BOOL              */
#define MUIA_Window_Borderless          0x80429b79 /* V4 i.. BOOL              */
#define MUIA_Window_CloseGadget         0x8042a110 /* V4 i.. BOOL              */
#define MUIA_Window_CloseRequest        0x8042e86e /* V4 ..g BOOL              */
#define MUIA_Window_DefaultObject       0x804294d7 /* V4 isg Object *          */
#define MUIA_Window_DepthGadget         0x80421923 /* V4 i.. BOOL              */
#define MUIA_Window_DragBar             0x8042045d /* V4 i.. BOOL              */
#define MUIA_Window_FancyDrawing        0x8042bd0e /* V8 isg BOOL              */
#define MUIA_Window_Height              0x80425846 /* V4 i.g LONG              */
#define MUIA_Window_ID                  0x804201bd /* V4 isg ULONG             */
#define MUIA_Window_InputEvent          0x804247d8 /* V4 ..g struct InputEvent * */
#define MUIA_Window_LeftEdge            0x80426c65 /* V4 i.g LONG              */
#ifdef MUI_OBSOLETE
#define MUIA_Window_Menu                0x8042db94 /* V4 i.. struct NewMenu *  */
#endif /* MUI_OBSOLETE */
#define MUIA_Window_MenuAction          0x80427521 /* V8 isg ULONG             */
#define MUIA_Window_Menustrip           0x8042855e /* V8 i.. Object *          */
#define MUIA_Window_NoMenus             0x80429df5 /* V4 is. BOOL              */
#define MUIA_Window_Open                0x80428aa0 /* V4 .sg BOOL              */
#define MUIA_Window_PublicScreen        0x804278e4 /* V6 isg STRPTR            */
#define MUIA_Window_RefWindow           0x804201f4 /* V4 is. Object *          */
#define MUIA_Window_RootObject          0x8042cba5 /* V4 i.. Object *          */
#define MUIA_Window_Screen              0x8042df4f /* V4 isg struct Screen *   */
#define MUIA_Window_ScreenTitle         0x804234b0 /* V5 isg STRPTR            */
#define MUIA_Window_SizeGadget          0x8042e33d /* V4 i.. BOOL              */
#define MUIA_Window_SizeRight           0x80424780 /* V4 i.. BOOL              */
#define MUIA_Window_Sleep               0x8042e7db /* V4 .sg BOOL              */
#define MUIA_Window_Title               0x8042ad3d /* V4 isg STRPTR            */
#define MUIA_Window_TopEdge             0x80427c66 /* V4 i.g LONG              */
#define MUIA_Window_Width               0x8042dcae /* V4 i.g LONG              */
#define MUIA_Window_Window              0x80426a42 /* V4 ..g struct Window *   */

#define MUIV_Window_ActiveObject_None 0
#define MUIV_Window_ActiveObject_Next -1
#define MUIV_Window_ActiveObject_Prev -2
#define MUIV_Window_AltHeight_MinMax(p) (0-(p))
#define MUIV_Window_AltHeight_Visible(p) (-100-(p))
#define MUIV_Window_AltHeight_Screen(p) (-200-(p))
#define MUIV_Window_AltHeight_Scaled -1000
#define MUIV_Window_AltLeftEdge_Centered -1
#define MUIV_Window_AltLeftEdge_Moused -2
#define MUIV_Window_AltLeftEdge_NoChange -1000
#define MUIV_Window_AltTopEdge_Centered -1
#define MUIV_Window_AltTopEdge_Moused -2
#define MUIV_Window_AltTopEdge_Delta(p) (-3-(p))
#define MUIV_Window_AltTopEdge_NoChange -1000
#define MUIV_Window_AltWidth_MinMax(p) (0-(p))
#define MUIV_Window_AltWidth_Visible(p) (-100-(p))
#define MUIV_Window_AltWidth_Screen(p) (-200-(p))
#define MUIV_Window_AltWidth_Scaled -1000
#define MUIV_Window_Height_MinMax(p) (0-(p))
#define MUIV_Window_Height_Visible(p) (-100-(p))
#define MUIV_Window_Height_Screen(p) (-200-(p))
#define MUIV_Window_Height_Scaled -1000
#define MUIV_Window_Height_Default -1001
#define MUIV_Window_LeftEdge_Centered -1
#define MUIV_Window_LeftEdge_Moused -2
#ifdef MUI_OBSOLETE
#define MUIV_Window_Menu_NoMenu -1
#endif /* MUI_OBSOLETE */
#define MUIV_Window_TopEdge_Centered -1
#define MUIV_Window_TopEdge_Moused -2
#define MUIV_Window_TopEdge_Delta(p) (-3-(p))
#define MUIV_Window_Width_MinMax(p) (0-(p))
#define MUIV_Window_Width_Visible(p) (-100-(p))
#define MUIV_Window_Width_Screen(p) (-200-(p))
#define MUIV_Window_Width_Scaled -1000
#define MUIV_Window_Width_Default -1001


/****************************************************************************/
/** Area                                                                   **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Area[];
#else
#define MUIC_Area "Area.mui"
#endif

/* Methods */

#define MUIM_AskMinMax                 0x80423874 /* Custom Class */ /* V4 */
#define MUIM_Cleanup                   0x8042d985 /* Custom Class */ /* V4 */
#define MUIM_Draw                      0x80426f3f /* Custom Class */ /* V4 */
#define MUIM_HandleInput               0x80422a1a /* Custom Class */ /* V4 */
#define MUIM_Hide                      0x8042f20f /* Custom Class */ /* V4 */
#define MUIM_Setup                     0x80428354 /* Custom Class */ /* V4 */
#define MUIM_Show                      0x8042cc84 /* Custom Class */ /* V4 */
struct  MUIP_AskMinMax                 { ULONG id; struct MUI_MinMax *MinMaxInfo; }; /* Custom Class */
struct  MUIP_Draw                      { ULONG id; ULONG flags; }; /* Custom Class */
struct  MUIP_HandleInput               { ULONG id; struct IntuiMessage *imsg; LONG muikey; }; /* Custom Class */
struct  MUIP_Setup                     { ULONG id; struct MUI_RenderInfo *RenderInfo; }; /* Custom Class */

/* Attributes */

#define MUIA_ApplicationObject          0x8042d3ee /* V4 ..g Object *          */
#define MUIA_Background                 0x8042545b /* V4 is. LONG              */
#define MUIA_BottomEdge                 0x8042e552 /* V4 ..g LONG              */
#define MUIA_ControlChar                0x8042120b /* V4 i.. char              */
#define MUIA_Disabled                   0x80423661 /* V4 isg BOOL              */
#define MUIA_ExportID                   0x8042d76e /* V4 isg LONG              */
#define MUIA_FixHeight                  0x8042a92b /* V4 i.. LONG              */
#define MUIA_FixHeightTxt               0x804276f2 /* V4 i.. LONG              */
#define MUIA_FixWidth                   0x8042a3f1 /* V4 i.. LONG              */
#define MUIA_FixWidthTxt                0x8042d044 /* V4 i.. STRPTR            */
#define MUIA_Font                       0x8042be50 /* V4 i.g struct TextFont * */
#define MUIA_Frame                      0x8042ac64 /* V4 i.. LONG              */
#define MUIA_FramePhantomHoriz          0x8042ed76 /* V4 i.. BOOL              */
#define MUIA_FrameTitle                 0x8042d1c7 /* V4 i.. STRPTR            */
#define MUIA_Height                     0x80423237 /* V4 ..g LONG              */
#define MUIA_HorizWeight                0x80426db9 /* V4 i.. LONG              */
#define MUIA_InnerBottom                0x8042f2c0 /* V4 i.. LONG              */
#define MUIA_InnerLeft                  0x804228f8 /* V4 i.. LONG              */
#define MUIA_InnerRight                 0x804297ff /* V4 i.. LONG              */
#define MUIA_InnerTop                   0x80421eb6 /* V4 i.. LONG              */
#define MUIA_InputMode                  0x8042fb04 /* V4 i.. LONG              */
#define MUIA_LeftEdge                   0x8042bec6 /* V4 ..g LONG              */
#define MUIA_Pressed                    0x80423535 /* V4 ..g BOOL              */
#define MUIA_RightEdge                  0x8042ba82 /* V4 ..g LONG              */
#define MUIA_Selected                   0x8042654b /* V4 isg BOOL              */
#define MUIA_ShowMe                     0x80429ba8 /* V4 isg BOOL              */
#define MUIA_ShowSelState               0x8042caac /* V4 i.. BOOL              */
#define MUIA_Timer                      0x80426435 /* V4 ..g LONG              */
#define MUIA_TopEdge                    0x8042509b /* V4 ..g LONG              */
#define MUIA_VertWeight                 0x804298d0 /* V4 i.. LONG              */
#define MUIA_Weight                     0x80421d1f /* V4 i.. LONG              */
#define MUIA_Width                      0x8042b59c /* V4 ..g LONG              */
#define MUIA_Window                     0x80421591 /* V4 ..g struct Window *   */
#define MUIA_WindowObject               0x8042669e /* V4 ..g Object *          */

#define MUIV_Font_Inherit 0
#define MUIV_Font_Normal -1
#define MUIV_Font_List -2
#define MUIV_Font_Tiny -3
#define MUIV_Font_Fixed -4
#define MUIV_Font_Title -5
#define MUIV_Font_Big -6
#define MUIV_Frame_None 0
#define MUIV_Frame_Button 1
#define MUIV_Frame_ImageButton 2
#define MUIV_Frame_Text 3
#define MUIV_Frame_String 4
#define MUIV_Frame_ReadList 5
#define MUIV_Frame_InputList 6
#define MUIV_Frame_Prop 7
#define MUIV_Frame_Gauge 8
#define MUIV_Frame_Group 9
#define MUIV_Frame_PopUp 10
#define MUIV_Frame_Virtual 11
#define MUIV_Frame_Slider 12
#define MUIV_Frame_Count 13
#define MUIV_InputMode_None 0
#define MUIV_InputMode_RelVerify 1
#define MUIV_InputMode_Immediate 2
#define MUIV_InputMode_Toggle 3


/****************************************************************************/
/** Rectangle                                                              **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Rectangle[];
#else
#define MUIC_Rectangle "Rectangle.mui"
#endif

/* Attributes */

#define MUIA_Rectangle_HBar             0x8042c943 /* V7 i.g BOOL              */
#define MUIA_Rectangle_VBar             0x80422204 /* V7 i.g BOOL              */



/****************************************************************************/
/** Image                                                                  **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Image[];
#else
#define MUIC_Image "Image.mui"
#endif

/* Attributes */

#define MUIA_Image_FontMatch            0x8042815d /* V4 i.. BOOL              */
#define MUIA_Image_FontMatchHeight      0x80429f26 /* V4 i.. BOOL              */
#define MUIA_Image_FontMatchWidth       0x804239bf /* V4 i.. BOOL              */
#define MUIA_Image_FreeHoriz            0x8042da84 /* V4 i.. BOOL              */
#define MUIA_Image_FreeVert             0x8042ea28 /* V4 i.. BOOL              */
#define MUIA_Image_OldImage             0x80424f3d /* V4 i.. struct Image *    */
#define MUIA_Image_Spec                 0x804233d5 /* V4 i.. char *            */
#define MUIA_Image_State                0x8042a3ad /* V4 is. LONG              */



/****************************************************************************/
/** Bitmap                                                                 **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Bitmap[];
#else
#define MUIC_Bitmap "Bitmap.mui"
#endif

/* Attributes */

#define MUIA_Bitmap_Bitmap              0x804279bd /* V8 isg struct BitMap *   */
#define MUIA_Bitmap_Height              0x80421560 /* V8 isg LONG              */
#define MUIA_Bitmap_MappingTable        0x8042e23d /* V8 isg UBYTE *           */
#define MUIA_Bitmap_SourceColors        0x80425360 /* V8 isg ULONG *           */
#define MUIA_Bitmap_Transparent         0x80422805 /* V8 isg LONG              */
#define MUIA_Bitmap_Width               0x8042eb3a /* V8 isg LONG              */



/****************************************************************************/
/** Bodychunk                                                              **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Bodychunk[];
#else
#define MUIC_Bodychunk "Bodychunk.mui"
#endif

/* Attributes */

#define MUIA_Bodychunk_Body             0x8042ca67 /* V8 isg UBYTE *           */
#define MUIA_Bodychunk_Compression      0x8042de5f /* V8 isg UBYTE             */
#define MUIA_Bodychunk_Depth            0x8042c392 /* V8 isg LONG              */
#define MUIA_Bodychunk_Masking          0x80423b0e /* V8 isg UBYTE             */



/****************************************************************************/
/** Text                                                                   **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Text[];
#else
#define MUIC_Text "Text.mui"
#endif

/* Attributes */

#define MUIA_Text_Contents              0x8042f8dc /* V4 isg STRPTR            */
#define MUIA_Text_HiChar                0x804218ff /* V4 i.. char              */
#define MUIA_Text_PreParse              0x8042566d /* V4 isg STRPTR            */
#define MUIA_Text_SetMax                0x80424d0a /* V4 i.. BOOL              */
#define MUIA_Text_SetMin                0x80424e10 /* V4 i.. BOOL              */



/****************************************************************************/
/** String                                                                 **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_String[];
#else
#define MUIC_String "String.mui"
#endif

/* Attributes */

#define MUIA_String_Accept              0x8042e3e1 /* V4 isg STRPTR            */
#define MUIA_String_Acknowledge         0x8042026c /* V4 ..g STRPTR            */
#define MUIA_String_AttachedList        0x80420fd2 /* V4 i.. Object *          */
#define MUIA_String_BufferPos           0x80428b6c /* V4 .sg LONG              */
#define MUIA_String_Contents            0x80428ffd /* V4 isg STRPTR            */
#define MUIA_String_DisplayPos          0x8042ccbf /* V4 .sg LONG              */
#define MUIA_String_EditHook            0x80424c33 /* V7 isg struct Hook *     */
#define MUIA_String_Format              0x80427484 /* V4 i.g LONG              */
#define MUIA_String_Integer             0x80426e8a /* V4 isg ULONG             */
#define MUIA_String_MaxLen              0x80424984 /* V4 i.. LONG              */
#define MUIA_String_Reject              0x8042179c /* V4 isg STRPTR            */
#define MUIA_String_Secret              0x80428769 /* V4 i.g BOOL              */

#define MUIV_String_Format_Left 0
#define MUIV_String_Format_Center 1
#define MUIV_String_Format_Right 2


/****************************************************************************/
/** Prop                                                                   **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Prop[];
#else
#define MUIC_Prop "Prop.mui"
#endif

/* Attributes */

#define MUIA_Prop_Entries               0x8042fbdb /* V4 isg LONG              */
#define MUIA_Prop_First                 0x8042d4b2 /* V4 isg LONG              */
#define MUIA_Prop_Horiz                 0x8042f4f3 /* V4 i.g BOOL              */
#define MUIA_Prop_Slider                0x80429c3a /* V4 isg BOOL              */
#define MUIA_Prop_Visible               0x8042fea6 /* V4 isg LONG              */



/****************************************************************************/
/** Gauge                                                                  **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Gauge[];
#else
#define MUIC_Gauge "Gauge.mui"
#endif

/* Attributes */

#define MUIA_Gauge_Current              0x8042f0dd /* V4 isg LONG              */
#define MUIA_Gauge_Divide               0x8042d8df /* V4 isg BOOL              */
#define MUIA_Gauge_Horiz                0x804232dd /* V4 i.. BOOL              */
#define MUIA_Gauge_InfoText             0x8042bf15 /* V7 isg char *            */
#define MUIA_Gauge_Max                  0x8042bcdb /* V4 isg LONG              */



/****************************************************************************/
/** Scale                                                                  **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Scale[];
#else
#define MUIC_Scale "Scale.mui"
#endif

/* Attributes */

#define MUIA_Scale_Horiz                0x8042919a /* V4 isg BOOL              */



/****************************************************************************/
/** Boopsi                                                                 **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Boopsi[];
#else
#define MUIC_Boopsi "Boopsi.mui"
#endif

/* Attributes */

#define MUIA_Boopsi_Class               0x80426999 /* V4 isg struct IClass *   */
#define MUIA_Boopsi_ClassID             0x8042bfa3 /* V4 isg char *            */
#define MUIA_Boopsi_MaxHeight           0x8042757f /* V4 isg ULONG             */
#define MUIA_Boopsi_MaxWidth            0x8042bcb1 /* V4 isg ULONG             */
#define MUIA_Boopsi_MinHeight           0x80422c93 /* V4 isg ULONG             */
#define MUIA_Boopsi_MinWidth            0x80428fb2 /* V4 isg ULONG             */
#define MUIA_Boopsi_Object              0x80420178 /* V4 ..g Object *          */
#define MUIA_Boopsi_Remember            0x8042f4bd /* V4 i.. ULONG             */
#define MUIA_Boopsi_TagDrawInfo         0x8042bae7 /* V4 isg ULONG             */
#define MUIA_Boopsi_TagScreen           0x8042bc71 /* V4 isg ULONG             */
#define MUIA_Boopsi_TagWindow           0x8042e11d /* V4 isg ULONG             */



/****************************************************************************/
/** Colorfield                                                             **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Colorfield[];
#else
#define MUIC_Colorfield "Colorfield.mui"
#endif

/* Attributes */

#define MUIA_Colorfield_Blue            0x8042d3b0 /* V4 isg ULONG             */
#define MUIA_Colorfield_Green           0x80424466 /* V4 isg ULONG             */
#define MUIA_Colorfield_Pen             0x8042713a /* V4 ..g ULONG             */
#define MUIA_Colorfield_Red             0x804279f6 /* V4 isg ULONG             */
#define MUIA_Colorfield_RGB             0x8042677a /* V4 isg ULONG *           */



/****************************************************************************/
/** List                                                                   **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_List[];
#else
#define MUIC_List "List.mui"
#endif

/* Methods */

#define MUIM_List_Clear                0x8042ad89 /* V4 */
#define MUIM_List_Exchange             0x8042468c /* V4 */
#define MUIM_List_GetEntry             0x804280ec /* V4 */
#define MUIM_List_Insert               0x80426c87 /* V4 */
#define MUIM_List_InsertSingle         0x804254d5 /* V7 */
#define MUIM_List_Jump                 0x8042baab /* V4 */
#define MUIM_List_Move                 0x804253c2 /* V9 */
#define MUIM_List_NextSelected         0x80425f17 /* V6 */
#define MUIM_List_Redraw               0x80427993 /* V4 */
#define MUIM_List_Remove               0x8042647e /* V4 */
#define MUIM_List_Select               0x804252d8 /* V4 */
#define MUIM_List_Sort                 0x80422275 /* V4 */
struct  MUIP_List_Exchange             { ULONG id; LONG pos1; LONG pos2; };
struct  MUIP_List_GetEntry             { ULONG id; LONG pos; APTR *entry; };
struct  MUIP_List_Insert               { ULONG id; APTR *entries; LONG count; LONG pos; };
struct  MUIP_List_InsertSingle         { ULONG id; APTR entry; LONG pos; };
struct  MUIP_List_Jump                 { ULONG id; LONG pos; };
struct  MUIP_List_Move                 { ULONG id; LONG from; LONG to; };
struct  MUIP_List_NextSelected         { ULONG id; LONG *pos; };
struct  MUIP_List_Redraw               { ULONG id; LONG pos; };
struct  MUIP_List_Remove               { ULONG id; LONG pos; };
struct  MUIP_List_Select               { ULONG id; LONG pos; LONG seltype; LONG *state; };

/* Attributes */

#define MUIA_List_Active                0x8042391c /* V4 isg LONG              */
#define MUIA_List_AdjustHeight          0x8042850d /* V4 i.. BOOL              */
#define MUIA_List_AdjustWidth           0x8042354a /* V4 i.. BOOL              */
#define MUIA_List_CompareHook           0x80425c14 /* V4 is. struct Hook *     */
#define MUIA_List_ConstructHook         0x8042894f /* V4 is. struct Hook *     */
#define MUIA_List_DestructHook          0x804297ce /* V4 is. struct Hook *     */
#define MUIA_List_DisplayHook           0x8042b4d5 /* V4 is. struct Hook *     */
#define MUIA_List_Entries               0x80421654 /* V4 ..g LONG              */
#define MUIA_List_First                 0x804238d4 /* V4 ..g LONG              */
#define MUIA_List_Format                0x80423c0a /* V4 isg STRPTR            */
#define MUIA_List_InsertPosition        0x8042d0cd /* V9 ..g LONG              */
#define MUIA_List_MultiTestHook         0x8042c2c6 /* V4 is. struct Hook *     */
#define MUIA_List_Quiet                 0x8042d8c7 /* V4 .s. BOOL              */
#define MUIA_List_SourceArray           0x8042c0a0 /* V4 i.. APTR              */
#define MUIA_List_Title                 0x80423e66 /* V6 isg char *            */
#define MUIA_List_Visible               0x8042191f /* V4 ..g LONG              */

#define MUIV_List_Active_Off -1
#define MUIV_List_Active_Top -2
#define MUIV_List_Active_Bottom -3
#define MUIV_List_Active_Up -4
#define MUIV_List_Active_Down -5
#define MUIV_List_Active_PageUp -6
#define MUIV_List_Active_PageDown -7
#define MUIV_List_ConstructHook_String -1
#define MUIV_List_CopyHook_String -1
#define MUIV_List_CursorType_None 0
#define MUIV_List_CursorType_Bar 1
#define MUIV_List_CursorType_Rect 2
#define MUIV_List_DestructHook_String -1


/****************************************************************************/
/** Floattext                                                              **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Floattext[];
#else
#define MUIC_Floattext "Floattext.mui"
#endif

/* Attributes */

#define MUIA_Floattext_Justify          0x8042dc03 /* V4 isg BOOL              */
#define MUIA_Floattext_SkipChars        0x80425c7d /* V4 is. STRPTR            */
#define MUIA_Floattext_TabSize          0x80427d17 /* V4 is. LONG              */
#define MUIA_Floattext_Text             0x8042d16a /* V4 isg STRPTR            */



/****************************************************************************/
/** Volumelist                                                             **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Volumelist[];
#else
#define MUIC_Volumelist "Volumelist.mui"
#endif


/****************************************************************************/
/** Scrmodelist                                                            **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Scrmodelist[];
#else
#define MUIC_Scrmodelist "Scrmodelist.mui"
#endif

/* Attributes */




/****************************************************************************/
/** Dirlist                                                                **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Dirlist[];
#else
#define MUIC_Dirlist "Dirlist.mui"
#endif

/* Methods */

#define MUIM_Dirlist_ReRead            0x80422d71 /* V4 */

/* Attributes */

#define MUIA_Dirlist_AcceptPattern      0x8042760a /* V4 is. STRPTR            */
#define MUIA_Dirlist_Directory          0x8042ea41 /* V4 is. STRPTR            */
#define MUIA_Dirlist_DrawersOnly        0x8042b379 /* V4 is. BOOL              */
#define MUIA_Dirlist_FilesOnly          0x8042896a /* V4 is. BOOL              */
#define MUIA_Dirlist_FilterDrawers      0x80424ad2 /* V4 is. BOOL              */
#define MUIA_Dirlist_FilterHook         0x8042ae19 /* V4 is. struct Hook *     */
#define MUIA_Dirlist_MultiSelDirs       0x80428653 /* V6 is. BOOL              */
#define MUIA_Dirlist_NumBytes           0x80429e26 /* V4 ..g LONG              */
#define MUIA_Dirlist_NumDrawers         0x80429cb8 /* V4 ..g LONG              */
#define MUIA_Dirlist_NumFiles           0x8042a6f0 /* V4 ..g LONG              */
#define MUIA_Dirlist_Path               0x80426176 /* V4 ..g STRPTR            */
#define MUIA_Dirlist_RejectIcons        0x80424808 /* V4 is. BOOL              */
#define MUIA_Dirlist_RejectPattern      0x804259c7 /* V4 is. STRPTR            */
#define MUIA_Dirlist_SortDirs           0x8042bbb9 /* V4 is. LONG              */
#define MUIA_Dirlist_SortHighLow        0x80421896 /* V4 is. BOOL              */
#define MUIA_Dirlist_SortType           0x804228bc /* V4 is. LONG              */
#define MUIA_Dirlist_Status             0x804240de /* V4 ..g LONG              */

#define MUIV_Dirlist_SortDirs_First 0
#define MUIV_Dirlist_SortDirs_Last 1
#define MUIV_Dirlist_SortDirs_Mix 2
#define MUIV_Dirlist_SortType_Name 0
#define MUIV_Dirlist_SortType_Date 1
#define MUIV_Dirlist_SortType_Size 2
#define MUIV_Dirlist_Status_Invalid 0
#define MUIV_Dirlist_Status_Reading 1
#define MUIV_Dirlist_Status_Valid 2


/****************************************************************************/
/** Group                                                                  **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Group[];
#else
#define MUIC_Group "Group.mui"
#endif

/* Methods */


/* Attributes */

#define MUIA_Group_ActivePage           0x80424199 /* V5 isg LONG              */
#define MUIA_Group_Child                0x804226e6 /* V4 i.. Object *          */
#define MUIA_Group_Columns              0x8042f416 /* V4 is. LONG              */
#define MUIA_Group_Horiz                0x8042536b /* V4 i.. BOOL              */
#define MUIA_Group_HorizSpacing         0x8042c651 /* V4 is. LONG              */
#define MUIA_Group_PageMode             0x80421a5f /* V5 is. BOOL              */
#define MUIA_Group_Rows                 0x8042b68f /* V4 is. LONG              */
#define MUIA_Group_SameHeight           0x8042037e /* V4 i.. BOOL              */
#define MUIA_Group_SameSize             0x80420860 /* V4 i.. BOOL              */
#define MUIA_Group_SameWidth            0x8042b3ec /* V4 i.. BOOL              */
#define MUIA_Group_Spacing              0x8042866d /* V4 is. LONG              */
#define MUIA_Group_VertSpacing          0x8042e1bf /* V4 is. LONG              */

#define MUIV_Group_ActivePage_First 0
#define MUIV_Group_ActivePage_Last -1
#define MUIV_Group_ActivePage_Prev -2
#define MUIV_Group_ActivePage_Next -3


/****************************************************************************/
/** Register                                                               **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Register[];
#else
#define MUIC_Register "Register.mui"
#endif

/* Attributes */

#define MUIA_Register_Frame             0x8042349b /* V7 i.g BOOL              */
#define MUIA_Register_Titles            0x804297ec /* V7 i.g STRPTR *          */



/****************************************************************************/
/** Virtgroup                                                              **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Virtgroup[];
#else
#define MUIC_Virtgroup "Virtgroup.mui"
#endif

/* Methods */


/* Attributes */

#define MUIA_Virtgroup_Height           0x80423038 /* V6 ..g LONG              */
#define MUIA_Virtgroup_Left             0x80429371 /* V6 isg LONG              */
#define MUIA_Virtgroup_Top              0x80425200 /* V6 isg LONG              */
#define MUIA_Virtgroup_Width            0x80427c49 /* V6 ..g LONG              */



/****************************************************************************/
/** Scrollgroup                                                            **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Scrollgroup[];
#else
#define MUIC_Scrollgroup "Scrollgroup.mui"
#endif

/* Attributes */

#define MUIA_Scrollgroup_Contents       0x80421261 /* V4 i.. Object *          */
#define MUIA_Scrollgroup_FreeHoriz      0x804292f3 /* V9 i.. BOOL              */
#define MUIA_Scrollgroup_FreeVert       0x804224f2 /* V9 i.. BOOL              */



/****************************************************************************/
/** Scrollbar                                                              **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Scrollbar[];
#else
#define MUIC_Scrollbar "Scrollbar.mui"
#endif


/****************************************************************************/
/** Listview                                                               **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Listview[];
#else
#define MUIC_Listview "Listview.mui"
#endif

/* Attributes */

#define MUIA_Listview_ClickColumn       0x8042d1b3 /* V7 ..g LONG              */
#define MUIA_Listview_DefClickColumn    0x8042b296 /* V7 isg LONG              */
#define MUIA_Listview_DoubleClick       0x80424635 /* V4 i.g BOOL              */
#define MUIA_Listview_Input             0x8042682d /* V4 i.. BOOL              */
#define MUIA_Listview_List              0x8042bcce /* V4 i.. Object *          */
#define MUIA_Listview_MultiSelect       0x80427e08 /* V7 i.. LONG              */
#define MUIA_Listview_SelectChange      0x8042178f /* V4 ..g BOOL              */

#define MUIV_Listview_MultiSelect_None 0
#define MUIV_Listview_MultiSelect_Default 1
#define MUIV_Listview_MultiSelect_Shifted 2
#define MUIV_Listview_MultiSelect_Always 3


/****************************************************************************/
/** Radio                                                                  **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Radio[];
#else
#define MUIC_Radio "Radio.mui"
#endif

/* Attributes */

#define MUIA_Radio_Active               0x80429b41 /* V4 isg LONG              */
#define MUIA_Radio_Entries              0x8042b6a1 /* V4 i.. STRPTR *          */



/****************************************************************************/
/** Cycle                                                                  **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Cycle[];
#else
#define MUIC_Cycle "Cycle.mui"
#endif

/* Attributes */

#define MUIA_Cycle_Active               0x80421788 /* V4 isg LONG              */
#define MUIA_Cycle_Entries              0x80420629 /* V4 i.. STRPTR *          */

#define MUIV_Cycle_Active_Next -1
#define MUIV_Cycle_Active_Prev -2


/****************************************************************************/
/** Slider                                                                 **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Slider[];
#else
#define MUIC_Slider "Slider.mui"
#endif

/* Attributes */

#define MUIA_Slider_Level               0x8042ae3a /* V4 isg LONG              */
#define MUIA_Slider_Max                 0x8042d78a /* V4 isg LONG              */
#define MUIA_Slider_Min                 0x8042e404 /* V4 isg LONG              */
#define MUIA_Slider_Quiet               0x80420b26 /* V6 i.. BOOL              */
#define MUIA_Slider_Reverse             0x8042f2a0 /* V4 isg BOOL              */



/****************************************************************************/
/** Coloradjust                                                            **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Coloradjust[];
#else
#define MUIC_Coloradjust "Coloradjust.mui"
#endif

/* Attributes */

#define MUIA_Coloradjust_Blue           0x8042b8a3 /* V4 isg ULONG             */
#define MUIA_Coloradjust_Green          0x804285ab /* V4 isg ULONG             */
#define MUIA_Coloradjust_ModeID         0x8042ec59 /* V4 isg ULONG             */
#define MUIA_Coloradjust_Red            0x80420eaa /* V4 isg ULONG             */
#define MUIA_Coloradjust_RGB            0x8042f899 /* V4 isg ULONG *           */



/****************************************************************************/
/** Palette                                                                **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Palette[];
#else
#define MUIC_Palette "Palette.mui"
#endif

/* Attributes */

#define MUIA_Palette_Entries            0x8042a3d8 /* V6 i.g struct MUI_Palette_Entry * */
#define MUIA_Palette_Groupable          0x80423e67 /* V6 isg BOOL              */
#define MUIA_Palette_Names              0x8042c3a2 /* V6 isg char **           */



/****************************************************************************/
/** Colorpanel                                                             **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Colorpanel[];
#else
#define MUIC_Colorpanel "Colorpanel.mui"
#endif

/* Methods */


/* Attributes */




/****************************************************************************/
/** Popstring                                                              **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Popstring[];
#else
#define MUIC_Popstring "Popstring.mui"
#endif

/* Methods */

#define MUIM_Popstring_Close           0x8042dc52 /* V7 */
#define MUIM_Popstring_Open            0x804258ba /* V7 */
struct  MUIP_Popstring_Close           { ULONG id; LONG result; };

/* Attributes */

#define MUIA_Popstring_Button           0x8042d0b9 /* V7 i.g Object *          */
#define MUIA_Popstring_CloseHook        0x804256bf /* V7 isg struct Hook *     */
#define MUIA_Popstring_OpenHook         0x80429d00 /* V7 isg struct Hook *     */
#define MUIA_Popstring_String           0x804239ea /* V7 i.g Object *          */
#define MUIA_Popstring_Toggle           0x80422b7a /* V7 isg BOOL              */



/****************************************************************************/
/** Popobject                                                              **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Popobject[];
#else
#define MUIC_Popobject "Popobject.mui"
#endif

/* Attributes */

#define MUIA_Popobject_Follow           0x80424cb5 /* V7 isg BOOL              */
#define MUIA_Popobject_Light            0x8042a5a3 /* V7 isg BOOL              */
#define MUIA_Popobject_Object           0x804293e3 /* V7 i.g Object *          */
#define MUIA_Popobject_ObjStrHook       0x8042db44 /* V7 isg struct Hook *     */
#define MUIA_Popobject_StrObjHook       0x8042fbe1 /* V7 isg struct Hook *     */
#define MUIA_Popobject_Volatile         0x804252ec /* V7 isg BOOL              */
#define MUIA_Popobject_WindowHook       0x8042f194 /* V9 isg struct Hook *     */



/****************************************************************************/
/** Poplist                                                                **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Poplist[];
#else
#define MUIC_Poplist "Poplist.mui"
#endif

/* Attributes */

#define MUIA_Poplist_Array              0x8042084c /* V8 i.. char **           */



/****************************************************************************/
/** Popasl                                                                 **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Popasl[];
#else
#define MUIC_Popasl "Popasl.mui"
#endif

/* Attributes */

#define MUIA_Popasl_Active              0x80421b37 /* V7 ..g BOOL              */
#define MUIA_Popasl_StartHook           0x8042b703 /* V7 isg struct Hook *     */
#define MUIA_Popasl_StopHook            0x8042d8d2 /* V7 isg struct Hook *     */
#define MUIA_Popasl_Type                0x8042df3d /* V7 i.g ULONG             */






/*************************************************************************
** Structures and Macros for creating custom classes.
*************************************************************************/


/*
** GENERAL NOTES:
**
** - Everything described in this header file is only valid within
**   MUI classes. You may never use any of these things out of
**   a class, e.g. in a traditional MUI application.
**
** - Except when otherwise stated, all structures are strictly read only.
*/


/* Global information for every object */

struct MUI_GlobalInfo
{
	ULONG priv0;
	Object                   *mgi_ApplicationObject;

	/* ... private data follows ... */
};


/* Instance data of notify class */

struct MUI_NotifyData
{
	struct MUI_GlobalInfo *mnd_GlobalInfo;
	ULONG                  mnd_UserData;
	ULONG priv1;
	ULONG priv2;
	ULONG priv3;
	ULONG priv4;
	ULONG priv5;
};


/* MUI_MinMax structure holds information about minimum, maximum
   and default dimensions of an object. */

struct MUI_MinMax
{
	WORD MinWidth;
	WORD MinHeight;
	WORD MaxWidth;
	WORD MaxHeight;
	WORD DefWidth;
	WORD DefHeight;
};

#define MUI_MAXMAX 10000 /* use this if a dimension is not limited. */


/* (partial) instance data of area class */

struct MUI_AreaData
{
	struct MUI_RenderInfo *mad_RenderInfo;  /* RenderInfo for this object */
	ULONG priv6;
	struct TextFont       *mad_Font;        /* Font */
	struct MUI_MinMax      mad_MinMax;      /* min/max/default sizes */
	struct IBox            mad_Box;         /* position and dimension */
	BYTE                   mad_addleft;     /* frame & innerspacing left offset */
	BYTE                   mad_addtop;      /* frame & innerspacing top offset  */
	BYTE                   mad_subwidth;    /* frame & innerspacing add. width  */
	BYTE                   mad_subheight;   /* frame & innerspacing add. height */
	ULONG                  mad_Flags;       /* see definitions below */

	/* ... private data follows ... */
};

/* Definitions for mad_Flags, other flags are private */

#define MADF_DRAWOBJECT        (1<< 0) /* completely redraw yourself */
#define MADF_DRAWUPDATE        (1<< 1) /* only update yourself */




/* MUI's draw pens */

#define MPEN_SHINE      0
#define MPEN_HALFSHINE  1
#define MPEN_BACKGROUND 2
#define MPEN_HALFSHADOW 3
#define MPEN_SHADOW     4
#define MPEN_TEXT       5
#define MPEN_FILL       6
#define MPEN_ACTIVEOBJ  7
#define MPEN_COUNT      8


/* Information on display environment */

struct MUI_RenderInfo
{
	Object          *mri_WindowObject;  /* valid between MUIM_Setup/MUIM_Cleanup */

	struct Screen   *mri_Screen;        /* valid between MUIM_Setup/MUIM_Cleanup */
	struct DrawInfo *mri_DrawInfo;      /* valid between MUIM_Setup/MUIM_Cleanup */
	UWORD           *mri_Pens;          /* valid between MUIM_Setup/MUIM_Cleanup */
	struct Window   *mri_Window;        /* valid between MUIM_Show/MUIM_Hide */
	struct RastPort *mri_RastPort;      /* valid between MUIM_Show/MUIM_Hide */

	/* ... private data follows ... */
};



/* the following macros can be used to get pointers to an objects
   GlobalInfo and RenderInfo structures. */

struct __dummyXFC2__
{
	struct MUI_NotifyData mnd;
	struct MUI_AreaData   mad;
};

#define muiNotifyData(obj) (&(((struct __dummyXFC2__ *)(obj))->mnd))
#define muiAreaData(obj)   (&(((struct __dummyXFC2__ *)(obj))->mad))

#define muiGlobalInfo(obj) (((struct __dummyXFC2__ *)(obj))->mnd.mnd_GlobalInfo)
#define muiUserData(obj)   (((struct __dummyXFC2__ *)(obj))->mnd.mnd_UserData)
#define muiRenderInfo(obj) (((struct __dummyXFC2__ *)(obj))->mad.mad_RenderInfo)



/* User configurable keyboard events coming with MUIM_HandleInput */

enum
{
	MUIKEY_RELEASE = -2, /* not a real key, faked when MUIKEY_PRESS is released */
	MUIKEY_NONE    = -1,
	MUIKEY_PRESS,
	MUIKEY_TOGGLE,
	MUIKEY_UP,
	MUIKEY_DOWN,
	MUIKEY_PAGEUP,
	MUIKEY_PAGEDOWN,
	MUIKEY_TOP,
	MUIKEY_BOTTOM,
	MUIKEY_LEFT,
	MUIKEY_RIGHT,
	MUIKEY_WORDLEFT,
	MUIKEY_WORDRIGHT,
	MUIKEY_LINESTART,
	MUIKEY_LINEEND,
	MUIKEY_GADGET_NEXT,
	MUIKEY_GADGET_PREV,
	MUIKEY_GADGET_OFF,
	MUIKEY_WINDOW_CLOSE,
	MUIKEY_WINDOW_NEXT,
	MUIKEY_WINDOW_PREV,
	MUIKEY_HELP,
	MUIKEY_POPUP,
	MUIKEY_COUNT /* counter */
};


/* Some useful shortcuts. define MUI_NOSHORTCUTS to get rid of them */

#ifndef MUI_NOSHORTCUTS

#define _app(obj)         (muiGlobalInfo(obj)->mgi_ApplicationObject)
#define _win(obj)         (muiRenderInfo(obj)->mri_WindowObject)
#define _dri(obj)         (muiRenderInfo(obj)->mri_DrawInfo)
#define _window(obj)      (muiRenderInfo(obj)->mri_Window)
#define _screen(obj)      (muiRenderInfo(obj)->mri_Screen)
#define _rp(obj)          (muiRenderInfo(obj)->mri_RastPort)
#define _left(obj)        (muiAreaData(obj)->mad_Box.Left)
#define _top(obj)         (muiAreaData(obj)->mad_Box.Top)
#define _width(obj)       (muiAreaData(obj)->mad_Box.Width)
#define _height(obj)      (muiAreaData(obj)->mad_Box.Height)
#define _right(obj)       (_left(obj)+_width(obj)-1)
#define _bottom(obj)      (_top(obj)+_height(obj)-1)
#define _addleft(obj)     (muiAreaData(obj)->mad_addleft  )
#define _addtop(obj)      (muiAreaData(obj)->mad_addtop   )
#define _subwidth(obj)    (muiAreaData(obj)->mad_subwidth )
#define _subheight(obj)   (muiAreaData(obj)->mad_subheight)
#define _mleft(obj)       (_left(obj)+_addleft(obj))
#define _mtop(obj)        (_top(obj)+_addtop(obj))
#define _mwidth(obj)      (_width(obj)-_subwidth(obj))
#define _mheight(obj)     (_height(obj)-_subheight(obj))
#define _mright(obj)      (_mleft(obj)+_mwidth(obj)-1)
#define _mbottom(obj)     (_mtop(obj)+_mheight(obj)-1)
#define _font(obj)        (muiAreaData(obj)->mad_Font)
#define _flags(obj)       (muiAreaData(obj)->mad_Flags)

#endif



/* MUI_CustomClass returned by MUI_CreateCustomClass() */

struct MUI_CustomClass
{
	APTR mcc_UserData;                  /* use for whatever you want */

	struct Library *mcc_UtilityBase;    /* MUI has opened these libraries */
	struct Library *mcc_DOSBase;        /* for you automatically. You can */
	struct Library *mcc_GfxBase;        /* use them or decide to open     */
	struct Library *mcc_IntuitionBase;  /* your libraries yourself.       */

	struct IClass *mcc_Super;           /* pointer to super class   */
	struct IClass *mcc_Class;           /* pointer to the new class */

	/* ... private data follows ... */
};


#endif /* MUI_H */
