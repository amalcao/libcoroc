// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "libcoroc.h"

int Num = 1000;
int NumCPU = 2;
double *U, *V, *T;
double *_u, *_v;
coroc_chan_t finishChan = NULL;

static inline double Dot(double *v, double *u, int n) {
  double sum = 0.0;
  int i;

  for (i = 0; i < n; i++) sum += v[i] * u[i];
  return sum;
}

static inline double A(int i, int j) {
  return ((i + j) * (i + j + 1) / 2 + i + 1);
}

void task_Atv(int kk) {
  int size = (Num + NumCPU - 1) / NumCPU;
  int start = size * kk;
  int end = size * (kk + 1);
  if (end > Num) end = Num;

  int ul = Num;
  int i, j;
  for (i = start; i < end; i++) {
    double vi = 0;
    for (j = 0; j < ul; j++) vi += (_u[j] / (double)(A(j, i)));

    _v[i] = vi;
  }

  bool finish = true;
  coroc_chan_send(finishChan, &finish);
}

void task_Av(int kk) {
  int size = (Num + NumCPU - 1) / NumCPU;
  int start = size * kk;
  int end = size * (kk + 1);
  if (end > Num) end = Num;

  int ul = Num;
  int i, j;
  for (i = start; i < end; i++) {
    double vi = 0;
    for (j = 0; j < ul; j++) vi += (_u[j] / (double)(A(i, j)));

    _v[i] = vi;
  }

  bool finish = true;
  coroc_chan_send(finishChan, &finish);
}

void mult_Atv(double *v, double *u) {
  int k;
  bool finish;
  _v = v;
  _u = u;
  for (k = 0; k < NumCPU; k++) {
    coroc_coroutine_t task =
        coroc_coroutine_allocate(task_Atv, k, "", 
                               TSC_COROUTINE_NORMAL, 
                               TSC_DEFAULT_PRIO, NULL);
  }

  for (k = 0; k < NumCPU; k++) coroc_chan_recv(finishChan, &finish);
}

void mult_Av(double *v, double *u) {
  int k;
  bool finish;
  _v = v;
  _u = u;
  for (k = 0; k < NumCPU; k++) {
    coroc_coroutine_t task =
        coroc_coroutine_allocate(task_Av, k, "",
                               TSC_COROUTINE_NORMAL, 
                               TSC_DEFAULT_PRIO, NULL);
  }

  for (k = 0; k < NumCPU; k++) coroc_chan_recv(finishChan, &finish);
}

void mult_AtAv(double *v, double *u, double *x) {
  mult_Av(x, u);
  mult_Atv(v, x);
}

double SpectralNorm(int n) {
  U = malloc(sizeof(double) * n);
  V = malloc(sizeof(double) * n);
  T = malloc(sizeof(double) * n);

  int i;

  for (i = 0; i < n; i++) U[i] = 1;

  for (i = 0; i < 10; i++) {
    mult_AtAv(V, U, T);
    mult_AtAv(U, V, T);
  }

  double dot_uv = Dot(U, V, n);
  double dot_vv = Dot(V, V, n);

  free(U);
  free(V);
  free(T);
  return sqrt(dot_uv / dot_vv);
}

void main(int argc, char **argv) {
  if (argc > 1) Num = atoi(argv[1]);

  finishChan = coroc_chan_allocate(sizeof(bool), 0);
  printf("%0.9f\n", SpectralNorm(Num));
  // coroc_chan_dealloc (finishChan);
}
