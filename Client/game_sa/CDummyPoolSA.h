/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        game_sa/CDummyPoolSA.h
 *  PURPOSE:     Dummy pool class
 *
 *  Multi Theft Auto is available from https://www.multitheftauto.com/
 *
 *****************************************************************************/

#pragma once

#include <vector>
#include <game/CDummyPool.h>
#include "CEntitySA.h"
#include "CPoolSAInterface.h"
#include <memory>

constexpr std::size_t MAX_DUMMIES_DEFAULT = 2500;

class CDummyPoolSA final : public CDummyPool
{
public:
    CDummyPoolSA();
    ~CDummyPoolSA() = default;

    void RemoveAllWithBackup() override;
    void RestoreBackup() override;
    void UpdateBuildingLods(const std::uint32_t offset);

private:
    void UpdateBackupLodOffset(const std::uint32_t offest);
    void UpdateLodsOffestInPool(const std::uint32_t offset);

private:
    CPoolSAInterface<CEntitySAInterface>** m_ppDummyPoolInterface;

    using building_buffer_t = std::uint8_t[sizeof(CEntitySAInterface)];
    using backup_entry_t = std::pair<bool, building_buffer_t>;
    using pool_backup_t = std::vector<backup_entry_t>;
    std::unique_ptr<pool_backup_t> m_pOriginalElementsBackup;
};
