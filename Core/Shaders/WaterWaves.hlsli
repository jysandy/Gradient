#ifndef __WATER_WAVES_HLSLI__
#define __WATER_WAVES_HLSLI__

struct Wave
{
    float amplitude;
    float wavelength;
    float speed;
    float sharpness;
    float3 direction;
};

float waveHeight(float3 position,
    Wave wave,
    float time)
{
    float w = 2.f / max(wave.wavelength, 0.0001);
    float phi = wave.speed * 2.f / max(wave.wavelength, 0.0001);
    
    float sinTerm = pow((sin(dot(wave.direction, position) * w - time * phi) + 1) / 2.f,
                        wave.sharpness);
    return 2 * wave.amplitude * sinTerm;
}

float ddxWaveHeight(float3 position,
    Wave wave,
    float time)
{
    float w = 2.f / max(wave.wavelength, 0.0001);
    float phi = wave.speed * 2.f / max(wave.wavelength, 0.0001);

    float DoP = dot(wave.direction, position);
    
    float sinTerm = pow((sin(DoP * w - time * phi) + 1) / 2.f,
                        wave.sharpness - 1);
    float cosTerm = cos(DoP * w - time * phi);
    return wave.sharpness * wave.direction.x * w * wave.amplitude * sinTerm * cosTerm;
}

float ddzWaveHeight(float3 position,
    Wave wave,
    float time)
{
    float w = 2.f / max(wave.wavelength, 0.00001);
    float phi = wave.speed * 2.f / max(wave.wavelength, 0.00001);

    float DoP = dot(wave.direction, position);
    
    float sinTerm = pow((sin(DoP * w - time * phi) + 1) / 2.f,
                        wave.sharpness - 1);
    float cosTerm = cos(DoP * w - time * phi);
    return wave.sharpness * wave.direction.z * w * wave.amplitude * sinTerm * cosTerm;
}

#endif