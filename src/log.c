/* $Header: /home/foxen/CR/FB/src/RCS/log.c,v 1.1 1996/06/12 02:41:45 foxen Exp $ */

/*
 * $Log: log.c,v $
 * Revision 1.1  1996/06/12 02:41:45  foxen
 * Initial revision
 *
 * Revision 5.3  1994/03/14  12:20:58  foxen
 * Fb5.20 release checkpoint.
 *
 * Revision 5.2  1994/01/18  20:52:20  foxen
 * Version 5.15 release.
 *
 * Revision 5.1  1993/12/17  00:07:33  foxen
 * initial revision.
 *
 * Revision 1.5  90/08/02  18:56:08  rearl
 * Fixed NULL file pointer bugs.  Netmuck no longer crashes on a non-existent
 * file / missing directory.  All errors / commands are logged to stderr
 * if a file error occurs.
 *
 * Revision 1.4  90/07/29  17:38:55  rearl
 * Fixed double spaces in log files.
 *
 * Revision 1.3  90/07/23  14:52:17  casie
 * *** empty log message ***
 *
 * Revision 1.2  90/07/21  01:42:22  casie
 * good question... lots of changes, in fact, a complete rewrite!
 *
 * Revision 1.1  90/07/19  23:03:51  casie
 * Initial revision
 *
 */

/*
 * Handles logging of tinymuck
 *
 * Copyright (c) 1990 Chelsea Dyerman
 * University of California, Berkeley (XCF)
 *
 */

#include "config.h"

#include <stdio.h>
#include <sys/types.h>
#include <stdarg.h>
#include <time.h>

#include "strings.h"
#include "externs.h"
#include "interface.h"

/* cks: these are varargs routines. We are assuming ANSI C. We could at least
   USE ANSI C varargs features, no? Sigh. */

void 
log2file(char *myfilename, char *format,...)
{
    va_list args;
    FILE   *fp;

    va_start(args, format);

    if ((fp = fopen(myfilename, "a")) == NULL) {
	fprintf(stderr, "Unable to open %s!\n", myfilename);
	vfprintf(stderr, format, args);
    } else {
	vfprintf(fp, format, args);
	fprintf(fp, "\n");
	fclose(fp);
    }
    va_end(args);
}

void 
log_status(char *format,...)
{
    va_list args;
    FILE   *fp;
    time_t  lt;
    char buf[40];

    va_start(args, format);
    lt = current_systime;

    *buf = '\0';
    if ((fp = fopen(LOG_STATUS, "a")) == NULL) {
	fprintf(stderr, "Unable to open %s!\n", LOG_STATUS);
	fprintf(stderr, "%.16s: ", ctime(&lt));
	vfprintf(stderr, format, args);
    } else {
        format_time(buf, 32, "%c", localtime(&lt));
	fprintf(fp, "%.32s: ", buf);
	vfprintf(fp, format, args);
	fclose(fp);
    }
    va_end(args);
}

void 
log_conc(char *format,...)
{
    va_list args;
    FILE   *conclog;

    va_start(args, format);
    if ((conclog = fopen(LOG_CONC, "a")) == NULL) {
	fprintf(stderr, "Unable to open %s!\n", LOG_CONC);
	vfprintf(stderr, format, args);
    } else {
	vfprintf(conclog, format, args);
	fclose(conclog);
    }
    va_end(args);
}

void 
log_muf(char *format,...)
{
    va_list args;
    FILE   *muflog;

    va_start(args, format);
    if ((muflog = fopen(LOG_MUF, "a")) == NULL) {
	fprintf(stderr, "Unable to open %s!\n", LOG_MUF);
	vfprintf(stderr, format, args);
    } else {
	vfprintf(muflog, format, args);
	fclose(muflog);
    }
    va_end(args);
}

void 
log_gripe(char *format,...)
{
    va_list args;
    FILE   *fp;
    time_t  lt;
    char buf[40];

    va_start(args, format);
    lt = current_systime;

    *buf = '\0';
    if ((fp = fopen(LOG_GRIPE, "a")) == NULL) {
	fprintf(stderr, "Unable to open %s!\n", LOG_GRIPE);
	fprintf(stderr, "%.16s: ", ctime(&lt));
	vfprintf(stderr, format, args);
    } else {
        format_time(buf, 32, "%c", localtime(&lt));
	fprintf(fp, "%.32s: ", buf);
	vfprintf(fp, format, args);
	fclose(fp);
    }
    va_end(args);
}

void 
log_command(char *format,...)
{
    va_list args;
    char buf[40];
    FILE   *fp;
    time_t  lt;

    va_start(args, format);
    lt = current_systime;

    *buf = '\0';
    if ((fp = fopen(COMMAND_LOG, "a")) == NULL) {
	fprintf(stderr, "Unable to open %s!\n", COMMAND_LOG);
	vfprintf(stderr, format, args);
    } else {
        format_time(buf, 32, "%c", localtime(&lt));
	fprintf(fp, "%.32s: ", buf);
	vfprintf(fp, format, args);
	fclose(fp);
    }
    va_end(args);
}

void 
notify_fmt(dbref player, char *format,...)
{
    va_list args;
    char    bufr[BUFFER_LEN];

    va_start(args, format);
    vsprintf(bufr, format, args);
    notify(player, bufr);
    va_end(args);
}
