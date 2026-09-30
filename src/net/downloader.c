// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "downloader.h"
#include "http.h"
#include "../cli/ui.h"
#include "../i18n.h"

struct pool_state
{
    struct download_task *tasks;
    size_t count;
    size_t next;
    size_t required;
    size_t reported;
    pthread_mutex_t lock;
};

struct task_ctx
{
    struct pool_state *pool;
    struct download_task *task;
};

static void
refresh_progress(struct pool_state *pool)
{
    size_t now = 0;
    size_t total = 0;
    for (size_t i = 0; i < pool->count; i++)
    {
        now += pool->tasks[i].now;
        total += pool->tasks[i].total;
    }
    ui_progress_update(now, total);
}

static void
on_progress(size_t now, size_t total, void *userdata)
{
    struct task_ctx *ctx = userdata;
    pthread_mutex_lock(&ctx->pool->lock);
    ctx->task->now = now;
    if (total > 0)
        ctx->task->total = total;
    refresh_progress(ctx->pool);
    pthread_mutex_unlock(&ctx->pool->lock);
}

static void
report_finished(struct pool_state *pool, struct download_task *task)
{
    if (task->optional)
        return;

    pool->reported++;
    char label[64];
    snprintf(label, sizeof(label), "%zu/%zu", pool->reported, pool->required);
    if (task->ok)
    {
        struct stat st;
        char size[32] = "";
        if (stat(task->dest_path, &st) == 0)
            ui_format_bytes((size_t)st.st_size, size, sizeof(size));
        ui_status(_("Downloaded"), "%s%s%s%s", task->label ? task->label : "",
                  size[0] ? " (" : "", size, size[0] ? ")" : "");
    }
    ui_progress_set_label(label);
}

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
        struct download_task *task = &pool->tasks[pool->next++];
        pthread_mutex_unlock(&pool->lock);

        struct task_ctx ctx = {.pool = pool, .task = task};
        struct http_response resp = {0};
        task->ok =
            http_download(task->url, task->dest_path, on_progress, &ctx, &resp);
        if (!task->ok)
            snprintf(task->error, sizeof(task->error), "%s", http_last_error());

        pthread_mutex_lock(&pool->lock);
        report_finished(pool, task);
        pthread_mutex_unlock(&pool->lock);
    }
}

bool
download_all_parallel(struct download_task *tasks, size_t count,
                      int max_parallel)
{
    if (count == 0)
        return true;

    if (max_parallel < 1)
        max_parallel = 1;

    size_t workers =
        (size_t)max_parallel < count ? (size_t)max_parallel : count;

    struct pool_state pool = {
        .tasks = tasks,
        .count = count,
    };
    pthread_mutex_init(&pool.lock, NULL);

    for (size_t i = 0; i < count; i++)
        if (!tasks[i].optional)
            pool.required++;
    char label[64];
    snprintf(label, sizeof(label), "0/%zu", pool.required);
    ui_progress_start(_("Downloading"), label, 0, UI_PROGRESS_BYTES);

    pthread_t *threads = malloc(workers * sizeof(*threads));
    size_t started = 0;
    for (size_t i = 0; threads && i < workers; i++)
        if (pthread_create(&threads[started], NULL, worker, &pool) == 0)
            started++;
    if (started == 0)
        worker(&pool);
    for (size_t i = 0; i < started; i++)
        pthread_join(threads[i], NULL);
    free(threads);

    ui_progress_finish();
    pthread_mutex_destroy(&pool.lock);

    bool all_ok = true;
    for (size_t i = 0; i < count; i++)
        if (!tasks[i].ok && !tasks[i].optional)
            all_ok = false;
    return all_ok;
}
