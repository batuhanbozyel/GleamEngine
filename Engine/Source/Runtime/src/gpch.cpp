#include "gpch.h"

const Gleam::Color Gleam::Color::clear(0.0f, 0.0f, 0.0f, 0.0f);
const Gleam::Color Gleam::Color::black(0.0f, 0.0f, 0.0f, 1.0f);
const Gleam::Color Gleam::Color::white(1.0f, 1.0f, 1.0f, 1.0f);
const Gleam::Color Gleam::Color::gray(0.5f, 0.5f, 0.5f, 1.0f);
const Gleam::Color Gleam::Color::red(1.0f, 0.0f, 0.0f, 1.0f);
const Gleam::Color Gleam::Color::green(0.0f, 1.0f, 0.0f, 1.0f);
const Gleam::Color Gleam::Color::blue(0.0f, 0.0f, 1.0f, 1.0f);
const Gleam::Color Gleam::Color::cyan(0.0f, 1.0f, 1.0f, 1.0f);
const Gleam::Color Gleam::Color::magenta(1.0f, 0.0f, 1.0f, 1.0f);
const Gleam::Color Gleam::Color::yellow(1.0f, 0.92f, 0.016f, 1.0f);

const Gleam::Quaternion Gleam::Quaternion::identity(1.0f, 0.0f, 0.0f, 0.0f);

const Gleam::Size Gleam::Size::zero(0.0f, 0.0f);
const Gleam::Size Gleam::Size::one(1.0f, 1.0f);

const Gleam::Float2x2 Gleam::Float2x2::zero
(
    0.0f, 0.0f,
    0.0f, 0.0f
);

const Gleam::Float2x2 Gleam::Float2x2::identity
(
    1.0f, 0.0f,
    0.0f, 1.0f
);

const Gleam::Float3x3 Gleam::Float3x3::zero
(
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f
);

const Gleam::Float3x3 Gleam::Float3x3::identity
(
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 1.0f
);

const Gleam::Float4x4 Gleam::Float4x4::zero
(
    0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f
);

const Gleam::Float4x4 Gleam::Float4x4::identity
(
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
);
