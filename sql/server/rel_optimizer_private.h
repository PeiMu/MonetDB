/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 1997 - July 2008 CWI, August 2008 - 2022 MonetDB B.V.
 */

#include "rel_rel.h"
#include "sql_mvc.h"

/* This file should be included by SQL optimizers or essential rewriters only! */

/* relations counts */
typedef struct global_props {
	int cnt[ddl_maxops];
	uint8_t
		instantiate:1,
		needs_mergetable_rewrite:1,
		needs_remote_replica_rewrite:1,
		needs_distinct:1,
		needs_setjoin_rewrite:1,
		opt_level:1; /* 0 run necessary rewriters, 1 run all optimizers */
	uint8_t opt_cycle; /* the optimization cycle number */
} global_props;

typedef sql_rel *(*run_optimizer)(visitor *v, global_props *gp, sql_rel *rel);

/* the definition of a single SQL optimizer */
typedef struct sql_optimizer {
	const int index; /* because some optimizers runs after the main loop, an index track is needed */
	const char *name;
	run_optimizer (*bind_optimizer)(visitor *v, global_props *gp); /* if cannot run (disabled or not needed) returns NULL */
} sql_optimizer;

/* At the moment the follwowing optimizers 'packs' can be disabled,
   later we could disable individual optimizers from the 'pack' */
#define split_select                        (1 << 0)
#define push_project_down                   (1 << 1)
#define merge_projects                      (1 << 2)
#define push_project_up                     (1 << 3)
#define split_project                       (1 << 4)
#define remove_redundant_join               (1 << 5)
#define simplify_math                       (1 << 6)
#define optimize_exps                       (1 << 7)
#define optimize_select_and_joins_bottomup  (1 << 8)
#define project_reduce_casts                (1 << 9)
#define optimize_unions_bottomup           (1 << 10)
#define optimize_projections               (1 << 11)
#define optimize_joins                     (1 << 12)
#define join_order                         (1 << 13)
#define optimize_semi_and_anti             (1 << 14)
#define optimize_select_and_joins_topdown  (1 << 15)
#define optimize_unions_topdown            (1 << 16)
#define dce                                (1 << 17)
#define push_func_and_select_down          (1 << 18)
#define push_topn_and_sample_down          (1 << 19)
#define distinct_project2groupby           (1 << 20)
#define push_select_up                     (1 << 21)

extern run_optimizer bind_split_select(visitor *v, global_props *gp) __attribute__((__visibility__("hidden")));
extern run_optimizer bind_push_project_down(visitor *v, global_props *gp) __attribute__((__visibility__("hidden")));
extern run_optimizer bind_merge_projects(visitor *v, global_props *gp) __attribute__((__visibility__("hidden")));
extern run_optimizer bind_push_project_up(visitor *v, global_props *gp) __attribute__((__visibility__("hidden")));
extern run_optimizer bind_split_project(visitor *v, global_props *gp) __attribute__((__visibility__("hidden")));
extern run_optimizer bind_remove_redundant_join(visitor *v, global_props *gp) __attribute__((__visibility__("hidden")));
extern run_optimizer bind_simplify_math(visitor *v, global_props *gp) __attribute__((__visibility__("hidden")));
extern run_optimizer bind_optimize_exps(visitor *v, global_props *gp) __attribute__((__visibility__("hidden")));
extern run_optimizer bind_optimize_select_and_joins_bottomup(visitor *v, global_props *gp) __attribute__((__visibility__("hidden")));
extern run_optimizer bind_project_reduce_casts(visitor *v, global_props *gp) __attribute__((__visibility__("hidden")));
extern run_optimizer bind_optimize_unions_bottomup(visitor *v, global_props *gp) __attribute__((__visibility__("hidden")));
extern run_optimizer bind_optimize_projections(visitor *v, global_props *gp) __attribute__((__visibility__("hidden")));
extern run_optimizer bind_optimize_joins(visitor *v, global_props *gp) __attribute__((__visibility__("hidden")));
extern run_optimizer bind_join_order(visitor *v, global_props *gp) __attribute__((__visibility__("hidden")));
extern run_optimizer bind_optimize_semi_and_anti(visitor *v, global_props *gp) __attribute__((__visibility__("hidden")));
extern run_optimizer bind_optimize_select_and_joins_topdown(visitor *v, global_props *gp) __attribute__((__visibility__("hidden")));
extern run_optimizer bind_optimize_unions_topdown(visitor *v, global_props *gp) __attribute__((__visibility__("hidden")));
extern run_optimizer bind_dce(visitor *v, global_props *gp) __attribute__((__visibility__("hidden")));
extern run_optimizer bind_push_func_and_select_down(visitor *v, global_props *gp) __attribute__((__visibility__("hidden")));
extern run_optimizer bind_push_topn_and_sample_down(visitor *v, global_props *gp) __attribute__((__visibility__("hidden")));
extern run_optimizer bind_distinct_project2groupby(visitor *v, global_props *gp) __attribute__((__visibility__("hidden")));
extern run_optimizer bind_push_select_up(visitor *v, global_props *gp) __attribute__((__visibility__("hidden")));
extern run_optimizer bind_merge_table_rewrite(visitor *v, global_props *gp) __attribute__((__visibility__("hidden")));
extern run_optimizer bind_setjoins_2_joingroupby(visitor *v, global_props *gp) __attribute__((__visibility__("hidden")));
extern run_optimizer bind_rewrite_remote(visitor *v, global_props *gp) __attribute__((__visibility__("hidden")));
extern run_optimizer bind_rewrite_replica(visitor *v, global_props *gp) __attribute__((__visibility__("hidden")));
extern run_optimizer bind_remote_func(visitor *v, global_props *gp) __attribute__((__visibility__("hidden")));

extern sql_rel *rel_split_project_(visitor *v, sql_rel *rel, int top) __attribute__((__visibility__("hidden")));
extern sql_exp *exp_push_down_prj(mvc *sql, sql_exp *e, sql_rel *f, sql_rel *t) __attribute__((__visibility__("hidden")));
extern sql_exp *add_exp_too_project(mvc *sql, sql_exp *e, sql_rel *rel) __attribute__((__visibility__("hidden")));

/* these functions are used across diferent optimizers */
extern sql_rel *rel_find_ref(sql_rel *r) __attribute__((__visibility__("hidden")));
extern void rel_rename_exps(mvc *sql, list *exps1, list *exps2) __attribute__((__visibility__("hidden")));
extern atom *reduce_scale(mvc *sql, atom *a) __attribute__((__visibility__("hidden")));
extern int exp_range_overlap(atom *min, atom *max, atom *emin, atom *emax, bool min_exclusive, bool max_exclusive) __attribute__((__visibility__("hidden")));
extern int is_numeric_upcast(sql_exp *e) __attribute__((__visibility__("hidden")));
extern sql_exp *list_exps_uses_exp(list *exps, const char *rname, const char *name) __attribute__((__visibility__("hidden")));
extern sql_exp *exps_uses_exp(list *exps, sql_exp *e) __attribute__((__visibility__("hidden")));
extern int sql_class_base_score(visitor *v, sql_column *c, sql_subtype *t, bool equality_based) __attribute__((__visibility__("hidden")));
