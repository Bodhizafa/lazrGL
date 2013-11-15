#include <stdio.h>
// call f(x, y) such that x, y have been transformed by the mvMatrix
#define mvApply(x, y, f) f(mvMatrix[0] * x + mvMatrix[1] * y + mvMatrix[2], mvMatrix[3] * x + mvMatrix[4] * y + mvMatrix[5])
#define _mvMove(x, y) mvApply(x, y, move)
float * mvMatrix = NULL;
void mvMove(float x, float y) {
  _mvMove(x, y);
}

struct MatrixStackEntry {
  float matrix[9];
  struct MatrixStackEntry *prev;
};
struct MatrixStackEntry *top = NULL;

void dump_mat2(float* mat, int count) {
  Serial.print(" [");
  for (  int i = 0; i < count; i++) {
    Serial.print(" ");
    Serial.print(mat[i]);
    if (i == 2 || i == 5) {
      Serial.print(" ]\n [");
    }
  }
  Serial.print(" ]\n");
}

void dumpMvMatrix() {
  dump_mat2(mvMatrix, 9);
}

static void mvMatrixPush() {
  struct MatrixStackEntry *n = (struct MatrixStackEntry*)malloc(sizeof(*n));
  n->prev = top;
  top = n;
  memcpy(n->matrix, mvMatrix, sizeof(float) * 9);
  mvMatrix = n->matrix;
}

static void mvMatrixPop() {
  struct MatrixStackEntry *old = top;
  if (top->prev == NULL) {
    Serial.write("MVMATRIX POPPED TOO FAR!");
    return;
  }
  top = top->prev;
  mvMatrix = top->matrix;
  free((void*)old);
}

void mvMatrixTransform(float* mat) {
  if (debug) {
    dump_mat2(mat, 9);
    Serial.print("X\n");
    dump_mat2(mvMatrix, 9);
  }
  static float temp[9];
  memcpy(temp, mvMatrix, sizeof(float) * 9);
/*
         [ 0 1 2 ]
         [ 3 4 5 ]
         [ 6 7 8 ]
[ 0 1 2 ] 
[ 3 4 5 ]
[ 6 7 8 ]
*/

  mvMatrix[0] = temp[0] * mat[0] +
                temp[1] * mat[3] +
                temp[2] * mat[6];
                
  mvMatrix[1] = temp[0] * mat[1] +
                temp[1] * mat[4] +
                temp[2] * mat[7];
                
  mvMatrix[2] = temp[0] * mat[2] +
                temp[1] * mat[5] +
                temp[2] * mat[8];
                
  mvMatrix[3] = temp[3] * mat[0] +
                temp[4] * mat[3] + 
                temp[5] * mat[6];
                
  mvMatrix[4] = temp[3] * mat[1] +
                temp[4] * mat[4] +
                temp[5] * mat[7];
  
  mvMatrix[5] = temp[3] * mat[2] +
                temp[4] * mat[5] +
                temp[5] * mat[8];

  mvMatrix[6] = temp[6] * mat[0] +
                temp[7] * mat[3] + 
                temp[8] * mat[6];
                
  mvMatrix[7] = temp[6] * mat[1] + 
                temp[7] * mat[4] +
                temp[8] * mat[7];
  
  mvMatrix[8] = temp[6] * mat[2] + 
                temp[7] * mat[5] +
                temp[8] * mat[8];
  
  if (debug) {
    Serial.print("=\n");
    dump_mat2(mvMatrix, 9);
  }
}

static void mvMatrixTranslate(float x, float y) {
  if (debug) {
    Serial.print("Translation by (");
    Serial.print(x);
    Serial.print(", ");
    Serial.print(y);
    Serial.print("}\n");
  }
  static float tf[] = {
    1, 0, 0,
    0, 1, 0,
    0, 0, 1 };
  tf[2] = x;
  tf[5] = y;
  mvMatrixTransform( tf );
}

// DarkSide - Psycic
static void mvMatrixRotate(float th) {
  if (debug) {
    Serial.print("Rotation by (");
    Serial.print(th);
    Serial.print("}\n");
  }
  static float tf[] = {
    0, 0, 0,
    0, 0, 0,
    0, 0, 1 };
  tf[0] = cos(th);
  tf[1] = sin(th);
  tf[3] = -sin(th);
  tf[4] = cos(th);
  mvMatrixTransform(tf);
}

static void mvMatrixScale(float x, float y) {
  if (debug) {
    Serial.print("Scale by (");
    Serial.print(x);
    Serial.print(", ");
    Serial.print(y);
    Serial.print("}\n");
  }
  static float tf[] = {
    1, 0, 0,
    0, 1, 0,
    0, 0, 1 };
  tf[1] = x;
  tf[3] = y;
  mvMatrixTransform(tf);
}

static void mvMatrixIdentity() {
  static float identity[] = {
    1, 0, 0,
    0, 1, 0,
    0, 0, 1};
  memcpy(mvMatrix, identity, sizeof(float) * 9);
}

static void mvMatrixDefault() {
  static float def[] = {
   1024, 0, 2048,
   0, 1024, 2048,
   0, 0   , 1};
   memcpy(mvMatrix, def, sizeof(float) * 9);
};
