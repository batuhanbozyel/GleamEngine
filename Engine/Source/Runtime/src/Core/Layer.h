#pragma once

namespace Gleam {

class Renderer;

class Layer
{
public:

	virtual void OnAttach() {};

	virtual void OnUpdate() {};

	virtual void OnRender() {};

	virtual void OnDetach() {};

};

}