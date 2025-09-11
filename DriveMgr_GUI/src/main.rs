use eframe::egui;
use std::env;
use std::fs;
use std::io::Error;
use std::path::PathBuf;

mod backend_logic;
use backend_logic::*;

fn load_log_file() -> String {
    let log_path = PathBuf::from(env::var("HOME").unwrap_or_else(|_| ".".into()))
        .join(".var/app/DriveMgr/log.dat");
    std::fs::read_to_string(log_path).unwrap_or_else(|_| "No log file found.".to_string())
}

#[derive(Default)]
pub struct DriveManager {
    pub show_sidebar: bool,
    pub current_function: Option<String>,
    pub logger_content: String,
    pub function_output: String,
    pub drives: Vec<String>,
    pub selected_drive: Option<String>,
    pub drive_input: String,
    pub fs_type: String,
    pub label: String,
    pub new_size: String,
    pub mountpoint: String,
    pub image_path: String,
    pub encryption_key: String,
}

impl DriveManager {
    fn list_drives(&mut self) {
        self.current_function = Some("List drives".to_owned());
        Logger::log("List drives");
        let cmd = "lsblk -o NAME,SIZE,TYPE,MOUNTPOINT,FSTYPE -d -n -p";
        match TerminalExec::exec(cmd) {
            Ok(lsblk_out) => {
                self.drives = lsblk_out
                    .lines()
                    .filter(|line| line.contains("disk"))
                    .filter_map(|line| line.split_whitespace().next().map(String::from))
                    .collect();
                
                let mut output = String::new();
                output.push_str("Connected Drives:\n\n");
                for (i, drive) in self.drives.iter().enumerate() {
                    let mut parts = lsblk_out
                        .lines()
                        .find(|line| line.starts_with(drive))
                        .unwrap_or("")
                        .split_whitespace();
                    let device = parts.next().unwrap_or("");
                    let size = parts.next().unwrap_or("");
                    let _type = parts.next().unwrap_or("");
                    let mount = parts.next().unwrap_or("");
                    let fstype = parts.next().unwrap_or("");
                    output.push_str(&format!(
                        "{}. {} ({}, {}, Mount: {}, FS: {})\n",
                        i + 1, device, size, _type, mount, fstype
                    ));
                }
                self.function_output = output;
            }
            Err(e) => {
                self.function_output = format!("Error listing drives: {}", e);
            }
        }
    }

    fn format_drive(&mut self) {
        if let Some(ref drive) = self.selected_drive {
            if self.fs_type.is_empty() || self.label.is_empty() {
                self.function_output = "Please enter filesystem type and label".to_string();
                return;
            }
            let cmd = format!("sudo mkfs.{} -L {} {}", self.fs_type, self.label, drive);
            match TerminalExec::exec(&cmd) {
                Ok(output) => self.function_output = output,
                Err(e) => self.function_output = format!("Error formatting drive: {}", e),
            }
        } else {
            self.function_output = "No drive selected".to_string();
        }
    }

    fn encrypt_decrypt_drive(&mut self) {
        if let Some(ref drive) = self.selected_drive {
            if self.encryption_key.is_empty() {
                self.function_output = "Please enter encryption key".to_string();
                return;
            }
            let cmd = format!(
                "cryptsetup -v --cipher aes-cbc-essiv:sha256 --key-size 256 --key-file <(echo -n '{}') open {} encrypted_{}",
                self.encryption_key,
                drive,
                drive.split('/').last().unwrap_or("")
            );
            match TerminalExec::exec(&cmd) {
                Ok(output) => self.function_output = output,
                Err(e) => self.function_output = format!("Error encrypting drive: {}", e),
            }
        } else {
            self.function_output = "No drive selected".to_string();
        }
    }

    fn resize_drive(&mut self) {
        if let Some(ref drive) = self.selected_drive {
            if let Ok(size) = self.new_size.parse::<u64>() {
                let cmd = format!("parted --script {} resizepart 1 {}MB", drive, size);
                match TerminalExec::exec(&cmd) {
                    Ok(output) => self.function_output = output,
                    Err(e) => self.function_output = format!("Error resizing drive: {}", e),
                }
            } else {
                self.function_output = "Invalid size value".to_string();
            }
        } else {
            self.function_output = "No drive selected".to_string();
        }
    }

    fn check_drive_health(&mut self) {
        if let Some(ref drive) = self.selected_drive {
            let cmd = format!("smartctl -H {}", drive);
            match TerminalExec::exec(&cmd) {
                Ok(output) => self.function_output = output,
                Err(e) => self.function_output = format!("Error checking drive health: {}", e),
            }
        } else {
            self.function_output = "No drive selected".to_string();
        }
    }

    fn analyze_disk_space(&mut self) {
        match TerminalExec::exec("df -h") {
            Ok(output) => self.function_output = output,
            Err(e) => self.function_output = format!("Error analyzing disk space: {}", e),
        }
    }

    fn overwrite_drive_data(&mut self) {
        if let Some(ref drive) = self.selected_drive {
            let cmd = format!("sudo dd if=/dev/zero of={} bs=1M status=progress && sync", drive);
            match TerminalExec::exec(&cmd) {
                Ok(output) => self.function_output = output,
                Err(e) => self.function_output = format!("Error overwriting drive: {}", e),
            }
        } else {
            self.function_output = "No drive selected".to_string();
        }
    }

    fn view_metadata(&mut self) {
        if let Some(ref drive) = self.selected_drive {
            let cmd = format!("hdparm -I {}", drive);
            match TerminalExec::exec(&cmd) {
                Ok(metadata) => {
                    self.function_output = format!("Drive Metadata for {}:\n\n{}", drive, metadata);
                }
                Err(e) => {
                    self.function_output = format!("Error getting metadata: {}", e);
                }
            }
        } else {
            self.function_output = "No drive selected".to_string();
        }
    }

    fn mount_unmount(&mut self) {
        if let Some(ref drive) = self.selected_drive {
            if !self.mountpoint.is_empty() {
                let cmd = format!("sudo mount {} {}", drive, self.mountpoint);
                match TerminalExec::exec(&cmd) {
                    Ok(output) => self.function_output = output,
                    Err(e) => self.function_output = format!("Error mounting drive: {}", e),
                }
            } else {
                let cmd = format!("sudo umount {}", drive);
                match TerminalExec::exec(&cmd) {
                    Ok(output) => self.function_output = output,
                    Err(e) => self.function_output = format!("Error unmounting drive: {}", e),
                }
            }
        } else {
            self.function_output = "No drive selected".to_string();
        }
    }

    fn forensic_analysis(&mut self) {
        if let Some(ref drive) = self.selected_drive {
            self.function_output = format!("Forensic analysis for {} (Coming soon)", drive);
        } else {
            self.function_output = "No drive selected".to_string();
        }
    }

    fn diskspace_visualizer(&mut self) {
        self.analyze_disk_space();
    }

    fn clear_current_function(&mut self) {
        self.current_function = None;
        self.function_output.clear();
    }

    fn set_static_output(&mut self, name: &str, msg: &str) {
        self.current_function = Some(name.to_string());
        Logger::log(name);
        self.function_output = msg.to_string();
    }
}

impl eframe::App for DriveManager {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        egui::TopBottomPanel::top("top_panel").show(ctx, |ui| {
            ui.horizontal_wrapped(|ui| {
                if ui.button("≡").clicked() {
                    self.show_sidebar = !self.show_sidebar;
                }
                ui.heading("Drive Manager");
            });
        });

        if self.show_sidebar {
            egui::SidePanel::left("side_panel")
                .resizable(false)
                .default_width(200.0)
                .show(ctx, |ui| {
                    ui.heading("Menu");
                    if ui.button("Project Info").clicked() {
                        self.set_static_output(
                            "Project Info",
                            "Drive Manager for Linux\nVersion 0.1.5-alpha\nGitHub: https://github.com/Dogwalker-kryt/Drive-Manager-for-Linux"
                        );
                    }
                    if ui.button("View Logger File").clicked() {
                        self.logger_content = load_log_file();
                        self.current_function = Some("Logger File Viewer".to_string());
                        self.function_output.clear();
                    }
                    ui.add_space(10.0);
                    ui.label("GitHub:");
                    ui.hyperlink("https://github.com/Dogwalker-kryt/Drive-Manager-for-Linux");
                    ui.add_space(10.0);
                    ui.label("Info:");
                    ui.label("This is the GUI version of the Drive-Manager-for-Linux, its written Rust. This verion is v0.0.1-alpha")
                });
        }

        egui::TopBottomPanel::bottom("bottom_panel").resizable(false).show(ctx, |ui| {
            ui.horizontal(|ui| {
                if let Some(ref func) = self.current_function {
                    ui.label(egui::RichText::new(format!("Current function: {}", func)).strong().color(egui::Color32::LIGHT_BLUE));
                    if ui.button("× Clear").clicked() {
                        self.clear_current_function();
                    }
                } else {
                    ui.label(egui::RichText::new("No function selected").italics());
                }
            });
        });

        egui::CentralPanel::default().show(ctx, |ui| {
            ui.heading("Management Functions");
            ui.label("Choose a function:");
            let functions: [(&str, fn(&mut DriveManager)); 11] = [
                ("1. List drives and manage Partitions", DriveManager::list_drives),
                ("2. Format drive", DriveManager::format_drive),
                ("3. Encrypt/Decrypt drive with AES-256", DriveManager::encrypt_decrypt_drive),
                ("4. Resize drive", DriveManager::resize_drive),
                ("5. Check drive health", DriveManager::check_drive_health),
                ("6. Analyze Disk Space", DriveManager::analyze_disk_space),
                ("7. Overwrite Drive Data", DriveManager::overwrite_drive_data),
                ("8. View Metadata of a Drive", DriveManager::view_metadata),
                ("9. Mount/Unmount iso's, Drives,...", DriveManager::mount_unmount),
                ("10. Forensic analysis", DriveManager::forensic_analysis),
                ("11. Diskspace Visualizer", DriveManager::diskspace_visualizer),
            ];

            egui::Grid::new("functions_grid")
                .spacing([20.0, 20.0])
                .min_col_width(280.0)
                .show(ui, |ui| {
                    for (i, (label, func)) in functions.iter().enumerate() {
                        if ui.add(egui::Button::new(*label).min_size(egui::vec2(280.0, 40.0))).clicked() {
                            func(self);
                        }
                        if i % 2 == 1 {
                            ui.end_row();
                        }
                    }
                });

            if !self.drives.is_empty() {
                ui.separator();
                ui.heading("Available Drives:");
                for drive in &self.drives {
                    if ui.button(drive).clicked() {
                        self.selected_drive = Some(drive.clone());
                    }
                }
            }

            if let Some(ref func) = self.current_function {
                match func.as_str() {
                    "Format Drive" => {
                        if let Some(ref drive) = self.selected_drive {
                            ui.separator();
                            ui.label(format!("Selected drive: {}", drive));
                            ui.label("Filesystem type:");
                            ui.text_edit_singleline(&mut self.fs_type);
                            ui.label("Label:");
                            ui.text_edit_singleline(&mut self.label);
                            if ui.button("Format").clicked() {
                                self.format_drive();
                            }
                        }
                    },
                    "Encrypt/Decrypt Drive" => {
                        if let Some(ref drive) = self.selected_drive {
                            ui.separator();
                            ui.label(format!("Selected drive: {}", drive));
                            ui.label("Encryption key:");
                            ui.text_edit_singleline(&mut self.encryption_key);
                            ui.horizontal(|ui| {
                                if ui.button("Encrypt").clicked() {
                                    self.encrypt_decrypt_drive();
                                }
                                if ui.button("Decrypt").clicked() {
                                    // Implement decryption
                                }
                            });
                        }
                    },
                    "Mount/Unmount Drive" => {
                        if let Some(ref drive) = self.selected_drive {
                            ui.separator();
                            ui.label(format!("Selected drive: {}", drive));
                            ui.label("Mount point:");
                            ui.text_edit_singleline(&mut self.mountpoint);
                            ui.horizontal(|ui| {
                                if ui.button("Mount").clicked() {
                                    self.mount_unmount();
                                }
                                if ui.button("Unmount").clicked() {
                                    self.mountpoint.clear();
                                    self.mount_unmount();
                                }
                            });
                        }
                    },
                    _ => {}
                }
            }

            if !self.logger_content.is_empty() {
                ui.separator();
                ui.horizontal(|ui| {
                    ui.heading("Logger File Content:");
                    if ui.button("× Clear Log Viewer").clicked() {
                        self.logger_content.clear();
                    }
                });
                egui::ScrollArea::vertical()
                    .max_height(200.0)
                    .show(ui, |ui| {
                        ui.label(&self.logger_content);
                    });
            }

            if !self.function_output.is_empty() {
                ui.separator();
                ui.heading("Function Output:");
                egui::ScrollArea::vertical().max_height(300.0).show(ui, |ui| {
                    ui.label(egui::RichText::new(&self.function_output).monospace());
                });
            }
        });
    }
}

fn main() -> Result<(), eframe::Error> {
    let options = eframe::NativeOptions::default();
    eframe::run_native(
        "Drive Manager",
        options,
        Box::new(|_cc| Box::new(DriveManager::default())),
    )
}
