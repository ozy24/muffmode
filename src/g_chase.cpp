// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"

void UpdateChaseCam(edict_t *ent) {
	vec3_t	o, ownerv, goal;
	edict_t	*targ;
	vec3_t	forward, right;
	trace_t	trace;
	vec3_t	oldgoal;
	vec3_t	angles;

	// is our chase target gone?
	if (!ent->client->chase_target->inuse || ClientIsSpectating(ent->client->chase_target->client)) {
		edict_t *old = ent->client->chase_target;

		ChaseNext(ent);

		ent->client->chase_target = nullptr;
		ent->client->ps.pmove.pm_flags &= ~(PMF_NO_POSITIONAL_PREDICTION | PMF_NO_ANGULAR_PREDICTION);

		if (ent->client->chase_target == old)
			return;

		ent->client->ps.kick_angles = {};
		ent->client->ps.gunangles = {};
		ent->client->ps.gunoffset = {};
		ent->client->ps.gunindex = 0;
		ent->client->ps.gunskin = 0;
		ent->client->ps.gunframe = 0;
		ent->client->ps.gunrate = 0;
		ent->client->ps.screen_blend = {};
		ent->client->ps.damage_blend = {};
		ent->client->ps.rdflags = RDF_NONE;

		ent->client->pers.hand = RIGHT_HANDED;
		ent->client->pers.weapon = nullptr;
	}

	targ = ent->client->chase_target;

	ownerv = targ->s.origin;
	oldgoal = ent->s.origin;

	// Q2Eaks eyecam handling
	if (g_eyecam->integer) {
		// mark the chased player as instanced so we can disable their model's visibility
		targ->svflags |= SVF_INSTANCED;

		// copy everything from ps but pmove, pov, stats, and team_id
		ent->client->ps.viewangles = ent->client->chase_target->client->ps.viewangles;
		ent->client->ps.viewoffset = ent->client->chase_target->client->ps.viewoffset;
		ent->client->ps.kick_angles = ent->client->chase_target->client->ps.kick_angles;
		ent->client->ps.gunangles = ent->client->chase_target->client->ps.gunangles;
		ent->client->ps.gunoffset = ent->client->chase_target->client->ps.gunoffset;
		ent->client->ps.gunindex = ent->client->chase_target->client->ps.gunindex;
		ent->client->ps.gunskin = ent->client->chase_target->client->ps.gunskin;
		ent->client->ps.gunframe = ent->client->chase_target->client->ps.gunframe;
		ent->client->ps.gunrate = ent->client->chase_target->client->ps.gunrate;
		ent->client->ps.screen_blend = ent->client->chase_target->client->ps.screen_blend;
		ent->client->ps.damage_blend = ent->client->chase_target->client->ps.damage_blend;
		ent->client->ps.rdflags = ent->client->chase_target->client->ps.rdflags;

		// do pmove stuff so view looks right, but not pm_flags
		ent->client->ps.pmove.origin = ent->client->chase_target->client->ps.pmove.origin;
		ent->client->ps.pmove.velocity = ent->client->chase_target->client->ps.pmove.velocity;
		ent->client->ps.pmove.pm_time = ent->client->chase_target->client->ps.pmove.pm_time;
		ent->client->ps.pmove.gravity = ent->client->chase_target->client->ps.pmove.gravity;
		ent->client->ps.pmove.delta_angles = ent->client->chase_target->client->ps.pmove.delta_angles;
		ent->client->ps.pmove.viewheight = ent->client->chase_target->client->ps.pmove.viewheight;
		/*
		ent->client->pers.hand = ent->client->chase_target->client->pers.hand;
		ent->client->pers.weapon = ent->client->chase_target->client->pers.weapon;
		*/
		//FIXME: color shells and damage blends not working

		// unadjusted view and origin handling
		angles = targ->client->v_angle;
		AngleVectors(angles, forward, right, nullptr);
		forward.normalize();
		o = ownerv;
		trace = gi.traceline(ownerv, o, targ, MASK_SOLID);
		goal = trace.endpos;
	}
	// vanilla chasecam code
	else {
		ownerv[2] += targ->viewheight;

		angles = targ->client->v_angle;
		if (angles[PITCH] > 56)
			angles[PITCH] = 56;
		AngleVectors(angles, forward, right, nullptr);
		forward.normalize();
		o = ownerv + (forward * -30);

		if (o[2] < targ->s.origin[2] + 20)
			o[2] = targ->s.origin[2] + 20;

		// jump animation lifts
		if (!targ->groundentity)
			o[2] += 16;

		trace = gi.traceline(ownerv, o, targ, MASK_SOLID);

		goal = trace.endpos;

		goal += (forward * 2);

		// pad for floors and ceilings
		o = goal;
		o[2] += 6;
		trace = gi.traceline(goal, o, targ, MASK_SOLID);
		if (trace.fraction < 1) {
			goal = trace.endpos;
			goal[2] -= 6;
		}

		o = goal;
		o[2] -= 6;
		trace = gi.traceline(goal, o, targ, MASK_SOLID);
		if (trace.fraction < 1) {
			goal = trace.endpos;
			goal[2] += 6;
		}
	}

	if (targ->deadflag)
		ent->client->ps.pmove.pm_type = PM_DEAD;
	else
		ent->client->ps.pmove.pm_type = PM_FREEZE;

	ent->s.origin = goal;
	ent->client->ps.pmove.delta_angles = targ->client->v_angle - ent->client->resp.cmd_angles;

	if (targ->deadflag) {
		ent->client->ps.viewangles[ROLL] = 40;
		ent->client->ps.viewangles[PITCH] = -15;
		ent->client->ps.viewangles[YAW] = targ->client->killer_yaw;
	} else {
		ent->client->ps.viewangles = targ->client->v_angle;
		ent->client->v_angle = targ->client->v_angle;
		AngleVectors(ent->client->v_angle, ent->client->v_forward, nullptr, nullptr);
	}

	ent->viewheight = 0;
	if (!g_eyecam->integer)
		ent->client->ps.pmove.pm_flags |= PMF_NO_POSITIONAL_PREDICTION | PMF_NO_ANGULAR_PREDICTION;
	gi.linkentity(ent);
}

void ChaseNext(edict_t *ent) {
	ptrdiff_t i;
	edict_t *e;

	if (!ent->client->chase_target)
		return;

	i = ent->client->chase_target - g_edicts;
	do {
		i++;
		if (i > game.maxclients)
			i = 1;
		e = g_edicts + i;
		if (!e->inuse)
			continue;
		if (!ClientIsSpectating(e->client))
			break;
	} while (e != ent->client->chase_target);

	ent->client->chase_target = e;
	ent->client->update_chase = true;
}

void ChasePrev(edict_t *ent) {
	int		 i;
	edict_t *e;

	if (!ent->client->chase_target)
		return;

	i = ent->client->chase_target - g_edicts;
	do {
		i--;
		if (i < 1)
			i = game.maxclients;
		e = g_edicts + i;
		if (!e->inuse)
			continue;
		if (!ClientIsSpectating(e->client))
			break;
	} while (e != ent->client->chase_target);

	ent->client->chase_target = e;
	ent->client->update_chase = true;
}

void GetChaseTarget(edict_t *ent) {
	uint32_t i;
	edict_t *other;

	for (i = 1; i <= game.maxclients; i++) {
		other = g_edicts + i;
		if (other->inuse && !ClientIsSpectating(other->client)) {
			ent->client->chase_target = other;
			ent->client->update_chase = true;
			UpdateChaseCam(ent);
			return;
		}
	}

	if (ent->client->chase_msg_time <= level.time) {
		gi.LocCenter_Print(ent, "$g_no_players_chase");
		ent->client->chase_msg_time = level.time + 5_sec;
	}
}
