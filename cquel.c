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

#include <my_global.h>
#include <string.h>
#include <unicode/ustdio.h>
#include <unicode/ustring.h>
#include <stdbool.h>
#include <ctype.h>

#include "cquel.h"

size_t CQ_QLEN = 0;
size_t CQ_FMAXLEN = 0;

void cq_init(size_t qlen, size_t fmaxlen)
{
    CQ_QLEN = qlen;
    CQ_FMAXLEN = fmaxlen;
}

static int cq_fields_to_utf8(char *buf, size_t buflen, size_t fieldc,
        char * const *fieldnames, bool usequotes)
{
    UChar *buf16;
    UErrorCode status = U_ZERO_ERROR;
    size_t num_left = fieldc;
    int rc = 0;

    if (num_left == 0)
        return 1;

    buf16 = calloc(buflen, sizeof(UChar));
    if (buf16 == NULL)
        return -1;

    for (size_t i = 0; i < fieldc; ++i) {
        bool escaped = fieldnames[i][0] == '\\';
        const char *field = escaped ? &fieldnames[i][1] : fieldnames[i];

        UChar *temp = calloc(buflen, sizeof(UChar));
        if (temp == NULL) {
            rc = -2;
            break;
        }

        u_strFromUTF8(temp, buflen, NULL, field, strlen(field), &status);
        if (!U_SUCCESS(status)) {
            rc = 2;
            free(temp);
            break;
        }

        bool isstr = false;
        if (!escaped && usequotes) {
            for (int32_t j = 0; j < u_strlen(temp); ++j) {
                if (!isdigit(temp[j])) {
                    isstr = true;
                    break;
                }
            }
        }

        if (isstr) u_strcat(buf16, u"'");
        u_strcat(buf16, temp);
        if (isstr) u_strcat(buf16, u"'");
        free(temp);
        if (--num_left > 0) {
            u_strcat(buf16, u",");
        }
    }

    u_strToUTF8(buf, buflen, NULL, buf16, u_strlen(buf16), &status);
    if (!U_SUCCESS(status))
        rc = 3;

    free(buf16);
    return rc;
}

static int cq_dlist_to_update_utf8(char *buf, size_t buflen, struct dlist list,
        struct drow row)
{
    UChar *buf16;
    UErrorCode status = U_ZERO_ERROR;
    size_t num_left = list.fieldc;
    int rc = 0;

    if (num_left == 0)
        return 1;

    buf16 = calloc(buflen, sizeof(UChar));
    if (buf16 == NULL)
        return -2;

    for (size_t i = 0; i < list.fieldc; ++i) {
        if (!strcmp(list.fieldnames[i], list.primkey)) {
            --num_left;
            continue;
        }

        UChar *ftemp = calloc(buflen, sizeof(UChar));
        if (ftemp == NULL) {
            rc = -3;
            break;
        }

        UChar *vtemp = calloc(buflen, sizeof(UChar));
        if (vtemp == NULL) {
            rc = -4;
            free(ftemp);
            break;
        }

        u_strFromUTF8(ftemp, buflen, NULL, list.fieldnames[i],
                strlen(list.fieldnames[i]), &status);
        if (!U_SUCCESS(status)) {
            rc = 2;
            free(ftemp);
            free(vtemp);
            break;
        }

        u_strFromUTF8(vtemp, buflen, NULL, row.values[i], strlen(row.values[i]),
                &status);
        if (!U_SUCCESS(status)) {
            rc = 3;
            free(ftemp);
            free(vtemp);
            break;
        }

        bool isstr = false;
        for (int32_t j = 0; j < u_strlen(vtemp); ++j)
            if (!isdigit(vtemp[j]))
                isstr = true;

        u_strcat(buf16, ftemp);
        u_strcat(buf16, u"=");
        if (isstr) u_strcat(buf16, u"'");
        u_strcat(buf16, vtemp);
        if (isstr) u_strcat(buf16, u"'");

        free(ftemp);
        free(vtemp);

        if (--num_left > 0)
            u_strcat(buf16, u",");
    }

    u_strToUTF8(buf, buflen, NULL, buf16, u_strlen(buf16), &status);
    if (!U_SUCCESS(status))
        rc = 4;

    free(buf16);
    return rc;
}

static int cq_dlist_fields_to_utf8(char *buf, size_t buflen, struct dlist list)
{
    return cq_fields_to_utf8(buf, buflen, list.fieldc, list.fieldnames,
            false);
}

static int cq_drow_to_utf8(char *buf, size_t buflen, struct drow row)
{
    return cq_fields_to_utf8(buf, buflen, row.fieldc, row.values,
            true);
}

struct dbconn cq_new_connection(const char *host, const char *user,
        const char *passwd, const char *database)
{
    struct dbconn out = {
        .host = host,
        .user = user,
        .passwd = passwd,
        .database = database
    };
    return out;
}

int cq_connect(struct dbconn *con)
{
    con->con = mysql_init(NULL);

    if (mysql_real_connect(con->con, con->host, con->user, con->passwd,
            con->database, 0, NULL, CLIENT_MULTI_STATEMENTS) == NULL) {
        return 1;
    }

    return 0;
}

void cq_close_connection(struct dbconn *con)
{
    mysql_close(con->con);
}

int cq_test(struct dbconn con)
{
    int rc;

    rc = cq_connect(&con);
    cq_close_connection(&con);
    return rc;
}

struct drow *cq_new_drow(size_t fieldc)
{
    struct drow *row = malloc(sizeof(struct drow));
    if (row == NULL)
        return NULL;

    row->fieldc = fieldc;

    if ((row->values = calloc(fieldc, sizeof(char *))) == NULL) {
        free(row);
        return NULL;
    }

    int rc = 0;
    size_t i;
    for (i = 0; i < fieldc; ++i) {
        if ((row->values[i] = calloc(CQ_FMAXLEN, sizeof(char))) == NULL) {
            rc = -1;
        }
    }

    if (rc) {
        for (size_t j = 0; j < i; ++j) {
            free(row->values[j]);
        }
        free(row->values);
        free(row);
        return NULL;
    }

    row->prev = NULL;
    row->next = NULL;
    return row;
}

void cq_free_drow(struct drow *row)
{
    if (row == NULL)
        return;
    for (size_t i = 0; i < row->fieldc; ++i)
        free(row->values[i]);
    free(row->values);
    free(row);
}

int cq_drow_set(struct drow *row, char * const *values)
{
    if (row == NULL)
        return 1;
    if (values == NULL)
        return 2;

    int rc = 0;
    size_t i;
    for (i = 0; i < row->fieldc; ++i) {
        if (strlen(values[i]) >= CQ_FMAXLEN) {
            rc = -1;
            break;
        }
        strcpy(row->values[i], values[i]);
    }

    if (rc) {
        for (size_t j = 0; j < i; ++j)
            free(row->values[j]);
        return -1;
    }

    return 0;
}

struct dlist *cq_new_dlist(size_t fieldc, char * const *fieldnames,
        const char *primkey)
{
    if (fieldnames == NULL)
        return NULL;
    if (strlen(primkey) >= CQ_FMAXLEN)
        return NULL;

    struct dlist *list = malloc(sizeof(struct dlist));
    if (list == NULL)
        return NULL;
    list->fieldc = fieldc;
    list->fieldnames = calloc(fieldc, sizeof(char *));

    if (list->fieldnames == NULL) {
        free(list);
        return NULL;
    }

    size_t i;
    int rc = 0;
    for (i = 0; i < fieldc; ++i) {
        if (fieldnames[i] == NULL) {
            rc = 1;
            break;
        }

        if (strlen(fieldnames[i]) >= CQ_FMAXLEN) {
            rc = -1;
            break;
        }
        list->fieldnames[i] = calloc(CQ_FMAXLEN, sizeof(char));
        if (list->fieldnames[i] == NULL) {
            rc = -1;
            break;
        }
        strcpy(list->fieldnames[i], fieldnames[i]);
    }
    if (rc) {
        for (size_t j = 0; j < i; ++j)
            free(list->fieldnames[j]);
        free(list->fieldnames);
        free(list);
        return NULL;
    }

    list->primkey = calloc(CQ_FMAXLEN, sizeof(char));
    if (list->primkey == NULL) {
        for (size_t j = 0; j < i; ++j)
            free(list->fieldnames[j]);
        free(list->fieldnames);
        free(list);
        return NULL;
    }
    strcpy(list->primkey, primkey);

    list->first = NULL;
    list->last = NULL;
    return list;
}

size_t cq_dlist_size(const struct dlist *list)
{
    if (list == NULL)
        return 0;
    size_t count = 0;
    struct drow *row = list->first;

    while (row != NULL) {
        ++count;
        row = row->next;
    }

    return count;
}

void cq_free_dlist(struct dlist *list)
{
    if (list == NULL)
        return;
    for (size_t i = 0; i < list->fieldc; ++i)
        free(list->fieldnames[i]);
    free(list->fieldnames);
    struct drow *row = list->first;
    struct drow *next;
    do {
        next = row->next;
        cq_free_drow(row);
    } while ((row = next) != NULL);
    free(list);
}

void cq_dlist_add(struct dlist *list, struct drow *row)
{
    if (list->last == NULL) {
        list->first = row;
        list->last = row;
        row->prev = NULL;
        row->next = NULL;
    } else {
        list->last->next = row;
        row->prev = list->last;
        list->last = row;
    }
}

static int dlist_meta_cmp(const struct dlist *a, const struct dlist *b)
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

struct dlist *cq_dlist_append(struct dlist **dest, const struct dlist *src)
{
	if (!dest || !*dest || !src || dlist_meta_cmp(*dest, src) )
		return NULL;

	struct drow *copy = NULL;
	struct drow *fallback = src->last;
	unsigned char error = 0;

	for (struct drow *iter = src->first; iter; iter=iter->next) {
		if (NULL == (copy = cq_new_drow(src->fieldc)) ) {
			error = 1;
			break;
		}

		if (cq_drow_set(copy, iter->values)) {
			error = 1;
			break;
		}

		cq_dlist_add(*dest, copy);
	}

	if (error) {
		if (copy)
			cq_free_drow(copy);

		for (struct drow *iter=fallback; iter; iter=iter->next) {
			cq_free_drow(iter);
		}

		fallback->next = NULL;
	}

	return *dest;
}

void cq_dlist_remove(struct dlist *list, struct drow *row)
{
    if (list == NULL || row == NULL)
        return;

    if (row == list->first && row == list->last) {
        list->first = NULL;
        list->last = NULL;
    } else if (row == list->first) {
        list->first = row->next;
        list->first->prev = NULL;
    } else if (row == list->last) {
        list->last = row->prev;
        list->last->next = NULL;
    } else {
        struct drow *after = row->next;
        struct drow *before = row->prev;
        after->prev = before;
        before->next = after;
    }

    cq_free_drow(row);
}

int cq_dlist_remove_field_str(struct dlist *list, const char *field)
{
    size_t i;
    bool found = false;
    char *s;
    for (i = 0; i < list->fieldc; ++i) {
        s = list->fieldnames[i];
        if (!strcmp(s, field)) {
            found = true;
            break;
        }
    }

    if (found) {
        cq_dlist_remove_field_at(list, i);
        return 0;
    }

    return 1;
}

int cq_dlist_remove_field_at(struct dlist *list, size_t index)
{
    if (list == NULL)
        return 1;

    if (index >= list->fieldc)
        return 2;

    if (!strcmp(list->fieldnames[index], list->primkey)) {
        free(list->primkey);
        list->primkey = NULL;
    }

    for (struct drow *row = list->first; row != NULL; row = row->next) {
        for (size_t i = index; i < row->fieldc; ++i) {
            if (i == (row->fieldc - 1)) {
                --row->fieldc;
                free(row->values[i]);
            } else {
                strcpy(row->values[i], row->values[i+1]);
            }
        }
    }

    for (size_t i = index; i < list->fieldc; ++i) {
        if (i == (list->fieldc - 1)) {
            --list->fieldc;
            free(list->fieldnames[i]);
        } else {
            strcpy(list->fieldnames[i], list->fieldnames[i+1]);
        }
    }

    return 0;
}

struct drow *cq_dlist_at(struct dlist *list, size_t index)
{
    if (list == NULL)
        return NULL;
    struct drow *row = list->first;
    while (row != NULL) {
        if (index-- == 0)
            return row;
        row = row->next;
    }
    return NULL;
}

int cq_field_to_index(struct dlist *list, const char *field, size_t *out)
{
    bool found = false;

    if (list == NULL)
        return -1;
    if (field == NULL)
        return -2;
    if (out == NULL)
        return -3;

    for (*out = 0; *out < list->fieldc; ++(*out)) {
        if (!strcmp(list->fieldnames[*out], field)) {
            found = true;
            break;
        }
    }

    return !found;
}

int cq_insert(struct dbconn con, const char *table, struct dlist *list)
{
    int rc;
    char *query, *columns, *values;
    const char *fmt = u8"INSERT INTO %s(%s) VALUES(%s)";

    if (table == NULL)
        return 1;
    if (list == NULL)
        return 2;

    query = calloc(CQ_QLEN, sizeof(char));
    if (query == NULL)
        return -1;

    columns = calloc(CQ_QLEN/2, sizeof(char));
    if (columns == NULL) {
        free(query);
        return -2;
    }

    values = calloc(CQ_QLEN/2, sizeof(char));
    if (values == NULL) {
        free(query);
        free(columns);
        return -3;
    }

    rc = cq_dlist_fields_to_utf8(columns, CQ_QLEN/2, *list);
    if (rc) {
        free(query);
        free(columns);
        free(values);
        return 100;
    }

    rc = cq_connect(&con);
    if (rc) {
        free(query);
        free(columns);
        free(values);
        return 200;
    }

    for (struct drow *r = list->first; r != NULL; r = r->next) {
        rc = cq_drow_to_utf8(values, CQ_QLEN/2, *r);
        if (rc)
            break;

        UChar *buf16 = calloc(CQ_QLEN, sizeof(UChar));
        if (buf16 == NULL) {
            rc = -4;
            break;
        }
        rc = u_snprintf(buf16, CQ_QLEN, fmt, table, columns, values);
        if ((size_t) rc >= CQ_QLEN) {
            free(buf16);
            rc = -5;
            break;
        }
        rc = 0;

        UErrorCode status = U_ZERO_ERROR;
        u_strToUTF8(query, CQ_QLEN, NULL, buf16, u_strlen(buf16), &status);
        free(buf16);
        if (!U_SUCCESS(status)) {
            rc = -1;
            break;
        }

        rc = mysql_query(con.con, query);
        if (rc) {
            rc = 201;
            break;
        }
    }

    cq_close_connection(&con);
    free(query);
    free(columns);
    free(values);
    return rc;
}

int cq_update(struct dbconn con, const char *table, struct dlist *list)
{
    int rc;
    char *query, *columns;
    const char *fmt = u8"UPDATE %s SET %s WHERE %s=%s";

    if (table == NULL)
        return 1;
    if (list == NULL)
        return 2;

    query = calloc(CQ_QLEN, sizeof(char));
    if (query == NULL)
        return -1;

    columns = calloc(CQ_QLEN/2, sizeof(char));
    if (columns == NULL) {
        free(query);
        return -2;
    }

    size_t pindex;
    bool found = false;
    for (pindex = 0; pindex < list->fieldc; ++pindex) {
        if (!strcmp(list->fieldnames[pindex], list->primkey)) {
            found = true;
            break;
        }
    }

    if (!found) {
        free(query);
        free(columns);
        return 3;
    }

    rc = cq_connect(&con);
    if (rc) {
        free(query);
        free(columns);
        return 200;
    }

    for (struct drow *r = list->first; r != NULL; r = r->next) {
        rc = cq_dlist_to_update_utf8(columns, CQ_QLEN/2, *list, *r);
        if (rc) {
            rc = 101;
            break;
        }

        rc = snprintf(query, CQ_QLEN, fmt, table, columns, list->primkey,
                r->values[pindex]);
        if ((size_t) rc >= CQ_QLEN) {
            rc = 102;
            break;
        }

        rc = mysql_query(con.con, query);
        if (rc) {
            rc = 201;
            break;
        }
    }

    cq_close_connection(&con);
    free(query);
    free(columns);
    return rc;
}

int cq_select_query(struct dbconn con, struct dlist **out, const char *q)
{
    int rc;
    char *query;

    if (q == NULL)
        return 1;
    if (strlen(q) >= CQ_QLEN)
        return 2;

    query = calloc(CQ_QLEN, sizeof(char));
    if (query == NULL)
        return -1;

    UChar *tempquery, *tempq;
    UErrorCode status = U_ZERO_ERROR;

    tempq = calloc(CQ_QLEN, sizeof(UChar));
    if (tempq == NULL) {
        free(query);
        return -2;
    }

    u_strFromUTF8(tempq, CQ_QLEN, NULL, q, strlen(q), &status);
    if (!U_SUCCESS(status)) {
        free(query);
        free(tempq);
        return 101;
    }

    tempquery = calloc(CQ_QLEN, sizeof(UChar));
    if (tempquery == NULL) {
        free(query);
        free(tempq);
        return 102;
    }

    rc = u_snprintf_u(tempquery, CQ_QLEN, u"SELECT %S", tempq);
    if ((size_t) rc >= CQ_QLEN) {
        free(query);
        free(tempq);
        free(tempquery);
        return 103;
    }

    u_strToUTF8(query, CQ_QLEN, NULL, tempquery, u_strlen(tempquery), &status);

    free(tempq);
    free(tempquery);

    if (!U_SUCCESS(status)) {
        free(query);
        return 104;
    }

    rc = cq_connect(&con);
    if (rc) {
        free(query);
        return 200;
    }

    rc = mysql_query(con.con, query);
    if (rc) {
        free(query);
        cq_close_connection(&con);
        return 201;
    }

    MYSQL_RES *result = mysql_store_result(con.con);
    cq_close_connection(&con);
    if (result == NULL) {
        free(query);
        return 202;
    }

    char *from = strcasestr(query, u8"FROM") + strlen(u8"FROM");
    /*while (isblank(*(++from)))
        ;*/
    do {
        ++from;
    } while (isblank(*from) || *from == '\n');
    size_t len = 0;
    while (*(from + len) && !isblank(*(from + len)) && *(from + len) != '\n')
        ++len;

    char *table = calloc(len+1, sizeof(char));
    strncpy(table, from, len);
    table[len] = '\0';
    free(query);

    size_t num_fields = mysql_num_fields(result);
    if (!num_fields) {
        free(table);
        mysql_free_result(result);
        *out = NULL;
        return 0;
    }

    char **fieldnames = calloc(num_fields, sizeof(char *));
    if (fieldnames == NULL) {
        free(table);
        mysql_free_result(result);
        return -3;
    }

    MYSQL_FIELD *field;
    size_t i;
    for (i = 0; i < num_fields; ++i) {
        if ((field = mysql_fetch_field(result)) == NULL)
            break;

        size_t flen = strlen(field->name);
        fieldnames[i] = calloc(flen, sizeof(char));
        if (fieldnames[i] == NULL) {
            rc = -4;
            break;
        }
        strcpy(fieldnames[i], field->name);
    }

    if (rc) {
        for (size_t j = 0; j < i; ++j) {
            free(fieldnames[j]);
        }
        free(fieldnames);
        free(table);
        mysql_free_result(result);
        return rc;
    }

    char *primkey = calloc(CQ_QLEN, sizeof(char));
    if (primkey == NULL) {
        for (size_t j = 0; j < i; ++j) {
            free(fieldnames[j]);
        }
        free(fieldnames);
        free(table);
        mysql_free_result(result);
        return -5;
    }

    rc = cq_get_primkey(con, table, primkey, CQ_QLEN);
    free(table);
    if (rc) {
        for (size_t j = 0; j < i; ++j) {
            free(fieldnames[j]);
        }
        free(fieldnames);
        mysql_free_result(result);
        return 205;
    }

    *out = cq_new_dlist(num_fields, fieldnames, primkey);
    for (size_t j = 0; j <= i; ++j) {
        free(fieldnames[j]);
    }
    free(fieldnames);
    free(primkey);

    if (*out == NULL) {
        mysql_free_result(result);
        return -6;
    }

    MYSQL_ROW row;
    char **rvals = calloc(num_fields, sizeof(char *));
    if (rvals == NULL) {
        cq_free_dlist(*out);
        mysql_free_result(result);
    }
    size_t in = 0;
    while ((row = mysql_fetch_row(result))) {
        struct drow *data = cq_new_drow(num_fields);
        if (data == NULL) {
            rc = -7;
            break;
        }

        for (in = 0; in < num_fields; ++in) {
            size_t len = strlen(row[in]);
            rvals[in] = calloc(len+1, sizeof(char));
            if (rvals[in] == NULL) {
                rc = -8;
                break;
            }
            strcpy(rvals[in], row[in]);
        }
        if (rc) {
            break;
        }

        rc = cq_drow_set(data, rvals);
        if (rc) {
            break;
        }

        cq_dlist_add(*out, data);
    }

    for (size_t j = 0; j < in; ++j) {
        free(rvals[j]);
    }
    free(rvals);
    mysql_free_result(result);

    if (rc) {
        cq_free_dlist(*out);
    }

    return 0;
}

int cq_select_all(struct dbconn con, const char *table, struct dlist **out,
        const char *conditions)
{
    int rc;
    char *query;
    const char *fmt = strcmp(conditions, u8"") ?
            u8"* FROM %s WHERE %s" : u8"* FROM %s%s";

    query = calloc(CQ_QLEN, sizeof(char));
    if (query == NULL)
        return -10;

/*
    UChar *buf16 = calloc(CQ_QLEN, sizeof(UChar));
    if (buf16 == NULL) {
        free(query);
        return -11;
    }

    rc = u_snprintf(buf16, CQ_QLEN, fmt, table, conditions);
    if ((size_t) rc >= CQ_QLEN) {
        free(query);
        free(buf16);
        return 100;
    }

    UErrorCode status = U_ZERO_ERROR;
    u_strToUTF8(query, CQ_QLEN, NULL, buf16, u_strlen(buf16), &status);
    free(buf16);
    if (!U_SUCCESS(status)) {
        free(query);
        return 101;
    }
*/

    rc = snprintf(query, CQ_QLEN, fmt, table, conditions);
    if (CQ_QLEN <= (size_t)rc) {
        free(query);
        return 100;
    }

    rc = cq_select_query(con, out, query);
    return rc;
}

int cq_get_primkey(struct dbconn con, const char *table, char *out,
        size_t len)
{
    int rc;
    char *query;
    const char *fmt = u8"SHOW KEYS FROM %s WHERE Key_name = 'PRIMARY'";

    query = calloc(CQ_QLEN, sizeof(char));
    if (query == NULL)
        return -20;

    UChar *buf16 = calloc(CQ_QLEN, sizeof(UChar));
    if (buf16 == NULL) {
        free(query);
        return -21;
    }

    rc = u_snprintf(buf16, CQ_QLEN, fmt, table);
    if ((size_t) rc >= CQ_QLEN) {
        free(buf16);
        free(query);
        return 100;
    }

    UErrorCode status = U_ZERO_ERROR;
    u_strToUTF8(query, CQ_QLEN, NULL, buf16, u_strlen(buf16), &status);
    free(buf16);
    if (!U_SUCCESS(status)) {
        free(query);
        return 101;
    }

    rc = cq_connect(&con);
    if (rc) {
        free(query);
        return 200;
    }

    rc = mysql_query(con.con, query);
    free(query);
    if (rc) {
        cq_close_connection(&con);
        return 201;
    }

    MYSQL_RES *result = mysql_store_result(con.con);
    cq_close_connection(&con);
    if (result == NULL)
        return 202;

    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row) {
        mysql_free_result(result);
        return 203;
    }

    /* 5th column is documented to be the column name */
    rc = 0;
    if (strlen(row[4]) >= len)
        rc = 204;
    else
        strcpy(out, row[4]);
    mysql_free_result(result);

    return rc;
}

int cq_get_fields(struct dbconn con, const char *table, size_t *out_fieldc,
        char **out_names, size_t nblen)
{
    int rc;
    char *query;
    const char *fmt = u8"SHOW COLUMNS IN %s";

    if (table == NULL)
        return 1;
    
    bool getting_names = !(out_names == NULL);
    bool getting_count = !(out_fieldc == NULL);

    UChar *tempq = calloc(CQ_QLEN, sizeof(UChar));
    if (tempq == NULL)
        return -1;
    rc = u_snprintf(tempq, CQ_QLEN, fmt, table);
    if ((size_t) rc >= CQ_QLEN) {
        free(tempq);
        return 100;
    }

    query = calloc(CQ_QLEN, sizeof(char));
    if (query == NULL) {
        free(tempq);
        return -2;
    }

    UErrorCode status = U_ZERO_ERROR;
    u_strToUTF8(query, CQ_QLEN, NULL, tempq, u_strlen(tempq), &status);
    free(tempq);
    if (!U_SUCCESS(status)) {
        free(query);
        return 101;
    }

    rc = cq_connect(&con);
    if (rc) {
        free(query);
        return 200;
    }

    rc = mysql_query(con.con, query);
    free(query);
    if (rc)
        return 201;

    MYSQL_RES *result = mysql_store_result(con.con);
    cq_close_connection(&con);
    if (result == NULL)
        return 202;

    MYSQL_ROW row;
    size_t num_rows = 0;
    rc = 0;
    while ((row = mysql_fetch_row(result))) {
        if (getting_names) {
            if (strlen(row[0]) >= nblen) {
                rc = 203;
                break;
            }
            strcpy(out_names[num_rows], row[0]);
        }
        ++num_rows;
    }
    mysql_free_result(result);

    if (getting_count)
        *out_fieldc = num_rows;

    return 0;
}

int cq_proc_arr(struct dbconn con, const char *proc, char * const *args,
        size_t num_args)
{
    int rc = 0;
    char *query, *fargs;
    const char *fmt = u8"CALL %s(%s)";

    if (NULL == proc || NULL == args)
        return 1;

    query = calloc(CQ_QLEN, sizeof(char));
    if (NULL == query)
        return -1;

    fargs = calloc(CQ_QLEN, sizeof(char));
    if (NULL == fargs) {
        free(query);
        return -2;
    }

    if (0 != num_args) {
        rc = cq_fields_to_utf8(fargs, CQ_QLEN, num_args, args, true);
        if (rc) {
            free(query);
            free(fargs);
            return 100;
        }
    }

    rc = snprintf(query, CQ_QLEN, fmt, proc, fargs);
    free(fargs);
    if (CQ_QLEN <= (size_t)rc) {
        free(query);
        return 101;
    }

    rc = cq_connect(&con);
    if (rc) {
        free(query);
        return 200;
    }

    rc = mysql_query(con.con, query);
    free(query);

    cq_close_connection(&con);
    return rc;
}

int cq_proc_drow(struct dbconn con, const char *proc, struct drow row)
{
    return cq_proc_arr(con, proc, row.values, row.fieldc);
}
