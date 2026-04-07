#include <cmath>

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
    m1_label     = builder.get_object<Gtk::Label>("m1_label");
    m2_label     = builder.get_object<Gtk::Label>("m2_label");
    m3_label     = builder.get_object<Gtk::Label>("m3_label");

    auto add_color_tint = [&](const gi::cstring &widget_id, auto color) {
        auto css     = Gtk::CssProvider::new_();
        auto widget  = builder.get_object<Gtk::Frame>(widget_id);
        auto css_str = fmt::format(
            "* {{ background-color: rgba({}, {}, {}, 0.15); }}"
          , std::round(color[0] * 255)
          , std::round(color[1] * 255)
          , std::round(color[2] * 255));
        css.load_from_string(css_str);
        widget.get_style_context().add_provider(css, Gtk::STYLE_PROVIDER_PRIORITY_APPLICATION_);
    };
    add_color_tint("m1_frame", gl_area->m1_color());
    add_color_tint("m2_frame", gl_area->m2_color());
    add_color_tint("m3_frame", gl_area->m3_color());

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

void MainWindow::physics_stepped(GLib::Object, double time, double E, double E1, double E2, double E3) {
    time_label.set_text(std::format("{:.1f}", time));
    energy_label.set_text(std::format("{:9.3e}", E));
    m1_label.set_text(std::format("{:+05.1f}", E1));
    m2_label.set_text(std::format("{:+05.1f}", E2));
    m3_label.set_text(std::format("{:+05.1f}", E3));
}
