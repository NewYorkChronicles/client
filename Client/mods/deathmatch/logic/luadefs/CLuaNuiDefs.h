/*****************************************************************************
 *
 *  PROJECT:     New York Chronicles Client
 *  FILE:        mods/deathmatch/logic/luadefs/CLuaNuiDefs.h
 *
 *****************************************************************************/
#pragma once
#include "CLuaDefs.h"

class CLuaNuiDefs : public CLuaDefs
{
public:
    static void LoadFunctions();

    LUA_DECLARE(ShowNuiFrame);
    LUA_DECLARE(HideNuiFrame);
    LUA_DECLARE(SendNuiMessage);
    LUA_DECLARE(RegisterNuiCallback);
    LUA_DECLARE(SetNuiFocus);
    LUA_DECLARE(IsNuiFocused);
    LUA_DECLARE(ToggleNuiDevTools);

private:
    static SString CallerResource(lua_State* L);
};
