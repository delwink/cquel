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

#ifndef DELWINK_CQUEL_H
#define DELWINK_CQUEL_H

#include <stdlib.h>
#include <mysql.h>

#ifdef __cplusplus
extern "C"
{
#endif

struct drow;

struct dbconn {
    MYSQL *con;
    const char *host;
    const char *user;
    const char *passwd;
    const char *database;
};

struct drow {
    size_t fieldc;
    char **values;

    struct drow *prev;
    struct drow *next;
};

struct drow *cq_new_drow(size_t fieldc);
void cq_free_drow(struct drow *row);
void cq_drow_set(struct drow *row, char **values);

struct dlist {
    size_t fieldc;
    const char **fieldnames;

    struct drow *first;
    struct drow *last;
};

struct dbconn cq_new_connection(const char *host, const char *user,
        const char *passwd, const char *database);

int cq_connect(struct dbconn *con);
void cq_close_connection(struct dbconn *con);
int cq_test(struct dbconn con);

struct dlist *cq_new_dlist(size_t fieldc, const char **fieldnames);
size_t cq_dlist_size(const struct dlist *list);
void cq_free_dlist(struct dlist *list);
void cq_dlist_add(struct dlist *list, struct drow *row);
void cq_dlist_remove(struct dlist *list, struct drow *row);
struct drow *cq_dlist_at(struct dlist *list, size_t index);

int cq_insert(struct dbconn con, const char *table, struct dlist *list);
int cq_update(struct dbconn con, const char *table, struct dlist *list
        const char *conditions);

int cq_select(struct dbconn con, const char *table, struct dlist *out,
        const char *conditions);

#ifdef __cplusplus
}
#endif

#endif
