#pragma once

namespace Gleam {

class Mesh;
class Material;

struct MeshRenderer
{
	RefCounted<Mesh> mesh;
	RefCounted<Material> material;
};

} // namespace Gleam
