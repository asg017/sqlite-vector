import sqlite3
import unittest
import time
import os

EXT_PATH="./build/vector0"


def connect(ext, path=":memory:"):
  db = sqlite3.connect(path)

  db.execute("create temp table base_functions as select name from pragma_function_list")
  db.execute("create temp table base_modules as select name from pragma_module_list")

  db.enable_load_extension(True)
  db.load_extension(ext)

  db.execute("create temp table loaded_functions as select name from pragma_function_list where name not in (select name from base_functions) order by name")
  db.execute("create temp table loaded_modules as select name from pragma_module_list where name not in (select name from base_modules) order by name")

  db.row_factory = sqlite3.Row
  return db


db = connect(EXT_PATH)

def explain_query_plan(sql):
  return db.execute("explain query plan " + sql).fetchone()["detail"]

def execute_all(cursor, sql, args=None):
  if args is None: args = []
  results = cursor.execute(sql, args).fetchall()
  return list(map(lambda x: dict(x), results))

FUNCTIONS = [
  'vector_debug',
  'vector_debug',
  'vector_from_blob',
  'vector_from_json',
  'vector_to_blob',
  'vector_to_json',
  'vector_version',
]

MODULES = []
class TestVector(unittest.TestCase):
  def test_funcs(self):
    funcs = list(map(lambda a: a[0], db.execute("select name from loaded_functions").fetchall()))
    self.assertEqual(funcs, FUNCTIONS)

  def test_modules(self):
    modules = list(map(lambda a: a[0], db.execute("select name from loaded_modules").fetchall()))
    self.assertEqual(modules, MODULES)

    
  def test_vector_version(self):
    self.assertEqual(db.execute("select vector_version()").fetchone()[0], "yo")

  def test_vector_debug(self):
    debug = db.execute("select vector_debug()").fetchone()[0].split('\n')
    self.assertEqual(len(debug), 1)
  
  def test_vector_from_blob(self):
    self.assertEqual(
      db.execute("select vector_debug(vector_from_blob(vector_to_blob(vector_from_json(?))))", ["[0.1,0.2]"]).fetchone()[0],
      "size: 2 [0.100000, 0.200000]"
    )
    def raises_small_blob_header(input):
      with self.assertRaisesRegex(sqlite3.OperationalError, "Vector blob size less than header length"):
        db.execute("select vector_from_blob(?)", [input]).fetchall()
    
    raises_small_blob_header(b"")
    raises_small_blob_header(b"v")
    raises_small_blob_header(b"v\x01")
    raises_small_blob_header(b"v\x01\x00")
    raises_small_blob_header(b"v\x01\x00\x00")
    raises_small_blob_header(b"v\x01\x00\x00\x00")
    
    with self.assertRaisesRegex(sqlite3.OperationalError, "Blob not well-formatted vector blob"):
        db.execute("select vector_from_blob(?)", [b"V\x01\x00\x00\x00\x00"]).fetchall()
    
    with self.assertRaisesRegex(sqlite3.OperationalError, "Blob type not right"):
        db.execute("select vector_from_blob(?)", [b"v\x00\x00\x00\x00\x00"]).fetchall()
    
    with self.assertRaisesRegex(sqlite3.OperationalError, "unreasonable blob type size, negative"):
        db.execute("select vector_from_blob(?)", [b"v\x01\xff\xff\xff\xff"]).fetchall()
    
    with self.assertRaisesRegex(sqlite3.OperationalError, "unreasonable vector size, doesn't match blob size"):
        db.execute("select vector_debug(vector_from_blob(?))", [b"v\x01\x01\x00\x00\x00"]).fetchone()
    


  def test_vector_to_blob(self):
    vector_to_blob = lambda x: db.execute("select vector_to_blob(vector_from_json(json(?)))", [x]).fetchone()[0]
    self.assertEqual(vector_to_blob("[]"), b"v\x01\x00\x00\x00\x00")
    self.assertEqual(vector_to_blob("[0.1]"), b"v\x01\x01\x00\x00\x00\xcd\xcc\xcc=")
  
  def test_vector_from_json(self):
    vector_from_json = lambda x: db.execute("select vector_debug(vector_from_json(json(?)))", [x]).fetchone()[0]
    self.assertEqual(vector_from_json('[0.1, 0.2, 0.3]'), "size: 3 [0.100000, 0.200000, 0.300000]")
    self.assertEqual(vector_from_json('[]'), "size: 0 []")
  
  def test_vector_to_json(self):
    vector_to_json = lambda x: db.execute("select vector_debug(vector_to_json(vector_from_json(json(?))))", [x]).fetchone()[0]
    self.assertEqual(vector_to_json('[0.1, 0.2, 0.3]'), "size: 3 [0.100000, 0.200000, 0.300000]")


class TestCoverage(unittest.TestCase):
  def test_coverage(self):
    test_methods = [method for method in dir(TestVector) if method.startswith('test_vector')]
    funcs_with_tests = set([x.replace("test_", "") for x in test_methods])
    for func in FUNCTIONS:
      self.assertTrue(func in funcs_with_tests, f"{func} does not have cooresponding test in {funcs_with_tests}")

if __name__ == '__main__':
    unittest.main()