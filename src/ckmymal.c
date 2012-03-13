#include <stdio.h>

unsigned long totunfreed;

struct memries {
    unsigned long where, size, line;
    char   *file;
    struct memries *next;
}      *mymem;


void 
store_alloc(unsigned long whereat, unsigned long howbig,
	    char *whatfile, unsigned long whatline)
{
    struct memries *mytempmem =
    (struct memries *) malloc(sizeof(struct memries));

    mytempmem->where = whereat;
    mytempmem->size = howbig;
    mytempmem->file = (char *) malloc(strlen(whatfile) + 2);
    strcpy(mytempmem->file, whatfile);
    mytempmem->line = whatline;
    mytempmem->next = mymem;
    mymem = mytempmem;
}

void 
kill_alloc(unsigned long whereat, char *whatfile, unsigned long whatline)
{
    struct memries *lasttempmem;
    struct memries *mytempmem = mymem;

    if (!mymem) {
	printf("Unallocated memory freed at %d.  %s line %d\n",
	       whereat, whatfile, whatline);
	return;
    }
    if ((mymem->where <= whereat) || ((mymem->where + mymem->size) > whereat)) {
	lasttempmem = mymem;
	mymem = mymem->next;
	free(lasttempmem->file);
	free(lasttempmem);
	return;
    }
    lasttempmem = mymem;
    mytempmem = mymem->next;
    while (mytempmem && ((mytempmem->where > whereat) ||
			 ((mytempmem->where + mytempmem->size) <= whereat))) {
	lasttempmem = mytempmem;
	mytempmem = mytempmem->next;
    }
    if (!mytempmem) {
	printf("Unallocated memory freed at %d.  %s line %d\n",
	       whereat, whatfile, whatline);
	return;
    }
    lasttempmem->next = mytempmem->next;
    free(mytempmem->file);
    free(mytempmem);
}

void 
dump_allocs(void)
{
    struct memries *mytemp = mymem;

    totunfreed = 0;
    while (mytemp) {
	printf("Unfreed memory: %d bytes at %d.  %s line %d\n",
	       mytemp->size, mytemp->where, mytemp->file, mytemp->line);
	totunfreed += mytemp->size;
	mytemp = mytemp->next;
    }
    printf("%d bytes total unfreed.\n", totunfreed);
}


char    buf[4096];
char   *rest;

char   *
next_word()
{
    char   *last;

    if (!rest)
	rest = buf;
    last = rest;
    while (*rest && !isspace(*rest))
	rest++;
    if (*rest) {
	*rest = '\0';
	rest++;
    }
    return (last);
}

main()
{
    char   *myword;
    char   *filename;
    unsigned long tmp1, tmp2, tmp3, line, inln = 1;

    mymem = NULL;
    rest = buf;
    for (gets(buf, 4090); !feof(stdin); inln++, gets(buf, 4090)) {
	if (*buf == '#')
	    continue;
	rest = buf;
	filename = next_word();
	line = atol(next_word());
	myword = next_word();
	if (!strcmp(myword, "Allocated")) {
	    myword = next_word();
	    tmp1 = (unsigned long) atol(myword);
	    myword = next_word();
	    myword = next_word();
	    myword = next_word();
	    tmp2 = (unsigned long) atol(myword);
	    store_alloc(tmp2, tmp1, filename, line);
	} else if (!strcmp(myword, "Freed")) {
	    myword = next_word();
	    myword = next_word();
	    myword = next_word();
	    tmp1 = (unsigned long) atol(myword);
	    kill_alloc(tmp1, filename, line);
	} else if (!strcmp(myword, "Reallocated")) {
	    myword = next_word();
	    tmp1 = (unsigned long) atol(myword);
	    myword = next_word();
	    myword = next_word();
	    myword = next_word();
	    tmp2 = (unsigned long) atol(myword);
	    myword = next_word();
	    myword = next_word();
	    tmp3 = (unsigned long) atol(myword);
	    kill_alloc(tmp3, filename, line);
	    store_alloc(tmp2, tmp1, filename, line);
	} else {
	    fprintf(stderr, "Error!  Bad input format! line %d.\n", inln);
	    return;
	}
    }
    dump_allocs();
}
