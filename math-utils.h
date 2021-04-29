#pragma once

#include "as/as-math-ops.hpp"

as::quat getRotationBetween(const as::vec3& u, const as::vec3& v);
as::mat3 rotateAlign(const as::vec3& u1, const as::vec3& u2);
