/*
**  $Source: $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:24:47 $
**  $Locker: $
**  $Author: $
**
**  Das Energiezeug für den Robo
**
**  (C) Copyright 1995 by Andreas Flemming
*/
#include <exec/types.h>

#include <math.h>
#include <stdlib.h>

#include "nucleus/nucleus2.h"
#include "engine/engine.h"
#include "transform/tform.h"
#include "ypa/ypaworldclass.h"
#include "ypa/ypabactclass.h"
#include "ypa/yparoboclass.h"
#include "input/input.h"


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine


#ifdef nondef

/*-----------------------------------------------------------**
**                                                           **
**  *** OBSOLETE *** OBSOLETE *** OBSOLETE *** OBSOLETE ***  **
**                                                           **
**-----------------------------------------------------------*/

_dispatcher(void, yr_YBM_GENERALENERGY, struct trigger_logic_msg *msg)
{

/*
**  FUNCTION    Neue Art der generalEnergy für den Robo. Hierbei kommt
**              eine Energie rein oder raus und dann gleichen sich alle
**              Batterien aus.
**
**              Normalerweise müßte der Fluß in der Summe 0 sein, aber
**              weil das, was fließen darf, manchmal mehr ist als das,
**              was zum Ausgleich noch fließen muß, mache ich kleine
**              fehler, die das Gesamtniveau verändern können.
**
**  INPUTS      Johnny Walker
**
**  CHANGED     created auch heute abend af
**      21-Apr-98   floh    Aufladung nicht mehr relativ zu Maximum,
**                          sondern zu RoboReloadConst
*/

    struct yparobo_data *yrd;
    LONG  multi, num_batt, common_energy, change, num_up, num_down, to_flow;
    FLOAT time, flow;
    struct getrldratio_msg gr;


    yrd = INST_DATA( cl, o);

    multi = yrd->bact->Sector->EnergyFactor;
    gr.owner = yrd->bact->Sector->Owner;
    _methoda( yrd->world, YWM_GETRLDRATIO, &gr );
    multi = (LONG)( ((FLOAT) multi) * gr.ratio );

    /*
    ** Wenn wir schon tot sind, dürfen wir uns nicht mehr aufladen. Das bringt
    ** die Energiebilanz durcheinander. 
    */
    
    if( yrd->bact->MainState == ACTION_DEAD ) return;

    /*** Auf grund von Rundungsfehlern muß time groß sein ***/
    if( (yrd->bact->internal_time - yrd->bact->time_energy) < 1500 ) return;

    time = (FLOAT) (yrd->bact->internal_time - yrd->bact->time_energy) / 1000.0;
    yrd->bact->time_energy = yrd->bact->internal_time;

    /* -----------------------------------------------------------
    ** autonomer Robo? Wie vehicle, aber mit Spareffekt in
    ** "Bausparbatterie". Diese Batterie dient nur dem Sparen, hat
    ** NICHTS mit UserBatterien zu tun!
    ** ---------------------------------------------------------*/
    if( !(yrd->RoboState & ROBO_USERROBO ) ) {

        LONG fluss;

        fluss = ((yrd->bact->RoboReloadConst * time * multi)/7000);

        if( yrd->bact->Owner == yrd->bact->Sector->Owner ) {

            /* -----------------------------------------
            ** NUR beim Aufladen verteilen wir auf beide
            ** Batterien
            ** ---------------------------------------*/
            yrd->BuildSpare   += (LONG)(0.15 * fluss);
            yrd->VehicleSpare += (LONG)(0.15 * fluss);
            yrd->bact->Energy += (LONG)(0.70 * fluss);
            }
        else
            yrd->bact->Energy -= fluss;

        if( yrd->bact->Energy < 0 )
            yrd->bact->Energy = 0;
        if( yrd->bact->Energy > yrd->bact->Maximum )
            yrd->bact->Energy = yrd->bact->Maximum;
        if( yrd->BuildSpare   > yrd->bact->Maximum )
            yrd->BuildSpare   = yrd->bact->Maximum;
        if( yrd->VehicleSpare > yrd->bact->Maximum )
            yrd->VehicleSpare = yrd->bact->Maximum;

        return;
        }



    /*** Wieviel Batterien müssen wir füllen? ***/
    num_batt      = 0;
    common_energy = 0;
    if( yrd->FillModus & YRF_Fill_System ) {
        num_batt++; common_energy += yrd->bact->Energy; }

    if( yrd->FillModus & YRF_Fill_Vehicle ) {
        num_batt++; common_energy += yrd->BattVehicle; }

    if( yrd->FillModus & YRF_Fill_Beam ) {
        num_batt++; common_energy += yrd->BattBeam; }

    /*** Wenn nix passiert, dann weg! ***/
    if( !num_batt ) return;

    /*** was fließt? ***/
    if( 0 == yrd->bact->Sector->Owner )
        flow = 0;
    else
        if( yrd->bact->Owner == yrd->bact->Sector->Owner )
            flow =  yrd->bact->RoboReloadConst * time * multi / 3000;
        else
            flow = -yrd->bact->RoboReloadConst * time * multi / 3000;

    /* -----------------------------------------------------------------
    ** Wir ermitteln das Gesamtenergieniveau, bilden dann den Mittelwert
    ** und gleichen soweit wie möglich an.
    ** ---------------------------------------------------------------*/

    common_energy += (LONG)flow;
    common_energy /= num_batt;

    /*** Change gibt an, was maximal fließen darf ***/
    change         = (LONG)(time * yrd->bact->RoboReloadConst / 60); // dann 1min zum Aufladen
    
    /* ---------------------------------------------------------------
    ** Was "nach oben" fließt, ist die gleiche Menge, die "nach unten"
    ** fließt. Das ist,weil wir ja schon den Mittelwert haben, die
    ** Hälfte aller Differenzen aus den energien und dem Mittelwert
    ** -------------------------------------------------------------*/
    to_flow = 0;
    num_up  = num_down = 0;

    if( yrd->FillModus & YRF_Fill_System ) {
        to_flow += abs( yrd->bact->Energy - common_energy );
        if( yrd->bact->Energy <= common_energy )
            num_up++;
        else
            num_down++;
        }

    if( yrd->FillModus & YRF_Fill_Vehicle ) {
        to_flow += abs( yrd->BattVehicle - common_energy );
        if( yrd->BattVehicle <= common_energy )
            num_up++;
        else
            num_down++;
        }

    if( yrd->FillModus & YRF_Fill_Beam ) {
        to_flow += abs( yrd->BattBeam - common_energy );
        if( yrd->BattBeam <= common_energy )
            num_up++;
        else
            num_down++;
        }

    to_flow /= 2;

    /*** Ausgleich notwendig ? ***/
    if( to_flow < 10 ) return;

    /*** Wenn die Ausgleichsmenge größer als der maximale Fluß ist, begrenzen ***/
    if( to_flow < change ) change = to_flow;

    /*** Ausgleich der Batterien ***/
    if( yrd->FillModus & YRF_Fill_System )
        if( yrd->bact->Energy <= common_energy ) {
            if( num_up )
                yrd->bact->Energy = min( yrd->bact->Energy + change / num_up,
                                         common_energy );
            }
        else {
            if( num_down )
                yrd->bact->Energy = max( yrd->bact->Energy - change / num_down,
                                         common_energy );
            }

    if( yrd->FillModus & YRF_Fill_Vehicle )
        if( yrd->BattVehicle <= common_energy ) {
            if( num_up )
                yrd->BattVehicle = min( yrd->BattVehicle + change / num_up,
                                        common_energy );
            }
        else {
            if( num_down )
                yrd->BattVehicle = max( yrd->BattVehicle - change / num_down,
                                        common_energy );
            }

    if( yrd->FillModus & YRF_Fill_Beam )
        if( yrd->BattBeam <= common_energy ) {
            if( num_up )
                yrd->BattBeam = min( yrd->BattBeam + change / num_up,
                                     common_energy );
            }
        else {
            if( num_down )
                yrd->BattBeam = max( yrd->BattBeam - change / num_down,
                                     common_energy );
            }


    /*** Die Energie der Bewegung ist sinnlos ***/
    
    /*** Begrenzungen ***/
    if( yrd->bact->Energy < 0 )   yrd->bact->Energy = 0;
    if( yrd->BattVehicle  < 0 )   yrd->BattVehicle  = 0;
    if( yrd->BattBeam     < 0 )   yrd->BattBeam     = 0;

    if( yrd->bact->Energy > yrd->bact->Maximum )
        yrd->bact->Energy = yrd->bact->Maximum;
    if( yrd->BattVehicle  > yrd->bact->Maximum )
        yrd->BattVehicle  = yrd->bact->Maximum;
    if( yrd->BattBeam     > yrd->bact->Maximum )
        yrd->BattBeam     = yrd->bact->Maximum;
}
#endif

/*-----------------------------------------------------------------*/
_dispatcher(void, yr_YBM_GENERALENERGY, struct trigger_logic_msg *msg)
/*
**  FUNCTION
**      Aufgeraeumte und upgedatete Version, alles auf
**      FLOAT umgestellt etc...
**
**  CHANGED
**      28-Apr-98   floh    created
**      29-Apr-98   floh    + funktioniert jetzt auch bei
**                            gesperrter System-Batterie
**                          + fuellt die Load/Loss Flags aus
*/
{
    struct yparobo_data *yrd = INST_DATA(cl,o);
    struct Bacterium *b = yrd->bact;
    FLOAT reload_const  = (FLOAT) b->RoboReloadConst;
    FLOAT td = ((FLOAT)(b->internal_time - b->time_energy)) / 1000.0;    
    
    /*** falls tot, oder Zeit noch nicht reif, nix machen ***/
    if ((b->MainState != ACTION_DEAD) && (td >= 0.25)) {
        FLOAT factor;        
        struct getrldratio_msg gr;
        LONG old_energy       = b->Energy;
        LONG old_batt_vehicle = yrd->BattVehicle;            
        LONG old_batt_beam    = yrd->BattBeam;            
            
        b->time_energy = b->internal_time;
        
        /*** berechne Multiplikator aus Energie-Faktor und ReloadRatio ***/
        gr.owner = b->Sector->Owner;
        _methoda(yrd->world, YWM_GETRLDRATIO, &gr);
        factor = ((FLOAT)b->Sector->EnergyFactor) * gr.ratio;

        /*** unterschiedliches Handling fuer User und autonome ***/
        if (!(yrd->RoboState & ROBO_USERROBO)) {

            /*** autonome Robos ***/
            FLOAT flow = (reload_const * td * factor) / 7000.0;
            if (b->Owner == b->Sector->Owner) {
                /*** bei Aufladung werden die "Baukonten" mit geladen ***/
                yrd->BuildSpare   += (LONG)(0.15 * flow);
                yrd->VehicleSpare += (LONG)(0.15 * flow);
                yrd->bact->Energy += (LONG)(0.70 * flow);
            } else {
                /*** bei Entladung wird nur die Hauptbatterie beeinflusst ***/
                yrd->bact->Energy -= (LONG)flow;
            };

        } else {
            
            /*** User-Robos sind etwas komplexer ***/
            LONG num_batt = 0; // Batterien, die aufgefuellt werden muessen
            FLOAT flow;
            
            /*** genereller Energie-Fluss ***/
            flow = (reload_const * td * factor) / 6000.0;                
            if (b->Sector->Owner == 0)             flow = 0.0;
            else if (b->Sector->Owner != b->Owner) flow = -flow;
            
            /*** wieviele Batterien muessen gefuellt werden? ***/  
            if ((yrd->FillModus & YRF_Fill_System) || (flow < 0.0))  num_batt++;
            if (yrd->FillModus & YRF_Fill_Vehicle) num_batt++; 
            if (yrd->FillModus & YRF_Fill_Beam)    num_batt++;
            if (num_batt > 0) {
            
                FLOAT max_balance_flow;
                LONG mid_energy = 0;            
                LONG up_flow, down_flow;            
                LONG num_up   = 0;
                LONG num_down = 0;            
            
                /*** Energie-Fluss pro aktiver Batterie ***/
                flow /= num_batt;
                
                /*** Batterien aufladen modifizieren ***/
                if ((yrd->FillModus & YRF_Fill_System) || (flow < 0.0))  b->Energy        += (LONG)flow;
                if (yrd->FillModus & YRF_Fill_Vehicle)                   yrd->BattVehicle += (LONG)flow;
                if (yrd->FillModus & YRF_Fill_Beam)                      yrd->BattBeam    += (LONG)flow;
                
                /*** BATTERIEN AUSGLEICHEN ***/
                max_balance_flow = (td * reload_const) / 30.0;
                
                /*** bilde Mittelwert aller Batterien ***/
                if (yrd->FillModus & YRF_Fill_System)  mid_energy += b->Energy;
                if (yrd->FillModus & YRF_Fill_Vehicle) mid_energy += yrd->BattVehicle;
                if (yrd->FillModus & YRF_Fill_Beam)    mid_energy += yrd->BattBeam;
                mid_energy /= num_batt;
                
                /*** wieviele Batterien laden sich auf, wieviele ab? ***/
                if (yrd->FillModus & YRF_Fill_System) {
                    if (b->Energy > mid_energy)      num_down++;
                    else if (b->Energy < mid_energy) num_up++;
                };
                if (yrd->FillModus & YRF_Fill_Vehicle) {
                    if (yrd->BattVehicle > mid_energy)      num_down++;
                    else if (yrd->BattVehicle < mid_energy) num_up++;
                };
                if (yrd->FillModus & YRF_Fill_Beam) {
                    if (yrd->BattBeam > mid_energy)      num_down++;
                    else if (yrd->BattBeam < mid_energy) num_up++;
                };
                
                /*** den Gesamt-Energie-Fluss auf die Auf-/Entladung verteilen ***/                
                if (num_up)   up_flow = ((LONG)max_balance_flow)/num_up;
                else          up_flow = 0;
                if (num_down) down_flow = ((LONG)max_balance_flow)/num_down;
                else          down_flow = 0;               
                
                /*** Batterien auf/entladen ***/
                if (yrd->FillModus & YRF_Fill_System) {
                    if (b->Energy > mid_energy) b->Energy = max(b->Energy-down_flow,mid_energy);
                    else                        b->Energy = min(b->Energy+up_flow,mid_energy);
                };
                if (yrd->FillModus & YRF_Fill_Vehicle) {
                    if (yrd->BattVehicle > mid_energy) yrd->BattVehicle = max(yrd->BattVehicle-down_flow,mid_energy);
                    else                               yrd->BattVehicle = min(yrd->BattVehicle+up_flow,mid_energy);
                };
                if (yrd->FillModus & YRF_Fill_Beam) {
                    if (yrd->BattBeam > mid_energy) yrd->BattBeam = max(yrd->BattBeam-down_flow,mid_energy);
                    else                            yrd->BattBeam = min(yrd->BattBeam+up_flow,mid_energy);
                };
            };                        
        };
        
        /*** Minimax Begrenzung ***/
        if (b->Energy < 0)                  b->Energy         = 0;
        if (yrd->BattVehicle  < 0)          yrd->BattVehicle  = 0;
        if (yrd->BattBeam     < 0)          yrd->BattBeam     = 0;
        if (yrd->BuildSpare   < 0)          yrd->BuildSpare   = 0;
        if (yrd->VehicleSpare < 0)          yrd->VehicleSpare = 0;
        if (b->Energy > b->Maximum)         b->Energy         = b->Maximum;
        if (yrd->BattVehicle  > b->Maximum) yrd->BattVehicle  = b->Maximum;
        if (yrd->BattBeam     > b->Maximum) yrd->BattBeam     = b->Maximum;
        if (yrd->BuildSpare   > b->Maximum) yrd->BuildSpare   = b->Maximum;
        if (yrd->VehicleSpare > b->Maximum) yrd->VehicleSpare = b->Maximum;
        
        /*** Load/Loss Flags ***/
        yrd->LoadFlags = 0;
        yrd->LossFlags = 0;
        if (old_energy > b->Energy)                     yrd->LossFlags |= YRF_Fill_System;
        else if (old_energy < b->Energy)                yrd->LoadFlags |= YRF_Fill_System;
        if (old_batt_vehicle > yrd->BattVehicle)        yrd->LossFlags |= YRF_Fill_Vehicle;
        else if (old_batt_vehicle < yrd->BattVehicle)   yrd->LoadFlags |= YRF_Fill_Vehicle;
        if (old_batt_beam > yrd->BattBeam)              yrd->LossFlags |= YRF_Fill_Beam;
        else if (old_batt_beam < yrd->BattBeam)         yrd->LoadFlags |= YRF_Fill_Beam; 
     };
} 
      
                                
    
        
      

