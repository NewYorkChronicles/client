/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto
 *               (Shared logic for modifications)
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/deathmatch/logic/CClientModelManager.h
 *  PURPOSE:     Model manager class
 *
 *****************************************************************************/

class CClientModelManager;

#pragma once

#include <list>
#include <vector>
#include <memory>
#include <mutex>
#include <cstdint>
#include "CClientModel.h"

// FLA may expand these limits — read dynamically from game engine.
// GetBaseIDforTXD() = DFF count (vanilla 20000, FLA 29000)
// GetBaseIDforCOL() = DFF+TXD count (vanilla 25000, FLA 39000)
#define MAX_MODEL_DFF_ID (static_cast<unsigned int>(g_pGame->GetBaseIDforTXD()))
#define MAX_MODEL_TXD_ID (static_cast<unsigned int>(g_pGame->GetBaseIDforCOL()))
#define MAX_MODEL_ID     MAX_MODEL_TXD_ID

class CClientModelManager
{
    friend class CClientModel;

public:
    CClientModelManager::CClientModelManager();
    ~CClientModelManager(void);

    void RemoveAll(void);

    void Add(const std::shared_ptr<CClientModel>& pModel);
    bool TryAdd(const std::shared_ptr<CClientModel>& pModel);
    bool Remove(const std::shared_ptr<CClientModel>& pModel);

    int  GetFirstFreeModelID(void);
    void ReleaseModelID(int iModelID);
    int  GetFreeTxdModelID();

    std::shared_ptr<CClientModel> FindModelByID(int iModelID);
    std::shared_ptr<CClientModel> Request(CClientManager* pManager, int iModelID, eClientModelType eType);

    std::vector<std::shared_ptr<CClientModel>> GetModelsByType(eClientModelType type, const unsigned int minModelID = 0);

    void DeallocateModelsAllocatedByResource(CResource* pResource);

private:
    std::unique_ptr<std::shared_ptr<CClientModel>[]> m_Models;
    unsigned int                                     m_modelCount = 0;
    std::mutex                                       m_idMutex;
    std::vector<std::uint8_t>                        m_reserved;
};
