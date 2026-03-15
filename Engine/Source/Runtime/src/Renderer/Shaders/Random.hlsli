#ifndef RANDOM_HLSL
#define RANDOM_HLSL

uint pcgHash(uint v)
{
	uint state = v * 747796405u + 2891336453u;
	uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	return (word >> 22u) ^ word;
}

float randFloat(inout uint seed)
{
	seed = pcgHash(seed);
	return float(seed) / float(0xFFFFFFFFu);
}

float2 randFloat2(inout uint seed)
{
	return float2(randFloat(seed), randFloat(seed));
}

#endif // RANDOM_HLSL