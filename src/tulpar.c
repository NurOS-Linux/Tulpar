/**
 * @file tulpar_errors.h
 * @brief Error codes and descriptions for Tulpar package manager
 * @author AnmiTaliDev
 * @date 2024-12-21 15:30:15 UTC
 */

#ifndef TULPAR_ERRORS_H
#define TULPAR_ERRORS_H

// Error codes
typedef enum {
    // Success
    TULPAR_SUCCESS = 0,
    
    // System errors (-1 to -99)
    TULPAR_ERROR_INTERNAL = -1,        // Internal system error
    TULPAR_ERROR_MEMORY = -2,          // Memory allocation failed  
    TULPAR_ERROR_IO = -3,              // Input/output error
    TULPAR_ERROR_PERMISSION = -4,      // Permission denied
    TULPAR_ERROR_NOT_FOUND = -5,       // File or resource not found
    TULPAR_ERROR_ALREADY_EXISTS = -6,  // Resource already exists
    TULPAR_ERROR_TIMEOUT = -7,         // Operation timed out
    TULPAR_ERROR_BUSY = -8,            // Resource busy or locked
    
    // Package errors (-100 to -199) 
    TULPAR_ERROR_PACKAGE_NOT_FOUND = -100,    // Package not found
    TULPAR_ERROR_PACKAGE_CORRUPT = -101,      // Package file corrupted
    TULPAR_ERROR_PACKAGE_CONFLICT = -102,     // Package conflicts with installed
    TULPAR_ERROR_PACKAGE_DEPS = -103,         // Dependency resolution failed
    TULPAR_ERROR_PACKAGE_INSTALL = -104,      // Package installation failed
    TULPAR_ERROR_PACKAGE_REMOVE = -105,       // Package removal failed
    TULPAR_ERROR_PACKAGE_CONFIG = -106,       // Package configuration failed
    TULPAR_ERROR_PACKAGE_VERIFY = -107,       // Package verification failed
    
    // Network errors (-200 to -299)
    TULPAR_ERROR_NETWORK = -200,              // Generic network error
    TULPAR_ERROR_DOWNLOAD = -201,             // Download failed
    TULPAR_ERROR_CONNECT = -202,              // Connection failed
    TULPAR_ERROR_DNS = -203,                  // DNS resolution failed
    TULPAR_ERROR_PROXY = -204,                // Proxy error
    TULPAR_ERROR_SSL = -205,                  // SSL/TLS error
    
    // Database errors (-300 to -399)
    TULPAR_ERROR_DB = -300,                   // Generic database error
    TULPAR_ERROR_DB_CORRUPT = -301,           // Database corrupted
    TULPAR_ERROR_DB_VERSION = -302,           // Database version mismatch
    TULPAR_ERROR_DB_LOCKED = -303,            // Database locked
    TULPAR_ERROR_DB_QUERY = -304,             // Invalid query
    
    // Input validation (-400 to -499)
    TULPAR_ERROR_INVALID_ARG = -400,          // Invalid argument
    TULPAR_ERROR_INVALID_NAME = -401,         // Invalid package name
    TULPAR_ERROR_INVALID_VERSION = -402,      // Invalid version format
    TULPAR_ERROR_INVALID_CHECKSUM = -403,     // Invalid checksum
    TULPAR_ERROR_INVALID_SIGNATURE = -404,    // Invalid signature
    
    // Repository errors (-500 to -599)
    TULPAR_ERROR_REPO_ADD = -500,             // Failed to add repository
    TULPAR_ERROR_REPO_REMOVE = -501,          // Failed to remove repository
    TULPAR_ERROR_REPO_UPDATE = -502,          // Failed to update repository
    TULPAR_ERROR_REPO_SYNC = -503,            // Failed to sync repository
    TULPAR_ERROR_REPO_ACCESS = -504,          // Repository access denied
    
    // Configuration errors (-600 to -699)
    TULPAR_ERROR_CONFIG_READ = -600,          // Failed to read config
    TULPAR_ERROR_CONFIG_WRITE = -601,         // Failed to write config
    TULPAR_ERROR_CONFIG_PARSE = -602,         // Failed to parse config
    TULPAR_ERROR_CONFIG_INVALID = -603,       // Invalid configuration
    
    // Transaction errors (-700 to -799)
    TULPAR_ERROR_TRANS_BEGIN = -700,          // Failed to begin transaction
    TULPAR_ERROR_TRANS_COMMIT = -701,         // Failed to commit transaction
    TULPAR_ERROR_TRANS_ROLLBACK = -702,       // Failed to rollback transaction
    TULPAR_ERROR_TRANS_LOCK = -703,           // Transaction lock failed
    
    // System resource errors (-800 to -899)
    TULPAR_ERROR_DISK_SPACE = -800,           // Insufficient disk space
    TULPAR_ERROR_MEMORY_LOW = -801,           // Low memory condition
    TULPAR_ERROR_CPU_LOAD = -802,             // High CPU load
    TULPAR_ERROR_TEMP_SPACE = -803,           // No temporary space
    
    // Security errors (-900 to -999)
    TULPAR_ERROR_SECURITY = -900,             // Generic security error 
    TULPAR_ERROR_PRIVILEGE = -901,            // Insufficient privileges
    TULPAR_ERROR_AUTH = -902,                 // Authentication failed
    TULPAR_ERROR_CERT = -903,                 // Certificate error
    TULPAR_ERROR_SIGNATURE = -904             // Invalid signature
} TulparError;

// Get error description
const char* tulpar_error_string(TulparError error) {
    switch(error) {
        case TULPAR_SUCCESS: return "Success";
        
        // System errors
        case TULPAR_ERROR_INTERNAL: return "Internal system error";
        case TULPAR_ERROR_MEMORY: return "Memory allocation failed";
        case TULPAR_ERROR_IO: return "Input/output error";
        case TULPAR_ERROR_PERMISSION: return "Permission denied";
        case TULPAR_ERROR_NOT_FOUND: return "File or resource not found";
        case TULPAR_ERROR_ALREADY_EXISTS: return "Resource already exists";
        case TULPAR_ERROR_TIMEOUT: return "Operation timed out";
        case TULPAR_ERROR_BUSY: return "Resource busy or locked";
        
        // Package errors
        case TULPAR_ERROR_PACKAGE_NOT_FOUND: return "Package not found";
        case TULPAR_ERROR_PACKAGE_CORRUPT: return "Package file corrupted";
        case TULPAR_ERROR_PACKAGE_CONFLICT: return "Package conflicts with installed packages";
        case TULPAR_ERROR_PACKAGE_DEPS: return "Dependency resolution failed";
        case TULPAR_ERROR_PACKAGE_INSTALL: return "Package installation failed";
        case TULPAR_ERROR_PACKAGE_REMOVE: return "Package removal failed";
        case TULPAR_ERROR_PACKAGE_CONFIG: return "Package configuration failed";
        case TULPAR_ERROR_PACKAGE_VERIFY: return "Package verification failed";
        
        // Network errors
        case TULPAR_ERROR_NETWORK: return "Network error";
        case TULPAR_ERROR_DOWNLOAD: return "Download failed";
        case TULPAR_ERROR_CONNECT: return "Connection failed";
        case TULPAR_ERROR_DNS: return "DNS resolution failed";
        case TULPAR_ERROR_PROXY: return "Proxy error";
        case TULPAR_ERROR_SSL: return "SSL/TLS error";
        
        // Add other error descriptions...
        
        default: return "Unknown error";
    }
}

#endif // TULPAR_ERRORS_H