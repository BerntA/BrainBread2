//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: BrainBread 2 Achievement & Stat Handler - Shared Definitions!
//
//========================================================================================//

#ifndef ACHIEVEMENTS_SHAREDDEFS_H
#define ACHIEVEMENTS_SHAREDDEFS_H

#ifdef _WIN32
#pragma once
#endif

enum AchievementTypes
{
	ACHIEVEMENT_TYPE_DEFAULT = 0,
	ACHIEVEMENT_TYPE_MAP,
	ACHIEVEMENT_TYPE_REWARD,
};

struct achievementStatItem_t
{
	const char *szAchievement;
	const char *szStat;
	int maxValue;
	int type;
	int rewardValue;
	bool hidden;
};

struct globalStatItem_t
{
	const char *szStat;
	int defaultValue;
};

#endif // ACHIEVEMENTS_SHAREDDEFS_H