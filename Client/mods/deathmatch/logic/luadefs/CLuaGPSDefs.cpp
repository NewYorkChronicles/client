/*****************************************************************************
 *
 *  PROJECT:     New York Chronicles
 *  FILE:        mods/shared_logic/luadefs/CLuaGPSDefs.cpp
 *  PURPOSE:     Lua GPS waypoint definitions class
 *
 *****************************************************************************/

#include "StdInc.h"
#include "CLuaGPSDefs.h"
#include <lua/CLuaFunctionParser.h>

void CLuaGPSDefs::LoadFunctions()
{
    constexpr static const std::pair<const char*, lua_CFunction> functions[]{
        {"setGPSWaypoint", ArgumentParser<SetGPSWaypoint>},
        {"setGPSWaypointToElement", ArgumentParser<SetGPSWaypointToElement>},
        {"clearGPSWaypoint", ArgumentParser<ClearGPSWaypoint>},
        {"isGPSWaypointActive", ArgumentParser<IsGPSWaypointActive>},
        {"getGPSDistance", ArgumentParser<GetGPSDistance>},
        {"getGPSPathNodes", GetGPSPathNodes},
    };

    for (const auto& [name, func] : functions)
        CLuaCFunctions::AddFunction(name, func);
}

int CLuaGPSDefs::SetGPSWaypoint(float fX, float fY, float fZ, std::optional<int> iR, std::optional<int> iG,
                                 std::optional<int> iB, std::optional<int> iA, std::optional<float> fLineWidth)
{
    int r = iR.value_or(255);
    int g = iG.value_or(0);
    int b = iB.value_or(255);
    int a = iA.value_or(200);

    DWORD dwColor = ((DWORD)(a & 0xFF) << 24) | ((DWORD)(r & 0xFF) << 16) | ((DWORD)(g & 0xFF) << 8) | (DWORD)(b & 0xFF);
    float fWidth = fLineWidth.value_or(4.0f);

    return g_pMultiplayer->SetGPSWaypoint(fX, fY, fZ, dwColor, fWidth);
}

int CLuaGPSDefs::SetGPSWaypointToElement(CClientEntity* pEntity, std::optional<int> iR, std::optional<int> iG,
                                          std::optional<int> iB, std::optional<int> iA, std::optional<float> fLineWidth)
{
    CVector vecPos;
    pEntity->GetPosition(vecPos);

    int r = iR.value_or(255);
    int g = iG.value_or(0);
    int b = iB.value_or(255);
    int a = iA.value_or(200);

    DWORD dwColor = ((DWORD)(a & 0xFF) << 24) | ((DWORD)(r & 0xFF) << 16) | ((DWORD)(g & 0xFF) << 8) | (DWORD)(b & 0xFF);
    float fWidth = fLineWidth.value_or(4.0f);

    return g_pMultiplayer->SetGPSWaypoint(vecPos.fX, vecPos.fY, vecPos.fZ, dwColor, fWidth);
}

bool CLuaGPSDefs::ClearGPSWaypoint(std::optional<int> waypointId)
{
    return g_pMultiplayer->ClearGPSWaypoint(waypointId.value_or(0));
}

bool CLuaGPSDefs::IsGPSWaypointActive()
{
    return g_pMultiplayer->IsGPSWaypointActive();
}

std::variant<bool, float> CLuaGPSDefs::GetGPSDistance(std::optional<int> waypointId)
{
    if (!g_pMultiplayer->IsGPSWaypointActive())
        return false;
    if (waypointId.has_value())
        return g_pMultiplayer->GetGPSWaypointDistance(waypointId.value());
    return g_pMultiplayer->GetGPSDistance();
}

int CLuaGPSDefs::GetGPSPathNodes(lua_State* luaVM)
{
    static CVector nodes[2048];
    int count = g_pMultiplayer->GetGPSPathNodes(nodes, 2048);
    if (count <= 0)
    {
        lua_pushboolean(luaVM, false);
        return 1;
    }
    lua_createtable(luaVM, count, 0);
    for (int i = 0; i < count; i++)
    {
        lua_createtable(luaVM, 0, 3);
        lua_pushnumber(luaVM, nodes[i].fX);
        lua_setfield(luaVM, -2, "x");
        lua_pushnumber(luaVM, nodes[i].fY);
        lua_setfield(luaVM, -2, "y");
        lua_pushnumber(luaVM, nodes[i].fZ);
        lua_setfield(luaVM, -2, "z");
        lua_rawseti(luaVM, -2, i + 1);
    }
    return 1;
}
