/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
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

#include "cquel.h"

int cq_fields_to_utf8(char *buf, size_t buflen, size_t fieldc,
        const char **fieldnames)
{
    UChar *buf16 = calloc(buflen, sizeof(UChar));
    UErrorCode status = U_ZERO_ERROR;
    size_t num_left = fieldc;
    int rc = 0;
    if (buf16 == NULL)
        return -1;

    if (num_left == 0)
        return 1;

    for (size_t i = 0; i < fieldc; ++i) {
        UChar *temp = calloc(buflen, sizeof(UChar));
        if (temp == NULL) {
            rc = -1;
            break;
        }

        u_strFromUTF8(temp, buflen, NULL, fieldnames[i], strlen(fieldnames[i]),
                &status);
        if (!U_SUCCESS(status)) {
            rc = 2;
            free(temp);
            break;
        }

        u_strcat(buf16, temp);
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

int cq_dlist_fields_to_utf8(char *buf, size_t buflen, struct dlist list)
{
    return cq_fields_to_utf8(buf, buflen, list.fieldc, list.fieldnames);
}

int cq_drow_to_utf8(char *buf, size_t buflen, struct drow row)
{
    return cq_fields_to_utf8(buf, buflen, row.fieldc,
            (const char **) row.values);
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
    row->values = calloc(fieldc, sizeof(char *));

    if (row->values == NULL) {
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
    free(row->values);
    free(row);
}

void cq_drow_set(struct drow *row, char **values)
{
    for (size_t i = 0; i < row->fieldc; ++i) {
        row->values[i] = values[i];
    }
}

struct dlist *cq_new_dlist(size_t fieldc, const char **fieldnames)
{
    struct dlist *list = malloc(sizeof(struct dlist));
    if (list == NULL)
        return NULL;
    list->fieldc = fieldc;
    list->fieldnames = calloc(fieldc, sizeof(char *));

    if (list->fieldnames == NULL) {
        free(list);
        return NULL;
    }

    for (size_t i = 0; i < fieldc; ++i)
        list->fieldnames[i] = fieldnames[i];

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

int cq_insert(struct dbconn con, const char *table, struct dlist *list)
{
    int rc;
    char *query, *columns, *values;
    const size_t qlen = 1024;
    const char *fmt = "INSERT INTO %s(%s) VALUES(%s)";

    query = calloc(qlen, sizeof(char));
    if (query == NULL)
        return -1;

    columns = calloc(qlen/2, sizeof(char));
    if (columns == NULL) {
        free(query);
        return -1;
    }

    values = calloc(qlen/2, sizeof(char));
    if (values == NULL) {
        free(query);
        free(columns);
        return -1;
    }

    rc = cq_dlist_fields_to_utf8(columns, qlen/2, *list);
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
        rc = cq_drow_to_utf8(values, qlen/2, *r);
        if (rc)
            break;

        UChar *buf16 = calloc(qlen, sizeof(UChar));
        if (buf16 == NULL) {
            rc = -1;
            break;
        }
        rc = u_snprintf(buf16, qlen, fmt, table, columns, values);
        if ((size_t) rc >= qlen) {
            free(buf16);
            break;
        }
        rc = 0;

        UErrorCode status = U_ZERO_ERROR;
        u_strToUTF8(query, qlen, NULL, buf16, u_strlen(buf16), &status);
        free(buf16);
        if (!U_SUCCESS(status))
            break;

        rc = mysql_query(con.con, query);
        if (rc)
            break;
    }

    cq_close_connection(&con);
    free(query);
    free(columns);
    free(values);
    return rc;
}
