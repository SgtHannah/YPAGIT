/*
**  $Source$
**  $Revision$
**  $Date$
**  $Locker$
**  $Author$
**
**  wdd_log.c -- Logging Routinen für windd.class und win3d.class
**
**  (C) Copyright 1998 by A.Weissflog
*/
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#define WINDD_LOGFILE ("env/d3dlog.txt")

/*-----------------------------------------------------------------*/
void wdd_InitLog(void)
/*
**  CHANGED
**      03-Mar-98   floh    created
*/
{
    FILE *fp = fopen(WINDD_LOGFILE,"w");
    if (fp) {
        fprintf(fp,"YPA DD/D3D driver log\n\---------------------\n");
        fclose(fp);
    };
}

/*-----------------------------------------------------------------*/
void wdd_Log(char *string,...)
/*
**  CHANGED
**      03-Mar-98   floh    created
*/
{
    FILE *fp = fopen(WINDD_LOGFILE,"a");
    if (fp) {
        va_list arglist;
        va_start(arglist,string);
        vfprintf(fp,string,arglist);
        fclose(fp);
        va_end(arglist);
    };
}




