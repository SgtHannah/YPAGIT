/*
**  $Source$
**  $Revision$
**  $Date$
**  $Locker$
**  $Author$
**
**  winp_log.c -- Logging Routinen für windd.class und win3d.class
**
**  (C) Copyright 1998 by A.Weissflog
*/
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#define WINP_LOGFILE ("env/dinplog.txt")

/*-----------------------------------------------------------------*/
void winp_InitLog(void)
/*
**  CHANGED
**      13-May-98   floh    created
*/
{
    FILE *fp = fopen(WINP_LOGFILE,"w");
    if (fp) {
        fprintf(fp,"YPA DirectInput log\n\---------------------\n");
        fclose(fp);
    };
}

/*-----------------------------------------------------------------*/
void winp_Log(char *string,...)
/*
**  CHANGED
**      13-May-98   floh    created
*/
{
    FILE *fp = fopen(WINP_LOGFILE,"a");
    if (fp) {
        va_list arglist;
        va_start(arglist,string);
        vfprintf(fp,string,arglist);
        fclose(fp);
        va_end(arglist);
    };
}
