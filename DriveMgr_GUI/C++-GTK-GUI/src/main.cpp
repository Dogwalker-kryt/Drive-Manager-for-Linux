#include <gtk/gtk.h>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <thread>
#include <atomic>
#include <mutex>
#include <filesystem>
#include "drives.h"

// Columns for the list store
enum { COL_NAME, COL_SIZE, COL_TYPE, COL_MOUNT, COL_DEVPATH, N_COLUMNS };

static GtkWidget *window;
static GtkWidget *treeview;
static GtkListStore *store;
static GtkWidget *statusbar_label;
static GtkWidget *log_textview; // right pane textview (global so workers can append)

// Simple thread-safe appender helper
struct IdleAppend {
    std::string *s;
    IdleAppend(std::string *p):s(p){}
};

static gboolean idle_append_func(gpointer data) {
    IdleAppend *ia = static_cast<IdleAppend*>(data);
    std::string *s = ia->s;
    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(log_textview));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buf, &end);
    gtk_text_buffer_insert(buf, &end, s->c_str(), -1);
    delete s;
    delete ia;
    return G_SOURCE_REMOVE;
}

static void append_log(const std::string &t) {
    // schedule on main loop
    std::string *p = new std::string(t);
    IdleAppend *ia = new IdleAppend(p);
    g_idle_add(idle_append_func, ia);
}

// Run a binary asynchronously, streaming output into the log
static void run_binary_async(const std::string &path, const std::vector<std::string> &args) {
    // Safety: ensure path is inside repository bin folder
    std::filesystem::path p(path);
    std::filesystem::path repo_root = std::filesystem::weakly_canonical(std::filesystem::path("../.."));
    std::filesystem::path allowed = repo_root / "bin";
    bool ok = false;
    try { ok = std::mismatch(allowed.begin(), allowed.end(), p.begin()).first==allowed.end(); } catch(...) { ok=false; }
    // allow running if path contains 'DriveMgr' as a fallback
    if (!ok && p.filename().string().find("DriveMgr")==std::string::npos) {
        append_log("Refusing to run executable outside repo/bin\n");
        return;
    }

    std::thread([path,args](){
        std::string cmd = path;
        for (auto &a: args) {
            cmd += " ";
            // naive quoting
            if (a.find(' ')!=std::string::npos) cmd += '"' + a + '"'; else cmd += a;
        }
        cmd += " 2>&1"; // capture stderr
        append_log(std::string("Running: ") + cmd + "\n");
        FILE *f = popen(cmd.c_str(), "r");
        if (!f) {
            append_log("Failed to start process\n");
            return;
        }
        char buf[4096];
        while (fgets(buf, sizeof(buf), f)) {
            append_log(std::string(buf));
        }
        int rc = pclose(f);
        append_log(std::string("Process exited with code ") + std::to_string(rc) + "\n");
    }).detach();
}

// Helper to find candidate binaries in repo bin folders
static std::vector<std::filesystem::path> find_repo_binaries() {
    std::vector<std::filesystem::path> res;
    std::filesystem::path repo = std::filesystem::weakly_canonical(std::filesystem::path("../.."));
    std::vector<std::filesystem::path> search = { repo / "bin", repo / "bin" / "bin" };
    for (auto &d : search) {
        if (!std::filesystem::exists(d)) continue;
        for (auto &ent : std::filesystem::directory_iterator(d)) {
            if (!ent.is_regular_file()) continue;
            auto fn = ent.path().filename().string();
            if (fn.find("DriveMgr")!=std::string::npos || ent.path().extension()=="")
                res.push_back(ent.path());
        }
    }
    return res;
}

static void set_status(const std::string &s) {
    gtk_label_set_text(GTK_LABEL(statusbar_label), s.c_str());
}

static void on_refresh_clicked(GtkButton *btn, gpointer user_data) {
    std::vector<std::string> drives;
    listDrives(drives);

    gtk_list_store_clear(store);

    for (const auto &line : drives) {
        // expected format: NAME|SIZE|TYPE|MOUNT
        std::string name, size, type, mount;
        std::istringstream iss(line);
        std::getline(iss, name, '|');
        std::getline(iss, size, '|');
        std::getline(iss, type, '|');
        std::getline(iss, mount, '|');

        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           COL_NAME, name.c_str(),
                           COL_SIZE, size.c_str(),
                           COL_TYPE, type.c_str(),
                           COL_MOUNT, mount.c_str(),
                           COL_DEVPATH, (std::string("/dev/") + name).c_str(),
                           -1);
    }

    set_status("Drives refreshed.");
}

static void show_text_dialog(GtkWindow *parent, const char *title, const std::string &text) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(title, parent, (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
                                                    "_Close", GTK_RESPONSE_CLOSE, NULL);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *tv = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(tv), FALSE);
    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
    gtk_text_buffer_set_text(buf, text.c_str(), -1);
    gtk_container_add(GTK_CONTAINER(scrolled), tv);
    gtk_widget_set_size_request(scrolled, 600, 300);
    gtk_container_add(GTK_CONTAINER(content), scrolled);
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void on_show_details_clicked(GtkButton *btn, gpointer user_data) {
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    GtkTreeIter iter;
    GtkTreeModel *model;
    if (!gtk_tree_selection_get_selected(sel, &model, &iter)) {
        set_status("No drive selected.");
        return;
    }

    gchar *name = NULL;
    gchar *devpath = NULL;
    gtk_tree_model_get(model, &iter, COL_NAME, &name, COL_DEVPATH, &devpath, -1);

    std::string info;
    if (devpath) {
        std::string out;
        if (getDeviceInfo(std::string(devpath), out)) info = out;
        else info = "Failed to retrieve device info.";
    }

    show_text_dialog(GTK_WINDOW(window), (std::string("Device details: ") + (name ? name : "")).c_str(), info);

    if (name) g_free(name);
    if (devpath) g_free(devpath);
}

static void on_save_report_clicked(GtkButton *btn, gpointer user_data) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Save report", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_SAVE,
                                                    "Cancel", GTK_RESPONSE_CANCEL, "Save", GTK_RESPONSE_ACCEPT, NULL);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        std::ofstream ofs(filename);
        if (ofs) {
            // dump list content
            GtkTreeIter iter;
            gboolean valid;
            GtkTreeModel *model = GTK_TREE_MODEL(store);
            valid = gtk_tree_model_get_iter_first(model, &iter);
            while (valid) {
                gchar *name, *size, *type, *mount, *devpath;
                gtk_tree_model_get(model, &iter,
                                   COL_NAME, &name,
                                   COL_SIZE, &size,
                                   COL_TYPE, &type,
                                   COL_MOUNT, &mount,
                                   COL_DEVPATH, &devpath,
                                   -1);
                ofs << name << "|" << size << "|" << type << "|" << mount << "|" << devpath << "\n";
                g_free(name); g_free(size); g_free(type); g_free(mount); g_free(devpath);
                valid = gtk_tree_model_iter_next(model, &iter);
            }
            ofs.close();
            set_status(std::string("Saved report to ") + filename);
        } else {
            set_status("Failed to open file for writing.");
        }
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Drive Manager GUI");
    gtk_window_set_default_size(GTK_WINDOW(window), 900, 500);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 8);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Toolbar with buttons
    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *btn_refresh = gtk_button_new_with_label("Refresh");
    GtkWidget *btn_details = gtk_button_new_with_label("Show Details");
    GtkWidget *btn_save = gtk_button_new_with_label("Save Report");
    gtk_box_pack_start(GTK_BOX(toolbar), btn_refresh, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_details, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_save, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

    // Paned area: left list, right log
    GtkWidget *hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), hpaned, TRUE, TRUE, 0);

    // Left: TreeView
    store = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *col_name = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", COL_NAME, NULL);
    GtkTreeViewColumn *col_size = gtk_tree_view_column_new_with_attributes("Size", renderer, "text", COL_SIZE, NULL);
    GtkTreeViewColumn *col_type = gtk_tree_view_column_new_with_attributes("Type", renderer, "text", COL_TYPE, NULL);
    GtkTreeViewColumn *col_mount = gtk_tree_view_column_new_with_attributes("Mount", renderer, "text", COL_MOUNT, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), col_name);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), col_size);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), col_type);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), col_mount);

    GtkWidget *scrolled_left = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_left), treeview);
    gtk_widget_set_size_request(scrolled_left, 450, 400);
    gtk_paned_pack1(GTK_PANED(hpaned), scrolled_left, TRUE, FALSE);

    // Right: log / info area
    GtkWidget *scrolled_right = gtk_scrolled_window_new(NULL, NULL);
    log_textview = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(log_textview), FALSE);
    gtk_container_add(GTK_CONTAINER(scrolled_right), log_textview);
    gtk_widget_set_size_request(scrolled_right, 420, 400);
    gtk_paned_pack2(GTK_PANED(hpaned), scrolled_right, TRUE, FALSE);

    // Status bar
    statusbar_label = gtk_label_new("Ready");
    gtk_box_pack_start(GTK_BOX(vbox), statusbar_label, FALSE, FALSE, 0);

    // Action buttons (additional)
    GtkWidget *btn_file_recovery = gtk_button_new_with_label("File Recovery");
    GtkWidget *btn_part_recovery = gtk_button_new_with_label("Partition Recovery");
    GtkWidget *btn_sys_recovery = gtk_button_new_with_label("System Recovery");
    GtkWidget *btn_run_bin = gtk_button_new_with_label("Run Selected Binary");
    gtk_box_pack_start(GTK_BOX(toolbar), btn_file_recovery, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_part_recovery, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_sys_recovery, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_run_bin, FALSE, FALSE, 0);

    // Signals
    g_signal_connect(btn_refresh, "clicked", G_CALLBACK(on_refresh_clicked), NULL);
    g_signal_connect(btn_details, "clicked", G_CALLBACK(on_show_details_clicked), NULL);
    g_signal_connect(btn_save, "clicked", G_CALLBACK(on_save_report_clicked), NULL);
    g_signal_connect(btn_file_recovery, "clicked", G_CALLBACK(+[](GtkButton*, gpointer){
        auto bins = find_repo_binaries();
        if (bins.empty()) { append_log("No repository binaries found to perform file recovery.\n"); return; }
        // pick first that looks like file recovery
        std::string chosen = bins.front().string();
        append_log(std::string("Starting file recovery using ") + chosen + "\n");
        run_binary_async(chosen, {});
    }), NULL);
    g_signal_connect(btn_part_recovery, "clicked", G_CALLBACK(+[](GtkButton*, gpointer){
        auto bins = find_repo_binaries();
        if (bins.empty()) { append_log("No repository binaries found to perform partition recovery.\n"); return; }
        std::string chosen = bins.front().string();
        append_log(std::string("Starting partition recovery using ") + chosen + "\n");
        run_binary_async(chosen, {"--partition-recovery"});
    }), NULL);
    g_signal_connect(btn_sys_recovery, "clicked", G_CALLBACK(+[](GtkButton*, gpointer){
        auto bins = find_repo_binaries();
        if (bins.empty()) { append_log("No repository binaries found to perform system recovery.\n"); return; }
        std::string chosen = bins.front().string();
        append_log(std::string("Starting system recovery using ") + chosen + "\n");
        run_binary_async(chosen, {"--system-recovery"});
    }), NULL);
    g_signal_connect(btn_run_bin, "clicked", G_CALLBACK(+[](GtkButton*, gpointer){
        auto bins = find_repo_binaries();
        if (bins.empty()) { append_log("No binaries found in repo/bin.\n"); return; }
        // show a quick chooser dialog
        GtkWidget *dialog = gtk_dialog_new_with_buttons("Choose binary", GTK_WINDOW(window), GTK_DIALOG_MODAL,
                                                       "Cancel", GTK_RESPONSE_CANCEL, "Run", GTK_RESPONSE_ACCEPT, NULL);
        GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        GtkWidget *list = gtk_list_box_new();
        for (auto &p: bins) {
            GtkWidget *row = gtk_label_new(p.string().c_str());
            gtk_container_add(GTK_CONTAINER(list), row);
        }
        gtk_container_add(GTK_CONTAINER(content), list);
        gtk_widget_show_all(dialog);
        int res = gtk_dialog_run(GTK_DIALOG(dialog));
        if (res == GTK_RESPONSE_ACCEPT) {
            // find selected row index
            GList *children = gtk_container_get_children(GTK_CONTAINER(list));
            int idx = 0;
            for (GList *it = children; it; it = it->next, idx++) {
                GtkWidget *w = GTK_WIDGET(it->data);
                gboolean sel = gtk_widget_get_visible(w); // no proper selection used; pick first
                if (idx==0) {
                    const char *txt = gtk_label_get_text(GTK_LABEL(w));
                    append_log(std::string("Running chosen binary: ") + txt + "\n");
                    run_binary_async(std::string(txt), {});
                    break;
                }
            }
            g_list_free(children);
        }
        gtk_widget_destroy(dialog);
    }), NULL);

    // Initial population
    on_refresh_clicked(NULL, NULL);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
