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
#include <mysql.h>
#include <string.h>
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
    int rc = 0;
    size_t num_left = fieldc, written = 0;

    if (num_left == 0)
        return 1;

    char *temp = calloc(CQ_FMAXLEN+3, sizeof(char));
    if (NULL == temp)
        return -1;

    /* prevent appending to buffer */
    buf[0] = '\0';

    for (size_t i = 0; i < fieldc; ++i) {
        bool escaped = fieldnames[i][0] == '\\';
        const char *field = escaped ? &fieldnames[i][1] : fieldnames[i];

        bool isstr = false;
        if (!escaped && usequotes) {
            for (size_t j = 0; j < strlen(field); ++j) {
                if (!isdigit(field[j])) {
                    isstr = true;
                    break;
                }
            }
        }

        const char *a = isstr ? "'" : "";
        const char *c = --num_left > 0 ? "," : "";
        written += snprintf(temp, CQ_FMAXLEN+3, "%s%s%s%s", a, field, a, c);
        if (written >= buflen) {
            rc = 2;
            break;
        }
            
        strcat(buf, temp);
    }

    free(temp);
    return rc;
}

static int cq_dlist_to_update_utf8(char *buf, size_t buflen, struct dlist list,
        struct drow row)
{
    int rc = 0;
    size_t num_left = list.fieldc, written = 0;

    if (num_left == 0)
        return 1;

    char *temp = calloc(CQ_FMAXLEN+3, sizeof(char));
    if (NULL == temp)
        return -1;

    /* prevent appending to buffer */
    buf[0] = '\0';

    for (size_t i = 0; i < list.fieldc; ++i) {
        if (!strcmp(list.fieldnames[i], list.primkey)) {
            --num_left;
            continue;
        }

        bool v_escaped = row.values[i][0] == '\\';
        const char *tempv = v_escaped ?
                &row.values[i][1] : row.values[i];

        bool isstr = false;
        if (!v_escaped) {
            for (size_t j = 0; j < strlen(tempv); ++j) {
                if (!isdigit(tempv[j])) {
                    isstr = true;
                    break;
                }
            }
        }

        const char *a = isstr ? "'" : "";
        const char *c = --num_left > 0 ? "," : "";
        written += snprintf(temp, CQ_FMAXLEN+3, "%s=%s%s%s%s",
                list.fieldnames[i],
                a, tempv, a, c);
        if (written >= buflen) {
            rc = 2;
            break;
        }

        strcat(buf, temp);
    }

    free(temp);
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
	bool error = false;

	for (struct drow *iter = src->first; iter; iter=iter->next) {
		if (NULL == (copy = cq_new_drow(src->fieldc)) ) {
			error = true;
			break;
		}

		if (cq_drow_set(copy, iter->values)) {
			error = true;
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

struct drow *cq_dlist_at(const struct dlist *list, size_t index)
{
    if (list == NULL)
        return NULL;
    for (struct drow *row = list->first; row != NULL; row = row->next)
        if (index-- == 0)
            return row;
    return NULL;
}

int cq_field_to_index(const struct dlist *list, const char *field, size_t *out)
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

int cq_insert(struct dbconn con, const char *table, const struct dlist *list)
{
    int rc;
    char *query, *columns, *values;
    const char *fmt = "INSERT INTO %s(%s) VALUES(%s)";

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

        rc = snprintf(query, CQ_QLEN, fmt, table, columns, values);
        if (CQ_QLEN <= (size_t) rc) {
            rc = -4;
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

int cq_update(struct dbconn con, const char *table, const struct dlist *list)
{
    int rc;
    char *query, *columns;
    const char *fmt = "UPDATE %s SET %s WHERE %s=%s";

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

    rc = snprintf(query, CQ_QLEN, "SELECT %s", q);
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
            "* FROM %s WHERE %s" : u8"* FROM %s%s";

    query = calloc(CQ_QLEN, sizeof(char));
    if (query == NULL)
        return -10;

    rc = snprintf(query, CQ_QLEN, fmt, table, conditions);
    if (CQ_QLEN <= (size_t)rc) {
        free(query);
        return 100;
    }

    rc = cq_select_query(con, out, query);
    free(query);
    return rc;
}

int cq_select_func_arr(struct dbconn con, const char *func, char * const *args,
        size_t num_args, struct dlist **out)
{
    int rc;
    char *query, *fargs;
    const char *fmt = "%s(%s)";

    if (NULL == func || NULL == args)
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
            return 110;
        }
    }

    rc = snprintf(query, CQ_QLEN, fmt, func, fargs);
    free(fargs);
    if (CQ_QLEN <= (size_t) rc) {
        free(query);
        return 111;
    }

    rc = cq_select_query(con, out, query);
    free(query);
    return rc;
}

int cq_select_func_drow(struct dbconn con, const char *func, struct drow row,
        struct dlist **out)
{
    return cq_select_func_arr(con, func, row.values, row.fieldc, out);
}

int cq_get_primkey(struct dbconn con, const char *table, char *out,
        size_t len)
{
    int rc;
    char *query;
    const char *fmt = "SHOW KEYS FROM %s WHERE Key_name = 'PRIMARY'";

    query = calloc(CQ_QLEN, sizeof(char));
    if (query == NULL)
        return -20;

    rc = snprintf(query, CQ_QLEN, fmt, table);
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
    const char *fmt = "SHOW COLUMNS IN %s";

    if (table == NULL)
        return 1;
    
    bool getting_names = !(out_names == NULL);
    bool getting_count = !(out_fieldc == NULL);

    query = calloc(CQ_QLEN, sizeof(char));
    if (query == NULL) {
        return -1;
    }

    rc = snprintf(query, CQ_QLEN, fmt, table);
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
    const char *fmt = "CALL %s(%s)";

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
    return rc ? 201 : 0;
}

int cq_proc_drow(struct dbconn con, const char *proc, struct drow row)
{
    return cq_proc_arr(con, proc, row.values, row.fieldc);
}

static int grant_revoke(struct dbconn con, const char *act, const char *perms,
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

int cq_grant(struct dbconn con, const char *perms, const char *table,
        const char *user, const char *host, const char *extra)
{
    return grant_revoke(con, u8"GRANT", perms, table, user, host, extra);
}

int cq_revoke(struct dbconn con, const char *perms, const char *table,
        const char *user, const char *host, const char *extra)
{
    return grant_revoke(con, u8"REVOKE", perms, table, user, host, extra);
}
