#pragma once

namespace Gleam {

namespace Math {

struct Vector2
{
	union
	{
		float x, y;
		float r, g;
	};
	
};

struct Vector3
{
	union
	{
		float x, y, z;
		float r, g, b;
	};

};

struct Vector4
{
	union
	{
		float x, y, z, w;
		float r, g, b, a;
	};
	
};

struct Matrix2
{
	union
	{
		float value[4];
		Vector2 row[2];
	};
};

struct Matrix3
{
	union
	{
		float value[9];
		Vector3 row[3];
	};
};

struct Matrix4
{
	union
	{
		float value[16];
		Vector4 row[4];
	};
};

}
}