/*** w3d_ebdp.c ***/
void w3d_BeginScene(struct windd_data *wdd, struct win3d_data *w3d);
void w3d_EndScene(struct windd_data *wdd, struct win3d_data *w3d);
void w3d_FlushPrimitives(struct windd_data *wdd, struct win3d_data *w3d);
void w3d_BeginRenderStates(struct windd_data *wdd, struct win3d_data *w3d);
void w3d_EndRenderStates(struct windd_data *wdd, struct win3d_data *w3d);
void w3d_RenderState(struct windd_data *wdd, struct win3d_data *w3d, D3DRENDERSTATETYPE r_type, DWORD r_state);
void w3d_BeginPrimitives(struct windd_data *wdd, struct win3d_data *w3d);
void w3d_EndPrimitives(struct windd_data *wdd, struct win3d_data *w3d);
void w3d_Primitive(struct windd_data *wdd, struct win3d_data *w3d, LPVOID v_array, DWORD v_num);
 
 
