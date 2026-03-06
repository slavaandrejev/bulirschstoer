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

#include <glbinding/gl/gl.h>

#include <glm/gtc/matrix_transform.hpp>

#include "three-body-glrender.h"

using namespace gl;

OpenGLRender::OpenGLRender(const InitData &id)
  : GlBoundGlArea(id, "OpenGLRender")
{
    // The following initial conditions correspond to a periodic solution of the
    // three-body problem known as "goggles". The parameters were taken from
    // "Three Classes of Newtonian Three-Body Planar Periodic Orbits" by Milovan
    // Šuvakov and V. Dmitrašinović (Phys. Rev. Lett. 110, 114301).
    //
    // Since the paper gave only 6 digits, these parameters were refined using
    // the same Bulirsch-Stoer integrator with multiprecision numbers (see the
    // refinegoggles project).
    //
    // This orbit was chosen because it is highly unstable and accumulates
    // errors very quickly, which makes it a perfect test for the quality of an
    // ODE solver.

    y(stid::x1) = -1;
    y(stid::x2) = 1;
    y(stid::x3) = 0;
    y(stid::y1) = y(stid::y2) = y(stid::y3) = 0;

    y(stid::dx1) = 0.0833000718504974698995;
    y(stid::dy1) = 0.1278892555197667064780;
    y(stid::dx2) = y(stid::dx1);
    y(stid::dy2) = y(stid::dy1);
    y(stid::dx3) = -2 * y(stid::dx1);
    y(stid::dy3) = -2 * y(stid::dy1);
}

void OpenGLRender::on_start_btn_clicked(Gtk::Button) {
    tick_callback_id = add_tick_callback(sigc::mem_fun(*this, &OpenGLRender::timer_event));
}

auto constexpr trace_pnt_size = 3 * sizeof(float);
auto constexpr ball_pnt_size  = 2 * sizeof(float);

bool OpenGLRender::render_(Gdk::GLContext context) noexcept {
    glClearColor(51 / 255.0f, 51 / 255.0f, 55 / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    auto scale_x   = get_width()  / scene_width;
    auto scale_y   = get_height() / scene_height;
    auto scale     = std::min(scale_x, scale_y);
    auto transform = glm::ortho(-float(get_width()) / 2, float(get_width()) / 2, -float(get_height()) / 2, float(get_height()) / 2);
    transform = glm::scale(transform, glm::vec3{scale});

    trace_shader->set("transform", transform);
    ball_shader->set("transform", transform);
    trace_shader->use();

    if (0 != tick_callback_id) {
        trace_shader->set("current_time", float(t * 1000));
        for (auto i = size_t{}; trace_vbo.size() > i; ++i) {
            trace_shader->set("line_color", line_colors[i]);
            glBindVertexArray(trace_vao[i]);
            if (buf_heads[i] > 3 + buf_tails[i]) {
                glDrawArrays(GL_LINE_STRIP_ADJACENCY, buf_tails[i], buf_heads[i] - buf_tails[i]);
            } else if (buf_heads[i] < buf_tails[i]) {
                glDrawArrays(GL_LINE_STRIP_ADJACENCY, buf_tails[i], buf_capacity[i] - buf_tails[i]);
                glDrawArrays(GL_LINE_STRIP_ADJACENCY, 0, buf_heads[i]);
            }
        }
    }
    ball_shader->use();
    for (auto i = size_t{}; ball_vbo.size() > i; ++i) {
        auto v = std::array<float, 2>{float(y(xs_ids[i])), float(y(ys_ids[i]))};
        ball_shader->set("ball_color", line_colors[i]);
        glNamedBufferSubData(ball_vbo[i], 0, ball_pnt_size, &v[0]);
        glBindVertexArray(ball_vao[i]);
        glDrawArrays(GL_POINTS, 0, 1);
    }

    return true;
}

void OpenGLRender::realize_() noexcept {
    GlBoundGlArea::realize_();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    trace_shader = std::make_unique<Shader>(
        "/trace-vs.glsl", GL_VERTEX_SHADER,
        "/trace-gs.glsl", GL_GEOMETRY_SHADER,
        "/trace-fs.glsl", GL_FRAGMENT_SHADER);

    ball_shader = std::make_unique<Shader>(
        "/ball-vs.glsl", GL_VERTEX_SHADER,
        "/ball-gs.glsl", GL_GEOMETRY_SHADER,
        "/ball-fs.glsl", GL_FRAGMENT_SHADER);

    trace_shader->set("half_thickness", 3.5e-3f);
    trace_shader->set("fade_duration", 1.5e4f);

    ball_shader->set("radius", 0.015f);
    ball_shader->set("outline_color", glm::vec3{98.0f / 255.0f, 130.0f / 255.0f, 41.0f / 255.0f});
    ball_shader->set("outline_thickness", 0.15f);

    glCreateVertexArrays(trace_vao.size(), &trace_vao[0]);
    glCreateBuffers(trace_vbo.size(), &trace_vbo[0]);

    glCreateVertexArrays(ball_vao.size(), &ball_vao[0]);
    glCreateBuffers(ball_vbo.size(), &ball_vbo[0]);

    for (auto i = size_t{}; trace_vbo.size() > i; ++i) {
        glNamedBufferStorage(trace_vbo[i], buf_capacity[i] * trace_pnt_size, nullptr, GL_DYNAMIC_STORAGE_BIT);

        // 2D coordinates
        glVertexArrayAttribBinding(trace_vao[i], 0, 0);
        glVertexArrayAttribFormat(trace_vao[i], 0, 2, GL_FLOAT, GL_FALSE, 0);
        glEnableVertexArrayAttrib(trace_vao[i], 0);

        // birth time
        glVertexArrayAttribBinding(trace_vao[i], 1, 0);
        glVertexArrayAttribFormat(trace_vao[i], 1, 1, GL_FLOAT, GL_FALSE, 2 * sizeof(float));
        glEnableVertexArrayAttrib(trace_vao[i], 1);

        glVertexArrayVertexBuffer(trace_vao[i], 0, trace_vbo[i], 0, trace_pnt_size);
    }
    for (auto i = size_t{}; ball_vbo.size() > i; ++i) {
        glNamedBufferStorage(ball_vbo[i], ball_pnt_size, nullptr, GL_DYNAMIC_STORAGE_BIT);
        glVertexArrayAttribBinding(ball_vao[i], 0, 0);
        glVertexArrayAttribFormat(ball_vao[i], 0, 2, GL_FLOAT, GL_FALSE, 0);
        glEnableVertexArrayAttrib(ball_vao[i], 0);
        glVertexArrayVertexBuffer(ball_vao[i], 0, ball_vbo[i], 0, ball_pnt_size);
    }
}

void OpenGLRender::unrealize_() noexcept {
    if (0 != tick_callback_id) {
        remove_tick_callback(tick_callback_id);
        tick_callback_id = 0;
    }
    glDeleteBuffers(ball_vbo.size(), &ball_vbo[0]);
    glDeleteVertexArrays(ball_vao.size(), &ball_vao[0]);
    glDeleteBuffers(trace_vbo.size(), &trace_vbo[0]);
    glDeleteVertexArrays(trace_vao.size(), &trace_vao[0]);
    trace_shader.reset();

    GlBoundGlArea::unrealize_();
}

bool OpenGLRender::timer_event(Gtk::Widget, Gdk::FrameClock frame_clock) {
    if (0 > start_time) {
        start_time = frame_clock.get_frame_time();

        for (auto i = size_t{}; trace_vbo.size() > i; ++i) {
            auto v = std::array<float, 3>{float(y(xs_ids[i])), float(y(ys_ids[i])), 0.0f};
            glNamedBufferSubData(trace_vbo[i], 0, sizeof(v), &v[0]);
            glNamedBufferSubData(trace_vbo[i], sizeof(v), sizeof(v), &v[0]);

            buf_heads[i] = 3; // leave space for repeating the second point two times
            buf_tails[i] = 0;
        }
    } else {
        auto frame_time = frame_clock.get_frame_time();
        auto target_t   = 1e-6 * (frame_time - start_time);
        auto 𝛿t         = target_t - t;

        // advance the physics simulation to match the GTK frame time
        while (std::abs(t - target_t) > 1e-14 * target_t) {
            auto time_remaining = target_t - t;
            auto current_step   = (std::abs(𝛿t) < std::abs(time_remaining)) ? 𝛿t : time_remaining;
            derivs(y, dy);
            auto yscal = (y.abs() + (dy * 𝛿t).abs()).eval();
            auto [new_step, success] = bulirsch_stoer(y, dy, t, current_step, 1e-14 / 1.3, yscal, [&](double, const auto &y, auto &dy) {
                derivs(y, dy);
            });
            if (success) {
                // inject the new coordinates into the VBO ring buffers
                t += current_step;
                for (auto i = size_t{}; trace_vbo.size() > i; ++i) {
                    // v holds 4 floats: [x, y, x, y].
                    // This pre-packages the actual new point AND the duplicate
                    // end-cap required by GL_LINE_STRIP_ADJACENCY.
                    auto v = std::array<float, 6>{float(y(xs_ids[i])), float(y(ys_ids[i])), float(t * 1000)};
                    auto &tl = buf_tails[i];
                    auto &hd = buf_heads[i];
                    // copy points inside GPU memory
                    auto copy_points = [&](size_t from, size_t to, size_t n) {
                        glCopyNamedBufferSubData(
                            trace_vbo[i]
                          , trace_vbo[i]
                          , from * trace_pnt_size
                          , to * trace_pnt_size
                          , trace_pnt_size * n);
                    };
                    // helper to advance the tail while preserving the start-cap context
                    auto inc_tail = [&]() {
                        ++tl;
                        // Copy the new tail coordinate into the slot immediately behind it (tl).
                        // This guarantees gl_in[0] == gl_in[1] when rendering, forcing the geometry
                        // shader to draw a clean, flat cut.
                        copy_points(tl + 1, tl, 1);
                    };
                    // contiguous buffer (head is physically ahead of tail)
                    if (hd > tl) {
                        if (buf_capacity[i] > hd) {
                            // normal append: write the point and the duplicate end-cap ahead of it
                            v[3] = v[0]; v[4] = v[1]; v[5] = v[2];
                            glNamedBufferSubData(trace_vbo[i], (hd - 1) * trace_pnt_size, 2 * trace_pnt_size, &v[0]);
                            ++hd;
                            // if the trace has reached max logical length, pull the tail forward
                            if (max_th_dist[i] < hd - tl) {
                                inc_tail();
                            }
                        } else {
                            // HEAD WRAP LOGIC: Head has hit the physical end of the VRAM buffer
                            // 1. Write the final point to the extreme end of the buffer
                            glNamedBufferSubData(trace_vbo[i], (hd - 1) * trace_pnt_size, trace_pnt_size, &v[0]);
                            // 2. Copy the last 3 points to indices 0, 1, 2 at the start of the buffer.
                            // This bridges the 4-vertex adjacency sliding window across the split.
                            copy_points(hd - 3, 0, 3);
                            // 3. Write the actual new point at index 3, and set head to 4
                            glNamedBufferSubData(trace_vbo[i], 3 * trace_pnt_size, trace_pnt_size, &v[0]);
                            hd = 4;
                            // Pull tail forward to maintain logical length
                            inc_tail();
                        }
                    }
                    // split buffer (head has wrapped and is physically behind tail)
                    else {
                        // normal append at the current wrapped head position
                        v[3] = v[0]; v[4] = v[1]; v[5] = v[2];
                        glNamedBufferSubData(trace_vbo[i], (hd - 1) * trace_pnt_size, 2 * trace_pnt_size, &v[0]);
                        ++hd;
                        // Collision avoidance: tail is pushed forward by head
                        // pressure. We keep distance 3 between then to retain
                        // the number of segments when wrap.
                        if (3 > tl - hd) {
                            if (buf_capacity[i] - 4 <= tl) {
                                // 1. Copy the C-2 point into index 0 to mathematically re-establish
                                // the padded start-cap (p0 == p1) for the new chunk at the beginning.
                                copy_points(buf_capacity[i] - 2, 0, 1);
                                // 2. Wrap tail to the beginning of the buffer
                                tl = 0;
                            } else {
                                // normal tail advance
                                inc_tail();
                            }
                        }
                    }
                }
            }
            𝛿t = new_step;
        }
        // the frame is ready to render
        queue_draw();
    }

    return true;
}
