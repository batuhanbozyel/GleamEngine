#pragma once

namespace Gleam {

namespace Math {

[[nodiscard]] constexpr Matrix4 Translate(const Matrix4& mat, const Vector3& vec);

[[nodiscard]] constexpr Matrix4 Rotate(const Matrix4& mat, float angle, const Vector3& axis);

[[nodiscard]] constexpr Matrix4 Scale(const Matrix4& mat, const Vector3& scale);

[[nodiscard]] constexpr Matrix4 UniformScale(const Matrix4& mat, float scale);

}}