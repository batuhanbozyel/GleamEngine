// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2024 Advanced Micro Devices, Inc.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef FFX_CORE_H
#define FFX_CORE_H

#define FFX_GPU  1
#define FFX_HALF 1
#define FFX_WAVE 1
#define FFX_HLSL 1

#define FfxFloat32Mat4 matrix <float, 4, 4>
#define FfxFloat32Mat3 matrix <float, 3, 3>

/// A typedef for a boolean value.
///
/// @ingroup HLSLTypes
typedef bool FfxBoolean;

/// @defgroup HLSL62Types HLSL 6.2 And Above Types
/// HLSL 6.2 and above type defines for all commonly used variables
/// 
/// @ingroup HLSLTypes

/// A typedef for a floating point value.
///
/// @ingroup HLSL62Types
typedef float32_t   FfxFloat32;

/// A typedef for a 2-dimensional floating point value.
///
/// @ingroup HLSL62Types
typedef float32_t2  FfxFloat32x2;

/// A typedef for a 3-dimensional floating point value.
///
/// @ingroup HLSL62Types
typedef float32_t3  FfxFloat32x3;

/// A typedef for a 4-dimensional floating point value.
///
/// @ingroup HLSL62Types
typedef float32_t4  FfxFloat32x4;

/// A [cacao_placeholder] typedef for matrix type until confirmed.
typedef float4x4 FfxFloat32x4x4;
typedef float3x4 FfxFloat32x3x4;
typedef float3x3 FfxFloat32x3x3;
typedef float2x2 FfxFloat32x2x2;

/// A typedef for a unsigned 32bit integer.
///
/// @ingroup HLSL62Types
typedef uint32_t    FfxUInt32;

/// A typedef for a 2-dimensional 32bit unsigned integer.
///
/// @ingroup HLSL62Types
typedef uint32_t2   FfxUInt32x2;

/// A typedef for a 3-dimensional 32bit unsigned integer.
///
/// @ingroup HLSL62Types
typedef uint32_t3   FfxUInt32x3;

/// A typedef for a 4-dimensional 32bit unsigned integer.
///
/// @ingroup HLSL62Types
typedef uint32_t4   FfxUInt32x4;

/// A typedef for a signed 32bit integer.
///
/// @ingroup HLSL62Types
typedef int32_t     FfxInt32;

/// A typedef for a 2-dimensional signed 32bit integer.
///
/// @ingroup HLSL62Types
typedef int32_t2    FfxInt32x2;

/// A typedef for a 3-dimensional signed 32bit integer.
///
/// @ingroup HLSL62Types
typedef int32_t3    FfxInt32x3;

/// A typedef for a 4-dimensional signed 32bit integer.
///
/// @ingroup HLSL62Types
typedef int32_t4    FfxInt32x4;

#if FFX_HALF
typedef float16_t   FfxFloat16;
typedef float16_t2  FfxFloat16x2;
typedef float16_t3  FfxFloat16x3;
typedef float16_t4  FfxFloat16x4;

/// A typedef for an unsigned 16bit integer.
///
/// @ingroup HLSLTypes
typedef uint16_t    FfxUInt16;
typedef uint16_t2   FfxUInt16x2;
typedef uint16_t3   FfxUInt16x3;
typedef uint16_t4   FfxUInt16x4;

/// A typedef for a signed 16bit integer.
///
/// @ingroup HLSLTypes
typedef int16_t     FfxInt16;
typedef int16_t2    FfxInt16x2;
typedef int16_t3    FfxInt16x3;
typedef int16_t4    FfxInt16x4;
#endif // FFX_HALF

#ifdef FFX_HALF

#define FFX_MIN16_SCALAR( TypeName, BaseComponentType )           typedef BaseComponentType##16_t TypeName;
#define FFX_MIN16_VECTOR( TypeName, BaseComponentType, COL )      typedef vector<BaseComponentType##16_t, COL> TypeName;
#define FFX_MIN16_MATRIX( TypeName, BaseComponentType, ROW, COL ) typedef matrix<BaseComponentType##16_t, ROW, COL> TypeName;

#define FFX_16BIT_SCALAR( TypeName, BaseComponentType )           typedef BaseComponentType##16_t TypeName;
#define FFX_16BIT_VECTOR( TypeName, BaseComponentType, COL )      typedef vector<BaseComponentType##16_t, COL> TypeName;
#define FFX_16BIT_MATRIX( TypeName, BaseComponentType, ROW, COL ) typedef matrix<BaseComponentType##16_t, ROW, COL> TypeName;

#else // FFX_HALF

#define FFX_MIN16_SCALAR( TypeName, BaseComponentType )           typedef BaseComponentType TypeName;
#define FFX_MIN16_VECTOR( TypeName, BaseComponentType, COL )      typedef vector<BaseComponentType, COL> TypeName;
#define FFX_MIN16_MATRIX( TypeName, BaseComponentType, ROW, COL ) typedef matrix<BaseComponentType, ROW, COL> TypeName;

#define FFX_16BIT_SCALAR( TypeName, BaseComponentType )           typedef BaseComponentType TypeName;
#define FFX_16BIT_VECTOR( TypeName, BaseComponentType, COL )      typedef vector<BaseComponentType, COL> TypeName;
#define FFX_16BIT_MATRIX( TypeName, BaseComponentType, ROW, COL ) typedef matrix<BaseComponentType, ROW, COL> TypeName;

#endif // FFX_HALF

#if defined(FFX_GPU)
// Common typedefs:
FFX_MIN16_SCALAR( FFX_MIN16_F , float );
FFX_MIN16_VECTOR( FFX_MIN16_F2, float, 2 );
FFX_MIN16_VECTOR( FFX_MIN16_F3, float, 3 );
FFX_MIN16_VECTOR( FFX_MIN16_F4, float, 4 );

FFX_MIN16_SCALAR( FFX_MIN16_I,  int );
FFX_MIN16_VECTOR( FFX_MIN16_I2, int, 2 );
FFX_MIN16_VECTOR( FFX_MIN16_I3, int, 3 );
FFX_MIN16_VECTOR( FFX_MIN16_I4, int, 4 );

FFX_MIN16_SCALAR( FFX_MIN16_U,  uint );
FFX_MIN16_VECTOR( FFX_MIN16_U2, uint, 2 );
FFX_MIN16_VECTOR( FFX_MIN16_U3, uint, 3 );
FFX_MIN16_VECTOR( FFX_MIN16_U4, uint, 4 );

FFX_16BIT_SCALAR( FFX_F16_t , float );
FFX_16BIT_VECTOR( FFX_F16_t2, float, 2 );
FFX_16BIT_VECTOR( FFX_F16_t3, float, 3 );
FFX_16BIT_VECTOR( FFX_F16_t4, float, 4 );

FFX_16BIT_SCALAR( FFX_I16_t,  int );
FFX_16BIT_VECTOR( FFX_I16_t2, int, 2 );
FFX_16BIT_VECTOR( FFX_I16_t3, int, 3 );
FFX_16BIT_VECTOR( FFX_I16_t4, int, 4 );

FFX_16BIT_SCALAR( FFX_U16_t,  uint );
FFX_16BIT_VECTOR( FFX_U16_t2, uint, 2 );
FFX_16BIT_VECTOR( FFX_U16_t3, uint, 3 );
FFX_16BIT_VECTOR( FFX_U16_t4, uint, 4 );

#define TYPEDEF_MIN16_TYPES(Prefix)           \
typedef FFX_MIN16_F     Prefix##_F;           \
typedef FFX_MIN16_F2    Prefix##_F2;          \
typedef FFX_MIN16_F3    Prefix##_F3;          \
typedef FFX_MIN16_F4    Prefix##_F4;          \
typedef FFX_MIN16_I     Prefix##_I;           \
typedef FFX_MIN16_I2    Prefix##_I2;          \
typedef FFX_MIN16_I3    Prefix##_I3;          \
typedef FFX_MIN16_I4    Prefix##_I4;          \
typedef FFX_MIN16_U     Prefix##_U;           \
typedef FFX_MIN16_U2    Prefix##_U2;          \
typedef FFX_MIN16_U3    Prefix##_U3;          \
typedef FFX_MIN16_U4    Prefix##_U4;

#define TYPEDEF_16BIT_TYPES(Prefix)           \
typedef FFX_16BIT_F     Prefix##_F;           \
typedef FFX_16BIT_F2    Prefix##_F2;          \
typedef FFX_16BIT_F3    Prefix##_F3;          \
typedef FFX_16BIT_F4    Prefix##_F4;          \
typedef FFX_16BIT_I     Prefix##_I;           \
typedef FFX_16BIT_I2    Prefix##_I2;          \
typedef FFX_16BIT_I3    Prefix##_I3;          \
typedef FFX_16BIT_I4    Prefix##_I4;          \
typedef FFX_16BIT_U     Prefix##_U;           \
typedef FFX_16BIT_U2    Prefix##_U2;          \
typedef FFX_16BIT_U3    Prefix##_U3;          \
typedef FFX_16BIT_U4    Prefix##_U4;

#define TYPEDEF_FULL_PRECISION_TYPES(Prefix)  \
typedef FfxFloat32      Prefix##_F;           \
typedef FfxFloat32x2    Prefix##_F2;          \
typedef FfxFloat32x3    Prefix##_F3;          \
typedef FfxFloat32x4    Prefix##_F4;          \
typedef FfxInt32        Prefix##_I;           \
typedef FfxInt32x2      Prefix##_I2;          \
typedef FfxInt32x3      Prefix##_I3;          \
typedef FfxInt32x4      Prefix##_I4;          \
typedef FfxUInt32       Prefix##_U;           \
typedef FfxUInt32x2     Prefix##_U2;          \
typedef FfxUInt32x3     Prefix##_U3;          \
typedef FfxUInt32x4     Prefix##_U4;
#endif // #if defined(FFX_GPU)

/// A define for abstracting select functionality
///
/// @ingroup HLSLCore
#define FFX_SELECT(cond, arg1, arg2) select(cond, arg1, arg2)

/// A define for abstracting shared memory between shading languages.
///
/// @ingroup HLSLCore
#define FFX_GROUPSHARED groupshared

/// A define for abstracting compute memory barriers between shading languages.
///
/// @ingroup HLSLCore
#define FFX_GROUP_MEMORY_BARRIER GroupMemoryBarrierWithGroupSync()

/// A define for abstracting compute atomic additions between shading languages.
///
/// @ingroup HLSLCore
#define FFX_ATOMIC_ADD(x, y) InterlockedAdd(x, y)

/// A define for abstracting compute atomic additions between shading languages.
///
/// @ingroup HLSLCore
#define FFX_ATOMIC_ADD_RETURN(x, y, r) InterlockedAdd(x, y, r)

/// A define for abstracting compute atomic OR between shading languages.
///
/// @ingroup HLSLCore
#define FFX_ATOMIC_OR(x, y) InterlockedOr(x, y)

/// A define for abstracting compute atomic min between shading languages.
///
/// @ingroup HLSLCore
#define FFX_ATOMIC_MIN(x, y) InterlockedMin(x, y)

/// A define for abstracting compute atomic max between shading languages.
///
/// @ingroup HLSLCore
#define FFX_ATOMIC_MAX(x, y) InterlockedMax(x, y)

/// A define added to accept static markup on functions to aid CPU/GPU portability of code.
///
/// @ingroup HLSLCore
#define FFX_STATIC static

/// A define for abstracting an output function parameter between shading languages.
///
/// @ingroup HLSLCore
#define FFX_PARAMETER_OUT out

/// A define for abstracting loop unrolling between shading languages.
///
/// @ingroup HLSLCore
#define FFX_UNROLL [unroll]

/// A define for abstracting a 'greater than' comparison operator between two types.
///
/// @ingroup HLSLCore
#define FFX_GREATER_THAN(x, y) x > y

/// A define for abstracting a 'greater than or equal' comparison operator between two types.
///
/// @ingroup HLSLCore
#define FFX_GREATER_THAN_EQUAL(x, y) x >= y

/// A define for abstracting a 'less than' comparison operator between two types.
///
/// @ingroup HLSLCore
#define FFX_LESS_THAN(x, y) x < y

/// A define for abstracting a 'less than or equal' comparison operator between two types.
///
/// @ingroup HLSLCore
#define FFX_LESS_THAN_EQUAL(x, y) x <= y

/// A define for abstracting an 'equal' comparison operator between two types.
///
/// @ingroup HLSLCore
#define FFX_EQUAL(x, y) x == y

/// A define for abstracting a 'not equal' comparison operator between two types.
///
/// @ingroup HLSLCore
#define FFX_NOT_EQUAL(x, y) x != y

/// A define for abstracting matrix multiply operations between shading languages.
///
/// @ingroup HLSLCore
#define FFX_MATRIX_MULTIPLY(a, b) mul(a, b)

/// A define for abstracting vector transformations between shading languages.
///
/// @ingroup HLSLCore
#define FFX_TRANSFORM_VECTOR(a, b) mul(a, b)

/// A define for abstracting modulo operations between shading languages.
///
/// @ingroup HLSLCore
#define FFX_MODULO(a, b) (fmod(a, b))

/// Broadcast a scalar value to a 1-dimensional floating point vector.
///
/// @ingroup HLSLCore
#define FFX_BROADCAST_FLOAT32(x) FfxFloat32(x)

/// Broadcast a scalar value to a 2-dimensional floating point vector.
///
/// @ingroup HLSLCore
#define FFX_BROADCAST_FLOAT32X2(x) FfxFloat32(x)

/// Broadcast a scalar value to a 3-dimensional floating point vector.
///
/// @ingroup HLSLCore
#define FFX_BROADCAST_FLOAT32X3(x) FfxFloat32(x)

/// Broadcast a scalar value to a 4-dimensional floating point vector.
///
/// @ingroup HLSLCore
#define FFX_BROADCAST_FLOAT32X4(x) FfxFloat32(x)

/// Broadcast a scalar value to a 1-dimensional unsigned integer vector.
///
/// @ingroup HLSLCore
#define FFX_BROADCAST_UINT32(x) FfxUInt32(x)

/// Broadcast a scalar value to a 2-dimensional unsigned integer vector.
///
/// @ingroup HLSLCore
#define FFX_BROADCAST_UINT32X2(x) FfxUInt32(x)

/// Broadcast a scalar value to a 4-dimensional unsigned integer vector.
///
/// @ingroup HLSLCore
#define FFX_BROADCAST_UINT32X3(x) FfxUInt32(x)

/// Broadcast a scalar value to a 4-dimensional unsigned integer vector.
///
/// @ingroup HLSLCore
#define FFX_BROADCAST_UINT32X4(x) FfxUInt32(x)

/// Broadcast a scalar value to a 1-dimensional signed integer vector.
///
/// @ingroup HLSLCore
#define FFX_BROADCAST_INT32(x) FfxInt32(x)

/// Broadcast a scalar value to a 2-dimensional signed integer vector.
///
/// @ingroup HLSLCore
#define FFX_BROADCAST_INT32X2(x) FfxInt32(x)

/// Broadcast a scalar value to a 3-dimensional signed integer vector.
///
/// @ingroup HLSLCore
#define FFX_BROADCAST_INT32X3(x) FfxInt32(x)

/// Broadcast a scalar value to a 4-dimensional signed integer vector.
///
/// @ingroup HLSLCore
#define FFX_BROADCAST_INT32X4(x) FfxInt32(x)

/// Broadcast a scalar value to a 1-dimensional half-precision floating point vector.
///
/// @ingroup HLSLCore
#define FFX_BROADCAST_MIN_FLOAT16(a)   FFX_MIN16_F(a)

/// Broadcast a scalar value to a 2-dimensional half-precision floating point vector.
///
/// @ingroup HLSLCore
#define FFX_BROADCAST_MIN_FLOAT16X2(a) FFX_MIN16_F(a)

/// Broadcast a scalar value to a 3-dimensional half-precision floating point vector.
///
/// @ingroup HLSLCore
#define FFX_BROADCAST_MIN_FLOAT16X3(a) FFX_MIN16_F(a)

/// Broadcast a scalar value to a 4-dimensional half-precision floating point vector.
///
/// @ingroup HLSLCore
#define FFX_BROADCAST_MIN_FLOAT16X4(a) FFX_MIN16_F(a)

/// Broadcast a scalar value to a 1-dimensional half-precision unsigned integer vector.
///
/// @ingroup HLSLCore
#define FFX_BROADCAST_MIN_UINT16(a)   FFX_MIN16_U(a)

/// Broadcast a scalar value to a 2-dimensional half-precision unsigned integer vector.
///
/// @ingroup HLSLCore
#define FFX_BROADCAST_MIN_UINT16X2(a) FFX_MIN16_U(a)

/// Broadcast a scalar value to a 3-dimensional half-precision unsigned integer vector.
///
/// @ingroup HLSLCore
#define FFX_BROADCAST_MIN_UINT16X3(a) FFX_MIN16_U(a)

/// Broadcast a scalar value to a 4-dimensional half-precision unsigned integer vector.
///
/// @ingroup HLSLCore
#define FFX_BROADCAST_MIN_UINT16X4(a) FFX_MIN16_U(a)

/// Broadcast a scalar value to a 1-dimensional half-precision signed integer vector.
///
/// @ingroup HLSLCore
#define FFX_BROADCAST_MIN_INT16(a)   FFX_MIN16_I(a)

/// Broadcast a scalar value to a 2-dimensional half-precision signed integer vector.
///
/// @ingroup HLSLCore
#define FFX_BROADCAST_MIN_INT16X2(a) FFX_MIN16_I(a)

/// Broadcast a scalar value to a 3-dimensional half-precision signed integer vector.
///
/// @ingroup HLSLCore
#define FFX_BROADCAST_MIN_INT16X3(a) FFX_MIN16_I(a)

/// Broadcast a scalar value to a 4-dimensional half-precision signed integer vector.
///
/// @ingroup HLSLCore
#define FFX_BROADCAST_MIN_INT16X4(a) FFX_MIN16_I(a)

/// Convert FfxFloat32 to half (in lower 16-bits of output).
/// 
/// This function implements the same fast technique that is documented here: ftp://ftp.fox-toolkit.org/pub/fasthalffloatconversion.pdf
/// 
/// The function supports denormals.
/// 
/// Some conversion rules are to make computations possibly "safer" on the GPU,
///  -INF & -NaN -> -65504
///  +INF & +NaN -> +65504
///
/// @param [in] f               The 32bit floating point value to convert.
/// 
/// @returns
/// The closest 16bit floating point value to <c><i>f</i></c>.
/// 
/// @ingroup HLSLCore
#define ffxF32ToF16 f32tof16

/// Pack 2x32-bit floating point values in a single 32bit value.
///
/// This function first converts each component of <c><i>value</i></c> into their nearest 16-bit floating
/// point representation, and then stores the X and Y components in the lower and upper 16 bits of the
/// 32bit unsigned integer respectively.
///
/// @param [in] value               A 2-dimensional floating point value to convert and pack.
///
/// @returns
/// A packed 32bit value containing 2 16bit floating point values.
///
/// @ingroup HLSLCore
FfxUInt32 ffxPackHalf2x16(FfxFloat32x2 value)
{
    return ffxF32ToF16(value.x) | (ffxF32ToF16(value.y) << 16);
}

/// Broadcast a scalar value to a 2-dimensional floating point vector.
///
/// @param [in] value               The value to to broadcast.
///
/// @returns
/// A 2-dimensional floating point vector with <c><i>value</i></c> in each component.
///
/// @ingroup HLSLCore
FfxFloat32x2 ffxBroadcast2(FfxFloat32 value)
{
    return FfxFloat32x2(value, value);
}

/// Broadcast a scalar value to a 3-dimensional floating point vector.
///
/// @param [in] value               The value to to broadcast.
///
/// @returns
/// A 3-dimensional floating point vector with <c><i>value</i></c> in each component.
///
/// @ingroup HLSLCore
FfxFloat32x3 ffxBroadcast3(FfxFloat32 value)
{
    return FfxFloat32x3(value, value, value);
}

/// Broadcast a scalar value to a 4-dimensional floating point vector.
///
/// @param [in] value               The value to to broadcast.
///
/// @returns
/// A 4-dimensional floating point vector with <c><i>value</i></c> in each component.
///
/// @ingroup HLSLCore
FfxFloat32x4 ffxBroadcast4(FfxFloat32 value)
{
    return FfxFloat32x4(value, value, value, value);
}

/// Broadcast a scalar value to a 2-dimensional signed integer vector.
///
/// @param [in] value               The value to to broadcast.
///
/// @returns
/// A 2-dimensional signed integer vector with <c><i>value</i></c> in each component.
///
/// @ingroup HLSLCore
FfxInt32x2 ffxBroadcast2(FfxInt32 value)
{
    return FfxInt32x2(value, value);
}

/// Broadcast a scalar value to a 3-dimensional signed integer vector.
///
/// @param [in] value               The value to to broadcast.
///
/// @returns
/// A 3-dimensional signed integer vector with <c><i>value</i></c> in each component.
///
/// @ingroup HLSLCore
FfxInt32x3 ffxBroadcast3(FfxInt32 value)
{
    return FfxInt32x3(value, value, value);
}

/// Broadcast a scalar value to a 4-dimensional signed integer vector.
///
/// @param [in] value               The value to to broadcast.
///
/// @returns
/// A 4-dimensional signed integer vector with <c><i>value</i></c> in each component.
///
/// @ingroup HLSLCore
FfxInt32x4 ffxBroadcast4(FfxInt32 value)
{
    return FfxInt32x4(value, value, value, value);
}

/// Broadcast a scalar value to a 2-dimensional unsigned integer vector.
///
/// @param [in] value               The value to to broadcast.
///
/// @returns
/// A 2-dimensional unsigned integer vector with <c><i>value</i></c> in each component.
///
/// @ingroup HLSLCore
FfxUInt32x2 ffxBroadcast2(FfxUInt32 value)
{
    return FfxUInt32x2(value, value);
}

/// Broadcast a scalar value to a 3-dimensional unsigned integer vector.
///
/// @param [in] value               The value to to broadcast.
///
/// @returns
/// A 3-dimensional unsigned integer vector with <c><i>value</i></c> in each component.
///
/// @ingroup HLSLCore
FfxUInt32x3 ffxBroadcast3(FfxUInt32 value)
{
    return FfxUInt32x3(value, value, value);
}

/// Broadcast a scalar value to a 4-dimensional unsigned integer vector.
///
/// @param [in] value               The value to to broadcast.
///
/// @returns
/// A 4-dimensional unsigned integer vector with <c><i>value</i></c> in each component.
///
/// @ingroup HLSLCore
FfxUInt32x4 ffxBroadcast4(FfxUInt32 value)
{
    return FfxUInt32x4(value, value, value, value);
}

FfxUInt32 ffxBitfieldExtract(FfxUInt32 src, FfxUInt32 off, FfxUInt32 bits)
{
    FfxUInt32 mask = (1u << bits) - 1;
    return (src >> off) & mask;
}

FfxUInt32 ffxBitfieldInsert(FfxUInt32 src, FfxUInt32 ins, FfxUInt32 mask)
{
    return (ins & mask) | (src & (~mask));
}

FfxUInt32 ffxBitfieldInsertMask(FfxUInt32 src, FfxUInt32 ins, FfxUInt32 bits)
{
    FfxUInt32 mask = (1u << bits) - 1;
    return (ins & mask) | (src & (~mask));
}

/// Interprets the bit pattern of x as an unsigned integer.
///
/// @param [in] x               The input value.
///
/// @returns
/// The input interpreted as an unsigned integer.
///
/// @ingroup HLSLCore
FfxUInt32 ffxAsUInt32(FfxFloat32 x)
{
    return asuint(x);
}

/// Interprets the bit pattern of x as an unsigned integer.
///
/// @param [in] x               The input value.
///
/// @returns
/// The input interpreted as an unsigned integer.
///
/// @ingroup HLSLCore
FfxUInt32x2 ffxAsUInt32(FfxFloat32x2 x)
{
    return asuint(x);
}

/// Interprets the bit pattern of x as an unsigned integer.
///
/// @param [in] x               The input value.
///
/// @returns
/// The input interpreted as an unsigned integer.
///
/// @ingroup HLSLCore
FfxUInt32x3 ffxAsUInt32(FfxFloat32x3 x)
{
    return asuint(x);
}

/// Interprets the bit pattern of x as an unsigned integer.
///
/// @param [in] x               The input value.
///
/// @returns
/// The input interpreted as an unsigned integer.
///
/// @ingroup HLSLCore
FfxUInt32x4 ffxAsUInt32(FfxFloat32x4 x)
{
    return asuint(x);
}

/// Interprets the bit pattern of x as a floating-point number.
///
/// @param [in] x               The input value.
///
/// @returns
/// The input interpreted as a floating-point number.
///
/// @ingroup HLSLCore
FfxFloat32 ffxAsFloat(FfxUInt32 x)
{
    return asfloat(x);
}

/// Interprets the bit pattern of x as a floating-point number.
///
/// @param [in] x               The input value.
///
/// @returns
/// The input interpreted as a floating-point number.
///
/// @ingroup HLSLCore
FfxFloat32x2 ffxAsFloat(FfxUInt32x2 x)
{
    return asfloat(x);
}

/// Interprets the bit pattern of x as a floating-point number.
///
/// @param [in] x               The input value.
///
/// @returns
/// The input interpreted as a floating-point number.
///
/// @ingroup HLSLCore
FfxFloat32x3 ffxAsFloat(FfxUInt32x3 x)
{
    return asfloat(x);
}

/// Interprets the bit pattern of x as a floating-point number.
///
/// @param [in] x               The input value.
///
/// @returns
/// The input interpreted as a floating-point number.
///
/// @ingroup HLSLCore
FfxFloat32x4 ffxAsFloat(FfxUInt32x4 x)
{
    return asfloat(x);
}

/// Compute the inverse of a value.
///
/// @param [in] x                   The value to calulate the inverse of.
///
/// @returns
/// The inverse of <c><i>x</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32 ffxReciprocal(FfxFloat32 x)
{
    return rcp(x);
}

/// Compute the inverse of a value.
///
/// @param [in] x                   The value to calulate the inverse of.
///
/// @returns
/// The inverse of <c><i>x</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32x2 ffxReciprocal(FfxFloat32x2 x)
{
    return rcp(x);
}

/// Compute the inverse of a value.
///
/// @param [in] x                   The value to calulate the inverse of.
///
/// @returns
/// The inverse of <c><i>x</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32x3 ffxReciprocal(FfxFloat32x3 x)
{
    return rcp(x);
}

/// Compute the inverse of a value.
///
/// @param [in] x                   The value to calulate the inverse of.
///
/// @returns
/// The inverse of <c><i>x</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32x4 ffxReciprocal(FfxFloat32x4 x)
{
    return rcp(x);
}

/// Compute the inverse square root of a value.
///
/// @param [in] x                   The value to calulate the inverse square root of.
///
/// @returns
/// The inverse square root of <c><i>x</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32 ffxRsqrt(FfxFloat32 x)
{
    return rsqrt(x);
}

/// Compute the inverse square root of a value.
///
/// @param [in] x                   The value to calulate the inverse square root of.
///
/// @returns
/// The inverse square root of <c><i>x</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32x2 ffxRsqrt(FfxFloat32x2 x)
{
    return rsqrt(x);
}

/// Compute the inverse square root of a value.
///
/// @param [in] x                   The value to calulate the inverse square root of.
///
/// @returns
/// The inverse square root of <c><i>x</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32x3 ffxRsqrt(FfxFloat32x3 x)
{
    return rsqrt(x);
}

/// Compute the inverse square root of a value.
///
/// @param [in] x                   The value to calulate the inverse square root of.
///
/// @returns
/// The inverse square root of <c><i>x</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32x4 ffxRsqrt(FfxFloat32x4 x)
{
    return rsqrt(x);
}

/// Compute the linear interopation between two values.
///
/// Implemented by calling the HLSL <c><i>mix</i></c> instrinsic function. Implements the
/// following math:
///
///     (1 - t) * x + t * y
///
/// @param [in] x               The first value to lerp between.
/// @param [in] y               The second value to lerp between.
/// @param [in] t               The value to determine how much of <c><i>x</i></c> and how much of <c><i>y</i></c>.
///
/// @returns
/// A linearly interpolated value between <c><i>x</i></c> and <c><i>y</i></c> according to <c><i>t</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32 ffxLerp(FfxFloat32 x, FfxFloat32 y, FfxFloat32 t)
{
    return lerp(x, y, t);
}

/// Compute the linear interopation between two values.
///
/// Implemented by calling the HLSL <c><i>mix</i></c> instrinsic function. Implements the
/// following math:
///
///     (1 - t) * x + t * y
///
/// @param [in] x               The first value to lerp between.
/// @param [in] y               The second value to lerp between.
/// @param [in] t               The value to determine how much of <c><i>x</i></c> and how much of <c><i>y</i></c>.
///
/// @returns
/// A linearly interpolated value between <c><i>x</i></c> and <c><i>y</i></c> according to <c><i>t</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32x2 ffxLerp(FfxFloat32x2 x, FfxFloat32x2 y, FfxFloat32 t)
{
    return lerp(x, y, t);
}

/// Compute the linear interopation between two values.
///
/// Implemented by calling the HLSL <c><i>mix</i></c> instrinsic function. Implements the
/// following math:
///
///     (1 - t) * x + t * y
///
/// @param [in] x               The first value to lerp between.
/// @param [in] y               The second value to lerp between.
/// @param [in] t               The value to determine how much of <c><i>x</i></c> and how much of <c><i>y</i></c>.
///
/// @returns
/// A linearly interpolated value between <c><i>x</i></c> and <c><i>y</i></c> according to <c><i>t</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32x2 ffxLerp(FfxFloat32x2 x, FfxFloat32x2 y, FfxFloat32x2 t)
{
    return lerp(x, y, t);
}

/// Compute the linear interopation between two values.
///
/// Implemented by calling the HLSL <c><i>mix</i></c> instrinsic function. Implements the
/// following math:
///
///     (1 - t) * x + t * y
///
/// @param [in] x               The first value to lerp between.
/// @param [in] y               The second value to lerp between.
/// @param [in] t               The value to determine how much of <c><i>x</i></c> and how much of <c><i>y</i></c>.
///
/// @returns
/// A linearly interpolated value between <c><i>x</i></c> and <c><i>y</i></c> according to <c><i>t</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32x3 ffxLerp(FfxFloat32x3 x, FfxFloat32x3 y, FfxFloat32 t)
{
    return lerp(x, y, t);
}

/// Compute the linear interopation between two values.
///
/// Implemented by calling the HLSL <c><i>mix</i></c> instrinsic function. Implements the
/// following math:
///
///     (1 - t) * x + t * y
///
/// @param [in] x               The first value to lerp between.
/// @param [in] y               The second value to lerp between.
/// @param [in] t               The value to determine how much of <c><i>x</i></c> and how much of <c><i>y</i></c>.
///
/// @returns
/// A linearly interpolated value between <c><i>x</i></c> and <c><i>y</i></c> according to <c><i>t</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32x3 ffxLerp(FfxFloat32x3 x, FfxFloat32x3 y, FfxFloat32x3 t)
{
    return lerp(x, y, t);
}

/// Compute the linear interopation between two values.
///
/// Implemented by calling the HLSL <c><i>mix</i></c> instrinsic function. Implements the
/// following math:
///
///     (1 - t) * x + t * y
///
/// @param [in] x               The first value to lerp between.
/// @param [in] y               The second value to lerp between.
/// @param [in] t               The value to determine how much of <c><i>x</i></c> and how much of <c><i>y</i></c>.
///
/// @returns
/// A linearly interpolated value between <c><i>x</i></c> and <c><i>y</i></c> according to <c><i>t</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32x4 ffxLerp(FfxFloat32x4 x, FfxFloat32x4 y, FfxFloat32 t)
{
    return lerp(x, y, t);
}

/// Compute the linear interopation between two values.
///
/// Implemented by calling the HLSL <c><i>mix</i></c> instrinsic function. Implements the
/// following math:
///
///     (1 - t) * x + t * y
///
/// @param [in] x               The first value to lerp between.
/// @param [in] y               The second value to lerp between.
/// @param [in] t               The value to determine how much of <c><i>x</i></c> and how much of <c><i>y</i></c>.
///
/// @returns
/// A linearly interpolated value between <c><i>x</i></c> and <c><i>y</i></c> according to <c><i>t</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32x4 ffxLerp(FfxFloat32x4 x, FfxFloat32x4 y, FfxFloat32x4 t)
{
    return lerp(x, y, t);
}

/// Clamp a value to a [0..1] range.
///
/// @param [in] x               The value to clamp to [0..1] range.
///
/// @returns
/// The clamped version of <c><i>x</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32 ffxSaturate(FfxFloat32 x)
{
    return saturate(x);
}

/// Clamp a value to a [0..1] range.
///
/// @param [in] x               The value to clamp to [0..1] range.
///
/// @returns
/// The clamped version of <c><i>x</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32x2 ffxSaturate(FfxFloat32x2 x)
{
    return saturate(x);
}

/// Clamp a value to a [0..1] range.
///
/// @param [in] x               The value to clamp to [0..1] range.
///
/// @returns
/// The clamped version of <c><i>x</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32x3 ffxSaturate(FfxFloat32x3 x)
{
    return saturate(x);
}

/// Clamp a value to a [0..1] range.
///
/// @param [in] x               The value to clamp to [0..1] range.
///
/// @returns
/// The clamped version of <c><i>x</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32x4 ffxSaturate(FfxFloat32x4 x)
{
    return saturate(x);
}

/// Compute the factional part of a decimal value.
///
/// This function calculates <c><i>x - floor(x)</i></c>. Where <c><i>floor</i></c> is the intrinsic HLSL function.
///
/// NOTE: This function should compile down to a single <c><i>V_MAX3_F32</i></c> operation on GCN/RDNA hardware. It is
/// worth further noting that this function is intentionally distinct from the HLSL <c><i>frac</i></c> intrinsic
/// function.
///
/// @param [in] x               The value to compute the fractional part from.
///
/// @returns
/// The fractional part of <c><i>x</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32 ffxFract(FfxFloat32 x)
{
    return x - floor(x);
}

/// Compute the factional part of a decimal value.
///
/// This function calculates <c><i>x - floor(x)</i></c>. Where <c><i>floor</i></c> is the intrinsic HLSL function.
///
/// NOTE: This function should compile down to a single <c><i>V_MAX3_F32</i></c> operation on GCN/RDNA hardware. It is
/// worth further noting that this function is intentionally distinct from the HLSL <c><i>frac</i></c> intrinsic
/// function.
///
/// @param [in] x               The value to compute the fractional part from.
///
/// @returns
/// The fractional part of <c><i>x</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32x2 ffxFract(FfxFloat32x2 x)
{
    return x - floor(x);
}

/// Compute the factional part of a decimal value.
///
/// This function calculates <c><i>x - floor(x)</i></c>. Where <c><i>floor</i></c> is the intrinsic HLSL function.
///
/// NOTE: This function should compile down to a single <c><i>V_MAX3_F32</i></c> operation on GCN/RDNA hardware. It is
/// worth further noting that this function is intentionally distinct from the HLSL <c><i>frac</i></c> intrinsic
/// function.
///
/// @param [in] x               The value to compute the fractional part from.
///
/// @returns
/// The fractional part of <c><i>x</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32x3 ffxFract(FfxFloat32x3 x)
{
    return x - floor(x);
}

/// Compute the factional part of a decimal value.
///
/// This function calculates <c><i>x - floor(x)</i></c>. Where <c><i>floor</i></c> is the intrinsic HLSL function.
///
/// NOTE: This function should compile down to a single <c><i>V_MAX3_F32</i></c> operation on GCN/RDNA hardware. It is
/// worth further noting that this function is intentionally distinct from the HLSL <c><i>frac</i></c> intrinsic
/// function.
///
/// @param [in] x               The value to compute the fractional part from.
///
/// @returns
/// The fractional part of <c><i>x</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32x4 ffxFract(FfxFloat32x4 x)
{
    return x - floor(x);
}

/// Rounds to the nearest integer. In case the fractional part is 0.5, it will round to the nearest even integer.
///
/// @param [in] x               The value to be rounded.
///
/// @returns
/// The nearest integer from <c><i>x</i></c>. The nearest even integer from <c><i>x</i></c> if equidistant from 2 integer.
///
/// @ingroup HLSLCore
FfxFloat32 ffxRound(FfxFloat32 x)
{
    return round(x);
}

/// Rounds to the nearest integer. In case the fractional part is 0.5, it will round to the nearest even integer.
///
/// @param [in] x               The value to be rounded.
///
/// @returns
/// The nearest integer from <c><i>x</i></c>. The nearest even integer from <c><i>x</i></c> if equidistant from 2 integer.
///
/// @ingroup HLSLCore
FfxFloat32x2 ffxRound(FfxFloat32x2 x)
{
    return round(x);
}

/// Rounds to the nearest integer. In case the fractional part is 0.5, it will round to the nearest even integer.
///
/// @param [in] x               The value to be rounded.
///
/// @returns
/// The nearest integer from <c><i>x</i></c>. The nearest even integer from <c><i>x</i></c> if equidistant from 2 integer.
///
/// @ingroup HLSLCore
FfxFloat32x3 ffxRound(FfxFloat32x3 x)
{
    return round(x);
}

/// Rounds to the nearest integer. In case the fractional part is 0.5, it will round to the nearest even integer.
///
/// @param [in] x               The value to be rounded.
///
/// @returns
/// The nearest integer from <c><i>x</i></c>. The nearest even integer from <c><i>x</i></c> if equidistant from 2 integer.
///
/// @ingroup HLSLCore
FfxFloat32x4 ffxRound(FfxFloat32x4 x)
{
    return round(x);
}

/// Compute the maximum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MAX3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the max calculation.
/// @param [in] y               The second value to include in the max calcuation.
/// @param [in] z               The third value to include in the max calcuation.
///
/// @returns
/// The maximum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32 ffxMax3(FfxFloat32 x, FfxFloat32 y, FfxFloat32 z)
{
    return max(x, max(y, z));
}

/// Compute the maximum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MAX3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the max calculation.
/// @param [in] y               The second value to include in the max calcuation.
/// @param [in] z               The third value to include in the max calcuation.
///
/// @returns
/// The maximum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32x2 ffxMax3(FfxFloat32x2 x, FfxFloat32x2 y, FfxFloat32x2 z)
{
    return max(x, max(y, z));
}

/// Compute the maximum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MAX3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the max calculation.
/// @param [in] y               The second value to include in the max calcuation.
/// @param [in] z               The third value to include in the max calcuation.
///
/// @returns
/// The maximum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32x3 ffxMax3(FfxFloat32x3 x, FfxFloat32x3 y, FfxFloat32x3 z)
{
    return max(x, max(y, z));
}

/// Compute the maximum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MAX3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the max calculation.
/// @param [in] y               The second value to include in the max calcuation.
/// @param [in] z               The third value to include in the max calcuation.
///
/// @returns
/// The maximum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32x4 ffxMax3(FfxFloat32x4 x, FfxFloat32x4 y, FfxFloat32x4 z)
{
    return max(x, max(y, z));
}

/// Compute the maximum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MAX3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the max calculation.
/// @param [in] y               The second value to include in the max calcuation.
/// @param [in] z               The third value to include in the max calcuation.
///
/// @returns
/// The maximum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSLCore
FfxUInt32 ffxMax3(FfxUInt32 x, FfxUInt32 y, FfxUInt32 z)
{
    return max(x, max(y, z));
}

/// Compute the maximum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MAX3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the max calculation.
/// @param [in] y               The second value to include in the max calcuation.
/// @param [in] z               The third value to include in the max calcuation.
///
/// @returns
/// The maximum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSLCore
FfxUInt32x2 ffxMax3(FfxUInt32x2 x, FfxUInt32x2 y, FfxUInt32x2 z)
{
    return max(x, max(y, z));
}

/// Compute the maximum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MAX3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the max calculation.
/// @param [in] y               The second value to include in the max calcuation.
/// @param [in] z               The third value to include in the max calcuation.
///
/// @returns
/// The maximum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSLCore
FfxUInt32x3 ffxMax3(FfxUInt32x3 x, FfxUInt32x3 y, FfxUInt32x3 z)
{
    return max(x, max(y, z));
}

/// Compute the maximum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MAX3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the max calculation.
/// @param [in] y               The second value to include in the max calcuation.
/// @param [in] z               The third value to include in the max calcuation.
///
/// @returns
/// The maximum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSLCore
FfxUInt32x4 ffxMax3(FfxUInt32x4 x, FfxUInt32x4 y, FfxUInt32x4 z)
{
    return max(x, max(y, z));
}

/// Compute the median of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MED3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the median calculation.
/// @param [in] y               The second value to include in the median calcuation.
/// @param [in] z               The third value to include in the median calcuation.
///
/// @returns
/// The median value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32 ffxMed3(FfxFloat32 x, FfxFloat32 y, FfxFloat32 z)
{
    return max(min(x, y), min(max(x, y), z));
}

/// Compute the median of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MED3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the median calculation.
/// @param [in] y               The second value to include in the median calcuation.
/// @param [in] z               The third value to include in the median calcuation.
///
/// @returns
/// The median value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32x2 ffxMed3(FfxFloat32x2 x, FfxFloat32x2 y, FfxFloat32x2 z)
{
    return max(min(x, y), min(max(x, y), z));
}

/// Compute the median of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MED3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the median calculation.
/// @param [in] y               The second value to include in the median calcuation.
/// @param [in] z               The third value to include in the median calcuation.
///
/// @returns
/// The median value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32x3 ffxMed3(FfxFloat32x3 x, FfxFloat32x3 y, FfxFloat32x3 z)
{
    return max(min(x, y), min(max(x, y), z));
}

/// Compute the median of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MED3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the median calculation.
/// @param [in] y               The second value to include in the median calcuation.
/// @param [in] z               The third value to include in the median calcuation.
///
/// @returns
/// The median value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32x4 ffxMed3(FfxFloat32x4 x, FfxFloat32x4 y, FfxFloat32x4 z)
{
    return max(min(x, y), min(max(x, y), z));
}

/// Compute the median of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MED3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the median calculation.
/// @param [in] y               The second value to include in the median calcuation.
/// @param [in] z               The third value to include in the median calcuation.
///
/// @returns
/// The median value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSL
FfxInt32 ffxMed3(FfxInt32 x, FfxInt32 y, FfxInt32 z)
{
    return max(min(x, y), min(max(x, y), z));
    // return min(max(min(y, z), x), max(y, z));
    // return max(max(x, y), z) == x ? max(y, z) : (max(max(x, y), z) == y ? max(x, z) : max(x, y));
}

/// Compute the median of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MED3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the median calculation.
/// @param [in] y               The second value to include in the median calcuation.
/// @param [in] z               The third value to include in the median calcuation.
///
/// @returns
/// The median value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSL
FfxInt32x2 ffxMed3(FfxInt32x2 x, FfxInt32x2 y, FfxInt32x2 z)
{
    return max(min(x, y), min(max(x, y), z));
    // return min(max(min(y, z), x), max(y, z));
    // return max(max(x, y), z) == x ? max(y, z) : (max(max(x, y), z) == y ? max(x, z) : max(x, y));
}

/// Compute the median of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MED3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the median calculation.
/// @param [in] y               The second value to include in the median calcuation.
/// @param [in] z               The third value to include in the median calcuation.
///
/// @returns
/// The median value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSL
FfxInt32x3 ffxMed3(FfxInt32x3 x, FfxInt32x3 y, FfxInt32x3 z)
{
    return max(min(x, y), min(max(x, y), z));
}

/// Compute the median of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MED3_I32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the median calculation.
/// @param [in] y               The second value to include in the median calcuation.
/// @param [in] z               The third value to include in the median calcuation.
///
/// @returns
/// The median value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSL
FfxInt32x4 ffxMed3(FfxInt32x4 x, FfxInt32x4 y, FfxInt32x4 z)
{
    return max(min(x, y), min(max(x, y), z));
}

/// Compute the minimum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MIN3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the min calculation.
/// @param [in] y               The second value to include in the min calcuation.
/// @param [in] z               The third value to include in the min calcuation.
///
/// @returns
/// The minimum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32 ffxMin3(FfxFloat32 x, FfxFloat32 y, FfxFloat32 z)
{
    return min(x, min(y, z));
}

/// Compute the minimum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MIN3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the min calculation.
/// @param [in] y               The second value to include in the min calcuation.
/// @param [in] z               The third value to include in the min calcuation.
///
/// @returns
/// The minimum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32x2 ffxMin3(FfxFloat32x2 x, FfxFloat32x2 y, FfxFloat32x2 z)
{
    return min(x, min(y, z));
}

/// Compute the minimum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MIN3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the min calculation.
/// @param [in] y               The second value to include in the min calcuation.
/// @param [in] z               The third value to include in the min calcuation.
///
/// @returns
/// The minimum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32x3 ffxMin3(FfxFloat32x3 x, FfxFloat32x3 y, FfxFloat32x3 z)
{
    return min(x, min(y, z));
}

/// Compute the minimum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MIN3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the min calculation.
/// @param [in] y               The second value to include in the min calcuation.
/// @param [in] z               The third value to include in the min calcuation.
///
/// @returns
/// The minimum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSLCore
FfxFloat32x4 ffxMin3(FfxFloat32x4 x, FfxFloat32x4 y, FfxFloat32x4 z)
{
    return min(x, min(y, z));
}

/// Compute the minimum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MIN3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the min calculation.
/// @param [in] y               The second value to include in the min calcuation.
/// @param [in] z               The third value to include in the min calcuation.
///
/// @returns
/// The minimum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSLCore
FfxUInt32 ffxMin3(FfxUInt32 x, FfxUInt32 y, FfxUInt32 z)
{
    return min(x, min(y, z));
}

/// Compute the minimum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MIN3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the min calculation.
/// @param [in] y               The second value to include in the min calcuation.
/// @param [in] z               The third value to include in the min calcuation.
///
/// @returns
/// The minimum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSLCore
FfxUInt32x2 ffxMin3(FfxUInt32x2 x, FfxUInt32x2 y, FfxUInt32x2 z)
{
    return min(x, min(y, z));
}

/// Compute the minimum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MIN3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the min calculation.
/// @param [in] y               The second value to include in the min calculation.
/// @param [in] z               The third value to include in the min calculation.
///
/// @returns
/// The minimum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSLCore
FfxUInt32x3 ffxMin3(FfxUInt32x3 x, FfxUInt32x3 y, FfxUInt32x3 z)
{
    return min(x, min(y, z));
}

/// Compute the minimum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MIN3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the min calculation.
/// @param [in] y               The second value to include in the min calcuation.
/// @param [in] z               The third value to include in the min calcuation.
///
/// @returns
/// The minimum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSLCore
FfxUInt32x4 ffxMin3(FfxUInt32x4 x, FfxUInt32x4 y, FfxUInt32x4 z)
{
    return min(x, min(y, z));
}


FfxUInt32 ffxAShrSU1(FfxUInt32 a, FfxUInt32 b)
{
    return FfxUInt32(FfxInt32(a) >> FfxInt32(b));
}

FfxUInt32 ffxPackF32(FfxFloat32x2 v)
{
    FfxUInt32x2 p = FfxUInt32x2(ffxF32ToF16(FfxFloat32x2(v).x), ffxF32ToF16(FfxFloat32x2(v).y));
    return p.x | (p.y << 16);
}

FfxFloat32x2 ffxUnpackF32(FfxUInt32 a)
{
    return f16tof32(FfxUInt32x2(a & 0xFFFF, a >> 16));
}

FfxUInt32x2 ffxPackF32x2(FfxFloat32x4 v)
{
    return FfxUInt32x2(ffxPackF32(v.xy), ffxPackF32(v.zw));
}

FfxFloat32x4 ffxUnpackF32x2(FfxUInt32x2 a)
{
    return FfxFloat32x4(ffxUnpackF32(a.x), ffxUnpackF32(a.y));
}

//==============================================================================================================================
//                                                          HLSL HALF
//==============================================================================================================================
//==============================================================================================================================
// Need to use manual unpack to get optimal execution (don't use packed types in buffers directly).
// Unpack requires this pattern: https://gpuopen.com/first-steps-implementing-fp16/
FFX_MIN16_F2 ffxUint32ToFloat16x2(FfxUInt32 x)
{
    FfxFloat32x2 t = f16tof32(FfxUInt32x2(x & 0xFFFF, x >> 16));
    return FFX_MIN16_F2(t);
}
FFX_MIN16_F4 ffxUint32x2ToFloat16x4(FfxUInt32x2 x)
{
    return FFX_MIN16_F4(ffxUint32ToFloat16x2(x.x), ffxUint32ToFloat16x2(x.y));
}
FFX_MIN16_U2 ffxUint32ToUint16x2(FfxUInt32 x)
{
    FfxUInt32x2 t = FfxUInt32x2(x & 0xFFFF, x >> 16);
    return FFX_MIN16_U2(t);
}
FFX_MIN16_U4 ffxUint32x2ToUint16x4(FfxUInt32x2 x)
{
    return FFX_MIN16_U4(ffxUint32ToUint16x2(x.x), ffxUint32ToUint16x2(x.y));
}

FfxUInt32x2 ffxFloat16x4ToUint32x2(FFX_MIN16_F4 v)
{
    FfxUInt32x2 result;
    result.x = ffxF32ToF16(v.x) | (ffxF32ToF16(v.y) << 16);
    result.y = ffxF32ToF16(v.z) | (ffxF32ToF16(v.w) << 16);
    return result;
}

/// @brief Inverts the value while avoiding division by zero. If the value is zero, zero is returned.
/// @param v Value to invert.
/// @return If v = 0 returns 0. If v != 0 returns 1/v.
FfxFloat32 ffxInvertSafe(FfxFloat32 v)
{
    FfxFloat32 s = FfxFloat32(sign(v));
    FfxFloat32 s2 = s * s;
    return s2 / (v + s2 - 1.0);
}

/// @brief Inverts the value while avoiding division by zero. If the value is zero, zero is returned.
/// @param v Value to invert.
/// @return If v = 0 returns 0. If v != 0 returns 1/v.
FfxFloat32x2 ffxInvertSafe(FfxFloat32x2 v)
{
    FfxFloat32x2 s = FfxFloat32x2(sign(v));
    FfxFloat32x2 s2 = s * s;
    return s2 / (v + s2 - FfxFloat32x2(1.0, 1.0));
}

/// @brief Inverts the value while avoiding division by zero. If the value is zero, zero is returned.
/// @param v Value to invert.
/// @return If v = 0 returns 0. If v != 0 returns 1/v.
FfxFloat32x3 ffxInvertSafe(FfxFloat32x3 v)
{
    FfxFloat32x3 s = FfxFloat32x3(sign(v));
    FfxFloat32x3 s2 = s * s;
    return s2 / (v + s2 - FfxFloat32x3(1.0, 1.0, 1.0));
}

/// @brief Inverts the value while avoiding division by zero. If the value is zero, zero is returned.
/// @param v Value to invert.
/// @return If v = 0 returns 0. If v != 0 returns 1/v.
FfxFloat32x4 ffxInvertSafe(FfxFloat32x4 v)
{
    FfxFloat32x4 s = FfxFloat32x4(sign(v));
    FfxFloat32x4 s2 = s * s;
    return s2 / (v + s2 - FfxFloat32x4(1.0, 1.0, 1.0, 1.0));
}

#define FFX_UINT32_TO_FLOAT16X2(x) ffxUint32ToFloat16x2(FfxUInt32(x))
#if FFX_HALF

#define FFX_UINT32X2_TO_FLOAT16X4(x) ffxUint32x2ToFloat16x4(FfxUInt32x2(x))
#define FFX_UINT32_TO_UINT16X2(x) ffxUint32ToUint16x2(FfxUInt32(x))
#define FFX_UINT32X2_TO_UINT16X4(x) ffxUint32x2ToUint16x4(FfxUInt32x2(x))

FfxUInt32 ffxPackF16(FfxFloat16x2 v)
{
    FfxUInt32x2 p = FfxUInt32x2(ffxF32ToF16(FfxFloat32x2(v).x), ffxF32ToF16(FfxFloat32x2(v).y));
    return p.x | (p.y << 16);
}

FfxFloat16x2 ffxUnpackF16(FfxUInt32 a)
{
    return FfxFloat16x2(f16tof32(FfxUInt32x2(a & 0xFFFF, a >> 16)));
}

//------------------------------------------------------------------------------------------------------------------------------
FfxUInt32 FFX_MIN16_F2ToUint32(FFX_MIN16_F2 x)
{
    return ffxF32ToF16(x.x) + (ffxF32ToF16(x.y) << 16);
}
FfxUInt32x2 FFX_MIN16_F4ToUint32x2(FFX_MIN16_F4 x)
{
    return FfxUInt32x2(FFX_MIN16_F2ToUint32(x.xy), FFX_MIN16_F2ToUint32(x.zw));
}
FfxUInt32 FFX_MIN16_U2ToUint32(FFX_MIN16_U2 x)
{
    return FfxUInt32(x.x) + (FfxUInt32(x.y) << 16);
}
FfxUInt32x2 FFX_MIN16_U4ToUint32x2(FFX_MIN16_U4 x)
{
    return FfxUInt32x2(FFX_MIN16_U2ToUint32(x.xy), FFX_MIN16_U2ToUint32(x.zw));
}
#define FFX_FLOAT16X2_TO_UINT32(x) FFX_MIN16_F2ToUint32(FFX_MIN16_F2(x))
#define FFX_FLOAT16X4_TO_UINT32X2(x) FFX_MIN16_F4ToUint32x2(FFX_MIN16_F4(x))
#define FFX_UINT16X2_TO_UINT32(x) FFX_MIN16_U2ToUint32(FFX_MIN16_U2(x))
#define FFX_UINT16X4_TO_UINT32X2(x) FFX_MIN16_U4ToUint32x2(FFX_MIN16_U4(x))

#if (FFX_HLSL_SM >= 62) && !defined(FFX_NO_16_BIT_CAST)
#define FFX_TO_UINT16(x) asuint16(x)
#define FFX_TO_UINT16X2(x) asuint16(x)
#define FFX_TO_UINT16X3(x) asuint16(x)
#define FFX_TO_UINT16X4(x) asuint16(x)
#else
#define FFX_TO_UINT16(a) FFX_MIN16_U(ffxF32ToF16(FfxFloat32(a)))
#define FFX_TO_UINT16X2(a) FFX_MIN16_U2(FFX_TO_UINT16((a).x), FFX_TO_UINT16((a).y))
#define FFX_TO_UINT16X3(a) FFX_MIN16_U3(FFX_TO_UINT16((a).x), FFX_TO_UINT16((a).y), FFX_TO_UINT16((a).z))
#define FFX_TO_UINT16X4(a) FFX_MIN16_U4(FFX_TO_UINT16((a).x), FFX_TO_UINT16((a).y), FFX_TO_UINT16((a).z), FFX_TO_UINT16((a).w))
#endif // #if (FFX_HLSL_SM>=62) && !defined(FFX_NO_16_BIT_CAST)

#if (FFX_HLSL_SM >= 62) && !defined(FFX_NO_16_BIT_CAST)
#define FFX_TO_FLOAT16(x) asfloat16(x)
#define FFX_TO_FLOAT16X2(x) asfloat16(x)
#define FFX_TO_FLOAT16X3(x) asfloat16(x)
#define FFX_TO_FLOAT16X4(x) asfloat16(x)
#else
#define FFX_TO_FLOAT16(a) FFX_MIN16_F(f16tof32(FfxUInt32(a)))
#define FFX_TO_FLOAT16X2(a) FFX_MIN16_F2(FFX_TO_FLOAT16((a).x), FFX_TO_FLOAT16((a).y))
#define FFX_TO_FLOAT16X3(a) FFX_MIN16_F3(FFX_TO_FLOAT16((a).x), FFX_TO_FLOAT16((a).y), FFX_TO_FLOAT16((a).z))
#define FFX_TO_FLOAT16X4(a) FFX_MIN16_F4(FFX_TO_FLOAT16((a).x), FFX_TO_FLOAT16((a).y), FFX_TO_FLOAT16((a).z), FFX_TO_FLOAT16((a).w))
#endif // #if (FFX_HLSL_SM>=62) && !defined(FFX_NO_16_BIT_CAST)

//==============================================================================================================================
#define FFX_BROADCAST_FLOAT16(a)   FFX_MIN16_F(a)
#define FFX_BROADCAST_FLOAT16X2(a) FFX_MIN16_F(a)
#define FFX_BROADCAST_FLOAT16X3(a) FFX_MIN16_F(a)
#define FFX_BROADCAST_FLOAT16X4(a) FFX_MIN16_F(a)

//------------------------------------------------------------------------------------------------------------------------------
#define FFX_BROADCAST_INT16(a)   FFX_MIN16_I(a)
#define FFX_BROADCAST_INT16X2(a) FFX_MIN16_I(a)
#define FFX_BROADCAST_INT16X3(a) FFX_MIN16_I(a)
#define FFX_BROADCAST_INT16X4(a) FFX_MIN16_I(a)

//------------------------------------------------------------------------------------------------------------------------------
#define FFX_BROADCAST_UINT16(a)   FFX_MIN16_U(a)
#define FFX_BROADCAST_UINT16X2(a) FFX_MIN16_U(a)
#define FFX_BROADCAST_UINT16X3(a) FFX_MIN16_U(a)
#define FFX_BROADCAST_UINT16X4(a) FFX_MIN16_U(a)

//==============================================================================================================================
FFX_MIN16_U ffxAbsHalf(FFX_MIN16_U a)
{
    return FFX_MIN16_U(abs(FFX_MIN16_I(a)));
}
FFX_MIN16_U2 ffxAbsHalf(FFX_MIN16_U2 a)
{
    return FFX_MIN16_U2(abs(FFX_MIN16_I2(a)));
}
FFX_MIN16_U3 ffxAbsHalf(FFX_MIN16_U3 a)
{
    return FFX_MIN16_U3(abs(FFX_MIN16_I3(a)));
}
FFX_MIN16_U4 ffxAbsHalf(FFX_MIN16_U4 a)
{
    return FFX_MIN16_U4(abs(FFX_MIN16_I4(a)));
}
//------------------------------------------------------------------------------------------------------------------------------
FFX_MIN16_F ffxClampHalf(FFX_MIN16_F x, FFX_MIN16_F n, FFX_MIN16_F m)
{
    return max(n, min(x, m));
}
FFX_MIN16_F2 ffxClampHalf(FFX_MIN16_F2 x, FFX_MIN16_F2 n, FFX_MIN16_F2 m)
{
    return max(n, min(x, m));
}
FFX_MIN16_F3 ffxClampHalf(FFX_MIN16_F3 x, FFX_MIN16_F3 n, FFX_MIN16_F3 m)
{
    return max(n, min(x, m));
}
FFX_MIN16_F4 ffxClampHalf(FFX_MIN16_F4 x, FFX_MIN16_F4 n, FFX_MIN16_F4 m)
{
    return max(n, min(x, m));
}
//------------------------------------------------------------------------------------------------------------------------------
// V_FRACT_F16 (note DX frac() is different).
FFX_MIN16_F ffxFract(FFX_MIN16_F x)
{
    return x - floor(x);
}
FFX_MIN16_F2 ffxFract(FFX_MIN16_F2 x)
{
    return x - floor(x);
}
FFX_MIN16_F3 ffxFract(FFX_MIN16_F3 x)
{
    return x - floor(x);
}
FFX_MIN16_F4 ffxFract(FFX_MIN16_F4 x)
{
    return x - floor(x);
}
//------------------------------------------------------------------------------------------------------------------------------
FFX_MIN16_F ffxLerp(FFX_MIN16_F x, FFX_MIN16_F y, FFX_MIN16_F a)
{
    return lerp(x, y, a);
}
FFX_MIN16_F2 ffxLerp(FFX_MIN16_F2 x, FFX_MIN16_F2 y, FFX_MIN16_F a)
{
    return lerp(x, y, a);
}
FFX_MIN16_F2 ffxLerp(FFX_MIN16_F2 x, FFX_MIN16_F2 y, FFX_MIN16_F2 a)
{
    return lerp(x, y, a);
}
FFX_MIN16_F3 ffxLerp(FFX_MIN16_F3 x, FFX_MIN16_F3 y, FFX_MIN16_F a)
{
    return lerp(x, y, a);
}
FFX_MIN16_F3 ffxLerp(FFX_MIN16_F3 x, FFX_MIN16_F3 y, FFX_MIN16_F3 a)
{
    return lerp(x, y, a);
}
FFX_MIN16_F4 ffxLerp(FFX_MIN16_F4 x, FFX_MIN16_F4 y, FFX_MIN16_F a)
{
    return lerp(x, y, a);
}
FFX_MIN16_F4 ffxLerp(FFX_MIN16_F4 x, FFX_MIN16_F4 y, FFX_MIN16_F4 a)
{
    return lerp(x, y, a);
}
//------------------------------------------------------------------------------------------------------------------------------
FFX_MIN16_F ffxMax3Half(FFX_MIN16_F x, FFX_MIN16_F y, FFX_MIN16_F z)
{
    return max(x, max(y, z));
}
FFX_MIN16_F2 ffxMax3Half(FFX_MIN16_F2 x, FFX_MIN16_F2 y, FFX_MIN16_F2 z)
{
    return max(x, max(y, z));
}
FFX_MIN16_F3 ffxMax3Half(FFX_MIN16_F3 x, FFX_MIN16_F3 y, FFX_MIN16_F3 z)
{
    return max(x, max(y, z));
}
FFX_MIN16_F4 ffxMax3Half(FFX_MIN16_F4 x, FFX_MIN16_F4 y, FFX_MIN16_F4 z)
{
    return max(x, max(y, z));
}
//------------------------------------------------------------------------------------------------------------------------------
FFX_MIN16_F ffxMin3Half(FFX_MIN16_F x, FFX_MIN16_F y, FFX_MIN16_F z)
{
    return min(x, min(y, z));
}
FFX_MIN16_F2 ffxMin3Half(FFX_MIN16_F2 x, FFX_MIN16_F2 y, FFX_MIN16_F2 z)
{
    return min(x, min(y, z));
}
FFX_MIN16_F3 ffxMin3Half(FFX_MIN16_F3 x, FFX_MIN16_F3 y, FFX_MIN16_F3 z)
{
    return min(x, min(y, z));
}
FFX_MIN16_F4 ffxMin3Half(FFX_MIN16_F4 x, FFX_MIN16_F4 y, FFX_MIN16_F4 z)
{
    return min(x, min(y, z));
}
//------------------------------------------------------------------------------------------------------------------------------
FFX_MIN16_F ffxMed3Half(FFX_MIN16_F x, FFX_MIN16_F y, FFX_MIN16_F z)
{
    return max(min(x, y), min(max(x, y), z));
}
FFX_MIN16_F2 ffxMed3Half(FFX_MIN16_F2 x, FFX_MIN16_F2 y, FFX_MIN16_F2 z)
{
    return max(min(x, y), min(max(x, y), z));
}
FFX_MIN16_F3 ffxMed3Half(FFX_MIN16_F3 x, FFX_MIN16_F3 y, FFX_MIN16_F3 z)
{
    return max(min(x, y), min(max(x, y), z));
}
FFX_MIN16_F4 ffxMed3Half(FFX_MIN16_F4 x, FFX_MIN16_F4 y, FFX_MIN16_F4 z)
{
    return max(min(x, y), min(max(x, y), z));
}
//------------------------------------------------------------------------------------------------------------------------------
FFX_MIN16_I ffxMed3Half(FFX_MIN16_I x, FFX_MIN16_I y, FFX_MIN16_I z)
{
    return max(min(x, y), min(max(x, y), z));
}
FFX_MIN16_I2 ffxMed3Half(FFX_MIN16_I2 x, FFX_MIN16_I2 y, FFX_MIN16_I2 z)
{
    return max(min(x, y), min(max(x, y), z));
}
FFX_MIN16_I3 ffxMed3Half(FFX_MIN16_I3 x, FFX_MIN16_I3 y, FFX_MIN16_I3 z)
{
    return max(min(x, y), min(max(x, y), z));
}
FFX_MIN16_I4 ffxMed3Half(FFX_MIN16_I4 x, FFX_MIN16_I4 y, FFX_MIN16_I4 z)
{
    return max(min(x, y), min(max(x, y), z));
}
//------------------------------------------------------------------------------------------------------------------------------
FFX_MIN16_F ffxReciprocalHalf(FFX_MIN16_F x)
{
    return rcp(x);
}
FFX_MIN16_F2 ffxReciprocalHalf(FFX_MIN16_F2 x)
{
    return rcp(x);
}
FFX_MIN16_F3 ffxReciprocalHalf(FFX_MIN16_F3 x)
{
    return rcp(x);
}
FFX_MIN16_F4 ffxReciprocalHalf(FFX_MIN16_F4 x)
{
    return rcp(x);
}
//------------------------------------------------------------------------------------------------------------------------------
FFX_MIN16_F ffxReciprocalSquareRootHalf(FFX_MIN16_F x)
{
    return rsqrt(x);
}
FFX_MIN16_F2 ffxReciprocalSquareRootHalf(FFX_MIN16_F2 x)
{
    return rsqrt(x);
}
FFX_MIN16_F3 ffxReciprocalSquareRootHalf(FFX_MIN16_F3 x)
{
    return rsqrt(x);
}
FFX_MIN16_F4 ffxReciprocalSquareRootHalf(FFX_MIN16_F4 x)
{
    return rsqrt(x);
}
//------------------------------------------------------------------------------------------------------------------------------
FFX_MIN16_F ffxSaturate(FFX_MIN16_F x)
{
    return saturate(x);
}
FFX_MIN16_F2 ffxSaturate(FFX_MIN16_F2 x)
{
    return saturate(x);
}
FFX_MIN16_F3 ffxSaturate(FFX_MIN16_F3 x)
{
    return saturate(x);
}
FFX_MIN16_F4 ffxSaturate(FFX_MIN16_F4 x)
{
    return saturate(x);
}
//------------------------------------------------------------------------------------------------------------------------------
FFX_MIN16_U ffxBitShiftRightHalf(FFX_MIN16_U a, FFX_MIN16_U b)
{
    return FFX_MIN16_U(FFX_MIN16_I(a) >> FFX_MIN16_I(b));
}
FFX_MIN16_U2 ffxBitShiftRightHalf(FFX_MIN16_U2 a, FFX_MIN16_U2 b)
{
    return FFX_MIN16_U2(FFX_MIN16_I2(a) >> FFX_MIN16_I2(b));
}
FFX_MIN16_U3 ffxBitShiftRightHalf(FFX_MIN16_U3 a, FFX_MIN16_U3 b)
{
    return FFX_MIN16_U3(FFX_MIN16_I3(a) >> FFX_MIN16_I3(b));
}
FFX_MIN16_U4 ffxBitShiftRightHalf(FFX_MIN16_U4 a, FFX_MIN16_U4 b)
{
    return FFX_MIN16_U4(FFX_MIN16_I4(a) >> FFX_MIN16_I4(b));
}
#endif // FFX_HALF

//==============================================================================================================================
//                                                         HLSL WAVE
//==============================================================================================================================
#if defined(FFX_WAVE)
// Where 'x' must be a compile time literal.
FfxFloat32 ffxWaveXorF1(FfxFloat32 v, FfxUInt32 x)
{
    return WaveReadLaneAt(v, WaveGetLaneIndex() ^ x);
}
FfxFloat32x2 ffxWaveXorF2(FfxFloat32x2 v, FfxUInt32 x)
{
    return WaveReadLaneAt(v, WaveGetLaneIndex() ^ x);
}
FfxFloat32x3 ffxWaveXorF3(FfxFloat32x3 v, FfxUInt32 x)
{
    return WaveReadLaneAt(v, WaveGetLaneIndex() ^ x);
}
FfxFloat32x4 ffxWaveXorF4(FfxFloat32x4 v, FfxUInt32 x)
{
    return WaveReadLaneAt(v, WaveGetLaneIndex() ^ x);
}
FfxUInt32 ffxWaveXorU1(FfxUInt32 v, FfxUInt32 x)
{
    return WaveReadLaneAt(v, WaveGetLaneIndex() ^ x);
}
FfxUInt32x2 ffxWaveXorU1(FfxUInt32x2 v, FfxUInt32 x)
{
    return WaveReadLaneAt(v, WaveGetLaneIndex() ^ x);
}
FfxUInt32x3 ffxWaveXorU1(FfxUInt32x3 v, FfxUInt32 x)
{
    return WaveReadLaneAt(v, WaveGetLaneIndex() ^ x);
}
FfxUInt32x4 ffxWaveXorU1(FfxUInt32x4 v, FfxUInt32 x)
{
    return WaveReadLaneAt(v, WaveGetLaneIndex() ^ x);
}
FfxBoolean ffxWaveIsFirstLane()
{
    return WaveIsFirstLane();
}
FfxUInt32 ffxWaveLaneIndex()
{
    return WaveGetLaneIndex();
}
FfxBoolean ffxWaveReadAtLaneIndexB1(FfxBoolean v, FfxUInt32 x)
{
    return WaveReadLaneAt(v, x);
}
FfxUInt32 ffxWavePrefixCountBits(FfxBoolean v)
{
    return WavePrefixCountBits(v);
}
FfxUInt32 ffxWaveActiveCountBits(FfxBoolean v)
{
    return WaveActiveCountBits(v);
}
FfxUInt32 ffxWaveReadLaneFirstU1(FfxUInt32 v)
{
    return WaveReadLaneFirst(v);
}
FfxUInt32x2 ffxWaveReadLaneFirstU2(FfxUInt32x2 v)
{
    return WaveReadLaneFirst(v);
}
FfxBoolean ffxWaveReadLaneFirstB1(FfxBoolean v)
{
    return WaveReadLaneFirst(v);
}
FfxUInt32 ffxWaveOr(FfxUInt32 a)
{
    return WaveActiveBitOr(a);
}
FfxUInt32 ffxWaveMin(FfxUInt32 a)
{
    return WaveActiveMin(a);
}
FfxFloat32 ffxWaveMin(FfxFloat32 a)
{
    return WaveActiveMin(a);
}
FfxUInt32 ffxWaveMax(FfxUInt32 a)
{
    return WaveActiveMax(a);
}
FfxFloat32 ffxWaveMax(FfxFloat32 a)
{
    return WaveActiveMax(a);
}
FfxUInt32 ffxWaveSum(FfxUInt32 a)
{
    return WaveActiveSum(a);
}
FfxFloat32 ffxWaveSum(FfxFloat32 a)
{
    return WaveActiveSum(a);
}
FfxUInt32 ffxWaveLaneCount()
{
    return WaveGetLaneCount();
}
FfxBoolean ffxWaveAllTrue(FfxBoolean v)
{
    return WaveActiveAllTrue(v);
}
FfxFloat32 ffxQuadReadX(FfxFloat32 v)
{
    return QuadReadAcrossX(v);
}
FfxFloat32x2 ffxQuadReadX(FfxFloat32x2 v)
{
    return QuadReadAcrossX(v);
}
FfxFloat32 ffxQuadReadY(FfxFloat32 v)
{
    return QuadReadAcrossY(v);
}
FfxFloat32x2 ffxQuadReadY(FfxFloat32x2 v)
{
    return QuadReadAcrossY(v);
}

#if FFX_HALF
FfxFloat16x2 ffxWaveXorFloat16x2(FfxFloat16x2 v, FfxUInt32 x)
{
    return FFX_UINT32_TO_FLOAT16X2(WaveReadLaneAt(FFX_FLOAT16X2_TO_UINT32(v), WaveGetLaneIndex() ^ x));
}
FfxFloat16x4 ffxWaveXorFloat16x4(FfxFloat16x4 v, FfxUInt32 x)
{
    return FFX_UINT32X2_TO_FLOAT16X4(WaveReadLaneAt(FFX_FLOAT16X4_TO_UINT32X2(v), WaveGetLaneIndex() ^ x));
}
FfxUInt16x2 ffxWaveXorUint16x2(FfxUInt16x2 v, FfxUInt32 x)
{
    return FFX_UINT32_TO_UINT16X2(WaveReadLaneAt(FFX_UINT16X2_TO_UINT32(v), WaveGetLaneIndex() ^ x));
}
FfxUInt16x4 ffxWaveXorUint16x4(FfxUInt16x4 v, FfxUInt32 x)
{
    return FFX_UINT32X2_TO_UINT16X4(WaveReadLaneAt(FFX_UINT16X4_TO_UINT32X2(v), WaveGetLaneIndex() ^ x));
}
#endif // FFX_HALF
#endif // #if defined(FFX_WAVE)

/// A define for a true value in a boolean expression.
///
/// @ingroup GPUCore
#define FFX_TRUE (true)

/// A define for a false value in a boolean expression.
///
/// @ingroup GPUCore
#define FFX_FALSE (false)

/// A define value for positive infinity.
///
/// @ingroup GPUCore
#define FFX_POSITIVE_INFINITY_FLOAT ffxAsFloat(0x7f800000u)

/// A define value for negative infinity.
///
/// @ingroup GPUCore
#define FFX_NEGATIVE_INFINITY_FLOAT ffxAsFloat(0xff800000u)

/// A define value for PI.
/// 
/// @ingroup GPUCore
#define FFX_PI  (3.14159)

#define FFX_HAS_FLAG(v, f) ((v & f) == f)

/// Compute the min of two values.
///
/// @param [in] x                   The first value to compute the min of.
/// @param [in] y                   The second value to compute the min of.
///
/// @returns
/// The the lowest of two values.
///
/// @ingroup GPUCore
FfxFloat32 ffxMin(FfxFloat32 x, FfxFloat32 y)
{
    return min(x, y);
}

/// Compute the min of two values.
///
/// @param [in] x                   The first value to compute the min of.
/// @param [in] y                   The second value to compute the min of.
///
/// @returns
/// The the lowest of two values.
///
/// @ingroup GPUCore
FfxFloat32x2 ffxMin(FfxFloat32x2 x, FfxFloat32x2 y)
{
    return min(x, y);
}

/// Compute the min of two values.
///
/// @param [in] x                   The first value to compute the min of.
/// @param [in] y                   The second value to compute the min of.
///
/// @returns
/// The the lowest of two values.
///
/// @ingroup GPUCore
FfxFloat32x3 ffxMin(FfxFloat32x3 x, FfxFloat32x3 y)
{
    return min(x, y);
}

/// Compute the min of two values.
///
/// @param [in] x                   The first value to compute the min of.
/// @param [in] y                   The second value to compute the min of.
///
/// @returns
/// The the lowest of two values.
///
/// @ingroup GPUCore
FfxFloat32x4 ffxMin(FfxFloat32x4 x, FfxFloat32x4 y)
{
    return min(x, y);
}

/// Compute the min of two values.
///
/// @param [in] x                   The first value to compute the min of.
/// @param [in] y                   The second value to compute the min of.
///
/// @returns
/// The the lowest of two values.
///
/// @ingroup GPUCore
FfxInt32 ffxMin(FfxInt32 x, FfxInt32 y)
{
    return min(x, y);
}

/// Compute the min of two values.
///
/// @param [in] x                   The first value to compute the min of.
/// @param [in] y                   The second value to compute the min of.
///
/// @returns
/// The the lowest of two values.
///
/// @ingroup GPUCore
FfxInt32x2 ffxMin(FfxInt32x2 x, FfxInt32x2 y)
{
    return min(x, y);
}

/// Compute the min of two values.
///
/// @param [in] x                   The first value to compute the min of.
/// @param [in] y                   The second value to compute the min of.
///
/// @returns
/// The the lowest of two values.
///
/// @ingroup GPUCore
FfxInt32x3 ffxMin(FfxInt32x3 x, FfxInt32x3 y)
{
    return min(x, y);
}

/// Compute the min of two values.
///
/// @param [in] x                   The first value to compute the min of.
/// @param [in] y                   The second value to compute the min of.
///
/// @returns
/// The the lowest of two values.
///
/// @ingroup GPUCore
FfxInt32x4 ffxMin(FfxInt32x4 x, FfxInt32x4 y)
{
    return min(x, y);
}

/// Compute the min of two values.
///
/// @param [in] x                   The first value to compute the min of.
/// @param [in] y                   The second value to compute the min of.
///
/// @returns
/// The the lowest of two values.
///
/// @ingroup GPUCore
FfxUInt32 ffxMin(FfxUInt32 x, FfxUInt32 y)
{
    return min(x, y);
}

/// Compute the min of two values.
///
/// @param [in] x                   The first value to compute the min of.
/// @param [in] y                   The second value to compute the min of.
///
/// @returns
/// The the lowest of two values.
///
/// @ingroup GPUCore
FfxUInt32x2 ffxMin(FfxUInt32x2 x, FfxUInt32x2 y)
{
    return min(x, y);
}

/// Compute the min of two values.
///
/// @param [in] x                   The first value to compute the min of.
/// @param [in] y                   The second value to compute the min of.
///
/// @returns
/// The the lowest of two values.
///
/// @ingroup GPUCore
FfxUInt32x3 ffxMin(FfxUInt32x3 x, FfxUInt32x3 y)
{
    return min(x, y);
}

/// Compute the min of two values.
///
/// @param [in] x                   The first value to compute the min of.
/// @param [in] y                   The second value to compute the min of.
///
/// @returns
/// The the lowest of two values.
///
/// @ingroup GPUCore
FfxUInt32x4 ffxMin(FfxUInt32x4 x, FfxUInt32x4 y)
{
    return min(x, y);
}

/// Compute the max of two values.
///
/// @param [in] x                   The first value to compute the max of.
/// @param [in] y                   The second value to compute the max of.
///
/// @returns
/// The the lowest of two values.
///
/// @ingroup GPUCore
FfxFloat32 ffxMax(FfxFloat32 x, FfxFloat32 y)
{
    return max(x, y);
}

/// Compute the max of two values.
///
/// @param [in] x                   The first value to compute the max of.
/// @param [in] y                   The second value to compute the max of.
///
/// @returns
/// The the lowest of two values.
///
/// @ingroup GPUCore
FfxFloat32x2 ffxMax(FfxFloat32x2 x, FfxFloat32x2 y)
{
    return max(x, y);
}

/// Compute the max of two values.
///
/// @param [in] x                   The first value to compute the max of.
/// @param [in] y                   The second value to compute the max of.
///
/// @returns
/// The the lowest of two values.
///
/// @ingroup GPUCore
FfxFloat32x3 ffxMax(FfxFloat32x3 x, FfxFloat32x3 y)
{
    return max(x, y);
}

/// Compute the max of two values.
///
/// @param [in] x                   The first value to compute the max of.
/// @param [in] y                   The second value to compute the max of.
///
/// @returns
/// The the lowest of two values.
///
/// @ingroup GPUCore
FfxFloat32x4 ffxMax(FfxFloat32x4 x, FfxFloat32x4 y)
{
    return max(x, y);
}

/// Compute the max of two values.
///
/// @param [in] x                   The first value to compute the max of.
/// @param [in] y                   The second value to compute the max of.
///
/// @returns
/// The the lowest of two values.
///
/// @ingroup GPUCore
FfxInt32 ffxMax(FfxInt32 x, FfxInt32 y)
{
    return max(x, y);
}

/// Compute the max of two values.
///
/// @param [in] x                   The first value to compute the max of.
/// @param [in] y                   The second value to compute the max of.
///
/// @returns
/// The the lowest of two values.
///
/// @ingroup GPUCore
FfxInt32x2 ffxMax(FfxInt32x2 x, FfxInt32x2 y)
{
    return max(x, y);
}

/// Compute the max of two values.
///
/// @param [in] x                   The first value to compute the max of.
/// @param [in] y                   The second value to compute the max of.
///
/// @returns
/// The the lowest of two values.
///
/// @ingroup GPUCore
FfxInt32x3 ffxMax(FfxInt32x3 x, FfxInt32x3 y)
{
    return max(x, y);
}

/// Compute the max of two values.
///
/// @param [in] x                   The first value to compute the max of.
/// @param [in] y                   The second value to compute the max of.
///
/// @returns
/// The the lowest of two values.
///
/// @ingroup GPUCore
FfxInt32x4 ffxMax(FfxInt32x4 x, FfxInt32x4 y)
{
    return max(x, y);
}

/// Compute the max of two values.
///
/// @param [in] x                   The first value to compute the max of.
/// @param [in] y                   The second value to compute the max of.
///
/// @returns
/// The the lowest of two values.
///
/// @ingroup GPUCore
FfxUInt32 ffxMax(FfxUInt32 x, FfxUInt32 y)
{
    return max(x, y);
}

/// Compute the max of two values.
///
/// @param [in] x                   The first value to compute the max of.
/// @param [in] y                   The second value to compute the max of.
///
/// @returns
/// The the lowest of two values.
///
/// @ingroup GPUCore
FfxUInt32x2 ffxMax(FfxUInt32x2 x, FfxUInt32x2 y)
{
    return max(x, y);
}

/// Compute the max of two values.
///
/// @param [in] x                   The first value to compute the max of.
/// @param [in] y                   The second value to compute the max of.
///
/// @returns
/// The the lowest of two values.
///
/// @ingroup GPUCore
FfxUInt32x3 ffxMax(FfxUInt32x3 x, FfxUInt32x3 y)
{
    return max(x, y);
}

/// Compute the max of two values.
///
/// @param [in] x                   The first value to compute the max of.
/// @param [in] y                   The second value to compute the max of.
///
/// @returns
/// The the lowest of two values.
///
/// @ingroup GPUCore
FfxUInt32x4 ffxMax(FfxUInt32x4 x, FfxUInt32x4 y)
{
    return max(x, y);
}

/// Compute the value of the first parameter raised to the power of the second.
///
/// @param [in] x                   The value to raise to the power y.
/// @param [in] y                   The power to which to raise x.
///
/// @returns
/// The value of the first parameter raised to the power of the second.
///
/// @ingroup GPUCore
FfxFloat32 ffxPow(FfxFloat32 x, FfxFloat32 y)
{
    return pow(x, y);
}

/// Compute the value of the first parameter raised to the power of the second.
///
/// @param [in] x                   The value to raise to the power y.
/// @param [in] y                   The power to which to raise x.
///
/// @returns
/// The value of the first parameter raised to the power of the second.
///
/// @ingroup GPUCore
FfxFloat32x2 ffxPow(FfxFloat32x2 x, FfxFloat32x2 y)
{
    return pow(x, y);
}

/// Compute the value of the first parameter raised to the power of the second.
///
/// @param [in] x                   The value to raise to the power y.
/// @param [in] y                   The power to which to raise x.
///
/// @returns
/// The value of the first parameter raised to the power of the second.
///
/// @ingroup GPUCore
FfxFloat32x3 ffxPow(FfxFloat32x3 x, FfxFloat32x3 y)
{
    return pow(x, y);
}

/// Compute the value of the first parameter raised to the power of the second.
///
/// @param [in] x                   The value to raise to the power y.
/// @param [in] y                   The power to which to raise x.
///
/// @returns
/// The value of the first parameter raised to the power of the second.
///
/// @ingroup GPUCore
FfxFloat32x4 ffxPow(FfxFloat32x4 x, FfxFloat32x4 y)
{
    return pow(x, y);
}

/// Compute the square root of a value.
///
/// @param [in] x                   The first value to compute the min of.
///
/// @returns
/// The the square root of <c><i>x</i></c>.
///
/// @ingroup GPUCore
FfxFloat32 ffxSqrt(FfxFloat32 x)
{
    return sqrt(x);
}

/// Compute the square root of a value.
///
/// @param [in] x                   The first value to compute the min of.
///
/// @returns
/// The the square root of <c><i>x</i></c>.
///
/// @ingroup GPUCore
FfxFloat32x2 ffxSqrt(FfxFloat32x2 x)
{
    return sqrt(x);
}

/// Compute the square root of a value.
///
/// @param [in] x                   The first value to compute the min of.
///
/// @returns
/// The the square root of <c><i>x</i></c>.
///
/// @ingroup GPUCore
FfxFloat32x3 ffxSqrt(FfxFloat32x3 x)
{
    return sqrt(x);
}

/// Compute the square root of a value.
///
/// @param [in] x                   The first value to compute the min of.
///
/// @returns
/// The the square root of <c><i>x</i></c>.
///
/// @ingroup GPUCore
FfxFloat32x4 ffxSqrt(FfxFloat32x4 x)
{
    return sqrt(x);
}

/// Copy the sign bit from 's' to positive 'd'.
///
/// @param [in] d                   The value to copy the sign bit into.
/// @param [in] s                   The value to copy the sign bit from.
/// 
/// @returns
/// The value of <c><i>d</i></c> with the sign bit from <c><i>s</i></c>.
/// 
/// @ingroup GPUCore
FfxFloat32 ffxCopySignBit(FfxFloat32 d, FfxFloat32 s)
{
    return ffxAsFloat(ffxAsUInt32(d) | (ffxAsUInt32(s) & FfxUInt32(0x80000000u)));
}

/// Copy the sign bit from 's' to positive 'd'.
///
/// @param [in] d                   The value to copy the sign bit into.
/// @param [in] s                   The value to copy the sign bit from.
///
/// @returns
/// The value of <c><i>d</i></c> with the sign bit from <c><i>s</i></c>.
///
/// @ingroup GPUCore
FfxFloat32x2 ffxCopySignBit(FfxFloat32x2 d, FfxFloat32x2 s)
{
    return ffxAsFloat(ffxAsUInt32(d) | (ffxAsUInt32(s) & ffxBroadcast2(0x80000000u)));
}

/// Copy the sign bit from 's' to positive 'd'.
///
/// @param [in] d                   The value to copy the sign bit into.
/// @param [in] s                   The value to copy the sign bit from.
///
/// @returns
/// The value of <c><i>d</i></c> with the sign bit from <c><i>s</i></c>.
///
/// @ingroup GPUCore
FfxFloat32x3 ffxCopySignBit(FfxFloat32x3 d, FfxFloat32x3 s)
{
    return ffxAsFloat(ffxAsUInt32(d) | (ffxAsUInt32(s) & ffxBroadcast3(0x80000000u)));
}

/// Copy the sign bit from 's' to positive 'd'.
///
/// @param [in] d                   The value to copy the sign bit into.
/// @param [in] s                   The value to copy the sign bit from.
///
/// @returns
/// The value of <c><i>d</i></c> with the sign bit from <c><i>s</i></c>.
///
/// @ingroup GPUCore
FfxFloat32x4 ffxCopySignBit(FfxFloat32x4 d, FfxFloat32x4 s)
{
    return ffxAsFloat(ffxAsUInt32(d) | (ffxAsUInt32(s) & ffxBroadcast4(0x80000000u)));
}

/// A single operation to return the following:
///     m = NaN := 0
///     m >= 0  := 0
///     m < 0   := 1
///
/// Uses the following useful floating point logic,
///     saturate(+a*(-INF)==-INF) := 0
///     saturate( 0*(-INF)== NaN) := 0
///     saturate(-a*(-INF)==+INF) := 1
/// 
/// This function is useful when creating masks for branch-free logic.
/// 
/// @param [in] m                       The value to test against 0.
/// 
/// @returns
/// 1.0 when the value is negative, or 0.0 when the value is 0 or position.
/// 
/// @ingroup GPUCore
FfxFloat32 ffxIsSigned(FfxFloat32 m)
{
    return ffxSaturate(m * FfxFloat32(FFX_NEGATIVE_INFINITY_FLOAT));
}

/// A single operation to return the following:
///     m = NaN := 0
///     m >= 0  := 0
///     m < 0   := 1
///
/// Uses the following useful floating point logic,
///     saturate(+a*(-INF)==-INF) := 0
///     saturate( 0*(-INF)== NaN) := 0
///     saturate(-a*(-INF)==+INF) := 1
///
/// This function is useful when creating masks for branch-free logic.
///
/// @param [in] m                       The value to test against 0.
///
/// @returns
/// 1.0 when the value is negative, or 0.0 when the value is 0 or position.
///
/// @ingroup GPUCore
FfxFloat32x2 ffxIsSigned(FfxFloat32x2 m)
{
    return ffxSaturate(m * ffxBroadcast2(FFX_NEGATIVE_INFINITY_FLOAT));
}

/// A single operation to return the following:
///     m = NaN := 0
///     m >= 0  := 0
///     m < 0   := 1
///
/// Uses the following useful floating point logic,
///     saturate(+a*(-INF)==-INF) := 0
///     saturate( 0*(-INF)== NaN) := 0
///     saturate(-a*(-INF)==+INF) := 1
///
/// This function is useful when creating masks for branch-free logic.
///
/// @param [in] m                       The value to test against 0.
///
/// @returns
/// 1.0 when the value is negative, or 0.0 when the value is 0 or position.
///
/// @ingroup GPUCore
FfxFloat32x3 ffxIsSigned(FfxFloat32x3 m)
{
    return ffxSaturate(m * ffxBroadcast3(FFX_NEGATIVE_INFINITY_FLOAT));
}

/// A single operation to return the following:
///     m = NaN := 0
///     m >= 0  := 0
///     m < 0   := 1
///
/// Uses the following useful floating point logic,
///     saturate(+a*(-INF)==-INF) := 0
///     saturate( 0*(-INF)== NaN) := 0
///     saturate(-a*(-INF)==+INF) := 1
///
/// This function is useful when creating masks for branch-free logic.
///
/// @param [in] m                       The value to test against for have the sign set.
///
/// @returns
/// 1.0 when the value is negative, or 0.0 when the value is 0 or positive.
///
/// @ingroup GPUCore
FfxFloat32x4 ffxIsSigned(FfxFloat32x4 m)
{
    return ffxSaturate(m * ffxBroadcast4(FFX_NEGATIVE_INFINITY_FLOAT));
}

/// A single operation to return the following:
///     m = NaN := 1
///     m > 0   := 0
///     m <= 0  := 1
///
/// This function is useful when creating masks for branch-free logic.
///
/// @param [in] m                       The value to test against zero.
///
/// @returns
/// 1.0 when the value is position, or 0.0 when the value is 0 or negative.
///
/// @ingroup GPUCore
FfxFloat32 ffxIsGreaterThanZero(FfxFloat32 m)
{
    return ffxSaturate(m * FfxFloat32(FFX_POSITIVE_INFINITY_FLOAT));
}

/// A single operation to return the following:
///     m = NaN := 1
///     m > 0   := 0
///     m <= 0  := 1
///
/// This function is useful when creating masks for branch-free logic.
///
/// @param [in] m                       The value to test against zero.
///
/// @returns
/// 1.0 when the value is position, or 0.0 when the value is 0 or negative.
///
/// @ingroup GPUCore
FfxFloat32x2 ffxIsGreaterThanZero(FfxFloat32x2 m)
{
    return ffxSaturate(m * ffxBroadcast2(FFX_POSITIVE_INFINITY_FLOAT));
}

/// A single operation to return the following:
///     m = NaN := 1
///     m > 0   := 0
///     m <= 0  := 1
///
/// This function is useful when creating masks for branch-free logic.
///
/// @param [in] m                       The value to test against zero.
///
/// @returns
/// 1.0 when the value is position, or 0.0 when the value is 0 or negative.
///
/// @ingroup GPUCore
FfxFloat32x3 ffxIsGreaterThanZero(FfxFloat32x3 m)
{
    return ffxSaturate(m * ffxBroadcast3(FFX_POSITIVE_INFINITY_FLOAT));
}

/// A single operation to return the following:
///     m = NaN := 1
///     m > 0   := 0
///     m <= 0  := 1
///
/// This function is useful when creating masks for branch-free logic.
///
/// @param [in] m                       The value to test against zero.
///
/// @returns
/// 1.0 when the value is position, or 0.0 when the value is 0 or negative.
///
/// @ingroup GPUCore
FfxFloat32x4 ffxIsGreaterThanZero(FfxFloat32x4 m)
{
    return ffxSaturate(m * ffxBroadcast4(FFX_POSITIVE_INFINITY_FLOAT));
}

/// Convert a 32bit floating point value to sortable integer.
/// 
///  - If sign bit=0, flip the sign bit (positives).
///  - If sign bit=1, flip all bits     (negatives).
/// 
/// The function has the side effects that:
///  - Larger integers are more positive values.
///  - Float zero is mapped to center of integers (so clear to integer zero is a nice default for atomic max usage).
/// 
/// @param [in] value                       The floating point value to make sortable.
/// 
/// @returns
/// The sortable integer value.
/// 
/// @ingroup GPUCore
FfxUInt32 ffxFloatToSortableInteger(FfxUInt32 value)
{
    return value ^ ((ffxAShrSU1(value, FfxUInt32(31))) | FfxUInt32(0x80000000));
}

/// Convert a sortable integer to a 32bit floating point value.
///
/// The function has the side effects that:
///  - If sign bit=1, flip the sign bit (positives).
///  - If sign bit=0, flip all bits     (negatives).
///
/// @param [in] value                       The floating point value to make sortable.
///
/// @returns
/// The sortable integer value.
///
/// @ingroup GPUCore
FfxUInt32 ffxSortableIntegerToFloat(FfxUInt32 value)
{
    return value ^ ((~ffxAShrSU1(value, FfxUInt32(31))) | FfxUInt32(0x80000000));
}

/// Calculate a low-quality approximation for the square root of a value.
///
/// For additional information on the approximation family of functions, you can refer to Michal Drobot's excellent 
/// presentation materials:
/// 
///  - https://michaldrobot.files.wordpress.com/2014/05/gcn_alu_opt_digitaldragons2014.pdf
///  - https://github.com/michaldrobot/ShaderFastLibs/blob/master/ShaderFastMathLib.h
/// 
/// @param [in] value           The value to calculate an approximate to the square root for.
///
/// @returns
/// An approximation of the square root, estimated to low quality.
///
/// @ingroup GPUCore
FfxFloat32 ffxApproximateSqrt(FfxFloat32 value)
{
    return ffxAsFloat((ffxAsUInt32(value) >> FfxUInt32(1)) + FfxUInt32(0x1fbc4639));
}

/// Calculate a low-quality approximation for the reciprocal of a value.
///
/// For additional information on the approximation family of functions, you can refer to Michal Drobot's excellent
/// presentation materials:
///
///  - https://michaldrobot.files.wordpress.com/2014/05/gcn_alu_opt_digitaldragons2014.pdf
///  - https://github.com/michaldrobot/ShaderFastLibs/blob/master/ShaderFastMathLib.h
///
/// @param [in] value           The value to calculate an approximate to the reciprocal for.
///
/// @returns
/// An approximation of the reciprocal, estimated to low quality.
///
/// @ingroup GPUCore
FfxFloat32 ffxApproximateReciprocal(FfxFloat32 value)
{
    return ffxAsFloat(FfxUInt32(0x7ef07ebb) - ffxAsUInt32(value));
}

/// Calculate a medium-quality approximation for the reciprocal of a value.
/// 
/// For additional information on the approximation family of functions, you can refer to Michal Drobot's excellent
/// presentation materials:
///
///  - https://michaldrobot.files.wordpress.com/2014/05/gcn_alu_opt_digitaldragons2014.pdf
///  - https://github.com/michaldrobot/ShaderFastLibs/blob/master/ShaderFastMathLib.h
///
/// @param [in] value           The value to calculate an approximate to the reciprocal for.
///
/// @returns
/// An approximation of the reciprocal, estimated to medium quality.
/// 
/// @ingroup GPUCore
FfxFloat32 ffxApproximateReciprocalMedium(FfxFloat32 value)
{
    FfxFloat32 b = ffxAsFloat(FfxUInt32(0x7ef19fff) - ffxAsUInt32(value));
    return b * (-b * value + FfxFloat32(2.0));
}

/// Calculate a low-quality approximation for the reciprocal of a value.
///
/// For additional information on the approximation family of functions, you can refer to Michal Drobot's excellent
/// presentation materials:
///
///  - https://michaldrobot.files.wordpress.com/2014/05/gcn_alu_opt_digitaldragons2014.pdf
///  - https://github.com/michaldrobot/ShaderFastLibs/blob/master/ShaderFastMathLib.h
///
/// @param [in] value           The value to calculate an approximate to the reciprocal square root for.
///
/// @returns
/// An approximation of the reciprocal square root, estimated to low quality.
///
/// @ingroup GPUCore
FfxFloat32 ffxApproximateReciprocalSquareRoot(FfxFloat32 value)
{
    return ffxAsFloat(FfxUInt32(0x5f347d74) - (ffxAsUInt32(value) >> FfxUInt32(1)));
}

/// Calculate a low-quality approximation for the square root of a value.
///
/// For additional information on the approximation family of functions, you can refer to Michal Drobot's excellent
/// presentation materials:
///
///  - https://michaldrobot.files.wordpress.com/2014/05/gcn_alu_opt_digitaldragons2014.pdf
///  - https://github.com/michaldrobot/ShaderFastLibs/blob/master/ShaderFastMathLib.h
///
/// @param [in] value           The value to calculate an approximate to the square root for.
///
/// @returns
/// An approximation of the square root, estimated to low quality.
///
/// @ingroup GPUCore
FfxFloat32x2 ffxApproximateSqrt(FfxFloat32x2 value)
{
    return ffxAsFloat((ffxAsUInt32(value) >> ffxBroadcast2(1u)) + ffxBroadcast2(0x1fbc4639u));
}

/// Calculate a low-quality approximation for the reciprocal of a value.
///
/// For additional information on the approximation family of functions, you can refer to Michal Drobot's excellent
/// presentation materials:
///
///  - https://michaldrobot.files.wordpress.com/2014/05/gcn_alu_opt_digitaldragons2014.pdf
///  - https://github.com/michaldrobot/ShaderFastLibs/blob/master/ShaderFastMathLib.h
///
/// @param [in] value           The value to calculate an approximate to the reciprocal for.
///
/// @returns
/// An approximation of the reciprocal, estimated to low quality.
///
/// @ingroup GPUCore
FfxFloat32x2 ffxApproximateReciprocal(FfxFloat32x2 value)
{
    return ffxAsFloat(ffxBroadcast2(0x7ef07ebbu) - ffxAsUInt32(value));
}

/// Calculate a medium-quality approximation for the reciprocal of a value.
///
/// For additional information on the approximation family of functions, you can refer to Michal Drobot's excellent
/// presentation materials:
///
///  - https://michaldrobot.files.wordpress.com/2014/05/gcn_alu_opt_digitaldragons2014.pdf
///  - https://github.com/michaldrobot/ShaderFastLibs/blob/master/ShaderFastMathLib.h
///
/// @param [in] value           The value to calculate an approximate to the reciprocal for.
///
/// @returns
/// An approximation of the reciprocal, estimated to medium quality.
///
/// @ingroup GPUCore
FfxFloat32x2 ffxApproximateReciprocalMedium(FfxFloat32x2 value)
{
    FfxFloat32x2 b = ffxAsFloat(ffxBroadcast2(0x7ef19fffu) - ffxAsUInt32(value));
    return b * (-b * value + ffxBroadcast2(2.0f));
}

/// Calculate a low-quality approximation for the square root of a value.
///
/// For additional information on the approximation family of functions, you can refer to Michal Drobot's excellent
/// presentation materials:
///
///  - https://michaldrobot.files.wordpress.com/2014/05/gcn_alu_opt_digitaldragons2014.pdf
///  - https://github.com/michaldrobot/ShaderFastLibs/blob/master/ShaderFastMathLib.h
///
/// @param [in] value           The value to calculate an approximate to the square root for.
///
/// @returns
/// An approximation of the square root, estimated to low quality.
///
/// @ingroup GPUCore
FfxFloat32x2 ffxApproximateReciprocalSquareRoot(FfxFloat32x2 value)
{
    return ffxAsFloat(ffxBroadcast2(0x5f347d74u) - (ffxAsUInt32(value) >> ffxBroadcast2(1u)));
}

/// Calculate a low-quality approximation for the square root of a value.
///
/// For additional information on the approximation family of functions, you can refer to Michal Drobot's excellent
/// presentation materials:
///
///  - https://michaldrobot.files.wordpress.com/2014/05/gcn_alu_opt_digitaldragons2014.pdf
///  - https://github.com/michaldrobot/ShaderFastLibs/blob/master/ShaderFastMathLib.h
///
/// @param [in] value           The value to calculate an approximate to the square root for.
///
/// @returns
/// An approximation of the square root, estimated to low quality.
///
/// @ingroup GPUCore
FfxFloat32x3 ffxApproximateSqrt(FfxFloat32x3 value)
{
    return ffxAsFloat((ffxAsUInt32(value) >> ffxBroadcast3(1u)) + ffxBroadcast3(0x1fbc4639u));
}

/// Calculate a low-quality approximation for the reciprocal of a value.
///
/// For additional information on the approximation family of functions, you can refer to Michal Drobot's excellent
/// presentation materials:
///
///  - https://michaldrobot.files.wordpress.com/2014/05/gcn_alu_opt_digitaldragons2014.pdf
///  - https://github.com/michaldrobot/ShaderFastLibs/blob/master/ShaderFastMathLib.h
///
/// @param [in] value           The value to calculate an approximate to the reciprocal for.
///
/// @returns
/// An approximation of the reciprocal, estimated to low quality.
///
/// @ingroup GPUCore
FfxFloat32x3 ffxApproximateReciprocal(FfxFloat32x3 value)
{
    return ffxAsFloat(ffxBroadcast3(0x7ef07ebbu) - ffxAsUInt32(value));
}

/// Calculate a medium-quality approximation for the reciprocal of a value.
///
/// For additional information on the approximation family of functions, you can refer to Michal Drobot's excellent
/// presentation materials:
///
///  - https://michaldrobot.files.wordpress.com/2014/05/gcn_alu_opt_digitaldragons2014.pdf
///  - https://github.com/michaldrobot/ShaderFastLibs/blob/master/ShaderFastMathLib.h
///
/// @param [in] value           The value to calculate an approximate to the reciprocal for.
///
/// @returns
/// An approximation of the reciprocal, estimated to medium quality.
///
/// @ingroup GPUCore
FfxFloat32x3 ffxApproximateReciprocalMedium(FfxFloat32x3 value)
{
    FfxFloat32x3 b = ffxAsFloat(ffxBroadcast3(0x7ef19fffu) - ffxAsUInt32(value));
    return b * (-b * value + ffxBroadcast3(2.0f));
}

/// Calculate a low-quality approximation for the square root of a value.
///
/// For additional information on the approximation family of functions, you can refer to Michal Drobot's excellent
/// presentation materials:
///
///  - https://michaldrobot.files.wordpress.com/2014/05/gcn_alu_opt_digitaldragons2014.pdf
///  - https://github.com/michaldrobot/ShaderFastLibs/blob/master/ShaderFastMathLib.h
///
/// @param [in] value           The value to calculate an approximate to the square root for.
///
/// @returns
/// An approximation of the square root, estimated to low quality.
///
/// @ingroup GPUCore
FfxFloat32x3 ffxApproximateReciprocalSquareRoot(FfxFloat32x3 value)
{
    return ffxAsFloat(ffxBroadcast3(0x5f347d74u) - (ffxAsUInt32(value) >> ffxBroadcast3(1u)));
}

/// Calculate a low-quality approximation for the square root of a value.
///
/// For additional information on the approximation family of functions, you can refer to Michal Drobot's excellent
/// presentation materials:
///
///  - https://michaldrobot.files.wordpress.com/2014/05/gcn_alu_opt_digitaldragons2014.pdf
///  - https://github.com/michaldrobot/ShaderFastLibs/blob/master/ShaderFastMathLib.h
///
/// @param [in] value           The value to calculate an approximate to the square root for.
///
/// @returns
/// An approximation of the square root, estimated to low quality.
///
/// @ingroup GPUCore
FfxFloat32x4 ffxApproximateSqrt(FfxFloat32x4 value)
{
    return ffxAsFloat((ffxAsUInt32(value) >> ffxBroadcast4(1u)) + ffxBroadcast4(0x1fbc4639u));
}

/// Calculate a low-quality approximation for the reciprocal of a value.
///
/// For additional information on the approximation family of functions, you can refer to Michal Drobot's excellent
/// presentation materials:
///
///  - https://michaldrobot.files.wordpress.com/2014/05/gcn_alu_opt_digitaldragons2014.pdf
///  - https://github.com/michaldrobot/ShaderFastLibs/blob/master/ShaderFastMathLib.h
///
/// @param [in] value           The value to calculate an approximate to the reciprocal for.
///
/// @returns
/// An approximation of the reciprocal, estimated to low quality.
///
/// @ingroup GPUCore
FfxFloat32x4 ffxApproximateReciprocal(FfxFloat32x4 value)
{
    return ffxAsFloat(ffxBroadcast4(0x7ef07ebbu) - ffxAsUInt32(value));
}

/// Calculate a medium-quality approximation for the reciprocal of a value.
///
/// For additional information on the approximation family of functions, you can refer to Michal Drobot's excellent
/// presentation materials:
///
///  - https://michaldrobot.files.wordpress.com/2014/05/gcn_alu_opt_digitaldragons2014.pdf
///  - https://github.com/michaldrobot/ShaderFastLibs/blob/master/ShaderFastMathLib.h
///
/// @param [in] value           The value to calculate an approximate to the reciprocal for.
///
/// @returns
/// An approximation of the reciprocal, estimated to medium quality.
///
/// @ingroup GPUCore
FfxFloat32x4 ffxApproximateReciprocalMedium(FfxFloat32x4 value)
{
    FfxFloat32x4 b = ffxAsFloat(ffxBroadcast4(0x7ef19fffu) - ffxAsUInt32(value));
    return b * (-b * value + ffxBroadcast4(2.0f));
}

/// Calculate a low-quality approximation for the square root of a value.
///
/// For additional information on the approximation family of functions, you can refer to Michal Drobot's excellent
/// presentation materials:
///
///  - https://michaldrobot.files.wordpress.com/2014/05/gcn_alu_opt_digitaldragons2014.pdf
///  - https://github.com/michaldrobot/ShaderFastLibs/blob/master/ShaderFastMathLib.h
///
/// @param [in] value           The value to calculate an approximate to the square root for.
///
/// @returns
/// An approximation of the square root, estimated to low quality.
///
/// @ingroup GPUCore
FfxFloat32x4 ffxApproximateReciprocalSquareRoot(FfxFloat32x4 value)
{
    return ffxAsFloat(ffxBroadcast4(0x5f347d74u) - (ffxAsUInt32(value) >> ffxBroadcast4(1u)));
}

/// Calculate dot product of 'a' and 'b'.
///
/// @param [in] a                   First vector input.
/// @param [in] b                   Second vector input.
///
/// @returns
/// The value of <c><i>a</i></c> dot <c><i>b</i></c>.
///
/// @ingroup GPUCore
FfxFloat32 ffxDot2(FfxFloat32x2 a, FfxFloat32x2 b)
{
    return dot(a, b);
}

/// Calculate dot product of 'a' and 'b'.
///
/// @param [in] a                   First vector input.
/// @param [in] b                   Second vector input.
///
/// @returns
/// The value of <c><i>a</i></c> dot <c><i>b</i></c>.
///
/// @ingroup GPUCore
FfxFloat32 ffxDot3(FfxFloat32x3 a, FfxFloat32x3 b)
{
    return dot(a, b);
}

/// Calculate dot product of 'a' and 'b'.
///
/// @param [in] a                   First vector input.
/// @param [in] b                   Second vector input.
///
/// @returns
/// The value of <c><i>a</i></c> dot <c><i>b</i></c>.
///
/// @ingroup GPUCore
FfxFloat32 ffxDot4(FfxFloat32x4 a, FfxFloat32x4 b)
{
    return dot(a, b);
}


/// Compute an approximate conversion from PQ to Gamma2 space.
///
/// PQ is very close to x^(1/8). The functions below Use the fast FfxFloat32 approximation method to do
/// PQ conversions to and from Gamma2 (4th power and fast 4th root), and PQ to and from Linear 
/// (8th power and fast 8th root). The maximum error is approximately 0.2%.
///
/// @param a                    The value to convert between PQ and Gamma2.
///
/// @returns
/// The value <c><i>a</i></c> converted into Gamma2.
///
/// @ingroup GPUCore
FfxFloat32 ffxApproximatePQToGamma2Medium(FfxFloat32 a)
{
    return a * a * a * a;
}

/// Compute an approximate conversion from PQ to linear space.
///
/// PQ is very close to x^(1/8). The functions below Use the fast FfxFloat32 approximation method to do
/// PQ conversions to and from Gamma2 (4th power and fast 4th root), and PQ to and from Linear
/// (8th power and fast 8th root). The maximum error is approximately 0.2%.
///
/// @param a                    The value to convert between PQ and linear.
///
/// @returns
/// The value <c><i>a</i></c> converted into linear.
///
/// @ingroup GPUCore
FfxFloat32 ffxApproximatePQToLinear(FfxFloat32 a)
{
    return a * a * a * a * a * a * a * a;
}

/// Compute an approximate conversion from gamma2 to PQ space.
///
/// PQ is very close to x^(1/8). The functions below Use the fast FfxFloat32 approximation method to do
/// PQ conversions to and from Gamma2 (4th power and fast 4th root), and PQ to and from Linear
/// (8th power and fast 8th root). The maximum error is approximately 0.2%.
///
/// @param a                    The value to convert between gamma2 and PQ.
///
/// @returns
/// The value <c><i>a</i></c> converted into PQ.
///
/// @ingroup GPUCore
FfxFloat32 ffxApproximateGamma2ToPQ(FfxFloat32 a)
{
    return ffxAsFloat((ffxAsUInt32(a) >> FfxUInt32(2)) + FfxUInt32(0x2F9A4E46));
}

/// Compute a more accurate approximate conversion from gamma2 to PQ space.
///
/// PQ is very close to x^(1/8). The functions below Use the fast FfxFloat32 approximation method to do
/// PQ conversions to and from Gamma2 (4th power and fast 4th root), and PQ to and from Linear
/// (8th power and fast 8th root). The maximum error is approximately 0.2%.
///
/// @param a                    The value to convert between gamma2 and PQ.
///
/// @returns
/// The value <c><i>a</i></c> converted into PQ.
///
/// @ingroup GPUCore
FfxFloat32 ffxApproximateGamma2ToPQMedium(FfxFloat32 a)
{
    FfxFloat32 b  = ffxAsFloat((ffxAsUInt32(a) >> FfxUInt32(2)) + FfxUInt32(0x2F9A4E46));
    FfxFloat32 b4 = b * b * b * b;
    return b - b * (b4 - a) / (FfxFloat32(4.0) * b4);
}

/// Compute a high accuracy approximate conversion from gamma2 to PQ space.
///
/// PQ is very close to x^(1/8). The functions below Use the fast FfxFloat32 approximation method to do
/// PQ conversions to and from Gamma2 (4th power and fast 4th root), and PQ to and from Linear
/// (8th power and fast 8th root). The maximum error is approximately 0.2%.
///
/// @param a                    The value to convert between gamma2 and PQ.
///
/// @returns
/// The value <c><i>a</i></c> converted into PQ.
///
/// @ingroup GPUCore
FfxFloat32 ffxApproximateGamma2ToPQHigh(FfxFloat32 a)
{
    return ffxSqrt(ffxSqrt(a));
}

/// Compute an approximate conversion from linear to PQ space.
///
/// PQ is very close to x^(1/8). The functions below Use the fast FfxFloat32 approximation method to do
/// PQ conversions to and from Gamma2 (4th power and fast 4th root), and PQ to and from Linear
/// (8th power and fast 8th root). The maximum error is approximately 0.2%.
///
/// @param a                    The value to convert between linear and PQ.
///
/// @returns
/// The value <c><i>a</i></c> converted into PQ.
///
/// @ingroup GPUCore
FfxFloat32 ffxApproximateLinearToPQ(FfxFloat32 a)
{
    return ffxAsFloat((ffxAsUInt32(a) >> FfxUInt32(3)) + FfxUInt32(0x378D8723));
}

/// Compute a more accurate approximate conversion from linear to PQ space.
///
/// PQ is very close to x^(1/8). The functions below Use the fast FfxFloat32 approximation method to do
/// PQ conversions to and from Gamma2 (4th power and fast 4th root), and PQ to and from Linear
/// (8th power and fast 8th root). The maximum error is approximately 0.2%.
///
/// @param a                    The value to convert between linear and PQ.
///
/// @returns
/// The value <c><i>a</i></c> converted into PQ.
///
/// @ingroup GPUCore
FfxFloat32 ffxApproximateLinearToPQMedium(FfxFloat32 a)
{
    FfxFloat32 b  = ffxAsFloat((ffxAsUInt32(a) >> FfxUInt32(3)) + FfxUInt32(0x378D8723));
    FfxFloat32 b8 = b * b * b * b * b * b * b * b;
    return b - b * (b8 - a) / (FfxFloat32(8.0) * b8);
}

/// Compute a very accurate approximate conversion from linear to PQ space.
///
/// PQ is very close to x^(1/8). The functions below Use the fast FfxFloat32 approximation method to do
/// PQ conversions to and from Gamma2 (4th power and fast 4th root), and PQ to and from Linear
/// (8th power and fast 8th root). The maximum error is approximately 0.2%.
///
/// @param a                    The value to convert between linear and PQ.
///
/// @returns
/// The value <c><i>a</i></c> converted into PQ.
///
/// @ingroup GPUCore
FfxFloat32 ffxApproximateLinearToPQHigh(FfxFloat32 a)
{
    return ffxSqrt(ffxSqrt(ffxSqrt(a)));
}

/// Compute an approximate conversion from PQ to Gamma2 space.
///
/// PQ is very close to x^(1/8). The functions below Use the fast FfxFloat32 approximation method to do
/// PQ conversions to and from Gamma2 (4th power and fast 4th root), and PQ to and from Linear
/// (8th power and fast 8th root). The maximum error is approximately 0.2%.
///
/// @param a                    The value to convert between PQ and Gamma2.
///
/// @returns
/// The value <c><i>a</i></c> converted into Gamma2.
///
/// @ingroup GPUCore
FfxFloat32x2 ffxApproximatePQToGamma2Medium(FfxFloat32x2 a)
{
    return a * a * a * a;
}

/// Compute an approximate conversion from PQ to linear space.
///
/// PQ is very close to x^(1/8). The functions below Use the fast FfxFloat32 approximation method to do
/// PQ conversions to and from Gamma2 (4th power and fast 4th root), and PQ to and from Linear
/// (8th power and fast 8th root). The maximum error is approximately 0.2%.
///
/// @param a                    The value to convert between PQ and linear.
///
/// @returns
/// The value <c><i>a</i></c> converted into linear.
///
/// @ingroup GPUCore
FfxFloat32x2 ffxApproximatePQToLinear(FfxFloat32x2 a)
{
    return a * a * a * a * a * a * a * a;
}

/// Compute an approximate conversion from gamma2 to PQ space.
///
/// PQ is very close to x^(1/8). The functions below Use the fast FfxFloat32 approximation method to do
/// PQ conversions to and from Gamma2 (4th power and fast 4th root), and PQ to and from Linear
/// (8th power and fast 8th root). The maximum error is approximately 0.2%.
///
/// @param a                    The value to convert between gamma2 and PQ.
///
/// @returns
/// The value <c><i>a</i></c> converted into PQ.
///
/// @ingroup GPUCore
FfxFloat32x2 ffxApproximateGamma2ToPQ(FfxFloat32x2 a)
{
    return ffxAsFloat((ffxAsUInt32(a) >> ffxBroadcast2(2u)) + ffxBroadcast2(0x2F9A4E46u));
}

/// Compute a more accurate approximate conversion from gamma2 to PQ space.
///
/// PQ is very close to x^(1/8). The functions below Use the fast FfxFloat32 approximation method to do
/// PQ conversions to and from Gamma2 (4th power and fast 4th root), and PQ to and from Linear
/// (8th power and fast 8th root). The maximum error is approximately 0.2%.
///
/// @param a                    The value to convert between gamma2 and PQ.
///
/// @returns
/// The value <c><i>a</i></c> converted into PQ.
///
/// @ingroup GPUCore
FfxFloat32x2 ffxApproximateGamma2ToPQMedium(FfxFloat32x2 a)
{
    FfxFloat32x2 b  = ffxAsFloat((ffxAsUInt32(a) >> ffxBroadcast2(2u)) + ffxBroadcast2(0x2F9A4E46u));
    FfxFloat32x2 b4 = b * b * b * b;
    return b - b * (b4 - a) / (FfxFloat32(4.0) * b4);
}

/// Compute a high accuracy approximate conversion from gamma2 to PQ space.
///
/// PQ is very close to x^(1/8). The functions below Use the fast FfxFloat32 approximation method to do
/// PQ conversions to and from Gamma2 (4th power and fast 4th root), and PQ to and from Linear
/// (8th power and fast 8th root). The maximum error is approximately 0.2%.
///
/// @param a                    The value to convert between gamma2 and PQ.
///
/// @returns
/// The value <c><i>a</i></c> converted into PQ.
///
/// @ingroup GPUCore
FfxFloat32x2 ffxApproximateGamma2ToPQHigh(FfxFloat32x2 a)
{
    return ffxSqrt(ffxSqrt(a));
}

/// Compute an approximate conversion from linear to PQ space.
///
/// PQ is very close to x^(1/8). The functions below Use the fast FfxFloat32 approximation method to do
/// PQ conversions to and from Gamma2 (4th power and fast 4th root), and PQ to and from Linear
/// (8th power and fast 8th root). The maximum error is approximately 0.2%.
///
/// @param a                    The value to convert between linear and PQ.
///
/// @returns
/// The value <c><i>a</i></c> converted into PQ.
///
/// @ingroup GPUCore
FfxFloat32x2 ffxApproximateLinearToPQ(FfxFloat32x2 a)
{
    return ffxAsFloat((ffxAsUInt32(a) >> ffxBroadcast2(3u)) + ffxBroadcast2(0x378D8723u));
}

/// Compute a more accurate approximate conversion from linear to PQ space.
///
/// PQ is very close to x^(1/8). The functions below Use the fast FfxFloat32 approximation method to do
/// PQ conversions to and from Gamma2 (4th power and fast 4th root), and PQ to and from Linear
/// (8th power and fast 8th root). The maximum error is approximately 0.2%.
///
/// @param a                    The value to convert between linear and PQ.
///
/// @returns
/// The value <c><i>a</i></c> converted into PQ.
///
/// @ingroup GPUCore
FfxFloat32x2 ffxApproximateLinearToPQMedium(FfxFloat32x2 a)
{
    FfxFloat32x2 b  = ffxAsFloat((ffxAsUInt32(a) >> ffxBroadcast2(3u)) + ffxBroadcast2(0x378D8723u));
    FfxFloat32x2 b8 = b * b * b * b * b * b * b * b;
    return b - b * (b8 - a) / (FfxFloat32(8.0) * b8);
}

/// Compute a very accurate approximate conversion from linear to PQ space.
///
/// PQ is very close to x^(1/8). The functions below Use the fast FfxFloat32 approximation method to do
/// PQ conversions to and from Gamma2 (4th power and fast 4th root), and PQ to and from Linear
/// (8th power and fast 8th root). The maximum error is approximately 0.2%.
///
/// @param a                    The value to convert between linear and PQ.
///
/// @returns
/// The value <c><i>a</i></c> converted into PQ.
///
/// @ingroup GPUCore
FfxFloat32x2 ffxApproximateLinearToPQHigh(FfxFloat32x2 a)
{
    return ffxSqrt(ffxSqrt(ffxSqrt(a)));
}

/// Compute an approximate conversion from PQ to Gamma2 space.
///
/// PQ is very close to x^(1/8). The functions below Use the fast FfxFloat32 approximation method to do
/// PQ conversions to and from Gamma2 (4th power and fast 4th root), and PQ to and from Linear
/// (8th power and fast 8th root). The maximum error is approximately 0.2%.
///
/// @param a                    The value to convert between PQ and Gamma2.
///
/// @returns
/// The value <c><i>a</i></c> converted into Gamma2.
///
/// @ingroup GPUCore
FfxFloat32x3 ffxApproximatePQToGamma2Medium(FfxFloat32x3 a)
{
    return a * a * a * a;
}

/// Compute an approximate conversion from PQ to linear space.
///
/// PQ is very close to x^(1/8). The functions below Use the fast FfxFloat32 approximation method to do
/// PQ conversions to and from Gamma2 (4th power and fast 4th root), and PQ to and from Linear
/// (8th power and fast 8th root). The maximum error is approximately 0.2%.
///
/// @param a                    The value to convert between PQ and linear.
///
/// @returns
/// The value <c><i>a</i></c> converted into linear.
///
/// @ingroup GPUCore
FfxFloat32x3 ffxApproximatePQToLinear(FfxFloat32x3 a)
{
    return a * a * a * a * a * a * a * a;
}

/// Compute an approximate conversion from gamma2 to PQ space.
///
/// PQ is very close to x^(1/8). The functions below Use the fast FfxFloat32 approximation method to do
/// PQ conversions to and from Gamma2 (4th power and fast 4th root), and PQ to and from Linear
/// (8th power and fast 8th root). The maximum error is approximately 0.2%.
///
/// @param a                    The value to convert between gamma2 and PQ.
///
/// @returns
/// The value <c><i>a</i></c> converted into PQ.
///
/// @ingroup GPUCore
FfxFloat32x3 ffxApproximateGamma2ToPQ(FfxFloat32x3 a)
{
    return ffxAsFloat((ffxAsUInt32(a) >> ffxBroadcast3(2u)) + ffxBroadcast3(0x2F9A4E46u));
}

/// Compute a more accurate approximate conversion from gamma2 to PQ space.
///
/// PQ is very close to x^(1/8). The functions below Use the fast FfxFloat32 approximation method to do
/// PQ conversions to and from Gamma2 (4th power and fast 4th root), and PQ to and from Linear
/// (8th power and fast 8th root). The maximum error is approximately 0.2%.
///
/// @param a                    The value to convert between gamma2 and PQ.
///
/// @returns
/// The value <c><i>a</i></c> converted into PQ.
///
/// @ingroup GPUCore
FfxFloat32x3 ffxApproximateGamma2ToPQMedium(FfxFloat32x3 a)
{
    FfxFloat32x3 b  = ffxAsFloat((ffxAsUInt32(a) >> ffxBroadcast3(2u)) + ffxBroadcast3(0x2F9A4E46u));
    FfxFloat32x3 b4 = b * b * b * b;
    return b - b * (b4 - a) / (FfxFloat32(4.0) * b4);
}

/// Compute a high accuracy approximate conversion from gamma2 to PQ space.
///
/// PQ is very close to x^(1/8). The functions below Use the fast FfxFloat32 approximation method to do
/// PQ conversions to and from Gamma2 (4th power and fast 4th root), and PQ to and from Linear
/// (8th power and fast 8th root). The maximum error is approximately 0.2%.
///
/// @param a                    The value to convert between gamma2 and PQ.
///
/// @returns
/// The value <c><i>a</i></c> converted into PQ.
///
/// @ingroup GPUCore
FfxFloat32x3 ffxApproximateGamma2ToPQHigh(FfxFloat32x3 a)
{
    return ffxSqrt(ffxSqrt(a));
}

/// Compute an approximate conversion from linear to PQ space.
///
/// PQ is very close to x^(1/8). The functions below Use the fast FfxFloat32 approximation method to do
/// PQ conversions to and from Gamma2 (4th power and fast 4th root), and PQ to and from Linear
/// (8th power and fast 8th root). The maximum error is approximately 0.2%.
///
/// @param a                    The value to convert between linear and PQ.
///
/// @returns
/// The value <c><i>a</i></c> converted into PQ.
///
/// @ingroup GPUCore
FfxFloat32x3 ffxApproximateLinearToPQ(FfxFloat32x3 a)
{
    return ffxAsFloat((ffxAsUInt32(a) >> ffxBroadcast3(3u)) + ffxBroadcast3(0x378D8723u));
}

/// Compute a more accurate approximate conversion from linear to PQ space.
///
/// PQ is very close to x^(1/8). The functions below Use the fast FfxFloat32 approximation method to do
/// PQ conversions to and from Gamma2 (4th power and fast 4th root), and PQ to and from Linear
/// (8th power and fast 8th root). The maximum error is approximately 0.2%.
///
/// @param a                    The value to convert between linear and PQ.
///
/// @returns
/// The value <c><i>a</i></c> converted into PQ.
///
/// @ingroup GPUCore
FfxFloat32x3 ffxApproximateLinearToPQMedium(FfxFloat32x3 a)
{
    FfxFloat32x3 b  = ffxAsFloat((ffxAsUInt32(a) >> ffxBroadcast3(3u)) + ffxBroadcast3(0x378D8723u));
    FfxFloat32x3 b8 = b * b * b * b * b * b * b * b;
    return b - b * (b8 - a) / (FfxFloat32(8.0) * b8);
}

/// Compute a very accurate approximate conversion from linear to PQ space.
///
/// PQ is very close to x^(1/8). The functions below Use the fast FfxFloat32 approximation method to do
/// PQ conversions to and from Gamma2 (4th power and fast 4th root), and PQ to and from Linear
/// (8th power and fast 8th root). The maximum error is approximately 0.2%.
///
/// @param a                    The value to convert between linear and PQ.
///
/// @returns
/// The value <c><i>a</i></c> converted into PQ.
///
/// @ingroup GPUCore
FfxFloat32x3 ffxApproximateLinearToPQHigh(FfxFloat32x3 a)
{
    return ffxSqrt(ffxSqrt(ffxSqrt(a)));
}

/// Compute an approximate conversion from PQ to Gamma2 space.
///
/// PQ is very close to x^(1/8). The functions below Use the fast FfxFloat32 approximation method to do
/// PQ conversions to and from Gamma2 (4th power and fast 4th root), and PQ to and from Linear
/// (8th power and fast 8th root). The maximum error is approximately 0.2%.
///
/// @param a                    The value to convert between PQ and Gamma2.
///
/// @returns
/// The value <c><i>a</i></c> converted into Gamma2.
///
/// @ingroup GPUCore
FfxFloat32x4 ffxApproximatePQToGamma2Medium(FfxFloat32x4 a)
{
    return a * a * a * a;
}

/// Compute an approximate conversion from PQ to linear space.
///
/// PQ is very close to x^(1/8). The functions below Use the fast FfxFloat32 approximation method to do
/// PQ conversions to and from Gamma2 (4th power and fast 4th root), and PQ to and from Linear
/// (8th power and fast 8th root). The maximum error is approximately 0.2%.
///
/// @param a                    The value to convert between PQ and linear.
///
/// @returns
/// The value <c><i>a</i></c> converted into linear.
///
/// @ingroup GPUCore
FfxFloat32x4 ffxApproximatePQToLinear(FfxFloat32x4 a)
{
    return a * a * a * a * a * a * a * a;
}

/// Compute an approximate conversion from gamma2 to PQ space.
///
/// PQ is very close to x^(1/8). The functions below Use the fast FfxFloat32 approximation method to do
/// PQ conversions to and from Gamma2 (4th power and fast 4th root), and PQ to and from Linear
/// (8th power and fast 8th root). The maximum error is approximately 0.2%.
///
/// @param a                    The value to convert between gamma2 and PQ.
///
/// @returns
/// The value <c><i>a</i></c> converted into PQ.
///
/// @ingroup GPUCore
FfxFloat32x4 ffxApproximateGamma2ToPQ(FfxFloat32x4 a)
{
    return ffxAsFloat((ffxAsUInt32(a) >> ffxBroadcast4(2u)) + ffxBroadcast4(0x2F9A4E46u));
}

/// Compute a more accurate approximate conversion from gamma2 to PQ space.
///
/// PQ is very close to x^(1/8). The functions below Use the fast FfxFloat32 approximation method to do
/// PQ conversions to and from Gamma2 (4th power and fast 4th root), and PQ to and from Linear
/// (8th power and fast 8th root). The maximum error is approximately 0.2%.
///
/// @param a                    The value to convert between gamma2 and PQ.
///
/// @returns
/// The value <c><i>a</i></c> converted into PQ.
///
/// @ingroup GPUCore
FfxFloat32x4 ffxApproximateGamma2ToPQMedium(FfxFloat32x4 a)
{
    FfxFloat32x4 b  = ffxAsFloat((ffxAsUInt32(a) >> ffxBroadcast4(2u)) + ffxBroadcast4(0x2F9A4E46u));
    FfxFloat32x4 b4 = b * b * b * b * b * b * b * b;
    return b - b * (b4 - a) / (FfxFloat32(4.0) * b4);
}

/// Compute a high accuracy approximate conversion from gamma2 to PQ space.
///
/// PQ is very close to x^(1/8). The functions below Use the fast FfxFloat32 approximation method to do
/// PQ conversions to and from Gamma2 (4th power and fast 4th root), and PQ to and from Linear
/// (8th power and fast 8th root). The maximum error is approximately 0.2%.
///
/// @param a                    The value to convert between gamma2 and PQ.
///
/// @returns
/// The value <c><i>a</i></c> converted into PQ.
///
/// @ingroup GPUCore
FfxFloat32x4 ffxApproximateGamma2ToPQHigh(FfxFloat32x4 a)
{
    return ffxSqrt(ffxSqrt(a));
}

/// Compute an approximate conversion from linear to PQ space.
///
/// PQ is very close to x^(1/8). The functions below Use the fast FfxFloat32 approximation method to do
/// PQ conversions to and from Gamma2 (4th power and fast 4th root), and PQ to and from Linear
/// (8th power and fast 8th root). The maximum error is approximately 0.2%.
///
/// @param a                    The value to convert between linear and PQ.
///
/// @returns
/// The value <c><i>a</i></c> converted into PQ.
///
/// @ingroup GPUCore
FfxFloat32x4 ffxApproximateLinearToPQ(FfxFloat32x4 a)
{
    return ffxAsFloat((ffxAsUInt32(a) >> ffxBroadcast4(3u)) + ffxBroadcast4(0x378D8723u));
}

/// Compute a more accurate approximate conversion from linear to PQ space.
///
/// PQ is very close to x^(1/8). The functions below Use the fast FfxFloat32 approximation method to do
/// PQ conversions to and from Gamma2 (4th power and fast 4th root), and PQ to and from Linear
/// (8th power and fast 8th root). The maximum error is approximately 0.2%.
///
/// @param a                    The value to convert between linear and PQ.
///
/// @returns
/// The value <c><i>a</i></c> converted into PQ.
///
/// @ingroup GPUCore
FfxFloat32x4 ffxApproximateLinearToPQMedium(FfxFloat32x4 a)
{
    FfxFloat32x4 b  = ffxAsFloat((ffxAsUInt32(a) >> ffxBroadcast4(3u)) + ffxBroadcast4(0x378D8723u));
    FfxFloat32x4 b8 = b * b * b * b * b * b * b * b;
    return b - b * (b8 - a) / (FfxFloat32(8.0) * b8);
}

/// Compute a very accurate approximate conversion from linear to PQ space.
///
/// PQ is very close to x^(1/8). The functions below Use the fast FfxFloat32 approximation method to do
/// PQ conversions to and from Gamma2 (4th power and fast 4th root), and PQ to and from Linear
/// (8th power and fast 8th root). The maximum error is approximately 0.2%.
///
/// @param a                    The value to convert between linear and PQ.
///
/// @returns
/// The value <c><i>a</i></c> converted into PQ.
///
/// @ingroup GPUCore
FfxFloat32x4 ffxApproximateLinearToPQHigh(FfxFloat32x4 a)
{
    return ffxSqrt(ffxSqrt(ffxSqrt(a)));
}

// An approximation of sine.
//
// Valid input range is {-1 to 1} representing {0 to 2 pi}, and the output range 
// is {-1/4 to 1/4} representing {-1 to 1}.
//
// @param [in] value            The value to calculate approximate sine for.
//
// @returns
// The approximate sine of <c><i>value</i></c>.
FfxFloat32 ffxParabolicSin(FfxFloat32 value)
{
    return value * abs(value) - value;
}

// An approximation of sine.
//
// Valid input range is {-1 to 1} representing {0 to 2 pi}, and the output range
// is {-1/4 to 1/4} representing {-1 to 1}.
//
// @param [in] value            The value to calculate approximate sine for.
//
// @returns
// The approximate sine of <c><i>value</i></c>.
FfxFloat32x2 ffxParabolicSin(FfxFloat32x2 x)
{
    return x * abs(x) - x;
}

// An approximation of cosine.
//
// Valid input range is {-1 to 1} representing {0 to 2 pi}, and the output range
// is {-1/4 to 1/4} representing {-1 to 1}.
//
// @param [in] value            The value to calculate approximate cosine for.
//
// @returns
// The approximate cosine of <c><i>value</i></c>.
FfxFloat32 ffxParabolicCos(FfxFloat32 x)
{
    x = ffxFract(x * FfxFloat32(0.5) + FfxFloat32(0.75));
    x = x * FfxFloat32(2.0) - FfxFloat32(1.0);
    return ffxParabolicSin(x);
}

// An approximation of cosine.
//
// Valid input range is {-1 to 1} representing {0 to 2 pi}, and the output range
// is {-1/4 to 1/4} representing {-1 to 1}.
//
// @param [in] value            The value to calculate approximate cosine for.
//
// @returns
// The approximate cosine of <c><i>value</i></c>.
FfxFloat32x2 ffxParabolicCos(FfxFloat32x2 x)
{
    x = ffxFract(x * ffxBroadcast2(0.5f) + ffxBroadcast2(0.75f));
    x = x * ffxBroadcast2(2.0f) - ffxBroadcast2(1.0f);
    return ffxParabolicSin(x);
}

// An approximation of both sine and cosine.
//
// Valid input range is {-1 to 1} representing {0 to 2 pi}, and the output range
// is {-1/4 to 1/4} representing {-1 to 1}.
//
// @param [in] value            The value to calculate approximate cosine for.
//
// @returns
// A <c><i>FfxFloat32x2</i></c> containing approximations of both sine and cosine of <c><i>value</i></c>.
FfxFloat32x2 ffxParabolicSinCos(FfxFloat32 x)
{
    FfxFloat32 y = ffxFract(x * FfxFloat32(0.5) + FfxFloat32(0.75));
    y = y * FfxFloat32(2.0) - FfxFloat32(1.0);
    return ffxParabolicSin(FfxFloat32x2(x, y));
}

/// Conditional free logic AND operation using values.
///
/// @param [in] x           The first value to be fed into the AND operator.
/// @param [in] y           The second value to be fed into the AND operator.
///
/// @returns
/// Result of the AND operation.
///
/// @ingroup GPUCore
FfxUInt32 ffxZeroOneAnd(FfxUInt32 x, FfxUInt32 y)
{
    return min(x, y);
}

/// Conditional free logic AND operation using two values.
///
/// @param [in] x           The first value to be fed into the AND operator.
/// @param [in] y           The second value to be fed into the AND operator.
///
/// @returns
/// Result of the AND operation.
///
/// @ingroup GPUCore
FfxUInt32x2 ffxZeroOneAnd(FfxUInt32x2 x, FfxUInt32x2 y)
{
    return min(x, y);
}

/// Conditional free logic AND operation using two values.
///
/// @param [in] x           The first value to be fed into the AND operator.
/// @param [in] y           The second value to be fed into the AND operator.
///
/// @returns
/// Result of the AND operation.
///
/// @ingroup GPUCore
FfxUInt32x3 ffxZeroOneAnd(FfxUInt32x3 x, FfxUInt32x3 y)
{
    return min(x, y);
}

/// Conditional free logic AND operation using two values.
///
/// @param [in] x           The first value to be fed into the AND operator.
/// @param [in] y           The second value to be fed into the AND operator.
///
/// @returns
/// Result of the AND operation.
///
/// @ingroup GPUCore
FfxUInt32x4 ffxZeroOneAnd(FfxUInt32x4 x, FfxUInt32x4 y)
{
    return min(x, y);
}

/// Conditional free logic NOT operation using two values.
///
/// @param [in] x           The first value to be fed into the NOT operator.
///
/// @returns
/// Result of the NOT operation.
///
/// @ingroup GPUCore
FfxUInt32 ffxZeroOneAnd(FfxUInt32 x)
{
    return x ^ FfxUInt32(1);
}

/// Conditional free logic NOT operation using two values.
///
/// @param [in] x           The first value to be fed into the NOT operator.
///
/// @returns
/// Result of the NOT operation.
///
/// @ingroup GPUCore
FfxUInt32x2 ffxZeroOneAnd(FfxUInt32x2 x)
{
    return x ^ ffxBroadcast2(1u);
}

/// Conditional free logic NOT operation using two values.
///
/// @param [in] x           The first value to be fed into the NOT operator.
///
/// @returns
/// Result of the NOT operation.
///
/// @ingroup GPUCore
FfxUInt32x3 ffxZeroOneAnd(FfxUInt32x3 x)
{
    return x ^ ffxBroadcast3(1u);
}

/// Conditional free logic NOT operation using two values.
///
/// @param [in] x           The first value to be fed into the NOT operator.
///
/// @returns
/// Result of the NOT operation.
///
/// @ingroup GPUCore
FfxUInt32x4 ffxZeroOneAnd(FfxUInt32x4 x)
{
    return x ^ ffxBroadcast4(1u);
}

/// Conditional free logic OR operation using two values.
///
/// @param [in] x           The first value to be fed into the OR operator.
/// @param [in] y           The second value to be fed into the OR operator.
///
/// @returns
/// Result of the OR operation.
///
/// @ingroup GPUCore
FfxUInt32 ffxZeroOneOr(FfxUInt32 x, FfxUInt32 y)
{
    return max(x, y);
}

/// Conditional free logic OR operation using two values.
///
/// @param [in] x           The first value to be fed into the OR operator.
/// @param [in] y           The second value to be fed into the OR operator.
///
/// @returns
/// Result of the OR operation.
///
/// @ingroup GPUCore
FfxUInt32x2 ffxZeroOneOr(FfxUInt32x2 x, FfxUInt32x2 y)
{
    return max(x, y);
}

/// Conditional free logic OR operation using two values.
///
/// @param [in] x           The first value to be fed into the OR operator.
/// @param [in] y           The second value to be fed into the OR operator.
///
/// @returns
/// Result of the OR operation.
///
/// @ingroup GPUCore
FfxUInt32x3 ffxZeroOneOr(FfxUInt32x3 x, FfxUInt32x3 y)
{
    return max(x, y);
}

/// Conditional free logic OR operation using two values.
///
/// @param [in] x           The first value to be fed into the OR operator.
/// @param [in] y           The second value to be fed into the OR operator.
///
/// @returns
/// Result of the OR operation.
///
/// @ingroup GPUCore
FfxUInt32x4 ffxZeroOneOr(FfxUInt32x4 x, FfxUInt32x4 y)
{
    return max(x, y);
}

/// Conditional free logic signed NOT operation using two half-precision FfxFloat32 values.
///
/// @param [in] x           The first value to be fed into the AND OR operator.
///
/// @returns
/// Result of the AND OR operation.
///
/// @ingroup GPUCore
FfxUInt32 ffxZeroOneAndToU1(FfxFloat32 x)
{
    return FfxUInt32(FfxFloat32(1.0) - x);
}

/// Conditional free logic signed NOT operation using two half-precision FfxFloat32 values.
///
/// @param [in] x           The first value to be fed into the AND OR operator.
///
/// @returns
/// Result of the AND OR operation.
///
/// @ingroup GPUCore
FfxUInt32x2 ffxZeroOneAndToU2(FfxFloat32x2 x)
{
    return FfxUInt32x2(ffxBroadcast2(1.0) - x);
}

/// Conditional free logic signed NOT operation using two half-precision FfxFloat32 values.
///
/// @param [in] x           The first value to be fed into the AND OR operator.
///
/// @returns
/// Result of the AND OR operation.
///
/// @ingroup GPUCore
FfxUInt32x3 ffxZeroOneAndToU3(FfxFloat32x3 x)
{
    return FfxUInt32x3(ffxBroadcast3(1.0) - x);
}

/// Conditional free logic signed NOT operation using two half-precision FfxFloat32 values.
///
/// @param [in] x           The first value to be fed into the AND OR operator.
///
/// @returns
/// Result of the AND OR operation.
///
/// @ingroup GPUCore
FfxUInt32x4 ffxZeroOneAndToU4(FfxFloat32x4 x)
{
    return FfxUInt32x4(ffxBroadcast4(1.0) - x);
}

/// Conditional free logic AND operation using two values followed by a NOT operation
/// using the resulting value and a third value.
///
/// @param [in] x           The first value to be fed into the AND operator.
/// @param [in] y           The second value to be fed into the AND operator.
/// @param [in] z           The second value to be fed into the OR operator.
///
/// @returns
/// Result of the AND OR operation.
///
/// @ingroup GPUCore
FfxFloat32 ffxZeroOneAndOr(FfxFloat32 x, FfxFloat32 y, FfxFloat32 z)
{
    return ffxSaturate(x * y + z);
}

/// Conditional free logic AND operation using two values followed by a NOT operation
/// using the resulting value and a third value.
///
/// @param [in] x           The first value to be fed into the AND operator.
/// @param [in] y           The second value to be fed into the AND operator.
/// @param [in] z           The second value to be fed into the OR operator.
///
/// @returns
/// Result of the AND OR operation.
///
/// @ingroup GPUCore
FfxFloat32x2 ffxZeroOneAndOr(FfxFloat32x2 x, FfxFloat32x2 y, FfxFloat32x2 z)
{
    return ffxSaturate(x * y + z);
}

/// Conditional free logic AND operation using two values followed by a NOT operation
/// using the resulting value and a third value.
///
/// @param [in] x           The first value to be fed into the AND operator.
/// @param [in] y           The second value to be fed into the AND operator.
/// @param [in] z           The second value to be fed into the OR operator.
///
/// @returns
/// Result of the AND OR operation.
///
/// @ingroup GPUCore
FfxFloat32x3 ffxZeroOneAndOr(FfxFloat32x3 x, FfxFloat32x3 y, FfxFloat32x3 z)
{
    return ffxSaturate(x * y + z);
}

/// Conditional free logic AND operation using two values followed by a NOT operation 
/// using the resulting value and a third value.
///
/// @param [in] x           The first value to be fed into the AND operator.
/// @param [in] y           The second value to be fed into the AND operator.
/// @param [in] z           The second value to be fed into the OR operator.
///
/// @returns
/// Result of the AND OR operation.
///
/// @ingroup GPUCore
FfxFloat32x4 ffxZeroOneAndOr(FfxFloat32x4 x, FfxFloat32x4 y, FfxFloat32x4 z)
{
    return ffxSaturate(x * y + z);
}

/// Given a value, returns 1.0 if greater than zero and 0.0 if not.
///
/// @param [in] x           The value to be compared.
///
/// @returns
/// Result of the greater than zero comparison.
///
/// @ingroup GPUCore
FfxFloat32 ffxZeroOneIsGreaterThanZero(FfxFloat32 x)
{
    return ffxSaturate(x * FfxFloat32(FFX_POSITIVE_INFINITY_FLOAT));
}

/// Given a value, returns 1.0 if greater than zero and 0.0 if not.
///
/// @param [in] x           The value to be compared.
///
/// @returns
/// Result of the greater than zero comparison.
///
/// @ingroup GPUCore
FfxFloat32x2 ffxZeroOneIsGreaterThanZero(FfxFloat32x2 x)
{
    return ffxSaturate(x * ffxBroadcast2(FFX_POSITIVE_INFINITY_FLOAT));
}

/// Given a value, returns 1.0 if greater than zero and 0.0 if not.
///
/// @param [in] x           The value to be compared.
///
/// @returns
/// Result of the greater than zero comparison.
///
/// @ingroup GPUCore
FfxFloat32x3 ffxZeroOneIsGreaterThanZero(FfxFloat32x3 x)
{
    return ffxSaturate(x * ffxBroadcast3(FFX_POSITIVE_INFINITY_FLOAT));
}

/// Given a value, returns 1.0 if greater than zero and 0.0 if not.
///
/// @param [in] x           The value to be compared.
///
/// @returns
/// Result of the greater than zero comparison.
///
/// @ingroup GPUCore
FfxFloat32x4 ffxZeroOneIsGreaterThanZero(FfxFloat32x4 x)
{
    return ffxSaturate(x * ffxBroadcast4(FFX_POSITIVE_INFINITY_FLOAT));
}

/// Conditional free logic signed NOT operation using two FfxFloat32 values.
///
/// @param [in] x           The first value to be fed into the AND OR operator.
///
/// @returns
/// Result of the AND OR operation.
///
/// @ingroup GPUCore
FfxFloat32 ffxZeroOneAnd(FfxFloat32 x)
{
    return FfxFloat32(1.0) - x;
}

/// Conditional free logic signed NOT operation using two FfxFloat32 values.
///
/// @param [in] x           The first value to be fed into the AND OR operator.
///
/// @returns
/// Result of the AND OR operation.
///
/// @ingroup GPUCore
FfxFloat32x2 ffxZeroOneAnd(FfxFloat32x2 x)
{
    return ffxBroadcast2(1.0) - x;
}

/// Conditional free logic signed NOT operation using two FfxFloat32 values.
///
/// @param [in] x           The first value to be fed into the AND OR operator.
///
/// @returns
/// Result of the AND OR operation.
///
/// @ingroup GPUCore
FfxFloat32x3 ffxZeroOneAnd(FfxFloat32x3 x)
{
    return ffxBroadcast3(1.0) - x;
}

/// Conditional free logic signed NOT operation using two FfxFloat32 values.
///
/// @param [in] x           The first value to be fed into the AND OR operator.
///
/// @returns
/// Result of the AND OR operation.
///
/// @ingroup GPUCore
FfxFloat32x4 ffxZeroOneAnd(FfxFloat32x4 x)
{
    return ffxBroadcast4(1.0) - x;
}

/// Conditional free logic OR operation using two FfxFloat32 values.
///
/// @param [in] x           The first value to be fed into the OR operator.
/// @param [in] y           The second value to be fed into the OR operator.
///
/// @returns
/// Result of the OR operation.
///
/// @ingroup GPUCore
FfxFloat32 ffxZeroOneOr(FfxFloat32 x, FfxFloat32 y)
{
    return max(x, y);
}

/// Conditional free logic OR operation using two FfxFloat32 values.
///
/// @param [in] x           The first value to be fed into the OR operator.
/// @param [in] y           The second value to be fed into the OR operator.
///
/// @returns
/// Result of the OR operation.
///
/// @ingroup GPUCore
FfxFloat32x2 ffxZeroOneOr(FfxFloat32x2 x, FfxFloat32x2 y)
{
    return max(x, y);
}

/// Conditional free logic OR operation using two FfxFloat32 values.
///
/// @param [in] x           The first value to be fed into the OR operator.
/// @param [in] y           The second value to be fed into the OR operator.
///
/// @returns
/// Result of the OR operation.
///
/// @ingroup GPUCore
FfxFloat32x3 ffxZeroOneOr(FfxFloat32x3 x, FfxFloat32x3 y)
{
    return max(x, y);
}

/// Conditional free logic OR operation using two FfxFloat32 values.
///
/// @param [in] x           The first value to be fed into the OR operator.
/// @param [in] y           The second value to be fed into the OR operator.
///
/// @returns
/// Result of the OR operation.
///
/// @ingroup GPUCore
FfxFloat32x4 ffxZeroOneOr(FfxFloat32x4 x, FfxFloat32x4 y)
{
    return max(x, y);
}

/// Choose between two FfxFloat32 values if the first paramter is greater than zero.
///
/// @param [in] x           The value to compare against zero.
/// @param [in] y           The value to return if the comparision is greater than zero.
/// @param [in] z           The value to return if the comparision is less than or equal to zero.
///
/// @returns
/// The selected value.
///
/// @ingroup GPUCore
FfxFloat32 ffxZeroOneSelect(FfxFloat32 x, FfxFloat32 y, FfxFloat32 z)
{
    FfxFloat32 r = (-x) * z + z;
    return x * y + r;
}

/// Choose between two FfxFloat32 values if the first paramter is greater than zero.
///
/// @param [in] x           The value to compare against zero.
/// @param [in] y           The value to return if the comparision is greater than zero.
/// @param [in] z           The value to return if the comparision is less than or equal to zero.
///
/// @returns
/// The selected value.
///
/// @ingroup GPUCore
FfxFloat32x2 ffxZeroOneSelect(FfxFloat32x2 x, FfxFloat32x2 y, FfxFloat32x2 z)
{
    FfxFloat32x2 r = (-x) * z + z;
    return x * y + r;
}

/// Choose between two FfxFloat32 values if the first paramter is greater than zero.
///
/// @param [in] x           The value to compare against zero.
/// @param [in] y           The value to return if the comparision is greater than zero.
/// @param [in] z           The value to return if the comparision is less than or equal to zero.
///
/// @returns
/// The selected value.
///
/// @ingroup GPUCore
FfxFloat32x3 ffxZeroOneSelect(FfxFloat32x3 x, FfxFloat32x3 y, FfxFloat32x3 z)
{
    FfxFloat32x3 r = (-x) * z + z;
    return x * y + r;
}

/// Choose between two FfxFloat32 values if the first paramter is greater than zero.
///
/// @param [in] x           The value to compare against zero.
/// @param [in] y           The value to return if the comparision is greater than zero.
/// @param [in] z           The value to return if the comparision is less than or equal to zero.
///
/// @returns
/// The selected value.
///
/// @ingroup GPUCore
FfxFloat32x4 ffxZeroOneSelect(FfxFloat32x4 x, FfxFloat32x4 y, FfxFloat32x4 z)
{
    FfxFloat32x4 r = (-x) * z + z;
    return x * y + r;
}

/// Given a value, returns 1.0 if less than zero and 0.0 if not.
///
/// @param [in] x           The value to be compared.
///
/// @returns
/// Result of the sign value.
///
/// @ingroup GPUCore
FfxFloat32 ffxZeroOneIsSigned(FfxFloat32 x)
{
    return ffxSaturate(x * FfxFloat32(FFX_NEGATIVE_INFINITY_FLOAT));
}

/// Given a value, returns 1.0 if less than zero and 0.0 if not.
///
/// @param [in] x           The value to be compared.
///
/// @returns
/// Result of the sign value.
///
/// @ingroup GPUCore
FfxFloat32x2 ffxZeroOneIsSigned(FfxFloat32x2 x)
{
    return ffxSaturate(x * ffxBroadcast2(FFX_NEGATIVE_INFINITY_FLOAT));
}

/// Given a value, returns 1.0 if less than zero and 0.0 if not.
///
/// @param [in] x           The value to be compared.
///
/// @returns
/// Result of the sign value.
///
/// @ingroup GPUCore
FfxFloat32x3 ffxZeroOneIsSigned(FfxFloat32x3 x)
{
    return ffxSaturate(x * ffxBroadcast3(FFX_NEGATIVE_INFINITY_FLOAT));
}

/// Given a value, returns 1.0 if less than zero and 0.0 if not.
///
/// @param [in] x           The value to be compared.
///
/// @returns
/// Result of the sign value.
///
/// @ingroup GPUCore
FfxFloat32x4 ffxZeroOneIsSigned(FfxFloat32x4 x)
{
    return ffxSaturate(x * ffxBroadcast4(FFX_NEGATIVE_INFINITY_FLOAT));
}

/// Compute a Rec.709 color space.
/// 
/// Rec.709 is used for some HDTVs.
/// 
/// Both Rec.709 and sRGB have a linear segment which as spec'ed would intersect the curved segment 2 times.
///  (a.) For 8-bit sRGB, steps {0 to 10.3} are in the linear region (4% of the encoding range).
///  (b.) For 8-bit  709, steps {0 to 20.7} are in the linear region (8% of the encoding range).
///
/// @param [in] color           The color to convert to Rec. 709.
/// 
/// @returns
/// The <c><i>color</i></c> in linear space.
/// 
/// @ingroup GPUCore
FfxFloat32 ffxRec709FromLinear(FfxFloat32 color)
{
    FfxFloat32x3 j = FfxFloat32x3(0.018 * 4.5, 4.5, 0.45);
    FfxFloat32x2 k = FfxFloat32x2(1.099, -0.099);
    return clamp(j.x, color * j.y, pow(color, j.z) * k.x + k.y);
}

/// Compute a Rec.709 color space.
///
/// Rec.709 is used for some HDTVs.
///
/// Both Rec.709 and sRGB have a linear segment which as spec'ed would intersect the curved segment 2 times.
///  (a.) For 8-bit sRGB, steps {0 to 10.3} are in the linear region (4% of the encoding range).
///  (b.) For 8-bit  709, steps {0 to 20.7} are in the linear region (8% of the encoding range).
///
/// @param [in] color           The color to convert to Rec. 709.
///
/// @returns
/// The <c><i>color</i></c> in linear space.
///
/// @ingroup GPUCore
FfxFloat32x2 ffxRec709FromLinear(FfxFloat32x2 color)
{
    FfxFloat32x3 j = FfxFloat32x3(0.018 * 4.5, 4.5, 0.45);
    FfxFloat32x2 k = FfxFloat32x2(1.099, -0.099);
    return clamp(j.xx, color * j.yy, pow(color, j.zz) * k.xx + k.yy);
}

/// Compute a Rec.709 color space.
///
/// Rec.709 is used for some HDTVs.
///
/// Both Rec.709 and sRGB have a linear segment which as spec'ed would intersect the curved segment 2 times.
///  (a.) For 8-bit sRGB, steps {0 to 10.3} are in the linear region (4% of the encoding range).
///  (b.) For 8-bit  709, steps {0 to 20.7} are in the linear region (8% of the encoding range).
///
/// @param [in] color           The color to convert to Rec. 709.
///
/// @returns
/// The <c><i>color</i></c> in linear space.
///
/// @ingroup GPUCore
FfxFloat32x3 ffxRec709FromLinear(FfxFloat32x3 color)
{
    FfxFloat32x3 j = FfxFloat32x3(0.018 * 4.5, 4.5, 0.45);
    FfxFloat32x2 k = FfxFloat32x2(1.099, -0.099);
    return clamp(j.xxx, color * j.yyy, pow(color, j.zzz) * k.xxx + k.yyy);
}

/// Compute a linear value from a REC.709 value.
///
/// @param [in] color           The value to convert to linear from REC.709.
///
/// @returns
/// A value in linear space.
///
/// @ingroup GPUCore
FfxFloat32 ffxLinearFromRec709(FfxFloat32 color)
{
    FfxFloat32x3 j = FfxFloat32x3(0.081 / 4.5, 1.0 / 4.5, 1.0 / 0.45);
    FfxFloat32x2 k = FfxFloat32x2(1.0 / 1.099, 0.099 / 1.099);
    return ffxZeroOneSelect(ffxZeroOneIsSigned(color - j.x), color * j.y, pow(color * k.x + k.y, j.z));
}

/// Compute a linear value from a REC.709 value.
///
/// @param [in] color           The value to convert to linear from REC.709.
///
/// @returns
/// A value in linear space.
///
/// @ingroup GPUCore
FfxFloat32x2 ffxLinearFromRec709(FfxFloat32x2 color)
{
    FfxFloat32x3 j = FfxFloat32x3(0.081 / 4.5, 1.0 / 4.5, 1.0 / 0.45);
    FfxFloat32x2 k = FfxFloat32x2(1.0 / 1.099, 0.099 / 1.099);
    return ffxZeroOneSelect(ffxZeroOneIsSigned(color - j.xx), color * j.yy, pow(color * k.xx + k.yy, j.zz));
}

/// Compute a linear value from a REC.709 value.
///
/// @param [in] color           The value to convert to linear from REC.709.
///
/// @returns
/// A value in linear space.
///
/// @ingroup GPUCore
FfxFloat32x3 ffxLinearFromRec709(FfxFloat32x3 color)
{
    FfxFloat32x3 j = FfxFloat32x3(0.081 / 4.5, 1.0 / 4.5, 1.0 / 0.45);
    FfxFloat32x2 k = FfxFloat32x2(1.0 / 1.099, 0.099 / 1.099);
    return ffxZeroOneSelect(ffxZeroOneIsSigned(color - j.xxx), color * j.yyy, pow(color * k.xxx + k.yyy, j.zzz));
}

/// Compute a gamma value from a linear value.
///
/// Typically 2.2 for some PC displays, or 2.4-2.5 for CRTs, or 2.2 FreeSync2 native.
/// 
/// Note: 'rcpX' is '1/x', where the 'x' is what would be used in <c><i>ffxLinearFromGamma</i></c>.
/// 
/// @param [in] value           The value to convert to gamma space from linear.
/// @param [in] power           The reciprocal of power value used for the gamma curve.
///
/// @returns
/// A value in gamma space.
///
/// @ingroup GPUCore
FfxFloat32 ffxGammaFromLinear(FfxFloat32 value, FfxFloat32 power)
{
    return pow(value, FfxFloat32(power));
}

/// Compute a gamma value from a linear value.
///
/// Typically 2.2 for some PC displays, or 2.4-2.5 for CRTs, or 2.2 FreeSync2 native.
/// 
/// Note: 'rcpX' is '1/x', where the 'x' is what would be used in <c><i>ffxLinearFromGamma</i></c>.
///
/// @param [in] value           The value to convert to gamma space from linear.
/// @param [in] power           The reciprocal of power value used for the gamma curve.
///
/// @returns
/// A value in gamma space.
///
/// @ingroup GPUCore
FfxFloat32x2 ffxGammaFromLinear(FfxFloat32x2 value, FfxFloat32 power)
{
    return pow(value, ffxBroadcast2(power));
}

/// Compute a gamma value from a linear value.
///
/// Typically 2.2 for some PC displays, or 2.4-2.5 for CRTs, or 2.2 FreeSync2 native.
///
/// Note: 'rcpX' is '1/x', where the 'x' is what would be used in <c><i>ffxLinearFromGamma</i></c>.
///
/// @param [in] value           The value to convert to gamma space from linear.
/// @param [in] power           The reciprocal of power value used for the gamma curve.
///
/// @returns
/// A value in gamma space.
///
/// @ingroup GPUCore
FfxFloat32x3 ffxGammaFromLinear(FfxFloat32x3 value, FfxFloat32 power)
{
    return pow(value, ffxBroadcast3(power));
}

/// Compute a linear value from a value in a gamma space.
///
/// Typically 2.2 for some PC displays, or 2.4-2.5 for CRTs, or 2.2 FreeSync2 native.
///
/// @param [in] color           The value to convert to linear in gamma space.
/// @param [in] power           The power value used for the gamma curve.
///
/// @returns
/// A value in linear space.
///
/// @ingroup GPUCore
FfxFloat32 ffxLinearFromGamma(FfxFloat32 color, FfxFloat32 power)
{
    return pow(color, FfxFloat32(power));
}

/// Compute a linear value from a value in a gamma space.
///
/// Typically 2.2 for some PC displays, or 2.4-2.5 for CRTs, or 2.2 FreeSync2 native.
///
/// @param [in] color           The value to convert to linear in gamma space.
/// @param [in] power           The power value used for the gamma curve.
///
/// @returns
/// A value in linear space.
///
/// @ingroup GPUCore
FfxFloat32x2 ffxLinearFromGamma(FfxFloat32x2 color, FfxFloat32 power)
{
    return pow(color, ffxBroadcast2(power));
}

/// Compute a linear value from a value in a gamma space.
///
/// Typically 2.2 for some PC displays, or 2.4-2.5 for CRTs, or 2.2 FreeSync2 native.
///
/// @param [in] color           The value to convert to linear in gamma space.
/// @param [in] power           The power value used for the gamma curve.
///
/// @returns
/// A value in linear space.
///
/// @ingroup GPUCore
FfxFloat32x3 ffxLinearFromGamma(FfxFloat32x3 color, FfxFloat32 power)
{
    return pow(color, ffxBroadcast3(power));
}

/// Compute a PQ value from a linear value.
///
/// @param [in] value           The value to convert to PQ from linear.
///
/// @returns
/// A value in linear space.
///
/// @ingroup GPUCore
FfxFloat32 ffxPQFromLinear(FfxFloat32 value)
{
    FfxFloat32 p = pow(value, FfxFloat32(0.159302));
    return pow((FfxFloat32(0.835938) + FfxFloat32(18.8516) * p) / (FfxFloat32(1.0) + FfxFloat32(18.6875) * p), FfxFloat32(78.8438));
}

/// Compute a PQ value from a linear value.
///
/// @param [in] value           The value to convert to PQ from linear.
///
/// @returns
/// A value in linear space.
///
/// @ingroup GPUCore
FfxFloat32x2 ffxPQFromLinear(FfxFloat32x2 value)
{
    FfxFloat32x2 p = pow(value, ffxBroadcast2(0.159302));
    return pow((ffxBroadcast2(0.835938) + ffxBroadcast2(18.8516) * p) / (ffxBroadcast2(1.0) + ffxBroadcast2(18.6875) * p), ffxBroadcast2(78.8438));
}

/// Compute a PQ value from a linear value.
///
/// @param [in] value           The value to convert to PQ from linear.
///
/// @returns
/// A value in linear space.
///
/// @ingroup GPUCore
FfxFloat32x3 ffxPQFromLinear(FfxFloat32x3 value)
{
    FfxFloat32x3 p = pow(value, ffxBroadcast3(0.159302));
    return pow((ffxBroadcast3(0.835938) + ffxBroadcast3(18.8516) * p) / (ffxBroadcast3(1.0) + ffxBroadcast3(18.6875) * p), ffxBroadcast3(78.8438));
}

/// Compute a linear value from a value in a PQ space.
///
/// Typically 2.2 for some PC displays, or 2.4-2.5 for CRTs, or 2.2 FreeSync2 native.
///
/// @param [in] value           The value to convert to linear in PQ space.
///
/// @returns
/// A value in linear space.
///
/// @ingroup GPUCore
FfxFloat32 ffxLinearFromPQ(FfxFloat32 value)
{
    FfxFloat32 p = pow(value, FfxFloat32(0.0126833));
    return pow(ffxSaturate(p - FfxFloat32(0.835938)) / (FfxFloat32(18.8516) - FfxFloat32(18.6875) * p), FfxFloat32(6.27739));
}

/// Compute a linear value from a value in a PQ space.
///
/// Typically 2.2 for some PC displays, or 2.4-2.5 for CRTs, or 2.2 FreeSync2 native.
///
/// @param [in] value           The value to convert to linear in PQ space.
///
/// @returns
/// A value in linear space.
///
/// @ingroup GPUCore
FfxFloat32x2 ffxLinearFromPQ(FfxFloat32x2 value)
{
    FfxFloat32x2 p = pow(value, ffxBroadcast2(0.0126833));
    return pow(ffxSaturate(p - ffxBroadcast2(0.835938)) / (ffxBroadcast2(18.8516) - ffxBroadcast2(18.6875) * p), ffxBroadcast2(6.27739));
}

/// Compute a linear value from a value in a PQ space.
///
/// Typically 2.2 for some PC displays, or 2.4-2.5 for CRTs, or 2.2 FreeSync2 native.
///
/// @param [in] value           The value to convert to linear in PQ space.
///
/// @returns
/// A value in linear space.
///
/// @ingroup GPUCore
FfxFloat32x3 ffxLinearFromPQ(FfxFloat32x3 value)
{
    FfxFloat32x3 p = pow(value, ffxBroadcast3(0.0126833));
    return pow(ffxSaturate(p - ffxBroadcast3(0.835938)) / (ffxBroadcast3(18.8516) - ffxBroadcast3(18.6875) * p), ffxBroadcast3(6.27739));
}

/// Compute an SRGB value from a linear value.
///
/// @param [in] value           The value to convert to SRGB from linear.
///
/// @returns
/// A value in SRGB space.
///
/// @ingroup GPUCore
FfxFloat32 ffxSrgbFromLinear(FfxFloat32 value)
{
    FfxFloat32x3 j = FfxFloat32x3(0.0031308 * 12.92, 12.92, 1.0 / 2.4);
    FfxFloat32x2 k = FfxFloat32x2(1.055, -0.055);
    return clamp(j.x, value * j.y, pow(value, j.z) * k.x + k.y);
}

/// Compute an SRGB value from a linear value.
///
/// @param [in] value           The value to convert to SRGB from linear.
///
/// @returns
/// A value in SRGB space.
///
/// @ingroup GPUCore
FfxFloat32x2 ffxSrgbFromLinear(FfxFloat32x2 value)
{
    FfxFloat32x3 j = FfxFloat32x3(0.0031308 * 12.92, 12.92, 1.0 / 2.4);
    FfxFloat32x2 k = FfxFloat32x2(1.055, -0.055);
    return clamp(j.xx, value * j.yy, pow(value, j.zz) * k.xx + k.yy);
}

/// Compute an SRGB value from a linear value.
///
/// @param [in] value           The value to convert to SRGB from linear.
///
/// @returns
/// A value in SRGB space.
///
/// @ingroup GPUCore
FfxFloat32x3 ffxSrgbFromLinear(FfxFloat32x3 value)
{
    FfxFloat32x3 j = FfxFloat32x3(0.0031308 * 12.92, 12.92, 1.0 / 2.4);
    FfxFloat32x2 k = FfxFloat32x2(1.055, -0.055);
    return clamp(j.xxx, value * j.yyy, pow(value, j.zzz) * k.xxx + k.yyy);
}

/// Compute a linear value from a value in a SRGB space.
///
/// Typically 2.2 for some PC displays, or 2.4-2.5 for CRTs, or 2.2 FreeSync2 native.
///
/// @param [in] value           The value to convert to linear in SRGB space.
///
/// @returns
/// A value in linear space.
///
/// @ingroup GPUCore
FfxFloat32 ffxLinearFromSrgb(FfxFloat32 value)
{
    FfxFloat32x3 j = FfxFloat32x3(0.04045, 1.0 / 12.92, 2.4);
    FfxFloat32x2 k = FfxFloat32x2(1.0 / 1.055, 0.055 / 1.055);
    return ffxZeroOneSelect(ffxZeroOneIsSigned(value - j.x), value * j.y, pow(value * k.x + k.y, j.z));
}

/// Compute a linear value from a value in a SRGB space.
///
/// Typically 2.2 for some PC displays, or 2.4-2.5 for CRTs, or 2.2 FreeSync2 native.
///
/// @param [in] value           The value to convert to linear in SRGB space.
///
/// @returns
/// A value in linear space.
///
/// @ingroup GPUCore
FfxFloat32x2 ffxLinearFromSrgb(FfxFloat32x2 value)
{
    FfxFloat32x3 j = FfxFloat32x3(0.04045, 1.0 / 12.92, 2.4);
    FfxFloat32x2 k = FfxFloat32x2(1.0 / 1.055, 0.055 / 1.055);
    return ffxZeroOneSelect(ffxZeroOneIsSigned(value - j.xx), value * j.yy, pow(value * k.xx + k.yy, j.zz));
}

/// Compute a linear value from a value in a SRGB space.
///
/// Typically 2.2 for some PC displays, or 2.4-2.5 for CRTs, or 2.2 FreeSync2 native.
///
/// @param [in] value           The value to convert to linear in SRGB space.
///
/// @returns
/// A value in linear space.
///
/// @ingroup GPUCore
FfxFloat32x3 ffxLinearFromSrgb(FfxFloat32x3 value)
{
    FfxFloat32x3 j = FfxFloat32x3(0.04045, 1.0 / 12.92, 2.4);
    FfxFloat32x2 k = FfxFloat32x2(1.0 / 1.055, 0.055 / 1.055);
    return ffxZeroOneSelect(ffxZeroOneIsSigned(value - j.xxx), value * j.yyy, pow(value * k.xxx + k.yyy, j.zzz));
}

/// A remapping of 64x1 to 8x8 imposing rotated 2x2 pixel quads in quad linear.
///
/// Remap illustration:
///
///     543210
///     ~~~~~~
///     ..xxx.
///     yy...y
/// 
/// @param [in] a       The input 1D coordinates to remap.
///
/// @returns
/// The remapped 2D coordinates.
///
/// @ingroup GPUCore
FfxUInt32x2 ffxRemapForQuad(FfxUInt32 a)
{
    return FfxUInt32x2(ffxBitfieldExtract(a, 1u, 3u), ffxBitfieldInsertMask(ffxBitfieldExtract(a, 3u, 3u), a, 1u));
}

/// A helper function performing a remap 64x1 to 8x8 remapping which is necessary for 2D wave reductions.
///
/// The 64-wide lane indices to 8x8 remapping is performed as follows:
/// 
///     00 01 08 09 10 11 18 19
///     02 03 0a 0b 12 13 1a 1b
///     04 05 0c 0d 14 15 1c 1d
///     06 07 0e 0f 16 17 1e 1f
///     20 21 28 29 30 31 38 39
///     22 23 2a 2b 32 33 3a 3b
///     24 25 2c 2d 34 35 3c 3d
///     26 27 2e 2f 36 37 3e 3f
///
/// @param [in] a       The input 1D coordinate to remap.
/// 
/// @returns
/// The remapped 2D coordinates.
/// 
/// @ingroup GPUCore
FfxUInt32x2 ffxRemapForWaveReduction(FfxUInt32 a)
{
    return FfxUInt32x2(ffxBitfieldInsertMask(ffxBitfieldExtract(a, 2u, 3u), a, 1u), ffxBitfieldInsertMask(ffxBitfieldExtract(a, 3u, 3u), ffxBitfieldExtract(a, 1u, 2u), 2u));
}

#endif // FFX_CORE_H