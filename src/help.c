/* $Header: /home/foxen/CR/FB/src/RCS/help.c,v 1.1 1996/06/12 02:22:36 foxen Exp $ */

/*
 * $Log: help.c,v $
 * Revision 1.1  1996/06/12 02:22:36  foxen
 * Initial revision
 *
 * Revision 5.19  1994/04/03  19:58:01  foxen
 * Fixed the man, mpi, and help commands for ULTRIX
 *
 * Revision 5.18  1994/03/21  11:00:42  foxen
 * Autoconfiguration mods.
 *
 * Revision 5.17  1994/03/14  12:20:58  foxen
 * Fb5.20 release checkpoint.
 *
 * Revision 5.16  1994/01/18  20:52:20  foxen
 * Version 5.15 release.
 *
 * Revision 5.15  1994/01/18  19:55:14  foxen
 * man, mpi, help, and news now look for seperate topic files before looking
 * in the single delimited helpfile.
 *
 * Revision 5.14  1994/01/14  01:42:08  foxen
 * added newline to error message when help file is not found.
 *
 * Revision 5.13  1994/01/08  05:38:19  foxen
 * removes setvbuf() calls.
 *
 * Revision 5.12  1994/01/06  03:12:12  foxen
 * version 5.12
 *
 * Revision 5.1  1993/12/17  00:07:33  foxen
 * initial revision.
 *
 * Revision 1.1  90/07/19  23:03:38  casie
 * Initial revision
 *
 *
 */

#include "copyright.h"
#include "config.h"

/* commands for giving help */

#include "db.h"
#include "props.h"
#include "params.h"
#include "tune.h"
#include "interface.h"
#include "externs.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

/*
 * Ok, directory stuff IS a bit ugly.
 */
#if defined(DIRENT) || defined(_POSIX_VERSION)
# include <dirent.h>
# define NLENGTH(dirent) (strlen((dirent)->d_name))
#else /* not (DIRENT or _POSIX_VERSION) */
# define dirent direct
# define NLENGTH(dirent) ((dirent)->d_namlen)
# ifdef SYSNDIR
#  include <sys/ndir.h>
# endif /* SYSNDIR */
# ifdef SYSDIR
#  include <sys/dir.h>
# endif /* SYSDIR */
# ifdef NDIR
#  include <ndir.h>
# endif /* NDIR */
#endif /* not (DIRENT or _POSIX_VERSION) */

#if defined(DIRENT) || defined(_POSIX_VERSION) || defined(SYSNDIR) || defined(SYSDIR) || defined(NDIR)
# define DIR_AVALIBLE
#endif

void 
spit_file_segment(dbref player, const char *filename, const char *seg)
{
    FILE   *f;
    char    buf[BUFFER_LEN];
    char    buf2[BUFFER_LEN];
    char    segbuf[BUFFER_LEN];
    char   *p, *ptr;
    int     startline, endline, currline;

    startline = endline = currline = 0;
    if (seg && *seg) {
	strcpy(segbuf, seg);
	for (p = segbuf; isdigit(*p); p++);
	if (*p) {
	    *p++ = '\0';
	    startline = atoi(segbuf);
	    while (*p && !isdigit(*p)) p++;
	    if (*p) endline = atoi(p);
	} else {
	    endline = startline = atoi(segbuf);
	}
    }
    if ((f = fopen(filename, "r")) == NULL) {
	sprintf(buf, "Sorry, %s is missing.  Management has been notified.",
		filename);
	notify(player, buf);
	fputs("spit_file:", stderr);
	perror(filename);
    } else {
	while (fgets(buf, sizeof buf, f)) {
	    for (p = buf; *p; p++)
		if (*p == '\n') {
		    *p = '\0';
		    break;
		}
	    currline++;
	    if ((!startline || (currline >= startline)) &&
		    (!endline || (currline <= endline))) {
		if (*buf) {
		    notify(player, buf);
		} else {
		    notify(player, "  ");
	        }
	    }
	}
	fclose(f);
    }
}

void 
spit_file(dbref player, const char *filename)
{
    spit_file_segment(player, filename, "");
}


void 
index_file(dbref player, const char *onwhat, const char *file)
{
    FILE   *f;
    char    buf[BUFFER_LEN];
    char    topic[BUFFER_LEN];
    char   *p;
    int     arglen, found;

    *topic = '\0';
    strcpy(topic, onwhat);
    if (*onwhat) {
	strcat(topic, "|");
    }

    if ((f = fopen(file, "r")) == NULL) {
	sprintf(buf,
		"Sorry, %s is missing.  Management has been notified.", file);
	notify(player, buf);
	fprintf(stderr, "help: No file %s!\n", file);
    } else {
	if (*topic) {
	    arglen = strlen(topic);
	    do {
		do {
		    if (!(fgets(buf, sizeof buf, f))) {
			sprintf(buf, "Sorry, no help available on topic \"%s\"",
				onwhat);
			notify(player, buf);
			fclose(f);
			return;
		    }
		} while (*buf != '~');
		do {
		    if (!(fgets(buf, sizeof buf, f))) {
			sprintf(buf, "Sorry, no help available on topic \"%s\"",
				onwhat);
			notify(player, buf);
			fclose(f);
			return;
		    }
		} while (*buf == '~');
		p = buf;
		found = 0;
		buf[strlen(buf) - 1] = '|';
		while (*p && !found) {
		    if (strncasecmp(p, topic, arglen)) {
			while (*p && (*p != '|')) p++;
			if (*p) p++;
		    } else {
			found = 1;
		    }
		}
	    } while (!found);
	}
	while (fgets(buf, sizeof buf, f)) {
	    if (*buf == '~')
		break;
	    for (p = buf; *p; p++)
		if (*p == '\n') {
		    *p = '\0';
		    break;
		}
	    if (*buf) {
		notify(player, buf);
	    } else {
		notify(player, "  ");
	    }
	}
	fclose(f);
    }
}


int 
show_subfile(dbref player, const char *dir, const char *topic, const char *seg,
	     int partial)
{
    char   buf[256];
    char   *ptr;
    struct stat st;
#ifdef DIR_AVALIBLE
    DIR		*df;
    struct dirent *dp;
#endif 

    if (!topic || !*topic) return 0;

    if ((*topic == '.') || (*topic == '~') || (index(topic, '/'))) {
	return 0;
    }
    if (strlen(topic) > 63) return 0;


#ifdef DIR_AVALIBLE
    /* TO DO: (1) exact match, or (2) partial match, but unique */
    *buf = 0;

    if (df = (DIR *) opendir(dir))
    {
        while (dp = readdir(df))
        {
            if ((partial  && string_prefix(dp->d_name, topic)) ||
                (!partial && !string_compare(dp->d_name, topic))
                )
            {
                sprintf(buf, "%s/%s", dir, dp->d_name);
                break;
            }
        }
        closedir(df);
    }
    
    if (!*buf)
    {
	return 0; /* no such file or directory */
    }
#else /* !DIR_AVALIBLE */
    sprintf(buf, "%s/%s", dir, topic);
#endif /* !DIR_AVALIBLE */

    if (stat(buf, &st)) {
	return 0;
    } else {
	spit_file_segment(player, buf, seg);
	return 1;
    }
}


void 
do_man(dbref player, char *topic, char *seg)
{
    if (show_subfile(player, MAN_DIR, topic, seg, FALSE)) return;
    index_file(player, topic, MAN_FILE);
}


void 
do_mpihelp(dbref player, char *topic, char *seg)
{
    if (show_subfile(player, MPI_DIR, topic, seg, FALSE)) return;
    index_file(player, topic, MPI_FILE);
}


void 
do_help(dbref player, char *topic, char *seg)
{
    if (show_subfile(player, HELP_DIR, topic, seg, FALSE)) return;
    index_file(player, topic, HELP_FILE);
}


void 
do_news(dbref player, char *topic, char *seg)
{
    if (show_subfile(player, NEWS_DIR, topic, seg, FALSE)) return;
    index_file(player, topic, NEWS_FILE);
}


void 
add_motd_text_fmt(const char *text)
{
    char    buf[80];
    const char *p = text;
    int     count = 4;

    buf[0] = buf[1] = buf[2] = buf[3] = ' ';
    while (*p) {
	while (*p && (count < 68))
	    buf[count++] = *p++;
	while (*p && !isspace(*p) && (count < 76))
	    buf[count++] = *p++;
	buf[count] = '\0';
	log2file(MOTD_FILE, "%s", buf);
	while (*p && isspace(*p))
	    p++;
	count = 0;
    }
}


void 
do_motd(dbref player, char *text)
{
    time_t  lt;

    if (!*text || !Wizard(OWNER(player))) {
	spit_file(player, MOTD_FILE);
	return;
    }
    if (!string_compare(text, "clear")) {
	unlink(MOTD_FILE);
	log2file(MOTD_FILE, "%s %s", "- - - - - - - - - - - - - - - - - - -",
		 "- - - - - - - - - - - - - - - - - - -");
	notify(player, "MOTD cleared.");
	return;
    }
    lt = current_systime;
    log2file(MOTD_FILE, "%.16s", ctime(&lt));
    add_motd_text_fmt(text);
    log2file(MOTD_FILE, "%s %s", "- - - - - - - - - - - - - - - - - - -",
	     "- - - - - - - - - - - - - - - - - - -");
    notify(player, "MOTD updated.");
}


void 
do_info(dbref player, const char *topic, const char *seg)
{
    char	*buf;
    struct	stat st;
#ifdef DIR_AVALIBLE
    DIR		*df;
    struct dirent *dp;
#endif
    int		f;
    int		cols;

    if (*topic) {
        if (!show_subfile(player, INFO_DIR, topic, seg, TRUE))
	{
	    notify(player, NO_INFO_MSG);
	}
    } else {
#ifdef DIR_AVALIBLE
	buf = (char *) calloc(1, 80);
	(void) strcpy(buf, "    ");
	f = 0;
	cols = 0;
	if (df = (DIR *) opendir(INFO_DIR)) 
	{
	    while (dp = readdir(df))
	    {
		
		if (*(dp->d_name) != '.') 
		{
		    if (!f)
			notify(player, "Available information files are:");
		    if ((cols++ > 2) || 
			((strlen(buf) + strlen(dp->d_name)) > 63)) 
		    {
			notify(player, buf);
			(void) strcpy(buf, "    ");
			cols = 0;
		    }
		    (void) strcat(strcat(buf, dp->d_name), " ");
		    f = strlen(buf);
		    while ((f % 20) != 4)
			buf[f++] = ' ';
		    buf[f] = '\0';
		}
	    }
	    closedir(df);
	}
	if (f)
	    notify(player, buf);
	else
	    notify(player, "No information files are available.");
	free(buf);
#else /* !DIR_AVALIBLE */
	notify(player, "Index not available on this system.");
#endif /* !DIR_AVALIBLE */
    }
}
