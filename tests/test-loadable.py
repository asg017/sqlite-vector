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
  'vector0',
  'vector_debug',
  'vector_debug',
  'vector_from_blob',
  'vector_from_json',
  'vector_from_raw',
  'vector_length',
  'vector_to_blob',
  'vector_to_json',
  'vector_to_raw',
  'vector_value_at',
  'vector_version',
]

MODULES = ['vector_fvecs_each']
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
  
  def test_vector0(self):
    
    self.assertEqual(db.execute("select vector0(null)").fetchone()[0], None)
  
  def test_vector_from_blob(self):
    self.assertEqual(
      db.execute("select vector_debug(vector_from_blob(vector_to_blob(vector_from_json(?))))", ["[0.1,0.2]"]).fetchone()[0],
      "size: 2 [0.100000, 0.200000]"
    )
    def raises_small_blob_header(input):
      with self.assertRaisesRegex(sqlite3.OperationalError, "Vector blob size less than header length"):
        db.execute("select vector_from_blob(?)", [input]).fetchall()
    #import pdb;pdb.set_trace()
    raises_small_blob_header(b"")
    raises_small_blob_header(b"v")
    
    with self.assertRaisesRegex(sqlite3.OperationalError, "Blob not well-formatted vector blob"):
        db.execute("select vector_from_blob(?)", [b"V\x01\x00\x00\x00\x00"]).fetchall()
    
    with self.assertRaisesRegex(sqlite3.OperationalError, "Blob type not right"):
        db.execute("select vector_from_blob(?)", [b"v\x00\x00\x00\x00\x00"]).fetchall()

  def test_vector_to_blob(self):
    vector_to_blob = lambda x: db.execute("select vector_to_blob(vector_from_json(json(?)))", [x]).fetchone()[0]
    self.assertEqual(vector_to_blob("[]"), b"v\x01")
    self.assertEqual(vector_to_blob("[0.1]"), b"v\x01\xcd\xcc\xcc=")
    self.assertEqual(vector_to_blob("[0.1, 0]"), b"v\x01\xcd\xcc\xcc=\x00\x00\x00\x00")
  
  def test_vector_to_raw(self):
    vector_to_raw = lambda x: db.execute("select vector_to_raw(vector_from_json(json(?)))", [x]).fetchone()[0]
    self.assertEqual(vector_to_raw("[]"), None) # TODO why not b""
    self.assertEqual(vector_to_raw("[0.1]"), b"\xcd\xcc\xcc=")
    self.assertEqual(vector_to_raw("[0.1, 0]"), b"\xcd\xcc\xcc=\x00\x00\x00\x00")
  
  def test_vector_from_raw(self):
    vector_from_raw_blob = lambda x: db.execute("select vector_debug(vector_from_raw(?))", [x]).fetchone()[0]
    self.assertEqual(
      vector_from_raw_blob(b""),
      'size: 0 []'
    )
    self.assertEqual(
      vector_from_raw_blob(b"\x00\x00\x00\x00"),
      'size: 1 [0.000000]'
    )
    self.assertEqual(
      vector_from_raw_blob(b"\x00\x00\x00\x00\xcd\xcc\xcc="),
      'size: 2 [0.000000, 0.100000]'
    )

    with self.assertRaisesRegex(sqlite3.OperationalError, "Invalid raw blob length, must be divisible by 4"):
      vector_from_raw_blob(b"abc")

  def test_vector_from_json(self):
    vector_from_json = lambda x: db.execute("select vector_debug(vector_from_json(json(?)))", [x]).fetchone()[0]
    self.assertEqual(vector_from_json('[0.1, 0.2, 0.3]'), "size: 3 [0.100000, 0.200000, 0.300000]")
    self.assertEqual(vector_from_json('[]'), "size: 0 []")
  
  def test_vector_to_json(self):
    vector_to_json = lambda x: db.execute("select vector_debug(vector_to_json(vector_from_json(json(?))))", [x]).fetchone()[0]
    self.assertEqual(vector_to_json('[0.1, 0.2, 0.3]'), "size: 3 [0.100000, 0.200000, 0.300000]")
  
  def test_vector_length(self):
    vector_length = lambda x: db.execute("select vector_length(vector_from_json(json(?)))", [x]).fetchone()[0]
    self.assertEqual(vector_length('[0.1, 0.2, 0.3]'), 3)
    self.assertEqual(vector_length('[0.1]'), 1)
    self.assertEqual(vector_length('[]'), 0)
  
  def test_vector_value_at(self):
    vector_value_at = lambda x, y: db.execute("select vector_value_at(vector_from_json(json(?)), ?)", [x, y]).fetchone()[0]
    self.assertAlmostEqual(vector_value_at('[0.1, 0.2, 0.3]', 0), 0.1)
    self.assertAlmostEqual(vector_value_at('[0.1, 0.2, 0.3]', 1), 0.2)
    self.assertAlmostEqual(vector_value_at('[0.1, 0.2, 0.3]', 2), 0.3)
    #self.assertAlmostEqual(vector_value_at('[0.1, 0.2, 0.3]', 3), 0.3)


class TestCoverage(unittest.TestCase):
  def test_coverage(self):
    test_methods = [method for method in dir(TestVector) if method.startswith('test_vector')]
    funcs_with_tests = set([x.replace("test_", "") for x in test_methods])
    for func in FUNCTIONS:
      self.assertTrue(func in funcs_with_tests, f"{func} does not have cooresponding test in {funcs_with_tests}")

if __name__ == '__main__':
    unittest.main()