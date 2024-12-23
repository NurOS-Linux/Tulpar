/**
 * @file config.h
 * @brief Tulpar Package Manager Configuration Paths
 * @author AnmiTaliDev
 * @date 2024-12-23 19:57:03 UTC
 */

// Системный конфиг
#define TULPAR_SYSTEM_CONFIG "/etc/tulpar/tulpar.conf"

// Пользовательский конфиг
#define TULPAR_USER_CONFIG "~/.config/tulpar/tulpar.conf"

// Основные директории
#define TULPAR_CONFIG_PATHS {"tulpar.conf", \
                            "~/.tulparrc", \
                            "~/.config/tulpar/tulpar.conf", \
                            "/etc/tulpar/tulpar.conf"}