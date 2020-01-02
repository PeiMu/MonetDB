/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 1997 - July 2008 CWI, August 2008 - 2020 MonetDB B.V.
 */

#include "monetdb_config.h"
#include "dotmonetdb.h"
#include <string.h>

void
parse_dotmonetdb(char **user, char **passwd, char **dbname, char **language, int *save_history, char **output, int *pagewidth)
{
	char *cfile;
	FILE *config = NULL;
	char buf[FILENAME_MAX];

	if ((cfile = getenv("DOTMONETDBFILE")) == NULL) {
		/* no environment variable: use a default */
		if ((config = fopen(".monetdb", "r")) == NULL) {
			if ((cfile = getenv("HOME")) != NULL) {
				int len = snprintf(buf, sizeof(buf), "%s%c.monetdb", cfile, DIR_SEP);
				if (len == -1 || len >= FILENAME_MAX) {
					cfile = NULL;
				} else {
					config = fopen(buf, "r");
					if (config)
						cfile = strdup(buf);
					else
						cfile = NULL;
				}
			}
		} else {
			cfile = strdup(".monetdb");
		}
	} else if (*cfile == 0) {
		/* empty environment variable: skip the file */
		cfile = NULL;
	} else if ((config = fopen(cfile, "r")) == NULL) {
		fprintf(stderr, "failed to open file '%s': %s\n",
			cfile, strerror(errno));
		cfile = NULL;
	} else {
		cfile = strdup(cfile);
	}

	if (user)
		*user = NULL;
	if (passwd)
		*passwd = NULL;
	if (dbname)
		*dbname = NULL;
	if (language)
		*language = NULL;
	if (output)
		*output = NULL;
	if (save_history)
		*save_history = 0;
	if (pagewidth)
		*pagewidth = 0;

	if (config) {
		int line = 0;
		char *q;
		while (fgets(buf, sizeof(buf), config) != NULL) {
			line++;
			q = strchr(buf, '\n');
			if (q)
				*q = 0;
			if (buf[0] == '\0' || buf[0] == '#')
				continue;
			if ((q = strchr(buf, '=')) == NULL) {
				fprintf(stderr, "%s:%d: syntax error: %s\n",
					cfile, line, buf);
				continue;
			}
			*q++ = '\0';
			/* this basically sucks big time, as I can't easily set
			 * a default, hence I only do things I think are useful
			 * for now, needs a better solution */
			if (strcmp(buf, "user") == 0) {
				if (user)
					*user = strdup(q);
				q = NULL;
			} else if (strcmp(buf, "password") == 0) {
				if (passwd)
					*passwd = strdup(q);
				q = NULL;
			} else if (strcmp(buf, "database") == 0) {
				if (dbname)
					*dbname = strdup(q);
				q = NULL;
			} else if (strcmp(buf, "language") == 0) {
				/* make sure we don't set garbage */
				if (strcmp(q, "sql") != 0 &&
				    strcmp(q, "mal") != 0) {
					fprintf(stderr, "%s:%d: unsupported "
						"language: %s\n",
						cfile, line, q);
				} else if (language)
					*language = strdup(q);
				q = NULL;
			} else if (strcmp(buf, "save_history") == 0) {
				if (strcmp(q, "true") == 0 ||
				    strcmp(q, "on") == 0) {
					if (save_history)
						*save_history = 1;
					q = NULL;
				} else if (strcmp(q, "false") == 0 ||
					   strcmp(q, "off") == 0) {
					if (save_history)
						*save_history = 0;
					q = NULL;
				}
			} else if (strcmp(buf, "format") == 0) {
				if (output)
					*output = strdup(q);
				q = NULL;
			} else if (strcmp(buf, "width") == 0) {
				if (pagewidth)
					*pagewidth = atoi(q);
				q = NULL;
			}
			if (q != NULL)
				fprintf(stderr, "%s:%d: unknown property: %s\n",
					cfile, line, buf);
		}
	}
	if (cfile)
		free(cfile);
	if (config)
		fclose(config);
}
