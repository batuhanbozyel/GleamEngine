#include "gpch.h"
#include "Transform.h"
#include "Types.h"

using namespace Gleam::Math;

[[nodiscard]] Matrix4 Translate(const Matrix4& mat, const Vector3& vec)
{
	return UNSAFE_CAST(Matrix4, glm::translate(UNSAFE_CONST_CAST(glm::mat4, mat), UNSAFE_CONST_CAST(glm::vec3, vec)));
}

[[nodiscard]] Matrix4 Rotate(const Matrix4& mat, float angle, const Vector3& axis)
{
	return UNSAFE_CAST(Matrix4, glm::rotate(UNSAFE_CONST_CAST(glm::mat4, mat), angle, UNSAFE_CONST_CAST(glm::vec3, axis)));
}

[[nodiscard]] Matrix4 Scale(const Matrix4& mat, const Vector3& scale)
{
	return UNSAFE_CAST(Matrix4, glm::scale(UNSAFE_CONST_CAST(glm::mat4, mat), UNSAFE_CONST_CAST(glm::vec3, scale)));
}

[[nodiscard]] Matrix4 UniformScale(const Matrix4& mat, float scale)
{
	return UNSAFE_CAST(Matrix4, glm::scale(UNSAFE_CONST_CAST(glm::mat4, mat), glm::vec3(scale)));
}