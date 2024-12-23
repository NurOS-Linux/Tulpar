/**
 * @file config.cpp
 * @brief Tulpar Package Manager Configuration Manager
 * @author AnmiTaliDev
 * @date 2024-12-23 19:58:56 UTC
 */

#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>
#include "config.hpp"

namespace tulpar {

class Config {
private:
    std::map<std::string, std::string> settings;
    std::vector<std::string> config_paths = {
        "tulpar.conf",
        std::string(getenv("HOME")) + "/.tulparrc",
        std::string(getenv("HOME")) + "/.config/tulpar/tulpar.conf", 
        "/etc/tulpar/tulpar.conf"
    };

    static Config* instance;
    bool initialized = false;

    // Конструктор приватный для синглтона
    Config() {}

public:
    static Config* getInstance() {
        if (instance == nullptr) {
            instance = new Config();
        }
        return instance;
    }

    // Инициализация конфигурации
    bool init() {
        if (initialized) return true;

        // Создаем директории если не существуют
        std::filesystem::create_directories("/etc/tulpar");
        std::filesystem::create_directories(
            std::string(getenv("HOME")) + "/.config/tulpar"
        );

        // Устанавливаем значения по умолчанию
        settings = {
            {"repo.main", "https://repo.tulpar.org/packages"},
            {"repo.testing", "https://testing.tulpar.org/packages"},
            {"network.timeout", "30"},
            {"network.max_downloads", "4"},
            {"network.retries", "3"},
            {"storage.cache_dir", "/var/cache/tulpar"},
            {"storage.db_path", "/var/lib/tulpar/packages.db"},
            {"storage.log_file", "/var/log/tulpar/tulpar.log"},
            {"storage.cache_limit", "1024"},
            {"security.verify_signatures", "true"},
            {"security.keyring", "/etc/tulpar/keyring.gpg"},
            {"security.allow_untrusted", "false"},
            {"defaults.force", "false"},
            {"defaults.yes", "false"},
            {"defaults.verbose", "false"},
            {"logging.level", "info"},
            {"logging.format", "%datetime% [%level%] %message%"},
            {"logging.use_syslog", "true"},
            {"logging.debug", "false"}
        };

        // Загружаем конфиги в порядке приоритета
        bool found = false;
        for (const auto& path : config_paths) {
            if (loadConfig(path)) {
                found = true;
                break;
            }
        }

        if (!found) {
            // Создаем дефолтный конфиг если не найден
            createDefaultConfig();
        }

        initialized = true;
        return true;
    }

    // Загрузка конфига из файла
    bool loadConfig(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) return false;

        std::string line;
        std::string section;

        while (std::getline(file, line)) {
            // Пропускаем комментарии и пустые строки
            if (line.empty() || line[0] == '#') continue;

            // Обработка секции
            if (line[0] == '[' && line.back() == ']') {
                section = line.substr(1, line.size() - 2);
                continue;
            }

            // Разбор параметров
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);

                // Удаляем пробелы
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);

                if (!section.empty()) {
                    key = section + "." + key;
                }

                settings[key] = value;
            }
        }

        return true;
    }

    // Создание дефолтного конфига
    void createDefaultConfig() {
        std::ofstream file("/etc/tulpar/tulpar.conf");
        if (!file.is_open()) return;

        file << "# Tulpar Package Manager Configuration\n\n";

        std::string current_section;
        for (const auto& [key, value] : settings) {
            size_t dot_pos = key.find('.');
            if (dot_pos != std::string::npos) {
                std::string section = key.substr(0, dot_pos);
                if (section != current_section) {
                    file << "\n[" << section << "]\n";
                    current_section = section;
                }
                file << key.substr(dot_pos + 1) << " = " << value << "\n";
            }
        }
    }

    // Получение значения параметра
    std::string get(const std::string& key) const {
        auto it = settings.find(key);
        return (it != settings.end()) ? it->second : "";
    }

    // Установка значения параметра
    void set(const std::string& key, const std::string& value) {
        settings[key] = value;
    }

    // Сохранение текущей конфигурации
    bool save() {
        std::ofstream file("/etc/tulpar/tulpar.conf");
        if (!file.is_open()) return false;

        file << "# Tulpar Package Manager Configuration\n";
        file << "# Generated: " << std::time(nullptr) << "\n\n";

        std::string current_section;
        for (const auto& [key, value] : settings) {
            size_t dot_pos = key.find('.');
            if (dot_pos != std::string::npos) {
                std::string section = key.substr(0, dot_pos);
                if (section != current_section) {
                    file << "\n[" << section << "]\n";
                    current_section = section;
                }
                file << key.substr(dot_pos + 1) << " = " << value << "\n";
            }
        }

        return true;
    }

    ~Config() {
        save();
    }
};

// Инициализация статического члена
Config* Config::instance = nullptr;

} // namespace tulpar

// Быстрая обнова в честь моего ДР:
/*
}
*/