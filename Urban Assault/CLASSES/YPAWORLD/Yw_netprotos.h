

/*** Netzwerkzeug ***/
void  yw_SendGameCopy( struct ypaworld_data *ywd, UBYTE owner, UBYTE *receiver_id );
BOOL  yw_RestoreShadows( struct ypaworld_data *ywd, struct ypamessage_update *upd,
                         UBYTE  owner );
struct OBNode *yw_GetRoboByOwner( struct ypaworld_data *ywd, UBYTE owner );
void   yw_RemoveAllShadows( struct ypaworld_data *ywd, struct OBNode *robo );
BOOL   yw_RestoreVehicleData( struct ypaworld_data *ywd );
BOOL   yw_DestroyPlayer( struct ypaworld_data *ywd, char *name );
void   yw_CleanupNetworkData( struct ypaworld_data *ywd );
void   yw_ParseSpecialLevelInfos( struct GameShellReq *GSR );
BOOL   yw_LaunchChatSample( struct ypaworld_data *ywd, WORD number );

void yw_HelpRoutine( struct ypaworld_data *ywd );
void yw_HelpInit( struct ypaworld_data *ywd );
void yw_SendChatMessage( struct ypaworld_data *ywd, char *text,
                         char *name, UBYTE owner);
void  yw_AddMessageToBuffer( struct ypaworld_data *ywd, char *sender, char *text );
ULONG yw_HandleThisMessage( struct ypaworld_data *ywd,
                            struct receivemessage_msg *rm,
                            UBYTE  *trouble_maker );
struct Bacterium *yw_GetBactByID( struct Bacterium *robo, ULONG ident );
struct Bacterium *yw_GetBactByID_3( struct Bacterium *robo, ULONG ident );
struct Bacterium *yw_GetBactByIDAndInfo( struct Bacterium *robo, ULONG ident,
                                         UBYTE kind, ULONG commander );
struct Bacterium *yw_GetMissileBactByRifleman( struct Bacterium *robo,
                                               ULONG ident, ULONG rifleman );
struct Bacterium *yw_GetMissileBactByID( struct Bacterium *robo, ULONG ident );
struct OBNode *yw_GetRoboByOwner( struct ypaworld_data *ywd, UBYTE owner );
BOOL yw_CollectVehicleData( struct ypaworld_data *ywd,
                            struct ypamessage_vehicledata_i *vdm );
BOOL yw_WasCreated( UBYTE owner, ULONG ident );
void yw_AddCreated( UBYTE owner, ULONG ident );
BOOL yw_WasDestroyed( UBYTE owner, ULONG ident );
void yw_AddDestroyed( UBYTE owner, ULONG ident );
BOOL yw_UserRobo( struct Bacterium *robo );
ULONG yw_GetVehicleNumber( struct Bacterium *robo );
void yw_ReleaseWeapon( struct ypaworld_data *ywd, struct Bacterium *bact );
void yw_RemoveAttacker( struct Bacterium *bact );
void yw_AddMessageToBuffer( struct ypaworld_data *ywd, char *sender, char *text );
void yw_ExtractVehicleData( struct ypaworld_data *ywd,
                            struct ypamessage_vehicledata_i *vdm,
                            struct OBNode *robo );
void yw_DrawNetworkStatusInfo( struct ypaworld_data *ywd );
void yw_SendAnnounceQuit( struct ypaworld_data *ywd );
void yw_PrintNetworkInfoStart( struct GameShellReq *GSR );
void yw_PrintNetworkInfoEnd( struct GameShellReq *GSR );
BOOL yw_NetLog( char* string, ... );
BOOL yw_OpenNetScript( void );
void yw_SendCheckSum( struct ypaworld_data *ywd, ULONG Levelnum );
void yw_TellAboutCheckSum( struct ypaworld_data *ywd );
void yw_DisConnectFromRobo( struct ypaworld_data *ywd, struct OBNode *vehicle );

