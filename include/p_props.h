extern void prim_getpropval(PRIM_PROTOTYPE);
extern void prim_getpropstr(PRIM_PROTOTYPE);
extern void prim_remove_prop(PRIM_PROTOTYPE);
extern void prim_envprop(PRIM_PROTOTYPE);
extern void prim_envpropstr(PRIM_PROTOTYPE);
extern void prim_addprop(PRIM_PROTOTYPE);
extern void prim_nextprop(PRIM_PROTOTYPE);
extern void prim_propdirp(PRIM_PROTOTYPE);
extern void prim_parseprop(PRIM_PROTOTYPE);
extern void prim_getprop(PRIM_PROTOTYPE);
extern void prim_setprop(PRIM_PROTOTYPE);

#define PRIMS_PROPS_FUNCS prim_getpropval, prim_getpropstr, prim_remove_prop, \
   prim_envprop, prim_envpropstr, prim_addprop, prim_nextprop, prim_propdirp, \
   prim_parseprop, prim_getprop, prim_setprop

#define PRIMS_PROPS_NAMES "GETPROPVAL", "GETPROPSTR", "REMOVE_PROP",  \
    "ENVPROP", "ENVPROPSTR", "ADDPROP", "NEXTPROP", "PROPDIR?", \
    "PARSEPROP", "GETPROP", "SETPROP"

#define PRIMS_PROPS_CNT 11

