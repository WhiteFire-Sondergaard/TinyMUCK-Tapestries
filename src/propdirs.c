
#include "copyright.h"
#include "config.h"
#include "params.h"

#include "db.h"
#include "tune.h"
#include "props.h"
#include "externs.h"
#include "interface.h"


/* propdirs.c -- handles propdirs property creation, deletion, and finding.  */
/* WARNING: These routines all hack up the path string passed to them. */

/* USES:

     PropPtr locate_prop(PropPtr list, char *name)
       if list is NULL, return NULL.

     PropPtr new_prop(PropPtr *list, char *name)
       if *list is NULL, create a new propdir, then insert the prop

     PropPtr delete_prop(PropPtr *list, char *name)
       when last prop in dir is deleted, destroy propdir & change *list to NULL

     PropPtr first_node(PropPtr list)
       if list is NULL, return NULL

     PropPtr next_node(PropPtr list)
       if list is NULL, return NULL

 */


/*
 * returns pointer to the new property node.  Returns a pointer to an
 *   existing elem of the given name, if one already exists.  Returns
 *   NULL if the name given is bad.
 * l is the pointer to the root propdir node.  This is updated in this
 *   routine to point to the new root node of the structure.
 * path is the name of the property to insert
 */
PropPtr
propdir_new_elem(PropPtr *l, char *path)
{
    PropPtr p;
    char *n;
    while (*path && *path == PROPDIR_DELIMITER) path++;
    if (!*path) return(NULL);
    n = index(path, PROPDIR_DELIMITER);
    while (n && *n == PROPDIR_DELIMITER) *(n++) = '\0';
    if (n && *n) {
	/* just another propdir in the path */
        p = locate_prop(*l, path);
	if (!p) {
	    /* propdir didn't exist */
	    p = new_prop(l, path);
	}
	return(propdir_new_elem(&PropDir(p), n));
    } else {
	/* aha, we are finally to the property itself. */
        p = locate_prop(*l, path);
	if (!p) {
	    /* property didn't exist */
	    p = new_prop(l, path);
	}
	return(p);
    }
}


/* returns pointer to the updated propdir structure's root node */
/* l is the pointer to the root propdir node */
/* path is the name of the property to delete */
PropPtr 
propdir_delete_elem(PropPtr l, char *path)
{
    PropPtr p;
    char *n;
    if (!l) return(NULL);
    while (*path && *path == PROPDIR_DELIMITER) path++;
    if (!*path) return(l);
    n = index(path, PROPDIR_DELIMITER);
    while (n && *n == PROPDIR_DELIMITER) *(n++) = '\0';
    if (n && *n) {
	/* just another propdir in the path */
        p = locate_prop(l, path);
	if (p && PropDir(p)) {
	    /* yup, found the propdir */
	    SetPDir(p, propdir_delete_elem(PropDir(p), n));
	    if (!PropDir(p) && PropType(p) == PROP_DIRTYP) {
	        l = delete_prop(&l, PropName(p));
	    }
	}
	/* return the updated plist pntr */
	return(l);
    } else {
	/* aha, we are finally to the property itself. */
	p = locate_prop(l, path);
	if (p && PropDir(p))
	    delete_proplist(PropDir(p));
	(void)delete_prop(&l, path);
	return(l);
    }
}


/* returns pointer to given property */
/* l is the pointer to the root propdir node */
/* path is the name of the property to find */
PropPtr 
propdir_get_elem(PropPtr l, char *path)
{
    PropPtr p;
    char *n;
    if (!l) return(NULL);
    while (*path && *path == PROPDIR_DELIMITER) path++;
    if (!*path) return(NULL);
    n = index(path, PROPDIR_DELIMITER);
    while (n && *n == PROPDIR_DELIMITER) *(n++) = '\0';
    if (n && *n) {
	/* just another propdir in the path */
        p = locate_prop(l, path);
	if (p && PropDir(p)) {
	    /* yup, found the propdir */
	    return(propdir_get_elem(PropDir(p), n));
	}
	return(NULL);
    } else {
	/* aha, we are finally to the property subname itself. */
        if ((p = locate_prop(l, path)))
	    return(p);  /* found the property! */
	return(NULL);   /* nope, doesn't exist */
    }
}


/* returns pointer to first property in the given propdir */
/* l is the pointer to the root propdir node */
/* path is the name of the propdir to find the first node of */
PropPtr 
propdir_first_elem(PropPtr l, char *path)
{
    PropPtr p;
    while (*path && *path == PROPDIR_DELIMITER) path++;
    if (!*path) return(first_node(l));
    p = propdir_get_elem(l, path);
    if(p && PropDir(p))
	return(first_node(PropDir(p)));  /* found the property! */
    return(NULL);   /* nope, doesn't exist */
}


/* returns pointer to next property after the given one in the propdir */
/* l is the pointer to the root propdir node */
/* path is the name of the property to find the next node after */
/* Note: Finds the next alphabetical property, regardless of the existence
	  of the original property given. */
PropPtr 
propdir_next_elem(PropPtr l, char *path)
{
    PropPtr p;
    char *n;
    if (!l) return(NULL);
    while (*path && *path == PROPDIR_DELIMITER) path++;
    if (!*path) return(NULL);
    n = index(path, PROPDIR_DELIMITER);
    while (n && *n == PROPDIR_DELIMITER) *(n++) = '\0';
    if (n && *n) {
	/* just another propdir in the path */
        p = locate_prop(l, path);
	if (p && PropDir(p)) {
	    /* yup, found the propdir */
	    return(propdir_next_elem(PropDir(p), n));
	}
	return(NULL);
    } else {
	/* aha, we are finally to the property subname itself. */
        return(next_node(l, path));
    }
}


/* return true if a property contains a propdir */
int
propdir_check(PropPtr l, char *path)
{
    PropPtr p;
    if ((p = propdir_get_elem(l, path)))
	return(PropDir(p) != NULL);
    return(0);
}


