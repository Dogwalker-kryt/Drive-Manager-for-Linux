#include "drivefunctions.h"

class Logger {
public:
    static void log(const std::string& operation) {
        auto now = std::chrono::system_clock::now();
        std::time_t currentTime = std::chrono::system_clock::to_time_t(now);

        char timeStr[100];
        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", std::localtime(&currentTime));

        std::string logMsg = std::string("[") + timeStr + "] executed " + operation;
        
        std::string user = std::getenv("USER");
        std::string logDir = "/home/" + user + "/.var/app/DriveMgr/";
        std::string logPath = logDir + "log.dat";
        std::ofstream logFile(logPath, std::ios::app);
        if (logFile) {
            logFile << logMsg << std::endl;
        } else {
            std::cerr << "[Error] Unable to open log file: " << logPath << " Reason: " << strerror(errno) << std::endl;
        } 
    }
};

// Command executer
class Terminalexec {
public:
    static std::string execTerminal(const char* cmd) {
        std::array<char, 128> buffer;
        std::string result;
        auto deleter = [](FILE* f) { if (f) pclose(f); };
        std::unique_ptr<FILE, decltype(deleter)> pipe(popen(cmd, "r"), deleter);
        if (!pipe) throw std::runtime_error("popen() failed!");
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    }
    //v2
    static std::string execTerminalv2(const std::string &command) {
        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
        if (!pipe) return "";
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    }
    //v3
    static std::string execTerminalv3(const std::string& cmd) {
        std::array<char, 512> buffer;
        std::string result;
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            throw std::runtime_error("popen() failed");
        }
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            result += buffer.data();
        }
        int returnCode = pclose(pipe);
        if (returnCode != 0) {
            return ""; 
        }
        while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
            result.pop_back();
        }
        return result;
    }
};
