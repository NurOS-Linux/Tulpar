/**
 * @file tulpard.c
 * @brief Tulpar Package Manager Daemon
 * @author AnmiTaliDev 
 * @date 2024-12-23 20:07:27 UTC
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <sys/stat.h>
#include <systemd/sd-daemon.h>
#include <sqlite3.h>
#include <curl/curl.h>
#include <pthread.h>
#include "tulpar.h"
#include "config.h"

#define TULPAR_PID "/run/tulpard.pid"
#define MAX_THREADS 4

typedef struct {
    int shutdown;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    sqlite3 *db;
    CURLM *curl;
    Config *config;
} TulparDaemon;

static TulparDaemon daemon_ctx = {0};

// Обработчик сигналов
static void signal_handler(int sig) {
    switch(sig) {
        case SIGTERM:
        case SIGINT:
            syslog(LOG_NOTICE, "Received shutdown signal");
            daemon_ctx.shutdown = 1;
            pthread_cond_broadcast(&daemon_ctx.cond);
            break;
            
        case SIGHUP:
            syslog(LOG_NOTICE, "Reloading configuration");
            if (daemon_ctx.config) {
                daemon_ctx.config->reload();
            }
            break;
    }
}

// Фоновая задача обновления кэша
static void* cache_update_thread(void *arg) {
    while (!daemon_ctx.shutdown) {
        pthread_mutex_lock(&daemon_ctx.lock);
        
        // Проверка и очистка старых файлов в кэше
        const char *cache_dir = daemon_ctx.config->get("storage.cache_dir");
        size_t cache_limit = atol(daemon_ctx.config->get("storage.cache_limit"));
        
        // TODO: Реализовать очистку кэша
        
        pthread_mutex_unlock(&daemon_ctx.lock);
        
        // Ждем следующей итерации или сигнала
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 3600; // Каждый час
        
        pthread_mutex_lock(&daemon_ctx.lock);
        pthread_cond_timedwait(&daemon_ctx.cond, &daemon_ctx.lock, &ts);
        pthread_mutex_unlock(&daemon_ctx.lock);
    }
    
    return NULL;
}

// Фоновая задача обновления БД
static void* db_update_thread(void *arg) {
    while (!daemon_ctx.shutdown) {
        pthread_mutex_lock(&daemon_ctx.lock);
        
        // Обновление информации о пакетах
        const char *repo_url = daemon_ctx.config->get("repo.main");
        
        // TODO: Реализовать обновление БД
        
        pthread_mutex_unlock(&daemon_ctx.lock);
        
        // Ждем следующей итерации или сигнала 
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 1800; // Каждые 30 минут
        
        pthread_mutex_lock(&daemon_ctx.lock);
        pthread_cond_timedwait(&daemon_ctx.cond, &daemon_ctx.lock, &ts);
        pthread_mutex_unlock(&daemon_ctx.lock);
    }
    
    return NULL;
}

// Основная функция демона
static int daemon_main(void) {
    // Инициализация логов
    openlog("tulpard", LOG_PID, LOG_DAEMON);
    syslog(LOG_NOTICE, "Tulpar daemon starting...");

    // Инициализация мьютексов
    pthread_mutex_init(&daemon_ctx.lock, NULL);
    pthread_cond_init(&daemon_ctx.cond, NULL);

    // Загрузка конфигурации
    daemon_ctx.config = Config::getInstance();
    if (!daemon_ctx.config->init()) {
        syslog(LOG_ERR, "Failed to load configuration");
        return 1;
    }

    // Инициализация БД
    const char *db_path = daemon_ctx.config->get("storage.db_path");
    if (sqlite3_open(db_path, &daemon_ctx.db)) {
        syslog(LOG_ERR, "Failed to open database");
        return 1;
    }

    // Инициализация CURL
    curl_global_init(CURL_GLOBAL_DEFAULT);
    daemon_ctx.curl = curl_multi_init();
    if (!daemon_ctx.curl) {
        syslog(LOG_ERR, "Failed to initialize CURL");
        return 1;
    }

    // Установка обработчиков сигналов
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGHUP, signal_handler);

    // Создание фоновых потоков
    pthread_t cache_thread, db_thread;
    pthread_create(&cache_thread, NULL, cache_update_thread, NULL);
    pthread_create(&db_thread, NULL, db_update_thread, NULL);

    // Уведомляем systemd о готовности
    sd_notify(0, "READY=1");
    syslog(LOG_NOTICE, "Tulpar daemon started successfully");

    // Основной цикл
    while (!daemon_ctx.shutdown) {
        // Обработка запросов от клиентов
        
        // Отправка watchdog
        sd_notify(0, "WATCHDOG=1");
        
        sleep(1);
    }

    // Завершение работы
    syslog(LOG_NOTICE, "Tulpar daemon shutting down...");

    // Ожидание завершения потоков
    pthread_join(cache_thread, NULL);
    pthread_join(db_thread, NULL);

    // Очистка ресурсов
    if (daemon_ctx.db) {
        sqlite3_close(daemon_ctx.db);
    }
    
    if (daemon_ctx.curl) {
        curl_multi_cleanup(daemon_ctx.curl);
        curl_global_cleanup();
    }

    pthread_mutex_destroy(&daemon_ctx.lock);
    pthread_cond_destroy(&daemon_ctx.cond);

    closelog();
    return 0;
}

int main(int argc, char *argv[]) {
    // Разбор аргументов
    int opt;
    const char *config_path = "/etc/tulpar/tulpar.conf";
    
    while ((opt = getopt(argc, argv, "c:")) != -1) {
        switch (opt) {
            case 'c':
                config_path = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s [-c config]\n", argv[0]);
                return 1;
        }
    }

    // Демонизация процесса
    pid_t pid = fork();
    if (pid < 0) {
        return 1;
    }
    
    if (pid > 0) {
        return 0;
    }

    // Создаем новую сессию
    if (setsid() < 0) {
        return 1;
    }

    // Закрываем стандартные потоки
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Записываем PID
    FILE *pidfile = fopen(TULPAR_PID, "w");
    if (pidfile) {
        fprintf(pidfile, "%d\n", getpid());
        fclose(pidfile);
    }

    // Запускаем основной код демона
    return daemon_main();
}