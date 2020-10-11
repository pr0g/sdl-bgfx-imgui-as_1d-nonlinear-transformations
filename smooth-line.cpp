#include "smooth-line.h"

#include "1d-nonlinear-transformations.h"

namespace dbg
{

void SmoothLine::draw(const as::vec3& begin, const as::vec3& end)
{
  const auto line_granularity = 100;
  const auto line_length = as::vec_distance(begin, end);
  for (auto segment = 0; segment < line_granularity; ++segment) {
    const float scale = 1.0f;
    float segmentBegin = segment / float(line_granularity);
    float segmentEnd = float(segment + 1) / float(line_granularity);

    debug_lines_.addLine(
      nlt::bezier5(
        begin, end, begin + as::vec3::axis_x(scale),
        begin + as::vec3::axis_x(scale), end - as::vec3::axis_x(scale),
        end - as::vec3::axis_x(scale), segmentBegin),
      nlt::bezier5(
        begin, end, begin + as::vec3::axis_x(scale),
        begin + as::vec3::axis_x(scale), end - as::vec3::axis_x(scale),
        end - as::vec3::axis_x(scale), segmentEnd),
      0xffffffff);
  }

  debug_lines_.submit();
}

} // namespace dbg
