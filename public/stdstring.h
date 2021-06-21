//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef STDSTRING_H
#define STDSTRING_H

#if defined( _WIN32 )
#pragma once
#endif

#ifdef _WIN32
#pragma warning(push)
#include <yvals.h>	// warnings get enabled in yvals.h 
#pragma warning(disable:4663)
#pragma warning(disable:4530)
#pragma warning(disable:4245)
#pragma warning(disable:4018)
#pragma warning(disable:4511)
#endif

#include "tier0/valve_minmax_off.h"	// GCC 4.2.2 headers screw up our min/max defs.
#include <string>
#include "tier0/valve_minmax_on.h"	// GCC 4.2.2 headers screw up our min/max defs.

#ifdef _WIN32
#pragma warning(pop)
#endif

#endif // STDSTRING_H
