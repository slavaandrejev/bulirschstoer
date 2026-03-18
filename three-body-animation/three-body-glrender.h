#pragma once

#include <memory>

#include <glm/vec3.hpp>

#include <gtk/gtk.hpp>

#include <Eigen/Dense>

#include <sigc++/signal.h>

#include <bulirschstoer.h>
#include <gnamespaces.h>

#include "glboundglarea.h"
#include "shader.h"

class OpenGLRender : public GlBoundGlArea
{
    friend struct WidgetClassDef::TypeInitData;
    friend struct GLAreaClassDef::TypeInitData;
private:
    using type_signal_physics_stepped = sigc::signal<void(double, double)>;

    type_signal_physics_stepped physics_stepped;

public:
    // We need this constructor only to satisfy the requirement of
    // `Gtk::Builder::get_object_derived`. This class is constructed by
    // GtkBuilder's C side and this constructor is never called.
    OpenGLRender(Gtk::GLArea cobj, Gtk::Builder builder)
      : GlBoundGlArea(cobj, builder)
    {}

    OpenGLRender(const InitData &id);

    static GType get_type_() {
        return register_type_<OpenGLRender>("OpenGLRender", 0, {}, {}, {});
    }

    void toggle_animation();

    auto signal_physics_stepped() {
        return physics_stepped;
    }

private:
    bool render_(Gdk::GLContext context) noexcept override;
    void realize_() noexcept override;
    void unrealize_() noexcept override;

    static auto constexpr n_vars = 12;
    using Ys = Eigen::Array<double, n_vars, 1>;

    static
    auto cube(auto x) {
        return x * x * x;
    }

    static
    void derivs(const Ys &y, Ys &dy) {
        auto r1 = y.segment<2>(stid::r1);
        auto r2 = y.segment<2>(stid::r2);
        auto r3 = y.segment<2>(stid::r3);

        auto r2mr1 = (r2 - r1) / cube((r2 - r1).matrix().norm());
        auto r3mr1 = (r3 - r1) / cube((r3 - r1).matrix().norm());
        auto r3mr2 = (r3 - r2) / cube((r3 - r2).matrix().norm());

        dy.segment<2>(stid::r1) = y.segment<2>(stid::dr1);
        dy.segment<2>(stid::r2) = y.segment<2>(stid::dr2);
        dy.segment<2>(stid::r3) = y.segment<2>(stid::dr3);

        dy.segment<2>(stid::dr1) =  r2mr1 + r3mr1;
        dy.segment<2>(stid::dr2) =  r3mr2 - r2mr1;
        dy.segment<2>(stid::dr3) = -r3mr1 - r3mr2;
    }

    static
    auto energy(const Ys &y) {
        auto r1 = y.segment<2>(stid::r1);
        auto r2 = y.segment<2>(stid::r2);
        auto r3 = y.segment<2>(stid::r3);

        auto r12 = (r2 - r1).matrix().norm();
        auto r23 = (r2 - r3).matrix().norm();
        auto r31 = (r3 - r1).matrix().norm();

        auto v1 = y.segment<2>(stid::dx1);
        auto v2 = y.segment<2>(stid::dx2);
        auto v3 = y.segment<2>(stid::dx3);

        return (v1.matrix().squaredNorm() +
                v2.matrix().squaredNorm() +
                v3.matrix().squaredNorm()) / 2 - (1 / r12 + 1 / r23 + 1 / r31);
    }

    struct stid {
        static auto constexpr r1  = 0;
        static auto constexpr r2  = 2;
        static auto constexpr r3  = 4;

        static auto constexpr dr1  = 6;
        static auto constexpr dr2  = 8;
        static auto constexpr dr3  = 10;

        static auto constexpr x1  = 0;
        static auto constexpr y1  = 1;
        static auto constexpr x2  = 2;
        static auto constexpr y2  = 3;
        static auto constexpr x3  = 4;
        static auto constexpr y3  = 5;

        static auto constexpr dx1 = 6;
        static auto constexpr dy1 = 7;
        static auto constexpr dx2 = 8;
        static auto constexpr dy2 = 9;
        static auto constexpr dx3 = 10;
        static auto constexpr dy3 = 11;
    };

    double t  = 0.0;
    double e0 = 0.0;
    Ys y;
    Ys dy;

    BulirschStoer<n_vars, 12> bulirsch_stoer;

    guint tick_callback_id{};
    bool timer_event(Gtk::Widget, Gdk::FrameClock frame_clock);

    // Place the initial locations to the trace buffers, reset heads and tails.
    void init_trace_buffers();

    // Inject new locations directly in the GPU memory. Update heads and tails.
    void update_trace_buffers();

    template <typename Func>
    void advance_physics(double target_t, Func &&gui_update);

    std::unique_ptr<Shader> trace_shader;
    std::unique_ptr<Shader> ball_shader;

    std::array<gl::GLuint, 3> trace_vao = {};
    std::array<gl::GLuint, 3> trace_vbo = {}; // ring buffers to draw traces

    std::array<gl::GLuint, 3> ball_vao = {};
    std::array<gl::GLuint, 3> ball_vbo = {};

    std::array<int, 3> buf_heads    = {};
    std::array<int, 3> buf_tails    = {};

    static constexpr std::array<int, 3> buf_capacity = {2000, 2000, 2000};
    static constexpr std::array<int, 3> max_th_dist  = {buf_capacity[0] - 3, buf_capacity[1] - 3, buf_capacity[2] - 3};

    static constexpr std::array<int, 3> xs_ids = {stid::x1, stid::x2, stid::x3};
    static constexpr std::array<int, 3> ys_ids = {stid::y1, stid::y2, stid::y3};

    std::array<glm::vec3, 3> line_colors = {{
        {230.0f / 255.0f,  92.0f / 255.0f,  13.0f / 255.0f}
      , { 93.0f / 255.0f, 109.0f / 255.0f, 194.0f / 255.0f}
      , {241.0f / 255.0f, 152.0f / 255.0f,   0.0f / 255.0f}
      }};

    float scene_width  = 2.140f;
    float scene_height = 1.516f;

    gint64 start_time = -1;
};
