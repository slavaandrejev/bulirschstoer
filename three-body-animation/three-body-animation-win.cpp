#include <fmt/format.h>

#include <gtk/gtk.hpp>

#include <sigc++/functors/mem_fun.h>

#include "three-body-glrender.h"
#include "three-body-animation-win.h"

MainWindow::MainWindow(Gtk::ApplicationWindow cobj, Gtk::Builder builder)
  : Gtk::impl::ApplicationWindowImpl(cobj, this)
{
    gl_area      = builder.get_object_derived<OpenGLRender>("animation_gl_area");
    start_btn    = builder.get_object<Gtk::Button>("start_btn");
    time_label   = builder.get_object<Gtk::Label>("time_label");
    energy_label = builder.get_object<Gtk::Label>("energy_label");

    start_btn.signal_clicked().connect([&](Gtk::Button) {
        gl_area->toggle_animation();
        if (animating) {
            start_btn.set_label("Start");
            animating = false;
        } else {
            start_btn.set_label("Stop");
            animating = true;
        }
    });
    gl_area->signal_physics_stepped().connect(sigc::mem_fun(*this, &MainWindow::physics_stepped));
}

gi::ref_ptr<MainWindow> MainWindow::new_() {
    auto builder = Gtk::Builder::new_();
    gi::register_type<OpenGLRender>();
    if (builder.add_from_resource("/three-body-animation-win.ui")) {
        return builder.get_object_derived<MainWindow>("main_window");
    }
    return gi::ref_ptr<MainWindow>();
}

void MainWindow::physics_stepped(double time, double energy) {
    time_label.set_text(std::format("{:.1f}", time));
    energy_label.set_text(std::format("{:9.3e}", energy));
}
