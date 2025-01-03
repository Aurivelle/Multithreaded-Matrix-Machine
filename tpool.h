#pragma once

#include <pthread.h>

/* You may define additional structures / typedef's in this header file if
 * needed.
 */

typedef int **Matrix;
typedef int *Vector;

typedef struct request
{
  Matrix a;
  Matrix b;
  Matrix c;
  int work_number;
  struct request *next;
} request;

typedef struct work
{
  Matrix a;
  Matrix b_transpose;
  Matrix c;
  int n;
  int start;
  int end;
  struct work *next;
} work;

struct tpool
{
  int n;
  int thread_number;
  int end;

  int total_request;
  int processed_request;

  pthread_t frontend_thread;
  pthread_t *backend_thread;

  request *f_head;
  request *f_tail;
  pthread_mutex_t front_mutex;
  pthread_cond_t front_cond;

  work *b_head;
  work *b_tail;
  pthread_mutex_t back_mutex;
  pthread_cond_t back_cond;

  pthread_mutex_t sync_mutex;
  pthread_cond_t sync_cond;

  int undone_work_num;
};

struct tpool *tpool_init(int num_threads, int n);
void tpool_request(struct tpool *, Matrix a, Matrix b, Matrix c, int num_works);
void tpool_synchronize(struct tpool *);
void tpool_destroy(struct tpool *);
int calculation(int n, Vector, Vector); // Already implemented
