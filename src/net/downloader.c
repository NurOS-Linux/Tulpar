// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <pthread.h>
#include <stdlib.h>

#include "downloader.h"
#include "http.h"

struct pool_state
{
    struct download_task *tasks;
    size_t count;
    size_t next;
    pthread_mutex_t lock;
};

static void *
worker(void *arg)
{
    struct pool_state *pool = arg;

    for (;;)
    {
        pthread_mutex_lock(&pool->lock);
        if (pool->next >= pool->count)
        {
            pthread_mutex_unlock(&pool->lock);
            return NULL;
        }
        size_t idx = pool->next++;
        pthread_mutex_unlock(&pool->lock);

        struct http_response resp = {0};
        pool->tasks[idx].ok =
            http_download(pool->tasks[idx].url, pool->tasks[idx].dest_path,
                          NULL, NULL, &resp);
    }
}

void
download_all_parallel(struct download_task *tasks, size_t count,
                      int max_parallel)
{
    if (count == 0)
        return;

    if (max_parallel < 1)
        max_parallel = 1;

    size_t workers =
        (size_t)max_parallel < count ? (size_t)max_parallel : count;

    struct pool_state pool = {
        .tasks = tasks,
        .count = count,
        .next = 0,
    };
    pthread_mutex_init(&pool.lock, NULL);

    pthread_t *threads = malloc(workers * sizeof(*threads));
    if (!threads)
    {
        worker(&pool);
        pthread_mutex_destroy(&pool.lock);
        return;
    }

    for (size_t i = 0; i < workers; i++)
    {
        if (pthread_create(&threads[i], NULL, worker, &pool) != 0)
            threads[i] = 0;
    }

    for (size_t i = 0; i < workers; i++)
    {
        if (threads[i] != 0)
            pthread_join(threads[i], NULL);
    }

    free(threads);
    pthread_mutex_destroy(&pool.lock);
}
