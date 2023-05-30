/***
 * cpddl
 * -------
 * Copyright (c)2016 Daniel Fiser <danfis@danfis.cz>,
 * AI Center, Department of Computer Science,
 * Faculty of Electrical Engineering, Czech Technical University in Prague.
 * All rights reserved.
 *
 * This file is part of cpddl.
 *
 * Distributed under the OSI-approved BSD License (the "License");
 * see accompanying file BDS-LICENSE for details or see
 * <http://www.opensource.org/licenses/bsd-license.php>.
 *
 * This software is distributed WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the License for more information.
 */

#include <boruvka/alloc.h>
#include <boruvka/hfunc.h>
#include "pddl/pddl.h"
#include "pddl/prep_action.h"
#include "err.h"

#define MUST_NEQ(A, I, J) \
    ((A)->must_neq[(I) * (A)->param_size + (J)])
#define MUST_EQ(A, I, J) \
    ((A)->must_eq[(I) * (A)->param_size + (J)])

struct action_ctx {
    pddl_prep_actions_t *as;
    pddl_prep_action_t *a;
    int a_id;
    const pddl_t *pddl;
    const pddl_action_t *action;
    int failed;
    bor_err_t *err;
};
typedef struct action_ctx action_ctx_t;

static int actionInitPre(pddl_cond_t *c, void *ud)
{
    action_ctx_t *ctx = ud;
    pddl_cond_atom_t *a;

    if (c->type == PDDL_COND_ATOM){
        a = PDDL_COND_CAST(c, atom);
        ctx->a->max_arg_size = BOR_MAX(ctx->a->max_arg_size, a->arg_size);
        if (a->pred == ctx->pddl->pred.eq_pred){
            pddlCondArrAdd(&ctx->a->pre_eq, c);
        }else{
            if (a->neg){
                pddlCondArrAdd(&ctx->a->pre_neg_static, c);
            }else{
                pddlCondArrAdd(&ctx->a->pre, c);
            }
        }
        return 0;

    }else if (c->type == PDDL_COND_AND){
        return 0;
    }else{
        BOR_ERR2(ctx->err, "Precondition is not a simple conjuction."
                 " It seems it was not normalized.");
        ctx->failed = 1;
        return -2;
    }
}

static int actionInitEff(pddl_cond_t *c, void *ud)
{
    action_ctx_t *ctx = ud;
    pddl_cond_atom_t *a;

    if (c->type == PDDL_COND_ATOM){
        a = PDDL_COND_CAST(c, atom);
        ctx->a->max_arg_size = BOR_MAX(ctx->a->max_arg_size, a->arg_size);
        if (a->neg){
            pddlCondArrAdd(&ctx->a->del_eff, c);
        }else{
            pddlCondArrAdd(&ctx->a->add_eff, c);
        }
        return 0;

    }else if (c->type == PDDL_COND_ASSIGN){
        BOR_ERR2(ctx->err, "(= ...) is not supported in operators' effects.");
        ctx->failed = 1;
        return -2;

    }else if (c->type == PDDL_COND_INCREASE){
        pddlCondArrAdd(&ctx->a->increase, c);
        return 0;

    }else if (c->type == PDDL_COND_WHEN){
        ++ctx->a->cond_eff_size;
        return -1;

    }else if (c->type == PDDL_COND_AND){
        return 0;
    }else{
        BOR_ERR2(ctx-> err, "Effect is not a simple conjuction"
                 " (possibly containingconditional effects and function"
                 " assignement). It seems it was not normalized.");
        ctx->failed = 1;
        return -2;
    }
}

static int actionInit2(pddl_prep_action_t *a,
                       const pddl_t *pddl,
                       const pddl_action_t *action,
                       pddl_cond_t *pre,
                       pddl_cond_t *eff,
                       bor_err_t *err)
{
    action_ctx_t ctx;
    ctx.a = a;
    ctx.pddl = pddl;
    ctx.failed = 0;
    ctx.err = err;

    bzero(a, sizeof(*a));
    a->action = action;
    a->parent_action = -1;
    a->param_size = action->param.param_size;
    a->param_type = BOR_ALLOC_ARR(int, a->param_size);
    for (int i = 0; i < a->param_size; ++i)
        a->param_type[i] = action->param.param[i].type;
    a->type = &pddl->type;

    pddlCondTraverse(pre, actionInitPre, NULL, &ctx);
    if (ctx.failed){
        BOR_TRACE_PREPEND_RET(err, -1, "Prepapration of action %s failed: ",
                              action->name);
    }

    pddlCondTraverse(eff, actionInitEff, NULL, &ctx);
    if (ctx.failed){
        BOR_TRACE_PREPEND_RET(err, -1, "Prepapration of action %s failed: ",
                              action->name);
    }

    return 0;
}

static int actionInit(pddl_prep_action_t *a,
                      const pddl_t *pddl,
                      const pddl_action_t *action,
                      bor_err_t *err)
{
    int ret;
    ret = actionInit2(a, pddl, action,
                      (pddl_cond_t *)action->pre,
                      (pddl_cond_t *)action->eff,
                      err);
    if (ret != 0)
        BOR_TRACE(err);
    return ret;
}

static void actionFree(pddl_prep_action_t *a)
{
    pddlCondArrFree(&a->pre_neg_static);
    pddlCondArrFree(&a->pre_eq);
    pddlCondArrFree(&a->pre);
    pddlCondArrFree(&a->add_eff);
    pddlCondArrFree(&a->del_eff);
    pddlCondArrFree(&a->increase);
    if (a->param_type != NULL)
        BOR_FREE(a->param_type);
}

static void actionsReserve(pddl_prep_actions_t *as)
{
    if (as->action_size >= as->action_alloc){
        as->action_alloc *= 2;
        as->action = BOR_REALLOC_ARR(as->action, pddl_prep_action_t,
                                     as->action_alloc);
    }
}

static int actionInitCondEff(pddl_cond_t *c, void *ud)
{
    action_ctx_t *ctx = ud;
    const pddl_cond_when_t *when;
    pddl_prep_action_t *a, *parent;

    if (c->type == PDDL_COND_WHEN){
        when = PDDL_COND_CAST(c, when);

        // Create a new action
        actionsReserve(ctx->as);
        a = ctx->as->action + ctx->as->action_size++;

        // Parse preconditions and effects of (when ) element
        if (actionInit2(a, ctx->pddl, ctx->action,
                        when->pre, when->eff, ctx->err) != 0){
            BOR_TRACE(ctx->err);
            ctx->failed = 1;
            return -2;
        }
        if (a->cond_eff_size > 0){
            BOR_ERR(ctx->err, "Preparation of the action %s failed:"
                    " Nested conditional effects are not supported.",
                ctx->action->name);
            ctx->failed = 1;
            return -2;
        }

        // Set its parent
        parent = ctx->as->action + ctx->a_id;
        a->parent_action = ctx->a_id;

        // Copy preconditions
        for (int i = 0; i < parent->pre_neg_static.size; ++i)
            pddlCondArrAdd(&a->pre_neg_static, parent->pre_neg_static.cond[i]);
        for (int i = 0; i < parent->pre_eq.size; ++i)
            pddlCondArrAdd(&a->pre_eq, parent->pre_eq.cond[i]);
        for (int i = 0; i < parent->pre.size; ++i)
            pddlCondArrAdd(&a->pre, parent->pre.cond[i]);
        a->max_arg_size = BOR_MAX(a->max_arg_size, parent->max_arg_size);


        return -1;
    }
    return 0;
}

static int actionsAddCondEff(pddl_prep_actions_t *as, int aid,
                             const pddl_t *pddl, bor_err_t *err)
{
    action_ctx_t ctx;
    ctx.as = as;
    ctx.a_id = aid;
    ctx.pddl = pddl;
    ctx.action = as->action[aid].action;
    ctx.failed = 0;
    ctx.err = err;

    pddlCondTraverse((pddl_cond_t *)ctx.action->eff,
                     actionInitCondEff, NULL, &ctx);
    if (ctx.failed)
        BOR_TRACE_RET(err, -1);
    return 0;
}

int pddlPrepActionsInit(const pddl_t *pddl, pddl_prep_actions_t *as,
                        bor_err_t *err)
{
    const pddl_action_t *action;

    bzero(as, sizeof(*as));
    as->action_alloc = 4;
    as->action = BOR_ALLOC_ARR(pddl_prep_action_t, as->action_alloc);

    for (int i = 0; i < pddl->action.action_size; ++i){
        actionsReserve(as);
        action = pddl->action.action + i;
        if (actionInit(as->action + as->action_size, pddl, action, err) != 0){
            pddlPrepActionsFree(as);
            BOR_TRACE_RET(err, -1);
        }
        ++as->action_size;
    }

    for (int i = 0; i < pddl->action.action_size; ++i){
        if (as->action[i].cond_eff_size > 0){
            if (actionsAddCondEff(as, i, pddl, err) != 0){
                pddlPrepActionsFree(as);
                BOR_TRACE_RET(err, -1);
            }
        }
    }

    return 0;
}

void pddlPrepActionsFree(pddl_prep_actions_t *as)
{
    for (int i = 0; i < as->action_size; ++i)
        actionFree(as->action + i);
    if (as->action)
        BOR_FREE(as->action);
}



static int checkPreAtomFact(const pddl_prep_action_t *a,
                            const pddl_cond_atom_t *atom,
                            const pddl_obj_id_t *arg)
{
    for (int i = 0; i < atom->arg_size; ++i){
        int param = atom->arg[i].param;
        if (param >= 0){
            int type = a->param_type[param];
            if (!pddlTypesObjHasType(a->type, type, arg[i]))
                return 0;
        }else{
            if (atom->arg[i].obj != arg[i])
                return 0;
        }
    }
    return 1;
}

static int checkPreAtom(const pddl_prep_action_t *a,
                        const pddl_cond_atom_t *atom,
                        const pddl_obj_id_t *arg)
{
    for (int i = 0; i < atom->arg_size; ++i){
        int param = atom->arg[i].param;
        if (param >= 0){
            int type = a->param_type[param];
            if (!pddlTypesObjHasType(a->type, type, arg[param]))
                return 0;
        }
    }
    return 1;
}

static int checkPre(const pddl_prep_action_t *a,
                    const pddl_cond_arr_t *pre,
                    const pddl_obj_id_t *arg)
{
    const pddl_cond_atom_t *atom;

    for (int i = 0; i < pre->size; ++i){
        atom = PDDL_COND_CAST(pre->cond[i], atom);
        if (!checkPreAtom(a, atom, arg))
            return 0;
    }
    return 1;
}

static int checkEq(const pddl_prep_action_t *a, const pddl_obj_id_t *arg,
                   int soft)
{
    const pddl_cond_atom_t *atom;
    const pddl_cond_t **pre = a->pre_eq.cond;
    int size = a->pre_eq.size;
    pddl_obj_id_t obj[2];

    for (int i = 0; i < size; ++i){
        atom = PDDL_COND_CAST(pre[i], atom);
        for (int j = 0; j < 2; ++j){
            if (atom->arg[j].param >= 0){
                obj[j] = arg[atom->arg[j].param];
            }else{
                obj[j] = atom->arg[j].obj;
            }
        }
        if (obj[0] == -1 && obj[1] == -1)
            continue;
        if (soft && (obj[0] == -1 || obj[1] == -1))
            continue;

        int eq = (obj[0] == obj[1]);
        if (eq && atom->neg){
            return 0;
        }else if (!eq && !atom->neg){
            return 0;
        }
    }

    return 1;
}

static int checkPreNegStatic(const pddl_prep_action_t *a,
                             const pddl_ground_atoms_t *static_facts,
                             const pddl_obj_id_t *arg)
{
    if (a->pre_neg_static.size == 0)
        return 1;

    const pddl_cond_atom_t *atom;

    for (int i = 0; i < a->pre_neg_static.size; ++i){
        atom = PDDL_COND_CAST(a->pre_neg_static.cond[i], atom);
        if (pddlGroundAtomsFindAtom(static_facts, atom, arg) != NULL)
            return 0;
    }

    return 1;
}

int pddlPrepActionCheck(const pddl_prep_action_t *a,
                        const pddl_ground_atoms_t *static_facts,
                        const pddl_obj_id_t *arg)
{
    return checkPre(a, &a->pre, arg)
            && checkEq(a, arg, 0)
            && checkPreNegStatic(a, static_facts, arg);
}

int pddlPrepActionCheckFact(const pddl_prep_action_t *a, int pre_i,
                            const pddl_obj_id_t *fact_args)
{
    const pddl_cond_atom_t *atom = PDDL_COND_CAST(a->pre.cond[pre_i], atom);
    pddl_obj_id_t arg[a->param_size];
    int param;

    if (!checkPreAtomFact(a, atom, fact_args))
        return 0;

    for (int i = 0; i < a->param_size; ++i)
        arg[i] = -1;
    for (int i = 0; i < atom->arg_size; ++i){
        param = atom->arg[i].param;
        if (param >= 0){
            if (arg[param] != -1 && arg[param] != fact_args[i])
                return 0;
            arg[param] = fact_args[i];
        }
    }

    return checkEq(a, arg, 1);
}

int pddlPrepActionCheckEqDef(const pddl_prep_action_t *a,
                             const pddl_obj_id_t *arg)
{
    return checkEq(a, arg, 1);
}
