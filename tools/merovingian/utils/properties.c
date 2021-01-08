/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 1997 - July 2008 CWI, August 2008 - 2021 MonetDB B.V.
 */

/**
 * properties
 * Fabian Groffen
 * Simple functions that deal with the property file
 */

#include "monetdb_config.h"
#include "properties.h"
#include "utils.h"
#include <string.h> /* memcpy */
#include <pthread.h>
#include <ctype.h>

#define MEROPROPFILEHEADER \
	"# DO NOT EDIT THIS FILE - use monetdb(1) and monetdbd(1) to set properties\n" \
	"# This file is used by monetdbd\n\n"

/* these are the properties used for starting an mserver */
static const confkeyval _internal_prop_keys[PROPLENGTH] = {
	{"type",        NULL, 0, STR},
	{"shared",      NULL, 0, STR},
	{"nthreads",    NULL, 0, INT},
	{"optpipe",     NULL, 0, STR},
	{"readonly",    NULL, 0, BOOLEAN},
	{"embedr",      NULL, 0, BOOLEAN},
	{"embedpy",     NULL, 0, BOOLEAN},
	{"embedpy3",    NULL, 0, BOOLEAN},
	{"embedc",      NULL, 0, BOOLEAN},
	{"listenaddr",  NULL, 0, LADDR},
	{"nclients",    NULL, 0, INT},
	{"mfunnel",     NULL, 0, STR},
	{"dbextra",     NULL, 0, STR},
	{"dbtrace",     NULL, 0, STR},
	{"memmaxsize",  NULL, 0, INT},
	{"vmmaxsize",   NULL, 0, INT},
	{"raw_strings", NULL, 0, BOOLEAN},
	{ NULL,         NULL, 0, INVALID}
};

static pthread_mutex_t readprops_lock = PTHREAD_MUTEX_INITIALIZER;

/**
 * Returns true if the key is a default property.
 */
int
defaultProperty(const char *property) {
	int i;
	if (property == NULL)
		return 0;
	for (i = 0; _internal_prop_keys[i].key != NULL; i++)
		if (strcmp(property, _internal_prop_keys[i].key) == 0)
			return 1;
	return 0;
}

/**
 * Returns the currently supported list of properties.  This list can be
 * used to read all values, modify some and write the file back again.
 * The returned list is malloced, the keys are a pointer to a static
 * copy and hence need not to be freed, e.g. free after freeConfFile
 * is enough.
 */
confkeyval *
getDefaultProps(void)
{
	confkeyval *ret = malloc(sizeof(_internal_prop_keys));
	memcpy(ret, _internal_prop_keys, sizeof(_internal_prop_keys));
	return(ret);
}

/**
 * Writes the given key-value list to MEROPROPFILE in the given path.
 * Returns 0 when the properties could be successfully written to the file.
 */
int
writeProps(confkeyval *ckv, const char *path)
{
	char file[1024];
	FILE *cnf;

	snprintf(file, 1024, "%s/" MEROPROPFILE, path);
	pthread_mutex_lock(&readprops_lock);
	if ((cnf = fopen(file, "w")) == NULL) {
		pthread_mutex_unlock(&readprops_lock);
		return(1);
	}

	fprintf(cnf, "%s", MEROPROPFILEHEADER);
	while (ckv->key != NULL) {
		if (ckv->val != NULL)
			fprintf(cnf, "%s=%s\n", ckv->key, ckv->val);
		ckv++;
	}

	fflush(cnf);
	fclose(cnf);
	pthread_mutex_unlock(&readprops_lock);

	return(0);
}

/**
 * Appends additional (non-default) property MEROPROPFILE in the given path.
 * Returns 0 when the property could be successfully appended to the file.
 */
static int
appendProp(confkeyval *ckv, const char *path)
{
	char file[1024];
	FILE *cnf;

	snprintf(file, 1024, "%s/" MEROPROPFILE, path);
	if ((cnf = fopen(file, "a")) == NULL)
		return(1);

	if (ckv->key != NULL && ckv->val != NULL) {
		fprintf(cnf, "%s=%s\n", ckv->key, ckv->val);
	}

	fflush(cnf);
	fclose(cnf);

	return(0);
}

/**
 * Writes the given key-value list to a buffer and sets its pointer to
 * buf.  This function deals with the allocation of the buffer, hence
 * the caller should free it.
 */
void
writePropsBuf(confkeyval *ckv, char **buf)
{
	confkeyval *w;
	size_t len = sizeof(MEROPROPFILEHEADER);
	char *p;

	w = ckv;
	while (w->key != NULL) {
		if (w->val != NULL)
			len += strlen(w->key) + 1 + strlen(w->val) + 1;
		w++;
	}

	p = *buf = malloc(sizeof(char) * len + 1);
	memcpy(p, MEROPROPFILEHEADER, sizeof(MEROPROPFILEHEADER));
	p += sizeof(MEROPROPFILEHEADER) - 1;
	w = ckv;
	while (w->key != NULL) {
		if (w->val != NULL)
			p += sprintf(p, "%s=%s\n", w->key, w->val);
		w++;
	}
}

/**
 * Read a property file, filling in the requested key-values.  Returns 0
 * when reading the property file succeeded.
 */
int
readProps(confkeyval *ckv, const char *path)
{
	char file[1024];
	FILE *cnf;

	snprintf(file, 1024, "%s/" MEROPROPFILE, path);
	pthread_mutex_lock(&readprops_lock);
	if ((cnf = fopen(file, "r")) != NULL) {
		readConfFile(ckv, cnf);
		fclose(cnf);
		pthread_mutex_unlock(&readprops_lock);
		return(0);
	}
	pthread_mutex_unlock(&readprops_lock);
	return(1);
}

/**
 * Read all properties from a  property file.
 * Returns 0 when reading the property file succeeded.
 */
int
readAllProps(confkeyval *ckv, const char *path)
{
	char file[1024];
	FILE *cnf;

	snprintf(file, 1024, "%s/" MEROPROPFILE, path);
	pthread_mutex_lock(&readprops_lock);
	if ((cnf = fopen(file, "r")) != NULL) {
		readConfFileFull(ckv, cnf);
		fclose(cnf);
		pthread_mutex_unlock(&readprops_lock);
		return(0);
	}
	pthread_mutex_unlock(&readprops_lock);
	return(1);
}

/**
 * Read properties from buf, filling in the requested key-values.
 */
void
readPropsBuf(confkeyval *ckv, char *buf)
{
	confkeyval *t;
	char *p;
	char *err;
	char *lasts = NULL;
	size_t len;

	while((p = strtok_r(buf, "\n", &lasts)) != NULL) {
		buf = NULL; /* strtok */
		for (t = ckv; t->key != NULL; t++) {
			len = strlen(t->key);
			if (strncmp(p, t->key, len) == 0 && p[len] == '=') {
				if ((err = setConfVal(t, p + len + 1)) != NULL)
					free(err); /* ignore, just fall back to default */
			}
		}
	}
}

/**
 * Sets a single property, performing all necessary checks to validate
 * the key and associated value.
 */
char *
setProp(const char *path, const char *key, const char *val)
{
	char *err;
	char buf[8096];
	confkeyval *props = getDefaultProps();
	confkeyval *kv;

	readProps(props, path);
	kv = findConfKey(props, key);
	if (kv != NULL && (err = setConfVal(kv, val)) != NULL) {
		/* first just attempt to set the value (type-check) in memory */
		freeConfFile(props);
		free(props);
		return(err);
	}

	if (val != NULL) {
		/* handle the semantially enriched types */
		if (strcmp(key, "forward") == 0) {
			if (strcmp(val, "proxy") != 0 && strcmp(val, "redirect") != 0) {
				snprintf(buf, sizeof(buf), "expected 'proxy' or 'redirect' "
						"for property 'forward', got: %s", val);
				freeConfFile(props);
				free(props);
				return(strdup(buf));
			}
		} else if (strcmp(key, "shared") == 0) {
			const char *value = val;
			/* check if tag matches [A-Za-z0-9./]+ */
			if (*value == '\0') {
				freeConfFile(props);
				free(props);
				return(strdup("tag to share cannot be empty"));
			}
			while (*value != '\0') {
				if (!(
							(*value >= 'A' && *value <= 'Z') ||
							(*value >= 'a' && *value <= 'z') ||
							isdigit((unsigned char) *value) ||
							(*value == '.' || *value == '/')
					 ))
				{
					snprintf(buf, sizeof(buf),
							"invalid character '%c' at %d "
							"in tag name '%s'\n",
							*value, (int)(value - val), val);
					freeConfFile(props);
					free(props);
					return(strdup(buf));
				}
				value++;
			}
		}
	}

	/* ok, if we've reached this point we can write this stuff out! */
	/* Let's check if this was a default property of an additional one.
	 * Non-default properties will have a NULL kv */
	if (kv == NULL && val != NULL) {
		confkeyval *addProperty = (struct _confkeyval *) malloc(sizeof(struct _confkeyval));
		addProperty->key = strdup(key);
		addProperty->val = strdup(val);
		addProperty->ival = 0;
		addProperty->type = STR;

		appendProp(addProperty, path);
		free(addProperty);
	} else {
		writeProps(props, path);
	}

	freeConfFile(props);
	free(props);

	return(NULL);
}

/* vim:set ts=4 sw=4 noexpandtab: */
