#include "voronoi.h"

#include <thh-bgfx-debug/debug-circle.hpp>
#include <thh-bgfx-debug/debug-line.hpp>

namespace vor
{

static float plot_parabola(
  const float x, const as::vec2& focus, const float directrix)
{
  // equation of parabola from focus and directrix
  // y = 1 / 2(b-k) * (x-a)^2 + 0.5(b+k)
  return ((1.0f / (2.0f * (focus.y - directrix)))
          * ((x - focus.x) * (x - focus.x)))
       + (0.5f * (focus.y + directrix));
}

static as::vec3 circle(as::vec2 p1, as::vec2 p2, as::vec2 p3)
{
  float x1 = p1.x;
  float x2 = p2.x;
  float x3 = p3.x;
  float y1 = p1.y;
  float y2 = p2.y;
  float y3 = p3.y;

  float mr = (y2 - y1) / (x2 - x1);
  float mt = (y3 - y2) / (x3 - x2);

  if (mr == mt) {
    return as::vec3::zero();
  }

  float x12 = x1 - x2;
  float x13 = x1 - x3;

  float y12 = y1 - y2;
  float y13 = y1 - y3;

  float y31 = y3 - y1;
  float y21 = y2 - y1;

  float x31 = x3 - x1;
  float x21 = x2 - x1;

  // x1^2 - x3^2
  float sx13 = (x1 * x1) - (x3 * x3);

  // y1^2 - y3^2
  float sy13 = (y1 * y1) - (y3 * y3);
  float sx21 = (x2 * x2) - (x1 * x1);
  float sy21 = (y2 * y2) - (y1 * y1);

  float f = ((sx13) * (x12) + (sy13) * (x12) + (sx21) * (x13) + (sy21) * (x13))
          / (2.0f * ((y31) * (x12) - (y21) * (x13)));
  float g = ((sx13) * (y12) + (sy13) * (y12) + (sx21) * (y13) + (sy21) * (y13))
          / (2.0f * ((x31) * (y12) - (x21) * (y13)));

  float c = -(x1 * x1) - (y1 * y1) - 2.0f * g * x1 - 2.0f * f * y1;

  float h = -g;
  float k = -f;
  float sqr_of_r = h * h + k * k - c;

  float r = std::sqrt(sqr_of_r);

  return as::vec3(h, k, r);
}

void voronoi_t::calculate()
{
  std::priority_queue<site_t> sites_queue(sites_.begin(), sites_.end());

  sweep_line_ = sites_queue.top().position_.y - 0.1f;

  beachline_.insert(
    {sites_queue.top().position_.x, sites_queue.top().position_});

  // while (!sites_queue.empty()) {
  // }

  // while (!sites_queue.empty()) {
  //   site_triple_.push_back(sites_queue.top());
  //   sites_queue.pop();
  //   if (site_triple_.size() == 3) {
  //     circles_.push_back(circle(
  //       site_triple_[0].position_, site_triple_[1].position_,
  //       site_triple_[2].position_));
  //     site_triple_.pop_front();
  //   }
  // }
}

void voronoi_t::display(dbg::DebugLines* lines, dbg::DebugCircles* circles)
{
  for (const auto& site : beachline_) {
    const auto tessellation = 100;
    const float delta_step =
      (bounds_.max_.x - bounds_.min_.x) / (float)tessellation;
    float x = bounds_.min_.x;
    for (int i = 0; i < tessellation; ++i) {
      float y = plot_parabola(x, site.second, sweep_line_);
      float x_next = x + delta_step;
      float y_next = plot_parabola(x + delta_step, site.second, sweep_line_);
      lines->addLine(
        as::vec3(x, y, 0.0f), as::vec3(x_next, y_next, 0.0f), 0xff000000);
      x += delta_step;
    }
  }

  display_bounds(bounds_, lines);

  for (const auto& site : sites_) {
    circles->addCircle(
      as::mat4_from_mat3_vec3(as::mat3_scale(0.1f), as::vec3(site.position_)),
      as::vec4(0.0f, 0.0f, 0.0f, 1.0f));
  }

  for (const auto& circle : circles_) {
    circles->addCircle(
      as::mat4_from_mat3_vec3(
        as::mat3_scale(circle.z),
        as::vec3_from_vec2(as::vec2_from_vec3(circle))),
      as::vec4(0.0f, 0.0f, 0.0f, 1.0f));
  }

  lines->addLine(
    as::vec3(bounds_.min_.x, sweep_line_, 0.0f),
    as::vec3(bounds_.max_.x, sweep_line_, 0.0f), 0xff000000);
}

void display_bounds(const aabb_t& bounds, dbg::DebugLines* lines)
{
  lines->addLine(
    as::vec3(bounds.min_), as::vec3(bounds.max_.x, bounds.min_.y, 0.0f),
    0xff000000);
  lines->addLine(
    as::vec3(bounds.max_.x, bounds.min_.y, 0.0f), as::vec3(bounds.max_),
    0xff000000);
  lines->addLine(
    as::vec3(bounds.max_), as::vec3(bounds.min_.x, bounds.max_.y, 0.0f),
    0xff000000);
  lines->addLine(
    as::vec3(bounds.min_.x, bounds.max_.y, 0.0f), as::vec3(bounds.min_),
    0xff000000);
}

#if 0
void legacy()
{
  ImGui::Begin("Voronoi");

  ImGui::SliderFloat("directrix", &directrix_, -10.0f, 20.0f);
  ImGui::SliderFloat("focus", &focus_.y, -10.0f, 20.0f);

  static bool standard = false;
  ImGui::Checkbox("standard", &standard);

  ImGui::End();

  auto y_fn = [=](float x, as::vec2 focus) {
    // equation of parabola from focus and directrix
    // y = 1 / 2(b-k) * (x-a)^2 + 0.5(b+k)
    return ((1.0f / (2.0f * (focus.y - directrix_)))
            * ((x - focus.x) * (x - focus.x)))
         + (0.5f * (focus.y + directrix_));
  };

  // vertex = directrix + (directrix - focus.y)
  auto vertex_delta = (focus_.y - directrix_) * 0.5f;
  auto vertex = as::vec3(focus_.x, directrix_ + vertex_delta, 0.0f);

  auto vertex_delta2 = (focus2_.y - directrix_) * 0.5f;
  auto vertex2 = as::vec3(focus2_.x, directrix_ + vertex_delta2, 0.0f);

  // vertex
  debug_draw.debug_circles->addCircle(
    as::mat4_from_mat3_vec3(as::mat3_scale(0.1f), vertex),
    as::vec4(0.0f, 0.0f, 0.0f, 1.0f));

  debug_draw.debug_circles->addCircle(
    as::mat4_from_mat3_vec3(as::mat3_scale(0.1f), vertex2),
    as::vec4(0.0f, 0.0f, 0.0f, 1.0f));

  // standard form
  // (1 / 4p)x^2 - (2h/4p)x + (h^2)/4p + k

  auto center = circle(focus_, focus2_, focus3_);

  debug_draw.debug_circles->addCircle(
    as::mat4_from_mat3_vec3(
      as::mat3_scale(0.1f), as::vec3(as::vec2_from_vec3(center), 0.0f)),
    as::vec4(1.0f, 1.0f, 1.0f, 1.0f));

  debug_draw.debug_circles->addCircle(
    as::mat4_from_mat3_vec3(
      as::mat3_scale(center.z), as::vec3(as::vec2_from_vec3(center), 0.0f)),
    as::vec4(1.0f, 1.0f, 1.0f, 1.0f));

  // p = vertex_delta
  auto p = vertex_delta;
  auto h = vertex.x;
  auto k = vertex.y;

  auto p2 = vertex_delta2;
  auto h2 = vertex2.x;
  auto k2 = vertex2.y;

  auto y_fn_std = [=](float x) {
    return ((1.0f / (4.0f * p)) * (x * x)) - (((2.0f * h) / (4.0f * p)) * x)
         + (((h * h) / (4.0f * p)) + k);
  };

  struct coefs_t
  {
    float a, b, c;
  };

  auto calculate_coefs_fn = [](float h, float p, float k) {
    return coefs_t{
      (1.0f / (4.0f * p)), -(2.0f * h) / (4.0f * p),
      ((h * h) / (4.0f * p)) + k};
  };

  auto coefs = calculate_coefs_fn(h, p, k);
  auto coefs2 = calculate_coefs_fn(h2, p2, k2);

  auto a = coefs.a - coefs2.a;
  auto b = coefs.b - coefs2.b;
  auto c = coefs.c - coefs2.c;

  auto x1 = (-b + std::sqrt((b * b) - (4.0f * a * c))) / (2.0f * a);
  auto x2 = (-b - std::sqrt((b * b) - (4.0f * a * c))) / (2.0f * a);

  auto y1 = (coefs.a * (x1 * x1)) + (coefs.b * x1) + coefs.c;
  auto y2 = (coefs2.a * (x2 * x2)) + (coefs2.b * x2) + coefs2.c;

  debug_draw.debug_circles->addCircle(
    as::mat4_from_mat3_vec3(as::mat3_scale(0.1f), as::vec3(x1, y1, 0.0f)),
    as::vec4(0.0f, 0.0f, 0.0f, 1.0f));

  debug_draw.debug_circles->addCircle(
    as::mat4_from_mat3_vec3(as::mat3_scale(0.1f), as::vec3(x2, y2, 0.0f)),
    as::vec4(0.0f, 0.0f, 0.0f, 1.0f));

  const as::vec2 sites[] = {focus_, focus2_, focus3_};
  std::vector<as::vec2> line;
  for (const auto& site : sites) {
    if (site.y >= directrix_) {
      line.push_back(site);
    }
  }

  const auto tessellation = 1000;
  const float delta_step = 20.0f / (float)tessellation;
  float x = -10.0f;
  if (std::any_of(std::begin(line), std::end(line), [this](auto site) {
        return site.y > directrix_;
      })) {
    for (int i = 0; i < tessellation; ++i) {
      auto y_line_fn = [line, y_fn](float x) {
        float y = 1000.0f;
        for (const auto f : line) {
          y = std::min(y_fn(x, f), y);
        }
        return y;
      };
      float y = y_line_fn(x);
      float x_next = x + delta_step;
      float y_next = y_line_fn(x + delta_step);

      if (!standard) {
        debug_draw.debug_lines->addLine(
          as::vec3(x, y, 0.0f), as::vec3(x_next, y_next, 0.0f), 0xff000000);
      } else {
        float ys = y_fn_std(x);
        float ys_next = y_fn_std(x + delta_step);
        debug_draw.debug_lines->addLine(
          as::vec3(x, ys, 0.0f), as::vec3(x_next, ys_next, 0.0f), 0xff0000ff);
      }
      x += delta_step;
    }
  }

  // x = (-b +/- sqrt(b*b - 4.0f * a * c)) / (2.0 * a)

  // add rendering of bounding box (use min/max extent for rendering)
  // to render combine all parabolas and do min of points to render beach head

  // focus
  debug_draw.debug_circles->addCircle(
    as::mat4_from_mat3_vec3(as::mat3_scale(0.1f), as::vec3(focus_)),
    as::vec4(0.0f, 0.0f, 0.0f, 1.0f));

  debug_draw.debug_circles->addCircle(
    as::mat4_from_mat3_vec3(as::mat3_scale(0.1f), as::vec3(focus2_)),
    as::vec4(0.0f, 0.0f, 0.0f, 1.0f));

  debug_draw.debug_circles->addCircle(
    as::mat4_from_mat3_vec3(as::mat3_scale(0.1f), as::vec3(focus3_)),
    as::vec4(0.0f, 0.0f, 0.0f, 1.0f));

  // directrix
  debug_draw.debug_lines->addLine(
    as::vec3(-50.0f, directrix_, 0.0f), as::vec3(50.0f, directrix_, 0.0f),
    0xff000000);
}
#endif

} // namespace vor
