/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 1997 - July 2008 CWI, August 2008 - 2023 MonetDB B.V.
 */

#ifndef MAL_PRELUDE
#define MAL_PRELUDE
#include "opt_support.h"

/* cf., gdk/gdk.h */
/* ! please keep this list sorted for easier maintenance ! */
mal_export  const char *affectedRowsRef;
mal_export  const char *aggrRef;
mal_export  const char *alarmRef;
mal_export  const char *algebraRef;
mal_export  const char *alter_add_range_partitionRef;
mal_export  const char *alter_add_tableRef;
mal_export  const char *alter_add_value_partitionRef;
mal_export  const char *alter_del_tableRef;
mal_export  const char *alter_seqRef;
mal_export  const char *alter_set_tableRef;
mal_export  const char *alter_tableRef;
mal_export  const char *alter_userRef;
mal_export  const char *appendBulkRef;
mal_export  const char *appendRef;
mal_export  const char *assertRef;
mal_export  const char *avgRef;
mal_export  const char *bandjoinRef;
mal_export  const char *batalgebraRef;
mal_export  const char *batcalcRef;
mal_export  const char *batcapiRef;
mal_export  const char *batmalRef;
mal_export  const char *batmkeyRef;
mal_export  const char *batmmathRef;
mal_export  const char *batmtimeRef;
mal_export  const char *batpyapi3Ref;
mal_export  const char *batrapiRef;
mal_export  const char *batRef;
mal_export  const char *batsqlRef;
mal_export  const char *batstrRef;
mal_export  const char *bbpRef;
mal_export  const char *betweenRef;
mal_export  const char *binddbatRef;
mal_export  const char *bindidxRef;
mal_export  const char *bindRef;
mal_export  const char *blockRef;
mal_export  const char *bstreamRef;
mal_export  const char *calcRef;
mal_export  const char *capiRef;
mal_export  const char *claimRef;
mal_export  const char *clear_tableRef;
mal_export  const char *columnBindRef;
mal_export  const char *comment_onRef;
mal_export  const char *compressRef;
mal_export  const char *connectRef;
mal_export  const char *containsRef;
mal_export  const char *copy_fromRef;
mal_export  const char *corrRef;
mal_export  const char *count_no_nilRef;
mal_export  const char *countRef;
mal_export  const char *create_functionRef;
mal_export  const char *create_roleRef;
mal_export  const char *create_schemaRef;
mal_export  const char *create_seqRef;
mal_export  const char *create_tableRef;
mal_export  const char *create_triggerRef;
mal_export  const char *create_typeRef;
mal_export  const char *create_userRef;
mal_export  const char *create_viewRef;
mal_export  const char *crossRef;
mal_export  const char *cume_distRef;
mal_export  const char *dataflowRef;
mal_export  const char *dblRef;
mal_export  const char *decompressRef;
mal_export  const char *defineRef;
mal_export  const char *deleteRef;
mal_export  const char *deltaRef;
mal_export  const char *dense_rankRef;
mal_export  const char *dependRef;
mal_export  const char *deregisterRef;
mal_export  const char *dictRef;
mal_export  const char *diffcandRef;
mal_export  const char *differenceRef;
mal_export  const char *diffRef;
mal_export  const char *disconnectRef;
mal_export  const char *divRef;
mal_export  const char *drop_constraintRef;
mal_export  const char *drop_functionRef;
mal_export  const char *drop_indexRef;
mal_export  const char *drop_roleRef;
mal_export  const char *drop_schemaRef;
mal_export  const char *drop_seqRef;
mal_export  const char *drop_tableRef;
mal_export  const char *drop_triggerRef;
mal_export  const char *drop_typeRef;
mal_export  const char *drop_userRef;
mal_export  const char *drop_viewRef;
mal_export  const char *emptybindidxRef;
mal_export  const char *emptybindRef;
mal_export  const char *endsWithRef;
mal_export  const char *eqRef;
mal_export  const char *evalRef;
mal_export  const char *execRef;
mal_export  const char *export_bin_columnRef;
mal_export  const char *exportOperationRef;
mal_export  const char *export_tableRef;
mal_export  const char *fetchRef;
mal_export  const char *findRef;
mal_export  const char *firstnRef;
mal_export  const char *first_valueRef;
mal_export  const char *forRef;
mal_export  const char *generatorRef;
mal_export  const char *getRef;
mal_export  const char *getTraceRef;
mal_export  const char *getVariableRef;
mal_export  const char *grant_functionRef;
mal_export  const char *grantRef;
mal_export  const char *grant_rolesRef;
mal_export  const char *groupbyRef;
mal_export  const char *groupdoneRef;
mal_export  const char *groupRef;
mal_export  const char *growRef;
mal_export  const char *hgeRef;
mal_export  const char *identityRef;
mal_export  const char *ifthenelseRef;
mal_export  const char *importColumnRef;
mal_export  const char *intersectcandRef;
mal_export  const char *intersectRef;
mal_export  const char *intRef;
mal_export  const char *ioRef;
mal_export  const char *iteratorRef;
mal_export  const char *joinRef;
mal_export  const char *jsonRef;
mal_export  const char *lagRef;
mal_export  const char *languageRef;
mal_export  const char *last_valueRef;
mal_export  const char *leadRef;
mal_export  const char *leftjoinRef;
mal_export  const char *likejoinRef;
mal_export  const char *likeRef;
mal_export  const char *likeselectRef;
mal_export  const char *likeuselectRef;
mal_export  const char *lockRef;
mal_export  const char *lookupRef;
mal_export  const char *malRef;
mal_export  const char *manifoldRef;
mal_export  const char *mapiRef;
mal_export  const char *maskRef;
mal_export  const char *matRef;
mal_export  const char *maxlevenshteinRef;
mal_export  const char *maxRef;
mal_export  const char *mdbRef;
mal_export  const char *mergecandRef;
mal_export  const char *mergepackRef;
mal_export  const char *mergetableRef;
mal_export  const char *minRef;
mal_export  const char *minjarowinklerRef;
mal_export  const char *minusRef;
mal_export  const char *mirrorRef;
mal_export  const char *mitosisRef;
mal_export  const char *mmathRef;
mal_export  const char *modRef;
mal_export  const char *mtimeRef;
mal_export  const char *mulRef;
mal_export  const char *multiplexRef;
mal_export  const char *mvcRef;
mal_export  const char *newRef;
mal_export  const char *nextRef;
mal_export  const char *not_likeRef;
mal_export  const char *notRef;
mal_export  const char *not_uniqueRef;
mal_export  const char *nth_valueRef;
mal_export  const char *ntileRef;
mal_export  const char *optimizerRef;
mal_export  const char *outerjoinRef;
mal_export  const char *packIncrementRef;
mal_export  const char *packRef;
mal_export  const char *parametersRef;
mal_export  const char *passRef;
mal_export  const char *percent_rankRef;
mal_export  const char *plusRef;
mal_export  const char *predicateRef;
mal_export  const char *printRef;
mal_export  const char *prodRef;
mal_export  const char *profilerRef;
mal_export  const char *projectdeltaRef;
mal_export  const char *projectionpathRef;
mal_export  const char *projectionRef;
mal_export  const char *projectRef;
mal_export  const char *putRef;
mal_export  const char *pyapi3mapRef;
mal_export  const char *pyapi3Ref;
mal_export  const char *querylogRef;
mal_export  const char *raiseRef;
mal_export  const char *rangejoinRef;
mal_export  const char *rankRef;
mal_export  const char *rapiRef;
mal_export  const char *reconnectRef;
mal_export  const char *registerRef;
mal_export  const char *register_supervisorRef;
mal_export  const char *remapRef;
mal_export  const char *remoteRef;
mal_export  const char *rename_columnRef;
mal_export  const char *rename_schemaRef;
mal_export  const char *rename_tableRef;
mal_export  const char *rename_userRef;
mal_export  const char *renumberRef;
mal_export  const char *replaceRef;
mal_export  const char *resultSetRef;
mal_export  const char *revoke_functionRef;
mal_export  const char *revokeRef;
mal_export  const char *revoke_rolesRef;
mal_export  const char *row_numberRef;
mal_export  const char *rpcRef;
mal_export  const char *rsColumnRef;
mal_export  const char *sampleRef;
mal_export  const char *selectNotNilRef;
mal_export  const char *selectRef;
mal_export  const char *semaRef;
mal_export  const char *semijoinRef;
mal_export  const char *seriesRef;
mal_export  const char *setAccessRef;
mal_export  const char *set_protocolRef;
mal_export  const char *setVariableRef;
mal_export  const char *singleRef;
mal_export  const char *sliceRef;
mal_export  const char *sortRef;
mal_export  const char *sortReverseRef;
mal_export  const char *sqlcatalogRef;
mal_export  const char *sqlRef;
mal_export  const char *startsWithRef;
mal_export  const char *stoptraceRef;
mal_export  const char *streamsRef;
mal_export  const char *strimpsRef;
mal_export  const char *strRef;
mal_export  const char *subavgRef;
mal_export  const char *subcountRef;
mal_export  const char *subdeltaRef;
mal_export  const char *subdiffRef;
mal_export  const char *subeval_aggrRef;
mal_export  const char *subgroupdoneRef;
mal_export  const char *subgroupRef;
mal_export  const char *submaxRef;
mal_export  const char *subminRef;
mal_export  const char *subprodRef;
mal_export  const char *subsliceRef;
mal_export  const char *subsumRef;
mal_export  const char *subuniformRef;
mal_export  const char *sumRef;
mal_export  const char *takeRef;
mal_export  const char *thetajoinRef;
mal_export  const char *thetaselectRef;
mal_export  const char *tidRef;
mal_export  const char *totalRef;
mal_export  const char *transaction_abortRef;
mal_export  const char *transaction_beginRef;
mal_export  const char *transaction_commitRef;
mal_export  const char *transactionRef;
mal_export  const char *transaction_releaseRef;
mal_export  const char *transaction_rollbackRef;
mal_export  const char *umaskRef;
mal_export  const char *uniqueRef;
mal_export  const char *unlockRef;
mal_export  const char *updateRef;
mal_export  const char *userRef;
mal_export  const char *window_boundRef;
mal_export  const char *zero_or_oneRef;
/* ! please keep this list sorted for easier maintenance ! */
#endif
