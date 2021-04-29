#include "math-utils.h"

as::quat getRotationBetween(const as::vec3& u, const as::vec3& v)
{
  float k_cos_theta = as::vec_dot(u, v);
  float k = std::sqrt(as::vec_length_sq(u) * as::vec_length_sq(v));

  if (k_cos_theta / k == -1) {
    // 180 degree rotation around any orthogonal vector
    return as::quat(0.0f, as::vec_normalize(as::vec3_orthogonal(u)));
  }

  return as::quat_normalize(as::quat(k_cos_theta + k, as::vec3_cross(u, v)));
}

as::mat3 rotateAlign(const as::vec3& u1, const as::vec3& u2)
{
  if (as::vec_near(u1, -u2)) {
    return as::mat3_rotation_axis(
      as::mat3_basis_y(as::orthonormal_basis(u1)), as::radians(180.0f));
  }

  const as::vec3 axis = as::vec3_cross(u1, u2);

  const float cos_a = as::vec_dot(u1, u2);
  const float k = 1.0f / (1.0f + cos_a);

  as::mat3 result(
    (axis.x * axis.x * k) + cos_a, (axis.y * axis.x * k) - axis.z,
    (axis.z * axis.x * k) + axis.y, (axis.x * axis.y * k) + axis.z,
    (axis.y * axis.y * k) + cos_a, (axis.z * axis.y * k) - axis.x,
    (axis.x * axis.z * k) - axis.y, (axis.y * axis.z * k) + axis.x,
    (axis.z * axis.z * k) + cos_a);

  return result;
}
