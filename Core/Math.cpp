#include "pch.h"

#include "Core/Math.h"
#include "PoissonGenerator.h"

using namespace DirectX::SimpleMath;

namespace Gradient::Math
{
    Vector2 FromPoissonPoint(const PoissonGenerator::Point& in)
    {
        return Vector2{ in.x, in.y };
    }

    std::vector<DirectX::SimpleMath::Vector2> GeneratePoissonDiskSamples(
        uint32_t numPoints,
        float diskRadius, 
        float minDistance)
    {
        // PoissonGenerator generates points in a circle centred at 
        // (0.5, 0.5) with radius 0.5. We need to transform in/out of 
        // this space.
        
        PoissonGenerator::DefaultPRNG prng;
        const auto points = PoissonGenerator::generatePoissonPoints(numPoints,
            prng,
            true,
            30U,
            minDistance / diskRadius);

        std::vector<Vector2> out(points.size());
        std::transform(points.begin(), points.end(),
            out.begin(), 
            [diskRadius](const PoissonGenerator::Point& in)
            {
                Vector2 out = FromPoissonPoint(in);
                out -= Vector2{ 0.5, 0.5 };

                out *= diskRadius / 0.5f;
                return out;
            });

        return out;
    }
}