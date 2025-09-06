#include "logger.h"
#include <windows.h>
#include <shlobj.h>

Logger::Logger() {
}

Logger::~Logger() {
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
}

void Logger::Initialize(bool enabled, const std::wstring& logPath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_enabled = enabled;
    
    if (!enabled) {
        if (m_logFile.is_open()) {
            m_logFile.close();
        }
        return;
    }
    
    // Determine log file path
    if (!logPath.empty()) {
        m_logPath = logPath;
    } else {
        m_logPath = GetDefaultLogPath();
    }
    
    // Convert wide string to narrow string for file operations
    std::string narrowPath;
    narrowPath.reserve(m_logPath.size());
    for (wchar_t wc : m_logPath) {
        narrowPath.push_back(static_cast<char>(wc)); // Simple conversion for ASCII paths
    }
    
    // Open log file in append mode
    m_logFile.open(narrowPath, std::ios::app);
    
    if (m_logFile.is_open()) {
        // Write startup message
        m_logFile << "\n===== Matrix Screensaver Started =====" << std::endl;
        m_logFile << GetTimestamp() << " [INFO] Logging initialized" << std::endl;
    }
}

void Logger::SetEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_enabled == enabled) return;
    
    m_enabled = enabled;
    
    if (!enabled && m_logFile.is_open()) {
        m_logFile << GetTimestamp() << " [INFO] Logging disabled" << std::endl;
        m_logFile.close();
    } else if (enabled && !m_logFile.is_open()) {
        Initialize(true, m_logPath);
    }
}

void Logger::Log(LogLevel level, const std::string& message) {
    if (!m_enabled) return;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_logFile.is_open()) {
        m_logFile << GetTimestamp() << " " << GetLevelString(level) << " " << message << std::endl;
        
        // Auto-flush for errors and warnings
        if (level >= LogLevel::Warning) {
            m_logFile.flush();
        }
    }
}

void Logger::Flush() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_logFile.is_open()) {
        m_logFile.flush();
    }
}

std::string Logger::GetTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    struct tm localTime;
    if (localtime_s(&localTime, &time_t) != 0) {
        return "[TIMESTAMP_ERROR]";
    }
    
    std::stringstream ss;
    ss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return ss.str();
}

std::string Logger::GetLevelString(LogLevel level) const {
    switch (level) {
        case LogLevel::Debug:   return "[DEBUG]";
        case LogLevel::Info:    return "[INFO] ";
        case LogLevel::Warning: return "[WARN] ";
        case LogLevel::Error:   return "[ERROR]";
        default:                return "[UNKN] ";
    }
}

std::wstring Logger::GetDefaultLogPath() const {
    wchar_t path[MAX_PATH];
    
    // Get AppData\Local folder
    if (SUCCEEDED(SHGetFolderPath(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, path))) {
        std::wstring logPath = path;
        logPath += L"\\MatrixScreensaver";
        
        // Create directory if it doesn't exist
        CreateDirectory(logPath.c_str(), nullptr);
        
        // Add log file name with timestamp
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        struct tm localTime;
        if (localtime_s(&localTime, &time_t) == 0) {
            std::wstringstream ss;
            ss << logPath << L"\\matrix_" 
               << std::put_time(&localTime, L"%Y%m%d")
               << L".log";
            return ss.str();
        }
        
        // Fallback if localtime_s fails
        return logPath + L"\\matrix_screensaver.log";
    }
    
    // Fallback to current directory
    return L"matrix_screensaver.log";
}