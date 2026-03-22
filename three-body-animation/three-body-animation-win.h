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
    void physics_stepped(GLib::Object, double time, double energy);

    bool animating = false;

    gi::ref_ptr<OpenGLRender> gl_area;
    Gtk::Button start_btn;
    Gtk::Label  time_label;
    Gtk::Label  energy_label;
};
