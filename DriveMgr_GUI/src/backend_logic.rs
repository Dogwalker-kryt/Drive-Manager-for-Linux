// backend.rs
use std::process::Command;
use std::io::{Error, ErrorKind, Read, Write};
use std::path::PathBuf;
use std::env;
use std::fs::{self, File, OpenOptions};
use chrono::Local;
use regex::Regex;

// --- Logger ---
pub struct Logger;
impl Logger {
    pub fn log(operation: &str) {
        let now = Local::now();
        let time_str = now.format("%Y-%m-%d %H:%M:%S").to_string();
        let log_msg = format!("[{}] executed {}", time_str, operation);
        let mut log_dir = PathBuf::from(env::var("HOME").unwrap_or_else(|_| ".".into()));
        log_dir.push(".var/app/DriveMgr");
        let log_path = log_dir.join("log.dat");
        if let Some(parent) = log_path.parent() {
            if !parent.exists() {
                if let Err(e) = fs::create_dir_all(parent) {
                    eprintln!("[Error] Unable to create log directory: {} Reason: {}", parent.display(), e);
                    return;
                }
            }
        }
        let mut log_file = match OpenOptions::new()
            .create(true)
            .append(true)
            .open(&log_path)
        {
            Ok(file) => file,
            Err(e) => {
                eprintln!("[Error] Unable to open log file: {} Reason: {}", log_path.display(), e);
                return;
            }
        };
        if let Err(e) = writeln!(log_file, "{}", log_msg) {
            eprintln!("[Error] Unable to write to log file: {} Reason: {}", log_path.display(), e);
        }
    }
}

// --- Terminal Exec ---
pub struct TerminalExec;
impl TerminalExec {
    pub fn exec(cmd: &str) -> Result<String, Error> {
        let output = Command::new("sh")
            .arg("-c")
            .arg(cmd)
            .output()?;
        if !output.status.success() {
            return Err(Error::new(
                ErrorKind::Other,
                format!("Command failed: {}", String::from_utf8_lossy(&output.stderr)),
            ));
        }
        Ok(String::from_utf8_lossy(&output.stdout).to_string())
    }
}

// --- Drive Metadata ---
#[derive(Debug, Default, Clone)]
pub struct DriveMetadata {
    pub name: String,
    pub size: String,
    pub model: String,
    pub serial: String,
    pub drive_type: String,
    pub mountpoint: String,
    pub vendor: String,
    pub fstype: String,
    pub uuid: String,
}

impl DriveMetadata {
    pub fn from_lsblk_json(json: &str) -> Self {
        let mut metadata = DriveMetadata::default();
        let re = Regex::new(r#""([^"]+)":\s*"([^"]*)""#).unwrap();
        for cap in re.captures_iter(json) {
            let key = &cap[1];
            let value = &cap[2];
            match key {
                "NAME" => metadata.name = value.to_string(),
                "SIZE" => metadata.size = value.to_string(),
                "MODEL" => metadata.model = value.to_string(),
                "SERIAL" => metadata.serial = value.to_string(),
                "TYPE" => metadata.drive_type = value.to_string(),
                "MOUNTPOINT" => metadata.mountpoint = value.to_string(),
                "VENDOR" => metadata.vendor = value.to_string(),
                "FSTYPE" => metadata.fstype = value.to_string(),
                "UUID" => metadata.uuid = value.to_string(),
                _ => (),
            }
        }
        metadata
    }
}

// --- Drive Functions ---
pub fn list_drives() -> Result<Vec<String>, Error> {
    let output = TerminalExec::exec("lsblk -o NAME,SIZE,TYPE,MOUNTPOINT,FSTYPE -d -n -p")?;
    let drives = output
        .lines()
        .filter(|line| line.contains("disk"))
        .filter_map(|line| line.split_whitespace().next().map(|s| s.to_string()))
        .collect();
    Ok(drives)
}

pub fn get_metadata(drive: &str) -> Result<DriveMetadata, Error> {
    let json = TerminalExec::exec(&format!("lsblk -J -o NAME,SIZE,MODEL,SERIAL,TYPE,MOUNTPOINT,VENDOR,FSTYPE,UUID -p {}", drive))?;
    let metadata = DriveMetadata::from_lsblk_json(&json);
    Ok(metadata)
}

pub fn check_drive_health(drive: &str) -> Result<String, Error> {
    TerminalExec::exec(&format!("smartctl -H {}", drive))
}

pub fn format_drive(drive: &str, fs_type: &str, label: &str) -> Result<String, Error> {
    TerminalExec::exec(&format!("mkfs.{} -L {} {}", fs_type, label, drive))
}

pub fn resize_drive(drive: &str, new_size: u64) -> Result<String, Error> {
    TerminalExec::exec(&format!("parted --script {} resizepart 1 {}MB", drive, new_size))
}

pub fn encrypt_drive(drive: &str, key: &str) -> Result<String, Error> {
    TerminalExec::exec(&format!(
        "cryptsetup -v --cipher aes-cbc-essiv:sha256 --key-size 256 --key-file <(echo -n '{}') open {} encrypted_{}",
        key,
        drive,
        drive.split('/').last().unwrap()
    ))
}

pub fn decrypt_drive(drive: &str, key: &str) -> Result<String, Error> {
    TerminalExec::exec(&format!(
        "cryptsetup -v --cipher aes-cbc-essiv:sha256 --key-size 256 --key-file <(echo -n '{}') open {} decrypted_{}",
        key,
        drive,
        drive.split('/').last().unwrap()
    ))
}

pub fn overwrite_drive(drive: &str) -> Result<String, Error> {
    TerminalExec::exec(&format!("sudo dd if=/dev/zero of={} bs=1M status=progress && sync", drive))
}

pub fn mount_drive(drive: &str, mountpoint: &str) -> Result<String, Error> {
    TerminalExec::exec(&format!("sudo mount {} {}", drive, mountpoint))
}

pub fn unmount_drive(drive: &str) -> Result<String, Error> {
    TerminalExec::exec(&format!("sudo umount {}", drive))
}

pub fn create_disk_image(drive: &str, image_path: &str) -> Result<String, Error> {
    TerminalExec::exec(&format!("sudo dd if={} of={} bs=4M status=progress && sync", drive, image_path))
}

pub fn list_partitions(drive: &str) -> Result<Vec<String>, Error> {
    let output = TerminalExec::exec(&format!("lsblk --ascii -o NAME,SIZE,TYPE,MOUNTPOINT,FSTYPE -n -p {}", drive))?;
    let partitions = output
        .lines()
        .filter(|line| line.contains("part"))
        .filter_map(|line| line.split_whitespace().next().map(|s| s.to_string()))
        .collect();
    Ok(partitions)
}

pub fn check_filesystem(drive: &str, fstype: &str) -> Result<String, Error> {
    let cmd = match fstype {
        "ext4" | "ext3" | "ext2" => format!("e2fsck -n {} 2>&1", drive),
        "ntfs" => format!("ntfsfix --no-action {} 2>&1", drive),
        "vfat" | "fat32" => format!("dosfsck -n {} 2>&1", drive),
        _ => return Ok("Unknown filesystem".to_string()),
    };
    TerminalExec::exec(&cmd)
}

// --- Encryption Info ---
#[derive(Debug, Default)]
pub struct EncryptionInfo {
    pub drive_name: String,
    pub key: [u8; 32],
    pub iv: [u8; 16],
}

impl EncryptionInfo {
    pub fn save(&self) -> Result<(), Error> {
        let mut path = PathBuf::from(env::var("HOME").unwrap_or_else(|_| ".".into()));
        path.push(".var/app/DriveMgr/keys.dat");
        let mut file = File::create(path)?;
        file.write_all(&self.drive_name.as_bytes())?;
        file.write_all(&self.key)?;
        file.write_all(&self.iv)?;
        Ok(())
    }

    pub fn load(drive_name: &str) -> Result<Self, Error> {
        let mut path = PathBuf::from(env::var("HOME").unwrap_or_else(|_| ".".into()));
        path.push(".var/app/DriveMgr/keys.dat");
        let mut file = File::open(path)?;
        let mut drive_name_buf = [0u8; 256];
        file.read_exact(&mut drive_name_buf)?;
        let drive_name_str = String::from_utf8_lossy(&drive_name_buf).trim_end_matches('\0').to_string();
        if drive_name_str != drive_name {
            return Err(Error::new(ErrorKind::NotFound, "Drive not found"));
        }
        let mut key = [0u8; 32];
        let mut iv = [0u8; 16];
        file.read_exact(&mut key)?;
        file.read_exact(&mut iv)?;
        Ok(EncryptionInfo { drive_name: drive_name_str, key, iv })
    }
}
