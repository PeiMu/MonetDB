/*
 * This code was created by Peter Harvey (mostly during Christmas 98/99).
 * This code is LGPL. Please ensure that this message remains in future
 * distributions and uses of this code (thats about all I get out of it).
 * - Peter Harvey pharvey@codebydesign.com
 * 
 * This file has been modified for the MonetDB project.  See the file
 * Copyright in this directory for more information.
 */

/**********************************************************************
 * SQLTablePrivileges()
 * CLI Compliance: ODBC (Microsoft)
 *
 * Note: this function is not implemented (it only sets an error),
 * because monetDB SQL frontend does not support table based authorization.
 *
 * Author: Martin van Dinther
 * Date  : 30 aug 2002
 *
 **********************************************************************/

#include "ODBCGlobal.h"
#include "ODBCStmt.h"
#include "ODBCUtil.h"


static SQLRETURN
SQLTablePrivileges_(ODBCStmt *stmt,
		    SQLCHAR *szCatalogName, SQLSMALLINT nCatalogNameLength,
		    SQLCHAR *szSchemaName, SQLSMALLINT nSchemaNameLength,
		    SQLCHAR *szTableName, SQLSMALLINT nTableNameLength)
{
	fixODBCstring(szCatalogName, nCatalogNameLength, addStmtError, stmt);
	fixODBCstring(szSchemaName, nSchemaNameLength, addStmtError, stmt);
	fixODBCstring(szTableName, nTableNameLength, addStmtError, stmt);

	/* check statement cursor state, no query should be prepared or executed */
	if (stmt->State != INITED) {
		/* 24000 = Invalid cursor state */
		addStmtError(stmt, "24000", NULL, 0);
		return SQL_ERROR;
	}

	/* SQLTablePrivileges returns a table with the following columns:
	   VARCHAR	table_cat
	   VARCHAR	table_schem
	   VARCHAR	table_name NOT NULL
	   VARCHAR	grantor
	   VARCHAR	grantee NOT NULL
	   VARCHAR	privilege NOT NULL
	   VARCHAR	is_grantable
	 */

	/* IM001 = Driver does not support this function */
	addStmtError(stmt, "IM001", NULL, 0);

	return SQL_ERROR;
}

SQLRETURN SQL_API
SQLTablePrivileges(SQLHSTMT hStmt,
		   SQLCHAR *szCatalogName, SQLSMALLINT nCatalogNameLength,
		   SQLCHAR *szSchemaName, SQLSMALLINT nSchemaNameLength,
		   SQLCHAR *szTableName, SQLSMALLINT nTableNameLength)
{
	ODBCStmt *stmt = (ODBCStmt *) hStmt;

#ifdef ODBCDEBUG
	ODBCLOG("SQLTablePrivileges\n");
#endif

	if (!isValidStmt(stmt))
		 return SQL_INVALID_HANDLE;

	clearStmtErrors(stmt);

	return SQLTablePrivileges_(stmt, szCatalogName, nCatalogNameLength,
				   szSchemaName, nSchemaNameLength,
				   szTableName, nTableNameLength);
}

SQLRETURN SQL_API
SQLTablePrivilegesW(SQLHSTMT hStmt,
		    SQLWCHAR *szCatalogName, SQLSMALLINT nCatalogNameLength,
		    SQLWCHAR *szSchemaName, SQLSMALLINT nSchemaNameLength,
		    SQLWCHAR *szTableName, SQLSMALLINT nTableNameLength)
{
	ODBCStmt *stmt = (ODBCStmt *) hStmt;
	SQLRETURN rc;
	SQLCHAR *catalog = NULL, *schema = NULL, *table = NULL;

#ifdef ODBCDEBUG
	ODBCLOG("SQLTablePrivilegesW\n");
#endif

	if (!isValidStmt(stmt))
		 return SQL_INVALID_HANDLE;

	clearStmtErrors(stmt);

	fixWcharIn(szCatalogName, nCatalogNameLength, catalog, addStmtError, stmt, goto exit);
	fixWcharIn(szSchemaName, nSchemaNameLength, schema, addStmtError, stmt, goto exit);
	fixWcharIn(szTableName, nTableNameLength, table, addStmtError, stmt, goto exit);

	rc = SQLTablePrivileges_(stmt, catalog, SQL_NTS, schema, SQL_NTS,
				 table, SQL_NTS);

  exit:
	if (catalog)
		free(catalog);
	if (schema)
		free(schema);
	if (table)
		free(table);

	return rc;
}
