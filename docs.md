# `sqlite-vector` Documentation

A full reference to every function and module that `sqlite-vector` offers.

As a reminder, `sqlite-vector` follows semver and is pre v1, so breaking changes are to be expected.

## The `sqlite-vector` BLOB encoding format

`sqlite-vector` can encode vectors into an efficient BLOB format with a 2-byte header. This is experimental and may change in the future, and only float vectors are supported for now.

### Float Vector format

`sqlite-vector` can read [32-bit](https://en.wikipedia.org/wiki/Single-precision_floating-point_format) float vectors from any of the following formats. 

#### 1. JSON

```sql
select vector_debug(json('[0.1, 0.2, 0.0]'));
```

#### 2. `sqlite-vector` BLOB format


#### 3. "Raw" `BLOB` format


https://numpy.org/doc/stable/reference/generated/numpy.ndarray.tobytes.html

https://numpy.org/doc/stable/reference/generated/numpy.ndarray.astype.html

```python
np.array([0.0, 0.1], dtype=np.float32).tobytes()

.astype('float32')

```

## API Reference

<h3 name="vector0"><code>vector0(pointer)</code></h3>

For most use-cases, returns `NULL`. For other extensions that want to build on top of `sqlite-vector`, such as [`sqlite-vss`](https://github.com/asg017/sqlite-vss), can use `vector0()` and SQLite's [Pointer Passing Interface](https://www.sqlite.org/bindptr.html) to expose functions like `xValueAsVector()` to other applications. See [`vectors.h`](./include/vectors.h) for more details.

```sql
select vector0(); -- NULL
```

<h3 name="vector_length"><code>vector_length()</code></h3>

```sql
select vector_length(); -- 
```

<h3 name="vector_value_at"><code>vector_value_at()</code></h3>

```sql
select vector_value_at(); -- 
```

<h3 name="vector_from_json"><code>vector_from_json()</code></h3>

```sql
select vector_from_json(); -- 
```

<h3 name="vector_to_json"><code>vector_to_json()</code></h3>

```sql
select vector_to_json(); -- 
```

<h3 name="vector_from_blob"><code>vector_from_blob()</code></h3>

```sql
select vector_from_blob(); -- 
```

<h3 name="vector_to_blob"><code>vector_to_blob()</code></h3>

```sql
select vector_to_blob(); -- 
```

<h3 name="vector_from_raw"><code>vector_from_raw()</code></h3>

```sql
select vector_from_raw(); -- 
```

<h3 name="vector_to_raw"><code>vector_to_raw()</code></h3>

```sql
select vector_to_raw(); -- 
```

<h3 name="vector_version"><code>vector_version()</code></h3>

Returns the semver version string of the current version of  `sqlite-vector`.

```sql
select vector_version(); -- 
```

<h3 name="vector_debug"><code>vector_debug([vector])</code></h3>

When no arguments are passed in, returns a debug string of various info about `sqlite-vector`. Includes the version string, build date, and commit hash. 

```sql
select vector_debug(); -- "..."
```

When an argument is passed in, then it is parsed as a vector and returns a debug string that include the vector's length and printed information. Useful when debugging. 

```sql
select vector_debug(json('[1.1, 1.2]'));
select vector_debug(json('[]'));
select vector_debug(x'00');
```