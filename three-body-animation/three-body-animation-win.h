#pragma once

#include <gtk/gtk.hpp>

#include <gnamespaces.h>

#include "three-body-glrender.h"

class MainWindow : public Gtk::impl::ApplicationWindowImpl
{
public:
    MainWindow(Gtk::ApplicationWindow cobj, Gtk::Builder builder);

    static gi::ref_ptr<MainWindow> new_();

private:
    void physics_stepped(GLib::Object, double time, double E, double E1, double E2, double E3);

    bool animating = false;

    gi::ref_ptr<OpenGLRender> gl_area;
    Gtk::Button start_btn;
    Gtk::Label  time_label;
    Gtk::Label  energy_label;
    Gtk::Label  m1_label;
    Gtk::Label  m2_label;
    Gtk::Label  m3_label;
};
