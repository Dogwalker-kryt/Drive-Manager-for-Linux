
#ifndef DRIVEFUNK_H
#define DRIVEFUNK_H

#include <string>
#include <vector>

/**
 * @brief Lists all available drives.
 * @return Vector containing the names of the found drives.
 */
std::vector<std::string> listDrives();

/**
 * @brief Formats a drive.
 * @param driveName Name or path of the drive.
 * @param label (Optional) New label for the drive.
 * @param fsType (Optional) Filesystem type (e.g. ext4, ntfs).
 * @return true on success, false otherwise.
 */
bool formatDrive(const std::string& driveName, const std::string& label = "", const std::string& fsType = "");

/**
 * @brief Encrypts or decrypts a drive.
 * @param driveName Name or path of the drive.
 * @param encrypt true = encrypt, false = decrypt.
 * @return true on success, false otherwise.
 */
bool encryptDrive(const std::string& driveName, bool encrypt);

/**
 * @brief Resizes a drive.
 * @param driveName Name or path of the drive.
 * @param newSize New size in GB.
 * @return true on success, false otherwise.
 */
bool resizeDrive(const std::string& driveName, int newSize);

/**
 * @brief Checks the health of a drive.
 * @param driveName Name or path of the drive.
 * @return true if the drive is healthy, false otherwise.
 */
bool checkDriveHealth(const std::string& driveName);

#endif // DRIVEFUNK_H
