/*** ypaworld.class prototypes ***/

/*** yw_mission.c ***/
ULONG yw_MissionMapParser(struct ScriptParser *);
BOOL yw_InitMissionBriefing(struct ypaworld_data *, ULONG);
void yw_KillMissionBriefing(struct ypaworld_data *);
void yw_TriggerMissionBriefing(struct ypaworld_data *,struct GameShellReq *,struct VFMInput *);
BOOL yw_MBActive(struct ypaworld_data *);
void yw_MBCancel(struct ypaworld_data *);
void yw_MBStart(struct ypaworld_data *);
void yw_MBContinue(struct ypaworld_data *);
void yw_MBPause(struct ypaworld_data *);
void yw_MBForward(struct ypaworld_data *);
void yw_MBRewind(struct ypaworld_data *);

/*** yw_radar.c ***/
void yw_RenderRadar(struct ypaworld_data *);

void yw_KillListView(struct ypaworld_data * , struct YPAListReq * );
void yw_ListSetRect(struct ypaworld_data * , struct YPAListReq * , short , short );
#ifdef AMIGA
BOOL __stdargs yw_InitListView(struct ypaworld_data * , struct YPAListReq * , unsigned long , ...);
#else
BOOL yw_InitListView(struct ypaworld_data * , struct YPAListReq * , unsigned long , ...);
#endif
void yw_ListHandleInput(struct ypaworld_data * , struct YPAListReq * , struct VFMInput * );
void yw_ListLayout(struct ypaworld_data * , struct YPAListReq * );
unsigned char * yw_LVItemsPreLayout(struct ypaworld_data * , struct YPAListReq * , unsigned char * , unsigned char , unsigned char * );
unsigned char * yw_LVItemsPostLayout(struct ypaworld_data * , struct YPAListReq * , unsigned char * , unsigned char , unsigned char * );

/*** yw_hud.c ***/
void yw_InitHUD(struct ypaworld_data *);
void yw_RenderHUD(struct ypaworld_data *);
void yw_KillHUD(struct ypaworld_data *);
void yw_VectorOutline(struct ypaworld_data *, struct Skeleton *, FLOAT,  FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, ULONG,
                      void (*local_color_hook)(struct ypaworld_data *ywd, FLOAT x0, FLOAT y0, FLOAT x1, FLOAT y1, ULONG *c0, ULONG *c1),
                      void (*global_color_hook)(struct ypaworld_data *ywd, FLOAT x0, FLOAT y0, FLOAT x1, FLOAT y1, ULONG *c0, ULONG *c1));
BOOL yw_RenderTechUpgrade(struct ypaworld_data *);
void yw_3DCursorOverBact(struct ypaworld_data *, struct Bacterium *);

/*** yw_sim.c ***/
void yw_ClearFootPrints(struct ypaworld_data *);
void yw_GetRealViewerPos(struct ypaworld_data *);
void yw_PutDebugInfo(struct ypaworld_data *, struct VFMInput *);

/*** yw_finder.c ***/
BOOL yw_InitFinder(unsigned long * , struct ypaworld_data * );
void yw_KillFinder(unsigned long * , struct ypaworld_data * );
void yw_LayoutFR(unsigned long * , struct ypaworld_data * );
void yw_HandleInputFR(struct ypaworld_data * , struct VFMInput * );
struct Bacterium * yw_FRGetBactUnderMouse(struct ypaworld_data *, long , long );

/*** yw_abort.c ***/
BOOL yw_InitAMR(struct ypaworld_data *);
void yw_KillAMR(struct ypaworld_data *);
void yw_HandleInputAMR(struct ypaworld_data *, struct VFMInput *);
void yw_OpenAMR(struct ypaworld_data *);
void yw_CloseAMR(struct ypaworld_data *);

/*** yw_logwin.c ***/
BOOL yw_InitLogWin(unsigned long * , struct ypaworld_data * );
void yw_KillLogWin(unsigned long * , struct ypaworld_data * );
void yw_HandleInputLW(struct ypaworld_data * , struct VFMInput * );
void yw_RenderQuickLog(struct ypaworld_data * );

/*** yw_fonts.c ***/
BOOL yw_LoadFontSet(struct ypaworld_data * );
void yw_KillFontSet(struct ypaworld_data * );

/*** yw_gui.c ***/
void yw_HandleGUIInput(struct ypaworld_data * , struct VFMInput * );
void yw_RenderRequesters(struct ypaworld_data * );
BOOL yw_InitGUIModule(unsigned long * , struct ypaworld_data * );
void yw_KillGUIModule(unsigned long * , struct ypaworld_data * );
void yw_LayoutGUI(unsigned long * , struct ypaworld_data * );
unsigned char * yw_BuildClippedItem(struct VFMFont * , unsigned char * , unsigned char * , long , unsigned char );
UBYTE *yw_BuildCenteredItem(struct VFMFont *, UBYTE *, UBYTE *, LONG, UBYTE);
UBYTE *yw_BuildReqTitle(struct ypaworld_data *, LONG, LONG, LONG, UBYTE *, UBYTE *, UBYTE, ULONG);
ULONG yw_StrLen(UBYTE * , struct VFMFont * );
UBYTE *yw_StpCpy(UBYTE *, UBYTE *);
UBYTE *yw_CenteredSkippedItem(struct VFMFont *, UBYTE *, UBYTE *, LONG);
UBYTE *yw_PutAlignedClippedString(struct ypaworld_data *, UBYTE *, struct ypa_ColumnItem *);
UBYTE *yw_BuildColumnItem(struct ypaworld_data *, UBYTE *, ULONG, struct ypa_ColumnItem *);

unsigned char * yw_TextBuildClippedItem(struct VFMFont * , unsigned char * , unsigned char * , long , unsigned char );
UBYTE *yw_TextBuildCenteredItem(struct VFMFont *, UBYTE *, UBYTE *, LONG, UBYTE);
UBYTE *yw_TextCenteredSkippedItem(struct VFMFont *, UBYTE *, UBYTE *, LONG);
UBYTE *yw_TextRelWidthItem(struct VFMFont *, UBYTE *, UBYTE *, LONG, ULONG);

/*** yw_statusbar.c ***/
BOOL yw_InitStatusReq(unsigned long * , struct ypaworld_data * );
void yw_KillStatusReq(struct ypaworld_data * );
void yw_StatusEnable(struct ypaworld_data * );
void yw_RenderStatusReq(struct ypaworld_data * );
void yw_HandleInputSR(struct ypaworld_data * , struct VFMInput * );
void yw_LayoutSR(struct ypaworld_data * );
void yw_RemapWeapons(struct ypaworld_data * );
void yw_RemapVehicles(struct ypaworld_data * );
void yw_RemapBuildings(struct ypaworld_data * );
void yw_RemapCommanders(struct ypaworld_data * );
UBYTE *yw_MenuLayoutItem(struct ypaworld_data *, struct YPAListReq *, UBYTE *, UBYTE *, UBYTE);
void yw_OpenReq(struct ypaworld_data *ywd, struct YPAReq *);
void yw_CloseReq(struct ypaworld_data *ywd, struct YPAReq *);
void yw_SRHandleVehicleSwitch(struct ypaworld_data *, struct Bacterium *, struct Bacterium *);

/*** yw_mapreq.c,yw_maprnd.c,yw_mapvhcl.c ***/
void yw_InitMapReq(unsigned long * , struct ypaworld_data * );
void yw_LayoutMR(unsigned long * , struct ypaworld_data * );
void yw_HandleInputMR(struct ypaworld_data *, struct VFMInput *);
void yw_RenderMap(struct ypaworld_data * );
void yw_RenderMapInterior(struct ypaworld_data * , unsigned char *, WORD, WORD );
void yw_RenderMapVehicles(struct ypaworld_data * );
void yw_RenderRadarVehicles(struct ypaworld_data *);
UBYTE *yw_GeneralMapInterior(struct ypaworld_data *, UBYTE *, WORD, WORD, WORD, WORD);
void yw_CheckMidPoint(struct ypaworld_data *, LONG , LONG);
UBYTE *yw_MRLayoutMapButtons(struct ypaworld_data *, UBYTE *);

/*** yw_attrs.c ***/
BOOL yw_initAttrs(unsigned long * , struct ypaworld_data * , struct TagItem * );
void yw_setAttrs(unsigned long * , struct ypaworld_data * , struct TagItem * );
void yw_getAttrs(unsigned long * , struct ypaworld_data * , struct TagItem * );

/*** yw_energy.c ***/
void yw_KillEnergyModule(struct ypaworld_data * );
BOOL yw_InitEnergyModule(struct ypaworld_data * );
long yw_AllocKraftWerk(struct ypaworld_data * , unsigned long , unsigned long , unsigned long );
void yw_FreeKraftWerk(struct ypaworld_data * , long );
long yw_FindKWbyCoords(struct ypaworld_data * , unsigned long , unsigned long );
long yw_FindKWbyPtr(struct ypaworld_data * , struct Cell * );
void yw_NewEnergyCycle(struct ypaworld_data * );
void yw_Energize(struct ypaworld_data * , unsigned long );
void yw_UpdateLego(struct ypaworld_data * , struct Cell * , unsigned short , unsigned short );
void yw_CheckSector(struct ypaworld_data *, struct Cell *, ULONG, ULONG, UBYTE, struct energymod_msg *);
void yw_ComputeRatios(struct ypaworld_data *);
void yw_SetOwner(struct ypaworld_data *, struct Cell *, ULONG, ULONG, UBYTE);

/*** yw_newinit.c ***/
void yw_KillSet(struct ypaworld_data * );
BOOL yw_LoadSet(struct ypaworld_data * , unsigned long );
void yw_KillVPSet(struct ypaworld_data *);
void yw_KillLegoSet(struct ypaworld_data *);
BOOL yw_LoadVPSet(struct ypaworld_data *, Object *);
BOOL yw_LoadLegoSet(struct ypaworld_data *, APTR, Object *);
BOOL yw_ReadSubSectorTypes(struct ypaworld_data *, APTR);
BOOL yw_ReadSectorTypes(struct ypaworld_data *, APTR);
BOOL yw_LoadSlurpSet(struct ypaworld_data *, Object *);
void yw_KillSlurpSet(struct ypaworld_data *);
void yw_InitProfiler(struct ypaworld_data *);
void yw_TriggerProfiler(struct ypaworld_data *);
void yw_LoadSISamples(struct SoundInit *);
void yw_KillSoundSamples(struct ypaworld_data *);
BOOL yw_CommonLevelInit(struct ypaworld_data *,struct LevelDesc *,ULONG,ULONG);
Object *yw_LoadSetObject(void);

/*** yw_parse.c ***/
BOOL yw_ParseProtoScript(struct ypaworld_data * , unsigned char * );
BOOL yw_ParseLDF(struct ypaworld_data * , struct LevelDesc * , unsigned char * );
ULONG yw_VhclProtoParser(struct ScriptParser *);
ULONG yw_BuildProtoParser(struct ScriptParser *);
BOOL yw_ParseExtSampleDef(struct SoundInit *, UBYTE *);
ULONG yw_ParseContextBlock(APTR, UBYTE *, ULONG, struct ScriptParser *, ULONG *, ULONG);
BOOL yw_ParseScript(UBYTE *, ULONG, struct ScriptParser *, ULONG);
BOOL yw_ParseLDFNetInfo(struct ypaworld_data *, struct LevelDesc *, UBYTE *);
BOOL yw_ParseLDFDebriefing(struct ypaworld_data *, struct LevelDesc *, UBYTE *);

/*** yw_render.c ***/
int __CDECL yw_cmp(struct pubstack_entry * , struct pubstack_entry * );
void yw_RenderFrame(unsigned long * , struct ypaworld_data * , struct trigger_msg * , BOOL);
void yw_RenderBeeBox(struct ypaworld_data * , long , long , struct sector_status * , struct basepublish_msg * );
void yw_RenderSector(struct ypaworld_data * , struct sector_status * , struct basepublish_msg * );
void yw_RenderHeaven(struct ypaworld_data * , struct basepublish_msg * );
unsigned long * yw_GetVSlurp(struct ypaworld_data * , struct sector_status * , struct sector_status * , float , float );
unsigned long * yw_GetHSlurp(struct ypaworld_data * , struct sector_status * , struct sector_status * , float , float );
void yw_RenderSlurps(struct ypaworld_data * , struct basepublish_msg * );

/*** yw_select.c ***/
BOOL yw_InitMouse(struct ypaworld_data * );
void yw_KillMouse(struct ypaworld_data * );
BOOL yw_SelBact2Cmdr(struct ypaworld_data * );
void yw_BuildTrLogicMsg(unsigned long *, struct ypaworld_data * , struct VFMInput * );

/*** aus yw_floodm68k.s bzw yw_floodx86.asm ***/
#ifdef AMIGA
__asm void yw_FloodFill(__a0 struct EMapElm *, __d0 ULONG, __d1 ULONG, __d2 ULONG);
#else
void yw_FloodFill(struct EMapElm *, ULONG, ULONG, ULONG);
#endif

/*** yw_design.c ***/
BOOL yw_InitPrototypeArrays(struct ypaworld_data * );
void yw_KillPrototypeArrays(struct ypaworld_data *);
void yw_CleanupPrototypeArrays(struct ypaworld_data *);

/*** yw_building.c ***/
void yw_SetSector(unsigned long * , struct ypaworld_data * , unsigned long , unsigned long , unsigned long , unsigned long, unsigned long );
ULONG yw_BuildProtoParser(struct ScriptParser *);

/*** yw_deathcache.c ***/
unsigned long * yw_PrivObtainBact(unsigned long * , struct ypaworld_data * , unsigned long );

/*** yw_jobs.c ***/
void yw_ResetBuildJobModule(struct ypaworld_data * );
BOOL yw_LockBuildJob(struct ypaworld_data * , unsigned long , unsigned long , long , long , unsigned long , unsigned long );
void yw_DoBuildJobs(unsigned long * , struct ypaworld_data * , unsigned long );

/*** yw_supp.c ***/
void yw_DoDigiStuff(struct ypaworld_data * , struct VFMInput * );
void yw_RenderVisor(struct ypaworld_data * );
void yw_FadeOut(struct ypaworld_data *);
void yw_FadeIn(struct ypaworld_data *);
void yw_BlackOut(struct ypaworld_data *);
void yw_ScreenShot(struct ypaworld_data *);
void yw_ShowDiskAccess(struct ypaworld_data *);
void yw_SaveBmpAsAscii(struct ypaworld_data *, Object *, UBYTE *, APTR);
Object *yw_CreateBmpFromAscii(struct ypaworld_data *, UBYTE *, APTR);
Object *yw_CopyBmpObject(Object *, UBYTE *);
void yw_BackToRoboNotify(struct ypaworld_data *);
ULONG yw_HandleGamePaused(struct ypaworld_data *, struct trigger_msg *);
ULONG yw_ParseColors(struct ScriptParser *);
ULONG yw_GetColor(struct ypaworld_data *, ULONG);
void yw_InitColors(struct ypaworld_data *);
void yw_GetTextTimerCallback(struct ypaworld_data *);
ULONG yw_ParseMovieData(struct ScriptParser *);
void yw_PlayMovie(struct ypaworld_data *, UBYTE *);
void yw_PlayIntroMovie(struct ypaworld_data *);
void yw_DbgKill(struct ypaworld_data *);
void yw_ClipFloatRect(struct rast_rect *);
ULONG yw_WorldMiscParser(struct ScriptParser *);
void yw_ParseAssignRegistryKeys(void);
struct Bacterium *yw_GetLastMessageSender(struct ypaworld_data *);
ULONG yw_CheckCD(ULONG, ULONG, char *, char *);

/*** yw_record.c ***/
void yw_FreeSequenceData(struct YPASeqData *);
struct YPASeqData *yw_AllocSequenceData(void);
BOOL yw_InitSceneRecorder(struct ypaworld_data *);
void yw_KillSceneRecorder(struct ypaworld_data *);
BOOL yw_RCNewScene(struct ypaworld_data *);
void yw_RCEndScene(struct ypaworld_data *);
void yw_RCNewFrame(struct ypaworld_data *, UWORD);
void yw_RCEndFrame(struct ypaworld_data *);

/*** yw_play.c ***/
void yw_RCSetPosition(struct ypaworld_data *, struct Bacterium *, struct flt_triple *);

/*** yw_locale.c ***/
BOOL yw_InitLocale(struct ypaworld_data *);
void yw_KillLocale(struct ypaworld_data *);
UBYTE *yw_LocStrCpy(UBYTE *, UBYTE *);

/*** yw_winlocale.c ***/
unsigned long yw_LoadLocaleDll(char *, char *, char *, char **, unsigned long);
void yw_UnloadLocaleDll(void);
unsigned long yw_ReadRegistryKeyString(char *, char *, long);
unsigned long yw_RawCDCheck(unsigned long);
unsigned long yw_RetryCancelMessageBox(char *, char *);

/*** yw_level.c ***/
BOOL yw_InitLevelNet(struct ypaworld_data *);
void yw_KillLevelNet(struct ypaworld_data *);
void yw_CreateBuddies(struct ypaworld_data *);
void yw_InitSquads(struct ypaworld_data *, ULONG , struct NLSquadDesc *);
void yw_InitGates(struct ypaworld_data *);
void yw_BeamGateCheck(struct ypaworld_data *);
void yw_DoLevelStatus(struct ypaworld_data *);

/*** yw_tooltip.c ***/
BOOL yw_InitTooltips(struct ypaworld_data *);
void yw_KillTooltips(struct ypaworld_data *);
void yw_Tooltip(struct ypaworld_data *, ULONG);
void yw_TooltipHotkey(struct ypaworld_data *, ULONG, ULONG);
void yw_RenderTooltip(struct ypaworld_data *);

#ifdef YPA_DESIGNMODE
void yw_RemapSectors(struct ypaworld_data * );
void yw_DesignSetSector(struct ypaworld_data * , struct Cell * , ULONG , ULONG , ULONG);
void yw_DesignSetOwner(struct ypaworld_data * , struct Cell * , ULONG , ULONG , ULONG);
void yw_DesignSetHeight(struct ypaworld_data * , struct Cell * , ULONG , ULONG , ULONG);
BOOL yw_DesignSaveMaps(struct ypaworld_data * );
BOOL yw_InitDesigner(struct ypaworld_data *);
void yw_KillDesigner(struct ypaworld_data *);
BOOL yw_DesignSaveProfile(struct ypaworld_data *ywd);
#endif

/*** yw_network.c ***/
BOOL yw_NetPlaceRobos( struct ypaworld_data *ywd, struct NLRoboDesc *robos, ULONG num_robos);
BOOL yw_InitNetwork( struct ypaworld_data *ywd );
void yw_KillNetwork( struct ypaworld_data *ywd );
void yw_CleanupNetworkSession( struct ypaworld_data *ywd );
void yw_HandleNetMessages( struct ypaworld_data *ywd );
void yw_NetMessageLoop( struct ypaworld_data *ywd );
void yw_RemovePlayerStuff( struct ypaworld_data *ywd, char *name, UBYTE owner, UBYTE mode );

/*** yw_netrequester.c ***/
BOOL yw_InitMW(struct ypaworld_data *);
void yw_KillMW(struct ypaworld_data *);
void yw_HandleInputMW(struct ypaworld_data *, struct VFMInput *);

/*** yw_netsupport.c ***/
void yw_GetNetGemProtos( struct ypaworld_data *, struct Wunderstein *, WORD *vproto, WORD *);
void yw_DisableNetWunderstein(struct ypaworld_data *, struct Cell *, ULONG );
void yw_ActivateNetWunderstein(struct ypaworld_data *, struct Cell *, ULONG );

/*** yw_force.c ***/
ULONG yw_InitForceFeedback(struct ypaworld_data *);
void yw_KillForceFeedback(struct ypaworld_data *);
void yw_FFVehicleChanged(struct ypaworld_data *);
void yw_FFTrigger(struct ypaworld_data *);

/*** yw_vehicle.c ***/
ULONG yw_VhclProtoParser(struct ScriptParser *);

/*** yw_weapon.c ***/
ULONG yw_WeaponProtoParser(struct ScriptParser *);

/*** yw_prldf.c ***/
ULONG yw_LevelDataParser(struct ScriptParser *);
ULONG yw_LevelRoboParser(struct ScriptParser *);
ULONG yw_LevelGateParser(struct ScriptParser *);
ULONG yw_LevelGemParser(struct ScriptParser *);
ULONG yw_LevelSquadParser(struct ScriptParser *);
ULONG yw_LevelEnableParser(struct ScriptParser *);
void yw_ParseGemAction(struct ypaworld_data *,ULONG,ULONG);
ULONG yw_MapParser(struct ScriptParser *);
ULONG yw_MapSizeOnlyParser(struct ScriptParser *);
BOOL yw_ScanLevels(struct ypaworld_data *);
ULONG yw_LevelSuperitemParser(struct ScriptParser *);

/*** yw_profile.c ***/
unsigned long yw_StartProfile(void);
unsigned long yw_EndProfile(unsigned long start_value);

/*** yw_analyze.c ***/
void yw_AnalyzeGameState(struct ypaworld_data *);

/*** yw_history.c ***/
BOOL yw_InitHistory(struct ypaworld_data *);
void yw_KillHistory(struct ypaworld_data *);
ULONG yw_HistoryParser(struct ScriptParser *);
BOOL yw_SaveHistory(struct ypaworld_data *, APTR);

/*** yw_debrief.c ***/
BOOL yw_InitDebriefing(struct ypaworld_data *);
void yw_KillDebriefing(struct ypaworld_data *);
void yw_TriggerDebriefing(struct ypaworld_data *,struct GameShellReq *,struct VFMInput *);
void yw_DBDoGlobalScore(struct ypaworld_data *);
ULONG yw_DebriefingMapParser(struct ScriptParser *);

/*** yw_voiceover.c ***/
ULONG yw_InitVoiceOverSystem(struct ypaworld_data *);
void yw_KillVoiceOverSystem(struct ypaworld_data *);
void yw_StartVoiceOver(struct ypaworld_data *, struct Bacterium *, LONG, ULONG);
void yw_TriggerVoiceOver(struct ypaworld_data *);

/*** yw_enbar.c ***/
BOOL yw_InitEB(struct ypaworld_data *);
void yw_KillEB(struct ypaworld_data *);
void yw_RenderEB(struct ypaworld_data *);
void yw_LayoutEB(struct ypaworld_data *);
void yw_HandleInputEB(struct ypaworld_data *, struct VFMInput *);

/*** yw_superitem.c ***/
void yw_InitSuperItems(struct ypaworld_data *);
void yw_TriggerSuperItems(struct ypaworld_data *);
void yw_RenderSuperItemStatus(struct ypaworld_data *);
ULONG yw_ParseSuperItemData(struct ScriptParser *);
void yw_RenderSuperItems(struct ypaworld_data *, struct basepublish_msg *);
void yw_InitActiveItem(struct ypaworld_data *, ULONG, BOOL);
void yw_InitTriggerItem(struct ypaworld_data *, ULONG);
void yw_InitFreezeItem(struct ypaworld_data *, ULONG);

/*** yw_bgshell.c ***/
BOOL yw_InitShellBgHandling(struct ypaworld_data *);
void yw_KillShellBgHandling(struct ypaworld_data *);
void yw_TriggerShellBg(struct ypaworld_data *, struct GameShellReq *, struct VFMInput *);

/*** yw_catch.c ***/
BOOL yw_InitEventCatcher(struct ypaworld_data *);
void yw_KillEventCatcher(struct ypaworld_data *);
void yw_TriggerEventCatcher(struct ypaworld_data *);
BOOL yw_SetEventLoop(struct ypaworld_data *, ULONG);

/*** yw_confirm.c ***/
BOOL yw_InitCR(struct ypaworld_data *);
void yw_KillCR(struct ypaworld_data *);
void yw_OpenCR(struct ypaworld_data *, UBYTE *); 
void yw_CloseCR(struct ypaworld_data *, BOOL);
void yw_HandleInputCR(struct ypaworld_data *, struct VFMInput *);
ULONG yw_CRGetStatus(struct ypaworld_data *);
ULONG yw_CRSetStatus(struct ypaworld_data *, ULONG);

