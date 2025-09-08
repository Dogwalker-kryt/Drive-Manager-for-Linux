use std::fs::OpenOptions;
use std::io::{Write, Error};
use std::path::PathBuf;
use std::env;
use chrono::Local;
use std::process::{Command, Stdio};
use eframe::egui;

#[derive(Default)]
pub struct DriveManager {
    pub show_sidebar: bool,
    pub current_function: Option<String>,
    pub logger_content: String,
    pub function_output: String,
}

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
                if let Err(e) = std::fs::create_dir_all(parent) {
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

pub struct Terminalexec;

impl Terminalexec {
    pub fn exec_terminal(cmd: &str) -> Result<String, Error> {
        let output = Command::new("sh")
            .arg("-c")
            .arg(cmd)
            .output()?;
        if !output.status.success() {
            return Err(std::io::Error::new(
                std::io::ErrorKind::Other,
                format!("Command failed: {}", String::from_utf8_lossy(&output.stderr)),
            ));
        }
        Ok(String::from_utf8_lossy(&output.stdout).to_string())
    }

    pub fn exec_terminal_v2(command: &str) -> String {
        let mut result = String::new();
        let mut pipe = match Command::new("sh")
            .arg("-c")
            .arg(command)
            .stdout(Stdio::piped())
            .spawn()
        {
            Ok(pipe) => pipe,
            Err(_) => return String::new(),
        };
        if let Some(stdout) = pipe.stdout.as_mut() {
            let _ = std::io::Read::read_to_string(stdout, &mut result);
        }
        result
    }

    pub fn exec_terminal_v3(cmd: &str) -> Result<String, Error> {
        let output = Command::new("sh")
            .arg("-c")
            .arg(cmd)
            .output()?;
        if !output.status.success() {
            return Ok(String::new());
        }
        let mut result = String::from_utf8_lossy(&output.stdout).to_string();
        while !result.is_empty() && (result.ends_with('\n') || result.ends_with('\r')) {
            result.pop();
        }
        Ok(result)
    }
}

fn load_log_file() -> String {
    let log_path = PathBuf::from(env::var("HOME").unwrap_or_else(|_| ".".into()))
        .join(".var/app/DriveMgr/log.dat");

    std::fs::read_to_string(log_path).unwrap_or_else(|_| "No log file found.".to_string())
}

// --- DriveManager Method Implementations ---

impl DriveManager {
    // In DriveManager impl:
    fn list_drives(&mut self) {
        self.current_function = Some("List drives".to_owned());
        Logger::log("List drives");
        self.logger_content.clear(); // Clear logger to avoid overlap

        // Run lsblk
        let cmd = "lsblk -o NAME,SIZE,TYPE,MOUNTPOINT,FSTYPE -d -n -p";
        match Terminalexec::exec_terminal(cmd) {
            Ok(lsblk_out) => {
                let mut output = String::new();
                output.push_str("Listing connected drives...\n\nConnected Drives:\n");
                output.push_str(&format!(
                    "{:<3}{:<18}{:<10}{:<10}{:<15}{:<10}{}\n",
                    "#", "Device", "Size", "Type", "Mountpoint", "FSType", "Status"
                ));
                output.push_str(&"-".repeat(90));
                output.push('\n');

                for (i, line) in lsblk_out.lines().enumerate() {
                    if line.contains("disk") {
                        let mut parts = line.split_whitespace();
                        let device = parts.next().unwrap_or("");
                        let size = parts.next().unwrap_or("");
                        let _type = parts.next().unwrap_or("");
                        let mount = parts.next().unwrap_or("");
                        let fstype = parts.next().unwrap_or("");

                        // mock filesystem check
                        let status = if fstype.is_empty() { "Unknown filesystem" } else { fstype };

                        output.push_str(&format!(
                            "{:<3}{:<18}{:<10}{:<10}{:<15}{:<10}{}\n",
                            i, device, size, _type, mount, fstype, status
                        ));
                    }
                }

                self.function_output = output;
            }
            Err(e) => {
                self.function_output = format!("Error listing drives: {}", e);
            }
        }
    }

    fn format_drive(&mut self) {
        self.set_static_output("Format drive", "Format drive function called (implementation pending)");
    }

    fn encrypt_decrypt_drive(&mut self) {
        self.set_static_output("Encrypt/Decrypt drive with AES-256", "Encrypt/Decrypt function called (implementation pending)");
    }

    fn resize_drive(&mut self) {
        self.set_static_output("Resize drive", "Resize drive function called (implementation pending)");
    }

    fn check_drive_health(&mut self) {
        self.set_function("Check drive health", Terminalexec::exec_terminal("smartctl -H /dev/sdX")); // Replace /dev/sdX with user input
    }

    fn analyze_disk_space(&mut self) {
        self.set_function("Analyze Disk Space", Terminalexec::exec_terminal("df -h"));
    }

    fn overwrite_drive_data(&mut self) {
        self.set_static_output("Overwrite Drive Data", "Overwrite drive data function called (implementation pending)");
    }

    fn view_metadata(&mut self) {
        self.set_static_output("View Metadata of a Drive", "View metadata function called (implementation pending)");
    }

    fn view_info(&mut self) {
        self.set_static_output("View Info", "View info function called (implementation pending)");
    }

    fn mount_unmount(&mut self) {
        self.set_static_output("Mount/Unmount iso's, Drives,...", "Mount/Unmount function called (in development)");
    }

    fn forensic_analysis(&mut self) {
        self.set_static_output("Forensic analysis", "Forensic analysis function called (in development)");
    }

    fn diskspace_visualizer(&mut self) {
        self.set_static_output("Diskspace Visualizer", "Diskspace Visualizer function called (in development)");
    }

    fn set_function(&mut self, name: &str, result: Result<String, Error>) {
        self.current_function = Some(name.to_string());
        Logger::log(name);
        self.function_output = match result {
            Ok(output) => output,
            Err(e) => format!("Error: {}", e),
        };
    }

    fn set_static_output(&mut self, name: &str, msg: &str) {
        self.current_function = Some(name.to_string());
        Logger::log(name);
        self.function_output = msg.to_string();
    }

    fn clear_current_function(&mut self) {
        self.current_function = None;
        self.function_output.clear();
    }
}

// --- GUI Implementation ---

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
                        self.set_static_output("Project Info", "Drive Manager for Linux\nVersion 0.0.1-alpha\nGitHub: https://github.com/Dogwalker-kryt/Drive-Manager-for-Linux");
                    }

                    if ui.button("View Logger File").clicked() {
                        self.logger_content = load_log_file();
                        self.current_function = Some("Logger File Viewer".to_string());
                        self.function_output.clear();
                    }

                    ui.add_space(10.0);
                    ui.label("GitHub:");
                    ui.hyperlink("https://github.com/Dogwalker-kryt/Drive-Manager-for-Linux");
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

            let functions: [(&str, fn(&mut DriveManager)); 12] = [
                ("1. List drives", DriveManager::list_drives),
                ("2. Format drive", DriveManager::format_drive),
                ("3. Encrypt/Decrypt drive with AES-256", DriveManager::encrypt_decrypt_drive),
                ("4. Resize drive", DriveManager::resize_drive),
                ("5. Check drive health", DriveManager::check_drive_health),
                ("6. Analyze Disk Space", DriveManager::analyze_disk_space),
                ("7. Overwrite Drive Data", DriveManager::overwrite_drive_data),
                ("8. View Metadata of a Drive", DriveManager::view_metadata),
                ("9. View Info", DriveManager::view_info),
                ("10. Mount/Unmount iso's, Drives,...", DriveManager::mount_unmount),
                ("11. Forensic analysis", DriveManager::forensic_analysis),
                ("12. Diskspace Visualizer", DriveManager::diskspace_visualizer),
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
        "Drive Management Utility GUI v0.0.1-alpha",
        options,
        Box::new(|_cc| Box::new(DriveManager::default())),
    )
}
