#ifndef YPA_YPASTRATEGY_H
#define YPA_YPASTRATEGY_H
/*
**  $Source: PRG:VFM/Include/ypa/ypastrategy.h,v $
**  $Revision: 38.1 $
**  $Date: 1998/01/06 14:28:31 $
**  $Locker:  $
**  $Author: floh $
**
**  Definition der Strategien
**
**
**  (C) Copyright 1995 by Andreas Flemming
**  (Achtung! Draußen schneit's !!)
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif

#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif

#ifndef TRANSFORM_TFORM_H
#include "transform/tform.h"
#endif


/*-------------------------------------------------------------------
** Diverse Aggressivitätsstufen
*/

#define AGGR_NONE           0       // macht gar nüscht, kämpft nicht
#define AGGR_FIGHTPRIM      25      // bekämpft sein Hauptziel, vorher 10
#define AGGR_FIGHTSECBACT   50      // bekämpft sein Bact.Nebenziel
#define AGGR_FIGHTSECSEC    75      // bekämpft sein SectorNebenziel
#define AGGR_SECBACT        50      // akzeptiert Nebenziel (Bacterium)
#define AGGR_SECSEC         75      // akzeptiert Sektoren als Teilziele, vorher 75
#define AGGR_ALL            100     // bekämpft alles
#define AGGR_AI_STANDARD    60      // Standardaggr. der AI-geschwader


/*** Default-texte für Messages an den User ***/
#define DEF_LMSG_DONE                   "DONE. I AM WAITING"
#define DEF_LMSG_CONQUERED              "SECTOR CONQUERED"
#define DEF_LMSG_FIGHTSECTOR            "FIGHT SECTOR"
#define DEF_LMSG_REMSECTOR              "REMOVED SECTOR TARGET"
#define DEF_LMSG_ENEMYDESTROYED         "DESTROYED ENEMY COMMAND"
#define DEF_LMSG_FOUNDROBO              "FOUND ENEMY STATION"
#define DEF_LMSG_FOUNDENEMY             "FOUND ENEMY SQUAD"
#define DEF_LMSG_LASTDIED               "LAST MAN OF A COMMAND DIED"
#define DEF_LMSG_ESCAPE                 "SQUAD ESCAPES"
#define DEF_LMSG_RELAXED                "SQUAD RELAXED"
#define DEF_LMSG_ROBODEAD               "STATION IS DEAD "
#define DEF_LMSG_USERROBODEAD           "YOUR STATION IS DEAD!"
#define DEF_LMSG_USERROBODANGER         "ENEMY NEAR STATION!"
#define DEF_LMSG_ROBO                   "STATION"
#define DEF_LMSG_COMMANDER              "COMMANDER"
#define DEF_LMSG_REQUESTSUPPORT         "REQUEST SUPPORT"
#define DEF_LMSG_ENEMYESCAPES           "ENEMY ESCAPES"
#define DEF_LMSG_HOST_ENERGY_CRITICAL   "HOST ENERGY CRITICAL"
#define DEF_LMSG_FLAK_DESTROYED         "FLAK DESTROYED"
#define DEF_LMSG_POWER_DESTROYED        "POWER STATION DESTROYED"
#define DEF_LMSG_RADAR_DESTROYED        "RADAR STATION DESTROYED"
#define DEF_LMSG_VEHICLE_DESTROYED      "VEHICLE DESTROYED"
#define DEF_LMSG_POWER_ATTACK           "ATTACK POWERSTATION"
#define DEF_LMSG_FLAK_ATTACK            "ATTACK FLAK"
#define DEF_LMSG_RADAR_ATTACK           "ATTACK RADAR STATION"

#define DEF_LMSG_N                 "N"
#define DEF_LMSG_NNE               "NNE"
#define DEF_LMSG_NE                "NE"
#define DEF_LMSG_NEE               "NEE"
#define DEF_LMSG_E                 "E"
#define DEF_LMSG_S                 "S"
#define DEF_LMSG_SSE               "SSE"
#define DEF_LMSG_SE                "SE"
#define DEF_LMSG_SEE               "SEE"
#define DEF_LMSG_W                 "W"
#define DEF_LMSG_NWW               "NWW"
#define DEF_LMSG_NW                "NW"
#define DEF_LMSG_NNW               "NNW"
#define DEF_LMSG_SSW               "SSW"
#define DEF_LMSG_SW                "SW"
#define DEF_LMSG_SWW               "SWW"


#define NUM_LOGMESSAGES           100
/*-----------------------------------------------------------------*/
#endif


