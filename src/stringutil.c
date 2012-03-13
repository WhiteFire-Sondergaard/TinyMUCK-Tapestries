/* $Header: /home/foxen/CR/FB/src/RCS/stringutil.c,v 1.1 1996/06/12 03:05:08 foxen Exp $ */

/*
 * $Log: stringutil.c,v $
 * Revision 1.1  1996/06/12 03:05:08  foxen
 * Initial revision
 *
 * Revision 5.17  1994/03/21  15:14:15  foxen
 * Syntactical bugfix in exit_prefix()
 *
 * Revision 5.16  1994/03/21  11:00:42  foxen
 * Autoconfiguration mods.
 *
 * Revision 5.15  1994/03/14  12:20:58  foxen
 * Fb5.20 release checkpoint.
 *
 * Revision 5.14  1994/03/14  12:08:46  foxen
 * Initial portability mods and bugfixes.
 *
 * Revision 5.13  1994/02/11  05:52:41  foxen
 * Memory cleanup and monitoring code mods.
 *
 * Revision 5.12  1994/01/18  20:52:20  foxen
 * Version 5.15 release.
 *
 * Revision 5.11  1993/12/20  06:22:51  foxen
 * *** empty log message ***
 *
 * Revision 5.1  1993/12/17  00:07:33  foxen
 * initial revision.
 *
 * Revision 1.10  90/09/28  12:25:08  rearl
 * Moved alloc_string() and alloc_prog_string() to here.
 *
 * Revision 1.9  90/09/18  08:02:25  rearl
 * Speedups mainly, improved upper/lowercase lookup.
 *
 * Revision 1.8  90/09/16  04:43:06  rearl
 * Preparation code added for disk-based MUCK.
 *
 * Revision 1.7  90/09/10  02:19:51  rearl
 * Introduced string compression of properties, for the
 * COMPRESS compiler option.
 *
 * Revision 1.6  90/08/15  03:08:38  rearl
 * Messed around with pronoun_substitute.  Hopefully made it easier to use.
 *
 * Revision 1.5  90/08/02  18:50:43  rearl
 * Fixed bug in capitalized self-substitutions.
 *
 * Revision 1.4  90/08/02  02:17:16  rearl
 * Capital % substitutions now automatically capitalize the
 * corresponding self-sub property.  Example: %O -> Her.
 *
 * Revision 1.3  90/07/29  17:45:08  rearl
 * Pronoun substitution for programs fixed.
 *
 * Revision 1.2  90/07/22  04:28:33  casie
 * Added %r/%R substitutions for reflexive pronouns.
 *
 * Revision 1.1  90/07/19  23:04:13  casie
 * Initial revision
 *
 *
 */

#include "copyright.h"
#include "config.h"
#include "db.h"
#include "tune.h"
#include "props.h"
#include "params.h"
#include "interface.h"

/* String utilities */

#include <stdio.h>
#include <ctype.h>
#include "externs.h"

extern const char *uppercase, *lowercase;

#define DOWNCASE(x) (lowercase[x])

#ifdef COMPRESS
extern const char *uncompress(const char *);

#endif				/* COMPRESS */

/*
 * routine to be used instead of strcasecmp() in a sorting routine
 * Sorts alphabetically or numerically as appropriate.
 * This would compare "network100" as greater than "network23"
 * Will compare "network007" as less than "network07"
 * Will compare "network23a" as less than "network23b"
 * Takes same params and returns same comparitive values as strcasecmp.
 * This ignores minus signs and is case insensitive.
 */
int alphanum_compare(const char *t1, const char *s2)
{
    int n1, n2, cnt1, cnt2;
    const char *u1, *u2;
    register const char *s1 = t1;
  
    while (*s1 && DOWNCASE(*s1) == DOWNCASE(*s2))
        s1++, s2++;
  
    /* if at a digit, compare number values instead of letters. */
    if (isdigit(*s1) && isdigit(*s2)) {
        u1 = s1;  u2 = s2;
        n1 = n2 = 0;  /* clear number values */
        cnt1 = cnt2 = 0;
  
        /* back up before zeros */
	if (s1 > t1 && *s2 == '0') s1--, s2--;     /* curr chars are diff */
        while (s1 > t1 && *s1 == '0') s1--, s2--;  /* prev chars are same */
        if (!isdigit(*s1)) s1++, s2++;
  
        /* calculate number values */
        while (isdigit(*s1)) cnt1++, n1 = (n1 * 10) + (*s1++ - '0');
        while (isdigit(*s2)) cnt2++, n2 = (n2 * 10) + (*s2++ - '0');
        
        /* if more digits than int can handle... */
        if (cnt1 > 8 || cnt2 > 8) {
            if (cnt1 == cnt2) return (*u1 - *u2);  /* cmp chars if mag same */
            return (cnt1 - cnt2);                  /* compare magnitudes */
        }
  
        /* if number not same, return count difference */
        if (n1 && n2 && n1 != n2) return (n1 - n2);
                                    
        /* else, return difference of characters */
        return (*u1 - *u2);
    }
    /* if characters not digits, and not the same, return difference */
    return (DOWNCASE(*s1) - DOWNCASE(*s2));
}

int
string_compare(register const char *s1, register const char *s2)
{
    while (*s1 && DOWNCASE(*s1) == DOWNCASE(*s2))
	s1++, s2++;

    return (DOWNCASE(*s1) - DOWNCASE(*s2));
}

const char *
exit_prefix(register const char *string, register const char *prefix)
{
    const char *p;
    const char *s = string;
    while (*s) {
        p = prefix;
	string = s;
        while(*s && *p && DOWNCASE(*s) == DOWNCASE(*p)) {
            s++; p++;
        }
	while (*s && isspace(*s)) s++;
        if (!*p && (!*s || *s == EXIT_DELIMITER)) {
	    return string;
        }
        while(*s && (*s != EXIT_DELIMITER)) s++;
        if (*s) s++;
	while (*s && isspace(*s)) s++;
    }
    return 0;
}

int
string_prefix(register const char *string, register const char *prefix)
{
    while (*string && *prefix && DOWNCASE(*string) == DOWNCASE(*prefix))
	string++, prefix++;
    return *prefix == '\0';
}

/* accepts only nonempty matches starting at the beginning of a word */
const char *
string_match(register const char *src, register const char *sub)
{
    if (*sub != '\0') {
	while (*src) {
	    if (string_prefix(src, sub))
		return src;
	    /* else scan to beginning of next word */
	    while (*src && isalnum(*src))
		src++;
	    while (*src && !isalnum(*src))
		src++;
	}
    }
    return 0;
}

/*
 * pronoun_substitute()
 *
 * %-type substitutions for pronouns
 *
 * %a/%A for absolute possessive (his/hers/its, His/Hers/Its)
 * %s/%S for subjective pronouns (he/she/it, He/She/It)
 * %o/%O for objective pronouns (him/her/it, Him/Her/It)
 * %p/%P for possessive pronouns (his/her/its, His/Her/Its)
 * %r/%R for reflexive pronouns (himself/herself/itself,
 *                                Himself/Herself/Itself)
 * %n    for the player's name.
 */
char   *
pronoun_substitute(dbref player, const char *str)
{
    char    c;
    char    d;
    char    prn[3];
    static char buf[BUFFER_LEN * 2];
    char    orig[BUFFER_LEN];
    char   *result;
    const char *self_sub = NULL;	/* self substitution code */
    dbref   mywhere = player;
    int sex;

    static const char *subjective[] = {"", "it",     "she",     "he",      "sie"};
    static const char *possessive[] = {"", "its",    "her",     "his",     "hir"};
    static const char *objective[] = {	"", "it",     "her",     "him",     "hir"};
    static const char *reflexive[] = {	"", "itself", "herself", "himself", "hirself"};
    static const char *absolute[] = {	"", "its",    "hers",    "his",     "hirs"};

    prn[0] = '%';
    prn[2] = '\0';

#ifdef COMPRESS
    str = uncompress(str);
#endif				/* COMPRESS */

    strcpy(orig, str);
    str = orig;

    sex = genderof(player);
    result = buf;
    while (*str) {
	if (*str == '%') {
	    *result = '\0';
	    prn[1] = c = *(++str);
	    if (c == '%') {
		*(result++) = '%';
		str++;
            } else if (!c) { /* 11/24/2003 WhiteFire String ends in % */
                return buf;
	    } else {
		mywhere = player;
		d = (isupper(c)) ? c : toupper(c);

		if (d != 'N' && d != 'n')
		{
                    if (d == 'A' || d == 'S' || d == 'O' ||
                        d == 'P' || d == 'R') {
#ifdef COMPRESS
                        self_sub = uncompress(get_property_class(mywhere, prn));
#else
                        self_sub = get_property_class(mywhere, prn);
#endif
                    } else {
#ifdef COMPRESS
                        self_sub = uncompress(envpropstr(&mywhere, prn));
#else
                        self_sub = envpropstr(&mywhere, prn);
#endif
                    }
		}

		if (self_sub) {
		    if (((result - buf) + strlen(self_sub)) > (BUFFER_LEN - 2))
			return buf;
		    strcat(result, self_sub);
		    if (isupper(prn[1]) && islower(*result))
			*result = toupper(*result);
		    result += strlen(result);
		    str++;
		} else if (sex == GENDER_UNASSIGNED) {
		    switch (c) {
			case 'n':
			case 'N':
			case 'o':
			case 'O':
			case 's':
			case 'S':
			case 'r':
			case 'R':
			    strcat(result, PNAME(player));
			    break;
			case 'a':
			case 'A':
			case 'p':
			case 'P':
			    strcat(result, PNAME(player));
			    strcat(result, "'s");
			    break;
			default:
			    result[0] = *str;
			    result[1] = 0;
			    break;
		    }
		    str++;
		    result += strlen(result);
		    if ((result - buf) > (BUFFER_LEN - 2)) {
			buf[BUFFER_LEN - 1] = '\0';
			return buf;
		    }
		} else {
		    switch (c) {
			case 'a':
			case 'A':
			    strcat(result, absolute[sex]);
			    break;
			case 's':
			case 'S':
			    strcat(result, subjective[sex]);
			    break;
			case 'p':
			case 'P':
			    strcat(result, possessive[sex]);
			    break;
			case 'o':
			case 'O':
			    strcat(result, objective[sex]);
			    break;
			case 'r':
			case 'R':
			    strcat(result, reflexive[sex]);
			    break;
			case 'n':
			case 'N':
			    strcat(result, PNAME(player));
			    break;
			default:
			    result[0] = *str;
			    result[1] = 0;
			    break;
		    }
		    if (isupper(c) && islower(*result)) {
			*result = toupper(*result);
		    }
		    result += strlen(result);
		    str++;
		    if ((result - buf) > (BUFFER_LEN - 2)) {
			buf[BUFFER_LEN - 1] = '\0';
			return buf;
		    }
		}
	    }
	} else {
	    if ((result - buf) > (BUFFER_LEN - 2)) {
		buf[BUFFER_LEN - 1] = '\0';
		return buf;
	    }
	    *result++ = *str++;
	}
    }
    *result = '\0';
    return buf;
}

#ifndef MALLOC_PROFILING

char   *
alloc_string(const char *string)
{
    char   *s;

    /* NULL, "" -> NULL */
    if (!string || !*string)
	return 0;

    if ((s = (char *) malloc(strlen(string) + 1)) == 0) {
	abort();
    }
    strcpy(s, string);
    return s;
}

#endif

struct shared_string *
alloc_prog_string(const char *s)
{
    struct shared_string *ss;
    int     length;

    if (s == NULL || *s == '\0')
	return (NULL);

    length = strlen(s);
    if ((ss = (struct shared_string *)
	 malloc(sizeof(struct shared_string) + length)) == NULL)
	abort();
    ss->links = 1;
    ss->length = length;
    bcopy(s, ss->data, ss->length + 1);
    return (ss);
}


#if !defined(MALLOC_PROFILING)
char   *
string_dup(const char *s)
{
    char   *p;

    p = (char *) malloc(1 + strlen(s));
    if (p)
	(void) strcpy(p, s);
    return (p);
}
#endif



char *
intostr(int i)
{
    static char num[16];
    int j, k;
    char *ptr2;

    k = i;
    ptr2 = num+14;
    num[15] = '\0';
    if (i < 0) i = -i;
    while (i) {
	j = i % 10;
	*ptr2-- = '0' + j;
	i /= 10;
    }
    if (!k) *ptr2-- = '0';
    if (k < 0) *ptr2-- = '-';
    return (++ptr2);
}

const char *
name_mangle(dbref obj)
{
#if defined(ANONYMITY)
    static char pad[32];
    PropPtr	p;

    if(!(p=get_property(obj, "@fakename")) || PropType(p)!=PROP_STRTYP)
	return db[obj].name;
    sprintf(pad, "\001%s\002", l64a(obj));
    return pad;
#else
	return db[obj].name;
#endif
}

const char *unmangle(dbref player, char *s)
{
#if defined(ANONYMITY)
    char	in[16384];
    static char	buf[16384];
    char	pad[1024];
    char	*ptr, *src, *name;
    dbref	is;
    PropPtr	p;

    if(!s)
	return s;

    strcpy(in, s);
    ptr = buf;
    src = in;
    *ptr = 0;

    while(name = strchr(src, 1))
    {
	*(name++) = 0;
	strcpy(ptr, src);
	if(src = strchr(name, 2))
	   *(src++) = 0;
        else
	   src = name+strlen(name);

	is = a64l(name);
	strcpy(pad, "@knows/");
	strcat(pad, NAME(is));

	if((p=get_property(is, "@disguise")) && PropType(p)==PROP_STRTYP) {
#ifdef DISKBASE
	    propfetch(is, p);
#endif
	    strcpy(pad, uncompress(PropDataStr(p)));
	} else if((p=get_property(player, pad)) && PropType(p)==PROP_STRTYP) {
#ifdef DISKBASE
	    propfetch(player, p);
#endif
	    strcpy(pad, uncompress(PropDataStr(p)));
	} else if((p=get_property(is,"@fakename"))&&PropType(p)==PROP_STRTYP) {
#ifdef DISKBASE
	    propfetch(is, p);
#endif
	    strcpy(pad, uncompress(PropDataStr(p)));
	else
	    strcpy(pad, NAME(is));

	strcat(ptr, pad);
	ptr += strlen(ptr);
    }

    strcat(ptr, src);

    return buf;
#else
    return s;
#endif
}



/*
 * Encrypt one string with another one.
 */

#define CHARCOUNT 96

static char enarr[256];
static int charset_count[] = {96, 0};
static int initialized_crypt = 0;

void
init_crypt(void)
{
    int i;
    for (i = 0; i <= 255; i++) enarr[i] = (char) i;
    for (i = 'A'; i <= 'M'; i++) enarr[i] = (char)enarr[i] + 13;
    for (i = 'N'; i <= 'Z'; i++) enarr[i] = (char)enarr[i] - 13;
    enarr['\r'] = 127;
    enarr[127] = '\r';
    initialized_crypt = 1;
}


const char *
strencrypt(const char *data, const char *key)
{
    static char linebuf[BUFFER_LEN];
    char buf[BUFFER_LEN + 8];
    const char *cp;
    unsigned char *ptr;
    unsigned char *ups;
    const unsigned char *upt;
    int linelen;
    int count;
    int seed, seed2, seed3;
    int limit = BUFFER_LEN;
    int result;

    if (!initialized_crypt)
        init_crypt();

    seed = 0;
    for (cp = key; *cp; cp++)
	seed = ((((*cp) ^ seed) + 170) % 192);

    seed2 = 0;
    for (cp = data; *cp; cp++)
	seed2 = ((((*cp) ^ seed2) + 21) & 0xff);
    seed3 = seed2 = ((seed2 ^ (seed ^ (RANDOM() >> 24))) & 0x3f);

    count = seed + 11;
    for (upt = data, ups = buf, cp = key; *upt; upt++) {
	count = (((*cp) ^ count) + (seed ^ seed2)) & 0xff;
	seed2 = ((seed2 + 1) & 0x3f);
	if (!*(++cp)) cp = key;
	result = (enarr[*upt] - (32 - (CHARCOUNT - 96))) + count + seed;
	*ups = enarr[(result % CHARCOUNT) + (32 - (CHARCOUNT - 96))];
	count = (((*upt) ^ count) + seed) & 0xff;
	ups++;
    }
    *ups++ = '\0';

    ptr = linebuf;

    linelen = strlen(data);
    *ptr++ = (' ' + 1);
    *ptr++ = (' ' + seed3);
    limit--;
    limit--;

    for(cp = buf; cp < &buf[linelen]; cp++){
	limit--;
	if (limit < 0) break;
	*ptr++ = *cp;
    }
    *ptr++ = '\0';
    return linebuf;
}



const char *
strdecrypt(const char *data, const char *key)
{
    char linebuf[BUFFER_LEN];
    static char buf[BUFFER_LEN];
    const char *cp;
    unsigned char *ptr;
    unsigned char *ups;
    const unsigned char *upt;
    int linelen;
    int outlen;
    int count;
    int seed, seed2;
    int result;
    int chrcnt;

    if (!initialized_crypt)
        init_crypt();

    ptr = linebuf;

    if ((data[0] - ' ') < 1 || (data[0] - ' ') > 1) {
        return "";
    }

    linelen = strlen(data);
    chrcnt = charset_count[(data[0] - ' ') - 1];
    seed2 = (data[1] - ' ');

    strcpy(linebuf, data + 2);

    seed = 0;
    for (cp = key; *cp; cp++)
	seed = (((*cp) ^ seed) + 170) % 192;

    count = seed + 11;
    outlen = strlen(linebuf);
    upt = linebuf;
    ups = buf;
    cp = key;
    while ((const char *)upt < &linebuf[outlen]) {
	count = (((*cp) ^ count) + (seed ^ seed2)) & 0xff;
	if (!*(++cp)) cp = key;
	seed2 = ((seed2 + 1) & 0x3f);

	result = (enarr[*upt] - (32 - (chrcnt - 96))) - (count + seed);
	while (result < 0) result += chrcnt;
	*ups = enarr[result + (32 - (chrcnt - 96))];

	count = (((*ups) ^ count) + seed) & 0xff;
	ups++; upt++;
    }
    *ups++ = '\0';

    return buf;
}



