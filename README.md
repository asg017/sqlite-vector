# sqlite-vector

ftp://ftp.irisa.fr/local/texmex/corpus/siftsmall.tar.gz

```sql
select vectori32(1, 2, 3, 4); -- NULL, pointer="vectori32v0"
select vector_to_blob(vectori32(1,2,3,4)); -- X'xxxxxx'
select vector_from_blob(data) from x; -- NULL, pointer="vectori32v0"

select vector_from_json(json('...'))
select vector_to_json(vectori32(1, 2, 3, 4)); -- '[1,2,3,4]', subtype=J



-- ???
select vector_to_fvecs(); -- X'xxxx'
select vector_from_fvecs(json('[]'));

select vector_group(value) from xxx;

select vector from vector_fvecs_each(readfile('file.fvecs'));
```

|     Flag      | Description    |
| :-----------: | -------------- |
| `0b0000_0001` | boolean vector |
| `0b0000_0010` | `f32` vector   |
| `0b0000_0011` | `f64` vector   |
| `0b0000_0100` | `i32` vector   |
| `0b0000_0101` | `i64` vector   |

## `f32` Float Vectors

version # -\ \ /
begin \ | | |
`11 00 01 00`

| Offset | Size | Description                              |
| :----: | :--: | ---------------------------------------- |
|   0    |  1   | header: `0x76` (ASCII `v`), for "vector" |
|   1    |  1   | header: `0x01` for "f32 vector"          |
|   2    |  4   | header: size of vector                   |
|   6    |  ?   | elements                                 |
