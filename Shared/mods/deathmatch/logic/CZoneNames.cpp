/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *               (Shared logic for modifications)
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/shared_logic/CZoneNames.cpp
 *
 *****************************************************************************/

#include "StdInc.h"
#include "CZoneNames.h"

static const char* const szUnknownZone = "Unknown";

static const SFixedArray<const char*, 4> cityNameList = {{
    "Portland",
    "Staunton Island",
    "Shoreside Vale",
    "Liberty City",
}};

static const SZone _zoneInfoList[] = {
    {416, -1178, -7, 730, -958, 142, "Portland Beach"},
    {730, -1251, -7, 1166, -1069, 142, "Trenton"},
    {1028, -1069, -12, 1480, -613, 137, "Portland Harbor"},
    {730, -1069, 7, 1028, -742, 157, "Saint Mark's"},
    {410, -908, -15, 730, -463, 135, "Chinatown"},
    {410, -463, -16, 730, -282, 153, "Red Light District"},
    {410, -282, -7, 730, -78, 142, "Hepburn Heights"},
    {730, -512, -8, 1053, -78, 141, "Portland View"},
    {410, -78, -42, 1053, 322, 107, "Harwood"},
    {1054, -613, -23, 1462, 199, 126, "East Bay"},
    {731, -741, -28, 1028, -512, 121, "Saint Mark's"},
    {-95, -411, 6, 279, -61, 169, "Fort Staunton"},
    {-560, -412, 6, -218, 160, 126, "Aspatria"},
    {-135, -1672, -55, 242, -1059, 438, "Torrington"},
    {-559, -1672, -55, -135, -1004, 438, "Bedford Point"},
    {-134, -1059, 5, 280, -412, 204, "Newport"},
    {-456, -1003, -40, -135, -413, 230, "Belleville Park"},
    {-217, -411, 6, -95, -61, 172, "Liberty Campus"},
    {-217, -61, -11, 280, 268, 89, "Rockford"},
    {-1967, -1344, -39, -803, -268, 260, "Francis International Airport"},
    {-1146, -268, -39, -706, 92, 260, "Wichita Gardens"},
    {-1202, 93, -44, -601, 650, 556, "Cedar Grove"},
    {-1742, -267, -43, -1147, 92, 256, "Pike Creek"},
    {-1729, 93, -40, -1202, 704, 259, "Cochrane Dam"},
    {282, -1329, -111, 1567, 434, 488, "Portland"},
    {-600, -1719, -108, 280, 367, 491, "Staunton Island"},
    {-1979, -1351, -111, -601, 1206, 489, "Shoreside Vale"},
};
static const IMPLEMENT_FIXED_ARRAY(SZone, zoneInfoList);

CZoneNames::CZoneNames()
{
    // Initialize zone tree
    for (uint i = 0; i < NUMELMS(zoneInfoList); i++)
    {
        const SZone* pZone = &zoneInfoList[i];
        CVector      vecBottomLeft(pZone->x1, pZone->y1, pZone->z1);
        CVector      vecTopRight(pZone->x2, pZone->y2, pZone->z2);
        m_ZoneTree.Insert(&vecBottomLeft.fX, &vecTopRight.fX, pZone);
    }

    // Initialize city name map
    for (uint i = 0; i < NUMELMS(cityNameList); i++)
        MapInsert(m_CityNameMap, cityNameList[i]);
}

//
// Get zone name at position
//
const char* CZoneNames::GetZoneName(const CVector& vecPosition)
{
    // Check inside map bounds
    if (vecPosition.fX < -3000 || vecPosition.fY < -3000 || vecPosition.fZ < -3000 || vecPosition.fX > 3000 || vecPosition.fY > 3000 || vecPosition.fZ > 3000)
    {
        return szUnknownZone;
    }

    // Use a gradually increasing sphere to get the nearest zone
    float fRadius = 0;
    for (uint i = 0; i < 10; i++)
    {
        const SZone* pZone = GetSmallestZoneInSphere(vecPosition, fRadius);
        if (pZone)
            return pZone->szName;

        fRadius = pow(2.f, (float)i);
    }

    return szUnknownZone;
}

//
// Get best match zone within 2D distance of position
//
const SZone* CZoneNames::GetSmallestZoneInSphere(const CVector& vecPosition, float fRadius)
{
    CBox box(vecPosition, fRadius);
    box.vecMin.fZ = vecPosition.fZ;
    box.vecMax.fZ = vecPosition.fZ;

    std::vector<const SZone*> results;
    m_ZoneTree.Search(&box.vecMin.fX, &box.vecMax.fX, results);

    float        fSmallestSize = 0;
    const SZone* pSmallestZone = NULL;
    for (uint i = 0; i < results.size(); i++)
    {
        const SZone* pZone = results[i];
        float        sizeX = static_cast<float>(pZone->x2 - pZone->x1);
        float        sizeY = static_cast<float>(pZone->y2 - pZone->y1);
        float        sizeZ = static_cast<float>(pZone->z2 - pZone->z1);
        float        fZoneSize = sizeX * sizeY * sizeZ;

        if (!pSmallestZone || fZoneSize < fSmallestSize)
        {
            fSmallestSize = fZoneSize;
            pSmallestZone = pZone;
        }
    }
    return pSmallestZone;
}

//
// Get city name at position
//
const char* CZoneNames::GetCityName(const CVector& vecPosition)
{
    // Check inside map bounds
    if (vecPosition.fX < -3000 || vecPosition.fY < -3000 || vecPosition.fZ < -3000 || vecPosition.fX > 3000 || vecPosition.fY > 3000 || vecPosition.fZ > 3000)
    {
        return szUnknownZone;
    }

    // Use a gradually increasing sphere to get the nearest city zone
    float fRadius = 0;
    for (uint i = 0; i < 10; i++)
    {
        const SZone* pZone = GetCityZoneInSphere(vecPosition, fRadius);
        if (pZone)
            return pZone->szName;

        fRadius = pow(2.f, (float)i);
    }

    return szUnknownZone;
}

//
// Get city zone within 2D distance of position
//
const SZone* CZoneNames::GetCityZoneInSphere(const CVector& vecPosition, float fRadius)
{
    CBox box(vecPosition, fRadius);
    box.vecMin.fZ = vecPosition.fZ;
    box.vecMax.fZ = vecPosition.fZ;

    std::vector<const SZone*> results;
    m_ZoneTree.Search(&box.vecMin.fX, &box.vecMax.fX, results);

    for (uint i = 0; i < results.size(); i++)
    {
        const SZone* pZone = results[i];
        if (MapContains(m_CityNameMap, pZone->szName))
            return pZone;
    }

    return NULL;
}
