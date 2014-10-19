cquel API Tutorial
==================

The cquel API relies on certain data structures. See [structures.md][1] for more
information. All functions take UTF-8 string input. Thus, the easiest way to
input is via the `u8""` string literal available in C11.

The API is initialized by creating an instance of `struct dbconn` via the
`cq_new_connection()` function.

``` c
struct dbconn mydb = cq_new_connection(u8"127.0.0.1", u8"myuser", u8"mypasswd",
        u8"mydb");
```

Inserting into a table
----------------------

We must first populate a `struct dlist` (data list) with `struct drow` (data
row) data before we can send it to cquel.

Let's assume we have a table `Person` in `mydb`, and the fields (excluding the
primary key) are `first`, `middle`, `last`, and `dob` (date of birth).

``` c
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

/* set primary key to NULL for brevity; it's not required */
struct dlist *mylist = cq_new_dlist(4, tabledata, NULL);
struct drow *rms = cq_new_drow(4);
cq_drow_set(rms, rmsdata); /* set this row to have the values from rmsdata */

cq_dlist_add(mylist, rms); /* add our row to the list */
```

With that set up, now we can insert.

``` c
if (cq_insert(*mydb, u8"Person", mylist)) {
    /* handle errors */
}
```

That function would send the following query to the database:

``` sql
INSERT INTO Person(first,middle,last,dob) VALUES(Richard,Matthew,Stallman,1953-03-16);
```

If `mylist` had more than one `struct drow`, it would run a separate, similar
query for each row, allowing you to mass-insert.

Finally, we must clean up.

``` c
cq_free_dlist(mylist);
```

The above function frees all the nodes in the list as well, so be sure not to
use them elsewhere without copying the memory. It does **NOT**, however, free 
any memory allocated to the string values stored in each `struct drow`. You must
free this memory separately.

[1]: structures.md
