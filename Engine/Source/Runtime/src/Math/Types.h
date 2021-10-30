#pragma once

namespace Gleam {

namespace Math {

struct Vector2
{
	union
	{
		struct
		{
			float x, y;
		};
		struct
		{
			float r, g;
		};
	};
	
};

struct Vector3
{
	union
	{
		struct
		{
			float x, y, z;
		};
		struct
		{
			float r, g, b;
		};
	};

};

struct Vector4
{
	union
	{
		struct
		{
			float x, y, z, w;
		};
		struct
		{
			float r, g, b, a;
		};
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