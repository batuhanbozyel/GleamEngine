#include "gpch.h"
#include "Transform.h"
#include "Types.h"

using namespace Gleam;

[[nodiscard]] constexpr Matrix4 Math::Translate(const Matrix4& mat, const Vector3& vec)
{
	return std::bit_cast<Matrix4>(glm::translate(std::bit_cast<glm::mat4>(mat), std::bit_cast<glm::vec3>(vec)));
}

[[nodiscard]] constexpr Matrix4 Math::Rotate(const Matrix4& mat, float angle, const Vector3& axis)
{
	return std::bit_cast<Matrix4>(glm::rotate(std::bit_cast<glm::mat4>(mat), angle, std::bit_cast<glm::vec3>(axis)));
}

[[nodiscard]] constexpr Matrix4 Math::Scale(const Matrix4& mat, const Vector3& scale)
{
	return std::bit_cast<Matrix4>(glm::scale(std::bit_cast<glm::mat4>(mat), std::bit_cast<glm::vec3>(scale)));
}

[[nodiscard]] constexpr Matrix4 Math::UniformScale(const Matrix4& mat, float scale)
{
	return std::bit_cast<Matrix4>(glm::scale(std::bit_cast<glm::mat4>(mat), glm::vec3(scale)));
}