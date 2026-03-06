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
#include <gtk/gtk.hpp>

#include <sigc++/functors/mem_fun.h>

#include "three-body-glrender.h"
#include "three-body-animation-win.h"

MainWindow::MainWindow(Gtk::ApplicationWindow cobj, Gtk::Builder builder)
  : Gtk::impl::ApplicationWindowImpl(cobj, this)
{}

gi::ref_ptr<MainWindow> MainWindow::new_() {
    auto builder = Gtk::Builder::new_();
    gi::register_type<OpenGLRender>();
    if (builder.add_from_resource("/three-body-animation-win.ui")) {
        auto gl_area   = builder.get_object_derived<OpenGLRender>("animation_gl_area");
        auto start_btn = builder.get_object<Gtk::Button>("start_btn");

        start_btn.signal_clicked().connect(sigc::mem_fun(*gl_area, &OpenGLRender::on_start_btn_clicked));

        return builder.get_object_derived<MainWindow>("main_window");
    }
    return gi::ref_ptr<MainWindow>();
}
