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

#include "cquel.h"

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
