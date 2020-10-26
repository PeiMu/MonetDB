/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 1997 - July 2008 CWI, August 2008 - 2020 MonetDB B.V.
 */

/* author Joeri van Ruth
 * This optimizer replaces calls to sql.importTable with a series of calls to
 * sql.importColumn.
 */
#include "monetdb_config.h"
#include "mal_builder.h"
#include "opt_append.h"

typedef struct parstate {
	InstrPtr prep_stmt;
} parstate;

static str transform(parstate *state, MalBlkPtr mb, InstrPtr importTable);
static int setup_append_prep(parstate *state, MalBlkPtr mb, InstrPtr old);


str
OPTparappendImplementation(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci)
{
	str msg = MAL_SUCCEED;
	InstrPtr *old_mb_stmt = NULL;
	parstate state = { NULL };

	(void)cntxt;
	(void)mb;
	(void)stk;
	(void)pci;

	int found_at = -1;
	for (int i = 0; i < mb->stop; i++) {
		InstrPtr p = getInstrPtr(mb, i);
		if (p->modname == sqlRef && p->fcnname == appendRef) {
			found_at = i;
			break;
		}
	}
	if (found_at == -1)
		return MAL_SUCCEED;

	old_mb_stmt = mb->stmt;
	int old_stop = mb->stop;
	if (newMalBlkStmt(mb, mb->stop + getInstrPtr(mb, found_at)->argc) < 0) {
		msg = createException(MAL, "optimizer.parappend", SQLSTATE(HY013) MAL_MALLOC_FAIL);
		goto end;
	}

	for (int i = 0; i < old_stop; i++) {
		InstrPtr p = old_mb_stmt[i];
		if (p->modname == sqlRef && p->fcnname == appendRef && isaBatType(getArgType(mb, p, 5))) {
			msg = transform(&state, mb, p);
		} else {
			if (mayhaveSideEffects(cntxt, mb, p, false)) {
				state = (parstate) { NULL };
			}
			pushInstruction(mb, p);
		}
		if (msg != MAL_SUCCEED)
			return msg;
	}

end:
	if (old_mb_stmt)
		GDKfree(old_mb_stmt);
	return msg;
}

static str
transform(parstate *state, MalBlkPtr mb, InstrPtr old)
{
	// take the old instruction apart
	assert(old->retc == 1);
	assert(old->argc == 1 + 5);
	int sname_var = getArg(old, 2);
	int tname_var = getArg(old, 3);
	int cname_var = getArg(old, 4);
	int data_var = getArg(old, 5);

	bool sname_constant = isVarConstant(mb, sname_var);
	bool tname_constant = isVarConstant(mb, tname_var);
	bool cname_constant = isVarConstant(mb, cname_var);

	if (!sname_constant || !tname_constant || !cname_constant) {
		// cannot transform this
		pushInstruction(mb, old);
		return MAL_SUCCEED;
	}


	int cookie_var = setup_append_prep(state, mb, old);

	str append_execRef = putName("append_exec");
	InstrPtr e = newFcnCall(mb, sqlRef, append_execRef);
	pushArgument(mb, e, cookie_var);
	pushArgument(mb, e, data_var);

	// fprintf(stderr, "TRIGGER\n");

	return MAL_SUCCEED;
}

static int
setup_append_prep(parstate *state, MalBlkPtr mb, InstrPtr old)
{
	// take the old instruction apart
	assert(old->retc == 1);
	assert(old->argc == 1 + 5);
	int chain_out_var = getArg(old, 0);
	int chain_in_var = getArg(old, 1);
	int sname_var = getArg(old, 2);
	int tname_var = getArg(old, 3);
	int cname_var = getArg(old, 4);

	// check if the state refers to a sql.append_prep statement that can be
	// reused.
	InstrPtr prep_stmt = NULL;
	do {
		if (state->prep_stmt == NULL)
			break;

		InstrPtr prev = state->prep_stmt;

		int existing_sname_var = getArg(prev, prev->retc + 1);
		int existing_tname_var = getArg(prev, prev->retc + 2);

		const char *existing_sname = getVarConstant(mb, existing_sname_var).val.sval;
		const char *incoming_sname = getVarConstant(mb, sname_var).val.sval;
		if (strcmp(existing_sname, incoming_sname) != 0)
			break;

		const char *existing_tname = getVarConstant(mb, existing_tname_var).val.sval;
		const char *incoming_tname = getVarConstant(mb, tname_var).val.sval;
		if (strcmp(existing_tname, incoming_tname) != 0)
			break;

		const char *incoming_cname = getVarConstant(mb, cname_var).val.sval;
		int existing_cols = prev->retc - 1;
		for (int i = prev->argc - existing_cols; i < prev->argc; i++) {
			int var = getArg(prev, i);
			const char *existing_cname = getVarConstant(mb, var).val.sval;
			if (strcmp(existing_cname, incoming_cname) == 0) {
				// We're not prepared for the complications that may arise
				// when there are multiple appends to the same column.
				// In particular we would have to track down where the previous
				// cookie is used and make sure we execute the next append
				// after that use.
				// This is unlikely to occur in practice, so instead we just start over.
				break;
			}
		}

		// It seems there is no objection to reusing the existing sql.append_prep.
		prep_stmt = prev;
	} while (0);

	int cookie_var = newTmpVariable(mb, TYPE_ptr);
	if (prep_stmt == NULL) {
		str append_prepRef = putName("append_prep");
		InstrPtr p = newFcnCall(mb, sqlRef, append_prepRef);
		setReturnArgument(p, chain_out_var);
		pushReturn(mb, p, cookie_var);
		pushArgument(mb, p, chain_in_var);
		pushArgument(mb, p, sname_var);
		pushArgument(mb, p, tname_var);
		pushArgument(mb, p, cname_var);
		state->prep_stmt = p;
	} else {
		// Append to existing first, to ensure there is room
		pushArgument(mb, prep_stmt, cname_var);
		pushArgument(mb, prep_stmt, cookie_var);
		// Now move the cookie_var to its proper location
		for (int i = prep_stmt->argc - 1; i > prep_stmt->retc; i--)
			setArg(prep_stmt, i, getArg(prep_stmt, i - 1));
		setArg(prep_stmt, prep_stmt->retc, cookie_var);
		prep_stmt->retc += 1;
		// Always use the chain_out of the latest sql_append:
		setArg(prep_stmt, 0, chain_out_var);
	}

	return cookie_var;
}
