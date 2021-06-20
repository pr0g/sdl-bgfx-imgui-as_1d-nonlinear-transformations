#include "smooth-line.h"

#include "1d-nonlinear-transformations.h"

namespace dbg
{

void SmoothLine::draw(const as::vec3& begin, const as::vec3& end)
{
  const auto line_granularity = 100;
  for (auto segment = 0; segment < line_granularity; ++segment) {
    const float scale = 1.0f;
    float segment_begin = segment / float(line_granularity);
    float segment_end = float(segment + 1) / float(line_granularity);

    debug_lines_->addLine(
      nlt::bezier5(
        begin, end, begin + as::vec3::axis_x(scale),
        begin + as::vec3::axis_x(scale), end - as::vec3::axis_x(scale),
        end - as::vec3::axis_x(scale), segment_begin),
      nlt::bezier5(
        begin, end, begin + as::vec3::axis_x(scale),
        begin + as::vec3::axis_x(scale), end - as::vec3::axis_x(scale),
        end - as::vec3::axis_x(scale), segment_end),
      0xffffffff);
  }
}

} // namespace dbg
