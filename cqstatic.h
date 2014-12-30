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

int cq_fields_to_utf8(char *buf, size_t buflen, size_t fieldc,
        char * const *fieldnames, bool usequotes);

int cq_dlist_to_update_utf8(char *buf, size_t buflen, struct dlist list,
        struct drow row);

int cq_dlist_fields_to_utf8(char *buf, size_t buflen, struct dlist list);

int cq_drow_to_utf8(char *buf, size_t buflen, struct drow row);

int dlist_meta_cmp(const struct dlist *a, const struct dlist *b);

int grant_revoke(struct dbconn con, const char *act, const char *perms,
        const char *table, const char *user, const char *host,
        const char *extra);
