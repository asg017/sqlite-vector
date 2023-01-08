#include <cstdio>
#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <vector>

#include "sqlite3ext.h"

SQLITE_EXTENSION_INIT1

#include "vectors.h"

// https://github.com/sqlite/sqlite/blob/master/src/json.c#L88-L89
#define JSON_SUBTYPE  74    /* Ascii for "J" */

#include <nlohmann/json.hpp>
using json = nlohmann::json;


struct VecX {
  int64_t size;
  float * data;
};


void del(void*p) {
  VectorFloat * vx = (VectorFloat *)p;
  sqlite3_free(vx->data);
  delete vx;
}

static void resultVector(sqlite3_context * context, std::vector<float>* v) {
  VectorFloat * vx = new VectorFloat();
  vx->size = v->size();
  vx->data = (float *) sqlite3_malloc(v->size()*sizeof(float));
  memcpy(vx->data, v->data(), v->size()*sizeof(float));
  //delete v;
  sqlite3_result_pointer(context, vx, VECTOR_FLOAT_POINTER_NAME, del);
}

#pragma region generic
static std::vector<float>* valueAsVector(sqlite3_value*value) {
  // Option 1: If the value is a "vectorf32v0" pointer, create vector from that
  VectorFloat* v = (VectorFloat*) sqlite3_value_pointer(value, VECTOR_FLOAT_POINTER_NAME);
  if (v!=NULL) return new std::vector<float>(v->data, v->data + v->size);

  // Option 2: if value is a JSON array coercible to float vector, use that
  if(sqlite3_value_subtype(value) == JSON_SUBTYPE) {
    std::vector<float> v; 
    json data = json::parse(sqlite3_value_text(value));
    data.get_to(v);
    return  new std::vector<float>(v);
  }

  // else, value isn't a vector
  return NULL;
}
#pragma endregion

#pragma region meta
static void vector_version(sqlite3_context *context, int argc, sqlite3_value **argv) {
  sqlite3_result_text(context, "yo", -1, SQLITE_STATIC);
}
static void vector_debug(sqlite3_context *context, int argc, sqlite3_value **argv) {
  if(argc){
    std::vector<float>* v = valueAsVector(argv[0]);
    if(v==NULL) {
      sqlite3_result_error(context, "value not vector", -1);
      return;
    }
    sqlite3_str * str = sqlite3_str_new(0);
    sqlite3_str_appendf(str, "size: %lld [", v->size());
    for(int i = 0; i < v->size(); i++) {
      if(i==0) sqlite3_str_appendf(str, "%f", v->at(i));
      else sqlite3_str_appendf(str, ", %f", v->at(i));
    }
    sqlite3_str_appendchar(str, 1, ']');
    sqlite3_result_text(context, sqlite3_str_finish(str), -1, sqlite3_free);
      
  }else {
    sqlite3_result_text(context, "yo", -1, SQLITE_STATIC);
  }
}
#pragma endregion


#pragma region vector generation
// TODO should return fvec, ivec, or bvec depending on input. How do bvec, though?
static void vector_from(sqlite3_context *context, int argc, sqlite3_value **argv) {
  std::vector<float> * v = new std::vector<float>();
  v->reserve(argc);
  for(int i = 0; i < argc; i++) {
    v->push_back(sqlite3_value_double(argv[i]));
  }
  resultVector(context, v);
}
#pragma endregion

#pragma region vector general
static void vector_value_at(sqlite3_context *context, int argc, sqlite3_value **argv) {
  std::vector<float>*v = valueAsVector(argv[0]);
  if(v == NULL) return;
  int at = sqlite3_value_int(argv[1]);
  try {
    float result = v->at(at);
    sqlite3_result_double(context, result);
  }
   catch (const std::out_of_range& oor) {
    char * errmsg = sqlite3_mprintf("%d out of range: %s", at, oor.what());
    if(errmsg != NULL){
      sqlite3_result_error(context, errmsg, -1);
      sqlite3_free(errmsg);
    }
    else sqlite3_result_error_nomem(context);
  }
}

static void vector_length(sqlite3_context *context, int argc, sqlite3_value **argv) {
  VectorFloat* v = (VectorFloat*) sqlite3_value_pointer(argv[0], VECTOR_FLOAT_POINTER_NAME);
  if(v==NULL) return;
  sqlite3_result_int64(context, v->size);
}
#pragma endregion




#pragma region json
static void vector_to_json(sqlite3_context *context, int argc, sqlite3_value **argv) {
  std::vector<float>*v = valueAsVector(argv[0]);
  if(v == NULL) return;

  json j = json(*v);
  sqlite3_result_text(context, j.dump().c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_result_subtype(context, JSON_SUBTYPE);
}
static void vector_from_json(sqlite3_context *context, int argc, sqlite3_value **argv) {
  const char * text = (const char *) sqlite3_value_text(argv[0]);
  json j = json::parse(text);
  std::vector<float> *v = new std::vector<float>(); 
  j.get_to(*v);
  resultVector(context, v);
  
}
#pragma endregion

#pragma region blob

/*

|Offset | Size | Description
|-|-|-
|a|a|A
*/
static void vector_to_blob(sqlite3_context *context, int argc, sqlite3_value **argv) {
  std::vector<float>*v = valueAsVector(argv[0]);
  if(v == NULL) return;
  
  int sz = v->size();
  int n = (sizeof(char)) + (sizeof(char)) + (sizeof(int)) + (sz * 4);
  void * b = sqlite3_malloc(n);
  memset(b, 0, n);

  memcpy((void *) ((char *) b+0), (void *) &VECTOR_BLOB_HEADER_BYTE, sizeof(char));
  memcpy((void *) ((char *) b+1), (void *) &VECTOR_BLOB_HEADER_TYPE, sizeof(char));
  memcpy((void *) ((char *) b+2), (void *) &sz, sizeof(int));
  memcpy((void *) ((char *) b+6), (void *) v->data(), sz*4);
  sqlite3_result_blob64(context, b, n, sqlite3_free);
  
}
static void vector_from_blob(sqlite3_context *context, int argc, sqlite3_value **argv) {
  int n = sqlite3_value_bytes(argv[0]);
  const void * b;
  char header;
  char type;
  int size;

  if(n < (6)) {
    sqlite3_result_error(context, "Vector blob size less than header length", -1);
    return;
  }
  b = sqlite3_value_blob(argv[0]);
  memcpy(&header, ((char *) b + 0), sizeof(char));
  memcpy(&type,   ((char *) b + 1), sizeof(char));
  memcpy(&size,   ((char *) b + 2), sizeof(int));

  if(header != VECTOR_BLOB_HEADER_BYTE) {
    sqlite3_result_error(context, "Blob not well-formatted vector blob", -1);
    return;
  }
  if(type != VECTOR_BLOB_HEADER_TYPE) {
    sqlite3_result_error(context, "Blob type not right", -1);
    return;
  }
  if(size < 0) {
    sqlite3_result_error(context, "unreasonable blob type size, negative", -1);
    return;
  }
  
  if(size != ((n - 1 - 1 - 4)) / sizeof(float)) {
    sqlite3_result_error(context, "unreasonable vector size, doesn't match blob size", -1);
    return;
  }
  
  float * v = (float *) ((char *)b + 6); 
  std::vector<float> *vec = new std::vector<float>(v, v+size); 
  resultVector(context, vec);
  
}
#pragma endregion




#pragma region fvecs vtab

typedef struct fvecsEach_vtab fvecsEach_vtab;
struct fvecsEach_vtab {
  sqlite3_vtab base;  /* Base class - must be first */
};

typedef struct fvecsEach_cursor fvecsEach_cursor;
struct fvecsEach_cursor {
  sqlite3_vtab_cursor base;  /* Base class - must be first */
  sqlite3_int64 iRowid;
  // malloc'ed copy of fvecs input blob
  void * pBlob;
  // total size of pBlob in bytes
  sqlite3_int64 iBlobN;
  sqlite3_int64 p;
  
  // current dimensions
  int iCurrentD;
  // pointer to current vector being read in
  std::vector<float>* pCurrentVector;
};

static int fvecsEachConnect(
  sqlite3 *db,
  void *pAux,
  int argc, const char *const*argv,
  sqlite3_vtab **ppVtab,
  char **pzErr
){
  fvecsEach_vtab *pNew;
  int rc;

  rc = sqlite3_declare_vtab(db,
           "CREATE TABLE x(dimensions, vector, input hidden)"
       );
#define FVECS_EACH_DIMENSIONS   0
#define FVECS_EACH_VECTOR       1
#define FVECS_EACH_INPUT        2
  if( rc==SQLITE_OK ){
    pNew = (fvecsEach_vtab *) sqlite3_malloc( sizeof(*pNew) );
    *ppVtab = (sqlite3_vtab*)pNew;
    if( pNew==0 ) return SQLITE_NOMEM;
    memset(pNew, 0, sizeof(*pNew));
  }
  return rc;
}

static int fvecsEachDisconnect(sqlite3_vtab *pVtab){
  fvecsEach_vtab *p = (fvecsEach_vtab*)pVtab;
  sqlite3_free(p);
  return SQLITE_OK;
}

static int fvecsEachOpen(sqlite3_vtab *p, sqlite3_vtab_cursor **ppCursor){
  fvecsEach_cursor *pCur;
  pCur = (fvecsEach_cursor *)sqlite3_malloc( sizeof(*pCur) );
  if( pCur==0 ) return SQLITE_NOMEM;
  memset(pCur, 0, sizeof(*pCur));
  *ppCursor = &pCur->base;
  return SQLITE_OK;
}

static int fvecsEachClose(sqlite3_vtab_cursor *cur){
  fvecsEach_cursor *pCur = (fvecsEach_cursor*)cur;
  sqlite3_free(pCur);
  return SQLITE_OK;
}


static int fvecsEachBestIndex(
  sqlite3_vtab *tab,
  sqlite3_index_info *pIdxInfo
){
  for (int i = 0; i < pIdxInfo->nConstraint; i++) {
    auto pCons = pIdxInfo->aConstraint[i];
    switch (pCons.iColumn) {
      case FVECS_EACH_INPUT: {
        if (pCons.op == SQLITE_INDEX_CONSTRAINT_EQ && pCons.usable) {
          pIdxInfo->aConstraintUsage[i].argvIndex = 1;
          pIdxInfo->aConstraintUsage[i].omit = 1;
        }
        break;
      }
    }
    }
  pIdxInfo->estimatedCost = (double)10;
  pIdxInfo->estimatedRows = 10;
  return SQLITE_OK;
}

static int fvecsEachFilter(
  sqlite3_vtab_cursor *pVtabCursor, 
  int idxNum, const char *idxStr,
  int argc, sqlite3_value **argv
){
  fvecsEach_cursor *pCur = (fvecsEach_cursor *)pVtabCursor;

  int n = sqlite3_value_bytes(argv[0]);
  const void * b = sqlite3_value_blob(argv[0]);

  pCur->pBlob = sqlite3_malloc(n);
  pCur->iBlobN = n;
  pCur->iRowid = 1;
  memcpy(pCur->pBlob, b, n);

  memcpy(&pCur->iCurrentD, pCur->pBlob, sizeof(int));
  float * v = (float *) ((char *)pCur->pBlob + sizeof(int)); 
  pCur->pCurrentVector = new std::vector<float>(v, v+pCur->iCurrentD);
  pCur->p = sizeof(int) + (pCur->iCurrentD*sizeof(float));
  
  return SQLITE_OK;
}

static int fvecsEachNext(sqlite3_vtab_cursor *cur){
  fvecsEach_cursor *pCur = (fvecsEach_cursor*)cur;
  
  memcpy(&pCur->iCurrentD, ((char *)pCur->pBlob + pCur->p), sizeof(int));
  float * v = (float *) (((char *)pCur->pBlob + pCur->p) + sizeof(int)); 
  pCur->pCurrentVector->clear();
  pCur->pCurrentVector->reserve(pCur->iCurrentD);// = new std::vector<float>(v, v+pCur->iCurrentD);
  pCur->pCurrentVector->insert(pCur->pCurrentVector->begin(), v, v+pCur->iCurrentD);

  pCur->p += (sizeof(int) + (pCur->iCurrentD*sizeof(float)));
  pCur->iRowid++;
  return SQLITE_OK;
}

static int fvecsEachEof(sqlite3_vtab_cursor *cur){
  fvecsEach_cursor *pCur = (fvecsEach_cursor*)cur;

  return pCur->p > pCur->iBlobN;
}

static int fvecsEachRowid(sqlite3_vtab_cursor *cur, sqlite_int64 *pRowid){
  fvecsEach_cursor *pCur = (fvecsEach_cursor*)cur;
  *pRowid = pCur->iRowid;
  return SQLITE_OK;
}

static int fvecsEachColumn(
  sqlite3_vtab_cursor *cur,   /* The cursor */
  sqlite3_context *context,       /* First argument to sqlite3_result_...() */
  int i                       /* Which column to return */
){
  fvecsEach_cursor *pCur = (fvecsEach_cursor*)cur;
  switch( i ){
    case FVECS_EACH_DIMENSIONS:
      sqlite3_result_int(context, pCur->iCurrentD);
      break;
    case FVECS_EACH_VECTOR:
      resultVector(context, pCur->pCurrentVector);
      break;
    case FVECS_EACH_INPUT:
      sqlite3_result_null(context);
      break;
  }
  return SQLITE_OK;
}

/*
** This following structure defines all the methods for the 
** virtual table.
*/
static sqlite3_module fvecsEachModule = {
  /* iVersion    */ 0,
  /* xCreate     */ fvecsEachConnect,
  /* xConnect    */ fvecsEachConnect,
  /* xBestIndex  */ fvecsEachBestIndex,
  /* xDisconnect */ fvecsEachDisconnect,
  /* xDestroy    */ 0,
  /* xOpen       */ fvecsEachOpen,
  /* xClose      */ fvecsEachClose,
  /* xFilter     */ fvecsEachFilter,
  /* xNext       */ fvecsEachNext,
  /* xEof        */ fvecsEachEof,
  /* xColumn     */ fvecsEachColumn,
  /* xRowid      */ fvecsEachRowid,
  /* xUpdate     */ 0,
  /* xBegin      */ 0,
  /* xSync       */ 0,
  /* xCommit     */ 0,
  /* xRollback   */ 0,
  /* xFindMethod */ 0,
  /* xRename     */ 0,
  /* xSavepoint  */ 0,
  /* xRelease    */ 0,
  /* xRollbackTo */ 0,
  /* xShadowName */ 0
};

#pragma endregion


#pragma region fvecs

static void vector_fvecs(sqlite3_context *context, int argc, sqlite3_value **argv) {
  sqlite3_int64 sz = sqlite3_value_bytes(argv[0]);
  const void * blob = sqlite3_value_blob(argv[0]);
  int d;
  memcpy((void *) &d, (void *) blob, sizeof(int));
  if(d <= 0 || d >= 1000000) {
    sqlite3_result_error(context, "unreasonable dimensions size", -1);
    return;
  }
  if( sz % ((d + 1) * 4) != 0) {
    sqlite3_result_error(context, "wrong blob size", -1);
    return;
  }
  size_t n = sz / ((d + 1) * 4);
  printf("sz=%lld, d=%d n=%zu\n", sz, d, n);

  float* x = new float[n * (d + 1)];
  memcpy(x, ((char*) blob) + sizeof(int), (sizeof(float)) * (n * (d + 1)));
  //for (size_t i = 0; i < n; i++)
  //      memmove(x + i * d, x + 1 + i * (d + 1), d * sizeof(*x));

  printf("x[0]=%f \n", x[0]);
  printf("x[1]=%f \n", x[1]);
  //sqlite3_result_text(context, "yo", -1, SQLITE_STATIC);
}
#pragma endregion


#pragma region entrypoint

extern "C" {
  #ifdef _WIN32
  __declspec(dllexport)
  #endif
  int sqlite3_extension_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi) {
    SQLITE_EXTENSION_INIT2(pApi);
    sqlite3_create_function_v2(db, "vector_version", 0, SQLITE_UTF8|SQLITE_DETERMINISTIC|SQLITE_INNOCUOUS, 0, vector_version, 0, 0, 0);
    sqlite3_create_function_v2(db, "vector_debug", 0, SQLITE_UTF8|SQLITE_DETERMINISTIC|SQLITE_INNOCUOUS, 0, vector_debug, 0, 0, 0); 
    sqlite3_create_function_v2(db, "vector_debug", 1, SQLITE_UTF8|SQLITE_DETERMINISTIC|SQLITE_INNOCUOUS, 0, vector_debug, 0, 0, 0); 

    sqlite3_create_function_v2(db, "vector_length", 1, SQLITE_UTF8|SQLITE_INNOCUOUS, 0, vector_length, 0, 0, 0); 
    sqlite3_create_function_v2(db, "vector_value_at", 2, SQLITE_UTF8|SQLITE_INNOCUOUS, 0, vector_value_at, 0, 0, 0); 
    sqlite3_create_function_v2(db, "vector_from_json", 1, SQLITE_UTF8|SQLITE_DETERMINISTIC|SQLITE_INNOCUOUS, 0, vector_from_json, 0, 0, 0); 
    sqlite3_create_function_v2(db, "vector_to_json", 1, SQLITE_UTF8|SQLITE_DETERMINISTIC|SQLITE_INNOCUOUS, 0, vector_to_json, 0, 0, 0); 
    
    sqlite3_create_function_v2(db, "vector_from_blob", 1, SQLITE_UTF8|SQLITE_DETERMINISTIC|SQLITE_INNOCUOUS, 0, vector_from_blob, 0, 0, 0); 
    sqlite3_create_function_v2(db, "vector_to_blob", 1, SQLITE_UTF8|SQLITE_DETERMINISTIC|SQLITE_INNOCUOUS, 0, vector_to_blob, 0, 0, 0); 

    sqlite3_create_module(db, "vector_fvecs_each", &fvecsEachModule, 0);

    
    //sqlite3_create_function_v2(db, "vector_fvecs", 1, SQLITE_UTF8|SQLITE_DETERMINISTIC|SQLITE_INNOCUOUS, 0, vector_fvecs, 0, 0, 0); 
    //sqlite3_create_function_v2(db, "vector_from", -1, SQLITE_UTF8|SQLITE_DETERMINISTIC|SQLITE_INNOCUOUS, 0, vector_from, 0, 0, 0); 
    return 0;
  }
}

#pragma endregion