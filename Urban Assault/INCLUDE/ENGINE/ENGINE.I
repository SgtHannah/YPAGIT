        IFND ENGINE_ENGINE_I
ENGINE_ENGINE_I     SET 1
**
**  $Source: PRG:VFM/Include/engine/engine.i,v $
**  $Revision: 38.17 $
**  $Date: 1994/10/19 00:50:57 $
**  $Locker:  $
**  $Author: floh $
**
**  Hier sind die Global Entry Tables aller dynamisch linkbarer
**  Module für den Zugriff von Assembler aus definiert.
**  Für nähere Informationen was die Register-Belegung der
**  Eingabe-Parameter angeht, bitte in "engine/engine.h" nachgucken.
**
**  (C) Copyright 1993 by A.Weissflog
**
        IFND EXEC_TYPES_I
        include 'exec/types.i'
        ENDC

**-----------------------------------------------------------------**
        STRUCTURE GET_ENGINE,0
        APTR ENGINE_OPEN
        APTR ENGINE_CLOSE
        APTR ENGINE_REQUESTDOWNSCALE
        APTR ENGINE_REPLYREQUEST
        APTR ENGINE_OPENENGINE
        APTR ENGINE_CLOSEENGINE
        APTR ENGINE_SETATTRIBUTES
        APTR ENGINE_GETSPECIFICGET
**-----------------------------------------------------------------**
        STRUCTURE OVE_GET_SPECIFIC,0
        APTR OVE_SPEC_SETDISPLAY
        APTR OVE_SPEC_LOADDISPLAY
        APTR OVE_SPEC_WAITTOF
        APTR OVE_SPEC_TOGGLEDISPLAYS
        APTR OVE_SPEC_CLEARDISPLAY
        APTR OVE_SPEC_LOADCOLORMAP
        APTR OVE_SPEC_DRAWBITMAP
        APTR OVE_SPEC_DRAWSCALEDBITMAP
        APTR OVE_SPEC_SETBLURMODE
        APTR OVE_SPEC_DRAWPOLYGON
**-----------------------------------------------------------------**
        STRUCTURE TE_GET_SPECIFIC,0
        APTR TE_SPEC_SETZOOM            /* OBSOLETE */
        APTR TE_SPEC_SETMAXZ            /* OBSOLETE */
        APTR TE_SPEC_SETVIEWERMATRIX
        APTR TE_SPEC_LOCALTOGLOBAL
        APTR TE_SPEC_GLOBALTOVIEWER
        APTR TE_SPEC_PROJECTTODISPLAY
        APTR TE_SPEC_GET3DCLIPCODES
        APTR TE_SPEC_CLIP3DLINEXZ       /* OBSOLETE */
        APTR TE_SPEC_CLIP3DLINEYZ       /* OBSOLETE */
        APTR TE_SPEC_CLIP3DPOLYGON
        APTR TE_SPEC_GETVIEWERORIENTATION
        APTR TE_SPEC_BACKCHECK2DPOLYGON
        APTR TE_SPEC_ALLOCIP3DPOOL
        APTR TE_SPEC_FREEIP3DPOOL
        APTR TE_SPEC_PIXEL3DTOIP3D
        APTR TE_SPEC_IP3DTOPIXEL3D
        APTR TE_SPEC_PIXEL3DTOIP3DPOOL
        APTR TE_SPEC_IP3DTOPIXEL3DPOOL
        APTR TE_SPEC_ISOLATE3DPOLYGON
        APTR TE_SPEC_POLYGONPIPELINE
**-----------------------------------------------------------------**
        STRUCTURE IE_GET_SPECIFIC,0
        APTR IE_SPEC_DEADENDMODE
        APTR IE_SPEC_GETINPUT
        APTR IE_SPEC_RESETABSOLUTE2D
**-----------------------------------------------------------------**
        ENDC
