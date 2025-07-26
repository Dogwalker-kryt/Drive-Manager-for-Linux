#include <gtkmm.h>
#include "logic.h"

class DriveWindow : public Gtk::Window {
public:
    DriveWindow() {
        set_title("Drive Manager");
        set_default_size(400, 200);

        button.set_label("List Drives");
        button.signal_clicked().connect(sigc::mem_fun(*this, &DriveWindow::on_button_clicked));
        add(button);
        show_all_children();
    }

protected:
    void on_button_clicked() {
        std::vector<std::string> drives;
        listDrives(drives); // Reuse your logic function
    }

    Gtk::Button button;
};

int main(int argc, char* argv[]) {
    auto app = Gtk::Application::create("org.example.drivemgr");
    DriveWindow window;
    return app->run(window);
}