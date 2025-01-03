#include "tpool.h"
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>

void transpose(Matrix b, int n)
{
    for (int i = 0; i < n; i++)
    {
        for (int j = i + 1; j < n; j++)
        {
            int temp = b[i][j];
            b[i][j] = b[j][i];
            b[j][i] = temp;
        }
    }
}

void *front_end(void *arg)
{
    struct tpool *pool = (struct tpool *)arg;

    while (1)
    {
        pthread_mutex_lock(&pool->front_mutex);
        while (!pool->f_head && !pool->end)
        {
            pthread_cond_wait(&pool->front_cond, &pool->front_mutex);
        }
        if (pool->end && !pool->f_head)
        {
            pthread_mutex_unlock(&pool->front_mutex);
            break;
        }

        request *req = pool->f_head;
        pool->f_head = req->next;
        if (!pool->f_head)
        {
            pool->f_tail = NULL;
        }
        pthread_mutex_unlock(&pool->front_mutex);

        transpose(req->b, pool->n);
        int total = pool->n * pool->n;
        int num_work = req->work_number;
        int base = total / num_work;
        int remainder = total % num_work;

        int start = 0;
        pthread_mutex_lock(&pool->sync_mutex);
        int work_count = num_work;
        pool->undone_work_num += work_count;
        pthread_mutex_unlock(&pool->sync_mutex);

        for (int i = 0; i < num_work; i++)
        {
            int work_size = base + (i < remainder ? 1 : 0);
            int end = start + work_size - 1;

            work *new = (work *)malloc(sizeof(work));
            new->a = req->a;
            new->b_transpose = req->b;
            new->c = req->c;

            new->n = pool->n;
            new->start = start;
            new->end = end;
            new->next = NULL;

            pthread_mutex_lock(&pool->back_mutex);
            if (!pool->b_head)
            {
                pool->b_head = new;
                pool->b_tail = new;
            }
            else
            {
                pool->b_tail->next = new;
                pool->b_tail = new;
            }
            pthread_cond_signal(&pool->back_cond);
            pthread_mutex_unlock(&pool->back_mutex);

            start = end + 1;
        }
        free(req);
        pthread_mutex_lock(&pool->sync_mutex);
        pool->processed_request++;
        pthread_mutex_unlock(&pool->sync_mutex);
    }
    return NULL;
}

void *back_end(void *arg)
{
    struct tpool *pool = (struct tpool *)arg;

    while (1)
    {
        pthread_mutex_lock(&pool->back_mutex);
        while (!pool->b_head && !pool->end)
        {
            pthread_cond_wait(&pool->back_cond, &pool->back_mutex);
        }

        if (pool->end && !pool->b_head)
        {
            pthread_mutex_unlock(&pool->back_mutex);
            break;
        }

        work *w = pool->b_head;
        pool->b_head = w->next;
        if (!pool->b_head)
        {
            pool->b_tail = NULL;
        }
        pthread_mutex_unlock(&pool->back_mutex);

        int n = w->n;
        for (int i = w->start; i <= w->end; i++)
        {
            int r = i / n;
            int c = i % n;
            w->c[r][c] = calculation(n, w->a[r], w->b_transpose[c]);
        }

        free(w);

        pthread_mutex_lock(&pool->sync_mutex);
        pool->undone_work_num--;
        if (pool->undone_work_num == 0)
        {
            pthread_cond_signal(&pool->sync_cond);
        }
        pthread_mutex_unlock(&pool->sync_mutex);
    }
    return NULL;
}

struct tpool *tpool_init(int num_threads, int n)
{
    struct tpool *pool = (struct tpool *)malloc(sizeof(struct tpool));

    pool->n = n; // dimension
    pool->thread_number = num_threads;
    pool->end = 0;
    pool->undone_work_num = 0;
    pool->total_request = 0;
    pool->processed_request = 0;

    pool->f_head = NULL;
    pool->f_tail = NULL;
    pthread_mutex_init(&pool->front_mutex, NULL);
    pthread_cond_init(&pool->front_cond, NULL);

    pool->b_head = NULL;
    pool->b_tail = NULL;
    pthread_mutex_init(&pool->back_mutex, NULL);
    pthread_cond_init(&pool->back_cond, NULL);

    pthread_mutex_init(&pool->sync_mutex, NULL);
    pthread_cond_init(&pool->sync_cond, NULL);

    pool->backend_thread = (pthread_t *)malloc(sizeof(pthread_t) * num_threads);

    pthread_create(&pool->frontend_thread, NULL, front_end, (void *)pool);
    for (int i = 0; i < num_threads; i++)
    {
        pthread_create(&pool->backend_thread[i], NULL, back_end, (void *)pool);
    }
    return pool;
}

void tpool_request(struct tpool *pool, Matrix a, Matrix b, Matrix c,
                   int num_works)
{

    request *req = (request *)malloc(sizeof(request));
    req->a = a;
    req->b = b;
    req->c = c;
    req->work_number = num_works;
    req->next = NULL;

    pthread_mutex_lock(&pool->front_mutex);

    if (!pool->f_head)
    {
        pool->f_head = req;
        pool->f_tail = req;
    }
    else
    {
        pool->f_tail->next = req;
        pool->f_tail = req;
    }
    pool->total_request++;
    pthread_cond_signal(&pool->front_cond);

    pthread_mutex_unlock(&pool->front_mutex);
}

void tpool_synchronize(struct tpool *pool)
{

    pthread_mutex_lock(&pool->sync_mutex);
    while (pool->undone_work_num != 0 || pool->processed_request != pool->total_request)
    {
        pthread_cond_wait(&pool->sync_cond, &pool->sync_mutex);
    }
    pthread_mutex_unlock(&pool->sync_mutex);
}

void tpool_destroy(struct tpool *pool)
{
    pthread_mutex_lock(&pool->front_mutex);
    pool->end = 1;
    pthread_cond_broadcast(&pool->front_cond);
    pthread_mutex_unlock(&pool->front_mutex);

    pthread_mutex_lock(&pool->back_mutex);
    pthread_cond_broadcast(&pool->back_cond);
    pthread_mutex_unlock(&pool->back_mutex);
    pthread_join(pool->frontend_thread, NULL);

    for (int i = 0; i < pool->thread_number; i++)
    {

        pthread_join(pool->backend_thread[i], NULL);
    }

    free(pool->backend_thread);

    pthread_mutex_destroy(&pool->front_mutex);
    pthread_cond_destroy(&pool->front_cond);
    pthread_mutex_destroy(&pool->back_mutex);
    pthread_cond_destroy(&pool->back_cond);
    pthread_mutex_destroy(&pool->sync_mutex);
    pthread_cond_destroy(&pool->sync_cond);

    free(pool);
}
