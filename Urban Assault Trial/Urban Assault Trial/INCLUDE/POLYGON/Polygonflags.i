        IFND OV_ENGINE_I
OV_ENGINE_I SET 1
**
**  $Source: PRG:VFM/Include/polygon/polygonflags.i,v $
**  $Revision: 38.1 $
**  $Date: 1994/10/19 00:50:39 $
**  $Locker:  $
**  $Author: floh $
**
**  Polygonflags-Definition.
**
**  (C) Copyright 1994 by A.Weissflog
**
        IFND EXEC_TYPES_I
        include 'exec/types.i'
        ENDC

**-----------------------------------------------------------------**
**  Polygon-Basis-Flagbits
**
PLGB_SCANLINE       equ 0
PLGB_MAPBIT1        equ 1
PLGB_MAPBIT2        equ 2

PLGB_TXTBIT1        equ 3

PLGB_SHADEBIT1      equ 4
PLGB_SHADEBIT2      equ 5

PLGB_TRACYBIT1      equ 6
PLGB_TRACYBIT2      equ 7
PLGB_TRACYBIT3      equ 8

**-------------------------------------------------------------------
**  Polygon-Basis-Flags
**
PLGF_SCANLINE       equ (1<<PLGB_SCANLINE)
PLGF_MAPBIT1        equ (1<<PLGB_MAPBIT1)
PLGF_MAPBIT2        equ (1<<PLGB_MAPBIT2)

PLGF_TXTBIT1        equ (1<<PLGB_TXTBIT1)

PLGF_SHADEBIT1      equ (1<<PLGB_SHADEBIT1)
PLGF_SHADEBIT2      equ (1<<PLGB_SHADEBIT2)

PLGF_TRACYBIT1      equ (1<<PLGB_TRACYBIT1)
PLGF_TRACYBIT2      equ (1<<PLGB_TRACYBIT2)
PLGF_TRACYBIT3      equ (1<<PLGB_TRACYBIT3)

**-------------------------------------------------------------------
**  Polygon-Flagbit-Kombinationen [für DrawPolygon()-Aufruf benutzen!]
**
PLGF_HORIZONTAL     equ (PLGF_SCANLINE)
PLGF_VERTICAL       equ (0)

PLGF_NONMAPPED      equ 0
PLGF_LINEARMAPPED   equ (PLGF_MAPBIT1)
PLGF_DEPTHMAPPED    equ (PLGF_MAPBIT1|PLGF_MAPBIT2)

PLGF_NOTEXTURE      equ 0
PLGF_TEXTUREMAPPED  equ (PLGF_TXTBIT1)

PLGF_NOSHADE        equ 0
PLGF_FLATSHADE      equ (PLGF_SHADEBIT1)
PLGF_LINESHADE      equ (PLGF_SHADEBIT2)
PLGF_GRADSHADE      equ (PLGF_SHADEBIT1|PLGF_SHADEBIT2)

PLGF_NOTRACY        equ 0
PLGF_CLEARTRACY     equ (PLGF_TRACYBIT1)
PLGF_FLATTRACY      equ (PLGF_TRACYBIT2)
PLGF_TRACYMAPPED    equ (PLGF_TRACYBIT1|PLGF_TRACYBIT2)

PLGF_DARKEN         equ 0
PLGF_LIGHTEN        equ (PLGF_TRACYBIT3)

**-----------------------------------------------------------------**
        ENDC


