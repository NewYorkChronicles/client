/*****************************************************************************
 *
 *  PROJECT:     New York Chronicles
 *  FILE:        mods/shared_logic/luadefs/CLuaGPSDefs.h
 *  PURPOSE:     Lua GPS waypoint definitions class header
 *
 *****************************************************************************/

#pragma once
#include "CLuaDefs.h"

class CLuaGPSDefs : public CLuaDefs
{
public:
    static void LoadFunctions();

private:
    static int  SetGPSWaypoint(float fX, float fY, float fZ, std::optional<int> iR, std::optional<int> iG,
                               std::optional<int> iB, std::optional<int> iA, std::optional<float> fLineWidth);
    static int  SetGPSWaypointToElement(CClientEntity* pEntity, std::optional<int> iR, std::optional<int> iG,
                                        std::optional<int> iB, std::optional<int> iA, std::optional<float> fLineWidth);
    static bool ClearGPSWaypoint(std::optional<int> waypointId);
    static bool IsGPSWaypointActive();
    static std::variant<bool, float> GetGPSDistance(std::optional<int> waypointId);
    static int GetGPSPathNodes(lua_State* luaVM);
};
