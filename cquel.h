/*
 *  cquel - MySQL C API wrapper with dynamic data structures
 *  Copyright (C) 2014 Delwink, LLC
 *
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

/**
 * @file cquel.h
 * @version 1.0
 * @date 10/26/2014
 * @author David McMackins II
 * @title Delwink cquel API
 * @brief MySQL C API wrapper with dynamic data structures
 */

#ifndef DELWINK_CQUEL_H
#define DELWINK_CQUEL_H

#define CQ_QLEN 1024

#include <mysql.h>

#ifdef __cplusplus
extern "C"
{
#endif

struct drow;

/**
 * @brief The universal database connection auxiliary structure for cquel.
 */
struct dbconn {
    MYSQL *con;
    const char *host;
    const char *user;
    const char *passwd;
    const char *database;
};

/**
 * @brief cq_new_connection Constructs a database connection.
 * @param host The hostname or IP address of the database server.
 * @param user The username with which to log into the database server.
 * @param passwd The password by which to be authenticated.
 * @param database The database to use; can be NULL.
 * @return A new database connection object.
 */
struct dbconn cq_new_connection(const char *host, const char *user,
        const char *passwd, const char *database);

int cq_connect(struct dbconn *con);
void cq_close_connection(struct dbconn *con);
int cq_test(struct dbconn con);

struct drow {
    size_t fieldc;
    char **values;

    struct drow *prev;
    struct drow *next;
};

struct drow *cq_new_drow(size_t fieldc);
void cq_free_drow(struct drow *row);
int cq_drow_set(struct drow *row, char **values);

struct dlist {
    size_t fieldc;
    char **fieldnames;
    const char *primkey;

    struct drow *first;
    struct drow *last;
};

struct dlist *cq_new_dlist(size_t fieldc, char **fieldnames,
        const char *primkey);
size_t cq_dlist_size(const struct dlist *list);
void cq_free_dlist(struct dlist *list);
void cq_dlist_add(struct dlist *list, struct drow *row);
void cq_dlist_remove(struct dlist *list, struct drow *row);
int cq_dlist_remove_field_str(struct dlist *list, char *field);
void cq_dlist_remove_field_at(struct dlist *list, size_t index);
struct drow *cq_dlist_at(struct dlist *list, size_t index);

int cq_field_to_index(struct dlist *list, const char *field, size_t *out);

int cq_insert(struct dbconn con, const char *table, struct dlist *list);
int cq_update(struct dbconn con, const char *table, struct dlist *list);

int cq_select(struct dbconn con, const char *table, struct dlist *out,
        const char *conditions);

#ifdef __cplusplus
}
#endif

#endif
