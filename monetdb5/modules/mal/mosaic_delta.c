/*
 * The contents of this file are subject to the MonetDB Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.monetdb.org/Legal/MonetDBLicense
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is the MonetDB Database System.
 *
 * The Initial Developer of the Original Code is CWI.
 * Portions created by CWI are Copyright (C) 1997-July 2008 CWI.
 *                * Copyright August 2008-2014 MonetDB B.V.
 * All Rights Reserved.
 */

/*
 * (c)2014 author Martin Kersten
 * Byte-wise delta encoding for SHT,INT,LNG,TIMESTAMP
 */

#include "monetdb_config.h"
#include "mosaic.h"
#include "mosaic_delta.h"

void
MOSdump_delta(Client cntxt, MOStask task)
{
	MosaicBlk blk = task->blk;
	mnstr_printf(cntxt->fdout,"#delta "LLFMT"\n", (lng)(blk->cnt));
}

void
MOSadvance_delta(MOStask task)
{
	MosaicBlk blk = task->blk;
	switch(task->type){
	case TYPE_sht: task->blk = (MosaicBlk)( ((char*) task->blk) + MosaicBlkSize + wordaligned(sizeof(sht) + blk->cnt-1)); break ;
	case TYPE_int: task->blk = (MosaicBlk)( ((char*) task->blk) + MosaicBlkSize + wordaligned(sizeof(int) + blk->cnt-1)); break ;
	case TYPE_lng: task->blk = (MosaicBlk)( ((char*) task->blk) + MosaicBlkSize + wordaligned(sizeof(lng) + blk->cnt-1)); break ;
	default:
		if( task->type == TYPE_timestamp)
			task->blk = (MosaicBlk)( ((char*) task->blk) + MosaicBlkSize + wordaligned(sizeof(timestamp) + blk->cnt-1)); 
	}
}

void
MOSskip_delta(MOStask task)
{
	MOSadvance_delta(task);
	if ( task->blk->tag == MOSAIC_EOL)
		task->blk = 0; // ENDOFLIST
}

// append a series of values into the non-compressed block
#define Estimate_delta(TYPE)\
{	TYPE *w = (TYPE*)task->src, val= *w, delta;\
	for(w++,i =1; i<task->elm; i++,w++){\
		delta = *w -val;\
		if ( delta < -127 || delta >127)\
			break;\
		val = *w;\
	}\
	percentage = 100 * (sizeof(TYPE)+(int)i-1) / ((int)i * sizeof(TYPE));\
}

int
MOSestimate_delta(Client cntxt, MOStask task)
{	BUN i = -1;
	int percentage = -1;
	(void) cntxt;

	switch(ATOMstorage(task->type)){
	case TYPE_sht: Estimate_delta(sht); break;
	case TYPE_lng: Estimate_delta(lng); break;
	case TYPE_int:
		{	int *w = (int*)task->src, val= *w, delta;
			for(w++,i =1; i<task->elm; i++,w++){
				delta = *w -val;
				if ( delta < -127 || delta >127)
					break;
				val = *w;
			}
			percentage = 100 * (sizeof(int)+(int)i-1) / ((int)i * sizeof(int));
		}
	}
#ifdef _DEBUG_MOSAIC_
	mnstr_printf(cntxt->fdout,"#estimate delta %d elm %d perc\n",(int)i,percentage);
#endif
	return percentage;
}

#define DELTAcompress(TYPE)\
{	TYPE *w = (TYPE*)task->src, val= *w, delta;\
	task->dst = ((char*) task->blk) + MosaicBlkSize;\
	*(TYPE*)task->dst = val;\
	task->dst += sizeof(TYPE);\
	for(w++,i =1; i<task->elm; i++,w++){\
		delta = *w -val;\
		if ( delta < -127 || delta >127)\
			break;\
		*(bte*)task->dst++ = (bte) delta;\
		val = val + delta;\
	}\
	task->src += i * sizeof(TYPE);\
	blk->cnt += i;\
}

// rather expensive simple value non-compressed store
void
MOScompress_delta(Client cntxt, MOStask task)
{
	MosaicBlk blk = (MosaicBlk) task->blk;
	BUN i = 0;

	(void) cntxt;
	blk->tag = MOSAIC_DELTA;
    task->time[MOSAIC_DELTA] = GDKusec();

	switch(ATOMstorage(task->type)){
	case TYPE_sht: DELTAcompress(sht); break;
	case TYPE_lng: DELTAcompress(lng); break;
	case TYPE_int:
		{	int *w = (int*)task->src, val= *w, delta;
			task->dst = ((char*) task->blk) + MosaicBlkSize;
			*(int*)task->dst = val;
			task->dst += sizeof(int);
			for(w++,i =1; i<task->elm; i++,w++){
				delta = *w -val;
				if ( delta < -127 || delta >127)
					break;
				*(bte*)task->dst++ = (bte) delta;
				val = val+ delta;
			}
			task->src += i * sizeof(int);
			blk->cnt += i;
		}
	}
#ifdef _DEBUG_MOSAIC_
	MOSdump_delta_(cntxt, task);
#endif
    task->time[MOSAIC_DELTA] = GDKusec() - task->time[MOSAIC_DELTA];
}

// the inverse operator, extend the src
#define DELTAdecompress(TYPE)\
{ TYPE val;\
	task->dst = ((char*) task->blk) + MosaicBlkSize;\
	val = *(TYPE*)task->dst ;\
	task->dst += sizeof(TYPE);\
	((int*)task->src)[0] = val;\
	for(i = 1; i < (BUN) blk->cnt; i++) {\
		val = ((TYPE*)task->src)[i] = val + *task->dst++;\
	}\
	task->src += i * sizeof(int);\
}

void
MOSdecompress_delta(Client cntxt, MOStask task)
{
	MosaicBlk blk = (MosaicBlk) task->blk;
	BUN i;
	lng clk = GDKusec();
	(void) cntxt;

	switch(ATOMstorage(task->type)){
	case TYPE_sht: DELTAdecompress(sht); break;
	case TYPE_lng: DELTAdecompress(lng); break;
	case TYPE_int:
	{ int val;
		task->dst = ((char*) task->blk) + MosaicBlkSize;
		val = *(int*)task->dst ;
		task->dst += sizeof(int);
		((int*)task->src)[0] = val;
		for(i = 1; i < (BUN) blk->cnt; i++) {
			val = ((int*)task->src)[i] = val + *(bte*) task->dst++;
		}
		task->src += i * sizeof(int);
	}
	}
    task->time[MOSAIC_DELTA] = GDKusec() - clk;
}

// The remainder should provide the minimal algebraic framework
//  to apply the operator to a DELTA compressed chunk

	
#define subselect_delta(TPE) {\
		TPE val= * (TPE*) (((char*) task->blk) + MosaicBlkSize);\
		task->dst = ((char*)task->blk)+ MosaicBlkSize + sizeof(TYPE);\
		if( !*anti){\
			if( *(TPE*) low == TPE##_nil && *(TPE*) hgh == TPE##_nil){\
				for( ; first < last; first++){\
					MOSskipit();\
					*o++ = (oid) first;\
				}\
			} else\
			if( *(TPE*) low == TPE##_nil ){\
				for( ; first < last; first++, val+= *(bte*)task->dst, task->dst++){\
					MOSskipit();\
					cmp  =  ((*hi && val <= * (TPE*)hgh ) || (!*hi && val < *(TPE*)hgh ));\
					if (cmp )\
						*o++ = (oid) first;\
				}\
			} else\
			if( *(TPE*) hgh == TPE##_nil ){\
				for( ; first < last; first++, val+= *(bte*)task->dst, task->dst++){\
					MOSskipit();\
					cmp  =  ((*li && val >= * (TPE*)low ) || (!*li && val > *(TPE*)low ));\
					if (cmp )\
						*o++ = (oid) first;\
				}\
			} else{\
				for( ; first < last; first++, val+= *(bte*)task->dst, task->dst++){\
					MOSskipit();\
					cmp  =  ((*hi && val <= * (TPE*)hgh ) || (!*hi && val < *(TPE*)hgh )) &&\
							((*li && val >= * (TPE*)low ) || (!*li && val > *(TPE*)low ));\
					if (cmp )\
						*o++ = (oid) first;\
				}\
			}\
		} else {\
			if( *(TPE*) low == TPE##_nil && *(TPE*) hgh == TPE##_nil){\
				/* nothing is matching */\
			} else\
			if( *(TPE*) low == TPE##_nil ){\
				for( ; first < last; first++, val+= *(bte*)task->dst, task->dst++){\
					MOSskipit();\
					cmp  =  ((*hi && val <= * (TPE*)hgh ) || (!*hi && val < *(TPE*)hgh ));\
					if ( !cmp )\
						*o++ = (oid) first;\
				}\
			} else\
			if( *(TPE*) hgh == TPE##_nil ){\
				for( ; first < last; first++, val+= *(bte*)task->dst, task->dst++){\
					MOSskipit();\
					cmp  =  ((*li && val >= * (TPE*)low ) || (!*li && val > *(TPE*)low ));\
					if ( !cmp )\
						*o++ = (oid) first;\
				}\
			} else{\
				for( ; first < last; first++, val+= *(bte*)task->dst, task->dst++){\
					MOSskipit();\
					cmp  =  ((*hi && val <= * (TPE*)hgh ) || (!*hi && val < *(TPE*)hgh )) &&\
							((*li && val >= * (TPE*)low ) || (!*li && val > *(TPE*)low ));\
					if ( !cmp )\
						*o++ = (oid) first;\
				}\
			}\
		}\
	}

str
MOSsubselect_delta(Client cntxt,  MOStask task, BUN first, BUN last, void *low, void *hgh, bit *li, bit *hi, bit *anti)
{
	oid *o;
	int cmp;
	(void) cntxt;
	(void) low;
	(void) hgh;
	(void) li;
	(void) hi;
	(void) anti;

	if ( first + task->blk->cnt > last)
		last = task->blk->cnt;
	o = task->lb;

	switch(task->type){
	case TYPE_bit: subselect_delta(bit); break;
	case TYPE_bte: subselect_delta(bte); break;
	case TYPE_sht: subselect_delta(sht); break;
	case TYPE_lng: subselect_delta(lng); break;
	case TYPE_flt: subselect_delta(flt); break;
	case TYPE_dbl: subselect_delta(dbl); break;
	case TYPE_int:
	// Expanded MOSselect_delta for debugging
		{ 	int val= *(int*) (((char*) task->blk) + MosaicBlkSize);
			task->dst = ((char *)task->blk) +MosaicBlkSize + sizeof(int);

			if( !*anti){
				if( *(int*) low == int_nil && *(int*) hgh == int_nil){
					for( ; first < last; first++, val+= *(bte*)task->dst, task->dst++){
						MOSskipit();
						*o++ = (oid) first;
					}
				} else
				if( *(int*) low == int_nil ){
					for( ; first < last; first++, val+= *(bte*)task->dst, task->dst++){
						MOSskipit();
						cmp  =  ((*hi && val <= * (int*)hgh ) || (!*hi && val < *(int*)hgh ));
						if (cmp )
							*o++ = (oid) first;
					}
				} else
				if( *(int*) hgh == int_nil ){
					for( ; first < last; first++, val+= *(bte*)task->dst, task->dst++){
						MOSskipit();
						cmp  =  ((*li && val >= * (int*)low ) || (!*li && val > *(int*)low ));
						if (cmp )
							*o++ = (oid) first;
					}
				} else{
					for( ; first < last; first++, val+= *(bte*)task->dst, task->dst++){
						MOSskipit();
						cmp  =  ((*hi && val <= * (int*)hgh ) || (!*hi && val < *(int*)hgh )) &&
								((*li && val >= * (int*)low ) || (!*li && val > *(int*)low ));
						if (cmp )
							*o++ = (oid) first;
					}
				}
			} else {
				if( *(int*) low == int_nil && *(int*) hgh == int_nil){
					/* nothing is matching */
				} else
				if( *(int*) low == int_nil ){
					for( ; first < last; first++, val+= *(bte*)task->dst, task->dst++){
						MOSskipit();
						cmp  =  ((*hi && val <= * (int*)hgh ) || (!*hi && val < *(int*)hgh ));
						if ( !cmp )
							*o++ = (oid) first;
					}
				} else
				if( *(int*) hgh == int_nil ){
					for( ; first < last; first++, val+= *(bte*)task->dst, task->dst++){
						MOSskipit();
						cmp  =  ((*li && val >= * (int*)low ) || (!*li && val > *(int*)low ));
						if ( !cmp )
							*o++ = (oid) first;
					}
				} else{
					for( ; first < last; first++, val+= *(bte*)task->dst, task->dst++){
						MOSskipit();
						cmp  =  ((*hi && val <= * (int*)hgh ) || (!*hi && val < *(int*)hgh )) &&
								((*li && val >= * (int*)low ) || (!*li && val > *(int*)low ));
						if ( !cmp )
							*o++ = (oid) first;
					}
				}
			}
		}
			break;
		default:
			if( task->type == TYPE_timestamp)
				{ 	lng val= *(lng*) (((char*) task->blk) + MosaicBlkSize);
					int lownil = timestamp_isnil(*(timestamp*)low);
					int hghnil = timestamp_isnil(*(timestamp*)hgh);

					if( !*anti){
						if( lownil && hghnil){
							for( ; first < last; first++, val+= *(bte*)task->dst, task->dst++){
								MOSskipit();
								*o++ = (oid) first;
							}
						} else
						if( lownil){
							for( ; first < last; first++, val+= *(bte*)task->dst, task->dst++){
								MOSskipit();
								cmp  =  ((*hi && val <= * (lng*)hgh ) || (!*hi && val < *(lng*)hgh ));
								if (cmp )
									*o++ = (oid) first;
							}
						} else
						if( hghnil){
							for( ; first < last; first++, val+= *(bte*)task->dst, task->dst++){
								MOSskipit();
								cmp  =  ((*li && val >= * (lng*)low ) || (!*li && val > *(lng*)low ));
								if (cmp )
									*o++ = (oid) first;
							}
						} else{
							for( ; first < last; first++, val+= *(bte*)task->dst, task->dst++){
								MOSskipit();
								cmp  =  ((*hi && val <= * (lng*)hgh ) || (!*hi && val < *(lng*)hgh )) &&
										((*li && val >= * (lng*)low ) || (!*li && val > *(lng*)low ));
								if (cmp )
									*o++ = (oid) first;
							}
						}
					} else {
						if( lownil && hghnil){
							/* nothing is matching */
						} else
						if( lownil){
							for( ; first < last; first++, val+= *(bte*)task->dst, task->dst++){
								MOSskipit();
								cmp  =  ((*hi && val <= * (lng*)hgh ) || (!*hi && val < *(lng*)hgh ));
								if ( !cmp )
									*o++ = (oid) first;
							}
						} else
						if( hghnil){
							for( ; first < last; first++, val+= *(bte*)task->dst, task->dst++){
								MOSskipit();
								cmp  =  ((*li && val >= * (lng*)low ) || (!*li && val > *(lng*)low ));
								if ( !cmp )
									*o++ = (oid) first;
							}
						} else{
							for( ; first < last; first++, val+= *(bte*)task->dst, task->dst++){
								MOSskipit();
								cmp  =  ((*hi && val <= * (lng*)hgh ) || (!*hi && val < *(lng*)hgh )) &&
										((*li && val >= * (lng*)low ) || (!*li && val > *(lng*)low ));
								if ( !cmp )
									*o++ = (oid) first;
							}
						}
					}
				}
	}
	task->lb = o;
	return MAL_SUCCEED;
}

#define thetasubselect_delta(TPE)\
{ 	TPE low,hgh, v;\
	low= hgh = TPE##_nil;\
	if ( strcmp(oper,"<") == 0){\
		hgh= *(TPE*) val;\
		hgh = PREVVALUE##TPE(hgh);\
	} else\
	if ( strcmp(oper,"<=") == 0){\
		hgh= *(TPE*) val;\
	} else\
	if ( strcmp(oper,">") == 0){\
		low = *(TPE*) val;\
		low = NEXTVALUE##TPE(low);\
	} else\
	if ( strcmp(oper,">=") == 0){\
		low = *(TPE*) val;\
	} else\
	if ( strcmp(oper,"!=") == 0){\
		hgh= low= *(TPE*) val;\
		anti++;\
	} else\
	if ( strcmp(oper,"==") == 0){\
		hgh= low= *(TPE*) val;\
	} \
	v= *(TPE*) (((char*) task->blk) + MosaicBlkSize);\
	task->dst = ((char *)task->blk) +MosaicBlkSize + sizeof(int);\
	for( ; first < last; first++, v+= *(bte*)task->dst, task->dst++){\
		if( (low == TPE##_nil || v >= low) && (v <= hgh || hgh == TPE##_nil) ){\
			if ( !anti) {\
				MOSskipit();\
				*o++ = (oid) first;\
			}\
		} else\
		if( anti){\
			MOSskipit();\
			*o++ = (oid) first;\
		}\
	}\
} 

str
MOSthetasubselect_delta(Client cntxt,  MOStask task, BUN first, BUN last, void *val, str oper)
{
	oid *o;
	int anti=0;
	(void) cntxt;
	
	if ( first + task->blk->cnt > last)
		last = task->blk->cnt;
	o = task->lb;

	switch(task->type){
	case TYPE_bit: thetasubselect_delta(bit); break;
	case TYPE_bte: thetasubselect_delta(bte); break;
	case TYPE_sht: thetasubselect_delta(sht); break;
	case TYPE_lng: thetasubselect_delta(lng); break;
	case TYPE_flt: thetasubselect_delta(flt); break;
	case TYPE_dbl: thetasubselect_delta(dbl); break;
	case TYPE_int:
		{ 	int low,hgh, v;
			low= hgh = int_nil;
			if ( strcmp(oper,"<") == 0){
				hgh= *(int*) val;
				hgh = PREVVALUEint(hgh);
			} else
			if ( strcmp(oper,"<=") == 0){
				hgh= *(int*) val;
			} else
			if ( strcmp(oper,">") == 0){
				low = *(int*) val;
				low = NEXTVALUEint(low);
			} else
			if ( strcmp(oper,">=") == 0){
				low = *(int*) val;
			} else
			if ( strcmp(oper,"!=") == 0){
				hgh= low= *(int*) val;
				anti++;
			} else
			if ( strcmp(oper,"==") == 0){
				hgh= low= *(int*) val;
			} 
		 	v= *(int*) (((char*) task->blk) + MosaicBlkSize);
			task->dst = ((char *)task->blk) +MosaicBlkSize + sizeof(int);
			for( ; first < last; first++, v+= *(bte*)task->dst, task->dst++){
				if( (low == int_nil || v >= low) && (v <= hgh || hgh == int_nil) ){
					if ( !anti) {
						MOSskipit();
						*o++ = (oid) first;
					}
				} else
				if( anti){
					MOSskipit();
					*o++ = (oid) first;
				}
			}
		} 
		break;
	default:
			if( task->type == TYPE_timestamp)
				thetasubselect_delta(lng); 
	}
	task->lb =o;
	return MAL_SUCCEED;
}

#define leftfetchjoin_delta(TPE)\
{	TPE *val, *v;\
	v= (TPE*) task->src;\
	val = (TPE*) (((char*) task->blk) + MosaicBlkSize);\
	for(; first < last; first++, val++){\
		MOSskipit();\
		*v++ = *val;\
		task->n--;\
	}\
	task->src = (char*) v;\
}

str
MOSleftfetchjoin_delta(Client cntxt,  MOStask task, BUN first, BUN last)
{
	(void) cntxt;

	switch(task->type){
		case TYPE_bit: leftfetchjoin_delta(bit); break;
		case TYPE_bte: leftfetchjoin_delta(bte); break;
		case TYPE_sht: leftfetchjoin_delta(sht); break;
		case TYPE_lng: leftfetchjoin_delta(lng); break;
		case TYPE_flt: leftfetchjoin_delta(flt); break;
		case TYPE_dbl: leftfetchjoin_delta(dbl); break;
		case TYPE_int:
		{	int *val, *v;
			v= (int*) task->src;
			val = (int*) (((char*) task->blk) + MosaicBlkSize);
			for(; first < last; first++, val++){
				MOSskipit();
				*v++ = *val;
				task->n--;
			}
			task->src = (char*) v;
		}
		break;
		default:
			if (task->type == TYPE_timestamp)
				leftfetchjoin_delta(lng); 
	}
	return MAL_SUCCEED;
}

#define join_delta(TPE)\
{	TPE *v, *w;\
	v = (TPE*) (((char*) task->blk) + MosaicBlkSize);\
	for(oo= (oid) first; first < last; first++, v++, oo++){\
		w = (TPE*) task->src;\
		for(n = task->elm, o = 0; n -- > 0; w++,o++)\
		if ( *w == *v){\
			BUNappend(task->lbat, &oo, FALSE);\
			BUNappend(task->rbat, &o, FALSE);\
		}\
	}\
}

str
MOSjoin_delta(Client cntxt,  MOStask task, BUN first, BUN last)
{
	BUN n;
	oid o, oo;
	(void) cntxt;

	switch(task->type){
		case TYPE_bit: join_delta(bit); break;
		case TYPE_bte: join_delta(bte); break;
		case TYPE_sht: join_delta(sht); break;
		case TYPE_lng: join_delta(lng); break;
		case TYPE_flt: join_delta(flt); break;
		case TYPE_dbl: join_delta(dbl); break;
		case TYPE_int:
		{	int *v, *w;
			v = (int*) (((char*) task->blk) + MosaicBlkSize);
			for(oo= (oid) first; first < last; first++, v++, oo++){
				w = (int*) task->src;
				for(n = task->elm, o = 0; n -- > 0; w++,o++)
				if ( *w == *v){
					BUNappend(task->lbat, &oo, FALSE);
					BUNappend(task->rbat, &o, FALSE);
				}
			}
		}
		break;
		default:
			if (task->type == TYPE_timestamp)
				join_delta(lng); 
	}
	return MAL_SUCCEED;
}
