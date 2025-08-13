#ifndef GUI_H
#define GUI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <gtk/gtk.h>
#include "../include/functions.h"

// Main window components
typedef struct {
    GtkWidget *window;
    GtkWidget *main_box;
    GtkWidget *drive_list;
    GtkWidget *status_label;
    GtkWidget *progress_bar;
} MainWindow;

// Function declarations
void show_error_dialog(GtkWindow *parent, const char *message);
void show_info_dialog(GtkWindow *parent, const char *message);
void update_drive_list(GtkWidget *list_box);
void on_list_drives_clicked(GtkWidget *widget, gpointer data);
void on_format_drive_clicked(GtkWidget *widget, gpointer data);
void on_encrypt_drive_clicked(GtkWidget *widget, gpointer data);
void on_decrypt_drive_clicked(GtkWidget *widget, gpointer data);
void on_analyze_space_clicked(GtkWidget *widget, gpointer data);
void on_check_health_clicked(GtkWidget *widget, gpointer data);
void on_zero_drive_clicked(GtkWidget *widget, gpointer data);
void on_about_clicked(GtkWidget *widget, gpointer data);

// Helper functions
void set_status_text(MainWindow *win, const char *text);
void set_progress(MainWindow *win, double progress);
GtkWidget *create_drive_list(void);
GtkWidget *create_button_with_icon(const char *label, const char *icon_name);

#endif // GUI_H
