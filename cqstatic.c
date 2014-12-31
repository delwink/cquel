/*
 *  cquel - MySQL C API wrapper with dynamic data structures
 *  Copyright (C) 2014 Delwink, LLC
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <mysql.h>

#include "cquel.h"

extern size_t CQ_QLEN;
extern size_t  CQ_FMAXLEN;

int cq_query(struct dbconn *con, const char *query)
{
    return mysql_query(con->con, query);
}

int cq_fields_to_utf8(struct dbconn *con, char *buf, size_t buflen,
        size_t fieldc, char * const *fieldnames, bool usequotes)
{
    int rc = 0;
    bool connecting = !con->isopen;
    size_t num_left = fieldc, written = 0;

    if (num_left == 0)
        return 1;

    char *temp = calloc(CQ_FMAXLEN+3, sizeof(char));
    if (NULL == temp)
        return -1;

    char *field = calloc((CQ_FMAXLEN+3)*2+1, sizeof(char));
    if (NULL == field) {
        free(temp);
        return -2;
    }

    /* prevent appending to buffer */
    buf[0] = '\0';

    if (connecting)
        cq_connect(con);
    for (size_t i = 0; i < fieldc; ++i) {
        bool escaped = fieldnames[i][0] == '\\';
        const char *orig = escaped ? &fieldnames[i][1] : fieldnames[i];
        const char *value;

        bool isstr = false;
        if (!escaped) {
            mysql_real_escape_string(con->con, field, orig, strlen(orig));
            value = field;
            if (usequotes)
                for (size_t j = 0; j < strlen(value); ++j) {
                    if (!isdigit(value[j])) {
                        isstr = true;
                        break;
                    }
                }
        } else {
            value = orig;
        }

        const char *a = isstr ? "'" : "";
        const char *c = --num_left > 0 ? "," : "";
        written += snprintf(temp, CQ_FMAXLEN+3, "%s%s%s%s", a, value, a, c);
        if (written >= buflen) {
            rc = 2;
            break;
        }

        strcat(buf, temp);
    }
    if (connecting)
        cq_close_connection(con);

    free(field);
    free(temp);
    return rc;
}

int cq_dlist_to_update_utf8(struct dbconn *con, char *buf, size_t buflen,
        struct dlist list, struct drow row)
{
    int rc = 0;
    bool connecting = !con->isopen;
    size_t num_left = list.fieldc, written = 0;

    if (num_left == 0)
        return 1;

    char *temp = calloc(CQ_FMAXLEN+3, sizeof(char));
    if (NULL == temp)
        return -1;

    char *tempf = calloc((CQ_FMAXLEN+3)*2+1, sizeof(char));
    if (NULL == tempf) {
        free(temp);
        return -2;
    }

    char *tempv = calloc((CQ_FMAXLEN+3)*2+1, sizeof(char));
    if (NULL == tempv) {
        free(temp);
        free(tempf);
        return -3;
    }

    /* prevent appending to buffer */
    buf[0] = '\0';

    if (connecting)
        cq_connect(con);
    for (size_t i = 0; i < list.fieldc; ++i) {
        if (!strcmp(list.fieldnames[i], list.primkey)) {
            --num_left;
            continue;
        }

        bool v_escaped = row.values[i][0] == '\\';
        const char *v_orig = v_escaped ?
                &row.values[i][1] : row.values[i];
        const char *f = list.fieldnames[i], *v_value;

        mysql_real_escape_string(con->con, tempf, f, strlen(f));

        bool isstr = false;
        if (!v_escaped) {
            mysql_real_escape_string(con->con, tempv, v_orig, strlen(v_orig));
            v_value = tempv;
            for (size_t j = 0; j < strlen(v_value); ++j) {
                if (!isdigit(v_value[j])) {
                    isstr = true;
                    break;
                }
            }
        } else {
            v_value = v_orig;
        }

        const char *a = isstr ? "'" : "";
        const char *c = --num_left > 0 ? "," : "";
        written += snprintf(temp, CQ_FMAXLEN+3, "%s=%s%s%s%s",
                tempf,
                a, v_value, a, c);
        if (written >= buflen) {
            rc = 2;
            break;
        }

        strcat(buf, temp);
    }
    if (connecting)
        cq_close_connection(con);

    free(tempv);
    free(tempf);
    free(temp);
    return rc;
}

int cq_dlist_fields_to_utf8(struct dbconn *con, char *buf, size_t buflen,
        struct dlist list)
{
    return cq_fields_to_utf8(con, buf, buflen, list.fieldc, list.fieldnames,
            false);
}

int cq_drow_to_utf8(struct dbconn *con, char *buf, size_t buflen,
        struct drow row)
{
    return cq_fields_to_utf8(con, buf, buflen, row.fieldc, row.values,
            true);
}

int dlist_meta_cmp(const struct dlist *a, const struct dlist *b)
{
	int rc = 0;
	if (a->fieldc != b->fieldc)
		return a->fieldc - b->fieldc;

	if ( (rc = strcmp(a->primkey, b->primkey)) )
		return rc;

	for (size_t i=0; i < a->fieldc; ++i) {
		if ( (rc = strcmp(a->fieldnames[i], b->fieldnames[i])) )
			break;
	}

	return rc;
}

int grant_revoke(struct dbconn con, const char *act, const char *perms,
        const char *table, const char *user, const char *host,
        const char *extra)
{
    int rc;
    char *query;
    const char *fmt = "%s %s ON %s %s '%s'@'%s' %s";

    if (NULL == act || NULL == perms || NULL == table || NULL == user
            || NULL == host || NULL == extra)
        return 1;

    query = calloc(CQ_QLEN, sizeof(char));
    if (NULL == query)
        return -1;

    rc = snprintf(query, CQ_QLEN, fmt,
            act,
            perms,
            table,
            strcmp(act, u8"GRANT") ? u8"FROM" : u8"TO",
            user,
            host,
            extra);
    if (CQ_QLEN <= (size_t) rc) {
        free(query);
        return 100;
    }

    rc = cq_connect(&con);
    if (rc) {
        free(query);
        return 200;
    }

    rc = mysql_query(con.con, query);

    cq_close_connection(&con);
    free(query);
    return rc ? 201 : 0;
}
