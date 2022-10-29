/* NetHack 3.6	objclass.h	$NHDT-Date: 1547255901 2019/01/12 01:18:21 $  $NHDT-Branch: NetHack-3.6.2-beta01 $:$NHDT-Revision: 1.20 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Pasi Kallinen, 2018. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef OBJCLASS_H
#define OBJCLASS_H

/* [misnamed] definition of a type of object; many objects are composites
   (liquid potion inside glass bottle, metal arrowhead on wooden shaft)
   and object definitions only specify one type on a best-fit basis */
enum obj_material_types {
    LIQUID      =  1, /* currently only for venom */
    WAX         =  2,
    VEGGY       =  3, /* foodstuffs */
    FLESH       =  4, /*   ditto    */
    PAPER       =  5,
    CLOTH       =  6,
    LEATHER     =  7,
    WOOD        =  8,
    BONE        =  9,
    DRAGON_HIDE = 10, /* not leather! */
    IRON        = 11, /* Fe */
    METAL       = 12, /* Stainless steel, Sn, &c. */
    COPPER      = 13, /* Cu - includes brass */
    SILVER      = 14, /* Ag */
    GOLD        = 15, /* Au */
    PLATINUM    = 16, /* Pt */
    MITHRIL     = 17,
    PLASTIC     = 18,
    GLASS       = 19,
    GEMSTONE    = 20,
    MINERAL     = 21,
    NUM_MATERIAL_TYPES
};

enum obj_armor_types {
    ARM_SUIT   = 0,
    ARM_SHIELD = 1,        /* needed for special wear function */
    ARM_HELM   = 2,
    ARM_GLOVES = 3,
    ARM_BOOTS  = 4,
    ARM_CLOAK  = 5,
    ARM_SHIRT  = 6
};

struct objclass {
    short oc_name_idx;              /* index of actual name */
    short oc_descr_idx;             /* description when name unknown */
    char *oc_uname;                 /* called by user */
    Bitfield(oc_name_known, 1);     /* discovered */
    Bitfield(oc_merge, 1);          /* merge otherwise equal objects */
    Bitfield(oc_uses_known, 1);     /* obj->known affects full description;
                                       otherwise, obj->dknown and obj->bknown
                                       tell all, and obj->known should always
                                       be set for proper merging behavior. */
    Bitfield(oc_pre_discovered, 1); /* Already known at start of game;
                                       won't be listed as a discovery. */
    Bitfield(oc_magic, 1);          /* inherently magical object */
    Bitfield(oc_charged, 1);        /* may have +n or (n) charges */
    Bitfield(oc_unique, 1);         /* special one-of-a-kind object */
    Bitfield(oc_nowish, 1);         /* cannot wish for this object */

    Bitfield(oc_big, 1);
#define oc_bimanual oc_big /* for weapons & tools used as weapons */
#define oc_bulky oc_big    /* for armor */
    Bitfield(oc_tough, 1); /* hard gems/rings */

    Bitfield(oc_dir, 3);
    /* oc_dir: zap style for wands and spells */
#define NODIR     1 /* non-directional */
#define IMMEDIATE 2 /* directional beam that doesn't ricochet */
#define RAY       3 /* beam that does bounce off walls */
    /* overloaded oc_dir: strike mode bit mask for weapons and weptools */
#define PIERCE   01 /* pointed weapon punctures target */
#define SLASH    02 /* sharp weapon cuts target */
#define WHACK    04 /* blunt weapon bashes target */

#define is_pierce(otmp) (objects[otmp->otyp].oc_dir == PIERCE)
#define is_slash(otmp) (objects[otmp->otyp].oc_dir == SLASH)
#define is_whack(otmp) (objects[otmp->otyp].oc_dir == WHACK)

    /* 4 free bits */

    Bitfield(oc_material, 5); /* one of obj_material_types */

#define is_organic(otmp) (otmp->material <= WOOD)
#define is_dragonhide(otmp) (otmp->material == DRAGON_HIDE)
#define is_mithril(otmp) (otmp->material == MITHRIL)
#define is_iron(otmp) (otmp->material == IRON)
#define is_silver(otmp) (otmp->material == SILVER)
#define is_glass(otmp) (otmp->material == GLASS)
#define is_wood(otmp) (otmp->material == WOOD)
#define is_bone(otmp) (otmp->material == BONE)
#define is_stone(otmp) (otmp->material == MINERAL)
#define is_crystal(otmp) (otmp->material == GEMSTONE)
#define is_metallic(otmp) \
    (otmp->material >= IRON && otmp->material <= MITHRIL)
#define is_heavy_metallic(otmp) \
    (otmp->material >= IRON && otmp->material <= PLATINUM)

#define is_hard(otmp) \
    (is_metallic(otmp) || is_glass(otmp) \
     || is_wood(otmp) || is_bone(otmp) || is_stone(otmp))

/* primary damage: fire/rust/--- */
/* is_flammable(otmp), is_rottable(otmp) in mkobj.c */
#define is_rustprone(otmp) (otmp->material == IRON)

/* secondary damage: rot/acid/acid */
#define is_corrodeable(otmp) \
    (otmp->material == COPPER || otmp->material == IRON)

/* inherently fooproof */
#define is_supermaterial(otmp) \
    (otmp->material == DRAGON_HIDE || otmp->material == MITHRIL \
     || otmp->material == GOLD || otmp->material == PLATINUM    \
     || otmp->material == SILVER || otmp->material == MINERAL   \
     || otmp->material == METAL)

#define is_damageable(otmp)                                        \
    (is_rustprone(otmp) || is_flammable(otmp) || is_rottable(otmp) \
     || is_corrodeable(otmp) || is_glass(otmp))

    /* 3 free bits */

    schar oc_subtyp;
#define oc_skill oc_subtyp  /* Skills of weapons, spellbooks, tools, gems */
#define oc_armcat oc_subtyp /* for armor (enum obj_armor_types) */

    uchar oc_oprop; /* property (invis, &c.) conveyed */
    char  oc_class; /* object class (enum obj_class_types) */
    schar oc_delay; /* delay when using such an object */
    uchar oc_color; /* color of the object */

    short oc_prob;            /* probability, used in mkobj() */
    unsigned short oc_weight; /* encumbrance (1 cn = 0.1 lb.) */
    short oc_cost;            /* base cost in shops */
    /* Check the AD&D rules!  The FIRST is small monster damage. */
    /* for weapons, and tools, rocks, and gems useful as weapons */
    schar oc_wsdam, oc_wldam; /* max small/large monster damage */
    schar oc_oc1, oc_oc2;
#define w_ammotyp	oc_oc2		/* type of ammo taken by ranged weapon */
#define WP_GENERIC	0		/* all ammo subclasses ok */
#define WP_BULLET	1
#define WP_SHELL	2
#define WP_ROCKET	3
#define WP_BOMB         4
#define oc_hitbon oc_oc1 /* weapons: "to hit" bonus */
#define w_acbon oc_oc2 /* weapons: ac bonus */

#define a_ac oc_oc1     /* armor class, used in armor_bonus in worn.c */
#define a_can oc_oc2    /* armor: used in mhitu.c */
#define oc_level oc_oc2 /* books: spell level */

    unsigned short oc_nutrition; /* food value */
};

struct class_sym {
    char sym;
    const char *name;
    const char *explain;
};

struct objdescr {
    const char *oc_name;  /* actual name */
    const char *oc_descr; /* description when name unknown */
};

extern NEARDATA struct objclass objects[];
extern NEARDATA struct objdescr obj_descr[];

/*
 * All objects have a class. Make sure that all classes have a corresponding
 * symbol below.
 */
enum obj_class_types {
    RANDOM_CLASS =  0, /* used for generating random objects */
    ILLOBJ_CLASS =  1,
    WEAPON_CLASS =  2,
    ARMOR_CLASS  =  3,
    RING_CLASS   =  4,
    AMULET_CLASS =  5,
    TOOL_CLASS   =  6,
    FOOD_CLASS   =  7,
    POTION_CLASS =  8,
    SCROLL_CLASS =  9,
    SPBOOK_CLASS = 10, /* actually SPELL-book */
    WAND_CLASS   = 11,
    COIN_CLASS   = 12,
    GEM_CLASS    = 13,
    ROCK_CLASS   = 14,
    BALL_CLASS   = 15,
    CHAIN_CLASS  = 16,
    VENOM_CLASS  = 17,

    MAXOCLASSES  = 18
};

#define ALLOW_COUNT (MAXOCLASSES + 1) /* Can be used in the object class    */
#define ALL_CLASSES (MAXOCLASSES + 2) /* input to getobj().                 */
#define ALLOW_NONE  (MAXOCLASSES + 3)

#define BURNING_OIL   (MAXOCLASSES + 1) /* Can be used as input to explode.           */
#define MON_EXPLODE   (MAXOCLASSES + 2) /* Exploding monster (e.g. gas spore)         */
#define TRAPPED_DOOR  (MAXOCLASSES + 3) /* Exploding booby-trapped doors (GruntHack)  */
#define MON_CASTBALL  (MAXOCLASSES + 4) /* For monsters casting area-effect spells    */
#define FORGE_EXPLODE (MAXOCLASSES + 5) /* Exploding forges                           */
#define TRAP_EXPLODE  (MAXOCLASSES + 6) /* Exploding magical trap due to cancellation */

#define SHK_NOMATCH     0       /* Shk !know this class of object       */
#define SHK_MATCH       1       /* Shk is expert                        */
#define SHK_GENERAL     2       /* Shk runs a general store             */

/*
 * FUNCTION shk_class_match
 *
 * Return TRUE if a object class matches the shop type.
 * I.e. shk_class_match(WEAPON_CLASS, shkp)
 *
 * Return:      SHK_MATCH, SHK_NOMATCH, SHK_GENERAL
 */

#define shk_class_match(class, shkp) \
        ((shtypes[ESHK(shkp)->shoptype-SHOPBASE].symb == RANDOM_CLASS) \
         ? SHK_GENERAL               \
         : ((shtypes[ESHK(shkp)->shoptype-SHOPBASE].symb == class)      \
                ? SHK_MATCH          \
                : SHK_NOMATCH))
                                    /* Menu keys */
#define SHK_ID_BASIC    0x00000001L /* i */
#define SHK_ID_PREMIUM  0x00000002L /* I */
#define SHK_ID_WEAPON   0x00000004L /* ) */
#define SHK_ID_ARMOR    0x00000008L /* [ */
#define SHK_ID_SCROLL   0x00000010L /* ? */
#define SHK_ID_BOOK     0x00000020L /* + */
#define SHK_ID_POTION   0x00000040L /* ! */
#define SHK_ID_RING     0x00000080L /* = */
#define SHK_ID_AMULET   0x00000100L /* " */
#define SHK_ID_WAND     0x00000200L /* / */
#define SHK_ID_TOOL     0x00000400L /* ( */
#define SHK_ID_GEM      0x00000800L /* * */
#define SHK_ID_FOOD     0x00001000L /* % */
#define SHK_UNCURSE     0x00002000L /* u */
#define SHK_WEP_FIX     0x00004000L /* f */
#define SHK_WEP_ENC     0x00008000L /* e */
#define SHK_WEP_POI     0x00010000L /* p */
#define SHK_ARM_FIX     0x00020000L /* f */
#define SHK_ARM_ENC     0x00040000L /* e */
#define SHK_CHG_BAS     0x00080000L /* c */
#define SHK_CHG_PRE     0x00100000L /* C */
#define SHK_RUMOR       0x00200000L /* r */

#if 0 /* moved to decl.h so that makedefs.c won't see them */
extern const struct class_sym
        def_oc_syms[MAXOCLASSES];       /* default class symbols */
extern uchar oc_syms[MAXOCLASSES];      /* current class symbols */
#endif

/* Default definitions of all object-symbols (must match classes above). */

#define ILLOBJ_SYM ']' /* also used for mimics */
#define WEAPON_SYM ')'
#define ARMOR_SYM '['
#define RING_SYM '='
#define AMULET_SYM '"'
#define TOOL_SYM '('
#define FOOD_SYM '%'
#define POTION_SYM '!'
#define SCROLL_SYM '?'
#define SPBOOK_SYM '+'
#define WAND_SYM '/'
#define GOLD_SYM '$'
#define GEM_SYM '*'
#define ROCK_SYM '`'
#define BALL_SYM '0'
#define CHAIN_SYM '_'
#define VENOM_SYM '*'

struct fruit {
    char fname[PL_FSIZ];
    int fid;
    struct fruit *nextf;
};
#define newfruit() (struct fruit *) alloc(sizeof(struct fruit))
#define dealloc_fruit(rind) free((genericptr_t)(rind))

#define OBJ_NAME(obj) (obj_descr[(obj).oc_name_idx].oc_name)
#define OBJ_DESCR(obj) (obj_descr[(obj).oc_descr_idx].oc_descr)
#endif /* OBJCLASS_H */
