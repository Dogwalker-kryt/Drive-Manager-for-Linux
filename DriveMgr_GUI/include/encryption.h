#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <string>
#include <vector>
#include <functional>

struct EncryptionProgress {
    double progress;          // 0.0 to 1.0
    bool isPaused;
    bool isCancelled;
};

class EncryptionManager {
public:
    static EncryptionManager& getInstance() {
        static EncryptionManager instance;
        return instance;
    }

    bool encryptDrive(const std::string& device, const std::string& password, 
                     std::function<void(double)> progress_callback = nullptr);
                     
    bool decryptDrive(const std::string& device, const std::string& password,
                     std::function<void(double)> progress_callback = nullptr);
                     
    bool isEncrypted(const std::string& device);
    
    void pauseOperation();
    void resumeOperation();
    void cancelOperation();
    
private:
    EncryptionManager() = default;
    EncryptionProgress progress_;
    std::string current_device_;
    bool is_operating_ = false;
};

#endif // ENCRYPTION_H
