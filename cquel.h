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

/**
 * @file cquel.h
 * @version 4.5
 * @date 12/27/2014
 * @authors David McMackins II, Darcy Brás da Silva
 * @brief MySQL C API wrapper with dynamic data structures
 */

#ifndef DELWINK_CQUEL_H
#define DELWINK_CQUEL_H

#include <stdbool.h>

/**
 * @brief The cquel interface version number.
 */
#define CQ_INTERFACE "4"

/**
 * @brief The cquel software version number.
 */
#define CQ_VERSION   "4.5"

/**
 * @brief Information about the cquel copyright holders and license.
 */
#define CQ_COPYRIGHT \
"cquel - MySQL C API wrapper with dynamic data structures\n"\
"Copyright (C) 2014 Delwink, LLC\n\n"\
"This program is free software: you can redistribute it and/or modify\n"\
"it under the terms of the GNU Affero General Public License as published by\n"\
"the Free Software Foundation.\n\n"\
"This program is distributed in the hope that it will be useful,\n"\
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"\
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"\
"GNU Affero General Public License for more details.\n\n"\
"You should have received a copy of the GNU Affero General Public License\n"\
"along with this program.  If not, see <http://www.gnu.org/licenses/>."

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Initializes the cquel library.
 * @param qlen Maximum length of the query strings used by cquel.
 * @param fmaxlen Length of the buffer for each field and value.
 */
void cq_init(size_t qlen, size_t fmaxlen);

struct drow;

/**
 * @brief The universal database connection auxiliary structure for cquel.
 */
struct dbconn {
    void *con;
    bool isopen;
    const char *host;
    const char *user;
    const char *passwd;
    const char *database;
};

/**
 * @brief Constructs a database connection.
 * @param host The hostname or IP address of the database server.
 * @param user The username with which to log into the database server.
 * @param passwd The password by which to be authenticated.
 * @param database The database to use; can be NULL.
 * @return A new database connection object.
 */
struct dbconn cq_new_connection(const char *host, const char *user,
        const char *passwd, const char *database);

/**
 * @brief Connects to the database server.
 * @param con Initialized database connection object.
 * @return Nonzero if an error occurred.
 */
int cq_connect(struct dbconn *con);

/**
 * @brief Disconnects from the database server.
 * @param con Connected database connection object.
 */
void cq_close_connection(struct dbconn *con);

/**
 * @brief Attempts to connect to and immediately disconnect from the database
 * server.
 * @param con Database connection object with connection details.
 * @return Nonzero if an error occurred.
 */
int cq_test(struct dbconn con);

/**
 * @brief Generic database row object to be added to a list.
 */
struct drow {
    size_t fieldc;
    char **values;

    struct drow *prev;
    struct drow *next;
};

/**
 * @brief Instantiates a new database row.
 * @param fieldc The number of columns in the row; must match parent list.
 * @return A pointer to the allocated memory for this row.
 */
struct drow *cq_new_drow(size_t fieldc);

/**
 * @brief Frees all the memory allocated to an instantiated database row.
 * @param row The row to be freed.
 */
void cq_free_drow(struct drow *row);

/**
 * @brief Sets the values for each column in a row.
 * @param values An array of UTF-8 strings containing the data in the row; must
 * match the structure of fieldnames in the parent list; non-numeric values 
 * which do not begin with '\\' will be surrounded with apostrophes in queries 
 * to the database.
 * @return Nonzero if input error.
 */
int cq_drow_set(struct drow *row, char * const *values);

/**
 * @brief A double linked list of database rows with metadata.
 */
struct dlist {
    size_t fieldc;
    char **fieldnames;
    char *primkey;

    struct drow *first;
    struct drow *last;
};

/**
 * @brief Instantiates a new data list.
 * @param fieldc The number of columns in each row of the list.
 * @param fieldnames An array of UTF-8 strings matching the column names in the
 * database table.
 * @param primkey A UTF-8 string matching the column name of the table's primary
 * key; can be NULL if only inserting.
 * @return A pointer to the allocated memory for this list.
 */
struct dlist *cq_new_dlist(size_t fieldc, char * const *fieldnames,
        const char *primkey);

/**
 * @brief Counts the number of members in a data list.
 * @param list The list to be examined.
 * @return The number of rows in the list.
 */
size_t cq_dlist_size(const struct dlist *list);

/**
 * @brief Frees all memory allocated to an instatiated data list.
 * @param list The list to be freed.
 */
void cq_free_dlist(struct dlist *list);

/**
 * @brief Adds a row to a data list.
 * @param list The list to which to add the row.
 * @param row The row to be added.
 */
void cq_dlist_add(struct dlist *list, struct drow *row);

/**
 * @brief Copies 'src' dlist items appending them to 'dest'
 * @param dest The list to append data to
 * @param src The list to be appended
 * @return Updated 'dest' or NULL on failure
 */
struct dlist *cq_dlist_append(struct dlist **dest, const struct dlist *src);

/**
 * @brief Removes a row from a data list.
 * @param list The list from which to remove the row.
 * @param row The row to be removed.
 */
void cq_dlist_remove(struct dlist *list, struct drow *row);

/**
 * @brief Removes the first column found by its name from a data list.
 * @param list The data list from which to remove the field.
 * @param field A UTF-8 string naming the field to be removed.
 * @return Nonzero if field not found.
 */
int cq_dlist_remove_field_str(struct dlist *list, const char *field);

/**
 * @brief Removes a column from a data list by an index.
 * @param list The data list from which to remove the field.
 * @param index The index of the field to be removed.
 * @return Nonzero if invalid input.
 */
int cq_dlist_remove_field_at(struct dlist *list, size_t index);

/**
 * @brief Gets a row from a data list by index.
 * @param list The list through which to be searched.
 * @param index Number indicating which element to get.
 * @return Pointer to the row at that index or NULL on failure.
 */
struct drow *cq_dlist_at(const struct dlist *list, size_t index);

/**
 * @brief Gets the index of a data list field by name.
 * @param list The list to be examined.
 * @param field A UTF-8 string naming the field for which to search.
 * @param out The output variable into which to store the index.
 * @return Nonzero if input error or field not found.
 */
int cq_field_to_index(const struct dlist *list, const char *field, size_t *out);

/**
 * @brief Inserts data into the database based on a data list.
 * @param con Database connection object with connection details.
 * @param table The database table to which to insert the data.
 * @param list The data list from which to insert data.
 * @return 0 on success; less than 0 if memory error; from 1 to 10 if input
 * error; from 100 to 199 if query setup error; 200 if database connection
 * error; 201 if error submitting query.
 */
int cq_insert(struct dbconn con, const char *table, const struct dlist *list);

/**
 * @brief Updates data in a database table based on a data list.
 * @param con Database connection object with connection details.
 * @param table The database table to which to update the data.
 * @param list The data list from which to derive the updated data.
 * @return 0 on success; less than 0 if memory error; from 1 to 10 if input
 * error; from 100 to 199 if query setup error; 200 if database connection
 * error; 201 if error submitting query.
 */
int cq_update(struct dbconn con, const char *table, const struct dlist *list);

/**
 * @brief Pulls data from the database based on a SELECT query.
 * @param con Database connection object with connection details.
 * @param out An unallocated data list into which the data will be inserted.
 * @param q UTF-8 SQL to be appended to "SELECT ".
 * @return 0 on success; less than 0 if memory error; from 1 to 10 if input
 * error; from 100 to 199 if query setup error; 200 if database connection
 * error; 201 if error submitting query; 202-299 if error parsing data.
 */
int cq_select_query(struct dbconn con, struct dlist **out, const char *query);

/**
 * @brief Pulls a table from the database.
 * @param con Database connection object with connection details.
 * @param table UTF-8 string matching the name of the table to be pulled.
 * @param out An unallocated data list into which the data will be inserted.
 * @param conditions UTF-8 SQL where_condition.
 * @return 0 on success; less than 0 if memory error; from 1 to 10 if input
 * error; from 100 to 199 if query setup error; 200 if database connection
 * error; 201 if error submitting query; 202-299 if error parsing data.
 */
int cq_select_all(struct dbconn con, const char *table, struct dlist **out,
        const char *conditions);

/**
 * @brief Populates a dlist with the return value of a basic function call.
 * @param con Database connection object with connection details.
 * @param func The name of the function to be called.
 * @param args UTF-8 string array of the arguments to the function.
 * @param num_args The number of arguments.
 * @param out An unallocated data list into which the data will be inserted.
 * @return 0 on success; less than 0 if memory error; from 1 to 10 if input
 * error; from 100 to 199 if query setup error; 200 if database connection
 * error; 201 if error submitting query; 202-299 if error parsing data.
 */
int cq_select_func_arr(struct dbconn con, const char *func, char * const *args,
        size_t num_args, struct dlist **out);

/**
 * @brief Populates a dlist with the return value of a function call with 
 * arguments derived from a drow structure.
 * @param con Database connection object with connection details.
 * @param func The name of the function to be called.
 * @param row The drow structure containing the argument data.
 * @return 0 on success; less than 0 if memory error; from 1 to 10 if input
 * error; from 100 to 199 if query setup error; 200 if database connection
 * error; 201 if error submitting query; 202-299 if error parsing data.
 */
int cq_select_func_drow(struct dbconn con, const char *func, struct drow row,
        struct dlist **out);

/**
 * @brief Gets the name of the primary key of a database table.
 * @param con Database connection object with connection details.
 * @param table UTF-8 string matching the name of the table to be examined.
 * @param out Buffer in which to store the result.
 * @param len Length of the out buffer.
 * @return 0 on success; less than 0 if memory error; from 1 to 10 if input
 * error; from 100 to 199 if query setup error; 200 if database connection
 * error; 201 if error submitting query; 202-299 if error parsing data.
 */
int cq_get_primkey(struct dbconn con, const char *table, char *out,
        size_t len);

/**
 * @brief Gets information about the fields in a database table.
 * @param con Database connection object with connection details.
 * @param table UTF-8 string matching the name of the table to be examined.
 * @param out_fieldc Destination for the number of fields; can be NULL.
 * @param out_names Destination for the field names; can be NULL.
 * @param nblen The length of each buffer in out_names.
 * @return 0 on success; less than 0 if memory error; from to to 10 if input
 * error; from 100 to 199 if query setup error; 200 if database connection
 * error; 201 if error submitting query; 202-299 if error parsing data.
 */
int cq_get_fields(struct dbconn con, const char *table, size_t *out_fieldc,
        char **out_names, size_t nblen);

/**
 * @brief Calls a stored database procedure with an array of arguments.
 * @param con Database connection object with connection details.
 * @param proc The name of the procedure to call (without parentheses).
 * @param args UTF-8 string array of the arguments to the function; non-numeric
 * values not prefixed with '\\' will be surrounded by apostrophes in the query.
 * @param num_args The number of elements in args.
 * @return 0 on success; less than 0 if memory error; from to to 10 if input
 * error; from 100 to 199 if query setup error; 200 if database connection
 * error; 201 if error submitting query.
 */
int cq_proc_arr(struct dbconn con, const char *proc, char * const *args,
        size_t num_args);

/**
 * @brief Calls a stored database procedure, deriving arguments from a drow 
 * structure.
 * @param con Database connection object with connection details.
 * @param proc The name of the procedure to call (without parentheses).
 * @param row The drow structure containing the argument data.
 * @return 0 on success; less than 0 if memory error; from to to 10 if input
 * error; from 100 to 199 if query setup error; 200 if database connection
 * error; 201 if error submitting query;
 */
int cq_proc_drow(struct dbconn con, const char *proc, struct drow row);

/**
 * @brief Grants permissions on a table or routine to a user.
 * @param con Database connection object with connection details.
 * @param perms Comma-separated list of permissions to be granted.
 * @param table The table or routine on which to grant permission.
 * @param user The database user to which to grant permission.
 * @param host The connection hostname or IP address for the user.
 * @param extra Any other additions to the query, such as a WITH statement.
 * @return 0 on success; less than 0 if memory error; from to to 10 if input
 * error; from 100 to 199 if query setup error; 200 if database connection
 * error; 201 if error submitting query;
 */
int cq_grant(struct dbconn con, const char *perms, const char *table,
        const char *user, const char *host, const char *extra);

/**
 * @brief Revokes permissions on a table or routine from a user.
 * @param con Database connection object with connection details.
 * @param perms Comma-separated list of permissions to be revoked.
 * @param table The table or routine on which to revoke permission.
 * @param user The database user from which to revoke permission.
 * @param host The connection hostname or IP address for the user.
 * @param extra Any other additions to the query, such as a WITH statement.
 * @return 0 on success; less than 0 if memory error; from to to 10 if input
 * error; from 100 to 199 if query setup error; 200 if database connection
 * error; 201 if error submitting query;
 */
int cq_revoke(struct dbconn con, const char *perms, const char *table,
        const char *user, const char *host, const char *extra);

#ifdef __cplusplus
}
#endif

#endif
