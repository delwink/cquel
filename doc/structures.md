cquel API Structures
====================

Operation in cquel requires a basic understanding of `struct`s in C and the
diagrams of the `struct`s used in cquel.

The first is `struct dbconn` which is used in every connected function in cquel
to determine how to connect to the database.

``` c
struct dbconn {
    MYSQL *con;
    const char *host;
    const char *user;
    const char *passwd;
    const char *database;
};
```

The database connection structure is mainly used as a utility. It should only be
altered by other calls to the library, such as `cq_insert()`.

The next structure is `struct drow`, which stores a row of data.

``` c
struct drow {
    size_t fieldc;
    char **values;

    struct drow *prev;
    struct drow *next;
};
```

`fieldc` indicates the number of columns that correspond to this row. The array
of `values` contains the corresponding field value for each column.

`prev` and `next` are utility pointers for advancing through the next structure,
`struct dlist`.

``` c
struct dlist {
    size_t fieldc;
    char **fieldnames;
    char *primkey

    struct drow *first;
    struct drow *last;
};
```

`fieldc` indicates the number of columns in the represented table, while
`fieldnames` contains the names of the fields. `primkey` stores the name of the
table's primary key.

`first` and `last` are utility pointers for iteration through the list.
