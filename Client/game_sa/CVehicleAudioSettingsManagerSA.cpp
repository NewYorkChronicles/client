/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        game_sa/CVehicleAudioSettingsEntry.cpp
 *  PURPOSE:     Implementation file for vehicle audio settings manager class
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#include "StdInc.h"
#include "CVehicleAudioSettingsManagerSA.h"
#include <array>

// CAEVehicleAudioEntity::GetVehicleAudioSettings at 0x4F5C10 copies audio settings
// for a given model ID into outBuffer. FLA patches the array base at runtime to support
// custom models. We call this directly for custom models to get FLA's audio data.
// Signature: __stdcall (outBuffer, modelId), retn 8
typedef void(__stdcall* GetVehicleAudioSettings_t)(tVehicleAudioSettings* outBuffer, short modelId);
static const auto GetGameVehicleAudioSettings = reinterpret_cast<GetVehicleAudioSettings_t>(0x4F5C10);

const auto (&ORIGINAL_AUDIO_SETTINGS)[VEHICLES_COUNT] = *reinterpret_cast<const tVehicleAudioSettings (*)[VEHICLES_COUNT]>(0x860AF0);
tVehicleAudioSettings const* pNextVehicleAudioSettings = nullptr;

CVehicleAudioSettingsManagerSA::CVehicleAudioSettingsManagerSA()
{
    ResetAudioSettingsData();
}

std::unique_ptr<CVehicleAudioSettingsEntry> CVehicleAudioSettingsManagerSA::CreateVehicleAudioSettingsData(uint32_t modelId)
{
    auto        settings = std::make_unique<CVehicleAudioSettingsEntrySA>();
    const auto& fromSetting = GetVehicleModelAudioSettingsData(modelId);
    settings->Assign(fromSetting);
    return settings;
}

CVehicleAudioSettingsEntry& CVehicleAudioSettingsManagerSA::GetVehicleModelAudioSettingsData(uint32_t modelId) noexcept
{
    if (modelId >= 400 && modelId <= 611)
        return m_modelEntrys[GetVehicleModelAudioSettingsID(modelId)];

    // Custom/FLA models — call game's function which FLA patches for extended audio
    static CVehicleAudioSettingsEntrySA customEntry;
    tVehicleAudioSettings settings{};
    GetGameVehicleAudioSettings(&settings, static_cast<short>(modelId));
    customEntry.Assign(settings);
    return customEntry;
}

void CVehicleAudioSettingsManagerSA::SetNextSettings(CVehicleAudioSettingsEntry const* pSettings) noexcept
{
    pNextVehicleAudioSettings = &static_cast<CVehicleAudioSettingsEntrySA const*>(pSettings)->GetInterface();
}

void CVehicleAudioSettingsManagerSA::SetNextSettings(uint32_t modelId) noexcept
{
    if (modelId >= 400 && modelId <= 611)
    {
        pNextVehicleAudioSettings = &m_modelEntrys[GetVehicleModelAudioSettingsID(modelId)].GetInterface();
    }
    else
    {
        // Custom/FLA models — call the game's GetVehicleAudioSettings which FLA patches
        // to read from its extended audio array. Store into a static buffer.
        static tVehicleAudioSettings customSettings;
        GetGameVehicleAudioSettings(&customSettings, static_cast<short>(modelId));
        pNextVehicleAudioSettings = &customSettings;
    }
}

void CVehicleAudioSettingsManagerSA::ResetModelSettings(uint32_t modelId) noexcept
{
    size_t index = GetVehicleModelAudioSettingsID(modelId);
    m_modelEntrys[index].Assign(ORIGINAL_AUDIO_SETTINGS[index]);
}

void CVehicleAudioSettingsManagerSA::ResetAudioSettingsData() noexcept
{
    for (size_t i = 0; i < VEHICLES_COUNT; i++)
        m_modelEntrys[i].Assign(ORIGINAL_AUDIO_SETTINGS[i]);
}

void CVehicleAudioSettingsManagerSA::StaticSetHooks() noexcept
{
    // Replace
    // 8D 34 B5 F0 0A 86 00 ; lea esi, _VehicleAudioProperties.m_eVehicleSoundType[esi*4]
    // to
    // 8b 35 XX XX XX XX ; mov esi, [pNextVehicleAudioSettings]
    // 90                ; nop
    MemCpy((void*)0x4F77CA, "\x8B\x35\x00\x00\x00\x00\x90", 7);
    MemPut(0x4F77CA + 2, (std::uint32_t)&pNextVehicleAudioSettings);
}
