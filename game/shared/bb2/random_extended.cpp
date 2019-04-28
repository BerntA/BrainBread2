//=========       Copyright © Reperio Studios 2019 @ Bernt Andreas Eide!       ============//
//
// Purpose: New C++11 Randomizers, with proper seeding, and proper statistical distributions, useful for more accurate randomization.
//
//========================================================================================//

#ifdef OSX // Because OSX is AIDS, we won't let OSX have fun.
#include "cbase.h"
#else
#include <random>
static std::random_device rd;
static std::mt19937 g(rd());
#endif

#include "random_extended.h"

bool TryTheLuck(double percent)
{
	if (percent >= 1.0)
		return true;
	else if (percent <= 0.0)
		return false;

#ifdef OSX
	double tmp = round(percent * PERCENT_BASE);
	return ((random->RandomInt(0, 100) <= ((int)tmp)));
#else
	std::bernoulli_distribution d(percent);
	return d(g);
#endif
}

int RandomIntegerNumber(int min, int max)
{
#ifdef OSX
	return random->RandomInt(min, max);
#else
	std::uniform_int_distribution<> d(min, max);
	return d(g);
#endif
}

double RandomDoubleNumber(double min, double max)
{
#ifdef OSX
	return (double)random->RandomFloat((float)min, (float)max);
#else
	std::uniform_real_distribution<> d(min, max);
	return d(g);
#endif
}