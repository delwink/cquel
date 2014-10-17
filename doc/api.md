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

[1]: structures.md
