cquel API Tutorial
==================

The cquel API relies on certain data structures. See [structures.md][1] for more
information. All functions take UTF-8 string input. Thus, the easiest way to
input is via the `u8""` string literal available in C11.

For all of these, you must `#include <cquel.h>`.

The API is initialized by creating an instance of `struct dbconn` via the
`cq_new_connection()` function.

``` c
struct dbconn mydb = cq_new_connection(u8"127.0.0.1", u8"myuser", u8"mypasswd",
        u8"mydb");
```

Reading from a table
--------------------

Let's read from an example table `Person` in `mydb` and print all the field
names and their values separated by tabs.

``` c
/* set global buffer sizes */
cq_init(1024, 128);

struct dbconn mydb = cq_new_connection(u8"127.0.0.1", u8"myuser", u8"mypasswd",
        u8"mydb");
```

There are two ways to select from a database: `cq_select_query()` and
`cq_select_all()`. The former allows for more control, but you are required to
have a bit of understanding of SQL.

``` c
struct dlist *people = NULL;

/* this call is the same as cq_select_query(mydb, &people, u8"* FROM Person") */
if (cq_select_all(mydb, u8"Person", &people, u8"")) {
    /* handle errors */
}
```

Now we can print the data to the screen.

``` c
for (size_t i = 0; i < people->fieldc; ++i) {
    printf("%s\t", people->fieldnames[i]);
}
putchar('\n');

for (struct drow *person = people->first; person != NULL;
        person = person->next) {
    for (size_t i = 0; i < person->fieldc; ++i) {
        printf("%s\t", person->values[i]);
    }
    putchar('\n');
}
```

Finally, we must clean up.

``` c
cq_free_dlist(people);
```

Inserting into a table
----------------------

We must first populate a `struct dlist` (data list) with `struct drow` (data
row) data before we can send it to cquel.

Let's assume we have a table `Person` in `mydb`, and the fields (excluding the
primary key) are `first`, `middle`, `last`, and `dob` (date of birth).

``` c
/* set global buffer sizes */
cq_init(1024, 128);

struct dbconn mydb = cq_new_connection(u8"127.0.0.1", u8"myuser", u8"mypasswd",
        u8"mydb");
char *tabledata[4];
tabledata[0] = u8"first";
tabledata[1] = u8"middle";
tabledata[2] = u8"last";
tabledata[3] = u8"dob";

char *rmsdata[4];
rmsdata[0] = u8"Richard";
rmsdata[1] = u8"Matthew";
rmsdata[2] = u8"Stallman";
rmsdata[3] = u8"1953-03-16";

/* set primary key to NULL for brevity; it's not required to insert */
struct dlist *mylist = cq_new_dlist(4, tabledata, NULL);
struct drow *rms = cq_new_drow(4);
cq_drow_set(rms, rmsdata); /* set this row to have the values from rmsdata */

cq_dlist_add(mylist, rms); /* add our row to the list */
```

With that set up, now we can insert.

``` c
if (cq_insert(mydb, u8"Person", mylist)) {
    /* handle errors */
}
```

That function would send the following query to the database:

``` sql
INSERT INTO Person(first,middle,last,dob) VALUES('Richard','Matthew','Stallman','1953-03-16');
```

If `mylist` had more than one `struct drow`, it would run a separate, similar
query for each row, allowing you to mass-insert.

Finally, we must clean up.

``` c
cq_free_dlist(mylist);
```

Updating a table
----------------

Updating a table in cquel is a bit non-conventional, since changes are made to
the data in memory and not remotely.

Let's say we have a table `Person` in `mydb`, and we want to stop using
nicknames in the database. Let's change all instances of "Bob" with "Robert" in
the person's name. Let's also assume that the first column is a primary key
`id`, and the other columns are shifted over by one.

First, we want to get all the instances of "Bob" in the `first` field of the
table.

``` c
/* set global buffer sizes */
cq_init(1024, 128);

struct dbconn mydb = cq_new_connection(u8"127.0.0.1", u8"myuser", u8"mypasswd",
        u8"mydb");

struct dlist *boblist = NULL;
if (cq_select_all(mydb, u8"Person", &boblist, u8"first='Bob'")) {
    /* handle errors */
}
```

Now, `boblist` is populated with the data. Let's change the appropriate values.

``` c
const char *newname = u8"Robert";
for (struct drow *row = boblist->first; row != NULL; row = row->next) {
    strcpy(row->values[1], newname); /* second column is first name */
}
```

Now that we've changed our data, we can commit to the database.

``` c
if (cq_update(mydb, u8"Person", boblist)) {
    /* handle errors */
}
```

Finally, we can clean up.

``` c
cq_free_dlist(boblist);
```

[1]: structures.md
