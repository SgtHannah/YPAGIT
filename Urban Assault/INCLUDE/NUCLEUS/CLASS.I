        IFND NUCLEUS_CLASS_I
NUCLEUS_CLASS_I SET 1
**
**      $Source: PRG:VFM/Include/nucleus/class.i,v $
**      $Revision: 38.5 $
**      $Date: 1994/12/31 21:51:01 $
**      $Locker:  $
**      $Author: floh $
**
**      Dies ist die Assembler-Entsprechung zu "nucleus/class.h".
**
**      07-Sep-93   floh    37.11   böser Bug wegen eingebetteter
**                                  STRUCT-Definitionen beseitigt
**                                  (STRUCTURE->STRUCT)
**      10-Sep-93   floh    37.12   INST_DATA-Macro eingeführt
**      21-Sep-93   floh            auf neuesten Stand gebracht
**      02-Oct-93   floh    38.1    RCS hat Revisionskontrolle übernommen.
**
**      (C) Copyright 1993 by A.Weissflog
**
        IFND EXEC_TYPES_I
        include "exec/types.i"
        ENDC

        IFND EXEC_NODES_I
        include "exec/nodes.i"
        ENDC

**--------------------------------------------------------------------**
NUCLEUS_NUMMETHODS      equ 1024

METHOD_DISTANCE         equ $40
ATTRIB_DISTANCE         equ $1000

**--------------------------------------------------------------------**
        STRUCTURE MethodArrayEntry,0
        APTR MAE_DISPATCHER
        APTR MAE_TRUECLASS
        LABEL MAE_SIZE              ; MUSS FÜR IMMER UND EWIG 8 BLEIBEN!!!

**--------------------------------------------------------------------**
        STRUCTURE Class,0
        STRUCT CL_NODE,LN_SIZE

        APTR CL_SUPERCLASS

        UWORD CL_INSTOFFSET
        UWORD CL_INSTSIZE

        APTR CL_USERDATA

        ULONG CL_CLASSCOUNT
        ULONG CL_SUBCLASSCOUNT
        ULONG CL_OBJECTCOUNT

        ULONG CL_FLAGS

        STRUCT CL_METHODS,MAE_SIZE

        APTR CL_SEG_HANDLE

        LABEL CL_SIZE
**--------------------------------------------------------------------**
**  Das INST_DATA-Macro verlangt als \1 den Pointer auf
**  die Klassen-Struktur (Adress-Register) und als \2 den
**  Pointer auf das Object (Adress-Register). Der Pointer auf
**  das Object wird dabei "zerstört", weil er nachher
**  zum Pointer auf die Locale Instance Data ist.
**
**  INPUTS
**      \1 (ax) -> Pointer auf Class
**      \2 (ax) -> Pointer auf Object
**
**  OUTPUTS
**      \2 -> zeigt auf Locale Instance Data
**
INST_DATA   macro
            add.w CL_INSTOFFSET(\1),\2
            endm

**--------------------------------------------------------------------**
NCTAG_DONE      equ -1
NCTAG_END       equ NCTAG_DONE
NCTAG_READY     equ NCTAG_DONE
**--------------------------------------------------------------------**

        ENDC

