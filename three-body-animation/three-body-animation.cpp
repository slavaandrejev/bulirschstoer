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

#include <fmt/printf.h>

#include <gnamespaces.h>

#include "three-body-animation-win.h"

int main (int argc, char *argv[]) {
    auto app = Gtk::Application::new_(Gio::ApplicationFlags::DEFAULT_FLAGS_);

    auto main_window = gi::ref_ptr<MainWindow>{};
    app.signal_activate().connect([&](Gio::Application app_) {
        try {
            // The application has been started, so let's show a window.
            main_window = MainWindow::new_();
            // Make sure that the application runs for as long this window is
            // still open.
            app.add_window(*main_window);
            main_window->present();
        }
        // If create_appwindow() throws an exception (perhaps from
        // Gtk::Builder), no window has been created, no window has been added
        // to the application, and therefore the application will stop running.
        catch (const GLib::Error& ex) {
            fmt::print(stderr, "Application::on_activate(): {}\n", ex.what());
            app_.quit();
        }
        catch (const std::exception& ex) {
            fmt::print(stderr, "Application::on_activate(): {}\n", ex.what());
            app_.quit();
        }
    });
    app.signal_window_removed().connect([&](Gtk::Application, Gtk::Window window) {
        if (main_window && *main_window == window) {
            main_window.reset();
        }
    });

    return app.run({argv, size_t(argc)});
}
