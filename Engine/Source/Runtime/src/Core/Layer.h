#pragma once

namespace Gleam {

class Renderer;

class Layer
{
public:

	Layer() = default;
	virtual ~Layer() = default;

	void SetRenderer(Renderer* renderer);

	void OnAttachSuper();

	void OnDetachSuper();

	void OnRenderSuper();

protected:

	virtual void OnAttach() {};

	virtual void OnRender() {};

	virtual void OnDetach() {};

private:

	Scope<Renderer> mRenderer;

};

}