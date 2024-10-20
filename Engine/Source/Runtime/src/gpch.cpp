#include "gpch.h"

using namespace Gleam;

const Color Color::clear(0.0f, 0.0f, 0.0f, 0.0f);
const Color Color::black(0.0f, 0.0f, 0.0f, 1.0f);
const Color Color::white(1.0f, 1.0f, 1.0f, 1.0f);
const Color Color::gray(0.5f, 0.5f, 0.5f, 1.0f);
const Color Color::red(1.0f, 0.0f, 0.0f, 1.0f);
const Color Color::green(0.0f, 1.0f, 0.0f, 1.0f);
const Color Color::blue(0.0f, 0.0f, 1.0f, 1.0f);
const Color Color::cyan(0.0f, 1.0f, 1.0f, 1.0f);
const Color Color::magenta(1.0f, 0.0f, 1.0f, 1.0f);
const Color Color::yellow(1.0f, 0.92f, 0.016f, 1.0f);

const Quaternion Quaternion::identity(1.0f, 0.0f, 0.0f, 0.0f);

const Size Size::zero(0.0f, 0.0f);
const Size Size::one(1.0f, 1.0f);

const Float2x2 Float2x2::zero
(
    0.0f, 0.0f,
    0.0f, 0.0f
);

const Float2x2 Float2x2::identity
(
    1.0f, 0.0f,
    0.0f, 1.0f
);

const Float3x3 Float3x3::zero
(
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f
);

const Float3x3 Float3x3::identity
(
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 1.0f
);

const Float4x4 Float4x4::zero
(
    0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f
);

const Float4x4 Float4x4::identity
(
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
);
