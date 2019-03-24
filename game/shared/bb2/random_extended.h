//=========       Copyright © Reperio Studios 2019 @ Bernt Andreas Eide!       ============//
//
// Purpose: New C++11 Randomizers, with proper seeding, and proper statistical distributions, useful for more accurate randomization.
//
//========================================================================================//

#ifndef RANDOM_EXTENDED_H
#define RANDOM_EXTENDED_H

#ifdef _WIN32
#pragma once
#endif

bool TryTheLuck(double percent); // Returns true if distribution included the percentage, false otherwise.

// Distribute evenly, min->max.
int RandomIntegerNumber(int min, int max);
double RandomDoubleNumber(double min, double max);

#define PERCENT_BASE 100.0

#endif // RANDOM_EXTENDED_H