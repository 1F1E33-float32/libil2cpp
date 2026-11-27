#include "il2cpp-config.h"
#include "MathF.h"

#include <cmath>
#include <limits>

namespace il2cpp
{
namespace icalls
{
namespace System
{
namespace Private
{
namespace CoreLib
{
namespace System
{
    static bool IsInteger(float value)
    {
        double unused;
        return std::modf(value, &unused) == 0.0;
    }

    // Use this function to test for odd integers instead of converting a
    // double to int64_t then ANDing with 1 (or modulo). In C++, double to integer
    // conversions are truncated but the behavior is undefined if the truncated
    // value cannot be represented in the destination type. This means that huge
    // doubles may not be handled correctly.
    static bool IsOddInteger(float value)
    {
        return std::fmod(value, 2.0) == std::copysign(1.0, value);
    }

    float MathF::Acos(float x)
    {
        return std::acos(x);
    }

    float MathF::Acosh(float x)
    {
        return std::acosh(x);
    }

    float MathF::Asin(float x)
    {
        return std::asin(x);
    }

    float MathF::Asinh(float x)
    {
        return std::asinh(x);
    }

    float MathF::Atan(float x)
    {
        return std::atan(x);
    }

    float MathF::Atan2(float y, float x)
    {
        return std::atan2(y, x);
    }

    float MathF::Atanh(float x)
    {
        return std::atanh(x);
    }

    float MathF::Cbrt(float x)
    {
        return std::cbrt(x);
    }

    float MathF::Ceiling(float x)
    {
        return std::ceil(x);
    }

    float MathF::Cos(float x)
    {
        return std::cos(x);
    }

    float MathF::Cosh(float x)
    {
        return std::cosh(x);
    }

    float MathF::Exp(float x)
    {
        return std::exp(x);
    }

    float MathF::Floor(float x)
    {
        return std::floor(x);
    }

    float MathF::FusedMultiplyAdd(float x, float y, float z)
    {
        return std::fma(x, y, z);
    }

    float MathF::Log(float x)
    {
        return std::log(x);
    }

    float MathF::Log10(float x)
    {
        return std::log10(x);
    }

    float MathF::Log2(float x)
    {
        return std::log2(x);
    }

    float MathF::ModF(float x, float* intptr)
    {
        return std::modf(x, intptr);
    }

    float MathF::Pow(float val, float exp)
    {
        if (std::isnan(val))
            return val;
        if (std::isnan(exp))
            return exp;

        if (val > -1 && val < 1 && exp == -std::numeric_limits<float>::infinity())
            return std::numeric_limits<float>::infinity();

        if (val > -1 && val < 1 && exp == std::numeric_limits<float>::infinity())
            return 0.0;

        if ((val < -1 || val > 1) && exp == -std::numeric_limits<float>::infinity())
            return 0.0;

        if ((val < -1 || val > 1) && exp == std::numeric_limits<float>::infinity())
            return std::numeric_limits<float>::infinity();

        if (val == -std::numeric_limits<float>::infinity())
        {
            if (exp < 0)
                return 0.0;

            if (exp > 0)
            {
                return IsOddInteger(exp) ? -std::numeric_limits<float>::infinity() : std::numeric_limits<float>::infinity();
            }

            return 1.0;
        }

        if (val < 0)
        {
            if (!IsInteger(exp) || exp == std::numeric_limits<float>::infinity() || exp == -std::numeric_limits<float>::infinity())
                return std::numeric_limits<float>::quiet_NaN();
        }

        float res = pow(val, exp);
        if (std::isnan(res))
            return 1.0;

        if (res == -0.0)
            return 0.0;

        return res;
    }

    float MathF::Sin(float x)
    {
        return std::sin(x);
    }

    float MathF::Sinh(float x)
    {
        return std::sinh(x);
    }

    float MathF::Sqrt(float x)
    {
        return std::sqrt(x);
    }

    float MathF::Tan(float x)
    {
        return std::tan(x);
    }

    float MathF::Tanh(float x)
    {
        return std::tanh(x);
    }
} // namespace System
} // namespace CoreLib
} // namespace Private
} // namespace System
} // namespace icalls
} // namespace il2cpp
