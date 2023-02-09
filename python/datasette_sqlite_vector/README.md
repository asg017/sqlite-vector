# The `datasette-sqlite-vector` Datasette Plugin

`datasette-sqlite-vector` is a [Datasette plugin](https://docs.datasette.io/en/stable/plugins.html) that loads the [`sqlite-vector`](https://github.com/asg017/sqlite-vector) extension in Datasette instances, allowing you to generate and work with [vectors](https://github.com/vector/spec) in SQL.

```
datasette install datasette-sqlite-vector
```

See [`docs.md`](../../docs.md) for a full API reference for the TODO SQL functions.

Alternatively, when publishing Datasette instances, you can use the `--install` option to install the plugin.

```
datasette publish cloudrun data.db --service=my-service --install=datasette-sqlite-vector

```
