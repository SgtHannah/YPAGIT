	IFND TRANSFORM_CLIP3D_I
TRANSFORM_CLIP3D_I  SET 1
**
**  $Source: PRG:VFM/Include/transform/clip3d.i,v $
**  $Revision: 38.1 $
**  $Date: 1994/01/24 14:47:51 $
**  $Locker:  $
**  $Author: floh $
**
**  Ein paar (wenige) Definitionen für den 3D-Clipper.
**
**  (C) Copyright 1994 by A.Weissflog
**
	IFND EXEC_TYPES_I
	include 'exec/types.i'
	ENDC

**-----------------------------------------------------------------**
**  3D-ClipExtra-Flags, die angeben, welche Extra-Clip-Funktionen
**  von den 3D-Clipp-Funktionen erledigt werden sollen.
**
**  Eine Flag- und Bitnummer-Ausführung.
**
CLIPEXTRAB_XY		    equ 0	;clippe 2D-X- und 2D-Y-Koordinate
CLIPEXTRAB_BRIGHTNESS	    equ 1	;clippe Brightness-Wert im
					;Color-Tag-Entry (nur CM_LOWCOLOR)
CLIPEXTRAB_TRUECOLOR	    equ 2	;clippe True-Color-Wert im
					;Color-Tag-Entry (nur CM_TRUECOLOR)


CLIPEXTRAF_XY		    equ (1<<CLIPEXTRAB_XY)
CLIPEXTRAF_BRIGHTNESS	    equ (1<<CLIPEXTRAB_BRIGHTNESS)
CLIPEXTRAF_TRUECOLOR	    equ (1<<CLIPEXTRAB_TRUECOLOR)

**-----------------------------------------------------------------**
	ENDC


