# SQLite for Neve

SQLite integration for the Neve programming language, available as `import db`.

## Install

On Ubuntu/Debian, install SQLite's development package:

```bash
apt-get update
apt-get install -y libsqlite3-dev
```

With Neve and nsm already installed:

```bash
export NEVE_LIBS="$HOME/.local/neve/lib"
export NSK_LIBS="$NEVE_LIBS"
nsm i danielcasanova12/db
test -s "$NEVE_LIBS/db/lib.so"
neve "$NEVE_LIBS/db/tests/smoke.nv"
```

The first release binary targets Linux x86-64, Ubuntu 22.04 and a Neve runtime
linked against LLVM 21. A different Neve/LLVM ABI requires rebuilding `lib.so`.
The smoke test prints `PACKAGE db PASS` on success.

## Example

```text
import db

main
    any connection = db_open(":memory:")
    if db_valid(connection)==0
        print("Open error: ", db_error())
    else
        int rc = db_sql(connection, "CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT, balance REAL); INSERT INTO users VALUES (1, 'Daniel', 42.5); SELECT * FROM users WHERE id=1;")
        if rc!=0
            print("SQL error: ", db_error())
        rc = db_close(connection)
        if rc!=0
            print("Close error: ", db_error())
```

## API

| Function | Purpose |
| --- | --- |
| `db_open(path)` / `db_valid(handle)` | Open a database and check for a non-null handle |
| `db_exec(connection, sql)` | Execute SQL without printing result rows |
| `db_sql(connection, sql)` | Execute SQL and print column names and result rows |
| `db_console(connection)` | Interactive SQL console |
| `db_prepare(connection, sql)` | Prepare a single SQL statement |
| `db_bind_int(statement, parameter, value)` | Bind a 32-bit integer |
| `db_bind_float(statement, parameter, value)` | Bind a 32-bit Neve float as SQLite REAL |
| `db_bind_text(statement, parameter, value)` | Bind text |
| `db_bind_null(statement, parameter)` | Bind SQL NULL |
| `db_step(statement)` | Return 100 for a row, 101 for completion, otherwise an error |
| `db_column_int/float/text(statement, column)` | Read a value from the current row |
| `db_column_is_null(statement, column)` | Distinguish NULL from empty text or zero |
| `db_reset(statement)` | Reuse a statement; existing bindings remain |
| `db_finalize(statement)` | Release a prepared statement |
| `db_changes(connection)` | Number of rows changed by the last write statement |
| `db_close(connection)` | Close the connection |
| `db_error()` | Last error message on the current thread |

Parameter indices start at 1; column indices start at 0. Read columns only after
`db_step` returns 100. Finalize statements before closing their connection.
Do not reuse handles after successful close/finalize. Check each operation's
return code: `db_error()` retains the last error until another error or a new
successful connection. `db_valid` checks only for a non-null pointer.

Neve `int` and `float` in this integration are 32-bit. SQLite REAL storage is
64-bit, but values passed through the Neve float interface retain 32-bit precision.
Use prepared statements and parameters for application input. INNER JOIN, WHERE,
transactions and other SQLite SQL are handled by SQLite itself.

## Build from source

Install `make`, `clang-21`, `llvm-21-dev` and `libsqlite3-dev`, together with
the matching Neve compiler headers. Then run:

```bash
export NEVE_LIBS="$HOME/.local/neve/lib"
make NEVE_ROOT="$HOME/.local/neve"
```

The build produces `lib.so` in this directory. Neve reads metadata from `src/`
when importing the library, so both sources and `include.nv` must remain in
the repository even when distributing a precompiled binary.

## Release layout

The source files live in the repository. The binary is attached to a GitHub
release with the literal tag `latest`, under the exact filename `lib.so`.
This matches the download URL used by `nsm i danielcasanova12/db`:

```text
https://github.com/danielcasanova12/db/releases/download/latest/lib.so
```

The precompiled binary is intentionally excluded from Git history.
