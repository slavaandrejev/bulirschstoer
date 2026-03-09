/*
 * Copyright (c) 2026, Slava Andrejev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * --- Optional exception to the license ---
 *
 * As an exception, if, as a result of your compiling your source code, portions
 * of this Software are embedded into a machine-executable object form of such
 * source code, you may redistribute such embedded portions in such object form
 * without including the above copyright and permission notices.
 */

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
    time_label   = builder.get_object<Gtk::Entry>("time_label");
    energy_label = builder.get_object<Gtk::Entry>("energy_label");

    start_btn.signal_clicked().connect(sigc::mem_fun(*gl_area, &OpenGLRender::on_start_btn_clicked));
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
    Gtk::Editable(time_label).set_text(std::format("{:.1f}", time));
    Gtk::Editable(energy_label).set_text(std::format("{:9.3e}", energy));
}
