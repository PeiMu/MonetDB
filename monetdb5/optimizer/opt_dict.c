/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 1997 - July 2008 CWI, August 2008 - 2021 MonetDB B.V.
 */

#include "monetdb_config.h"
#include "opt_dict.h"

static InstrPtr
ReplaceWithNil(MalBlkPtr mb, InstrPtr p, int pos, int tpe)
{
	p = pushNil(mb, p, tpe); /* push at end */
	getArg(p, pos) = getArg(p, p->argc-1);
	p->argc--;
	return p;
}

str
OPTdictImplementation(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci)
{
	int i, j, k, limit, slimit;
	InstrPtr p=0, *old=NULL;
	int actions = 0;
	int *varisdict=NULL, *vardictvalue=NULL;
	str msg= MAL_SUCCEED;

	(void) cntxt;
	(void) stk;		/* to fool compilers */

	if (mb->inlineProp)
		goto wrapup;

	varisdict = GDKzalloc(2 * mb->vtop * sizeof(int));
	vardictvalue = GDKzalloc(2 * mb->vtop * sizeof(int));
	if (varisdict == NULL || vardictvalue == NULL)
		goto wrapup;

	limit = mb->stop;
	slimit = mb->ssize;
	old = mb->stmt;
	if (newMalBlkStmt(mb, mb->ssize) < 0) {
		GDKfree(varisdict);
		throw(MAL,"optimizer.dict", SQLSTATE(HY013) MAL_MALLOC_FAIL);
	}

	// Consolidate the actual need for variables
	for (i = 0; i < limit; i++) {
		p = old[i];
		if( p == 0)
			continue; //left behind by others?
		if (p->retc == 1 && getModuleId(p) == dictRef && getFunctionId(p) == decompressRef) {
			// remember we have encountered a dict decompress function
			k =  getArg(p,0);
			varisdict[k] = getArg(p,1);
			vardictvalue[k] = getArg(p, 2);
			continue;
		}
		int done = 0;
		for(j=p->retc; j< p->argc; j++){
			k = getArg(p,j);
			if (varisdict[k]) { // maybe we could delay this usage
				if (getModuleId(p) == algebraRef && getFunctionId(p) == projectionRef) {
					/* projection(cand, col) with col = dict.decompress(o,u)
					 * v1 = projection(cand, o)
					 * dict.decompress(v1, u) */
					InstrPtr r = copyInstruction(p);
					int tpe = getVarType(mb, varisdict[k]);
					int l = getArg(r, 0);
					getArg(r, 0) = newTmpVariable(mb, tpe);
					getArg(r, j) = varisdict[k];
					varisdict[l] = getArg(r,0);
					vardictvalue[l] = vardictvalue[k];
					pushInstruction(mb,r);
					done = 1;
					break;
				} else if (getModuleId(p) == algebraRef && getFunctionId(p) == subsliceRef) {
					/* pos = subslice(col, l, h) with col = dict.decompress(o,u)
					 * pos = subslice(o, l, h) */
					InstrPtr r = copyInstruction(p);
					getArg(r, j) = varisdict[k];
					pushInstruction(mb,r);
					done = 1;
					break;
				} else if (getModuleId(p) == algebraRef && getFunctionId(p) == selectRef) {
					/* pos = select(col, cand, l, h, ...) with col = dict.decompress(o,u)
					 * tp = select(u, nil, l, h, ...)
					 * tp2 = bte/sht/int(tp)
					 * pos = intersect(o, tp2, cand, nil) */

					int cand = getArg(p, j+1);
					InstrPtr r = copyInstruction(p);
					getArg(r, j) = vardictvalue[k];
					if (cand)
						r = ReplaceWithNil(mb, r, j+1, TYPE_bat); /* no candidate list */
					pushInstruction(mb,r);

					int tpe = getVarType(mb, varisdict[k]);
					InstrPtr s = newInstructionArgs(mb, batcalcRef, putName(ATOMname(getBatType(tpe))), 3);
					getArg(s, 0) = newTmpVariable(mb, tpe);
					addArgument(mb, s, getArg(r, 0));
					s = pushNil(mb, s, TYPE_bat);
					pushInstruction(mb,s);

					InstrPtr t = newInstructionArgs(mb, algebraRef, intersectRef, 5);
					getArg(t, 0) = getArg(p, 0);
					addArgument(mb, t, varisdict[k]);
					addArgument(mb, t, getArg(s, 0));
					addArgument(mb, t, cand);
					t = pushNil(mb, t, TYPE_bat);
					t = pushBit(mb, t, FALSE);    /* nil matches */
					t = pushBit(mb, t, TRUE);     /* max_one */
					t = pushNil(mb, t, TYPE_lng); /* estimate */
					pushInstruction(mb,t);

					done = 1;
					break;
				} else {
					/* need to decompress */
					int tpe = getArgType(mb, p, j);
					InstrPtr r = newInstructionArgs(mb, dictRef, decompressRef, 3);
					getArg(r, 0) = newTmpVariable(mb, tpe);
					r = addArgument(mb, r, varisdict[k]);
					r = addArgument(mb, r, vardictvalue[k]);
					pushInstruction(mb, r);

					getArg(p, j) = getArg(r, 0);
				}
			}
		}
		if (!done)
			pushInstruction(mb, p);
	}

	for(; i<slimit; i++)
		if (old[i])
			pushInstruction(mb, old[i]);
	/* Defense line against incorrect plans */
	if (actions > 0){
		msg = chkTypes(cntxt->usermodule, mb, FALSE);
		if (!msg)
			msg = chkFlow(mb);
		if (!msg)
			msg = chkDeclarations(mb);
	}
	/* keep all actions taken as a post block comment */
wrapup:
	/* keep actions taken as a fake argument*/
	(void) pushInt(mb, pci, actions);

	GDKfree(old);
	GDKfree(varisdict);
	GDKfree(vardictvalue);
	return msg;
}
