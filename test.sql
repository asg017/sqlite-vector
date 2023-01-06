.load build/vector0

.header on
.mode box
.echo on

select 
  rowid, 
  dimensions, 
  (select sum(value) from json_each(vector_to_json(vector))) as x
from vector_fvecs_each(readfile('tests/data/siftsmall/siftsmall_base.fvecs'))
limit 10;

.timer on
select count(*) from vector_fvecs_each(readfile('tests/data/siftsmall/siftsmall_learn.fvecs'));

select vector_value_at(vector, 128) 
from vector_fvecs_each(readfile('tests/data/siftsmall/siftsmall_base.fvecs'))
limit 10;

.exit

.mode quote 

select vector_debug(
  vector_from_json(json('[0.1, 0.2, 0.3]'))
);

select vector_to_blob(json('[]'));1
select vector_to_blob(json('[0.0]'));
select vector_to_blob(json('[0.1, 0.2, 0.3]'));

select vector_debug(vector_from_blob(vector_to_blob(json('[0.1, 0.2, 0.3]'))));


.header on
.mode box

select 
  length(json('[0.1,0.2,0.3]')) as json_length, 
  length(vector_to_blob(json('[0.1, 0.2, 0.3]'))) as vector_blob_length;

.exit
select vector_fvecs(readfile('tests/data/siftsmall/siftsmall_base.fvecs'));


select vector_to_json(json_array(.1, .2, .3));

.print =================================

select vector_to_json(vector_from(1.9,2.8,3.7,4.6));