/**
 * @file tulpar_cpp.cpp 
 * @brief C++ wrapper for Tulpar Package Manager
 * @author AnmiTaliDev
 * @date 2024-12-23 19:43:42 UTC
 */

#include <string>
#include <stdexcept>
#include <memory>
#include <vector>
#include <mutex>
#include "tulpar.h"

namespace tulpar {

// Исключение для ошибок Tulpar
class Exception : public std::runtime_error {
public:
    explicit Exception(TulparError err) : 
        std::runtime_error("Tulpar error"), code_(err) {}
    
    TulparError code() const { return code_; }
private:
    TulparError code_;
};

// Класс для работы с пакетом
class Package {
public:
    Package(const std::string& name, const std::string& version = "") :
        name_(name), version_(version) {}
    
    const std::string& name() const { return name_; }
    const std::string& version() const { return version_; }
    
    bool operator==(const Package& other) const {
        return name_ == other.name_;
    }

private:
    std::string name_;
    std::string version_;
};

// Главный класс для работы с менеджером пакетов
class Manager {
public:
    Manager() {
        TulparError err = tulpar_init();
        if (err != TULPAR_OK) {
            throw Exception(err);
        }
    }

    ~Manager() {
        tulpar_cleanup();
    }

    // Установка пакета
    void install(const Package& pkg, bool force = false) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        TulparOptions opts = {0};
        opts.force = force;
        
        TulparError err = tulpar_install(pkg.name().c_str(), &opts);
        if (err != TULPAR_OK) {
            throw Exception(err);
        }
        
        installed_.push_back(pkg);
    }

    // Удаление пакета
    void remove(const Package& pkg) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        TulparError err = tulpar_remove(pkg.name().c_str());
        if (err != TULPAR_OK) {
            throw Exception(err);
        }

        auto it = std::find(installed_.begin(), installed_.end(), pkg);
        if (it != installed_.end()) {
            installed_.erase(it);
        }
    }

    // Проверка установки пакета
    bool is_installed(const Package& pkg) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return std::find(installed_.begin(), installed_.end(), pkg) != installed_.end();
    }

    // Получение списка установленных пакетов
    std::vector<Package> get_installed() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return installed_;
    }

    // Обновление пакета
    void update(const Package& pkg) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!is_installed(pkg)) {
            throw Exception(TULPAR_ERR_INVALID);
        }

        TulparOptions opts = {0};
        opts.force = true;
        
        TulparError err = tulpar_install(pkg.name().c_str(), &opts);
        if (err != TULPAR_OK) {
            throw Exception(err);
        }
    }

    // Очистка кэша
    void clear_cache() {
        std::lock_guard<std::mutex> lock(mutex_);
        // TODO: Реализовать очистку кэша
    }

private:
    std::vector<Package> installed_;
    mutable std::mutex mutex_;

    // Запрет копирования и присваивания
    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
};

} // namespace tulpar

// Пример использования:
/*
int main() {
    try {
        tulpar::Manager mgr;
        
        // Установка пакета
        tulpar::Package pkg("nginx");
        mgr.install(pkg);
        
        // Проверка установки
        if (mgr.is_installed(pkg)) {
            std::cout << "nginx installed" << std::endl;
        }
        
        // Обновление
        mgr.update(pkg);
        
        // Удаление
        mgr.remove(pkg);
        
    } catch (const tulpar::Exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
*/