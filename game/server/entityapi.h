//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef ENTITYAPI_H
#define ENTITYAPI_H

class SendTable;

extern ServerClass* DispatchGetObjectServerClass(edict_t *pent);
extern ServerClass* GetAllServerClasses();
extern void SaveWriteFields( CSaveRestoreData *pSaveData, const char *pname, void *pBaseData, datamap_t *pMap, typedescription_t *pFields, int fieldCount );
extern void SaveReadFields( CSaveRestoreData *pSaveData, const char *pname, void *pBaseData, datamap_t *pMap, typedescription_t *pFields, int fieldCount );
extern CSaveRestoreData *SaveInit( int size  );
extern int CreateEntityTransitionList( CSaveRestoreData *pSaveData, int levelMask );
extern void FreeContainingEntity( edict_t *ed );

class ISaveRestoreBlockHandler;
ISaveRestoreBlockHandler *GetEntitySaveRestoreBlockHandler();

#endif			// ENTITYAPI_H