/*
**  $Source: PRG:VFM/Nucleus2/pc/tags.c,v $
**  $Revision: 38.2 $
**  $Date: 1996/01/23 18:06:56 $
**  $Locker:  $
**  $Author: floh $
**
**  TagItem-Funktionen für PC-Nucleus-Kernel.
**  [voll ANSI-kompatibel]
**
**  (C) Copyright 1994 by A.Weissflog
*/
#include <exec/types.h>
#include <utility/tagitem.h>

/*-----------------------------------------------------------------*/
struct TagItem *nc_FindTagItem(ULONG tagValue, struct TagItem *tagList)
/*
**  FUNCTION
**      Durchsucht eine TagList nach einem gegebenen Tag und
**      gibt einen Pointer darauf zurück, bzw. NULL, falls
**      die TagList dieses Tag nicht enthält.
**
**  INPUTS
**      tagValue    -> das zu suchende Tag
**      tagList     -> die zu scannende TagList
**
**  RESULTS
**      Pointer auf gefundenes TagItem, oder NULL, falls Suche
**      erfolglos war.
**
**  CHANGED
**      22-Dec-94   floh    created from scratch
**      20-Jan-96   floh    "long standing bug": die Spezial-Tags
**                          TAG_MORE und TAG_SKIP wurden falsch
**                          behandelt (je ein Tag wurde ausgelassen).
*/
{
    register ULONG act_tag;
    while ((act_tag = tagList->ti_Tag) != tagValue) {

        /* System-Tags... */
        switch (act_tag) {
            case TAG_DONE:  return(NULL);
                            break;
            case TAG_MORE:  tagList = (struct TagItem *) tagList->ti_Data;
                            break;
            case TAG_SKIP:  tagList += tagList->ti_Data;
                            break;
            default:        tagList++;
                            break;
        };
    };

    return(tagList);
};

/*-----------------------------------------------------------------*/
ULONG nc_GetTagData(ULONG tagValue, 
                    ULONG defaultVal, 
                    struct TagItem *tagList)
/*
**  FUNCTION
**      Durchsucht eine TagList nach einem gegebenen Tag
**      und gibt dessen Wert zurück, bzw. den Default-Wert,
**      falls das TagItem nicht in der TagList ist.
**
**  INPUTS
**      tagValue    -> das gesuchte Tag
**      defaultVal  -> wird zurückgegeben, falls Tag nicht vorhanden
**      tagList     -> Pointer auf die zu scannende TagList
**
**  RESULTS
**      Der ti_Data-Wert des gefundenen Tags, bzw. defaultVal,
**      falls das Tag nicht gefunden wurde.
**
**  CHANGED
**      22-Dec-94   floh    created
*/
{
    register struct TagItem *ti;
    if (ti = nc_FindTagItem(tagValue,tagList)) return(ti->ti_Data);
    else                                       return(defaultVal);
};

