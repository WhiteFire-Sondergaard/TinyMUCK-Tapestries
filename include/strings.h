/* $Header: /home/foxen/CR/FB/include/RCS/strings.h,v 1.1 1996/06/17 17:29:45 foxen Exp foxen $ */

/*
 * (c) 1990 Chelsea Dyerman, University of California, Berkeley (XCF)
 *
 * Portions of this header file are copyright of Regents of U of California.
 * This file is being distributed along with a copy of the Berkeley software
 * agreement. 
 *
 */

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)strings.h	5.3 (Berkeley) 11/18/87
 */

/*
 * External function definitions
 * for routines described in string(3).
 */

/*
char	*strcat();
char	*strncat();
int	strcmp();
int	strncmp();
int	strcasecmp();
int	strncasecmp();
char	*strcpy();
char	*strncpy();
int	strlen();
char	*index();
char	*rindex();
*/

const char*	name_mangle();
const char*	unmangle();
const char*	strencrypt(const char *, const char *);
const char*	strdecrypt(const char *, const char *);

