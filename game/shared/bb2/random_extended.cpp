//=========       Copyright © Reperio Studios 2019 @ Bernt Andreas Eide!       ============//
//
// Purpose: New C++11 Randomizers, with proper seeding, and proper statistical distributions, useful for more accurate randomization.
//
//========================================================================================//

#include "random_extended.h"
#include <random>

static std::random_device rd;
static std::mt19937 g(rd());

bool TryTheLuck(double percent)
{
	if (percent >= 1.0)
		return true;
	else if (percent <= 0.0)
		return false;

	std::bernoulli_distribution d(percent);
	return d(g);
}

int RandomIntegerNumber(int min, int max)
{
	std::uniform_int_distribution<> d(min, max);
	return d(g);
}

double RandomDoubleNumber(double min, double max)
{
	std::uniform_real_distribution<> d(min, max);
	return d(g);
}