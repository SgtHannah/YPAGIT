	IFND RESOURCES_RESOURCES_I
RESOURCES_RESOURCES_I	SET 1
**
**  $Source: PRG:VFM/Include/resources/resources.i,v $
**  $Revision: 38.3 $
**  $Date: 1994/02/28 14:24:16 $
**  $Locker:  $
**  $Author: floh $
**
**  Assembler-Entsprechung zu resources/resources.h,
**  es sind nur die momentan benötigten Definitionen
**  enthalten.
**
**  (C) Copyright 1993 by A.Weissflog
**
	IFND EXEC_TYPES_I
	include "exec/types.i"
	ENDIF

	IFND TYPES_I
	include "types.i"
	ENDIF

**-----------------------------------------------------------------**
	STRUCTURE ResourceBitmap,0

	APTR RESBMP_RESNODE
	APTR RESBMP_DATA
	UWORD RESBMP_WIDTH
	UWORD RESBMP_HEIGHT
	ULONG RESBMP_COLORMODEL
	ULONG RESBMP_BYTESPERROW
	APTR RESBMP_LINEOFFSETS

	LABEL RESBMP_SIZE
**-----------------------------------------------------------------**
	STRUCTURE Resource3DPool,0

	APTR RES3D_RESNODE
	APTR RES3D_DATA
	ULONG RES3D_NUMELEMENTS

	LABEL RES3D_SIZE
**-----------------------------------------------------------------**
	STRUCTURE ResourceSample,0

	APTR RESSMP_RESNODE
	APTR RESSMP_DATA
	ULONG RESSMP_NUMBYTES

	LABEL RESSMP_SIZE
**-----------------------------------------------------------------**
	ENDC
