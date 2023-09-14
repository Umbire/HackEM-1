/* NetHack 3.6	timeout.c	$NHDT-Date: 1573290422 2019/11/09 09:07:02 $  $NHDT-Branch: NetHack-3.6 $:$NHDT-Revision: 1.93 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2018. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "lev.h" /* for checking save modes */

STATIC_DCL void NDECL(stoned_dialogue);
STATIC_DCL void NDECL(phasing_dialogue);
STATIC_DCL void NDECL(vomiting_dialogue);
STATIC_DCL void NDECL(choke_dialogue);
STATIC_DCL void NDECL(levitation_dialogue);
STATIC_DCL void NDECL(slime_dialogue);
STATIC_DCL void FDECL(slimed_to_death, (struct kinfo *));
STATIC_DCL void NDECL(sickness_dialogue);
STATIC_DCL void NDECL(phaze_dialogue);
STATIC_DCL void FDECL(done_timeout, (int, int));
STATIC_DCL void NDECL(slip_or_trip);
STATIC_DCL void FDECL(see_lamp_flicker, (struct obj *, const char *));
STATIC_DCL void FDECL(lantern_message, (struct obj *));
STATIC_DCL void FDECL(cleanup_burn, (ANY_P *, long));

/* used by wizard mode #timeout and #wizintrinsic; order by 'interest'
   for timeout countdown, where most won't occur in normal play */
const struct propname {
    int prop_num;
    const char *prop_name;
} propertynames[] = {
    { INVULNERABLE, "invulnerable" },
    { STONED, "petrifying" },
    { LARVACARRIER, "hosting monster eggs" },
    { SLIMED, "becoming slime" },
    { STRANGLED, "strangling" },
    { SICK, "fatally sick" },
    { STUNNED, "stunned" },
    { SLOW, "slow" },
    { CONFUSION, "confused" },
    { AFRAID, "afraid" },
    { HALLUC, "hallucinating" },
    { BLINDED, "blinded" },
    { DEAF, "deafness" },
    { VOMITING, "vomiting" },
    { GLIB, "slippery fingers" },
    { WOUNDED_LEGS, "wounded legs" },
    { SLEEPY, "sleepy" },
    { TELEPORT, "teleporting" },
    { POLYMORPH, "polymorphing" },
    { LEVITATION, "levitating" },
    { FAST, "very fast" }, /* timed 'FAST' is very fast */
    { CLAIRVOYANT, "clairvoyant" },
    { DETECT_MONSTERS, "monster detection" },
    { SEE_INVIS, "see invisible" },
    { INVIS, "invisible" },
    /* properties beyond here don't have timed values during normal play,
       so there's not much point in trying to order them sensibly;
       they're either on or off based on equipment, role, actions, &c */
    { FIRE_RES, "fire resistance" },
    { COLD_RES, "cold resistance" },
    { SLEEP_RES, "sleep resistance" },
    { DISINT_RES, "disintegration resistance" },
    { SHOCK_RES, "shock resistance" },
    { POISON_RES, "poison resistance" },
    { ACID_RES, "acid resistance" },
    { STONE_RES, "stoning resistance" },
    { PSYCHIC_RES, "psionic resistance" },
    { DRAIN_RES, "drain resistance" },
    { STUN_RES, "stun resistance" },
    { SICK_RES, "sickness resistance" },
    { DEATH_RES, "death magic resistance" },
    { ANTIMAGIC, "magic resistance" },
    { HALLUC_RES, "hallucination resistance" },
    { FUMBLING, "fumbling" },
    { HUNGER, "voracious hunger" },
    { FOOD_SENSE, "recognizes detrimental food" },
    { TELEPAT, "telepathic" },
    { WARNING, "warning" },
    { WARN_OF_MON, "warn: monster type or class" },
    { WARN_UNDEAD, "warn: undead" },
    { SEARCHING, "searching" },
    { INFRAVISION, "infravision" },
    { ADORNED, "adorned (+/- Cha)" },
    { DISPLACED, "displaced" },
    { STEALTH, "stealthy" },
    { AGGRAVATE_MONSTER, "monster aggravation" },
    { CONFLICT, "conflict" },
    { JUMPING, "jumping" },
    { TELEPORT_CONTROL, "teleport control" },
    { FLYING, "flying" },
    { WWALKING, "water walking" },
    { SWIMMING, "swimming" },
    { MAGICAL_BREATHING, "magical breathing" },
    { PASSES_WALLS, "phasing" },
    { SLOW_DIGESTION, "slow digestion" },
    { HALF_SPDAM, "half spell damage" },
    { HALF_PHDAM, "half physical damage" },
    { REGENERATION, "HP regeneration" },
    { ENERGY_REGENERATION, "energy regeneration" },
    { PROTECTION, "extra protection" },
    { PROT_FROM_SHAPE_CHANGERS, "protection from shape changers" },
    { POLYMORPH_CONTROL, "polymorph control" },
    { UNCHANGING, "unchanging" },
    { REFLECTING, "reflecting" },
    { FREE_ACTION, "free action" },
    { FIXED_ABIL, "fixed abilities" },
    { WITHERING, "withering away" },
    { STABLE,    "extraodinarily stable" },
    { LIFESAVED, "life will be saved" },
    {  0, 0 },
};

/* He is being petrified - dialogue by inmet!tower */
static NEARDATA const char *const stoned_texts[] = {
    "You are slowing down.",                       /* 5 */
    "Your limbs are stiffening.",                  /* 4 */
    "Your limbs have turned to stone.",            /* 3 */
    "You have almost completely turned to stone.", /* 2 */
    "You are a statue."                            /* 1 */
};

STATIC_OVL void
stoned_dialogue()
{
    register long i = (Stoned & TIMEOUT);

    if (i > 0L && i <= SIZE(stoned_texts)) {
        char buf[BUFSZ];

        Strcpy(buf, stoned_texts[SIZE(stoned_texts) - i]);
        if (nolimbs(youmonst.data) && strstri(buf, "limbs"))
            (void) strsubst(buf, "limbs", "extremities");
        pline1(buf);
    }
    switch ((int) i) {
    case 5: /* slowing down */
        HFast = 0L;
        if (multi > 0)
            nomul(0);
        break;
    case 4: /* limbs stiffening */
        /* just one move left to save oneself so quit fiddling around;
           don't stop attempt to eat tin--might be lizard or acidic */
        if (!Popeye(STONED))
            stop_occupation();
        if (multi > 0)
            nomul(0);
        break;
    case 3: /* limbs turned to stone */
        stop_occupation();
        nomul(-3); /* can't move anymore */
        multi_reason = "getting stoned";
        nomovemsg = You_can_move_again; /* not unconscious */
        /* "your limbs have turned to stone" so terminate wounded legs */
        if (Wounded_legs && !u.usteed)
            heal_legs(2);
        break;
    case 2: /* turned to stone */
        if ((HDeaf & TIMEOUT) > 0L && (HDeaf & TIMEOUT) < 5L)
            set_itimeout(&HDeaf, 5L); /* avoid Hear_again at tail end */
        /* if also vomiting or turning into slime, stop those (no messages) */
        if (Vomiting)
            make_vomiting(0L, FALSE);
        if (Slimed)
            make_slimed(0L, (char *) 0);
        break;
    default:
        break;
    }
    exercise(A_DEX, FALSE);
}

STATIC_OVL void
phasing_dialogue()
{
    if (HPasses_walls == 15) {
        if (!Hallucination) {
            Your("body is beginning to feel more solid.");
        } else {
            You_feel("more distant from the spirit world.");
        }
        stop_occupation();
    } else if (HPasses_walls == 1) {
        if (!Hallucination) {
            Your("body is solid again.");
        } else {
            You_feel("totally separated from the spirit world.");
        }
        stop_occupation();
    }
}

/* hero is getting sicker and sicker prior to vomiting */
static NEARDATA const char *const vomiting_texts[] = {
    "are feeling mildly nauseated.", /* 14 */
    "feel slightly confused.",       /* 11 */
    "can't seem to think straight.", /* 8 */
    "feel incredibly sick.",         /* 5 */
    "are about to vomit."            /* 2 */
};

STATIC_OVL void
vomiting_dialogue()
{
    const char *txt = 0;
    long v = (Vomiting & TIMEOUT);

    /* note: nhtimeout() hasn't decremented timed properties for the
       current turn yet, so we use Vomiting-1 here */
    switch ((int) (v - 1L)) {
    case 14:
        txt = vomiting_texts[0];
        break;
    case 11:
        txt = vomiting_texts[1];
        break;
    case 6:
        make_stunned((HStun & TIMEOUT) + (long) d(2, 4), FALSE);
        if (!Popeye(VOMITING))
            stop_occupation();
    /*FALLTHRU*/
    case 9:
        make_confused((HConfusion & TIMEOUT) + (long) d(2, 4), FALSE);
        if (multi > 0)
            nomul(0);
        break;
    case 8:
        txt = vomiting_texts[2];
        break;
    case 5:
        txt = vomiting_texts[3];
        break;
    case 2:
        txt = vomiting_texts[4];
        if (cantvomit(youmonst.data))
            txt = "gag uncontrollably.";
        else if (Hallucination)
            /* "hurl" is short for "hurl chunks" which is slang for
               relatively violent vomiting... */
            txt = "are about to hurl!";
        break;
    case 0:
        stop_occupation();
        if (!cantvomit(youmonst.data)) {
            morehungry(20);
            /* case 2 used to be "You suddenly vomit!" but it wasn't sudden
               since you've just been through the earlier messages of the
               countdown, and it was still possible to move around between
               that message and "You can move again." (from vomit()'s
               nomul(-2)) with no intervening message; give one here to
               have more specific point at which hero became unable to move
               [vomit() issues its own message for the cantvomit() case
               and for the FAINTING-or-worse case where stomach is empty] */
            if (u.uhs < FAINTING)
                You("%s!", !Hallucination ? "vomit" : "hurl chunks");
        }
        vomit();
        break;
    default:
        break;
    }
    if (txt)
        You1(txt);
    exercise(A_CON, FALSE);
}

static NEARDATA const char *const choke_texts[] = {
    "You find it hard to breathe.",
    "You're gasping for air.",
    "You can no longer breathe.",
    "You're turning %s.",
    "You suffocate."
};

static NEARDATA const char *const choke_texts2[] = {
    "Your %s is becoming constricted.",
    "Your blood is having trouble reaching your brain.",
    "The pressure on your %s increases.",
    "Your consciousness is fading.",
    "You suffocate."
};

STATIC_OVL void
choke_dialogue()
{
    register long i = (Strangled & TIMEOUT);

    if (i > 0 && i <= SIZE(choke_texts)) {
        if (Breathless || !rn2(50))
            pline(choke_texts2[SIZE(choke_texts2) - i], body_part(NECK));
        else {
            const char *str = choke_texts[SIZE(choke_texts) - i];

            if (index(str, '%'))
                pline(str, hcolor(NH_BLUE));
            else
                pline1(str);
        }
    }
    exercise(A_STR, FALSE);
}

static NEARDATA const char *const levi_texts[] = {
    "You float slightly lower.",
    "You wobble unsteadily %s the %s."
};

STATIC_OVL void
levitation_dialogue()
{
    /* -1 because the last message comes via float_down() */
    long i = (((HLevitation & TIMEOUT) - 1L) / 2L);

    if (ELevitation)
        return;

    if (!ACCESSIBLE(levl[u.ux][u.uy].typ)
        && !is_pool_or_lava(u.ux,u.uy))
        return;

    if (((HLevitation & TIMEOUT) % 2L) && i > 0L && i <= SIZE(levi_texts)) {
        const char *s = levi_texts[SIZE(levi_texts) - i];

        if (index(s, '%')) {
            boolean danger = (is_pool_or_lava(u.ux, u.uy)
                              && !Is_waterlevel(&u.uz));

            pline(s, danger ? "over" : "in",
                  danger ? surface(u.ux, u.uy) : "air");
        } else
            pline1(s);
    }
}

static NEARDATA const char *const larva_texts[] = {
    "You are feeling a little strange.",
    "Your skin is crawling.",
    "You can feel something moving inside your body!",
    "%s burst from your body!"
};

static void
larva_dialogue()
{
    register long i = (LarvaCarrier & TIMEOUT) / 2L;

    if (((LarvaCarrier & TIMEOUT) % 2L) && i >= 0L && i < SIZE(larva_texts)) {
        char buf[BUFSZ];

        Strcpy(buf, larva_texts[SIZE(larva_texts) - i - 1L]);
        if (index(buf, '%')) {
            pline(buf,
                  makeplural(Hallucination ? rndmonnam(NULL) : "Insect"));
        } else
            pline1(buf);
    }
    if (i <= 4L) {
        stop_occupation();
    }
    exercise(A_CON, FALSE);
}

static NEARDATA const char *const slime_texts[] = {
    "You are turning a little %s.",   /* 5 */
    "Your limbs are getting oozy.",   /* 4 */
    "Your skin begins to peel away.", /* 3 */
    "You are turning into %s.",       /* 2 */
    "You have become %s."             /* 1 */
};

STATIC_OVL void
slime_dialogue()
{
    register long i = (Slimed & TIMEOUT) / 2L;

    if (i == 1L) {
        /* display as green slime during "You have become green slime."
           but don't worry about not being able to see self; if already
           mimicking something else at the time, implicitly be revealed */
        youmonst.m_ap_type = M_AP_MONSTER;
        youmonst.mappearance = PM_GREEN_SLIME;
    }
    if (((Slimed & TIMEOUT) % 2L) && i >= 0L && i < SIZE(slime_texts)) {
        char buf[BUFSZ];

        Strcpy(buf, slime_texts[SIZE(slime_texts) - i - 1L]);
        if (nolimbs(youmonst.data) && strstri(buf, "limbs"))
            (void) strsubst(buf, "limbs", "extremities");

        if (index(buf, '%')) {
            if (i == 4L) {  /* "you are turning green" */
                if (!Blind) /* [what if you're already green?] */
                    pline(buf, hcolor(NH_GREEN));
            } else
                pline(buf,
                      an(Hallucination ? rndmonnam(NULL) : "green slime"));
        } else
            pline1(buf);
    }

    switch (i) {
    case 3L:  /* limbs becoming oozy */
        HFast = 0L; /* lose intrinsic speed */
        if (!Popeye(SLIMED))
            stop_occupation();
        if (multi > 0)
            nomul(0);
        break;
    case 2L: /* skin begins to peel */
        if ((HDeaf & TIMEOUT) > 0L && (HDeaf & TIMEOUT) < 5L)
            set_itimeout(&HDeaf, 5L); /* avoid Hear_again at tail end */
        break;
    case 1L: /* turning into slime */
        /* if also turning to stone, stop doing that (no message) */
        if (Stoned)
            make_stoned(0L, (char *) 0, KILLED_BY_AN, (char *) 0);
        break;
    }
    exercise(A_DEX, FALSE);
}

void
burn_away_slime()
{
    if (Slimed) {
        make_slimed(0L, "The slime that covers you is burned away!");
    }
}

/* countdown timer for turning into green slime has run out; kill our hero */
STATIC_OVL void
slimed_to_death(kptr)
struct kinfo *kptr;
{
    uchar save_mvflags;

    /* redundant: polymon() cures sliming when polying into green slime */
    if (Upolyd && youmonst.data == &mons[PM_GREEN_SLIME]) {
        dealloc_killer(kptr);
        return;
    }
    /* more sure killer reason is set up */
    if (kptr && kptr->name[0]) {
        killer.format = kptr->format;
        Strcpy(killer.name, kptr->name);
    } else {
        killer.format = NO_KILLER_PREFIX;
        Strcpy(killer.name, "turned into green slime");
    }
    dealloc_killer(kptr);

    /*
     * Polymorph into a green slime, which might destroy some worn armor
     * (potentially affecting bones) and dismount from steed.
     * Can't be Unchanging; wouldn't have turned into slime if we were.
     * Despite lack of Unchanging, neither done() nor savelife() calls
     * rehumanize() if hero dies while polymorphed.
     * polymon() undoes the slime countdown's mimick-green-slime hack
     * but does not perform polyself()'s light source bookkeeping.
     * No longer need to manually increment uconduct.polyselfs to reflect
     * [formerly implicit] change of form; polymon() takes care of that.
     * Temporarily ungenocide if necessary.
     */
    if (emits_light(youmonst.data))
        del_light_source(LS_MONSTER, monst_to_any(&youmonst));
    save_mvflags = mvitals[PM_GREEN_SLIME].mvflags;
    mvitals[PM_GREEN_SLIME].mvflags = save_mvflags & ~G_GENOD;
    /* become a green slime; also resets youmonst.m_ap_type+.mappearance */
    (void) polymon(PM_GREEN_SLIME);
    mvitals[PM_GREEN_SLIME].mvflags = save_mvflags;
    done_timeout(TURNED_SLIME, SLIMED);

    /* life-saved; even so, hero still has turned into green slime;
       player may have genocided green slimes after being infected */
    if ((mvitals[PM_GREEN_SLIME].mvflags & G_GENOD) != 0) {
        char slimebuf[BUFSZ];

        killer.format = KILLED_BY;
        Strcpy(killer.name, "slimicide");
        /* vary the message depending upon whether life-save was due to
           amulet or due to declining to die in explore or wizard mode */
        Strcpy(slimebuf, "green slime has been genocided...");
        if (iflags.last_msg == PLNMSG_OK_DONT_DIE)
            /* follows "OK, so you don't die." and arg is second sentence */
            pline("Yes, you do.  %s", upstart(slimebuf));
        else
            /* follows "The medallion crumbles to dust." */
            pline("Unfortunately, %s", slimebuf);
        /* die again; no possibility of amulet this time */
        done(GENOCIDED); /* [should it be done_timeout(GENOCIDED, SLIMED)?] */
        /* could be life-saved again (only in explore or wizard mode)
           but green slimes are gone; just stay in current form */
    }
    return;
}

static NEARDATA const char *const sickness_texts[] = {
    "Your illness feels worse.",
    "Your illness is severe.",
    "You are at Death's door.",
};

STATIC_OVL void
sickness_dialogue()
{
    long j = (Sick & TIMEOUT), i = j / 2L;

    if (i > 0L && i <= SIZE(sickness_texts) && (j % 2) != 0) {
        char buf[BUFSZ], pronounbuf[40];

        Strcpy(buf, sickness_texts[SIZE(sickness_texts) - i]);
        /* change the message slightly for food poisoning */
        if ((u.usick_type & SICK_NONVOMITABLE) == 0)
            (void) strsubst(buf, "illness", "sickness");
        /* change final message slightly for zombie sickness */
        if ((u.usick_type & SICK_ZOMBIE) != 0
            && !(Race_if(PM_ILLITHID) || Race_if(PM_TORTLE)))
            (void) strsubst(buf, "You are at Death's door",
                            "You feel a horrifying change coming over you");
        if (Hallucination && strstri(buf, "Death's door")) {
            /* youmonst: for Hallucination, mhe()'s mon argument isn't used */
            Strcpy(pronounbuf, mhe(&youmonst));
            Sprintf(eos(buf), "  %s %s inviting you in.",
                    /* upstart() modifies its argument but vtense() doesn't
                       care whether or not that has already happened */
                    upstart(pronounbuf), vtense(pronounbuf, "are"));
        }
        pline("%s", buf);
    }
    exercise(A_CON, FALSE);
}

/* Intrinsic Passes_walls is temporary when your god is trying to fix
   all troubles and then TROUBLE_STUCK_IN_WALL calls safe_teleds() but
   it can't find anywhere to place you.  If that happens you get a small
   value for (HPasses_walls & TIMEOUT) to move somewhere yourself.
   Message given is "you feel much slimmer" as a joke hint that you can
   move between things which are closely packed--like the substance of
   solid rock! */
static NEARDATA const char *const phaze_texts[] = {
    "You start to feel bloated.",
    "You are feeling rather flabby.",
};

STATIC_OVL void
phaze_dialogue()
{
    long i = ((HPasses_walls & TIMEOUT) / 2L);

    if (EPasses_walls || (HPasses_walls & ~TIMEOUT))
        return;

    if (((HPasses_walls & TIMEOUT) % 2L) && i > 0L && i <= SIZE(phaze_texts))
        pline1(phaze_texts[SIZE(phaze_texts) - i]);
}

/* when a status timeout is fatal, keep the status line indicator shown
   during end of game rundown (and potential dumplog);
   timeout has already counted down to 0 by the time we get here */
STATIC_OVL void
done_timeout(how, which)
int how, which;
{
    long *intrinsic_p = &u.uprops[which].intrinsic;

    *intrinsic_p |= I_SPECIAL; /* affects final disclosure */
    done(how);

    /* life-saved */
    *intrinsic_p &= ~I_SPECIAL;
    context.botl = TRUE;
}

void
nh_timeout()
{
    register struct prop *upp;
    struct kinfo *kptr;
    boolean was_flying;
    int sleeptime;
    int m_idx;
    int baseluck = (flags.moonphase == FULL_MOON) ? 1 : 0;

    if (flags.friday13)
        baseluck -= 1;
    if (Role_if(PM_ARCHEOLOGIST) && uarmh && uarmh->otyp == FEDORA)
        baseluck += lucky_fedora();
    if (quest_status.killed_leader)
        baseluck -= 4;
    if (flags.quest_boon)
        baseluck += 3;
    
    if (u.uluck != baseluck) {
	int timeout = 600;
	int time_luck = stone_luck(FALSE);
	/* Cursed luckstones slow bad luck timing out; blessed luckstones
	 * slow good luck timing out; normal luckstones slow both;
	 * neither is affected if you don't have a luckstone.
         * Luck is based at 0 usually, +1 if a full moon and -1 on Friday 13th
         */
	if (has_luckitem() && (!time_luck
            || (time_luck > 0 && u.uluck > baseluck)
            || (time_luck < 0 && u.uluck < baseluck))) {

	/* The slowed timeout depends on the distance between your
	 * luck (not including luck bonuses) and your base luck.
	 *
	 * distance	timeout
	 * --------------------
	 *  1		24800
	 *  2		24200
	 *  3		23200
	 *  4		21800
	 *  5		20000
	 *  6		17800
	 *  7		15200
	 *  8		12200
	 *  9		8800
	 *  10		5000
	 *  11		800
	 */
	int base_dist = u.uluck - baseluck;
	int slow_timeout = 25000 - 200 * (base_dist * base_dist);
	if (slow_timeout > timeout) timeout = slow_timeout;
	}

	if (u.uhave.amulet || u.ugangr) timeout = timeout / 2;

	if (moves >= u.luckturn + timeout) {
	    if (u.uluck > baseluck)
		u.uluck--;
	    else if (u.uluck < baseluck)
		u.uluck++;
            u.luckturn = moves;
	}
    }
    
    /* WAC -- check for timeout of specials */
	tech_timeout();

    if (HPasses_walls)
        phasing_dialogue();
    if (u.uinvulnerable)
        return; /* things past this point could kill you */
    if (Stoned)
        stoned_dialogue();
    if (LarvaCarrier)
        larva_dialogue();
    if (Slimed)
        slime_dialogue();
    if (Vomiting)
        vomiting_dialogue();
    if (Strangled)
        choke_dialogue();
    if (Sick)
        sickness_dialogue();
    if (HLevitation & TIMEOUT)
        levitation_dialogue();
    if (HPasses_walls & TIMEOUT)
        phaze_dialogue();
    if (u.mtimedone && !--u.mtimedone) {
        if (Unchanging ||
            (ublindf && ublindf->otyp == MASK))
            u.mtimedone = rnd(100 * youmonst.data->mlevel + 1);
        else if (is_were(youmonst.data))
            you_unwere(FALSE); /* if polycontrl, asks whether to rehumanize */
        else
            rehumanize();
    }
    if (u.ucreamed)
        u.ucreamed--;

    /* Dissipate spell-based protection. */
    if (u.usptime) {
        if (--u.usptime == 0 && u.uspellprot) {
            u.usptime = u.uspmtime;
            u.uspellprot--;
            find_ac();
            if (!Blind)
                Norep("The %s haze around you %s.", hcolor(NH_GOLDEN),
                      u.uspellprot ? "becomes less dense" : "disappears");
        }
    }

    /* Give a small warning that spell-based reflection is running out. */
    if (HReflecting == 20 && !Blind)
        pline_The("shimmering globe around you is starting to fade.");

    if (u.ugallop) {
        if (--u.ugallop == 0L && u.usteed)
            pline("%s stops galloping.", Monnam(u.usteed));
    }

    if (Hidinshell && --u.uinshell == 1) {
        toggleshell();
        nomul(0);
    } else if (u.uinshell < 0) {
        u.uinshell++;
    }
    
    was_flying = Flying;
    for (upp = u.uprops; upp < u.uprops + SIZE(u.uprops); upp++)
        if (!(upp->intrinsic & HAVEPARTIAL) /* partial intrinsics do not time out */
            && (upp->intrinsic & TIMEOUT)
            && !(--upp->intrinsic & TIMEOUT)) {
            kptr = find_delayed_killer((int) (upp - u.uprops));
            switch (upp - u.uprops) {
            case STONED:
                if (kptr && kptr->name[0]) {
                    killer.format = kptr->format;
                    Strcpy(killer.name, kptr->name);
                } else {
                    killer.format = NO_KILLER_PREFIX;
                    Strcpy(killer.name, "killed by petrification");
                }
                dealloc_killer(kptr);
                /* (unlike sliming, you aren't changing form here) */
                done_timeout(STONING, STONED);
                break;
            case LARVACARRIER:
                /* must be in this order for bones files. */
                create_critters(rnd(3), &mons[u.ueggpm], TRUE);
                if (u.ueggpm == PM_GIBBERSLUG)
                    losehp(d(4, 5), "being eaten from the inside by slugs", KILLED_BY);
                else
                    losehp(d(1, 5), "being eaten from the inside by insects", KILLED_BY);
                
                u.uhp = (int) (u.uhp / 2);
                break;
            case SLIMED:
                /* involuntarily break "never changed form" conduct */
                if (!u.uconduct.polyselfs++)
                    livelog_write_string(LL_CONDUCT,
                            "changed form for the first time by turning to slime");
                slimed_to_death(kptr); /* done_timeout(TURNED_SLIME,SLIMED) */
                break;
            case VOMITING:
                if (uamul && uamul->otyp == AMULET_OF_NAUSEA) {
                    make_vomiting((long) rnd(100), FALSE);
                } else  {
                    make_vomiting(0L, TRUE);
                } break;
            case SICK:
                if ((u.usick_type & SICK_NONVOMITABLE) == 0)
                    You("die from your sickness.");
                else
                    You("die from your illness.");
                if (kptr && kptr->name[0]) {
                    killer.format = kptr->format;
                    Strcpy(killer.name, kptr->name);
                } else {
                    killer.format = KILLED_BY_AN;
                    killer.name[0] = 0; /* take the default */
                }
                dealloc_killer(kptr);

                if ((m_idx = name_to_mon(killer.name)) >= LOW_PM) {
                    if (type_is_pname(&mons[m_idx])) {
                        killer.format = KILLED_BY;
                    } else if (mons[m_idx].geno & G_UNIQ) {
                        Strcpy(killer.name, the(killer.name));
                        killer.format = KILLED_BY;
                    }
                }

                if (!!(u.usick_type & SICK_ZOMBIE)) {
                    if (Race_if(PM_ILLITHID) || Race_if(PM_TORTLE)) {
                        killer.format = NO_KILLER_PREFIX;
                        Sprintf(killer.name, "diseased by %s",
                                an(killer.name));
                    } else {
                        u.ugrave_arise = urace.zombienum;
                        killer.format = NO_KILLER_PREFIX;
                        Sprintf(killer.name, "zombified by %s",
                                an(killer.name));
                    }
                }
                done_timeout(POISONING, SICK);
                u.usick_type = 0;
                break;
            case WITHERING:
                You("are no longer withering away.");
                context.botl = TRUE;
                break;
            case STABLE:
                You("are a little less sure of your footing.");
                break;
            case FAST:
                if (!Very_fast)
                    You_feel("yourself slowing down%s.",
                             Fast ? " a bit" : "");
                break;
            case SLOW:
                HSlow &= ~FROMOUTSIDE;
                You_feel("less sluggish.");
                context.botl = TRUE;
                break;
            case REFLECTING:
                if (!Blind)
                    pline_The("shimmering globe around you flickers and vanishes.");
                else
                    You("don't feel very smooth anymore.");
                break;
            /* all these need to make sure the external intrinsic isn't there too */
            case VULN_FIRE:
                if (!Vulnerable_fire)
                    You("are no longer vulnerable to fire.");
                break;
            case VULN_COLD:
                if (!Vulnerable_cold)
                    You("are no longer vulnerable to cold.");
                break;
            case VULN_ELEC:
                if (!Vulnerable_elec)
                    You("are no longer vulnerable to electricity.");
                break;
            case VULN_ACID:
                if (!Vulnerable_acid)
                    You("are no longer vulnerable to acid.");
                break;
            case CONFUSION:
                /* So make_confused works properly */
                set_itimeout(&HConfusion, 1L);
                make_confused(0L, TRUE);
                if (!Confusion)
                    stop_occupation();
                break;
            case STUNNED:
                set_itimeout(&HStun, 1L);
                make_stunned(0L, TRUE);
                if (!Stunned)
                    stop_occupation();
                break;
            case AFRAID:
                set_itimeout(&HAfraid, 1L);
                make_afraid(0L, TRUE);
                if (!Afraid) {
                    stop_occupation();
                    You("are no longer afraid.");
                    remove_fearedmon();
                }
                break;
            case BLINDED:
                set_itimeout(&Blinded, 1L);
                make_blinded(0L, TRUE);
                if (!Blind)
                    stop_occupation();
                break;
            case DEAF:
                set_itimeout(&HDeaf, 1L);
                make_deaf(0L, TRUE);
                context.botl = TRUE;
                if (!Deaf)
                    stop_occupation();
                break;
            case INVIS:
                newsym(u.ux, u.uy);
                if (!Invis && !BInvis && !Blind) {
                    You(!See_invisible
                            ? "are no longer invisible."
                            : "can no longer see through yourself.");
                    stop_occupation();
                }
                break;
            case SEE_INVIS:
                set_mimic_blocking(); /* do special mimic handling */
                see_monsters();       /* make invis mons appear */
                newsym(u.ux, u.uy);   /* make self appear */
                stop_occupation();
                break;
            case WOUNDED_LEGS:
                heal_legs(0);
                stop_occupation();
                break;
            case HALLUC:
                set_itimeout(&HHallucination, 1L);
                (void) make_hallucinated(0L, TRUE, 0L);
                if (!Hallucination)
                    stop_occupation();
                break;
            case SLEEPY:
                if (unconscious() || how_resistant(SLEEP_RES) == 100) {
                    incr_itimeout(&HSleepy, rnd(100));
                } else if (Sleepy) {
                    You("fall asleep.");
                    sleeptime = resist_reduce(rnd(20), SLEEP_RES);
                    fall_asleep(-sleeptime, TRUE);
                    incr_itimeout(&HSleepy, sleeptime + rnd(100));
                }
                break;
            case LEVITATION:
                (void) float_down(I_SPECIAL | TIMEOUT, 0L);
                break;
            case FLYING:
                /* timed Flying is via #wizintrinsic only */
                if (was_flying && !Flying) {
                    context.botl = 1;
                    You("land.");
                    spoteffects(TRUE);
                }
                break;
            case DISPLACED:
                if (!Displaced) /* give a message */
                    toggle_displacement((struct obj *) 0, 0L, FALSE);
                break;
            case WARN_OF_MON:
                /* timed Warn_of_mon is via #wizintrinsic only */
                if (!Warn_of_mon) {
                    context.warntype.speciesidx = NON_PM;
                    if (context.warntype.species) {
                        You("are no longer warned about %s.",
                            makeplural(context.warntype.species->mname));
                        context.warntype.species = (struct permonst *) 0;
                    }
                }
                break;
            case PASSES_WALLS:
                if (!Passes_walls) {
                    if (stuck_in_wall())
                        You_feel("hemmed in again.");
                    else
                        pline("You're back to your %s self again.",
                              !Upolyd ? "normal" : "unusual");
                }
                break;
            case INVULNERABLE:
                if (!Invulnerable)
                    You("are no longer invulnerable.");
                break;
            case STRANGLED:
                if (u.uswallow) {
                    killer.format = NO_KILLER_PREFIX;
                    Sprintf(killer.name, "suffocated by %s",
                            x_monnam(u.ustuck, ARTICLE_A, (char *) 0,
                                     (SUPPRESS_IT | SUPPRESS_HALLUCINATION),
                                     FALSE));
                    done_timeout(DIED, STRANGLED);
                } else {
                    killer.format = KILLED_BY;
                    Strcpy(killer.name,
                           (u.uburied) ? "suffocation" : "strangulation");
                    done_timeout(DIED, STRANGLED);
                    /* must be declining to die in explore|wizard mode;
                       treat like being cured of strangulation by prayer */
                    if (uamul && uamul->otyp == AMULET_OF_STRANGULATION) {
                        Your("amulet vanishes!");
                        useup(uamul);
                    }
                }
                break;
            case FUMBLING:
                /* call this only when a move took place.  */
                /* otherwise handle fumbling msgs locally. */
                if (u.umoved && !Levitation) {
                    slip_or_trip();
                    nomul(-2);
                    multi_reason = "fumbling";
                    nomovemsg = "";
                    /* The more you are carrying the more likely you
                     * are to make noise when you fumble.  Adjustments
                     * to this number must be thoroughly play tested.
                     */
                    if ((inv_weight() > -500)) {
                        (Hallucination) ? You("forget the rules to Charades") 
                            : You("make a lot of noise!");
                        wake_nearby();
                    }
                }
                /* from outside means slippery ice; don't reset
                   counter if that's the only fumble reason */
                HFumbling &= ~FROMOUTSIDE;
                if (Fumbling)
                    incr_itimeout(&HFumbling, rnd(20));
                break;
            case DETECT_MONSTERS:
                see_monsters();
                break;
            case GLIB:
                make_glib(0); /* might update persistent inventory */
                break;
            }
        }

    run_timers();
}

void
fall_asleep(how_long, wakeup_msg)
int how_long;
boolean wakeup_msg;
{
    if (how_long == 0)
        return;

    stop_occupation();
    nomul(how_long);
    multi_reason = "sleeping";
    /* generally don't notice sounds while sleeping */
    if (wakeup_msg && multi == how_long) {
        /* caller can follow with a direct call to Hear_again() if
           there's a need to override this when wakeup_msg is true */
        incr_itimeout(&HDeaf, how_long);
        context.botl = TRUE;
        afternmv = Hear_again; /* this won't give any messages */
    }
    /* early wakeup from combat won't be possible until next monster turn */
    u.usleep = monstermoves;
    nomovemsg = wakeup_msg ? "You wake up." : You_can_move_again;
}

/* Attach an explosion timeout to a given explosive device */
void
attach_bomb_blow_timeout(bomb, fuse, yours)
struct obj *bomb;
int fuse;
boolean yours;
{
    long expiretime;

    if (bomb->cursed && !rn2(2))
        return; /* doesn't arm if not armed */

    /* Now if you play with other people's property... */
    if (yours && (!carried(bomb)
            && costly_spot(bomb->ox, bomb->oy) 
            && (!bomb->no_charge || bomb->unpaid))) {
        verbalize("You play with it, you pay for it!");
        bill_dummy_object(bomb);
    }

    expiretime = stop_timer(BOMB_BLOW, obj_to_any(bomb));

    if (expiretime > 0L) 
        fuse = fuse - (expiretime - monstermoves);
    bomb->yours = yours;
    bomb->oarmed = TRUE;

    (void) start_timer((long)fuse, TIMER_OBJECT, BOMB_BLOW, obj_to_any(bomb));
}


/* Attach an fade timeout to a given spirit */
void
attach_spirit_fade_timeout(spirit, when)
struct obj *spirit;
long when;
{
    int i;
    /* stop previous timer, if any */
    (void) stop_timer(SPIRIT_FADE, obj_to_any(spirit));
    
    if (!when) {
        for (i = (MIN_SPIRIT_FADE_TIME) + 1; i <= MAX_SPIRIT_FADE_TIME; i++)
            if (rnd(i) > 15) {
                /* spirit will fade */
                when = (long) i;
                break;
            }
    }
    
    if (when) {
        (void) start_timer(when, TIMER_OBJECT, SPIRIT_FADE, obj_to_any(spirit));
    }
}


/* timer callback routine: detonate the explosives */
void
bomb_blow(arg, timeout)
anything *arg;
long timeout;
{
    struct obj *bomb;
    xchar x,y;
    boolean silent, underwater;
    struct monst *mtmp = (struct monst *)0;

    bomb = arg->a_obj;

    silent = (timeout != monstermoves);     /* exploded while away */

    if (get_obj_location(bomb, &x, &y, BURIED_TOO | CONTAINED_TOO)) {
        switch (bomb->where) {
        case OBJ_MINVENT:
            mtmp = bomb->ocarry;
            if (bomb == MON_WEP(mtmp)) {
                bomb->owornmask &= ~W_WEP;
                MON_NOWEP(mtmp);
            }
            if (!silent) {
                if (canseemon(mtmp))
                    You_see("%s engulfed in an explosion!", mon_nam(mtmp));
            }
            
            if (damage_mon(mtmp, d(2, 5), AD_PHYS)) {
                if (!bomb->yours)
                    monkilled(mtmp, (silent ? "" : "explosion"), AD_PHYS);
                else
                    xkilled(mtmp, !silent);
            }
            break;
        case OBJ_INVENT:
            /* This shouldn't be silent! */
            pline("Something explodes inside your knapsack!");

            if (bomb == uwep) {
                uwepgone();
                stop_occupation();
            } else if (bomb == uswapwep) {
                uswapwepgone();
                stop_occupation();
            } else if (bomb == uquiver) {
                uqwepgone();
                stop_occupation();
            }
            losehp(d(2, 5), "carrying live explosives", KILLED_BY);
            break;
        case OBJ_FLOOR:
            underwater = is_pool(x, y);
            if (!silent) {
                if (x == u.ux && y == u.uy) {
                    if (underwater && (Flying || Levitation))
                        pline_The("water boils beneath you.");
                    else if (underwater && Wwalking)
                        pline_The("water erupts around you.");
                    else
                        pline("A bomb explodes under your %s!",
                              makeplural(body_part(FOOT)));
                    
                    losehp(d(2, 5), "enfulfed in an explosion", KILLED_BY);
                } else if (cansee(x, y)) {
                    You(underwater ? "see a plume of water shoot up."
                                   : "see a bomb explode.");
                }
            }
            break;
        default: /* Buried, contained, etc. */
            if (!silent)
                You_hear("a muffled explosion.");
            goto free_bomb;
            break;
        }
        bomb_explode(bomb, x, y, bomb->yours);

free_bomb:
        if (carried(bomb)) {
            useup(bomb);
        } else if (bomb) {
            obj_extract_self(bomb);
            obfree(bomb, (struct obj *)0);
        }
        maybe_unhide_at(x, y);
        newsym(x, y);
        return;
    } /* Migrating bombs "blow up in midair" */
}


/* timer callback routine: fade the spirit */
void
spirit_fade(arg, timeout)
anything *arg;
long timeout;
{
    struct obj *spirit;
    struct monst *mon;
    xchar x, y;
    boolean cansee_fadespot = FALSE;
    boolean silent = (timeout != monstermoves);     /* exploded while away */
    spirit = arg->a_obj;
    
    if (get_obj_location(spirit, &x, &y, 0)){
        cansee_fadespot = cansee(x, y);
        
        if (!silent && cansee_fadespot)
            You_see("a spirit fade away!");
    }

    /* free spirit here because we use it above */
    obj_extract_self(spirit);
    obfree(spirit, (struct obj *) 0);
    if ((mon = m_at(x, y)) && !hideunder(mon) && cansee(x, y)) {
        ;
    } else if (!rn2(level.flags.has_swamp ? 10 : 256)) {
        /* Once in a while, a will-o'-the-wisp will emerge from a faded spirit 
         * Much higher chance on swamp levels. */
        makemon(&mons[PM_WILL_O__THE_WISP], x, y, NO_MM_FLAGS);
    }
    newsym(x, y);
}


/* Attach an egg hatch timeout to the given egg.
 *      when = Time to hatch, usually only passed if re-creating an
 *             existing hatch timer. Pass 0L for random hatch time.
 */
void
attach_egg_hatch_timeout(egg, when)
struct obj *egg;
long when;
{
    int i;

    /* stop previous timer, if any */
    (void) stop_timer(HATCH_EGG, obj_to_any(egg));

    /*
     * Decide if and when to hatch the egg.  The old hatch_it() code tried
     * once a turn from age 151 to 200 (inclusive), hatching if it rolled
     * a number x, 1<=x<=age, where x>150.  This yields a chance of
     * hatching > 99.9993%.  Mimic that here.
     */
    if (!when) {
        for (i = (MAX_EGG_HATCH_TIME - 50) + 1; i <= MAX_EGG_HATCH_TIME; i++)
            if (rnd(i) > 150) {
                /* egg will hatch */
                when = (long) i;
                break;
            }
    }
    if (when) {
        (void) start_timer(when, TIMER_OBJECT, HATCH_EGG, obj_to_any(egg));
    }
}

/* prevent an egg from ever hatching */
void
kill_egg(egg)
struct obj *egg;
{
    /* stop previous timer, if any */
    (void) stop_timer(HATCH_EGG, obj_to_any(egg));
}

/* timer callback routine: hatch the given egg */
void
hatch_egg(arg, timeout)
anything *arg;
long timeout;
{
    struct obj *egg;
    struct monst *mon, *mon2;
    coord cc;
    xchar x, y;
    boolean yours, silent, knows_egg = FALSE;
    boolean cansee_hatchspot = FALSE;
    int i, mnum, hatchcount = 0;

    egg = arg->a_obj;
    /* sterilized while waiting */
    if (egg->corpsenm == NON_PM)
        return;

    mon = mon2 = (struct monst *) 0;
    mnum = big_to_little(egg->corpsenm);
    /* The identity of one's father is learned, not innate */
    yours = (egg->spe || (!flags.female && carried(egg) && !rn2(2)));
    silent = (timeout != monstermoves); /* hatched while away */

    /* only can hatch when in INVENT, FLOOR, MINVENT */
    if (get_obj_location(egg, &x, &y, 0)) {
        hatchcount = rnd((int) egg->quan);
        cansee_hatchspot = cansee(x, y) && !silent;
        if (!(mons[mnum].geno & G_UNIQ)
            && !(mvitals[mnum].mvflags & (G_GENOD | G_EXTINCT))) {
            for (i = hatchcount; i > 0; i--) {
                if (!enexto(&cc, x, y, &mons[mnum])
                    || !(mon = makemon(&mons[mnum], cc.x, cc.y, NO_MINVENT)))
                    break;
                /* tame if your own egg hatches while you're on the
                   same dungeon level, or any dragon egg which hatches
                   while it's in your inventory */
                if ((yours && !silent)
                    || (carried(egg) && mon->data->mlet == S_DRAGON)) {
                    if (tamedog(mon, (struct obj *) 0)) {
                        if (carried(egg) && mon->data->mlet != S_DRAGON) {
                            mon->mtame = 20;
                            u.uconduct.pets++;
                        }
                    }
                }
                if (mvitals[mnum].mvflags & G_EXTINCT)
                    break;  /* just made last one */
                mon2 = mon; /* in case makemon() fails on 2nd egg */
            }
            if (!mon)
                mon = mon2;
            hatchcount -= i;
            egg->quan -= (long) hatchcount;
        }
#if 0
    /*
     * We could possibly hatch while migrating, but the code isn't
     * set up for it...
     */
    } else if (obj->where == OBJ_MIGRATING) {
        /*
         * We can do several things.  The first ones that come to
         * mind are:
         * + Create the hatched monster then place it on the migrating
         *   mons list.  This is tough because all makemon() is made
         *   to place the monster as well.  Makemon() also doesn't lend
         *   itself well to splitting off a "not yet placed" subroutine.
         * + Mark the egg as hatched, then place the monster when we
         *   place the migrating objects.
         * + Or just kill any egg which gets sent to another level.
         *   Falling is the usual reason such transportation occurs.
         */
        cansee_hatchspot = FALSE;
        mon = ???;
#endif
    }

    if (mon) {
        char monnambuf[BUFSZ], carriedby[BUFSZ];
        boolean siblings = (hatchcount > 1), redraw = FALSE;

        if (cansee_hatchspot) {
            /* [bug?  m_monnam() yields accurate monster type
               regardless of hallucination] */
            Sprintf(monnambuf, "%s%s", siblings ? "some " : "",
                    siblings ? makeplural(m_monnam(mon)) : an(m_monnam(mon)));
            /* we don't learn the egg type here because learning
               an egg type requires either seeing the egg hatch
               or being familiar with the egg already,
               as well as being able to see the resulting
               monster, checked below
            */
        }
        switch (egg->where) {
        case OBJ_INVENT:
            knows_egg = TRUE; /* true even if you are blind */
            if (!cansee_hatchspot)
                You_feel("%s %s from your pack!", something,
                         locomotion(mon->data, "drop"));
            else
                You_see("%s %s out of your pack!", monnambuf,
                        locomotion(mon->data, "drop"));
            if (yours) {
                pline("%s cries sound like \"%s%s\"",
                      siblings ? "Their" : "Its",
                      flags.female ? "mommy" : "daddy", egg->spe ? "." : "?");
            } else if (mon->data->mlet == S_DRAGON && !Deaf) {
                verbalize("Gleep!"); /* Mything eggs :-) */
            }
            break;
        case OBJ_FLOOR:
            if (cansee_hatchspot) {
                knows_egg = TRUE;
                You_see("%s hatch.", monnambuf);
                redraw = TRUE; /* update egg's map location */
            }
            break;
        case OBJ_MINVENT:
            if (cansee_hatchspot) {
                /* egg carrying monster might be invisible */
                mon2 = egg->ocarry;
                if (canseemon(mon2)
                    && (!mon2->wormno || cansee(mon2->mx, mon2->my))) {
                    Sprintf(carriedby, "%s pack",
                            s_suffix(a_monnam(mon2)));
                    knows_egg = TRUE;
                } else if (is_pool(mon->mx, mon->my)) {
                    Strcpy(carriedby, "empty water");
                } else {
                    Strcpy(carriedby, "thin air");
                }
                You_see("%s %s out of %s!", monnambuf,
                        locomotion(mon->data, "drop"), carriedby);
            }
            break;
#if 0
        case OBJ_MIGRATING:
            break;
#endif
        default:
            impossible("egg hatched where? (%d)", (int) egg->where);
            break;
        }

        if (cansee_hatchspot && knows_egg)
            learn_egg_type(mnum);

        if (egg->quan > 0) {
            /* still some eggs left */
            /* Instead of ordinary egg timeout use a short one */
            attach_egg_hatch_timeout(egg, (long) rnd(12));
        } else if (carried(egg)) {
            useup(egg);
        } else {
            /* free egg here because we use it above */
            obj_extract_self(egg);
            obfree(egg, (struct obj *) 0);
            if ((mon = m_at(x,y)) && !hideunder(mon) && cansee(x, y))
                redraw = TRUE;
        }
        if (redraw)
            newsym(x, y);

        maybe_unhide_at(x, y);
    }
}

/* Learn to recognize eggs of the given type. */
void
learn_egg_type(mnum)
int mnum;
{
    /* baby monsters hatch from grown-up eggs */
    mnum = little_to_big(mnum);
    mvitals[mnum].mvflags |= MV_KNOWS_EGG;
    /* we might have just learned about other eggs being carried */
    update_inventory();
}

/* Attach a fig_transform timeout to the given figurine. */
void
attach_fig_transform_timeout(figurine)
struct obj *figurine;
{
    int i;

    /* stop previous timer, if any */
    (void) stop_timer(FIG_TRANSFORM, obj_to_any(figurine));

    /*
     * Decide when to transform the figurine.
     */
    i = rnd(9000) + 200;
    /* figurine will transform */
    (void) start_timer((long) i, TIMER_OBJECT, FIG_TRANSFORM,
                       obj_to_any(figurine));
}

/* give a fumble message */
STATIC_OVL void
slip_or_trip()
{
    struct obj *otmp = vobj_at(u.ux, u.uy), *otmp2;
    const char *what;
    char buf[BUFSZ];
    boolean on_foot = TRUE;
    if (u.usteed)
        on_foot = FALSE;

    if (otmp && on_foot && !u.uinwater && is_pool(u.ux, u.uy))
        otmp = 0;

    if (otmp && on_foot) { /* trip over something in particular */
        /*
          If there is only one item, it will have just been named
          during the move, so refer to by via pronoun; otherwise,
          if the top item has been or can be seen, refer to it by
          name; if not, look for rocks to trip over; trip over
          anonymous "something" if there aren't any rocks.
        */
        what = (iflags.last_msg == PLNMSG_ONE_ITEM_HERE)
                ? ((otmp->quan == 1L) ? "it"
                      : Hallucination ? "they" : "them")
                : (otmp->dknown || !Blind)
                      ? doname(otmp)
                      : ((otmp2 = sobj_at(ROCK, u.ux, u.uy)) == 0
                             ? something
                             : (otmp2->quan == 1L ? "a rock" : "some rocks"));
        if (Hallucination) {
            what = strcpy(buf, what);
            buf[0] = highc(buf[0]);
            pline("Egads!  %s bite%s your %s!", what,
                  (!otmp || otmp->quan == 1L) ? "s" : "", body_part(FOOT));
        } else {
            You("trip over %s.", what);
        }
        if (!uarmf && otmp->otyp == CORPSE
            && touch_petrifies(&mons[otmp->corpsenm]) && !Stone_resistance) {
            Sprintf(killer.name, "tripping over %s corpse",
                    an(mons[otmp->corpsenm].mname));
            instapetrify(killer.name);
        }
    } else if (rn2(3) && is_ice(u.ux, u.uy)) {
        pline("%s %s%s on the ice.",
              u.usteed ? upstart(x_monnam(u.usteed,
                                          (has_mname(u.usteed)) ? ARTICLE_NONE
                                                                : ARTICLE_THE,
                                          (char *) 0, (SUPPRESS_SADDLE | SUPPRESS_BARDING),
                                          FALSE))
                       : "You",
              rn2(2) ? "slip" : "slide", on_foot ? "" : "s");
    } else {
        if (on_foot) {
            switch (rn2(4)) {
            case 1:
                You("trip over your own %s.",
                    Hallucination ? "elbow" : makeplural(body_part(FOOT)));
                break;
            case 2:
                You("slip %s.",
                    Hallucination ? "on a banana peel" : "and nearly fall");
                break;
            case 3:
                You("flounder.");
                break;
            default:
                You("stumble.");
                break;
            }
        } else {
            switch (rn2(4)) {
            case 1:
                Your("%s slip out of the stirrups.",
                     makeplural(body_part(FOOT)));
                break;
            case 2:
                You("let go of the reins.");
                break;
            case 3:
                You("bang into the saddle-horn.");
                break;
            default:
                You("slide to one side of the saddle.");
                break;
            }
            dismount_steed(DISMOUNT_FELL);
        }
    }
}

/* Print a lamp flicker message with tailer. */
STATIC_OVL void
see_lamp_flicker(obj, tailer)
struct obj *obj;
const char *tailer;
{
    switch (obj->where) {
    case OBJ_INVENT:
    case OBJ_MINVENT:
        pline("%s flickers%s.", Yname2(obj), tailer);
        break;
    case OBJ_FLOOR:
        You_see("%s flicker%s.", an(xname(obj)), tailer);
        break;
    }
}

/* Print a dimming message for lanterns. */
STATIC_OVL void
lantern_message(obj)
struct obj *obj;
{
    /* from adventure */
    switch (obj->where) {
    case OBJ_INVENT:
        Your("lantern is getting dim.");
        if (Hallucination)
            pline("Batteries have not been invented yet.");
        break;
    case OBJ_FLOOR:
        (Hallucination) ? You_feel("a strange urge to give a lantern a hug.") :
                          You_see("a lantern getting dim.");
        break;
    case OBJ_MINVENT:
        pline("%s lantern is getting dim.", s_suffix(Monnam(obj->ocarry)));
        break;
    }
}

/*
 * Timeout callback for for objects that are burning. E.g. lamps, candles.
 * See begin_burn() for meanings of obj->age and obj->spe.
 */
void
burn_object(arg, timeout)
anything *arg;
long timeout;
{
    struct obj *obj = arg->a_obj;
    boolean canseeit, many, menorah, need_newsym, need_invupdate;
    xchar x, y;
    char whose[BUFSZ];

    menorah = obj->otyp == CANDELABRUM_OF_INVOCATION;
    many = menorah ? obj->spe > 1 : obj->quan > 1L;

    /* timeout while away */
    if (timeout != monstermoves) {
        long how_long = monstermoves - timeout;

        if (how_long >= obj->age) {
            obj->age = 0;
            end_burn(obj, FALSE);

            if (menorah) {
                obj->spe = 0; /* no more candles */
                obj->owt = weight(obj);
            } else if (Is_candle(obj) || obj->otyp == POT_OIL) {
                /* get rid of candles and burning oil potions;
                   we know this object isn't carried by hero,
                   nor is it migrating */
                obj_extract_self(obj);
                obfree(obj, (struct obj *) 0);
                obj = (struct obj *) 0;
            }

        } else {
            obj->age -= how_long;
            begin_burn(obj, TRUE);
        }
        return;
    }

    /* only interested in INVENT, FLOOR, and MINVENT */
    if (get_obj_location(obj, &x, &y, 0)) {
        canseeit = !Blind && cansee(x, y);
        /* set `whose[]' to be "Your " or "Fred's " or "The goblin's " */
        (void) Shk_Your(whose, obj);
    } else {
        canseeit = FALSE;
    }
    need_newsym = need_invupdate = FALSE;

    /* obj->age is the age remaining at this point.  */
    switch (obj->otyp) {
    case POT_OIL:
        /* this should only be called when we run out */
        if (canseeit) {
            switch (obj->where) {
            case OBJ_INVENT:
                need_invupdate = TRUE;
                /*FALLTHRU*/
            case OBJ_MINVENT:
                pline("%spotion of oil has burnt away.", whose);
                break;
            case OBJ_FLOOR:
                (Hallucination) ? You("missed a great opportunity to try a flaming shot.") :
                                  You_see("a burning potion of oil go out.");
                need_newsym = TRUE;
                break;
            }
        }
        end_burn(obj, FALSE); /* turn off light source */
        if (carried(obj)) {
            useupall(obj);
        } else {
            /* clear migrating obj's destination code before obfree
               to avoid false complaint of deleting worn item */
            if (obj->where == OBJ_MIGRATING)
                obj->owornmask = 0L;
            obj_extract_self(obj);
            obfree(obj, (struct obj *) 0);
        }
        obj = (struct obj *) 0;
        break;

    case TORCH:
    case LANTERN:
    case OIL_LAMP:
        switch ((int) obj->age) {
        case 150:
        case 100:
        case 50:
            if (canseeit) {
                if (obj->otyp == LANTERN)
                    lantern_message(obj);
                else
                    see_lamp_flicker(obj,
                                     obj->age == 50L ? " considerably" : "");
            }
            break;

        case 25:
            if (canseeit) {
                if (obj->otyp == LANTERN)
                    lantern_message(obj);
                else {
                    switch (obj->where) {
                    case OBJ_INVENT:
                    case OBJ_MINVENT:
                        pline("%s seems about to go out.", Yname2(obj));
                        break;
                    case OBJ_FLOOR:
                        You_see("%s about to go out.", an(xname(obj)));
                        break;
                    }
                }
            }
            break;

        case 0:
            /* even if blind you'll know if holding it */
            if (canseeit || obj->where == OBJ_INVENT) {
                switch (obj->where) {
                case OBJ_INVENT:
                    need_invupdate = TRUE;
                    /*FALLTHRU*/
                case OBJ_MINVENT:
                    if (obj->otyp == LANTERN)
                        pline("%slantern has run out of power.", whose);
                    else
                        pline("%s has gone out.", Yname2(obj));
                    break;
                case OBJ_FLOOR:
                    if (obj->otyp == LANTERN)
                        (Hallucination) ? You_feel("like the floor is less happy now.") :
                                          You_see("a lantern run out of power.");
                    else
                        You_see("%s go out.", an(xname(obj)));
                    break;
                }
            }
            
			/* MRKR: Burnt out torches are considered worthless */
			
			if (obj->otyp == TORCH) {
                if (obj->unpaid && costly_spot(u.ux, u.uy)) {
                    const char *ithem = obj->quan > 1L ? "them" : "it";
                    verbalize("You burn %s, you bought %s!", ithem, ithem);
                    bill_dummy_object(obj);
                }
			}

            end_burn(obj, FALSE);
            break;

        default:
            /*
             * Someone added fuel to the lamp while it was
             * lit.  Just fall through and let begin burn
             * handle the new age.
             */
            break;
        }

        if (obj->age)
            begin_burn(obj, TRUE);

        break;

    case CANDELABRUM_OF_INVOCATION:
    case TALLOW_CANDLE:
    case WAX_CANDLE:
        switch (obj->age) {
        case 75:
            if (canseeit)
                switch (obj->where) {
                case OBJ_INVENT:
                case OBJ_MINVENT:
                    pline("%s%scandle%s getting short.", whose,
                          menorah ? "candelabrum's " : "",
                          many ? "s are" : " is");
                    break;
                case OBJ_FLOOR:
                    You_see("%scandle%s getting short.",
                            menorah ? "a candelabrum's " : many ? "some "
                                                                : "a ",
                            many ? "s" : "");
                    break;
                }
            break;

        case 15:
            if (canseeit)
                switch (obj->where) {
                case OBJ_INVENT:
                case OBJ_MINVENT:
                    pline("%s%scandle%s flame%s flicker%s low!", whose,
                          menorah ? "candelabrum's " : "", many ? "s'" : "'s",
                          many ? "s" : "", many ? "" : "s");
                    break;
                case OBJ_FLOOR:
                    You_see("%scandle%s flame%s flicker low!",
                            menorah ? "a candelabrum's " : many ? "some "
                                                                : "a ",
                            many ? "s'" : "'s", many ? "s" : "");
                    break;
                }
            break;

        case 0:
            /* we know even if blind and in our inventory */
            if (canseeit || obj->where == OBJ_INVENT) {
                if (menorah) {
                    switch (obj->where) {
                    case OBJ_INVENT:
                        need_invupdate = TRUE;
                        /*FALLTHRU*/
                    case OBJ_MINVENT:
                        pline("%scandelabrum's flame%s.", whose,
                              many ? "s die" : " dies");
                        break;
                    case OBJ_FLOOR:
                        You_see("a candelabrum's flame%s die.",
                                many ? "s" : "");
                        break;
                    }
                } else {
                    switch (obj->where) {
                    case OBJ_INVENT:
                        /* no need_invupdate for update_inventory() necessary;
                           useupall() -> freeinv() handles it */
                        /*FALLTHRU*/
                    case OBJ_MINVENT:
                        pline("%s %s consumed!", Yname2(obj),
                              many ? "are" : "is");
                        break;
                    case OBJ_FLOOR:
                        /*
                          You see some wax candles consumed!
                          You see a wax candle consumed!
                         */
                        You_see("%s%s consumed!", many ? "some " : "",
                                many ? xname(obj) : an(xname(obj)));
                        need_newsym = TRUE;
                        break;
                    }

                    /* post message */
                    pline(Hallucination
                              ? (many ? "They shriek!" : "It shrieks!")
                              : Blind ? "" : (many ? "Their flames die."
                                                   : "Its flame dies."));
                }
            }
            end_burn(obj, FALSE);

            if (menorah) {
                obj->spe = 0;
                obj->owt = weight(obj);
            } else {
                if (carried(obj)) {
                    useupall(obj);
                } else {
                    /* clear migrating obj's destination code
                       so obfree won't think this item is worn */
                    if (obj->where == OBJ_MIGRATING)
                        obj->owornmask = 0L;
                    obj_extract_self(obj);
                    obfree(obj, (struct obj *) 0);
                }
                obj = (struct obj *) 0;
            }
            break; /* case [age ==] 0 */

        default:
            /*
             * Someone added fuel (candles) to the menorah while
             * it was lit.  Just fall through and let begin burn
             * handle the new age.
             */
            break;
        }

        if (obj && obj->age)
            begin_burn(obj, TRUE);
        break; /* case [otyp ==] candelabrum|tallow_candle|wax_candle */

    case RED_DOUBLE_LIGHTSABER:
        if (obj->altmode && obj->cursed && !rn2(25)) {
            obj->altmode = FALSE;
            pline("%s %s reverts to single blade mode!",
                whose, xname(obj));
        }
        /* FALLTHROUGH */
    case GREEN_LIGHTSABER:
    case BLUE_LIGHTSABER:
    case RED_LIGHTSABER:
        /* Callback is checked every 5 turns - 
            lightsaber automatically deactivates if not wielded */
        if ((obj->cursed && !rn2(50)) 
            || (obj->where == OBJ_FLOOR) 
            || (obj->where == OBJ_MINVENT 
                && (!MON_WEP(obj->ocarry) 
                  || MON_WEP(obj->ocarry) != obj))
            || (obj->where == OBJ_INVENT 
              && ((!uwep || uwep != obj) 
              && (!u.twoweap || !uswapwep || obj != uswapwep)))) {
            
            lightsaber_deactivate(obj, FALSE);
        }
    
        switch (obj->age) {			
        case 100:
            /* Single warning time */
            if (canseeit) {
                switch (obj->where) {
                case OBJ_INVENT:
                case OBJ_MINVENT:
                    pline("%s%s dims!",whose, xname(obj));
                    break;
                case OBJ_FLOOR:
                    You_see("%s dim!", an(xname(obj)));
                    break;
                }
            } else {
                You_hear("the hum of %s change!", an(xname(obj)));
            }
            break;
        case 0:
            lightsaber_deactivate(obj, FALSE);
            break;

        default:
        /*
            * Someone added fuel to the lightsaber while it was
            * lit.  Just fall through and let begin burn
            * handle the new age.
            */
            break;
        }
    
        if (obj && obj->age && obj->lamplit) /* might be deactivated */
            begin_burn(obj, TRUE);
        break;

    default:
        impossible("burn_object: unexpected obj %s", xname(obj));
        break;
    
    }
    if (need_newsym)
        newsym(x, y);
    if (need_invupdate)
        update_inventory();
}

/* lightsabers deactivate when they hit the ground/not wielded */
/* assumes caller checks for correct conditions */
void
lightsaber_deactivate (obj, timer_attached)
struct obj *obj;
boolean timer_attached;
{
    xchar x,y;
    char whose[BUFSZ];

    (void) Shk_Your(whose, obj);
            
    if (get_obj_location(obj, &x, &y, 0)) {
        if (cansee(x, y)) {
            switch (obj->where) {
                case OBJ_INVENT:
                case OBJ_MINVENT:
                    pline("%s%s deactivates.", whose, xname(obj));
                    break;
                case OBJ_FLOOR:
                    You_see("%s deactivate.", an(xname(obj)));
                    break;
            }
        } else {
            You_hear("a lightsaber deactivate.");
        }
    }
    if (obj->otyp == RED_DOUBLE_LIGHTSABER) 
        obj->altmode = FALSE;
    if ((obj == uwep) || (u.twoweap && obj != uswapwep)) 
        unweapon = TRUE;
    end_burn(obj, timer_attached);
}


/*
 * Start a burn timeout on the given object. If not "already lit" then
 * create a light source for the vision system.  There had better not
 * be a burn already running on the object.
 *
 * Magic lamps stay lit as long as there's a genie inside, so don't start
 * a timer.
 *
 * Burn rules:
 *      potions of oil, lamps & candles:
 *              age = # of turns of fuel left
 *              spe = <unused>
 *      magic lamps:
 *              age = <unused>
 *              spe = 0 not lightable, 1 lightable forever
 *      candelabrum:
 *              age = # of turns of fuel left
 *              spe = # of candles
 *
 * Once the burn begins, the age will be set to the amount of fuel
 * remaining _once_the_burn_finishes_.  If the burn is terminated
 * early then fuel is added back.
 *
 * This use of age differs from the use of age for corpses and eggs.
 * For the latter items, age is when the object was created, so we
 * know when it becomes "bad".
 *
 * This is a "silent" routine - it should not print anything out.
 */
void
begin_burn(obj, already_lit)
struct obj *obj;
boolean already_lit;
{
    int radius = 3;
    long turns = 0;
    boolean do_timer = TRUE;

    if (obj->age == 0 
      && obj->otyp != MAGIC_LAMP 
      && obj->otyp != MAGIC_CANDLE
      && !artifact_light(obj))
        return;

    switch (obj->otyp) {
    case MAGIC_LAMP:
    case MAGIC_CANDLE:
        obj->lamplit = 1;
        do_timer = FALSE;
        if (obj->otyp == MAGIC_CANDLE) 
            obj->age = 300L;
        if (!obj->cursed) {
            if (lightdamage(obj, FALSE, 5)) {
                    ; /* light haters */
            }
        }
        break;
    case RED_DOUBLE_LIGHTSABER:
        if (obj->altmode && obj->age > 1) {
            obj->age--; /* Double power usage */
            radius = 3; /* Gives off more light */
        } else
            radius = 2;
        turns = 1;
        break;
    case RED_LIGHTSABER:
    case BLUE_LIGHTSABER:
    case GREEN_LIGHTSABER:
        turns = 1;
        radius = 2;
        if (obj->oartifact == ART_LIGHTSABER_PROTOTYPE) {
            do_timer = FALSE;
            obj->lamplit = 1;
        }
        break;
    case POT_OIL:
        turns = obj->age;
        if (obj->odiluted)
            turns = (3L * turns + 2L) / 4L;
        radius = 1; /* very dim light */
        break;
    case LANTERN:
    case OIL_LAMP:
    case TORCH:
        /* magic times are 150, 100, 50, 25, and 0 */
        if (obj->age > 150L)
            turns = obj->age - 150L;
        else if (obj->age > 100L)
            turns = obj->age - 100L;
        else if (obj->age > 50L)
            turns = obj->age - 50L;
        else if (obj->age > 25L)
            turns = obj->age - 25L;
        else
            turns = obj->age;
        break;
    case CANDELABRUM_OF_INVOCATION:
    case TALLOW_CANDLE:
    case WAX_CANDLE:
        /* magic times are 75, 15, and 0 */
        if (obj->age > 75L)
            turns = obj->age - 75L;
        else if (obj->age > 15L)
            turns = obj->age - 15L;
        else
            turns = obj->age;
        radius = candle_light_range(obj);
        break;

    default:
        /* [ALI] Support artifact light sources */
        if (artifact_light(obj)) {
            obj->lamplit = 1;
            do_timer = FALSE;
            radius = arti_light_radius(obj);
        } else {
            impossible("begin burn: unexpected %s", xname(obj));
            turns = obj->age;
        }
        break;
    }

    if (do_timer) {
        if (start_timer(turns, TIMER_OBJECT, BURN_OBJECT, obj_to_any(obj))) {
            obj->lamplit = 1;
            obj->age -= turns;
            if (carried(obj) && !already_lit)
                update_inventory();
        } else {
            obj->lamplit = 0;
        }
    } else {
        if (carried(obj) && !already_lit)
            update_inventory();
    }

    if (obj->lamplit && !already_lit) {
        xchar x, y;

        if (get_obj_location(obj, &x, &y, CONTAINED_TOO | BURIED_TOO))
            new_light_source(x, y, radius, LS_OBJECT, obj_to_any(obj));
        else
            impossible("begin_burn: can't get obj position");
    }
}

/*
 * Stop a burn timeout on the given object if timer attached.  Darken
 * light source.
 */
void
end_burn(obj, timer_attached)
struct obj *obj;
boolean timer_attached;
{
    if (!obj->lamplit) {
        impossible("end_burn: obj %s not lit", xname(obj));
        return;
    }

    if (obj->otyp == MAGIC_LAMP 
            || obj->otyp == MAGIC_CANDLE
            || obj->oartifact == ART_LIGHTSABER_PROTOTYPE 
            || artifact_light(obj))
        timer_attached = FALSE;

    if (!timer_attached) {
        /* [DS] Cleanup explicitly, since timer cleanup won't happen */
        del_light_source(LS_OBJECT, obj_to_any(obj));
        obj->lamplit = 0;
        if (obj->where == OBJ_INVENT)
            update_inventory();
    } else if (!stop_timer(BURN_OBJECT, obj_to_any(obj)))
        impossible("end_burn: obj %s not timed!", xname(obj));
}

/*
 * Cleanup a burning object if timer stopped.
 */
static void
cleanup_burn(arg, expire_time)
anything *arg;
long expire_time;
{
    struct obj *obj = arg->a_obj;
    if (!obj->lamplit) {
        impossible("cleanup_burn: obj %s not lit", xname(obj));
        return;
    }

    del_light_source(LS_OBJECT, obj_to_any(obj));

    /* restore unused time */
    obj->age += expire_time - monstermoves;

    obj->lamplit = 0;

    if (obj->where == OBJ_INVENT)
        update_inventory();
}

/*
 * MRKR: Use up some fuel quickly, eg: when hitting a monster with
 *       a torch.
 */

void burn_faster(struct obj *obj) {
    int turns = rnd(6);
    
    if (!obj->lamplit) {
        impossible("burn_faster: obj %s not lit", xname(obj));
        return;
    }
    
    if (cansee(obj->ox, obj->oy)) {
        if (turns > 4)
            pline_The("torch sparks and flares!");
        else
            pline_The("torch flares up!");
    }
    obj->age -= turns;

    if (obj->age < 1)
        obj->age = 1;
}

void hatch_faster(struct obj *obj) {
    /* For phoenix eggs */
    kill_egg(obj);
    attach_egg_hatch_timeout(obj, 1L);
}

void
do_storms()
{
    int nstrike;
    register int x, y;
    int dirx, diry;
    int count;

    /* no lightning if not the air level or too often, even then */
    if (!Is_airlevel(&u.uz) || rn2(8))
        return;

    /* the number of strikes is 8-log2(nstrike) */
    for (nstrike = rnd(64); nstrike <= 64; nstrike *= 2) {
        count = 0;
        do {
            x = rnd(COLNO - 1);
            y = rn2(ROWNO);
        } while (++count < 100 && levl[x][y].typ != CLOUD);

        if (count < 100) {
            dirx = rn2(3) - 1;
            diry = rn2(3) - 1;
            if (dirx != 0 || diry != 0)
                buzz(-15, /* "monster" LIGHTNING spell */
                     8, x, y, dirx, diry);
        }
    }

    if (levl[u.ux][u.uy].typ == CLOUD) {
        /* Inside a cloud during a thunder storm is deafening. */
        /* Even if already deaf, we sense the thunder's vibrations. */
        pline("Kaboom!!!  Boom!!  Boom!!");
        incr_itimeout(&HDeaf, rn1(20, 30));
        context.botl = TRUE;
        if (!u.uinvulnerable) {
            stop_occupation();
            nomul(-3);
            multi_reason = "hiding from thunderstorm";
            nomovemsg = 0;
        }
    } else
        You_hear("a rumbling noise.");
}

/* -------------------------------------------------------------------------
 */
/*
 * Generic Timeout Functions.
 *
 * Interface:
 *
 * General:
 *  boolean start_timer(long timeout,short kind,short func_index,
 *                      anything *arg)
 *      Start a timer of kind 'kind' that will expire at time
 *      monstermoves+'timeout'.  Call the function at 'func_index'
 *      in the timeout table using argument 'arg'.  Return TRUE if
 *      a timer was started.  This places the timer on a list ordered
 *      "sooner" to "later".  If an object, increment the object's
 *      timer count.
 *
 *  long stop_timer(short func_index, anything *arg)
 *      Stop a timer specified by the (func_index, arg) pair.  This
 *      assumes that such a pair is unique.  Return the time the
 *      timer would have gone off.  If no timer is found, return 0.
 *      If an object, decrement the object's timer count.
 *
 *  long peek_timer(short func_index, anything *arg)
 *      Return time specified timer will go off (0 if no such timer).
 *
 *  void run_timers(void)
 *      Call timers that have timed out.
 *
 * Save/Restore:
 *  void save_timers(int fd, int mode, int range)
 *      Save all timers of range 'range'.  Range is either global
 *      or local.  Global timers follow game play, local timers
 *      are saved with a level.  Object and monster timers are
 *      saved using their respective id's instead of pointers.
 *
 *  void restore_timers(int fd, int range, boolean ghostly, long adjust)
 *      Restore timers of range 'range'.  If from a ghost pile,
 *      adjust the timeout by 'adjust'.  The object and monster
 *      ids are not restored until later.
 *
 *  void relink_timers(boolean ghostly)
 *      Relink all object and monster timers that had been saved
 *      using their object's or monster's id number.
 *
 * Object Specific:
 *  void obj_move_timers(struct obj *src, struct obj *dest)
 *      Reassign all timers from src to dest.
 *
 *  void obj_split_timers(struct obj *src, struct obj *dest)
 *      Duplicate all timers assigned to src and attach them to dest.
 *
 *  void obj_stop_timers(struct obj *obj)
 *      Stop all timers attached to obj.
 *
 *  boolean obj_has_timer(struct obj *object, short timer_type)
 *      Check whether object has a timer of type timer_type.
 */

STATIC_DCL const char *FDECL(kind_name, (SHORT_P));
STATIC_DCL void FDECL(print_queue, (winid, timer_element *));
STATIC_DCL void FDECL(insert_timer, (timer_element *));
STATIC_DCL timer_element *FDECL(remove_timer,
                                (timer_element **, SHORT_P, ANY_P *));
STATIC_DCL void FDECL(write_timer, (int, timer_element *));
STATIC_DCL boolean FDECL(mon_is_local, (struct monst *));
STATIC_DCL boolean FDECL(timer_is_local, (timer_element *));
STATIC_DCL int FDECL(maybe_write_timer, (int, int, BOOLEAN_P));

/* ordered timer list */
static timer_element *timer_base; /* "active" */
static unsigned long timer_id = 1;

/* If defined, then include names when printing out the timer queue */
#define VERBOSE_TIMER

typedef struct {
    timeout_proc f, cleanup;
#ifdef VERBOSE_TIMER
    const char *name;
#define TTAB(a, b, c) \
    {                 \
        a, b, c       \
    }
#else
#define TTAB(a, b, c) \
    {                 \
        a, b          \
    }
#endif
} ttable;

/* table of timeout functions */
static const ttable timeout_funcs[NUM_TIME_FUNCS] = {
    TTAB(rot_organic, (timeout_proc) 0, "rot_organic"),
    TTAB(rot_corpse, (timeout_proc) 0, "rot_corpse"),
    TTAB(revive_mon, (timeout_proc) 0, "revive_mon"),
    TTAB(zombify_mon, (timeout_proc) 0, "zombify_mon"),
    TTAB(burn_object, cleanup_burn, "burn_object"),
    TTAB(hatch_egg, (timeout_proc) 0, "hatch_egg"),
    TTAB(fig_transform, (timeout_proc) 0, "fig_transform"),
    TTAB(melt_ice_away, (timeout_proc) 0, "melt_ice_away"),
    TTAB(moldy_corpse, (timeout_proc) 0, "moldy_corpse"),
    TTAB(bomb_blow, (timeout_proc) 0, "bomb_blow"),
    TTAB(spirit_fade, (timeout_proc) 0, "spirit_fade"),
    TTAB(ferment, (timeout_proc) 0, "ferment"),
    TTAB(fixture_activate, (timeout_proc) 0, "fixture_activate")
};
#undef TTAB

STATIC_OVL const char *
kind_name(kind)
short kind;
{
    switch (kind) {
    case TIMER_LEVEL:
        return "level";
    case TIMER_GLOBAL:
        return "global";
    case TIMER_OBJECT:
        return "object";
    case TIMER_MONSTER:
        return "monster";
    }
    return "unknown";
}

STATIC_OVL void
print_queue(win, base)
winid win;
timer_element *base;
{
    timer_element *curr;
    char buf[BUFSZ];

    if (!base) {
        putstr(win, 0, " <empty>");
    } else {
        putstr(win, 0, "timeout  id   kind   call");
        for (curr = base; curr; curr = curr->next) {
#ifdef VERBOSE_TIMER
            Sprintf(buf, " %4ld   %4ld  %-6s %s(%s)", curr->timeout,
                    curr->tid, kind_name(curr->kind),
                    timeout_funcs[curr->func_index].name,
                    fmt_ptr((genericptr_t) curr->arg.a_void));
#else
            Sprintf(buf, " %4ld   %4ld  %-6s #%d(%s)", curr->timeout,
                    curr->tid, kind_name(curr->kind), curr->func_index,
                    fmt_ptr((genericptr_t) curr->arg.a_void));
#endif
            putstr(win, 0, buf);
        }
    }
}

int
wiz_timeout_queue()
{
    winid win;
    char buf[BUFSZ];
    const char *propname;
    long intrinsic;
    int i, p, count, longestlen, ln, specindx = 0;

    win = create_nhwindow(NHW_MENU); /* corner text window */
    if (win == WIN_ERR)
        return 0;

    Sprintf(buf, "Current time = %ld.", monstermoves);
    putstr(win, 0, buf);
    putstr(win, 0, "");
    putstr(win, 0, "Active timeout queue:");
    putstr(win, 0, "");
    print_queue(win, timer_base);

    /* Timed properies:
     * check every one; the majority can't obtain temporary timeouts in
     * normal play but those can be forced via the #wizintrinsic command.
     */
    count = longestlen = 0;
    for (i = 0; (propname = propertynames[i].prop_name) != 0; ++i) {
        p = propertynames[i].prop_num;
        intrinsic = u.uprops[p].intrinsic;
        if (intrinsic & TIMEOUT) {
            ++count;
            if ((ln = (int) strlen(propname)) > longestlen)
                longestlen = ln;
        }
        if (specindx == 0 && p == FIRE_RES)
            specindx = i;
    }
    putstr(win, 0, "");
    if (!count) {
        putstr(win, 0, "No timed properties.");
    } else {
        putstr(win, 0, "Timed properties:");
        putstr(win, 0, "");
        for (i = 0; (propname = propertynames[i].prop_name) != 0; ++i) {
            p = propertynames[i].prop_num;
            intrinsic = u.uprops[p].intrinsic;
            if (intrinsic & TIMEOUT) {
                if (specindx > 0 && i >= specindx) {
                    putstr(win, 0, " -- settable via #wizinstrinc only --");
                    specindx = 0;
                }
                /* timeout value can be up to 16777215 (0x00ffffff) but
                   width of 4 digits should result in values lining up
                   almost all the time (if/when they don't, it won't
                   look nice but the information will still be accurate) */
                Sprintf(buf, " %*s %4ld", -longestlen, propname,
                        (intrinsic & TIMEOUT));
                putstr(win, 0, buf);
            }
        }
    }
    display_nhwindow(win, FALSE);
    destroy_nhwindow(win);

    return 0;
}

void
timer_sanity_check()
{
    timer_element *curr;

    /* this should be much more complete */
    for (curr = timer_base; curr; curr = curr->next) {
        if (curr->kind == TIMER_OBJECT) {
            struct obj *obj = curr->arg.a_obj;

            if (obj->timed == 0) {
                impossible("timer sanity: untimed obj %s, timer %ld",
                      fmt_ptr((genericptr_t) obj), curr->tid);
            }
        } else if (curr->kind == TIMER_LEVEL) {
            long where = curr->arg.a_long;
            xchar x = (xchar) ((where >> 16) & 0xFFFF),
                  y = (xchar) (where & 0xFFFF);

            if (!isok(x, y)) {
                impossible("timer sanity: spot timer %ld at <%d,%d>",
                           curr->tid, x, y);
            } else if (curr->func_index == MELT_ICE_AWAY && !is_ice(x, y)) {
                impossible("timer sanity: melt timer %ld on non-ice %d <%d,%d>",
                           curr->tid, levl[x][y].typ, x, y);
            }
        }
    }
}

/*
 * Pick off timeout elements from the global queue and call their functions.
 * Do this until their time is less than or equal to the move count.
 */
void
run_timers()
{
    timer_element *curr;

    /*
     * Always use the first element.  Elements may be added or deleted at
     * any time.  The list is ordered, we are done when the first element
     * is in the future.
     */
    while (timer_base && timer_base->timeout <= monstermoves) {
        curr = timer_base;
        timer_base = curr->next;

        if (curr->kind == TIMER_OBJECT)
            (curr->arg.a_obj)->timed--;
        (*timeout_funcs[curr->func_index].f)(&curr->arg, curr->timeout);
        free((genericptr_t) curr);
    }
}

/*
 * Start a timer.  Return TRUE if successful.
 */
boolean
start_timer(when, kind, func_index, arg)
long when;
short kind;
short func_index;
anything *arg;
{
    timer_element *gnu, *dup;

    if (kind < 0 || kind >= NUM_TIMER_KINDS
        || func_index < 0 || func_index >= NUM_TIME_FUNCS)
        panic("start_timer (%s: %d)", kind_name(kind), (int) func_index);

    /* fail if <arg> already has a <func_index> timer running */
    for (dup = timer_base; dup; dup = dup->next)
        if (dup->kind == kind
            && dup->func_index == func_index
            && dup->arg.a_void == arg->a_void)
            break;
    if (dup) {
        char idbuf[QBUFSZ];

#ifdef VERBOSE_TIMER
        Sprintf(idbuf, "%s timer", timeout_funcs[func_index].name);
#else
        Sprintf(idbuf, "%s timer (%d)", kind_name(kind), (int) func_index);
#endif
        impossible("Attempted to start duplicate %s, aborted.", idbuf);
        return FALSE;
    }

    gnu = (timer_element *) alloc(sizeof *gnu);
    (void) memset((genericptr_t) gnu, 0, sizeof *gnu);
    gnu->next = 0;
    gnu->tid = timer_id++;
    gnu->timeout = monstermoves + when;
    gnu->kind = kind;
    gnu->needs_fixup = 0;
    gnu->func_index = func_index;
    gnu->arg = *arg;
    insert_timer(gnu);

    if (kind == TIMER_OBJECT) /* increment object's timed count */
        (arg->a_obj)->timed++;

    return TRUE;
}

/*
 * Remove the timer from the current list and free it up.  Return the time
 * remaining until it would have gone off, 0 if not found.
 */
long
stop_timer(func_index, arg)
short func_index;
anything *arg;
{
    timer_element *doomed;
    long timeout;

    doomed = remove_timer(&timer_base, func_index, arg);

    if (doomed) {
        timeout = doomed->timeout;
        if (doomed->kind == TIMER_OBJECT)
            (arg->a_obj)->timed--;
        if (timeout_funcs[doomed->func_index].cleanup)
            (*timeout_funcs[doomed->func_index].cleanup)(arg, timeout);
        free((genericptr_t) doomed);
        return (timeout - monstermoves);
    }
    return 0L;
}

/*
 * Find the timeout of specified timer; return 0 if none.
 */
long
peek_timer(type, arg)
short type;
anything *arg;
{
    timer_element *curr;

    for (curr = timer_base; curr; curr = curr->next) {
        if (curr->func_index == type && curr->arg.a_void == arg->a_void)
            return curr->timeout;
    }
    return 0L;
}

/*
 * Move all object timers from src to dest, leaving src untimed.
 */
void
obj_move_timers(src, dest)
struct obj *src, *dest;
{
    int count;
    timer_element *curr;

    for (count = 0, curr = timer_base; curr; curr = curr->next)
        if (curr->kind == TIMER_OBJECT && curr->arg.a_obj == src) {
            curr->arg.a_obj = dest;
            dest->timed++;
            count++;
        }
    if (count != src->timed)
        panic("obj_move_timers");
    src->timed = 0;
}

/*
 * Find all object timers and duplicate them for the new object "dest".
 */
void
obj_split_timers(src, dest)
struct obj *src, *dest;
{
    timer_element *curr, *next_timer = 0;

    for (curr = timer_base; curr; curr = next_timer) {
        next_timer = curr->next; /* things may be inserted */
        if (curr->kind == TIMER_OBJECT && curr->arg.a_obj == src) {
            (void) start_timer(curr->timeout - monstermoves, TIMER_OBJECT,
                               curr->func_index, obj_to_any(dest));
        }
    }
}

/*
 * Stop all timers attached to this object.  We can get away with this because
 * all object pointers are unique.
 */
void
obj_stop_timers(obj)
struct obj *obj;
{
    timer_element *curr, *prev, *next_timer = 0;

    for (prev = 0, curr = timer_base; curr; curr = next_timer) {
        next_timer = curr->next;
        if (curr->kind == TIMER_OBJECT && curr->arg.a_obj == obj) {
            if (prev)
                prev->next = curr->next;
            else
                timer_base = curr->next;
            if (timeout_funcs[curr->func_index].cleanup)
                (*timeout_funcs[curr->func_index].cleanup)(&curr->arg,
                                                           curr->timeout);
            free((genericptr_t) curr);
        } else {
            prev = curr;
        }
    }
    obj->timed = 0;
}

/*
 * Check whether object has a timer of type timer_type.
 */
boolean
obj_has_timer(object, timer_type)
struct obj *object;
short timer_type;
{
    long timeout = peek_timer(timer_type, obj_to_any(object));

    return (boolean) (timeout != 0L);
}

/*
 * Stop all timers of index func_index at this spot.
 *
 */
void
spot_stop_timers(x, y, func_index)
xchar x, y;
short func_index;
{
    timer_element *curr, *prev, *next_timer = 0;
    long where = (((long) x << 16) | ((long) y));

    for (prev = 0, curr = timer_base; curr; curr = next_timer) {
        next_timer = curr->next;
        if (curr->kind == TIMER_LEVEL && curr->func_index == func_index
            && curr->arg.a_long == where) {
            if (prev)
                prev->next = curr->next;
            else
                timer_base = curr->next;
            if (timeout_funcs[curr->func_index].cleanup)
                (*timeout_funcs[curr->func_index].cleanup)(&curr->arg,
                                                           curr->timeout);
            free((genericptr_t) curr);
        } else {
            prev = curr;
        }
    }
}

/*
 * When is the spot timer of type func_index going to expire?
 * Returns 0L if no such timer.
 */
long
spot_time_expires(x, y, func_index)
xchar x, y;
short func_index;
{
    timer_element *curr;
    long where = (((long) x << 16) | ((long) y));

    for (curr = timer_base; curr; curr = curr->next) {
        if (curr->kind == TIMER_LEVEL && curr->func_index == func_index
            && curr->arg.a_long == where)
            return curr->timeout;
    }
    return 0L;
}

long
spot_time_left(x, y, func_index)
xchar x, y;
short func_index;
{
    long expires = spot_time_expires(x, y, func_index);
    return (expires > 0L) ? expires - monstermoves : 0L;
}

/* Insert timer into the global queue */
STATIC_OVL void
insert_timer(gnu)
timer_element *gnu;
{
    timer_element *curr, *prev;

    for (prev = 0, curr = timer_base; curr; prev = curr, curr = curr->next)
        if (curr->timeout >= gnu->timeout)
            break;

    gnu->next = curr;
    if (prev)
        prev->next = gnu;
    else
        timer_base = gnu;
}

STATIC_OVL timer_element *
remove_timer(base, func_index, arg)
timer_element **base;
short func_index;
anything *arg;
{
    timer_element *prev, *curr;

    for (prev = 0, curr = *base; curr; prev = curr, curr = curr->next)
        if (curr->func_index == func_index && curr->arg.a_void == arg->a_void)
            break;

    if (curr) {
        if (prev)
            prev->next = curr->next;
        else
            *base = curr->next;
    }

    return curr;
}

STATIC_OVL void
write_timer(fd, timer)
int fd;
timer_element *timer;
{
    anything arg_save;

    arg_save = zeroany;
    switch (timer->kind) {
    case TIMER_GLOBAL:
    case TIMER_LEVEL:
        /* assume no pointers in arg */
        bwrite(fd, (genericptr_t) timer, sizeof(timer_element));
        break;

    case TIMER_OBJECT:
        if (timer->needs_fixup)
            bwrite(fd, (genericptr_t) timer, sizeof(timer_element));
        else {
            /* replace object pointer with id */
            arg_save.a_obj = timer->arg.a_obj;
            timer->arg = zeroany;
            timer->arg.a_uint = (arg_save.a_obj)->o_id;
            timer->needs_fixup = 1;
            bwrite(fd, (genericptr_t) timer, sizeof(timer_element));
            timer->arg.a_obj = arg_save.a_obj;
            timer->needs_fixup = 0;
        }
        break;

    case TIMER_MONSTER:
        if (timer->needs_fixup)
            bwrite(fd, (genericptr_t) timer, sizeof(timer_element));
        else {
            /* replace monster pointer with id */
            arg_save.a_monst = timer->arg.a_monst;
            timer->arg = zeroany;
            timer->arg.a_uint = (arg_save.a_monst)->m_id;
            timer->needs_fixup = 1;
            bwrite(fd, (genericptr_t) timer, sizeof(timer_element));
            timer->arg.a_monst = arg_save.a_monst;
            timer->needs_fixup = 0;
        }
        break;

    default:
        panic("write_timer");
        break;
    }
}

/*
 * Return TRUE if the object will stay on the level when the level is
 * saved.
 */
boolean
obj_is_local(obj)
struct obj *obj;
{
    switch (obj->where) {
    case OBJ_INVENT:
    case OBJ_MIGRATING:
        return FALSE;
    case OBJ_FLOOR:
    case OBJ_BURIED:
        return TRUE;
    case OBJ_CONTAINED:
        return obj_is_local(obj->ocontainer);
    case OBJ_MINVENT:
        return mon_is_local(obj->ocarry);
    }
    panic("obj_is_local");
    return FALSE;
}

/*
 * Return TRUE if the given monster will stay on the level when the
 * level is saved.
 */
STATIC_OVL boolean
mon_is_local(mon)
struct monst *mon;
{
    struct monst *curr;

    for (curr = migrating_mons; curr; curr = curr->nmon)
        if (curr == mon)
            return FALSE;
    /* `mydogs' is used during level changes, never saved and restored */
    for (curr = mydogs; curr; curr = curr->nmon)
        if (curr == mon)
            return FALSE;
    return TRUE;
}

/*
 * Return TRUE if the timer is attached to something that will stay on the
 * level when the level is saved.
 */
STATIC_OVL boolean
timer_is_local(timer)
timer_element *timer;
{
    switch (timer->kind) {
    case TIMER_LEVEL:
        return TRUE;
    case TIMER_GLOBAL:
        return FALSE;
    case TIMER_OBJECT:
        return obj_is_local(timer->arg.a_obj);
    case TIMER_MONSTER:
        return mon_is_local(timer->arg.a_monst);
    }
    panic("timer_is_local");
    return FALSE;
}

/*
 * Part of the save routine.  Count up the number of timers that would
 * be written.  If write_it is true, actually write the timer.
 */
STATIC_OVL int
maybe_write_timer(fd, range, write_it)
int fd, range;
boolean write_it;
{
    int count = 0;
    timer_element *curr;

    for (curr = timer_base; curr; curr = curr->next) {
        if (range == RANGE_GLOBAL) {
            /* global timers */

            if (!timer_is_local(curr)) {
                count++;
                if (write_it)
                    write_timer(fd, curr);
            }

        } else {
            /* local timers */

            if (timer_is_local(curr)) {
                count++;
                if (write_it)
                    write_timer(fd, curr);
            }
        }
    }

    return count;
}

/*
 * Save part of the timer list.  The parameter 'range' specifies either
 * global or level timers to save.  The timer ID is saved with the global
 * timers.
 *
 * Global range:
 *      + timeouts that follow the hero (global)
 *      + timeouts that follow obj & monst that are migrating
 *
 * Level range:
 *      + timeouts that are level specific (e.g. storms)
 *      + timeouts that stay with the level (obj & monst)
 */
void
save_timers(fd, mode, range)
int fd, mode, range;
{
    timer_element *curr, *prev, *next_timer = 0;
    int count;

    if (perform_bwrite(mode)) {
        if (range == RANGE_GLOBAL)
            bwrite(fd, (genericptr_t) &timer_id, sizeof(timer_id));

        count = maybe_write_timer(fd, range, FALSE);
        bwrite(fd, (genericptr_t) &count, sizeof count);
        (void) maybe_write_timer(fd, range, TRUE);
    }

    if (release_data(mode)) {
        for (prev = 0, curr = timer_base; curr; curr = next_timer) {
            next_timer = curr->next; /* in case curr is removed */

            if (!(!!(range == RANGE_LEVEL) ^ !!timer_is_local(curr))) {
                if (prev)
                    prev->next = curr->next;
                else
                    timer_base = curr->next;
                free((genericptr_t) curr);
                /* prev stays the same */
            } else {
                prev = curr;
            }
        }
    }
}

/*
 * Pull in the structures from disk, but don't recalculate the object and
 * monster pointers.
 */
void
restore_timers(fd, range, ghostly, adjust)
int fd, range;
boolean ghostly; /* restoring from a ghost level */
long adjust;     /* how much to adjust timeout */
{
    int count;
    timer_element *curr;

    if (range == RANGE_GLOBAL)
        mread(fd, (genericptr_t) &timer_id, sizeof timer_id);

    /* restore elements */
    mread(fd, (genericptr_t) &count, sizeof count);
    while (count-- > 0) {
        curr = (timer_element *) alloc(sizeof(timer_element));
        mread(fd, (genericptr_t) curr, sizeof(timer_element));
        if (ghostly)
            curr->timeout += adjust;
        insert_timer(curr);
    }
}

/* to support '#stats' wizard-mode command */
void
timer_stats(hdrfmt, hdrbuf, count, size)
const char *hdrfmt;
char *hdrbuf;
long *count, *size;
{
    timer_element *te;

    Sprintf(hdrbuf, hdrfmt, (long) sizeof (timer_element));
    *count = *size = 0L;
    for (te = timer_base; te; te = te->next) {
        ++*count;
        *size += (long) sizeof *te;
    }
}

/* reset all timers that are marked for reseting */
void
relink_timers(ghostly)
boolean ghostly;
{
    timer_element *curr;
    unsigned nid;

    for (curr = timer_base; curr; curr = curr->next) {
        if (curr->needs_fixup) {
            if (curr->kind == TIMER_OBJECT) {
                if (ghostly) {
                    if (!lookup_id_mapping(curr->arg.a_uint, &nid))
                        panic("relink_timers 1");
                } else
                    nid = curr->arg.a_uint;
                curr->arg.a_obj = find_oid(nid);
                if (!curr->arg.a_obj)
                    panic("cant find o_id %d", nid);
                curr->needs_fixup = 0;
            } else if (curr->kind == TIMER_MONSTER) {
                panic("relink_timers: no monster timer implemented");
            } else
                panic("relink_timers 2");
        }
    }
}

/*timeout.c*/
