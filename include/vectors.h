#include <stdint.h>

struct VectorFloat {
  int64_t size;
  float * data;
};
char VECTOR_BLOB_HEADER_BYTE = 'v';
char VECTOR_BLOB_HEADER_TYPE = 1;
const char * VECTOR_FLOAT_POINTER_NAME = "vectorf32v0";
