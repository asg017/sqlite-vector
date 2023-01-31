# sqlite-vector

A SQLite extension for working with float and binary vectors.

Still a work in progress, not meant to be wildly shared!

Once complete, you'll be able to do things like:

```sql
.load vector0


-- create a "vector" object from json
select vector_from_json(
  json('[0.1, 0.2, 0.3]')
);

-- convert a vector object into a compact BLOB
select vector_to_blob(
  vector_from_json(json('[0.1, 0.2, 0.3]'))
);
-- Result: X'760103000000cdcccc3dcdcc4c3e9a99993e'

select vector_length(
  vector_from_blob(X'760103000000cdcccc3dcdcc4c3e9a99993e')
);
-- Result: 3

select vector_value_at(
  vector_from_blob(X'760103000000cdcccc3dcdcc4c3e9a99993e'),
  2
);
-- Result: 0.3


select *
from vector_fvecs_each(readfile('sift1m_base.fvecs'));
```

- `vector_fvecs_each_read()`
- `vector_ivecs_each()`
- `vector_ivecs_each_read()`
- `vector_bvecs_each()`
- `vector_bvecs_each_read()`
