//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "usermessages.h"
#include "shake.h"
#include "voice_gamemgr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void RegisterUserMessages(void)
{
	usermessages->Register("Train", 1);
	usermessages->Register("HudText", -1);
	usermessages->Register("SayText", -1);
	usermessages->Register("SayText2", -1);
	usermessages->Register("TextMsg", -1);
	usermessages->Register("HudMsg", -1);
	usermessages->Register("ResetHUD", 1);		// called every respawn
	usermessages->Register("GameTitle", 0);
	usermessages->Register("ItemPickup", -1);
	usermessages->Register("ShowMenu", -1);
	usermessages->Register("Shake", 13);
	usermessages->Register("Fade", 10);
	usermessages->Register("VGUIMenu", -1);	// Show VGUI menu
	usermessages->Register("Rumble", 3);	// Send a rumble to a controller
	usermessages->Register("Damage", 1);
	usermessages->Register("VoiceMask", VOICE_MAX_PLAYERS_DW * 4 * 2 + 1);
	usermessages->Register("RequestState", 0);
	usermessages->Register("CloseCaption", -1); // Show a caption (by string id number)(duration in 10th of a second)
	usermessages->Register("AmmoDenied", 2);

	// BB2 : 
	usermessages->Register("ShowNote", -1); // Sends the note filename and its header.
	usermessages->Register("ClientEffect", -1); // Client Command Stuff to the client from server.
	usermessages->Register("PlayerInit", -1); // Called when the client connects to a server.
	usermessages->Register("InventoryUpdate", -1); // Called when the client picks up or removes an inventory item.
	usermessages->Register("GameTip", -1); // In-Game Tip, like 'Press E to open the door' kind of tip...
	usermessages->Register("ToolTip", -1); // Tool tips that fade in from the far left. 
	usermessages->Register("CapturePointProgress", -1); // Sends the capture info when capturing points.
	usermessages->Register("DamageTextInfo", -1); // Sends how much damage you dealt to your target.
	usermessages->Register("SkillSoundCue", 1); // Send a client skill sound cue idx to the player in question.
	usermessages->Register("ExperienceTextInfo", 2); // Send how much XP was gained.
}